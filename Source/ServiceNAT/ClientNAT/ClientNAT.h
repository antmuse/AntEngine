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

    s32 start(const s8* ipt, const s8* userid, const s8* password, const s8* peerid);

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

    void processMsg(s32 status, void* msg);

    s32 mStatus;
    u32 mSN;
    u64 mUserID;
    u64 mPeerID;
    String mPassword;
    s8 mFindCount;
    s8 mMaxFind;
    net::NetAddress mServerAddr;
    net::NetAddress mPeerAddr;
    HandleTime mTime;
    net::HandleUDP mUDP;
    Loop& mLoop;
};

} // namespace app
