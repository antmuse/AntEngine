#pragma once

#include "MsgNAT.h"
#include "Loop.h"
#include "Net/NetAddress.h"
#include "Net/HandleUDP.h"


namespace app {

class ClientNAT {
public:
    ClientNAT();

    ~ClientNAT();

    s32 start(const s8* ipt);

    bool isActive() {
        return mTime.getGrabCount() > 0;
    }

    void onClose(Handle* it);

    s32 onTimeout(HandleTime* it);
    void onRead(RequestUDP* it);
    void onWrite(RequestUDP* it);

private:
    static void funcOnClose(Handle* it) {
        ClientNAT& nd = *(ClientNAT*)it->getUser();
        nd.onClose(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        ClientNAT& nd = *(ClientNAT*)it->getUser();
        return nd.onTimeout(it);
    }

    static void funcOnWrite(RequestFD* it) {
        ClientNAT& nd = *(ClientNAT*)it->mUser;
        nd.onWrite((RequestUDP*)it);
    }

    static void funcOnRead(RequestFD* it) {
        ClientNAT& nd = *(ClientNAT*)it->mUser;
        nd.onRead((RequestUDP*)it);
    }

    s32 mStatus;
    HandleTime mTime;
    net::HandleUDP mUDP;
    Loop& mLoop;
};

} // namespace app
