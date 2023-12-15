#include "SenderUDP2.h"
#include "Engine.h"
#include "AppTicker.h"

namespace app {
const u32 GMAX_UDP_SIZE = 1500;

u32 SenderUDP2::mSN = 0;
u32 SenderUDP2::mID = 0x00000000U;

SenderUDP2::SenderUDP2() :
    mTimeExt(0), mMaxMsg(1), mTLS(true), mMemPool(nullptr), mLoop(&Engine::getInstance().getLoop()),
    mPack(GMAX_UDP_SIZE) {
}


SenderUDP2::~SenderUDP2() {
    if (mMemPool) {
        mProto.clear();
        MemPool::releaseMemPool(mMemPool);
        mMemPool = nullptr;
    }
}


s32 SenderUDP2::sendKcpRaw(const void* buf, s32 len, void* user) {
    DASSERT(user);
    SenderUDP2& udplus = *(SenderUDP2*)user;
    return udplus.sendRawMsg(buf, len);
}


void SenderUDP2::setMaxMsg(s32 cnt) {
    mMaxMsg = cnt > 0 ? cnt : 1;
}


s32 SenderUDP2::open(const String& addr, bool tls) {
    mTimeExt = 0;
    mTLS = tls;
    mUDP.setClose(EHT_UDP, SenderUDP2::funcOnClose, this);
    mUDP.setTime(SenderUDP2::funcOnTime, 100, 20, -1);
    if (!mMemPool) {
        mMemPool = MemPool::createMemPool(10 * 1024 * 1024);
    }
    mProto.init(sendKcpRaw, mMemPool, ++mID, this);
    RequestUDP* it = RequestUDP::newRequest(GMAX_UDP_SIZE);
    it->mUser = this;
    it->mCall = funcOnRead;
    s32 ret = mUDP.open(it, addr.c_str(), nullptr, 2);
    if (EE_OK != ret) {
        RequestUDP::delRequest(it);
        return ret;
    }

    sendMsgs(1);
    mProto.flush();
    return ret;
}


s32 SenderUDP2::onTimeout(HandleTime& it) {
    DASSERT(mUDP.getGrabCount() > 0);
    u32 val = (u32)mLoop->getTime();
    mProto.update(val);
    if (EE_OK != onReadKCP()) {
        if (mTimeExt > 0) {
            val -= mTimeExt;
            if (val > 1000 * 15) {
                return EE_ERROR; // close mUDP;
            }
            if (val >= 1000 * 10 && 0 == mMaxMsg) {
                s32 step = mProto.sendKCP(nullptr, 0); // assert(0==step);
                if (0 != step) {
                    Logger::log(ELL_INFO, "SenderUDP2::onTimeout>> tick step=%d", step);
                }
            }
        } else {
            mTimeExt = val;
        }
    }
    return EE_OK;
}


void SenderUDP2::onClose(Handle* it) {
    DASSERT(mMemPool && (&mUDP == it) && "SenderUDP2::onClose tls handle?");
    if (mMemPool) {
        mProto.clear();
        MemPool::releaseMemPool(mMemPool);
        mMemPool = nullptr;
    }
    // delete this;
}


void SenderUDP2::onWrite(RequestUDP* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "SenderUDP2::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    } else {
        mTimeExt = 0;
    }
    RequestUDP::delRequest(it);
}


void SenderUDP2::onRead(RequestUDP* it) {
    s32 err = it->mError;
    if (it->mUsed > 0) {
        mTimeExt = 0;
        StringView dat = it->getReadBuf();
        err = mProto.importRaw(dat.mData, dat.mLen);
    }
    if (EE_OK == it->mError) {
        it->mUsed = 0;
        if (EE_OK != readIF(it)) {
            RequestUDP::delRequest(it);
            Logger::log(ELL_ERROR, "SenderUDP2::onRead>>post read ecode=%d", it->mError);
            return;
        }
        onReadKCP();
    } else {
        Logger::log(ELL_ERROR, "SenderUDP2::onRead>>read=%u, ecode=%d", it->mUsed, err);
        RequestUDP::delRequest(it);
    }
}


s32 SenderUDP2::onReadKCP() {
    s32 rdsz = mProto.receiveKCP(mPack.getWritePointer(), mPack.getWriteSize());
    while (EE_INVALID_PARAM == rdsz && mPack.capacity() < mProto.getMaxSupportedMsgSize()) {
        mPack.reallocate(mPack.capacity() + GMAX_UDP_SIZE);
        rdsz = mProto.receiveKCP(mPack.getWritePointer(), mPack.getWriteSize());
    }
    if (rdsz < 0) {
        if (EE_RETRY != rdsz) {
            Logger::log(ELL_INFO, "SenderUDP2::onReadKCP>> ecode=%d", rdsz);
        }
        rdsz = 0;
    }
    s32 send = 0;
    mPack.resize(mPack.size() + rdsz);
    AppTicker::gTotalSizeIn += rdsz;
    MsgHeader* head = (MsgHeader*)mPack.getPointer();
    usz got = mPack.size();
    while (got >= sizeof(MsgHeader::mSize) && head->mSize <= got) {
        switch (head->mType) {
        case net::EPT_ACTIVE:
            break;
        case net::EPT_ACTIVE_RESP:
            ++AppTicker::gTotalActiveResp;
            break;
        case net::EPT_SUBMIT:
            ++AppTicker::gTotalPacketIn;
            break;
        case net::EPT_SUBMIT_RESP:
            break;
        } // switch
        got -= head->mSize;
        head = (MsgHeader*)((s8*)head + head->mSize);
        send |= 1;
    } // while
    mPack.clear(mPack.size() - got);

    return send > 0 ? sendMsgs(1) : EE_RETRY;
}


s32 SenderUDP2::sendMsgs(s32 max) {
    static s8 buf[sizeof(net::PackSubmit) + 2048];
    static s32 sentsz = 0;
    DASSERT(sizeof(buf) <= mProto.getMaxSupportedMsgSize());
    net::PackSubmit& head = *(net::PackSubmit*)buf;
    s32 step;
    s32 ret = EE_RETRY;
    for (s32 i = 0; mMaxMsg > 0 && i < max; ++i) {
        if (0 == sentsz) {
            head.clear();
            net::PackSubmit::Item& val = head.getNode(head.EI_VALUE);
            val.mOffset = head.mSize;
            head.mSN = ++mSN;
            val.mSize = 1 + snprintf(buf + head.mSize, sizeof(buf) - head.mSize, "packet sn=%u", head.mSN);
            head.mSize += val.mSize;
            head.mSize = sizeof(buf); // just for test
        }
        //@note mProto is not stream mode
        step = mProto.sendKCP((const s8*)&head + sentsz, (s32)head.mSize - sentsz);
        if (step < 0) {
            Logger::log(ELL_ERROR, "SenderUDP2::sendMsgs>>retry step=%d", step);
            break;
        } else if (0 == step) {
            Logger::log(ELL_INFO, "SenderUDP2::sendMsgs>>step=%d", step);
            break; // need retry
        } else {
            ret = EE_OK;
            sentsz += step;
            if (sentsz == head.mSize) {
                sentsz = 0;
                AppTicker::gTotalSizeOut += head.mSize;
                ++AppTicker::gTotalPacketOut;
                --mMaxMsg;
            }
        }
    }
    return ret;
}


s32 SenderUDP2::sendRawMsg(const void* buf, s32 len) {
    if (mUDP.isClosing()) {
        Logger::log(ELL_ERROR, "SenderUDP2::sendRawMsg>>send on closed udp");
        return EE_ERROR;
    }
    RequestUDP* out = RequestUDP::newRequest(len);
    memcpy(out->mData, buf, len);
    out->mUsed = len;
    out->mUser = this;
    out->mCall = SenderUDP2::funcOnWrite;
    if (0 != writeIF(out)) {
        RequestUDP::delRequest(out);
        Logger::log(ELL_ERROR, "SenderUDP2::sendRawMsg>>ecode=%d", out->mError);
        return EE_ERROR;
    }
    return EE_OK;
}

} // namespace app
