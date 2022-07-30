#ifndef APP_LINKER_H
#define	APP_LINKER_H

#include "Loop.h"
#include "Packet.h"
#include "Net/HandleTLS.h"
#include "Net/Acceptor.h"

namespace app {
namespace net {

class NetServer;
class Linker :public RefCount {
public:
    Linker();

    virtual ~Linker();

    bool onLink(NetServer* sev, net::RequestTCP* it);

private:

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);


    DFINLINE s32 writeIF(net::RequestTCP* it) {
        return mTLS ? mTCP.write(it) : mTCP.getHandleTCP().write(it);
    }

    DFINLINE s32 readIF(net::RequestTCP* it) {
        return mTLS ? mTCP.read(it) : mTCP.getHandleTCP().read(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        Linker& nd = *(Linker*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(net::RequestTCP* it) {
        Linker& nd = *(Linker*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(net::RequestTCP* it) {
        Linker& nd = *(Linker*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnClose(Handle* it) {
        Linker& nd = *(Linker*)it->getUser();
        nd.onClose(it);
    }

    bool mTLS;
    NetServer* mServer;
    net::HandleTLS mTCP;
    Packet mPack;
};

class NetServer :public RefCount {
public:
    NetServer(bool tls) :mTLS(tls) { }
    virtual ~NetServer() { }

    static void funcOnLink(net::RequestTCP* it) {
        DASSERT(it && it->mUser);
        net::Acceptor* accp = reinterpret_cast<net::Acceptor*>(it->mUser);
        NetServer* web = reinterpret_cast<NetServer*>(accp->getUser());
        web->onLink(it);
    }

    bool isTLS()const {
        return mTLS;
    }

    void bind(net::Linker* it) {
        grab();
        it->grab();
    }

    void unbind(net::Linker* it) {
        drop();
        it->drop();
    }

private:
    //friend class HttpLayer;

    void onLink(net::RequestTCP* it) {
        net::Linker* con = new net::Linker();
        con->onLink(this, it);
        con->drop();
    }

    bool mTLS;
};


} //namespace net
} //namespace app


#endif //APP_LINKER_H
