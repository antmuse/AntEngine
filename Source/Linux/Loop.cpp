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
#include <sys/epoll.h>
#include "System.h"
#include "Timer.h"
#include "Logger.h"
#include "Net/HandleTCP.h"


namespace app {

Loop::Loop() :
    mTimeHub(HandleTime::lessTime),
    mRequest(nullptr),
    mTime(Timer::getRelativeTime()),
    mStop(0),
    mFlyRequest(0),
    mGrabCount(0) {
}

Loop::~Loop() {
    DASSERT(0 == mGrabCount && 0 == mFlyRequest);
}

bool Loop::run() {
    const s32 pmax = 128;
    EventPoller::SEvent evts[pmax];
    s32 ecode = 0;
    net::Socket& sock = mPoller.getSocketPair().getSocketB();

    u32 timeout = getWaitTime();
    s32 max = mPoller.getEvents(evts, pmax, timeout);
    if (max > 0) {
        for (s32 i = 0; i < max; ++i) {
            u32 eflag = evts[i].mEvent;
            if (eflag & (EPOLLERR | EPOLLHUP)) {
                eflag |= (EPOLLIN | EPOLLOUT);
            }
            if (evts[i].mData.mPointer) {
                net::HandleTCP& han = *(net::HandleTCP*)(evts[i].mData.mPointer);
                Request* req;
                if (EPOLLIN & eflag) {
                    req = han.popReadReq();
                    if (req) {
                        addPending(req);
                        //Logger::log(ELL_ERROR, "Loop::run>>Poll=%s", ERT_ACCEPT == req->mType ? "accept" : "read");
                    } else {
                        han.mFlag |= EHF_SYNC_READ;
                        //Logger::log(ELL_ERROR, "Loop::run>>Poll=EHF_SYNC_READ");
                    }
                }
                if (EPOLLOUT & eflag) {
                    req = han.popWriteReq();
                    if (req) {
                        addPending(req);
                        //Logger::log(ELL_ERROR, "Loop::run>>Poll=write");
                    } else {
                        han.mFlag |= EHF_SYNC_WRITE;
                        //Logger::log(ELL_ERROR, "Loop::run>>Poll=EHF_SYNC_WRITE");
                    }
                }
            } else {//cmd
                if (EPOLLIN & eflag) {
                    s8 tmp[32];
                    sock.receive(tmp, sizeof(tmp));
                }
            }
        }//for
    } else if (0 == max) {
        //timeout
    } else {
        ecode = System::getAppError();
        if (EE_INTR != ecode) {
            Logger::log(ELL_ERROR, "Loop::run>>Poll=%d events, ecode=%d", max, ecode);
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
            if (0 == nd->mError) {
                nd->mError = hnd->getSock().getError();
                if (0 == nd->mError) {
                    hnd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
                    relinkTime((HandleTime*)hnd);
                } else {
                    closeHandle(hnd);
                }
            } else {
                //@see HandleTCP::connect()
                //已经在发起connect时，closeHandle(hnd);
            }
            nd->mCall(nd);
            unbindFly(hnd);
            break;
        }
        case ERT_READ:
        {
            net::RequestTCP* nd = (net::RequestTCP*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            s32 err = 0;
            StringView buf;
            while (nd) {
                if (EHF_READABLE & hnd->mFlag) {
                    buf = nd->getWriteBuf();
                    s32 rdsz = hnd->mSock.receive(buf.mData, (s32)buf.mLen);
                    if (rdsz > 0) {
                        nd->mError = 0;
                        nd->mUsed += rdsz;
                        if (rdsz < buf.mLen) {
                            hnd->mFlag &= ~EHF_SYNC_READ;
                            nd->mCall(nd);
                            unbindFly(hnd);
                            break;
                        }
                    } else if (0 == rdsz) {
                        err = EE_NO_READABLE;
                        nd->mError = err;
                        hnd->mFlag &= ~EHF_READABLE;
                        closeHandle(hnd);
                    } else {
                        err = System::getAppError();
                        if (EE_INTR == err) {
                            err = 0;
                            continue;
                        } else if (EE_RETRY == err) {
                            hnd->mFlag &= ~EHF_SYNC_READ;
                            hnd->addReadPendingHead(nd);
                            break;
                        } else {
                            nd->mError = err;
                            hnd->mFlag &= ~EHF_READABLE;
                            closeHandle(hnd);
                        }
                    }
                } else {
                    DASSERT(0 != err);
                    //Logger::log(ELL_ERROR, "Loop::updatePending>>read sock=%d", hnd->mSock.getValue());
                    nd->mError = err;
                }

                nd->mCall(nd);
                nd = (net::RequestTCP*)hnd->popReadReq();
                unbindFly(hnd);
            }//while
            relinkTime((HandleTime*)hnd);
            break;
        }
        case ERT_WRITE:
        {
            net::RequestTCP* nd = (net::RequestTCP*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            StringView buf;
            s32 err = 0;
            while (nd) {
                if (EHF_WRITEABLE & hnd->mFlag) {
                    buf = nd->getReadBuf();
                    buf.mData += nd->mStepSize;
                    buf.mLen -= nd->mStepSize;
                    s32 wdsz = hnd->mSock.send(buf.mData, (s32)buf.mLen);
                    if (wdsz > 0) {
                        nd->mError = 0;
                        nd->mStepSize += wdsz;
                        if (nd->mStepSize < nd->mUsed) {
                            hnd->mFlag &= ~EHF_SYNC_WRITE;
                            hnd->addWritePendingHead(nd);
                            break;
                        }
                    } else if (0 == wdsz) {
                        err = System::getAppError();
                        nd->mError = err;
                        hnd->mFlag &= ~EHF_WRITEABLE;
                        closeHandle(hnd);
                    } else {
                        err = System::getAppError();
                        if (EE_INTR == err) {
                            err = 0;
                            continue;
                        } else if (EE_RETRY == err) {
                            hnd->mFlag &= ~EHF_SYNC_WRITE;
                            hnd->addWritePendingHead(nd);
                            break;
                        } else {
                            nd->mError = err;
                            hnd->mFlag &= ~EHF_WRITEABLE;
                            closeHandle(hnd);
                        }
                    }
                } else {
                    DASSERT(0 != err);
                    //Logger::log(ELL_ERROR, "Loop::updatePending>>write sock=%d", hnd->mSock.getValue());
                    nd->mError = err;
                }

                nd->mCall(nd);
                nd = (net::RequestTCP*)hnd->popWriteReq();
                unbindFly(hnd);
            }//while
            relinkTime((HandleTime*)hnd);
            break;
        }
        case ERT_ACCEPT:
        {
            net::RequestAccept* nd = (net::RequestAccept*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            s32 err = 0;
            while (nd) {
                if (EHF_READABLE & hnd->mFlag) {
                    nd->mSocket = hnd->mSock.accept(nd->mRemote);
                    if (nd->mSocket.isOpen()) {
                        nd->mError = 0;
                        nd->mSocket.getLocalAddress(nd->mLocal);
                        //nd->mSocket.getRemoteAddress(nd->mRemote);
                        if (0 != nd->mSocket.setBlock(false)) {
                            nd->mError = System::getAppError();
                            Logger::log(ELL_ERROR, "Loop::updatePending>>accept unblock sock=%d,err=%d",
                                nd->mSocket.getValue(), nd->mError);
                            nd->mSocket.close();
                            //hnd->mFlag &= ~EHF_READABLE;
                        }
                    } else {
                        err = System::getAppError();
                        if (EE_INTR == err) {
                            err = 0;
                            continue;
                        } else if (EE_RETRY == err) {
                            hnd->mFlag &= ~EHF_SYNC_READ;
                            hnd->addReadPendingHead(nd);
                            break;
                        } else {
                            nd->mError = err;
                            closeHandle(hnd);
                            Logger::log(ELL_ERROR, "Loop::updatePending>>accept ecode=%d", err);
                        }
                    }
                } else {
                    DASSERT(0 != err);
                    Logger::log(ELL_ERROR, "Loop::updatePending>>accept fail, sock=%d", nd->mSocket.getValue());
                    nd->mError = err;
                }
                nd->mCall(nd);
                nd = (net::RequestAccept*)hnd->popReadReq();
                unbindFly(hnd);
            } //while
            break;
        }
        default:
            DASSERT(0);
            Logger::log(ELL_ERROR, "Loop::updatePending>>invalid type=%d, handle=%p", req->mType, req);
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

void Loop::start() {
    EventPoller::SEvent evt;
    net::Socket& sock = mPoller.getSocketPair().getSocketB();
    evt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLHUP;
    evt.mData.mPointer = nullptr;
    if (!mPoller.add(sock, evt)) {
        Logger::log(ELL_ERROR, "Loop::start>>mPoller.add socket pair, err=%d", System::getAppError());
    }
}

void Loop::stop() {
    if (0 != mStop) {
        return;
    }
    mStop = 1;

    net::Socket& sock = mPoller.getSocketPair().getSocketB();
    if (!mPoller.remove(sock)) {
        Logger::log(ELL_ERROR, "Loop::start>>mPoller.remove socket pair, err=%d", System::getAppError());
    }

    const Node2* head = &mHandleActive;
    Node2* next = mHandleActive.mNext;
    for (Node2* curr = next; head != curr; curr = next) {
        next = next->mNext;
        closeHandle((Handle*)curr);
    }
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
    Node3* nd;
    for (nd = mTimeHub.getTop(); nd; nd = mTimeHub.getTop()) {
        handle = DGET_HOLDER(nd, HandleTime, mLink);
        if (handle->mTimeout > mTime) {
            s64 diff = handle->mTimeout - mTime;
            return diff > 1000 ? 1000 : (u32)diff;
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
    case EHT_TCP_ACCEPT:
    case EHT_TCP_CONNECT:
    case EHT_TCP_LINK:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        if (nd->mCallTime) {
            mTimeHub.remove(&nd->mLink);
        }
        if (mPoller.remove(nd->getSock())) {
            //TODO>> CLEAR ALL requests
        } else {
            Logger::log(ELL_ERROR, "Loop::closeHandle>>remove=%d, ecode=%d", nd->getSock().getValue(), System::getError());
        }
        nd->close();
        addPendingAll(nd->mReadQueue);
        nd->mReadQueue = nullptr;
        addPendingAll(nd->mWriteQueue);
        nd->mWriteQueue = nullptr;
        break;
    }
    case EHT_TIME:
    {
        HandleTime* nd = reinterpret_cast<HandleTime*>(it);
        mTimeHub.remove(&nd->mLink);
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
    if (mStop > 0) {
        it->mFlag |= EHF_CLOSING;
        return EE_CLOSING;
    }
    DASSERT(it);

    s32 ret = EE_OK;

    it->mFlag = EHF_INIT;
    it->mLoop = this;

    switch (it->mType) {
    case EHT_TCP_ACCEPT:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        net::Socket& sock = nd->getSock();
        if (!sock.openSeniorTCP()) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,tcp open ecode=%d", nd->mLocal.getStr(), ret);
        } else if (0 != sock.setReuseIP(true)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,reIP ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (0 != sock.setReusePort(true)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,rePort ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (0 != sock.bind(nd->mLocal)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,bind ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else if (0 != sock.listen(0x7FFFFFFF)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,listen ecode=%d", nd->mLocal.getStr(), ret);
            sock.close();
        } else {
            EventPoller::SEvent evt;
            evt.mEvent = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
            evt.mData.mPointer = nd;
            if (mPoller.add(sock, evt)) {
                nd->mFlag |= EHF_READABLE;
            } else {
                ret = EE_NO_OPEN;
                sock.close();
            }
        }
        break;
    }
    case EHT_TCP_CONNECT:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        net::Socket& sock = nd->getSock();
        if (!sock.openSeniorTCP()) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,tcp.con open, ecode=%d", nd->mRemote.getStr(), ret);
        } else if (0 != sock.setBlock(false)) {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,tcp.con unblock, ecode=%d", nd->mRemote.getStr(), ret);
            sock.close();
        } else {
            if (nd->mCallTime) {
                nd->mTimeout += Timer::getRelativeTime();
                mTimeHub.insert(&nd->mLink);
            }
        }
        break;
    }
    case EHT_TCP_LINK:
    {
        net::HandleTCP* nd = reinterpret_cast<net::HandleTCP*>(it);
        EventPoller::SEvent evt;
        evt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
        evt.mData.mPointer = nd;
        if (mPoller.add(nd->getSock(), evt)) {
            nd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
            if (nd->mCallTime) {
                nd->mTimeout += Timer::getRelativeTime();
                mTimeHub.insert(&nd->mLink);
            }
        } else {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,tcp.link add, sock=%d, ecode=%d", nd->mRemote.getStr(), nd->getSock().getValue(), ret);
            nd->getSock().close();
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


void Loop::addPendingAll(Request* it) {
    if (it) {
        if (mRequest) {
            Request* head = it->mNext;
            it->mNext = mRequest->mNext;
            mRequest->mNext = head;
        }
        mRequest = it;
    }
}



} //namespace app
