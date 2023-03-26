#ifndef APP_LINKERUDP_H
#define	APP_LINKERUDP_H

#include "Engine.h"
#include "RefCount.h"
#include "Packet.h"
#include "Net/HandleUDP.h"

namespace app {
namespace net {

class NetServerUDP;

class LinkerUDP :public RefCount {
public:
    LinkerUDP();

    ~LinkerUDP();

    s32 onLink(const NetServerUDP& sev, RequestUDP* sit);

private:
    s32 sendMsgs(s32 max);
    s32 sendMsgs(RequestUDP* it);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(RequestUDP* it);

    void onRead(RequestUDP* it);


    static s32 funcOnTime(HandleTime* it) {
        LinkerUDP& nd = *(LinkerUDP*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        LinkerUDP& nd = *(LinkerUDP*)it->mUser;
        nd.onWrite(reinterpret_cast<RequestUDP*>(it));
    }

    static void funcOnRead(RequestFD* it) {
        LinkerUDP& nd = *(LinkerUDP*)it->mUser;
        nd.onRead(reinterpret_cast<RequestUDP*>(it));
    }

    static void funcOnClose(Handle* it) {
        LinkerUDP& nd = *(LinkerUDP*)it->getUser();
        nd.onClose(it);
    }

    static u32 mSN;
    Loop* mLoop;
    net::HandleUDP mUDP;
    Packet mPack;
};


class NetServerUDP :public RefCount {
public:
    NetServerUDP(bool tls) :mTLS(tls) { }
    virtual ~NetServerUDP() { }

    s32 open(const String& addr);

    bool isTLS()const {
        return mTLS;
    }

    void bind(net::LinkerUDP* it) const {
        grab();
        it->grab();
    }

    void unbind(net::LinkerUDP* it) const {
        drop();
        it->drop();
    }

    const net::HandleUDP& getHandleUDP() const {
        return mUDP;
    }

private:
    s32 onTimeout(HandleTime& it);
    void onClose(Handle* it);
    void onRead(RequestUDP* it);
    
    static void funcOnRead(RequestFD* it) {
        DASSERT(it && it->mUser);
        NetServerUDP& nd = *reinterpret_cast<NetServerUDP*>(it->getUser());
        nd.onRead(reinterpret_cast<RequestUDP*>(it));
    }
    static void funcOnClose(Handle* it) {
        NetServerUDP& nd = *reinterpret_cast<NetServerUDP*>(it->getUser());
        nd.onClose(it);
    }
    static s32 funcOnTime(HandleTime* it) {
        NetServerUDP& nd = *reinterpret_cast<NetServerUDP*>(it->getUser());
        return nd.onTimeout(*it);
    }
    bool mTLS;
    net::HandleUDP mUDP;
};


} //namespace net
} //namespace app


#endif //APP_LINKERUDP_H
