/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#include "Net/TcpProxy.h"
#include "EngineConfig.h"
#include "Net/Acceptor.h"

namespace app {
namespace net {

const u32 gCacheSZ = 4 * 1024;


TcpProxy::TcpProxy(Loop& loop) :
    mLoop(loop), mType(0), mHub(nullptr) {

    //front
    mTLS.getHandleTCP().setClose(EHT_TCP_LINK, TcpProxy::funcOnClose, this);
    mTLS.getHandleTCP().setTime(TcpProxy::funcOnTime, 20 * 1000, 30 * 1000, -1);

    //backend
    mTLS2.getHandleTCP().setClose(EHT_TCP_CONNECT, TcpProxy::funcOnClose2, this);
    mTLS2.getHandleTCP().setTime(TcpProxy::funcOnTime2, 30 * 1000, 30 * 1000, -1);
}


TcpProxy::~TcpProxy() {
}

s32 TcpProxy::onTimeout(HandleTime& it) {
    Logger::log(ELL_INFO, "TcpProxy::onTimeout>>time=%lld", mLoop.getTime());
    mLoop.closeHandle(&mTLS2.getHandleTCP());
    return EE_ERROR;
}


s32 TcpProxy::onTimeout2(HandleTime& it) {
    Logger::log(ELL_INFO, "TcpProxy::onTimeout2>>time=%lld", mLoop.getTime());
    mLoop.closeHandle(&mTLS.getHandleTCP());
    return EE_ERROR;
}

void TcpProxy::unbind() {
    if (mHub) {
        mHub->drop();
        mHub = nullptr;
    }
    drop();
}

void TcpProxy::onClose(Handle* it) {
    if ((2 & mType) > 0 ? mTLS2.isClose() : mTLS2.getHandleTCP().isClose()) {
        Logger::log(ELL_INFO, "TcpProxy::onClose>>front=%s", mTLS.getRemote().getStr());
        unbind();
    } else {
        mLoop.closeHandle(&mTLS2.getHandleTCP());
    }
}


void TcpProxy::onClose2(Handle* it) {
    if ((1 & mType) > 0 ? mTLS.isClose() : mTLS.getHandleTCP().isClose()) {
        Logger::log(ELL_INFO, "TcpProxy::onClose2>>backend=%s", mTLS2.getRemote().getStr());
        unbind();
    } else {
        mLoop.closeHandle(&mTLS.getHandleTCP());
    }
}


void TcpProxy::onWrite(RequestFD* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "TcpProxy::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestFD::delRequest(it);
}


void TcpProxy::onWrite2(RequestFD* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "TcpProxy::onWrite2>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestFD::delRequest(it);
}


void TcpProxy::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        RequestFD* out = RequestFD::newRequest(gCacheSZ);
        out->mUser = this;
        out->mCall = TcpProxy::funcOnRead;
        if (0 != ((1 & mType) > 0 ? mTLS.read(out) : mTLS.getHandleTCP().read(out))) {
            RequestFD::delRequest(out);
        }
        it->mCall = TcpProxy::funcOnWrite2;
        it->setStepSize(0);
        if (EE_OK != ((2 & mType) > 0 ? mTLS2.write(it) : mTLS2.getHandleTCP().write(it))) {
            RequestFD::delRequest(it);
        }
    } else {
        printf("TcpProxy::onRead>>read 0 bytes, ecode=%d\n", it->mError);
        RequestFD::delRequest(it);
    }
}


void TcpProxy::onRead2(RequestFD* it) {
    if (it->mUsed > 0) {
        RequestFD* out = RequestFD::newRequest(gCacheSZ);
        out->mUser = this;
        out->mCall = TcpProxy::funcOnRead2;
        if (0 != ((2 & mType) > 0 ? mTLS2.read(out) : mTLS2.getHandleTCP().read(out))) {
            RequestFD::delRequest(out);
        }

        it->mCall = TcpProxy::funcOnWrite;
        it->setStepSize(0);
        if (EE_OK != ((1 & mType) > 0 ? mTLS.write(it) : mTLS.getHandleTCP().write(it))) {
            RequestFD::delRequest(it);
        }
    } else {
        printf("TcpProxy::onRead2>>read 0 bytes, ecode=%d\n", it->mError);
        RequestFD::delRequest(it);
    }
}


void TcpProxy::onConnect(RequestFD* it) {
    if (EE_OK == it->mError) {
        it->mCall = TcpProxy::funcOnRead2;
        if (0 == ((2 & mType) > 0 ? mTLS2.read(it) : mTLS2.getHandleTCP().read(it))) {
            RequestFD* read = RequestFD::newRequest(gCacheSZ);
            read->mUser = this;
            read->mCall = TcpProxy::funcOnRead;
            if (0 != ((1 & mType) > 0 ? mTLS.read(read) : mTLS.getHandleTCP().read(read))) {
                RequestFD::delRequest(read);
                mLoop.closeHandle(&mTLS.getHandleTCP());
                mLoop.closeHandle(&mTLS2.getHandleTCP());
            }
            return;
        }
    }

    Logger::log(ELL_ERROR, "TcpProxy::onConnect>>ecode=%d", it->mError);
    RequestFD::delRequest(it);
    mLoop.closeHandle(&mTLS.getHandleTCP());
}


//s32 TcpProxy::open() {
//    s32 ret = mLoop.openHandle(&mTLS.getHandleTCP()); //have not start receive yet
//    if (EE_OK == ret) {
//        if (EE_OK == mLoop.openHandle(&mTLS2.getHandleTCP())) {
//            RequestFD* it = RequestFD::newRequest(gCacheSZ);
//            it->mUser = this;
//            it->mCall = funcOnConnect;
//            if (EE_OK != mTLS2.getHandleTCP().connect(it)) {
//                RequestFD::delRequest(it);
//                mLoop.closeHandle(&mTLS.getHandleTCP());
//                mLoop.closeHandle(&mTLS2.getHandleTCP());
//            }
//        }
//    } else {
//        mTLS.getHandleTCP().getSock().close();
//    }
//
//    return ret;
//}


void TcpProxy::onLink(RequestFD* it) {
    DASSERT(nullptr == mHub);

    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    RequestAccept* req = (RequestAccept*)it;
    mHub = reinterpret_cast<TcpProxyHub*>(accp->getUser());

    mType = mHub->getConfig().mType;
    s64 tmout = accp->getHandleTCP().getTimeout();
    s64 tgap = accp->getHandleTCP().getTimeGap();
    /*mTLS.getHandleTCP().setSocket(req->mSocket);
    mTLS.getHandleTCP().setLocal(req->mLocal);
    mTLS.getHandleTCP().setRemote(req->mRemote);
    mTLS.getHandleTCP().setTimeout(tmout);
    mTLS.getHandleTCP().setTimeGap(tgap);*/

    mTLS2.getHandleTCP().setTimeout(tmout);
    mTLS2.getHandleTCP().setTimeGap(tgap);
    mTLS2.getHandleTCP().setLocal(accp->getHandleTCP().getLocal());
    mTLS2.getHandleTCP().setRemote(accp->getHandleTCP().getRemote());

    if (0 == mTLS.getHandleTCP().getTimeGap()) {
        mTLS.getHandleTCP().setTimeCaller(nullptr);
    }
    if (0 == mTLS2.getHandleTCP().getTimeGap()) {
        mTLS2.getHandleTCP().setTimeCaller(nullptr);
    }

    //open first, have not start receive
    s32 ecode = EE_OK;
    if ((1 & mType) > 0) {
        mTLS.setClose(EHT_TCP_LINK, TcpProxy::funcOnClose, this);
        mTLS.setTime(TcpProxy::funcOnTime, tmout, tgap, -1);
        ecode = mTLS.open(*req, nullptr);
    } else {
        ecode = mTLS.getHandleTCP().open(*req, nullptr);
    }
    if (EE_OK != ecode) {
        mTLS.getHandleTCP().getSock().close();
        mHub = nullptr;
        return;
    }

    //bind
    grab();
    mHub->grab();

    //backend start connect
    RequestFD* conn = RequestFD::newRequest(gCacheSZ);
    conn->mUser = this;
    conn->mCall = funcOnConnect;
    if ((2 & mType) > 0) {
        mTLS2.setClose(EHT_TCP_CONNECT, TcpProxy::funcOnClose2, this);
        mTLS2.setTime(TcpProxy::funcOnTime2, tmout, tgap, -1);
        ecode = mTLS2.open(mTLS2.getRemote(), conn);
    } else {
        ecode = mTLS2.getHandleTCP().open(mTLS2.getHandleTCP().getRemote(), conn);
    }

    if (EE_OK != ecode) {
        Logger::log(ELL_ERROR, "TcpProxy::onLink>> [%s->%s->%s], ecode=%d",
            mTLS.getRemote().getStr(), mTLS2.getLocal().getStr(), mTLS2.getRemote().getStr(), ecode);
        RequestFD::delRequest(it);
        mLoop.closeHandle(&mTLS.getHandleTCP());
        return;
    }

    Logger::log(ELL_INFO, "TcpProxy::onLink>> [%s->%s->%s]",
        mTLS.getRemote().getStr(), mTLS2.getLocal().getStr(), mTLS2.getRemote().getStr());
}

} //namespace net
} //namespace app
