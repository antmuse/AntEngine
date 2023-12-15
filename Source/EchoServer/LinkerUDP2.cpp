#include "LinkerUDP2.h"
#include "Engine.h"
#include "AppTicker.h"
#include "Timer.h"

namespace app {
namespace net {

const u32 GMAX_UDP_SIZE = 1500;

u32 LinkerUDP2::mSN = 0;

LinkerUDP2::LinkerUDP2() :
    mTimeExt(0), mMemPool(nullptr), mSev(nullptr), mLoop(&Engine::getInstance().getLoop()), mPack(GMAX_UDP_SIZE) {
}


LinkerUDP2::~LinkerUDP2() {
    mProto.clear();
}

bool LinkerUDP2::lessTime(const Node3* va, const Node3* vb) {
    LinkerUDP2& tva = *DGET_HOLDER(va, LinkerUDP2, mNode);
    LinkerUDP2& tvb = *DGET_HOLDER(vb, LinkerUDP2, mNode);
    return (tva.mProto.getNextFlushTime() < tvb.mProto.getNextFlushTime());
}

LinkerUDP2* LinkerUDP2::convNode(const Node3* val) {
    LinkerUDP2* ret = DGET_HOLDER(val, LinkerUDP2, mNode);
    return ret;
}

s32 LinkerUDP2::sendKcpRaw(const void* buf, s32 len, void* user) {
    DASSERT(user);
    LinkerUDP2& udplus = *(LinkerUDP2*)user;
    return udplus.sendRawMsg(buf, len);
}


s32 LinkerUDP2::sendRawMsg(const void* buf, s32 len) {
    return mSev->sendRawMsg(*this, buf, len);
}


s32 LinkerUDP2::onLink(u32 id, NetServerUDP2* sev, RequestUDP* sit) {
    mTimeExt = 0;
    mSev = sev;
    mMemPool = sev->getMemPool();
    mRemote = sit->mRemote;
    return mProto.init(LinkerUDP2::sendKcpRaw, mMemPool, id, this) ? EE_OK : EE_ERROR;
}

s32 LinkerUDP2::onTimeout(u32 val) {
    mProto.update(val);
    s32 rdsz = mProto.receiveKCP(mPack.getWritePointer(), mPack.getWriteSize());
    while (EE_INVALID_PARAM == rdsz && mPack.capacity() < mProto.getMaxSupportedMsgSize()) {
        mPack.reallocate(mPack.capacity() + GMAX_UDP_SIZE);
        rdsz = mProto.receiveKCP(mPack.getWritePointer(), mPack.getWriteSize());
    }
    if (rdsz < 0) {
        if (EE_RETRY == rdsz) {
            if (0 != mTimeExt) {
                s32 diff = val - mTimeExt;
                if (diff > 15000 || diff < 15000) {
                    mSev->closeNode(this);
                    return EE_ERROR;
                }
            } else {
                mTimeExt = val;
            }
        }
        if (0 == mPack.size()) {
            return EE_OK;
        }
        rdsz = 0;
    }
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

        if (EE_OK != sendMsg(head, head->mSize)) {
            break;
        }

        switch (head->mType) {
        case net::EPT_ACTIVE:
            break;
        case net::EPT_ACTIVE_RESP:
            ++AppTicker::gTotalActive;
            break;
        case net::EPT_SUBMIT:
            ++AppTicker::gTotalPacketOut;
            AppTicker::gTotalSizeOut += head->mSize;
            break;
        case net::EPT_SUBMIT_RESP:
            break;
        } // switch
        got -= head->mSize;
        head = (MsgHeader*)((s8*)head + head->mSize);
    } // while
    mPack.clear(mPack.size() - got);
    return EE_OK;
}

void LinkerUDP2::onClose(Handle* it) {
    mProto.clear();
    Logger::log(ELL_INFO, "LinkerUDP2::onClose>>grab cnt=%d", it->getGrabCount());
    mSev->closeNode(this);
}


void LinkerUDP2::onWrite(RequestUDP* it) {
    if (EE_OK == it->mError) {
        mTimeExt = 0;
        //++AppTicker::gTotalActive;
        //++AppTicker::gTotalPacketOut;
        AppTicker::gTotalSizeOut += it->mUsed;
        mProto.update((s32)mLoop->getTime());
    } else {
        Logger::log(ELL_ERROR, "LinkerUDP2::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestUDP::delRequest(it);
}


void LinkerUDP2::onRead(RequestUDP* it) {
    s32 err = it->mError;
    if (it->mUsed > 0) {
        mTimeExt = 0;
        StringView dat = it->getReadBuf();
        s32 ecode = mProto.importRaw(dat.mData, dat.mLen);
        if (ecode < 0) {
            err = EE_ERROR;
        }
    }
    if (EE_OK != err) {
        Logger::log(ELL_ERROR, "LinkerUDP2::onRead>>read=%u, ecode=%d", it->mUsed, it->mError);
        return;
    }

    onTimeout((u32)mLoop->getTime());
}


s32 LinkerUDP2::sendMsg(const void* buf, s32 len) {
    //@note mProto is not stream mode
    s32 ret = mProto.sendKCP((const s8*)buf, len);
    if (EE_RETRY == ret) {
        return EE_RETRY;
    }
    return ret == len ? EE_OK : EE_ERROR;
}



//----------------------------------------------------------------------------------------------------
// NetServerUDP2
//----------------------------------------------------------------------------------------------------
NetServerUDP2::NetServerUDP2(bool tls) : mTLS(tls), mSN(0), mTimeHub(LinkerUDP2::lessTime) {
    mMemPool = MemPool::createMemPool(10 * 1024 * 1024);
}

NetServerUDP2::~NetServerUDP2() {
    if (mMemPool) {
        MemPool::releaseMemPool(mMemPool);
        mMemPool = nullptr;
    }
}

s32 NetServerUDP2::open(const String& addr) {
    mUDP.setClose(EHT_UDP, NetServerUDP2::funcOnClose, this);
    mUDP.setTime(NetServerUDP2::funcOnTime, 100, 50, -1);
    mUDP.setLocal(addr);

    RequestUDP* it = RequestUDP::newRequest(GMAX_UDP_SIZE);
    it->mUsed = 0;
    it->mUser = this;
    it->mCall = NetServerUDP2::funcOnRead;
    it->mRemote.setAddrSize(mUDP.getLocal().getAddrSize());
    s32 ret = mUDP.open(it, &mUDP.getLocal(), &mUDP.getLocal(), 1);
    if (EE_OK != ret) {
        RequestUDP::delRequest(it);
    }
    return ret;
}

void NetServerUDP2::closeNode(LinkerUDP2* nd) {
    if (nd) {
        u32 id = nd->getID();
        TMap<u32, LinkerUDP2*>::Node* sid = mClients.find(id);
        if (sid) {
            Logger::log(ELL_INFO, "NetServerUDP2::closeNode>>close id=%u, remote=%s, cnt=%lu/%lu", id,
                nd->getRemote().getStr(), mClients.size(), mTimeHub.getSize());
            mTimeHub.remove(&sid->getValue()->getTimeNode());
            s32 dret = sid->getValue()->drop();
            mClients.remove(sid);
        } else {
            Logger::log(
                ELL_ERROR, "NetServerUDP2::closeNode>>not close id=%u, remote=%s", id, nd->getRemote().getStr());
        }
    } else {
        Logger::log(ELL_ERROR, "NetServerUDP2::closeNode>>node=null");
    }
}

void NetServerUDP2::onRead(RequestUDP* it) {
    if (it->mUsed >= 24) { // 24=IKCP_OVERHEAD
        u32 id = KCProtocal::getConv(it->getBuf());
        it->mRemote.reverse();
        TMap<u32, LinkerUDP2*>::Node* nd = mClients.find(id);
        bool create1 = false;
        if (nd) {
            nd->getValue()->onRead(it);
        } else {
            create1 = true;
        }
        if (create1) { // TODO: use password for new client
            net::LinkerUDP2* con = new net::LinkerUDP2();
            if (EE_OK == con->onLink(id, this, it)) {
                mClients.insert(id, con);
                mTimeHub.insert(&con->getTimeNode());
                con->onRead(it);
                Logger::log(ELL_INFO, "NetServerUDP2::onRead>>new id=%u, remote=%s, cnt=%lu/%lu", id,
                    it->mRemote.getStr(), mClients.size(), mTimeHub.getSize());
            } else {
                con->drop();
                mClients.remove(id);
                Logger::log(ELL_ERROR, "NetServerUDP2::onRead>>fail new id=%u, remote=%s, cnt=x/%lu = %lu", id,
                    it->mRemote.getStr(), mClients.size(), mTimeHub.getSize());
            }
        }
    } else {
        Logger::log(ELL_ERROR, "NetServerUDP2::onRead>>size=%u, remote=%s, ecode=%d", it->mUsed, it->mRemote.getStr(),
            it->mError);
    }
    it->mUsed = 0;
    if (EE_OK != mUDP.readFrom(it)) {
        Logger::log(ELL_ERROR, "NetServerUDP2::onRead>>size=%u, ecode=%d", it->mUsed, it->mError);
        RequestUDP::delRequest(it);
    }
}

void NetServerUDP2::onWrite(RequestUDP* it) {
    RequestUDP::delRequest(it);
}

s32 NetServerUDP2::onTimeout(HandleTime& it) {
    DASSERT(mUDP.getGrabCount() > 0);

    LinkerUDP2* val;
    u32 curr;
    u32 tim = (u32)Engine::getInstance().getLoop().getTime();
    for (Node3* nd = mTimeHub.getTop(); nd; nd = mTimeHub.getTop()) {
        val = LinkerUDP2::convNode(nd);
        DASSERT(val);
        curr = val->getProto().getNextFlushTime();
        if (curr > tim) {
            break;
        }
        if (EE_OK == val->onTimeout(tim)) { // relink
            mTimeHub.remove(&val->getTimeNode());
            mTimeHub.insert(&val->getTimeNode());
        } else {
            // have removed by it self
        }
    }
    return EE_OK;
}

void NetServerUDP2::onClose(Handle* it) {
    DASSERT(&mUDP == it && "NetServerUDP2::onClose handle");
    Logger::log(ELL_INFO, "NetServerUDP2::onClose>>grab cnt=%d", it->getGrabCount());
    for (TMap<u32, LinkerUDP2*>::Iterator i = mClients.getIterator(); !i.atEnd(); i++) {
        s32 cnt = (*i).getValue()->drop();
        DASSERT(0 == cnt);
    }
    mClients.clear();
    drop();
}


s32 NetServerUDP2::sendRawMsg(LinkerUDP2& nd, const void* buf, s32 len) {
    if (mUDP.isClosing()) {
        Logger::log(ELL_ERROR, "LinkerUDP2::sendRawMsg>>send on closed udp");
        return EE_ERROR;
    }
    RequestUDP* out = RequestUDP::newRequest(len);
    memcpy(out->mData, buf, len);
    out->mUsed = len;
    out->mUser = this;
    out->mCall = NetServerUDP2::funcOnWrite;
    out->mRemote = nd.getRemote();
    if (0 != mUDP.writeTo(out)) {
        RequestUDP::delRequest(out);
        Logger::log(ELL_ERROR, "LinkerUDP2::sendRawMsg>>ecode=%d", out->mError);
        return EE_ERROR;
    }
    return EE_OK;
}

} // namespace net
} // namespace app
