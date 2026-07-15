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


#pragma once
#ifndef APP_HTTPLAYER_H
#define APP_HTTPLAYER_H


#include "EngineConfig.h"
#include "Loop.h"
#include "System.h"
#include "MemoryPool.h"
#include "Net/HandleTLS.h"
#include "Net/TlsContext.h"
#include "Net/HTTP/Http1Parser.h"

namespace app {
namespace net {


class HttpLayer : public RefCount {
public:
    HttpLayer();

    virtual ~HttpLayer();

    bool isHTTPS() const {
        return nullptr != mConnTLS;
    }

    HttpMsg* getReqMsg() const {
        return mMsgReq;
    }

    Website* getWebsite() const {
        return mWebSite;
    }

    s32 launch(HttpMsg* msg);

    const HandleTCP* getHandleTCP() const {
        return mConnTCP;
    }
    const HandleTLS* getHandleTLS() const {
        return mConnTLS;
    }
    const NetAddress* getLocal() const {
        return mConnTLS ? &mConnTLS->getLocal() : (mConnTCP ? &mConnTCP->getLocal() : nullptr);
    }
    const NetAddress* getRemote() const {
        return mConnTLS ? &mConnTLS->getRemote() : (mConnTCP ? &mConnTCP->getRemote() : nullptr);
    }

    bool onLink(RequestFD* it, TlsContext* tlsContext = nullptr);


    bool sendReq(RequestFD* nd);
    s32 sendOut(HttpMsg* msg);

    const s8* getErrStr() const {
        return mParser->getErrStr();
    }

    RequestFD* createMem(usz len);

    void deleteMem(RequestFD* it);

private:
    friend class Http1Parser;

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it, HttpMsg* msg);

    void onRead(RequestFD* it);
    void onReadBeginTLS(RequestFD* it);

    void postClose();

    DFINLINE s32 writeIF(RequestFD* it) {
        return mConnTLS ? mConnTLS->write(it) : (mConnTCP ? mConnTCP->write(it) : EE_INVALID_HANDLE);
    }

    DFINLINE s32 readIF(RequestFD* it) {
        return mConnTLS ? mConnTLS->read(it) : (mConnTCP ? mConnTCP->read(it) : EE_INVALID_HANDLE);
    }

    static s32 funcOnTime(HandleTime* it) {
        HttpLayer& nd = *(HttpLayer*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        HttpMsg* msg = reinterpret_cast<HttpMsg*>(it->mUser);
        HttpLayer* nd = msg->getHttpLayer();
        DASSERT(msg && nd);
        nd->onWrite(it, msg);
    }

    static void funcOnReadBeginTLS(RequestFD* it) {
        reinterpret_cast<HttpLayer*>(it->mUser)->onReadBeginTLS(it);
    }
    static void funcOnRead(RequestFD* it) {
        HttpLayer& nd = *(HttpLayer*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(RequestFD* it) {
        HttpLayer& nd = *(HttpLayer*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        HttpLayer& nd = *(HttpLayer*)it->getUser();
        nd.onClose(it);
    }

    void clear();


    TlsContext* mTlsContext = nullptr;
    Website* mWebSite = nullptr;
    HandleTLS* mConnTLS = nullptr;
    HandleTCP* mConnTCP = nullptr;
    HttpMsg* mMsgReq = nullptr; // request msg
    MemPool* mPool = nullptr;
    Http1Parser* mParser = nullptr;
};

} // namespace net
} // namespace app

#endif // APP_HTTPLAYER_H