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


#ifndef APP_THREADPOOL_H
#define APP_THREADPOOL_H

#include "Config.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace app {

using FuncVoid = void (*)();
using FuncTask = void (*)(void*);
using FuncTaskClass = void (*)(const void*, void*);


class TaskNode {
public:
    TaskNode* mNext;
    FuncTask mCall;
    const void* mThis;
    void* mData;
    TaskNode() : mNext(nullptr), mCall(nullptr), mThis(nullptr), mData(nullptr) {
    }

    ~TaskNode() {
    }

    void clear() {
        mNext = nullptr;
        mCall = nullptr;
        mThis = nullptr;
        mData = nullptr;
    }

    template <class P>
    void pack(void (*func)(P*), P* dat) {
        mCall = func;
        mThis = nullptr;
        mData = dat;
    }

    template <class T, class P>
    void pack(void (T::*func)(P*), const void* it, P* dat) {
        void* fff = reinterpret_cast<void*>(&func);
        mCall = *(FuncTask*)fff;
        mThis = it;
        mData = dat;
    }

    void operator()() {
        if (mThis) {
            ((FuncTaskClass)mCall)(mThis, mData);
        } else {
            mCall(mData);
        }
    }
};


class ThreadPool {
public:
    ThreadPool() :
        mRunning(false), mTaskCount(0), mAllocated(0), mMaxAllocated(1000), mAllTask(nullptr), mIdleTask(nullptr),
        mThreadInit(nullptr), mThreadUninit(nullptr) {
    }

    ThreadPool(u32 cnt, FuncVoid init = nullptr, FuncVoid uninit = nullptr) :
        mRunning(false), mTaskCount(0), mAllocated(0), mMaxAllocated(1000), mAllTask(nullptr), mIdleTask(nullptr),
        mThreadInit(init), mThreadUninit(uninit) {
        start(cnt);
    }

    ~ThreadPool() {
        stop();
        clear();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;

    /**
     * @brief 在ThreadPool::start前设置每个线程需要调用的初始化或清理函数.
     * @param init 线程开始时回调函数
     * @param uninit 线程结束时回调函数
     */
    void setThreadCalls(FuncVoid init, FuncVoid uninit) {
        mThreadInit = init;
        mThreadUninit = uninit;
    }

    s64 getTaskCount() const {
        return mTaskCount.load();
    }

    u32 getAllocated() const {
        std::unique_lock<std::mutex> ak(mMutex);
        return mAllocated;
    }

    /**
     * @note 仅支持普通的类成员函数, 不支持虑函数
     * @param func 类成员函数, 不支持虑函数
     * @param hold 类对象地址, 即this指针
     * @param dat 回调参数
     * @param urgent 优先级
     * @return ture if success, else failed.
     */
    template <class T, class P>
    bool postTask(void (T::*func)(P*), const T* hold, P* dat, bool urgent = false) {
        std::unique_lock<std::mutex> ak(mMutex);
        if (!mRunning) {
            return false;
        }
        TaskNode* task = popFreeTask();
        task->mThis = hold;
        void* fff = reinterpret_cast<void*>(&func);
        task->mCall = *(FuncTask*)fff;
        task->mData = dat;
        pushTask(task, urgent);
        mCondition.notify_one();
        return true;
    }

    template <class P>
    bool postTask(void (*func)(P*), P* dat, bool urgent = false) {
        std::unique_lock<std::mutex> ak(mMutex);
        if (!mRunning) {
            return false;
        }
        TaskNode* task = popFreeTask();
        task->mCall = reinterpret_cast<FuncTask>(func);
        task->mData = dat;
        pushTask(task, urgent);
        mCondition.notify_one();
        return true;
    }

    void setMaxAllocated(u32 it) {
        if (it > 0) {
            std::unique_lock<std::mutex> ak(mMutex);
            mMaxAllocated = it < 0x0FFFFFFF ? it : 0x0FFFFFFF;
        }
    }


    void start(u32 threads) {
        {
            std::unique_lock<std::mutex> ak(mMutex);
            if (mRunning) {
                return;
            }
            mRunning = true;
        }
        for (u32 i = 0; i < threads; ++i) {
            mWorkers.emplace_back(&ThreadPool::run, this, i);
        }
    }

    void stop() {
        {
            std::unique_lock<std::mutex> ak(mMutex);
            if (!mRunning) {
                return;
            }
            mRunning = false;
        }
        mCondition.notify_all();
        for (std::thread& worker : mWorkers) {
            worker.join();
        }
        mWorkers.resize(0);
    }


private:
    std::vector<std::thread> mWorkers;
    mutable std::mutex mMutex;
    std::condition_variable mCondition;
    std::atomic<s64> mTaskCount;
    u32 mAllocated;
    u32 mMaxAllocated;
    bool mRunning;
    TaskNode* mAllTask;  // 单向环型链表，指向队尾
    TaskNode* mIdleTask; // 单向链表
    FuncVoid mThreadInit;
    FuncVoid mThreadUninit;

    void run(u32 idx) {
        // std::this_thread::get_id();
        // std::thread.native_handle();

        // printf("ThreadPool::run>>start thread[%d] id=%u\n", idx, std::this_thread::get_id());
        if (mThreadInit) {
            mThreadInit();
        }
        TaskNode* task = nullptr;
        for (;;) {
            {
                std::unique_lock<std::mutex> ank(mMutex);
                if (task) {
                    pushFreeTask(task);
                }
                while (mRunning && !mAllTask) {
                    mCondition.wait(ank);
                }
                if (!mRunning && !mAllTask) {
                    break;
                }
                task = popTask();
            }
            // do it
            (*task)();
        }
        if (mThreadUninit) {
            mThreadUninit();
        }
        // printf("ThreadPool::run>>stop thread[%d] id=%u\n", idx, std::this_thread::get_id());
    }

    TaskNode* popFreeTask() {
        ++mTaskCount;
        if (mIdleTask) {
            TaskNode* ret = mIdleTask;
            mIdleTask = ret->mNext;
            ret->mThis = nullptr;
            ret->mNext = nullptr;
            return ret;
        } else {
            ++mAllocated;
            return new TaskNode();
        }
    }

    void pushFreeTask(TaskNode* it) {
        --mTaskCount;
        if (mAllocated > mMaxAllocated) {
            --mAllocated;
            delete it;
            return;
        }
        it->mNext = mIdleTask;
        mIdleTask = it;
    }

    TaskNode* popTask() {
        if (mAllTask) {
            TaskNode* head = mAllTask->mNext;
            if (head == mAllTask) {
                mAllTask = nullptr;
            } else {
                mAllTask->mNext = head->mNext;
            }
            return head;
        }
        return nullptr;
    }

    void pushTask(TaskNode* it, bool urgent) {
        if (mAllTask) {
            it->mNext = mAllTask->mNext;
            mAllTask->mNext = it;
            if (!urgent) { // push on tail
                mAllTask = it;
            }
        } else {
            it->mNext = it;
            mAllTask = it;
        }
    }

    void clear() {
        // u32 cnt = 0;
        TaskNode* head = mIdleTask;
        while (head) {
            mIdleTask = head->mNext;
            delete head;
            head = mIdleTask;
            //++cnt;
        }
        /*if (cnt != mAllocated) {
            printf("err\n");
        }*/
        mAllocated = 0;
    }
};

} // namespace app


#endif // APP_THREADPOOL_H
