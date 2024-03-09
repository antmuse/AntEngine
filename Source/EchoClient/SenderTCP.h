#ifndef APP_SENDERTCP_H
#define	APP_SENDERTCP_H

#include "Engine.h"
#include "Packet.h"
#include "Net/HandleTLS.h"

namespace app {


class SenderTCP {
public:
    SenderTCP();

    ~SenderTCP();

    //@param cnt 发送次数
    void setMaxMsg(s32 cnt);

    s32 open(const String& addr, bool tls);

private:

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it);

    void onRead(RequestFD* it);

    //net::HandleTCP& getHandleTCP() {
    //    return mTCP;
    //}

    static s32 funcOnTime(HandleTime* it) {
        SenderTCP& nd = *(SenderTCP*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        SenderTCP& nd = *(SenderTCP*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(RequestFD* it) {
        SenderTCP& nd = *(SenderTCP*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(RequestFD* it) {
        SenderTCP& nd = *(SenderTCP*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        SenderTCP& nd = *(SenderTCP*)it->getUser();
        nd.onClose(it);
    }

    DFINLINE s32 writeIF(RequestFD* it) {
        return mTLS ? mTCP.write(it) : mTCP.getHandleTCP().write(it);
    }

    DFINLINE s32 readIF(RequestFD* it) {
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


#endif //APP_SENDERTCP_H
