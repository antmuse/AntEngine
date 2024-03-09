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


#ifndef APP_QUEUE_H
#define	APP_QUEUE_H

#include <mutex>
#include <condition_variable>
#include "RefCount.h"

namespace app {

/**
 * @brief 通用的队列结点, 带引用计数
 */
class QueueNode {
public:
    u16 mUsed;
    u16 mAllocated;  //must <= 64K = 2^16 = 65535
    std::atomic<s32> mRefCount;
    QueueNode* mNext;
    //s8 mBuffer[0];

    template <class T>
    T* getData()const {
        return reinterpret_cast<T*>(((s8*)(this)) + sizeof(*this));
    }

    static s32 getLinkOffset() {
        return static_cast<s32>(DOFFSET(QueueNode, mNext));
    }

    /** @param bsz should be <=64K */
    static QueueNode* createNode(usz bsz) {
        bsz &= 0xFFFF;
        QueueNode* ret = reinterpret_cast<QueueNode*>(new s8[bsz + sizeof(QueueNode)]);
        ret->mUsed = 0;
        ret->mAllocated = static_cast<u16>(bsz);
        ret->mRefCount = 1;
        //ret->mNext = 0; managered by queue
        return ret;
    }

    static void releaseNode(void* it) {
        delete[] reinterpret_cast<s8*>(it);
    }
};


class Queue : public RefCountAtomic {
public:
    /**
     * @brief [多生产者-多消费者]类型的消息队列, 不管理消息的内存且链表指针需在消息上
     * @param maxlen 写队列最大数量
     * @param linkOFF 链表指针在msg上的相对位置
     */
    Queue(s32 linkOFF, usz maxlen) :
        mLinkOFF(linkOFF),
        mMsgMax(AppClamp(maxlen, 8ULL, 0x0FFFFFFFULL)),
        mMsgCount(0),
        mBlockFlag(1 | 2),
        mHeadA(nullptr),
        mHeadB(nullptr) {
        mHeadRead = &mHeadA;
        mHeadWrite = &mHeadB;
        mTailWrite = &mHeadB;
    }

    ~Queue() {
    }

    usz getMsgCount()const {
        return mMsgCount;
    }

    void setBlock(bool block) {
        std::atomic<s32>& flag = *reinterpret_cast<std::atomic<s32>*>(&mBlockFlag);
        if (block) {
            flag = (1 | 2);
        } else {
            mMutexWrite.lock();
            mConditionRead.notify_one();
            mConditionWrite.notify_all();
            mMutexWrite.unlock();
        }
    }

    void setBlockRead(bool block) {
        std::atomic<s32>& flag = *reinterpret_cast<std::atomic<s32>*>(&mBlockFlag);
        if (block) {
            flag |= 1;
        } else {
            flag &= ~1;
            mMutexRead.lock();
            mConditionRead.notify_one();
            mMutexRead.unlock();
        }
    }

    void setBlockWrite(bool block) {
        std::atomic<s32>& flag = *reinterpret_cast<std::atomic<s32>*>(&mBlockFlag);
        if (block) {
            flag |= 2;
        } else {
            flag &= ~2;
            mMutexWrite.lock();
            mConditionWrite.notify_all();
            mMutexWrite.unlock();
        }
    }

    void writeMsg(void* msg) {
        void** link = (void**)((s8*)msg + mLinkOFF);
        *link = nullptr;
        {
            std::unique_lock<std::mutex> ak(mMutexWrite);
            while (mMsgCount > mMsgMax - 1 && isBlockWrite()) {
                mConditionWrite.wait(ak); //maybe more than 1 writer waiting here
            }
            *mTailWrite = link;
            mTailWrite = link;
            ++mMsgCount;
        }
        mConditionRead.notify_one();
    }

    void* readMsg() {
        void* msg = nullptr;

        mMutexRead.lock();
        if (*mHeadRead || swapQueue() > 0) {
            msg = (s8*)*mHeadRead - mLinkOFF;
            *mHeadRead = *(void**)*mHeadRead;
            //*(void**)((s8*)(msg)+mLinkOFF) = nullptr;
        }
        mMutexRead.unlock();
        return msg;
    }

    void* getHead() const {
        s8* msg = reinterpret_cast<s8*>(*mHeadRead);
        return msg ? (msg - mLinkOFF) : nullptr;
    }

private:
    Queue(const Queue&) = delete;
    Queue(const Queue&&) = delete;
    const Queue& operator=(const Queue&) = delete;
    const Queue& operator=(const Queue&&) = delete;

    usz swapQueue() {
        void** headrd = mHeadRead;
        usz cnt;

        mHeadRead = mHeadWrite;
        {
            //同时持有消费者锁和生产者锁
            std::unique_lock<std::mutex> ak(mMutexWrite);
            while (0 == mMsgCount && isBlockRead()) {
                mConditionRead.wait(ak); //only 1 reader waiting here
            }
            cnt = mMsgCount;
            if (cnt > mMsgMax - 1) {
                mConditionWrite.notify_all();
            }
            mHeadWrite = headrd;
            mTailWrite = headrd;
            mMsgCount = 0;
        }
        return cnt;
    }

    bool isBlockRead()const {
        return (mBlockFlag & 1) > 0;
    }
    bool isBlockWrite()const {
        return (mBlockFlag & 2) > 0;
    }
    /**
     * 1=消费者阻塞
     * 2=则生产者队列最大长度为mMsgMax时阻塞, 0=不限制最大长度且不阻塞
     * 0=皆不阻塞
     */
    s32 mBlockFlag;

    s32 mLinkOFF;
    usz mMsgMax;
    usz mMsgCount;
    void* mHeadA;
    void* mHeadB;
    void** mHeadRead;
    void** mHeadWrite;
    void** mTailWrite;
    std::mutex mMutexRead;
    std::mutex mMutexWrite;
    std::condition_variable mConditionRead;
    std::condition_variable mConditionWrite;
};

} //namespace app

#endif //APP_QUEUE_H