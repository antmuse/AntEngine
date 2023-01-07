#ifndef APP_SENDERUDP_H
#define	APP_SENDERUDP_H

#include "Engine.h"
#include "Packet.h"
#include "Net/HandleUDP.h"

namespace app {


class SenderUDP {
public:
    SenderUDP();

    ~SenderUDP();

    //@param cnt ·¢ËÍ´ÎÊý
    void setMaxMsg(s32 cnt);

    s32 open(const String& addr);

private:
    s32 sendMsgs(s32 max);
    s32 sendMsgs(net::RequestTCP* it);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);


    static s32 funcOnTime(HandleTime* it) {
        SenderUDP& nd = *(SenderUDP*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(net::RequestTCP* it) {
        SenderUDP& nd = *(SenderUDP*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(net::RequestTCP* it) {
        SenderUDP& nd = *(SenderUDP*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnClose(Handle* it) {
        SenderUDP& nd = *(SenderUDP*)it->getUser();
        nd.onClose(it);
    }

    static u32 mSN;
    s32 mMaxMsg;
    Loop* mLoop;
    net::HandleUDP mUDP;
    Packet mPack;
};


} //namespace app


#endif //APP_SENDERUDP_H
