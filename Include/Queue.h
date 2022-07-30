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
#include "Config.h"

namespace app {


class Queue {
public:
    /**
     * @brief [多生产者-多消费者]类型的消息队列, 不管理消息的内存且链表指针需在消息上
     * @param maxlen 写队列最大数量
     * @param linkOFF 链表指针在msg上的相对位置
     */
    Queue(s32 linkOFF, usz maxlen) :
        mLinkOFF(linkOFF),
        mMsgMax(maxlen),
        mMsgCount(0),
        mNoBlock(0),
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

    void setNoBlock() {
        mNoBlock = 1;
        mMutexWrite.lock();
        mConditionRead.notify_all();
        mConditionWrite.notify_all();
        mMutexWrite.unlock();
    }

    void setBlock() {
        mNoBlock = 0;
    }


    void writeMsg(void* msg) {
        void** link = (void**)((s8*)msg + mLinkOFF);
        *link = nullptr;
        {
            std::unique_lock<std::mutex> ak(mMutexWrite);
            while (mMsgCount > mMsgMax - 1 && !mNoBlock) {
                mConditionWrite.wait(ak);
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
            while (0 == mMsgCount && !mNoBlock) {
                mConditionRead.wait(ak);
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


    //1=则生产者队列最大长度为mMsgMax, 0=不限制最大长度
    s32 mNoBlock;
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