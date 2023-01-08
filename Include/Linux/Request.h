#ifndef APP_REQUEST_H
#define	APP_REQUEST_H

#include "Config.h"
#include "Nocopy.h"
#include "Strings.h"
#include "Net/NetAddress.h"
#include "Net/Socket.h"

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

    void* getUser() const {
        return mUser;
    }

    u32 getStepSize() const {
        return mStepSize;
    }

    void setStepSize(u32 it) {
        mStepSize = it;
    }

    Request* mNext;
    Handle* mHandle;
    void* mUser;
    s32 mType;      //ERequestType
    s32 mError;     //0=success,else failed code
    u32 mStepSize;
};



namespace net {

class RequestTCP : public Request {
public:
    RequestTCP() {
        clear();
    }

    ~RequestTCP() { }

    static RequestTCP* newRequest(u32 cache_size) {
        RequestTCP* it = reinterpret_cast<RequestTCP*>(new s8[sizeof(RequestTCP) + cache_size]);
        new ((void*)it) RequestTCP();
        it->mAllocated = cache_size;
        it->mData = reinterpret_cast<s8*>(it + 1);
        return it;
    }

    static void delRequest(net::RequestTCP* it) {
        delete[] reinterpret_cast<s8*>(it);
    }

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


class RequestUDP : public net::RequestTCP {
public:
    u32 mFlags;   //bits: [1=had connected, ...]
    net::NetAddress mRemote;

    static RequestUDP* newRequest(u32 cache_size) {
        RequestUDP* it = reinterpret_cast<RequestUDP*>(new s8[sizeof(RequestUDP) + cache_size]);
        new ((void*)it) RequestUDP();
        it->mAllocated = cache_size;
        it->mData = reinterpret_cast<s8*>(it + 1);
        return it;
    }

    static void delRequest(net::RequestUDP* it) {
        delete[] reinterpret_cast<s8*>(it);
    }

    RequestUDP(){
        memset(this, 0, sizeof(*this));
    }

    ~RequestUDP() {
    }
};


class RequestAccept : public net::RequestTCP {
public:
    net::Socket mSocket;
    net::NetAddress mLocal;
    net::NetAddress mRemote;

    RequestAccept(FunReqTcpCallback func, void* iUser, s32 addrSize) {
        mLocal.setAddrSize(addrSize);
        mRemote.setAddrSize(addrSize);
        mUser = iUser;
        mCall = func;
        mData = (s8*)(this + 1);
        mUsed = 0;
        mAllocated = 0;
    }

    ~RequestAccept() {
        mSocket.close();
    }
};

} //namespace net

} //namespace app

#endif //APP_REQUEST_H
