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


#include "Loop.h"
#include "System.h"
#include "Timer.h"
#include "Logger.h"
#include "HandleFile.h"
#include "Net/HandleTCP.h"
#include "Engine.h"

namespace app {

Loop::Loop() :
    mTimeHub(HandleTime::lessTime),
    mRequest(nullptr),
    mTime(Timer::getRelativeTime()),
    mStop(0),
    mPackCMD(256),
    mFlyRequest(0),
    mGrabCount(0) {
}

Loop::~Loop() {
    DASSERT(0 == mGrabCount && 0 == mFlyRequest);
}

void Loop::onCommands() {
    net::Socket& sock = mPoller.getSocketPair().getSocketB();
    for (s32 rsz = 1; rsz > 0;) {
        rsz = sock.receive(mPackCMD.getWritePointer(), (s32)mPackCMD.getWriteSize());
        if (rsz > 0) {
            mPackCMD.resize(mPackCMD.size() + rsz);
            u32 pksz = 0;
            for (MsgHeader* cmd = (MsgHeader*)mPackCMD.getPointer();
                mPackCMD.size() >= sizeof(MsgHeader) && pksz < mPackCMD.size();
                cmd = (MsgHeader*)((s8*)cmd + cmd->mSize)) {
                switch (cmd->mType) {
                case ECT_EXIT:
                    Logger::logInfo("Loop::onCommands>> exit");
                    stop();
                    break;
                case ECT_ACTIVE:
                    Logger::logInfo("Loop::onCommands>> active");
                    break;
                default:break;
                }//switch
                pksz += cmd->mSize;
            }
            mPackCMD.clear(pksz);
        } else if (0 == rsz) {
            const s32 ecode = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::onCommands>> read=0, ecode=%d", ecode);
            stop();
        } else {
            s32 ecode = System::getAppError();
            if (EE_RETRY != ecode) {
                stop();
                Logger::log(ELL_ERROR, "Loop::onCommands>> read<0, ecode=%d", ecode);
            } else {
                if (!sock.receive(&mReadCMD)) {
                    ecode = System::getAppError();
                    Logger::log(ELL_ERROR, "Loop::onCommands>>post cmd recv, ecode=%d", ecode);
                    stop();
                }
            }
        }
    }//for
}

bool Loop::run() {
    const s32 pmax = 128;
    u32 timeout = getWaitTime();
    EventPoller::SEvent evts[pmax];
    s32 max = mPoller.getEvents(evts, pmax, timeout);
    if (max > 0) {
        Request* req;
        for (s32 i = 0; i < max; ++i) {
            if (evts[i].mPointer) {
                req = DGET_HOLDER(evts[i].mPointer, Request, mOverlapped);
                if (req->mHandle) {
                    addPending(req);
                } else {//cmd
                    DASSERT((&mReadCMD) == req);
                    onCommands();
                }
            } else {
                Logger::log(ELL_ERROR, "Loop::run>>Poll null");
            }
        }
    } else {
        const s32 ecode = System::getAppError();
        if (EE_TIMEOUT != ecode) {
            Logger::log(ELL_ERROR, "Loop::run>>Poll events ecode=%d", ecode);
        }
    }
    updatePending();
    updateClosed();
    return mGrabCount > 0;
}


void Loop::updatePending() {
    if (nullptr == mRequest) {
        return;
    }
    Request* first = mRequest->mNext;
    Request* next = first;
    mRequest = nullptr;
    for (Request* req = next; next; req = next) {
        next = req->mNext != first ? req->mNext : nullptr;
        switch (req->mType) {
        case ERT_CONNECT:
        {
            net::RequestTCP* nd = (net::RequestTCP*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            if ((NTSTATUS)(nd->mOverlapped.Internal) >= 0) {
                if (0 == hnd->getSock().updateByConnect()) {
                    hnd->mFlag |= (EHF_READABLE | EHF_WRITEABLE);
                    nd->mError = 0;
                    relinkTime((HandleTime*)hnd);
                } else {
                    nd->mError = System::getAppError();
                    closeHandle(hnd);
                }
            } else {
                nd->mError = System::convNtstatus2NetError((NTSTATUS)(nd->mOverlapped.Internal));
                closeHandle(hnd);
            }
            nd->mCall(nd); //nd maybe deleted
            unbindFly(hnd);
            break;
        }
        case ERT_READ:
        {//read on HandleTCP or HandleFile
            net::RequestTCP* nd = (net::RequestTCP*)req;
            Handle* hnd = nd->mHandle;
            if ((NTSTATUS)(nd->mOverlapped.Internal) >= 0) {
                nd->mUsed += (u32)nd->mOverlapped.InternalHigh;
                nd->mError = 0;
                if (0 == nd->mOverlapped.InternalHigh) {
                    closeHandle(hnd);
                } else {
                    relinkTime((HandleTime*)hnd);
                }
            } else {
                nd->mError = System::convNtstatus2NetError((NTSTATUS)(nd->mOverlapped.Internal));
                closeHandle(hnd);
            }
            nd->mCall(nd); //nd maybe deleted
            unbindFly(hnd);
            break;
        }
        case ERT_WRITE:
        {//write on HandleTCP or HandleFile
            net::RequestTCP* nd = (net::RequestTCP*)req;
            Handle* hnd = nd->mHandle;
            if ((NTSTATUS)(nd->mOverlapped.Internal) >= 0) {
                nd->mError = (nd->mUsed) ^ ((u32)nd->mOverlapped.InternalHigh);
                relinkTime((HandleTime*)hnd);
            } else {
                nd->mError = System::convNtstatus2NetError((NTSTATUS)(nd->mOverlapped.Internal));
                closeHandle(hnd);
            }
            nd->mCall(nd); //nd maybe deleted
            unbindFly(hnd);
            break;
        }
        case ERT_ACCEPT:
        {
            net::RequestAccept* nd = (net::RequestAccept*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            if ((NTSTATUS)(nd->mOverlapped.Internal) >= 0) {
                nd->mError = 0;
                if (0 == nd->mSocket.updateByAccepter(hnd->mSock)) {
                    u32 ipsz = hnd->mLocal.getAddrSize();
                    nd->mLocal.setAddrSize(ipsz);
                    nd->mRemote.setAddrSize(ipsz);
                    ipsz = (ipsz + 16) << 1;
                    if (!hnd->mSock.getAddress(nd->mCache, ipsz, nd->mLocal, nd->mRemote)) {
                        nd->mError = System::getAppError();
                        Logger::log(ELL_ERROR, "Loop::updatePending>>getAddress ecode=%d", nd->mError);
                    }
                } else {
                    nd->mError = System::getAppError();
                    Logger::log(ELL_ERROR, "Loop::updatePending>>updateByAccepter ecode=%d", nd->mError);
                }
            } else {
                nd->mError = System::convNtstatus2NetError((NTSTATUS)(nd->mOverlapped.Internal));
                closeHandle(hnd);
            }
            nd->mCall(nd); //nd maybe deleted
            unbindFly(hnd);
            break;
        }
        default:
            DASSERT(0);
            break;
        }//switch

    }//for
}


void Loop::updateClosed() {
    if (mHandleClose.empty()) {
        return;
    }

    Node2 list;
    mHandleClose.splitAndJoin(list);

    Handle* curr;
    while (&list != list.mNext) {
        curr = (Handle*)list.mNext;
        curr->delink();
        curr->mFlag |= EHF_CLOSE;
        if (curr->mCallClose) {
            curr->mCallClose(curr);
        }
        drop();
    }
}


u32 Loop::getWaitTime() {
    mTime = Timer::getRelativeTime();
    if (mRequest || !mHandleClose.empty()) {
        return 0;
    }
    return updateTimeHub();
}


bool Loop::start() {
    String unpath = Engine::getInstance().getConfig().mLogPath;
    unpath += System::getPID();
    unpath += ".unpath";
    mPoller.open(unpath.c_str());
    net::Socket& sock = mPoller.getSocketPair().getSocketB();
    if (0 != sock.setBlock(false)) {
        return false;
    }
    if (!mPoller.add(sock, nullptr)) {
        return false;
    }
    mReadCMD.mType = ERT_READ;
    mReadCMD.mError = 0;
    mReadCMD.mHandle = nullptr;
    if (!sock.receive(&mReadCMD)) {
        mReadCMD.mError = System::getAppError();
        Logger::log(ELL_ERROR, "Loop::start>>cmd recv, ecode=%d", mReadCMD.mError);
        return false;
    }
    return true;
}


void Loop::stop() {
    if (0 != mStop) {
        return;
    }
    mStop = 1;

    const Node2* head = &mHandleActive;
    Node2* next = mHandleActive.mNext;
    for (Node2* curr = next; head != curr; curr = next) {
        next = next->mNext;
        closeHandle((Handle*)curr);
    }
    net::Socket& sock = mPoller.getSocketPair().getSocketB();
    sock.close();
}


void Loop::relinkTime(HandleTime* handle) {
    DASSERT(handle);
    if (0 == (EHF_CLOSING & handle->mFlag)) {
        mTimeHub.remove(&handle->mLink);
        handle->mTimeout = mTime + handle->mTimeGap;
        mTimeHub.insert(&handle->mLink);
        //printf("Loop::relinkTime>>%lld, gap=%lld\n", handle->mTimeout, handle->mTimeGap / 1000);
    }
}


u32 Loop::updateTimeHub() {
    HandleTime* handle;
    for (Node3* nd = mTimeHub.getTop(); nd; nd = mTimeHub.getTop()) {
        handle = DGET_HOLDER(nd, HandleTime, mLink);
        if (handle->mTimeout > mTime) {
            u64 diff = (handle->mTimeout - mTime);
            return diff > 1000 ? 1000 : (u32)diff;
            //break;
        }
        if (EE_OK == handle->mCallTime(handle)) {
            if (handle->mRepeat > 0) {
                mTimeHub.remove(nd);
                --handle->mRepeat;
                handle->mTimeout = mTime + handle->mTimeGap;
                mTimeHub.insert(nd);
            } else if (handle->mRepeat < 0) {
                mTimeHub.remove(nd);
                handle->mTimeout = mTime + handle->mTimeGap;
                mTimeHub.insert(nd);
            } else {
                closeHandle(handle);
                /*handle->mFlag |= EHF_CLOSING;
                handle->drop();
                addClose(handle);*/
            }
        } else {
            closeHandle(handle);
            /*mTimeHub.remove(nd);
            handle->mFlag |= EHF_CLOSING;
            handle->drop();
            addClose(handle);*/
        }
    }
    return 0;
}


void Loop::addClose(Handle* it) {
    it->delink();
    mHandleClose.pushFront(*it);
}


s32 Loop::closeHandle(Handle* it) {
    DASSERT(it);
    if (it->isClosing()) {
        return EE_CLOSING;
    }
    it->mFlag |= EHF_CLOSING;
    it->mFlag &= ~(EHF_READABLE | EHF_WRITEABLE);

    s32 ret = EE_OK;

    switch (it->mType) {
    case EHT_TCP_LINK:
    case EHT_TCP_ACCEPT:
    case EHT_TCP_CONNECT:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        ret = nd->close();
        if (nd->mCallTime) {
            mTimeHub.remove(&nd->mLink);
        }
        break;
    }
    case EHT_TIME:
    {
        HandleTime* nd = reinterpret_cast<HandleTime*>(it);
        mTimeHub.remove(&nd->mLink);
        break;
    }
    case EHT_FILE:
    {
        HandleFile* nd = reinterpret_cast<HandleFile*>(it);
        ret = nd->close();
        if (nd->mCallTime) {
            mTimeHub.remove(&nd->mLink);
        }
        break;
    }
    default:
        ret = EE_INVALID_HANDLE;
        break;
    }//switch

    if (EE_OK == ret) {
        if (0 == it->drop() && 0 == it->getFlyRequest()) {
            addClose(it);
        }
    }
    return ret;
}


s32 Loop::openHandle(Handle* it) {
    DASSERT(it);
    if (mStop > 0) {
        it->mFlag |= (EHF_CLOSING | EHF_CLOSE);
        return EE_CLOSING;
    }

    s32 ret = EE_OK;

    it->mFlag = EHF_INIT;
    it->mLoop = this;

    switch (it->mType) {
    case EHT_TCP_CONNECT:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        net::Socket& sock = nd->getSock();
        if (!sock.openSeniorTCP()) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.con, open, addr=%s, ecode=%d", nd->mRemote.getStr(), ret);
        } else if (0 != sock.setBlock(false)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.con, unblock, addr=%s, ecode=%d", nd->mRemote.getStr(), ret);
            sock.close();
        } else if (0 != sock.bind()) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.con, bind, addr=%s, ecode=%d", nd->mRemote.getStr(), ret);
            sock.close();
        } else if (0 != sock.setDelay(false)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.con, delay, addr=%s, ecode=%d", nd->mRemote.getStr(), ret);
            sock.close();
        } else if (!mPoller.add(sock, nd)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.con, add poll, addr=%s, ecode=%d", nd->mRemote.getStr(), ret);
            sock.close();
        } else {
            if (nd->mCallTime) {
                nd->mTimeout += Timer::getRelativeTime();
                mTimeHub.insert(&nd->mLink);
            }
        }
        break;
    }
    case EHT_TCP_ACCEPT:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        net::Socket& sock = nd->getSock();
        if (!sock.openSeniorTCP()) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.acc, open, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
        } else if (0 != sock.setReuseIP(true)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.acc, reIP, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (0 != sock.setReusePort(true)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.acc, rePort, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (0 != sock.bind(nd->mLocal)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.acc, bind, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (0 != sock.listen(1024)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.acc, listen, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (!mPoller.add(sock, nd)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.acc, add, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else {
            nd->mFlag |= EHF_READABLE;
        }
        break;
    }
    case EHT_TCP_LINK:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        if (!mPoller.add(nd->getSock(), nd)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>tcp.link, add, addr=%s, ecode=%d", nd->mLocal.getStr(), ret);
            nd->getSock().close();
        } else {
            nd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
            if (nd->mCallTime) {
                nd->mTimeout += Timer::getRelativeTime();
                mTimeHub.insert(&nd->mLink);
            }
        }
        break;
    }
    case EHT_TIME:
    {
        HandleTime* nd = reinterpret_cast<HandleTime*>(it);
        nd->mTimeout += Timer::getRelativeTime();
        mTimeHub.insert(&nd->mLink);
        break;
    }
    case EHT_FILE:
    {
        HandleFile* nd = reinterpret_cast<HandleFile*>(it);
        if (!mPoller.add(nd->getHandle(), nd)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>file=%s, ecode=%d", nd->getFileName().c_str(), ret);
            nd->close();
        } else {
            nd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
            if (nd->mCallTime) {
                nd->mTimeout += Timer::getRelativeTime();
                mTimeHub.insert(&nd->mLink);
            }
        }
        break;
    }
    case EHT_UNKNOWN:
    default:
        DASSERT(0 && "invalid handle type");
        ret = EE_INVALID_HANDLE;
        break;
    }//switch

    if (EE_OK == ret) {
        bindHandle(it);
        it->mFlag |= EHF_OPEN;
        mHandleActive.pushBack(*it);
    } else {
        it->mFlag |= (EHF_CLOSING | EHF_CLOSE);
    }

    return ret;
}


void Loop::addPending(Request* it) {
    if (mRequest) {
        it->mNext = mRequest->mNext;
        mRequest->mNext = it;
    } else {
        it->mNext = it;
    }
    mRequest = it;
}

} //namespace app
