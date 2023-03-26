#include "SenderTCP.h"
#include "Engine.h"
#include "AppTicker.h"

namespace app {

u32 SenderTCP::mSN = 0;

SenderTCP::SenderTCP() :
    mMaxMsg(1),
    mTLS(true),
    mLoop(&Engine::getInstance().getLoop()),
    mPack(1024) {
}


SenderTCP::~SenderTCP() {
}


void SenderTCP::setMaxMsg(s32 cnt) {
    mMaxMsg = cnt > 0 ? cnt : 1;
}


s32 SenderTCP::open(const String& addr, bool tls) {
    mTLS = tls;
    if (mTLS) {
        mTCP.setClose(EHT_TCP_CONNECT, SenderTCP::funcOnClose, this);
        mTCP.setTime(SenderTCP::funcOnTime, 15 * 1000, 20 * 1000, -1);
    } else {
        mTCP.getHandleTCP().setClose(EHT_TCP_CONNECT, SenderTCP::funcOnClose, this);
        mTCP.getHandleTCP().setTime(SenderTCP::funcOnTime, 15 * 1000, 20 * 1000, -1);
    }

    RequestFD* it = RequestFD::newRequest(4 * 1024);
    it->mUser = this;
    it->mCall = funcOnConnect;
    s32 ret = mTLS ? mTCP.open(addr, it) : mTCP.getHandleTCP().open(addr, it);
    if (EE_OK != ret) {
        RequestFD::delRequest(it);
    }
    return ret;
}


s32 SenderTCP::onTimeout(HandleTime& it) {
    DASSERT(mTCP.getGrabCount() > 0);

    RequestFD* tick = RequestFD::newRequest(sizeof(net::PackActive));
    tick->mUser = this;
    tick->mCall = SenderTCP::funcOnWrite;
    net::PackActive* head = (net::PackActive*)(tick->getWriteBuf().mData);
    head->pack(++mSN);
    tick->mUsed = head->mSize;
    if (0 != writeIF(tick)) {
        RequestFD::delRequest(tick);
        //@note: 此时,不管是否调用close handle，皆必须返回EE_ERROR
        return EE_ERROR;
    }

    return EE_OK;
}


void SenderTCP::onClose(Handle* it) {
    if (mTLS) {
        DASSERT(&mTCP == it && "SenderTCP::onClose tls handle?");
    } else {
        DASSERT(&mTCP.getHandleTCP() == it && "SenderTCP::onClose tcp handle?");
    }
    //delete this;
}


void SenderTCP::onWrite(RequestFD* it) {
    if (0 == it->mError) {
        StringView dat = it->getReadBuf();
        MsgHeader* head = (MsgHeader*)dat.mData;
        switch (head->mType) {
        case net::EPT_ACTIVE:
            ++AppTicker::gTotalActive;
            break;
        case net::EPT_ACTIVE_RESP:
            break;
        case net::EPT_SUBMIT:
            AppTicker::gTotalSizeOut += it->mUsed;
            ++AppTicker::gTotalPacketOut;
            break;
        }
        //printf("SenderTCP::onWrite>>success = %u\n", it->mUsed);
    } else {
        Logger::log(ELL_ERROR, "SenderTCP::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestFD::delRequest(it);
    //mTCP.getSock().closeSend();
    //mTCP.getSock().closeReceive();
}


void SenderTCP::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        StringView dat = it->getReadBuf();
        usz got = mPack.write(dat.mData, dat.mLen);
        AppTicker::gTotalSizeIn += dat.mLen;
        it->mUsed = 0;
        if (EE_OK != readIF(it)) {
            RequestFD::delRequest(it);
            return;
        }

        MsgHeader* head = (MsgHeader*)mPack.getPointer();
        while (got >= sizeof(MsgHeader::mSize) && head->mSize <= got) {
            switch (head->mType) {
            case net::EPT_ACTIVE:
                break;
            case net::EPT_ACTIVE_RESP:
                ++AppTicker::gTotalActiveResp;
                break;
            case net::EPT_SUBMIT:
                ++AppTicker::gTotalPacketIn;
                if (mMaxMsg > 0) {
                    --mMaxMsg;
                    RequestFD* out = RequestFD::newRequest(head->mSize);
                    out->mUser = this;
                    out->mCall = SenderTCP::funcOnWrite;
                    dat = out->getWriteBuf();
                    memcpy(dat.mData, head, head->mSize);
                    ((MsgHeader*)dat.mData)->mSN = ++mSN;
                    u32 sz;
                    s8* val = head->readItem(net::PackSubmit::EI_VALUE, sz);
                    out->mUsed = head->mSize;
                    if (0 != writeIF(out)) {
                        RequestFD::delRequest(out);
                        return;
                    }
                }
                break;
            case net::EPT_SUBMIT_RESP:
                break;
            }//switch
            got -= head->mSize;
            head = (MsgHeader*)((s8*)head + head->mSize);
        }//while
        mPack.clear(mPack.size() - got);
        //printf("SenderTCP::onRead>>%.*s\n", (s32)dat.mLen, dat.mData);
        return;
    } else {
        //stop read
        Logger::log(ELL_INFO, "SenderTCP::onRead>>read 0, ecode=%d", it->mError);
        RequestFD::delRequest(it);
    }
}


void SenderTCP::onConnect(RequestFD* it) {
    if (0 == it->mError) {
        //printf("SenderTCP::onConnect>>success\n");
        it->mCall = SenderTCP::funcOnRead;
        if (0 != readIF(it)) {
            RequestFD::delRequest(it);
            return;
        }

        /*for (s32 i = 0; i < 2; ++i) {
            RequestFD* read = RequestFD::newRequest(4*1024);
            read->mUser = this;
            read->mCall = SenderTCP::funcOnRead;
            if (0 != readIF(read)) {
                RequestFD::delRequest(read);
                return;
            }
        }*/

        for (s32 i = 0; mMaxMsg > 0 && i < 2; ++i) {
            RequestFD* out = RequestFD::newRequest(1024);
            StringView buf = out->getWriteBuf();
            net::PackSubmit& head = *(net::PackSubmit*)buf.mData;
            head.clear();
            net::PackSubmit::Item& val = head.getNode(head.EI_VALUE);
            val.mOffset = head.mSize;
            val.mSize = 1 + snprintf(buf.mData + head.mSize, buf.mLen - head.mSize,
                "%s", "GET / HTTP/1.1\r\nHost: 10.1.63.128\r\nConnection : keep-alive\r\n\r\n");
            head.mSize += val.mSize;
            head.mSN = ++mSN;
            head.mSize = 1024; //just for test

            out->mUsed = head.mSize;
            out->mCall = SenderTCP::funcOnWrite;
            if (0 == writeIF(out)) {
                --mMaxMsg;
            } else {
                RequestFD::delRequest(out);
                break;
            }
        }
    } else {
        Logger::log(ELL_ERROR, "SenderTCP::onConnect>>ecode=%d", it->mError);
        RequestFD::delRequest(it);
    }
}


} //namespace app
