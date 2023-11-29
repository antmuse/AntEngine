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
#include "Timer.h"
#include "System.h"
#include "Engine.h"
#include "Logger.h"
#include "HandleFile.h"
#include "Net/HandleTCP.h"
#include "Net/HandleUDP.h"


namespace app {

Loop::Loop() :
    mTimeHub(HandleTime::lessTime),
    mRequest(nullptr),
    mTime(Timer::getTime()),
    mStop(0),
    mTaskHead(nullptr),
    mTaskHeadIdle(nullptr),
    mTaskIdleCount(0),
    mTaskIdleMax(1000),
    mMaxEvents(128), 
    mPackCMD(1024),
    mFlyRequest(0),
    mGrabCount(0) {
    mEvents = new EventPoller::SEvent[mMaxEvents];
    mTaskTailPos = reinterpret_cast<void**>(&mTaskHead);
}

Loop::~Loop() {
    DASSERT(0 == mGrabCount && 0 == mFlyRequest);
    //mPoller.close();
    delete[] mEvents;
    mEvents = nullptr;
    freeAllTaskNode();
}


s32 Loop::onTimeout(HandleTime& it) {
    DASSERT(mCMD.getGrabCount() > 0);
    //do something?
    return EE_OK;
}

void Loop::onClose(Handle* it) {
    DASSERT(&mCMD == it && "Loop::onClose closed cmd handle?");
    //delete this;
}

void Loop::onRead(RequestFD* it) {
    if (it->mError) {
        Logger::logCritical("Loop::onRead>>pid=%d, CMD read err=%d", Engine::getInstance().getPID(), it->mError);
        stop();
        return;
    }

    mPackCMD.resize(mPackCMD.size() + it->mUsed);
    u32 pksz = 0;
    for (MsgHeader* cmd = (MsgHeader*)mPackCMD.getPointer();
        pksz + sizeof(MsgHeader::mSize) < mPackCMD.size() && cmd->mSize + pksz <= mPackCMD.size();
        cmd = reinterpret_cast<MsgHeader*>((s8*)cmd + cmd->mSize)) {
        switch (cmd->mType) {
        case ECT_EXIT:
            Logger::logInfo("Loop::onCommands>>pid=%d, exit", Engine::getInstance().getPID());
            stop();
            break;
        case ECT_ACTIVE:
            Logger::logInfo("Loop::onCommands>>pid=%d, active", Engine::getInstance().getPID());
            break;
        case ECT_TASK:
        {
            CommandTask* task = reinterpret_cast<CommandTask*>(cmd);
            (*task)();
            break;
        }
        default:
            break;
        } // switch
        pksz += cmd->mSize;
    }
    mPackCMD.clear(pksz);

    if (0 != mStop) {
        return;
    }
    mReadCMD.mData = mPackCMD.getWritePointer();
    mReadCMD.mAllocated = (u32)mPackCMD.getWriteSize();
    mReadCMD.mUsed = 0;
    if (EE_OK != mCMD.read(&mReadCMD)) {
        Logger::logCritical("Loop::onRead>>pid=%d, CMD read err=%d", Engine::getInstance().getPID(), mReadCMD.mError);
        stop();
    }
}


bool Loop::run() {
    s32 ecode = 0;
    u32 timeout = getWaitTime();
    s32 max = mPoller.getEvents(mEvents, mMaxEvents, timeout);
    if (max > 0) {
        for (s32 i = 0; i < max; ++i) {
            u32 eflag = mEvents[i].mEvent;
            if (eflag & (EPOLLERR | EPOLLHUP)) {
                eflag |= (EPOLLIN | EPOLLOUT);
            }
            if (mEvents[i].mData.mPointer) {
                if (&mPoller.getIOURing() == mEvents[i].mData.mPointer) {
                    mPoller.getIOURing().updatePending();
                } else {
                    Handle& han = *(Handle*)(mEvents[i].mData.mPointer);
                    RequestFD* req;
                    if (EPOLLIN & eflag) {
                        req = han.popReadReq();
                        if (req) {
                            addPending(req);
                        } else {
                            han.mFlag |= EHF_SYNC_READ;
                        }
                    }
                    if (EPOLLOUT & eflag) {
                        req = han.popWriteReq();
                        if (req) {
                            addPending(req);
                        } else {
                            han.mFlag |= EHF_SYNC_WRITE;
                        }
                    }
                }
            } else {
                Logger::log(ELL_ERROR, "Loop::run>>Poll null");
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
    return mGrabCount > 0 || mPoller.getIOURing().getSize();
}


void Loop::updatePending() {
    if (nullptr == mRequest) {
        return;
    }
    RequestFD* first = mRequest->mNext;
    RequestFD* next = first;
    mRequest = nullptr;

    for (RequestFD* req = next; next; req = next) {
        next = req->mNext != first ? req->mNext : nullptr;
        switch (req->mType) {
        case ERT_CONNECT:
        {
            RequestFD* nd = (RequestFD*)req;
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
                //�Ѿ��ڷ���connectʱ��closeHandle(hnd);
            }
            nd->mCall(nd);
            unbindFly(hnd);
            break;
        }
        case ERT_READ:
        {
            RequestFD* nd = (RequestFD*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            s32 err = 0;
            StringView buf;
            while (nd) {
                if (EHF_READABLE & hnd->mFlag) {
                    buf = nd->getWriteBuf();
                    s32 rdsz;
                    if (EHT_UDP == hnd->mType) {
                        RequestUDP* ndu = (RequestUDP*)req;
                        if ((1 & ndu->mFlags) > 0) {
                            rdsz = hnd->mSock.receive(buf.mData, (s32)buf.mLen);
                        } else {
                            rdsz = hnd->mSock.receiveFrom(buf.mData, (s32)buf.mLen, ndu->mRemote);
                        }
                    } else { // TCP currently
                        rdsz = hnd->mSock.receive(buf.mData, (s32)buf.mLen);
                    }
                    if (rdsz > 0) {
                        nd->mError = 0;
                        nd->mUsed += rdsz;
                        if (rdsz < buf.mLen) {
                            hnd->mFlag &= ~EHF_SYNC_READ;
                            nd->mCall(nd);
                            unbindFly(hnd);
                            err = EE_RETRY;
                            break;
                        }
                    } else if (0 == rdsz) {
                        err = EE_NO_READABLE;
                        nd->mError = err;
                        hnd->mFlag &= ~(EHF_READABLE | EHF_SYNC_READ);
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
                            hnd->mFlag &= ~(EHF_READABLE | EHF_SYNC_READ);
                            closeHandle(hnd);
                        }
                    }
                } else {
                    nd->mError = EE_NO_READABLE;
                }

                nd->mCall(nd);
                nd = (RequestFD*)hnd->popReadReq();
                unbindFly(hnd);
            }//while

            // set flag for next step
            if ((hnd->mFlag & EHF_READABLE) && EE_RETRY != err) {
                hnd->mFlag |= EHF_SYNC_READ;
            }
            relinkTime((HandleTime*)hnd);
            break;
        }
        case ERT_WRITE:
        {
            RequestFD* nd = (RequestFD*)req;
            net::HandleTCP* hnd = (net::HandleTCP*)(nd->mHandle);
            StringView buf;
            s32 err = 0;
            while (nd) {
                if (EHF_WRITEABLE & hnd->mFlag) {
                    buf = nd->getReadBuf();
                    buf.mData += nd->mStepSize;
                    buf.mLen -= nd->mStepSize;
                    s32 wdsz;
                    if (EHT_UDP == hnd->mType) {
                        RequestUDP* ndu = (RequestUDP*)req;
                        if ((1 & ndu->mFlags) > 0) {
                            wdsz = hnd->mSock.send(buf.mData, (s32)buf.mLen);
                        } else {
                            wdsz = hnd->mSock.sendTo(buf.mData, (s32)buf.mLen, ndu->mRemote);
                        }
                    } else { // TCP currently
                        wdsz = hnd->mSock.send(buf.mData, (s32)buf.mLen);
                    }
                    if (wdsz > 0) {
                        nd->mError = 0;
                        nd->mStepSize += wdsz;
                        if (nd->mStepSize < nd->mUsed) {
                            hnd->mFlag &= ~EHF_SYNC_WRITE;
                            hnd->addWritePendingHead(nd);
                            err = EE_RETRY;
                            break;
                        }
                    } else if (0 == wdsz) {
                        err = System::getAppError();
                        nd->mError = err;
                        hnd->mFlag &= ~(EHF_WRITEABLE | EHF_SYNC_WRITE);
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
                            hnd->mFlag &= ~(EHF_WRITEABLE | EHF_SYNC_WRITE);
                            closeHandle(hnd);
                        }
                    }
                } else {
                    nd->mError = EE_NO_WRITEABLE;
                }

                nd->mCall(nd);
                nd = (RequestFD*)hnd->popWriteReq();
                unbindFly(hnd);
            }//while

            // set flag for next step
            if ((hnd->mFlag & EHF_WRITEABLE) && EE_RETRY != err) {
                hnd->mFlag |= EHF_SYNC_WRITE;
            }
            relinkTime((HandleTime*)hnd);
            break;
        }
        case ERT_ACCEPT:
        {
            RequestAccept* nd = (RequestAccept*)req;
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
                            // Logger::log(ELL_INFO, "Loop::updatePending>>accept retry, ecode=%d", err);
                            break;
                        } else {
                            nd->mError = err;
                            hnd->mFlag &= ~(EHF_READABLE | EHF_SYNC_READ);
                            closeHandle(hnd);
                            Logger::log(ELL_ERROR, "Loop::updatePending>>accept ecode=%d", err);
                        }
                    }
                } else {
                    if(nd->mSocket.isOpen()){
                        Logger::log(ELL_ERROR, "Loop::updatePending>>accept fail, sock=%d", nd->mSocket.getValue());
                    }
                    nd->mError = EE_NO_READABLE;
                }
                nd->mCall(nd);
                nd = (RequestAccept*)hnd->popReadReq();
                unbindFly(hnd);
            } //while
            
            // set flag for next step
            if ((hnd->mFlag & EHF_READABLE) && EE_RETRY != err) {
                hnd->mFlag |= EHF_SYNC_READ;
            }
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
    mTime = Timer::getTime();
    if (mRequest || !mHandleClose.empty()) {
        return 0;
    }
    return updateTimeHub();
}


void Loop::bindHandle(Handle* it) {
    ++Engine::getInstance().getEngineStats().mTotalHandles;
    it->grab();
    grab();
}


void Loop::unbindHandle(Handle* it) {
    if (0 == it->drop() && 0 == it->getFlyRequest()) {
        addClose(it);
    }
}


void Loop::bindFly(Handle* it) {
    ++Engine::getInstance().getEngineStats().mFlyRequests;
    it->grabFlyReq();
    grabFlyReq();
}


void Loop::unbindFly(Handle* it) {
    --Engine::getInstance().getEngineStats().mFlyRequests;
    dropFlyReq();
    if (0 == it->dropFlyReq() && 0 == it->getGrabCount()) {
        addClose(it);
    }
}


bool Loop::start(net::Socket& sock, net::Socket& sockW) {
    mCMD.mSock = sock;
    mCMD.setClose(EHT_TCP_LINK, LoopOnClose, this);
    mCMD.setTime(LoopOnTime, 15 * 1000, 20 * 1000, -1);

    if(!mPoller.open()) {
        sock.close();
        sockW.close();
        return false;
    }
    if (0 != mCMD.mSock.setBlock(false)) {
        sock.close();
        sockW.close();
        return false;
    }
    if (EE_OK != openHandle(&mCMD)) {
        sock.close();
        sockW.close();
        return false;
    }
    mReadCMD.mType = ERT_READ;
    mReadCMD.mError = 0;
    mReadCMD.mHandle = &mCMD;
    mReadCMD.mUser = this;
    mReadCMD.mCall = LoopOnRead;
    mReadCMD.mData = mPackCMD.getWritePointer();
    mReadCMD.mAllocated = (u32)mPackCMD.getWriteSize();
    mReadCMD.mUsed = 0;

    if (EE_OK != mCMD.read(&mReadCMD)) {
        Logger::log(ELL_ERROR, "Loop::start>>cmd recv, ecode=%d", mReadCMD.mError);
        return false;
    }
    mSendCMD = sockW;
    sock = DINVALID_SOCKET;
    sockW = DINVALID_SOCKET;
    return true;
}


void Loop::stop() {
    if (0 != mStop) {
        return;
    }
    mStop = 1;
    Logger::flush();

    const Node2* head = &mHandleActive;
    Node2* next = mHandleActive.mNext;
    for (Node2* curr = next; head != curr; curr = next) {
        next = next->mNext;
        closeHandle((Handle*)curr);
    }
    mCMD.mSock.close();
    mSendCMD.close();
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
    --Engine::getInstance().getEngineStats().mTotalHandles;
    it->delink();
    mHandleClose.pushFront(*it);
}


s32 Loop::closeHandle(Handle* it) {
    DASSERT(it);
    if (it->isClosing() || !it->isOpen()) {
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
            Logger::log(ELL_ERROR, "Loop::closeHandle>>remove tcp=%d, ecode=%d", nd->getSock().getValue(), System::getError());
        }
        nd->close();
        addPendingAll(nd->mReadQueue);
        nd->mReadQueue = nullptr;
        addPendingAll(nd->mWriteQueue);
        nd->mWriteQueue = nullptr;
        break;
    }
    case EHT_UDP:
    {
        net::HandleUDP* nd = reinterpret_cast<net::HandleUDP*>(it);
        if (nd->mCallTime) {
            mTimeHub.remove(&nd->mLink);
        }
        if (!mPoller.remove(nd->getSock())) {
            Logger::log(ELL_ERROR, "Loop::closeHandle>>remove udp=%d, ecode=%d", nd->getSock().getValue(), System::getError());
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
        unbindHandle(it);
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
            evt.mEvent = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLEXCLUSIVE;
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
            EventPoller::SEvent evt;
            evt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
            evt.mData.mPointer = nd;
            if (mPoller.add(nd->getSock(), evt)) {
                nd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
                if (nd->mCallTime) {
                    nd->mTimeout += Timer::getTime();
                    mTimeHub.insert(&nd->mLink);
                }
            } else {
                ret = System::getAppError();
                Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,tcp.conn add, sock=%d, ecode=%d",
                    nd->mRemote.getStr(), nd->getSock().getValue(), ret);
                nd->getSock().close();
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
                nd->mTimeout += Timer::getTime();
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
        nd->mTimeout += Timer::getTime();
        mTimeHub.insert(&nd->mLink);
        break;
    }
    case EHT_FILE:
    {
        HandleFile* nd = reinterpret_cast<HandleFile*>(it);
        //Logger::log(ELL_ERROR, "Loop::openHandle>>file=%s, ecode=%d", nd->getFileName().c_str(), ret);
        nd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
        if (nd->mCallTime) {
            nd->mTimeout += Timer::getTime();
            mTimeHub.insert(&nd->mLink);
        }
        break;
    }
    case EHT_UDP:
    {
        net::HandleUDP* nd = reinterpret_cast<net::HandleUDP*>(it);
        EventPoller::SEvent evt;
        evt.mEvent = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
        evt.mData.mPointer = nd;
        if (mPoller.add(nd->getSock(), evt)) {
            nd->mFlag |= (EHF_READABLE | EHF_WRITEABLE | EHF_SYNC_WRITE);
            if (nd->mCallTime) {
                nd->mTimeout += Timer::getTime();
                mTimeHub.insert(&nd->mLink);
            }
        } else {
            ret = System::getAppError();
            Logger::log(ELL_ERROR, "Loop::openHandle>>addr=%s,udp add, sock=%d, ecode=%d", nd->mRemote.getStr(), nd->getSock().getValue(), ret);
            nd->getSock().close();
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


void Loop::addPending(RequestFD* it) {
    if (mRequest) {
        it->mNext = mRequest->mNext;
        mRequest->mNext = it;
    } else {
        it->mNext = it;
    }
    mRequest = it;
}


s32 Loop::postTask(const MsgHeader& task) {
    /** @note: keep send is thread/process safe
     *  @see Engine::postCommand
     */
    return task.mSize == mSendCMD.send(&task, task.mSize) ? EE_OK : EE_ERROR;
}


s32 Loop::postTask(TaskNode* task) {
    if (!task || isStop()) {
        return EE_ERROR;
    }

    mTaskLock.lock();
    bool active = (nullptr == mTaskHead);
    void** link = (void**)((s8*)task + DOFFSET(TaskNode, mNext));
    *link = nullptr;
    *mTaskTailPos = (void*)link;
    mTaskTailPos = (void**)link;
    mTaskLock.unlock();

    if (active) {
        CommandTask activeTask;
        activeTask.pack(&Loop::onTask, this, (void*)nullptr);
        if (activeTask.mSize != mSendCMD.send(&activeTask, activeTask.mSize)) {
            Logger::log(ELL_ERROR, "Loop::postTask>>send cmd, failed to active the loop");
        }
    }
    return EE_OK;
}


void Loop::onTask(void* it) {
    void** head = nullptr;

    mTaskLock.lock();
    if (mTaskHead) {
        head = (void**)(mTaskHead);
        //clear queue
        mTaskHead = nullptr;
        mTaskTailPos = reinterpret_cast<void**>(&mTaskHead);
    }
    mTaskLock.unlock();

    //process all tasks
    while (head) {
        TaskNode* nd = reinterpret_cast<TaskNode*>((s8*)head - DOFFSET(TaskNode, mNext));
        (*nd)();
        head = (void**)(*head); //next
        pushTaskNode(nd);
    }
}


s32 Loop::postRequest(RequestFD* it) {
    DASSERT(it && it->mHandle);
    if (!it || isStop()) {
        return EE_ERROR;
    }
    switch (it->mHandle->getType()) {
    case EHT_FILE:
        return mPoller.getIOURing().postReq(it);
    default:
        break;
    }
    return EE_ERROR;
}

} //namespace app
