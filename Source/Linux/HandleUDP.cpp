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


#include "Net/HandleUDP.h"
#if defined(DOS_ANDROID) || defined(DOS_LINUX)
#include "Linux/Request.h"
#include <sys/epoll.h>
#endif
#include "Engine.h"
#include "Loop.h"
#include "Net/Acceptor.h"
#include "System.h"


namespace app {
namespace net {

s32 HandleUDP::open(RequestUDP* it, const NetAddress* remote, const NetAddress* local, s32 flag) {
    if (!it || (!remote && !local)) {
        return EE_INVALID_PARAM;
    }
    mType = EHT_UDP;
    mLoop = &Engine::getInstance().getLoop();
    mFlags = 0;

    s32 ret = EE_OK;
    if (!mSock.openSeniorUDP(local ? local->isIPV6() : (remote ? remote->isIPV6() : false))) {
        ret = System::getAppError();
        Logger::log(ELL_ERROR, "HandleUDP::open>>udp.con, open, addr=%s, ecode=%d", mRemote.getStr(), ret);
    }
    // local
    if (EE_OK == ret) {
        if (local) {
            mLocal = *local;
        }
        if ((flag & 1) > 0) {
            mFlags |= 1;
            if (0 != mSock.setReuseIP(true)) {
                ret = System::getAppError();
                Logger::log(ELL_ERROR, "HandleUDP::open>>reuseIP, addr=%s, ecode=%d", mLocal.getStr(), ret);
            } else if (0 != mSock.setReusePort(true)) {
                ret = System::getAppError();
                Logger::log(ELL_ERROR, "HandleUDP::open>>reusePort, addr=%s, ecode=%d", mLocal.getStr(), ret);
            }
        }
        if (EE_OK == ret && 0 != mSock.bind(mLocal)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "HandleUDP::open>>bind, addr=%s, ecode=%d", mRemote.getStr(), ret);
        }
        if (!local) {
            mSock.getLocalAddress(mLocal);
        }
    }

    if (EE_OK == ret && remote && (flag & 2) > 0) {
        mRemote = *remote;
        it->mRemote = *remote;
        mFlags |= 2;
        if (0 != mSock.connect(mRemote)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "HandleUDP::open>>connect, addr=%s, ecode=%d", mRemote.getStr(), ret);
        }
    }

    if (EE_OK == ret && 0 != mSock.setBlock(false)) {
        ret = System::getAppError();
        Logger::log(ELL_ERROR, "HandleUDP::open>>unblock, addr=%s, ecode=%d", mRemote.getStr(), ret);
    }

    if (EE_OK != ret) {
        mSock.close();
        return ret;
    }

    if (EE_OK != mLoop->openHandle(this)) {
        return EE_ERROR;
    }
    return (mFlags & 2) > 0 ? read(it) : readFrom(it);
}


s32 HandleUDP::open(RequestUDP* it, const s8* remote, const s8* local, s32 flag) {
    bool val1 = remote && 0 != remote[0];
    bool val2 = local && 0 != local[0];
    DASSERT(it && (val1 || val2));
    if (!it || (!val1 && !val2)) {
        return EE_INVALID_PARAM;
    }
    if (val1) {
        mRemote.setIPort(remote);
    }
    if (val2) {
        mLocal.setIPort(local);
    }
    return open(it, val1 ? &mRemote : nullptr, val2 ? &mLocal : nullptr, flag);
}


s32 HandleUDP::close() {
    mSock.close();
    return EE_OK;
}


s32 HandleUDP::read(RequestUDP* it) {
    return read(it, 1);
}

s32 HandleUDP::write(RequestUDP* it) {
    return write(it, 1);
}

s32 HandleUDP::readFrom(RequestUDP* it) {
    return read(it, 0);
}

s32 HandleUDP::writeTo(RequestUDP* it) {
    return write(it, 0);
}

s32 HandleUDP::read(RequestUDP* it, s32 flag) {
    DASSERT(it);
    it->mType = ERT_READ;
    it->mHandle = this;
    it->mFlags = flag;

    if (0 == (EHF_READABLE & mFlag)) {
        it->mError = EE_NO_READABLE;
        return EE_NO_READABLE;
    }

    it->mError = 0;
    mLoop->bindFly(this);

    if (EHF_SYNC_READ & mFlag) {
        if (mReadQueue) {
            addReadPendingTail(it);
            it = (RequestUDP*)popReadReq();
        }
        mLoop->addPending(it);
        mFlag &= ~EHF_SYNC_READ;
        return EE_OK;
    }

    addReadPendingTail(it);
    return EE_OK;
}


s32 HandleUDP::write(RequestUDP* it, s32 flag) {
    DASSERT(it);
    it->mType = ERT_WRITE;
    it->mHandle = this;
    it->mStepSize = 0;
    it->mFlags = flag;

    if (0 == (EHF_WRITEABLE & mFlag)) {
        it->mError = EE_NO_WRITEABLE;
        return EE_NO_WRITEABLE;
    }

    it->mError = 0;
    mLoop->bindFly(this);

    if (EHF_SYNC_WRITE & mFlag) {
        if (mWriteQueue) {
            addWritePendingTail(it);
            it = (RequestUDP*)popWriteReq();
        }
        mLoop->addPending(it);
        mFlag &= ~EHF_SYNC_WRITE;
        return EE_OK;
    }

    addWritePendingTail(it);
    return EE_OK;
}


} // namespace net
} // namespace app
