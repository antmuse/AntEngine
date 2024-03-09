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

    //@param cnt 发送次数
    void setMaxMsg(s32 cnt);

    s32 open(const String& addr);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it);

    void onRead(RequestFD* it);

    net::HandleTLS& getHandleTCP() {
        return mTCP;
    }

private:

    static s32 funcOnTime(HandleTime* it) {
        TlsConnector& nd = *(TlsConnector*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        TlsConnector& nd = *(TlsConnector*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(RequestFD* it) {
        TlsConnector& nd = *(TlsConnector*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(RequestFD* it) {
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
