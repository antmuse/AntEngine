#ifndef APP_TLSCONNECTOR_H
#define	APP_TLSCONNECTOR_H

#include "Engine.h"
#include "Packet.h"
#include "Net/HandleTLS.h"

namespace app {


class TlsConnector {
public:
    TlsConnector();

    ~TlsConnector();

    //@param cnt ·¢ËÍ´ÎÊý
    void setMaxMsg(s32 cnt);

    s32 open(const String& addr);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(net::RequestTCP* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);

    net::HandleTLS& getHandleTCP() {
        return mTCP;
    }

private:

    static s32 funcOnTime(HandleTime* it) {
        TlsConnector& nd = *(TlsConnector*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(net::RequestTCP* it) {
        TlsConnector& nd = *(TlsConnector*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(net::RequestTCP* it) {
        TlsConnector& nd = *(TlsConnector*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(net::RequestTCP* it) {
        TlsConnector& nd = *(TlsConnector*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        TlsConnector& nd = *(TlsConnector*)it->getUser();
        nd.onClose(it);
    }

    static u32 mSN;
    usz mMaxMsg;
    Loop* mLoop;
    net::HandleTLS mTCP;
    Packet mPack;
};


} //namespace app


#endif //APP_TLSCONNECTOR_H
