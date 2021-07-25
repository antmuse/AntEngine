#ifndef APP_CONNECTOR_H
#define	APP_CONNECTOR_H

#include "Engine.h"
#include "Packet.h"
#include "Net/HandleTLS.h"

namespace app {


class Connector {
public:
    Connector();

    ~Connector();

    //@param cnt ·¢ËÍ´ÎÊý
    void setMaxMsg(s32 cnt);

    s32 open(const String& addr, bool tls);

private:

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(net::RequestTCP* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);

    //net::HandleTCP& getHandleTCP() {
    //    return mTCP;
    //}

    static s32 funcOnTime(HandleTime* it) {
        Connector& nd = *(Connector*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(net::RequestTCP* it) {
        Connector& nd = *(Connector*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(net::RequestTCP* it) {
        Connector& nd = *(Connector*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(net::RequestTCP* it) {
        Connector& nd = *(Connector*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        Connector& nd = *(Connector*)it->getUser();
        nd.onClose(it);
    }

    DFINLINE s32 writeIF(net::RequestTCP* it) {
        return mTLS ? mTCP.write(it) : mTCP.getHandleTCP().write(it);
    }

    DFINLINE s32 readIF(net::RequestTCP* it) {
        return mTLS ? mTCP.read(it) : mTCP.getHandleTCP().read(it);
    }

    static u32 mSN;
    bool mTLS;
    usz mMaxMsg;
    Loop* mLoop;
    net::HandleTLS mTCP;
    Packet mPack;
};


} //namespace app


#endif //APP_CONNECTOR_H
