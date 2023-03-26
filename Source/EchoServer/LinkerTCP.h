#ifndef APP_LINKERTCP_H
#define	APP_LINKERTCP_H

#include "Loop.h"
#include "Packet.h"
#include "Net/HandleTLS.h"
#include "Net/Acceptor.h"

namespace app {
namespace net {

class NetServerTCP;
class LinkerTCP :public RefCount {
public:
    LinkerTCP();

    virtual ~LinkerTCP();

    bool onLink(NetServerTCP* sev, RequestFD* it);

private:

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(RequestFD* it);

    void onRead(RequestFD* it);


    DFINLINE s32 writeIF(RequestFD* it) {
        return mTLS ? mTCP.write(it) : mTCP.getHandleTCP().write(it);
    }

    DFINLINE s32 readIF(RequestFD* it) {
        return mTLS ? mTCP.read(it) : mTCP.getHandleTCP().read(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        LinkerTCP& nd = *(LinkerTCP*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        LinkerTCP& nd = *(LinkerTCP*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(RequestFD* it) {
        LinkerTCP& nd = *(LinkerTCP*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnClose(Handle* it) {
        LinkerTCP& nd = *(LinkerTCP*)it->getUser();
        nd.onClose(it);
    }

    bool mTLS;
    NetServerTCP* mServer;
    net::HandleTLS mTCP;
    Packet mPack;
};

class NetServerTCP :public RefCount {
public:
    NetServerTCP(bool tls) :mTLS(tls) { }
    virtual ~NetServerTCP() { }

    static void funcOnLink(RequestFD* it) {
        DASSERT(it && it->mUser);
        net::Acceptor* accp = reinterpret_cast<net::Acceptor*>(it->mUser);
        NetServerTCP* web = reinterpret_cast<NetServerTCP*>(accp->getUser());
        web->onLink(it);
    }

    bool isTLS()const {
        return mTLS;
    }

    void bind(net::LinkerTCP* it) {
        grab();
        it->grab();
    }

    void unbind(net::LinkerTCP* it) {
        drop();
        it->drop();
    }

private:
    void onLink(RequestFD* it) {
        net::LinkerTCP* con = new net::LinkerTCP();
        con->onLink(this, it);
        con->drop();
    }

    bool mTLS;
};


} //namespace net
} //namespace app


#endif //APP_LINKERTCP_H
