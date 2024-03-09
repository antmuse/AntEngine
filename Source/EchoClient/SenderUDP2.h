#ifndef APP_SENDERUDP2_H
#define	APP_SENDERUDP2_H

#include "Engine.h"
#include "Packet.h"
#include "Net/HandleUDP.h"
#include "Net/KCProtocal.h"

namespace app {


class SenderUDP2 {
public:
    SenderUDP2();

    ~SenderUDP2();

    //@param cnt 发送次数
    void setMaxMsg(s32 cnt);

    s32 open(const String& addr, bool tls = false);

private:
    s32 sendMsgs(s32 max);
    s32 sendRawMsg(const void* buf, s32 len);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(RequestUDP* it);

    void onRead(RequestUDP* it);

    //@return 0 if none error;
    s32 onReadKCP();

    static s32 sendKcpRaw(const void* buf, s32 len, void* user);

    static s32 funcOnTime(HandleTime* it) {
        SenderUDP2& nd = *(SenderUDP2*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        SenderUDP2& nd = *(SenderUDP2*)it->mUser;
        nd.onWrite(reinterpret_cast<RequestUDP*>(it));
    }

    static void funcOnRead(RequestFD* it) {
        SenderUDP2& nd = *(SenderUDP2*)it->mUser;
        nd.onRead(reinterpret_cast<RequestUDP*>(it));
    }

    static void funcOnClose(Handle* it) {
        SenderUDP2& nd = *(SenderUDP2*)it->getUser();
        nd.onClose(it);
    }

    DFINLINE s32 writeIF(RequestUDP* it) {
        return mUDP.write(it);
    }

    DFINLINE s32 readIF(RequestUDP* it) {
        return mUDP.read(it);
    }

    static u32 mSN;
    static u32 mID;
    bool mTLS;
    u32 mTimeExt;
    usz mMaxMsg;
    Loop* mLoop;
    MemPool* mMemPool;
    net::HandleUDP mUDP;
    net::KCProtocal mProto;
    Packet mPack;
};


} //namespace app


#endif //APP_SENDERUDP2_H
