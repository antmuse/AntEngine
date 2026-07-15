#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpParserDef.h"
#include "Net/HTTP/Website.h"
#include "Net/Acceptor.h"
#include "Loop.h"
#include "Timer.h"

namespace app {
namespace net {


HttpLayer::HttpLayer() {
}


HttpLayer::~HttpLayer() {
    clear();
}


RequestFD* HttpLayer::createMem(usz len) {
    RequestFD* it = reinterpret_cast<RequestFD*>(mPool->allocate(sizeof(RequestFD) + len));
    new ((void*)it) RequestFD();
    it->mAllocated = len;
    it->mData = (s8*)(it + 1);
    // it->mUsed = 0;
    return it;
}


void HttpLayer::deleteMem(RequestFD* it) {
    mPool->release(it);
}


s32 HttpLayer::launch(HttpMsg* msg) {
    if (!mPool) {
        mPool = MemPool::createMemPool(16 * 1024);
    }
    if (mMsgReq) {
        return EE_ERROR;
    }
    mParser = new Http1Parser(this, EHTTP_RESPONSE);
    bool vhttps = msg->getURL().isHttps();
    String host = msg->getURL().getHost();
    if (vhttps) {
        if (!mConnTLS) {
            mConnTLS = new HandleTLS();
        }
        mConnTLS->setHost(host);
        mConnTLS->setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mConnTLS->setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    } else {
        if (!mConnTCP) {
            mConnTCP = new HandleTCP();
        }
        mConnTCP->setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mConnTCP->setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    }
    NetAddress addr(msg->getURL().getPort());
    // s8 shost[256]; // 255, Maximum host name defined in RFC 1035
    // snprintf(shost, sizeof(shost), "%.*s", (s32)(host.mLen), host.mData);
    addr.setDomain(host.data());

    RequestFD* nd = createMem(4 * 1024);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnConnect;
    s32 ret = mConnTLS ? mConnTLS->open(addr, nd) : mConnTCP->open(addr, nd);
    if (EE_OK != ret) {
        deleteMem(nd);
        clear();
        return ret;
    }
    mMsgReq = msg;
    msg->grab();
    grab();
    return EE_OK;
}


bool HttpLayer::sendReq(RequestFD* nd) {
    nd->mUser = mMsgReq;
    nd->mCall = HttpLayer::funcOnWrite;
    s32 ret = writeIF(nd);
    if (EE_OK != ret) {
        deleteMem(nd);
        return false;
    }
    mMsgReq->grab();
    return true;
}


s32 HttpLayer::sendOut(HttpMsg* msg) {
    bool chk = msg->getHead().isChunked();
    RequestFD* it = createMem(msg->sumCacheSize());
    msg->dumpLine(it);
    msg->dumpHead(it);
    msg->dumpBody(it);

    // it->mUser = this;
    it->mUser = msg;
    it->mCall = HttpLayer::funcOnWrite;
    // Packet& pack = msg->getBody();
    // it->mAllocated = pack.capacity();
    // it->mData = pack.data();
    // it->mUsed = pack.size();
    s32 ret = writeIF(it);
    if (EE_OK != ret) {
        deleteMem(it);
        return ret;
    }
    msg->grab();
    return EE_OK;
}



s32 HttpLayer::onTimeout(HandleTime& it) {
    DASSERT(mParser);
    return mParser->shouldKeepAlive() ? EE_OK : EE_ERROR;
}


void HttpLayer::onClose(Handle* it) {
#ifdef DDEBUG
    DASSERT((mConnTLS ? mConnTLS == it : mConnTCP == it) && "HttpLayer::onClose https/http handle?");
#endif
    if (mMsgReq) {
        if (mMsgReq->getEvent()) {
            mMsgReq->getEvent()->onLayerClose(mMsgReq);
            mMsgReq->setEvent(nullptr);
        }
        s32 cnt = mMsgReq->drop();
        mMsgReq = nullptr;
        DLOG(ELL_INFO, "onClose>> req_msg_grab= %d", cnt);
    }
    mParser->onClose();
    s32 my = drop();
    DLOG(ELL_INFO, "onClose>> my_grab= %d", my);
}


bool HttpLayer::onLink(RequestFD* it, TlsContext* tlsContext) {
    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    RequestAccept& req = *(RequestAccept*)it;
    mWebSite = reinterpret_cast<Website*>(accp->getUser());
    if (!mWebSite) {
        DLOG(ELL_ERROR, "HttpLayer::onLink>> invalid website");
        return false;
    }
    mParser = new Http1Parser(this, EHTTP_REQUEST);
    mTlsContext = tlsContext;
    if (!mPool) {
        mPool = MemPool::createMemPool(16 * 1024);
    }
    RequestFD* nd = createMem(4 * 1024);
    nd->mUser = this;
    if (tlsContext) {
        if (!mConnTLS) {
            mConnTLS = new HandleTLS();
        }
        mConnTLS->setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mConnTLS->setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
        nd->mCall = HttpLayer::funcOnReadBeginTLS;
    } else {
        if (!mConnTCP) {
            mConnTCP = new HandleTCP();
        }
        mConnTCP->setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mConnTCP->setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
        nd->mCall = HttpLayer::funcOnRead;
    }
    s32 ret = mConnTLS ? mConnTLS->open(req, nd, mTlsContext) : mConnTCP->open(req, nd);
    if (EE_OK == ret) {
        mWebSite->grab();
        grab();
        Logger::log(ELL_INFO, "HttpLayer::onLink>> [%s->%s]", getRemote()->getStr(), getLocal()->getStr());
    } else {
        deleteMem(nd);
        Logger::log(ELL_ERROR, "HttpLayer::onLink>> [%s->%s], ecode=%d", getRemote()->getStr(), getLocal()->getStr(),
            ret);
        clear();
    }
    return EE_OK == ret;
}

void HttpLayer::onConnect(RequestFD* it) {
    if (EE_OK == it->mError) {
        RequestFD* nd = mMsgReq->buildReq();
        if (sendReq(nd)) {
            it->mCall = HttpLayer::funcOnRead;
            if (EE_OK == readIF(it)) {
                return;
            }
        }
    }
    DLOG(ELL_ERROR, "HttpLayer::onConnect>> [url=%s] [ip=%s], ecode=%d", mMsgReq->getURL().data().data(),
        getRemote()->getStr(), it->mError);
    deleteMem(it);
}


void HttpLayer::onWrite(RequestFD* it, HttpMsg* msg) {
    if (EE_OK != it->mError) {
        DLOG(ELL_ERROR, "onWrite>>size=%u, ecode=%d, msg=%s", it->mUsed, it->mError, msg->getRealPath().data());
        if (msg->getEvent()) {
            msg->getEvent()->onRespWriteError(msg);
        }
    } else {
        if (msg->getEvent()) {
            if (EE_OK != msg->getEvent()->onRespWrite(msg)) {
                if (!msg->isKeepAlive()) {
                    // mConnTCP->setTimeout(10000);  TODO
                    // postClose();  //delay close on timeout
                }
            }
        }
    }
    deleteMem(it);
    msg->drop();
}


void HttpLayer::onReadBeginTLS(RequestFD* it) {
    // TODO: check if http2
    s32 alnp = mConnTLS->getALPN();
    DLOG(ELL_INFO, "HttpLayer::onReadBeginTLS>> remote= %s, ALNP = %d", getRemote()->getStr(), alnp);

    it->mCall = HttpLayer::funcOnRead;
    onRead(it);
}

void HttpLayer::onRead(RequestFD* it) {
    const s8* dat = it->getBuf();
    if (it->mUsed > 0 && EE_OK == it->mError) {
        ssz datsz = it->mUsed;
        ssz parsed = 0;
        ssz stepsz;
        while (datsz > 0 && HPE_OK == mParser->getError()) {
            stepsz = mParser->parseBuf(dat + parsed, datsz);
            parsed += stepsz;
            if (stepsz < datsz) {
                break; // leftover
            }
            datsz -= stepsz;
        }
        it->clearData((u32)parsed);
        if (HPE_OK == mParser->getError() && it->getWriteSize() > 0 && EE_OK == readIF(it)) {
            return; // step success, go on...
        }
        // 如果getWriteSize=0, 则可能受到超长header攻击
        DLOG(ELL_ERROR, "HttpLayer::onRead>> remote= %s, cache size=%u, parser err= %d = %s, ecode = %d",
            getRemote()->getStr(), it->getWriteSize(), mParser->getError(), getErrStr(), it->mError);
    } else {
        mParser->parseBuf(dat, 0); // make the http-msg-finish callback
    }

    DLOG(ELL_ERROR, "HttpLayer::onRead>> remote= %s, read= %u, ecode= %d, parser stat= %d = %s", getRemote()->getStr(),
        it->mUsed, it->mError, mParser->getStatus(), getErrStr());
    deleteMem(it);
    postClose();
}


void HttpLayer::postClose() {
    if (mConnTLS) {
        mConnTLS->getHandleTCP().launchClose();
    }
    if (mConnTCP) {
        mConnTCP->launchClose();
    }
}


void HttpLayer::clear() {
    if (mMsgReq) {
        if (mMsgReq->getEvent()) {
            mMsgReq->getEvent()->onLayerClose(mMsgReq);
            mMsgReq->setEvent(nullptr);
        }
        s32 cnt = mMsgReq->drop();
        mMsgReq = nullptr;
        DLOG(ELL_INFO, "clear>> req_msg_grab= %d", cnt);
    }
    mTlsContext = nullptr;
    if (mConnTLS) {
        delete mConnTLS;
        mConnTLS = nullptr;
    }
    if (mConnTCP) {
        delete mConnTCP;
        mConnTCP = nullptr;
    }
    if (mParser) {
        delete mParser;
        mParser = nullptr;
    }
    if (mWebSite) {
        mWebSite->drop();
        mWebSite = nullptr;
    }
    if (mPool) {
        MemPool::releaseMemPool(mPool);
        mPool = nullptr;
    }
}


} // namespace net
} // namespace app
