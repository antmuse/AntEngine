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


#include "Net/HandleTCP.h"
#if defined(DOS_WINDOWS)
#include "Windows/Request.h"
#endif
#include "Loop.h"
#include "System.h"
#include "Engine.h"
#include "Net/Acceptor.h"


namespace app {
namespace net {

s32 HandleTCP::open(const RequestAccept& req, RequestFD* it) {
    mType = EHT_TCP_LINK;
    mLoop = &Engine::getInstance().getLoop();

    net::Acceptor* accp = (net::Acceptor*)(req.mUser);
    mSock = req.mSocket;
    mLocal = req.mLocal;
    mRemote = req.mRemote;
    setTimeout(accp->getHandleTCP().getTimeout());
    setTimeGap(accp->getHandleTCP().getTimeGap());

    if (EE_OK != mLoop->openHandle(this)) {
        mSock.close();
        return EE_ERROR;
    }
    s32 ret = it ? read(it) : EE_OK;
    if (EE_OK != ret) {
        mSock.close();
    }
    return ret;
}

s32 HandleTCP::open(const NetAddress& addr, RequestFD* it) {
    DASSERT(it);
    mType = EHT_TCP_CONNECT;
    mLoop = &Engine::getInstance().getLoop();
    mRemote = addr;
    if (EE_OK != mLoop->openHandle(this)) {
        return EE_ERROR;
    }
    return connect(it);
}

s32 HandleTCP::open(const String& addr, RequestFD* it) {
    DASSERT(it);
    mType = EHT_TCP_CONNECT;
    mLoop = &Engine::getInstance().getLoop();
    if (addr.getLen() > 0) {
        mRemote.setIPort(addr.c_str());
    }
    if (EE_OK != mLoop->openHandle(this)) {
        return EE_ERROR;
    }
    return connect(it);
}

s32 HandleTCP::close() {
    mSock.close();
    return EE_OK;
}


s32 HandleTCP::accept(RequestAccept* it) {
    DASSERT(it);
    if (0 == (EHF_OPEN & mFlag)) {
        return EE_NO_OPEN;
    }
    it->mType = ERT_ACCEPT;
    it->mError = 0;
    it->mHandle = this;
    it->mSocket.setInvalid();
    if (!it->mSocket.openSeniorTCP()) {
        it->mError = System::getAppError();
        Logger::log(ELL_ERROR, "HandleTCP::accept>>tcp.link, open, addr=%s, ecode=%d", mLocal.getStr(), it->mError);
        return it->mError;
    }
    StringView wbuf = it->getWriteBuf();
    if (!mSock.accept(it->mSocket, it, wbuf.mData, (u32)wbuf.mLen)) {
        it->mError = System::getAppError();
        Logger::log(ELL_ERROR, "HandleTCP::accept>>tcp.link, accept, addr=%s, ecode=%d", mLocal.getStr(), it->mError);
        it->mSocket.close();
        return it->mError;
    }
    mLoop->bindFly(this);
    return EE_OK;
}


s32 HandleTCP::connect(RequestFD* it) {
    if (0 == (EHF_OPEN & mFlag)) {
        return EE_NO_OPEN;
    }
    DASSERT(it);
    it->mType = ERT_CONNECT;
    it->mError = 0;
    it->mHandle = this;
    if (!mSock.connect(mRemote, it)) {
        it->mError = System::getAppError();
        Logger::log(ELL_ERROR, "HandleTCP::connect>>addr=%s, ecode=%d", mRemote.getStr(), it->mError);
        mLoop->closeHandle(this);
        return it->mError;
    }
    mLoop->bindFly(this);
    return EE_OK;
}


s32 HandleTCP::read(RequestFD* it) {
    DASSERT(it);
    if (0 == (EHF_READABLE & mFlag)) {
        return EE_NO_READABLE;
    }
    it->mType = ERT_READ;
    it->mError = 0;
    it->mHandle = this;
    if (!mSock.receive(it)) {
        it->mError = System::getAppError();
        Logger::log(ELL_ERROR, "HandleTCP::read>>addr=%s, ecode=%d", mRemote.getStr(), it->mError);
        mLoop->closeHandle(this);
        return it->mError;
    }
    mLoop->bindFly(this);
    return EE_OK;
}


s32 HandleTCP::write(RequestFD* it) {
    DASSERT(it);
    if (0 == (EHF_WRITEABLE & mFlag)) {
        return EE_NO_WRITEABLE;
    }
    it->mType = ERT_WRITE;
    it->mError = 0;
    it->mHandle = this;
    if (!mSock.send(it)) {
        it->mError = System::getAppError();
        Logger::log(ELL_ERROR, "HandleTCP::write>>addr=%s, ecode=%d", mRemote.getStr(), it->mError);
        mLoop->closeHandle(this);
        return it->mError;
    }
    mLoop->bindFly(this);
    return EE_OK;
}

} //namespace net
} //namespace app
