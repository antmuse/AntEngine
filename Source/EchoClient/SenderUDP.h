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

    void setMaxMsg(s32 cnt);

    s32 open(const String& addr);

private:
    s32 sendMsgs(s32 max);
    s32 sendMsgs(RequestUDP* it);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(RequestUDP* it);

    void onRead(RequestUDP* it);


    static s32 funcOnTime(HandleTime* it) {
        SenderUDP& nd = *(SenderUDP*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        SenderUDP& nd = *(SenderUDP*)it->mUser;
        nd.onWrite(reinterpret_cast<RequestUDP*>(it));
    }

    static void funcOnRead(RequestFD* it) {
        SenderUDP& nd = *(SenderUDP*)it->mUser;
        nd.onRead(reinterpret_cast<RequestUDP*>(it));
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
