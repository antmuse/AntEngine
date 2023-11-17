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


#ifndef APP_LOOP_H
#define	APP_LOOP_H

#include "Nocopy.h"
#include "Strings.h"
#include "MsgHeader.h"
#include "BinaryHeap.h"
#include "Spinlock.h"
#include "ThreadPool.h"
#include "Handle.h"
#include "Net/HandleTCP.h"
#include "Packet.h"
#include "Net/EventPoller.h"

#if defined(DOS_WINDOWS)
#include "Windows/Request.h"
#else
#include "Linux/Request.h"
#endif

namespace app {

class Loop : public Nocopy {
public:
    Loop();

    ~Loop();

    bool run();

    bool isStop()const {
        return mStop > 0;
    }

    bool start(net::Socket& sockRead, net::Socket& sockWrite);

    void stop();

    /**
    * @return Handle count
    */
    s32 getHandleCount()const {
        return mGrabCount;
    }

    s32 grab()const {
        return ++mGrabCount;
    }

    s32 drop()const {
        DASSERT(mGrabCount > 0);
        return --mGrabCount;
    }

    s32 getFlyRequest()const {
        return mFlyRequest;
    }

    s32 grabFlyReq()const {
        return ++mFlyRequest;
    }

    s32 dropFlyReq() const {
        DASSERT(mFlyRequest > 0);
        return --mFlyRequest;
    }

    BinaryHeap& getTimeHeap() {
        return mTimeHub;
    }

    s64 getTime()const {
        return mTime;
    }

    void setTime(s64 it) {
        mTime = it;
    }

    /**
    * @return EE_OK if success, else ecode. */
    s32 openHandle(Handle* it);

    s32 closeHandle(Handle* it);

    EventPoller& getEventPoller() {
        return mPoller;
    }

    template <class P>
    s32 postTask(void (*func)(P*), P* dat) {
        TaskNode* task = popTaskNode();
        task->pack(func, dat);
        if (EE_OK != postTask(task)) {
            pushTaskNode(task);
            return EE_ERROR;
        }
        return EE_OK;
    }

    template <class T, class P>
    s32 postTask(void (T::*func)(P*), const void* it, P* dat) {
        TaskNode* task = popTaskNode();
        task->pack(func, it, dat);
        if (EE_OK != postTask(task)) {
            pushTaskNode(task);
            return EE_ERROR;
        }
        return EE_OK;
    }

    s32 postTask(const MsgHeader& task);

    void addPending(RequestFD* it);

    void bindFly(Handle* it);
    void unbindFly(Handle* it);

#if defined(DOS_LINUX) || defined(DOS_ANDROID)
    s32 postRequest(RequestFD* it);
#endif

protected:
    void updatePending();
    u32 updateTimeHub();
    void updateClosed();
    u32 getWaitTime();
    void addClose(Handle* it);
    void relinkTime(HandleTime* it);

    void bindHandle(Handle* it);
    void unbindHandle(Handle* it);

    //linux
    void addPendingAll(RequestFD* it) {
        if (it) {
            if (mRequest) {
                RequestFD* head = it->mNext;
                it->mNext = mRequest->mNext;
                mRequest->mNext = head;
            }
            mRequest = it;
        }
    }

private:
    s64 mTime;
    mutable s32 mFlyRequest;
    mutable s32 mGrabCount; //=HandleCount
    s32 mMaxEvents;
    s32 mStop;
    BinaryHeap mTimeHub;    //最小堆用于管理超时事件
    Node2 mHandleActive;
    Node2 mHandleClose;
    RequestFD* mRequest;
    EventPoller mPoller;
    EventPoller::SEvent* mEvents;

    // for task queue
    Spinlock mTaskLock;
    Spinlock mTaskIdleLock;
    void** mTaskTailPos;
    TaskNode* mTaskHead;
    TaskNode* mTaskHeadIdle;
    s32 mTaskIdleCount;
    s32 mTaskIdleMax;

    net::HandleTCP mCMD;
    Packet mPackCMD;
    RequestFD mReadCMD;

    net::Socket mSendCMD;

    //cmd callbacks for Loop
    static s32 LoopOnTime(HandleTime* it) {
        Loop& nd = *(Loop*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void LoopOnRead(RequestFD* it) {
        Loop& nd = *(Loop*)it->mUser;
        nd.onRead(it);
    }

    static void LoopOnClose(Handle* it) {
        Loop& nd = *(Loop*)it->getUser();
        nd.onClose(it);
    }

    //cmd read
    void onRead(RequestFD* it);
    //cmd close
    void onClose(Handle* it);
    //cmd timeout
    s32 onTimeout(HandleTime& it);

    void onTask(void* it);
    s32 postTask(TaskNode* task);

    TaskNode* popTaskNode() {
        TaskNode* ret = nullptr;

        mTaskIdleLock.lock();
        if (mTaskHeadIdle) {
            ret = mTaskHeadIdle;
            mTaskHeadIdle = ret->mNext;
            --mTaskIdleCount;
        }
        mTaskIdleLock.unlock();

        if (ret) {
            ret->clear();
            return ret;
        }
        return new TaskNode();
    }

    void pushTaskNode(TaskNode* it) {
        DASSERT(it);

        mTaskIdleLock.lock();
        if (mTaskIdleCount < mTaskIdleMax) {
            it->mNext = mTaskHeadIdle;
            mTaskHeadIdle = it;
            ++mTaskIdleCount;
            it = nullptr;
        }
        mTaskIdleLock.unlock();

        if (it) {
            delete it;
        }
    }

    void freeAllTaskNode() {
        mTaskIdleLock.lock();
        for (TaskNode* nd = mTaskHeadIdle; nd; nd = mTaskHeadIdle) {
            mTaskHeadIdle = nd->mNext;
            delete nd;
        }
        mTaskIdleCount = 0;
        mTaskIdleLock.unlock();
    }
};

} //namespace app

#endif //APP_LOOP_H