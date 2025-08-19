#pragma once

#include "MsgNAT.h"
#include "Loop.h"
#include "Net/NetAddress.h"
#include "Net/HandleUDP.h"
#include "TMap.h"


namespace app {

class ServerNAT {
public:
    ServerNAT();

    ~ServerNAT();

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
        ServerNAT& nd = *(ServerNAT*)it->getUser();
        nd.onClose(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        ServerNAT& nd = *(ServerNAT*)it->getUser();
        return nd.onTimeout(it);
    }

    static void funcOnWrite(RequestFD* it) {
        ServerNAT& nd = *(ServerNAT*)it->mUser;
        nd.onWrite((RequestUDP*)it);
    }

    static void funcOnRead(RequestFD* it) {
        ServerNAT& nd = *(ServerNAT*)it->mUser;
        nd.onRead((RequestUDP*)it);
    }

    HandleTime mTime;
    net::HandleUDP mUDP;
    Loop& mLoop;
    u32 mSN;
    u64 mUserSN;
    TMap<u64, String> mBinds;
};

}//namespace app
