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
#include "Net/Acceptor.h"

namespace app {
namespace net {

const u32 gCacheSZ = 4 * 1024;

TcpProxy::TcpProxy(Loop& loop) :
    mLoop(loop) {

    //front
    mTCP.setClose(EHT_TCP_LINK, TcpProxy::funcOnClose, this);
    mTCP.setTime(TcpProxy::funcOnTime, 20 * 1000, 30 * 1000, -1);

    //backend
    mTcpBackend.setClose(EHT_TCP_CONNECT, TcpProxy::funcOnClose2, this);
    mTcpBackend.setTime(TcpProxy::funcOnTime2, 30 * 1000, 30 * 1000, -1);
}


TcpProxy::~TcpProxy() {
}

s32 TcpProxy::onTimeout(HandleTime& it) {
    Logger::log(ELL_INFO, "TcpProxy::onTimeout>>time=%lld", mLoop.getTime());
    mLoop.closeHandle(&mTcpBackend);
    return EE_ERROR;
}


s32 TcpProxy::onTimeout2(HandleTime& it) {
    Logger::log(ELL_INFO, "TcpProxy::onTimeout2>>time=%lld", mLoop.getTime());
    DASSERT(&mTcpBackend == &it);
    mLoop.closeHandle(&mTCP);
    return EE_ERROR;
}


void TcpProxy::onClose(Handle* it) {
    DASSERT(&mTCP == it);
    if (mTcpBackend.isClose()) {
        delete this;
    } else {
        mLoop.closeHandle(&mTcpBackend);
    }
}


void TcpProxy::onClose2(Handle* it) {
    DASSERT(&mTcpBackend == it);
    if (mTCP.isClose()) {
        delete this;
    } else {
        Logger::log(ELL_INFO, "TcpProxy::onClose2>>server=%s", mTcpBackend.getRemote().getStr());
        mLoop.closeHandle(&mTCP);
    }
}


void TcpProxy::onWrite(net::RequestTCP* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "TcpProxy::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestTCP::delRequest(it);
}


void TcpProxy::onWrite2(net::RequestTCP* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "TcpProxy::onWrite2>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestTCP::delRequest(it);
}


void TcpProxy::onRead(net::RequestTCP* it) {
    if (it->mUsed > 0) {
        net::RequestTCP* out = RequestTCP::newRequest(gCacheSZ);
        out->mUser = this;
        out->mCall = TcpProxy::funcOnRead;
        if (0 != mTCP.read(out)) {
            RequestTCP::delRequest(out);
        }
        it->mCall = TcpProxy::funcOnWrite2;
        it->setStepSize(0);
        if (EE_OK != mTcpBackend.write(it)) {
            RequestTCP::delRequest(it);
        }
    } else {
        printf("TcpProxy::onRead>>read 0 bytes, ecode=%d\n", it->mError);
        RequestTCP::delRequest(it);
    }
}


void TcpProxy::onRead2(net::RequestTCP* it) {
    if (it->mUsed > 0) {
        net::RequestTCP* out = RequestTCP::newRequest(gCacheSZ);
        out->mUser = this;
        out->mCall = TcpProxy::funcOnRead2;
        if (0 != mTcpBackend.read(out)) {
            RequestTCP::delRequest(out);
        }

        it->mCall = TcpProxy::funcOnWrite;
        it->setStepSize(0);
        if (EE_OK != mTCP.write(it)) {
            RequestTCP::delRequest(it);
        }
    } else {
        printf("TcpProxy::onRead2>>read 0 bytes, ecode=%d\n", it->mError);
        RequestTCP::delRequest(it);
    }
}


void TcpProxy::onConnect(net::RequestTCP* it) {
    if (0 == it->mError) {
        it->mCall = TcpProxy::funcOnRead2;
        if (0 == mTcpBackend.read(it)) {
            net::RequestTCP* read = RequestTCP::newRequest(gCacheSZ);
            read->mUser = this;
            read->mCall = TcpProxy::funcOnRead;
            if (0 != mTCP.read(read)) {
                RequestTCP::delRequest(read);
                mLoop.closeHandle(&mTCP);
                mLoop.closeHandle(&mTcpBackend);
            }
            return;
        }
    }

    Logger::log(ELL_ERROR, "TcpProxy::onConnect>>ecode=%d", it->mError);
    RequestTCP::delRequest(it);
    mLoop.closeHandle(&mTCP);
}


s32 TcpProxy::open() {
    s32 ret = mLoop.openHandle(&mTCP);
    if (EE_OK == ret) {
        if (EE_OK == mLoop.openHandle(&mTcpBackend)) {
            net::RequestTCP* it = RequestTCP::newRequest(gCacheSZ);
            it->mUser = this;
            it->mCall = funcOnConnect;
            if (EE_OK != mTcpBackend.connect(it)) {
                RequestTCP::delRequest(it);
                mLoop.closeHandle(&mTCP);
                mLoop.closeHandle(&mTcpBackend);
            }
        }
    } else {
        mTCP.getSock().close();
    }

    return ret;
}


void TcpProxy::onLink(net::RequestTCP* it) {
    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    net::RequestAccept* req = (net::RequestAccept*)it;

    mTCP.setSocket(req->mSocket);
    mTCP.setLocal(req->mLocal);
    mTCP.setRemote(req->mRemote);
    mTCP.setTimeout(accp->getHandleTCP().getTimeout());
    mTCP.setTimeGap(accp->getHandleTCP().getTimeGap());

    mTcpBackend.setTimeout(accp->getHandleTCP().getTimeout());
    mTcpBackend.setTimeGap(accp->getHandleTCP().getTimeGap());
    mTcpBackend.setLocal(accp->getHandleTCP().getLocal());
    mTcpBackend.setRemote(accp->getHandleTCP().getRemote());

    if (0 == mTCP.getTimeGap()) {
        mTCP.setTimeCaller(nullptr);
    }
    if (0 == mTcpBackend.getTimeGap()) {
        mTcpBackend.setTimeCaller(nullptr);
    }

    s32 ret = open();
    if (0 == ret) {
        Logger::log(ELL_INFO, "TcpProxy::onLink>> [%s->%s->%s]",
            mTCP.getRemote().getStr(), mTcpBackend.getLocal().getStr(), mTcpBackend.getRemote().getStr());
    } else {
        Logger::log(ELL_ERROR, "TcpProxy::onLink>> [%s->%s->%s], ecode=%d",
            mTCP.getRemote().getStr(), mTcpBackend.getLocal().getStr(), mTcpBackend.getRemote().getStr(), ret);
        delete this;
    }
}

} //namespace net
} //namespace app
