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


#ifndef APP_REQUEST_H
#define	APP_REQUEST_H

#include "Config.h"
#if defined(DOS_WINDOWS)
#include <winsock2.h>
#include <Windows.h>
#include <MSWSock.h>
#endif
#include "Nocopy.h"
#include "Strings.h"
#include "Net/NetAddress.h"

namespace app {
class Handle;
namespace net {
class RequestTCP;
}
typedef void (*FunReqTcpCallback)(net::RequestTCP*);

enum ERequestType {
    ERT_UNKNOWN = 0,
    ERT_CONNECT = 1,
    ERT_ACCEPT = 2,
    ERT_READ = 3,
    ERT_WRITE = 4,
    ERT_COUNT
};

class Request : public Nocopy {
public:
    Request() :
        mNext(nullptr),
        mHandle(nullptr),
        mType(ERT_UNKNOWN),
        mError(0),
        mUser(nullptr) {
    }

    ~Request() { }

    Request* mNext;
    Handle* mHandle;
    void* mUser;
    s32 mType;      //ERequestType
    s32 mError;     //0=success,else failed code

    void* getUser()const {
        return mUser;
    }

#if defined(DOS_WINDOWS)
    OVERLAPPED mOverlapped;
    void clearOverlap() {
        memset(&mOverlapped, 0, sizeof(mOverlapped));
    }

    // @return 此次读写字节数.
    u32 getStepSize()const {
        return ((u32)mOverlapped.InternalHigh);
    }

    void setStepSize(u32 it) {
        mOverlapped.InternalHigh = it;
    }
#else
    u32 mStepSize;
    // @return 此次读写字节数.
    u32 getStepSize()const {
        return mStepSize;
    }
    void setStepSize(u32 it) {
        mStepSize = it;
    }
#endif
};



namespace net {

class RequestTCP : public Request {
public:
    static RequestTCP* newRequest(u32 cache_size) {
        RequestTCP* it = (RequestTCP*)(new s8[sizeof(RequestTCP) + cache_size]);
        new ((void*)it) RequestTCP();
        it->mAllocated = cache_size;
        it->mData = (s8*)(it + 1);
        return it;
    }

    static void delRequest(net::RequestTCP* it) {
        delete it;
    }

    RequestTCP() {
        clear();
    }

    ~RequestTCP() { }

    void clear() {
        memset(this, 0, sizeof(*this));
    }

    s8* getBuf()const {
        return mData;
    }

    StringView getReadBuf()const {
        StringView ret(mData, mUsed);
        return ret;
    }

    StringView getWriteBuf()const {
        StringView ret(mData + mUsed, mAllocated - mUsed);
        return ret;
    }

    //@return 返回剩余可写空间大小
    u32 getWriteSize()const {
        return mAllocated - mUsed;
    }

    void clearData(u32 used) {
        if (used >= mUsed) {
            mUsed = 0;
        } else if (used > 0) {
            memmove(mData, mData + used, mUsed - used);
            mUsed -= used;
        }
    }

    FunReqTcpCallback mCall;
    s8* mData;
    u32 mAllocated;
    u32 mUsed;          //data size
};

class RequestAccept : public net::RequestTCP {
public:
    net::Socket mSocket;
    net::NetAddress mLocal;
    net::NetAddress mRemote;

    //ipv4 need (2*(sizeof(sockaddr_in) + 16))
    //ipv6 need (2*(sizeof(sockaddr_in6) + 16))
    s8 mCache[88];

    RequestAccept(FunReqTcpCallback func, void* iUser, s32 addrSize) {
        mCache[sizeof(mCache) - 1] = 0;
        u32 ipsz = (addrSize + 16) * 2;
        DASSERT(ipsz <= sizeof(net::RequestAccept::mCache));
        mUser = iUser;
        mCall = func;
        mUsed = 0;
        mData = (s8*)this + DOFFSET(RequestAccept, mCache);
        mAllocated = ipsz;
        DASSERT(mCache == getWriteBuf().mData);
    }

    ~RequestAccept() {
        mSocket.close();
    }

private:
    RequestAccept() { }
};


} //namespace net

} //namespace app

#endif //APP_REQUEST_H