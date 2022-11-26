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
#if defined(DOS_ANDROID) || defined(DOS_LINUX)
#include <sys/epoll.h>
#include "Linux/Request.h"
#endif
#include "Loop.h"
#include "Engine.h"
#include "System.h"
#include "Net/Acceptor.h"


namespace app {
namespace net {

s32 HandleTCP::open(const RequestAccept& req, RequestTCP* it) {
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

s32 HandleTCP::open(const NetAddress& addr, RequestTCP* it) {
    DASSERT(it);
    mType = EHT_TCP_CONNECT;
    mLoop = &Engine::getInstance().getLoop();
    mRemote = addr;
    if (EE_OK != mLoop->openHandle(this)) {
        return EE_ERROR;
    }
    return connect(it);
}

s32 HandleTCP::open(const String& addr, RequestTCP* it) {
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
    it->mType = ERT_ACCEPT;
    it->mHandle = this;
    it->mSocket.setInvalid();

    if (0 == (EHF_READABLE & mFlag)) {
        it->mError = EE_NO_READABLE;
        return EE_NO_READABLE;
    }
    it->mError = 0;
    mLoop->bindFly(this);

    if (EHF_SYNC_READ & mFlag) {
        /* accept: Allow packets disorder
        if (mReadQueue) {
            addReadPendingTail(it);
            it = (RequestTCP*)popReadReq();
        } */
        mLoop->addPending(it);
        mFlag &= ~EHF_SYNC_READ;
        return EE_OK;
    }

    addReadPendingTail(it);
    return EE_OK;
}


s32 HandleTCP::connect(RequestTCP* it) {
    DASSERT(it);
    it->mType = ERT_CONNECT;
    it->mHandle = this;
    if (0 == (EHF_OPEN & mFlag)) {
        it->mError = EE_NO_OPEN;
        return EE_NO_OPEN;
    }

    if (mWriteQueue) {
        return EE_ERROR;
    }

    it->mError = 0;
    mLoop->bindFly(this);

    if (0 != mSock.connect(mRemote)) {
        s32 err = System::getAppError();
        if (EE_POSTED == err) {
            EventPoller::SEvent evt;
            evt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
            evt.mData.mPointer = this;
            if (mLoop->getEventPoller().add(mSock, evt)) {
                addWritePendingTail(it);
                return EE_OK;
            }
            err = System::getAppError();
        }
        it->mError = err;
    } else { //success now
        EventPoller::SEvent evt;
        evt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
        evt.mData.mPointer = this;
        if (mLoop->getEventPoller().add(mSock, evt)) {
            mLoop->addPending(it);
            return EE_OK;
        }
        it->mError = System::getAppError();
    }

    mLoop->closeHandle(this);
    mLoop->addPending(it);
    return EE_OK;
}


s32 HandleTCP::read(RequestTCP* it) {
    DASSERT(it);
    it->mType = ERT_READ;
    it->mHandle = this;
    if (0 == (EHF_READABLE & mFlag)) {
        it->mError = EE_NO_READABLE;
        return EE_NO_READABLE;
    }

    it->mError = 0;
    mLoop->bindFly(this);

    if (EHF_SYNC_READ & mFlag) {
        if (mReadQueue) {
            addReadPendingTail(it);
            it = (RequestTCP*)popReadReq();
        }
        mLoop->addPending(it);
        mFlag &= ~EHF_SYNC_READ;
        return EE_OK;
    }

    addReadPendingTail(it);
    return EE_OK;
}


s32 HandleTCP::write(RequestTCP* it) {
    DASSERT(it);
    it->mType = ERT_WRITE;
    it->mHandle = this;
    it->mStepSize = 0;

    if (0 == (EHF_WRITEABLE & mFlag)) {
        it->mError = EE_NO_WRITEABLE;
        return EE_NO_WRITEABLE;
    }

    it->mError = 0;
    mLoop->bindFly(this);

    if (EHF_SYNC_WRITE & mFlag) {
        if (mWriteQueue) {
            addWritePendingTail(it);
            it = (RequestTCP*)popWriteReq();
        }
        mLoop->addPending(it);
        mFlag &= ~EHF_SYNC_WRITE;
        return EE_OK;
    }

    addWritePendingTail(it);
    return EE_OK;
}

} //namespace net
} //namespace app
