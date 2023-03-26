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


#ifndef APP_HANDLETLS_H
#define APP_HANDLETLS_H


#include "RingBuffer.h"
#include "Loop.h"
#include "Net/TlsContext.h"
#include "Net/HandleTCP.h"

namespace app {
namespace net {

class HandleTLS : public HandleTime {
public:
    HandleTLS();

    ~HandleTLS();

    //connect
    s32 open(const String& addr, RequestFD* it, const net::TlsContext* tlsctx = nullptr);

    //connect
    s32 open(const NetAddress& addr, RequestFD* it, const net::TlsContext* tlsctx = nullptr);

    //link
    s32 open(const RequestAccept& accp, RequestFD* it, const net::TlsContext* tlsctx = nullptr);

    s32 write(RequestFD* it);

    s32 read(RequestFD* it);

    void setClose(EHandleType tp, FunCloseCallback fc, void* user) {
        HandleTime::setClose(tp, fc, user);
        mTCP.setClose(tp, HandleTLS::funcOnClose, this);
    }

    void setTime(FunTimeCallback fn, s64 firstTimeGap, s64 gap, s32 repeat) {
        HandleTime::setTime(fn, firstTimeGap, gap, repeat);
        mTCP.setTime(HandleTLS::funcOnTime, firstTimeGap, gap, repeat);
    }

    s32 close() {
        return mTCP.close();
    }

    s32 setHost(const s8* hostName, usz length);

    s32 verify(s32 vfg);

    const NetAddress& getLocal()const {
        return mTCP.getLocal();
    }

    const NetAddress& getRemote()const {
        return mTCP.getRemote();
    }

    HandleTCP& getHandleTCP() {
        return mTCP;
    }


protected:
    s32 handshake();

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it);
    void onWriteHello(RequestFD* it);

    void onRead(RequestFD* it);
    void onReadHello(RequestFD* it);

    static s32 funcOnTime(HandleTime* it) {
        HandleTLS& nd = *(HandleTLS*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        HandleTLS& nd = *(HandleTLS*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnWriteHello(RequestFD* it) {
        HandleTLS& nd = *(HandleTLS*)it->mUser;
        nd.onWriteHello(it);
    }

    static void funcOnRead(RequestFD* it) {
        HandleTLS& nd = *(HandleTLS*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnReadHello(RequestFD* it) {
        HandleTLS& nd = *(HandleTLS*)it->mUser;
        nd.onReadHello(it);
    }

    static void funcOnConnect(RequestFD* it) {
        HandleTLS& nd = *(HandleTLS*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        HandleTLS& nd = *(HandleTLS*)it->getUser();
        nd.onClose(it);
    }

    void init(const TlsContext& tlsCTX);

    void uninit();

    //解密文
    void doRead();

    //写密文
    s32 postWrite();

    //读密文
    s32 postRead();

    //回调读到的明文
    void landReads() {
        landQueue(mLandReads);
    }

    //回调写完的明文
    void landWrites() {
        landQueue(mLandWrites);
    }

    static void landQueue(RequestFD*& que);

    HandleTCP mTCP;
    RequestFD mRead;
    RequestFD mWrite;
    RequestFD* mFlyWrites;
    RequestFD* mFlyReads;
    RequestFD* mLandWrites;
    RequestFD* mLandReads;
    void* mTlsSession;
    s8 mHostName[256];
    RingBuffer mInBuffers;
    RingBuffer mOutBuffers;
    SRingBufPos mCommitPos;
};


}//namespace net
}//namespace app


#endif //APP_HANDLETLS_H