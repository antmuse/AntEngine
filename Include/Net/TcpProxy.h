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


#ifndef APP_CONNECTOR_H
#define	APP_CONNECTOR_H

#include "RefCount.h"
#include "Loop.h"
#include "EngineConfig.h"
#include "Net/HandleTLS.h"

namespace app {
namespace net {


class TcpProxyHub;

class TcpProxy :public RefCount {
public:
    TcpProxy(Loop& loop);

    virtual ~TcpProxy();

    void onLink(RequestFD* it);

private:

    s32 onTimeout(HandleTime& it);
    s32 onTimeout2(HandleTime& it);

    void onClose(Handle* it);
    void onClose2(Handle* it);

    void onWrite(RequestFD* it);
    void onWrite2(RequestFD* it);

    void onRead(RequestFD* it);
    void onRead2(RequestFD* it);

    void onConnect(RequestFD* it);

    static s32 funcOnTime(HandleTime* it) {
        TcpProxy& nd = *(TcpProxy*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        TcpProxy& nd = *(TcpProxy*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(RequestFD* it) {
        TcpProxy& nd = *(TcpProxy*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnClose(Handle* it) {
        TcpProxy& nd = *(TcpProxy*)it->getUser();
        nd.onClose(it);
    }


    static s32 funcOnTime2(HandleTime* it) {
        TcpProxy& nd = *(TcpProxy*)it->getUser();
        return nd.onTimeout2(*it);
    }

    static void funcOnWrite2(RequestFD* it) {
        TcpProxy& nd = *(TcpProxy*)it->mUser;
        nd.onWrite2(it);
    }

    static void funcOnRead2(RequestFD* it) {
        TcpProxy& nd = *(TcpProxy*)it->mUser;
        nd.onRead2(it);
    }

    static void funcOnClose2(Handle* it) {
        TcpProxy& nd = *(TcpProxy*)it->getUser();
        nd.onClose2(it);
    }

    static void funcOnConnect(RequestFD* it) {
        TcpProxy& nd = *(TcpProxy*)it->mUser;
        nd.onConnect(it);
    }

    void unbind();

    //0=[tcp-tcp], 1=[tls-tcp], 2=[tcp-tls], 3=[tls-tls]
    u8 mType;
    Loop& mLoop;
    TcpProxyHub* mHub;
    net::HandleTLS mTLS;
    net::HandleTLS mTLS2; //backend
};



class TcpProxyHub :public RefCount {
public:
    TcpProxyHub(EngineConfig::ProxyCfg& cfg) :mConfig(cfg) { }
    virtual ~TcpProxyHub() { }

    static void funcOnLink(RequestFD* it) {
        TcpProxy* con = new TcpProxy(*it->mHandle->getLoop());
        con->onLink(it);
        con->drop();
    }

    const EngineConfig::ProxyCfg& getConfig()const {
        return mConfig;
    }

private:
    EngineConfig::ProxyCfg mConfig;
};

} //namespace net
} //namespace app


#endif //APP_CONNECTOR_H
