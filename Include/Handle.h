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


#ifndef APP_HANDLE_H
#define	APP_HANDLE_H

#include <string.h>
#include <string>
#include "Node.h"
#include "Nocopy.h"
#include "Logger.h"


namespace app {
class Loop;
class Handle;
class HandleTime;
class RequestFD;

typedef void (*FunCloseCallback)(Handle*);

/**
 * @note 此函数被调用时，此时间句柄还没有从时间管理器中移出
 *       在此回调中Loop::closeHandle(HandleTime)后, 不可返回0;
 * @return 0 if go on, else close and remove this HandleTime from time manager.
 */
typedef s32(*FunTimeCallback)(HandleTime*);


enum EHandleType {
    EHT_UNKNOWN = 0,
    EHT_TIME = 1,
    EHT_TCP_ACCEPT = 2,
    EHT_TCP_CONNECT = 3,
    EHT_TCP_LINK = 4,
    EHT_UDP = 5,
    EHT_FILE = 6,
    EHT_COUNT
};


enum EHandleFlag {
    EHF_OPEN = 0x00000001,
    EHF_CLOSING = 0x00000002,
    EHF_CLOSE = 0x00000004,
    EHF_READABLE = 0x00000008,
    EHF_WRITEABLE = 0x00000010,
    EHF_SYNC_READ = 0x00000020,
    EHF_SYNC_WRITE = 0x00000040,
    EHF_INIT = 0x0
};


class Handle : public Node2 {
public:
    Handle() :
        mFlag(EHF_INIT),
        mGrabCount(0),
        mFlyRequest(0),
        mLoop(nullptr),
        mUser(nullptr),
        mCallClose(nullptr),
        mType(EHT_UNKNOWN) {
#if defined(DOS_ANDROID) || defined(DOS_LINUX)
        mReadQueue = nullptr;
        mWriteQueue = nullptr;
#endif
    }

    ~Handle() {
        DASSERT(0 == mGrabCount && 0 == mFlyRequest && "~Handle");
    }

    void setClose(EHandleType tp, FunCloseCallback fc, void* user) {
        mType = tp;
        mCallClose = fc;
        mUser = user;
    }

    void* getUser()const {
        return mUser;
    }

    bool isClosing()const {
        return (EHF_CLOSING & mFlag) > 0;
    }

    bool isOpen()const {
        return (EHF_OPEN & mFlag) > 0;
    }

    bool isClose()const {
        return (EHF_CLOSE & mFlag) > 0;
    }

    s32 getGrabCount()const {
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

    s32 dropFlyReq()const {
        DASSERT(mFlyRequest > 0);
        return --mFlyRequest;
    }

    Loop* getLoop()const {
        return mLoop;
    }

    s32 launchClose();

    s32 getType() const {
        return mType;
    }

protected:

#if defined(DOS_ANDROID) || defined(DOS_LINUX)
    RequestFD* mReadQueue;  //point to tail,mReadQueue->mNext is head
    RequestFD* mWriteQueue; //point to tail,mReadQueue->mNext is head
    RequestFD* popReadReq();
    RequestFD* popWriteReq();
    void addReadPendingTail(RequestFD* it);
    void addReadPendingHead(RequestFD* it);
    void addWritePendingTail(RequestFD* it);
    void addWritePendingHead(RequestFD* it);
#endif

    mutable s32 mGrabCount;
    mutable s32 mFlyRequest;
    mutable u32 mFlag;
    s32 mType;
    Loop* mLoop;
    void* mUser;
    FunCloseCallback mCallClose;

    friend class Loop;
    Handle(const Handle&) = delete;
    Handle(const Handle&&) = delete;
    const Handle& operator=(const Handle&) = delete;
    const Handle& operator=(const Handle&&) = delete;
};


class HandleTime : public Handle {
public:
    HandleTime() :
        mRepeat(0),
        mTimeout(0),
        mTimeGap(0),
        mCallTime(nullptr) {
        mType = EHT_TIME;
    }

    ~HandleTime() { }

    /**
    * @brief 时间单位毫秒
    * @param fn 时间回调函数
    * @param firstTimeGap 首次超时时间(相对当前时间)
    * @param gap 时间间隔
    * @param repeat 大于0为重复次数，等于0则只一次，小于0则永远重复
    */
    void setTime(FunTimeCallback fn, s64 firstTimeGap, s64 gap, s32 repeat) {
        mTimeout = firstTimeGap;
        mTimeGap = gap;
        mRepeat = repeat;
        mCallTime = fn;
    }

    void setTimeCaller(FunTimeCallback fn) {
        mCallTime = fn;
    }

    FunTimeCallback getTimeCaller()const {
        return mCallTime;
    }

    Node3& getLink() {
        return mLink;
    }

    u32 getFlag()const {
        return mFlag;
    }

    void setTimeout(s64 it) {
        mTimeout = it;
    }

    s64 getTimeout()const {
        return mTimeout;
    }

    void setTimeGap(s64 gap) {
        mTimeGap = gap;
    }

    s64 getTimeGap()const {
        return mTimeGap;
    }

    bool operator<(const HandleTime& val)const {
        return mTimeout < val.mTimeout;
    }

    static bool lessTime(const Node3* va, const Node3* vb) {
        const HandleTime& tva = *DGET_HOLDER(va, HandleTime, mLink);
        const HandleTime& tvb = *DGET_HOLDER(vb, HandleTime, mLink);
        return (tva < tvb);
    }

protected:
    friend class Loop;

    s32 mRepeat;  // <0为永循环, >0为循环次数, 0为触发一次
    Node3 mLink;
    s64 mTimeout;
    s64 mTimeGap;
    FunTimeCallback mCallTime;
};


} //namespace app

#endif //APP_HANDLE_H