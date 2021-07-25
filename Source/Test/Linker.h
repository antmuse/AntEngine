#ifndef APP_LINKER_H
#define	APP_LINKER_H

#include "Loop.h"
#include "Packet.h"
#include "Net/HandleTLS.h"

namespace app {
namespace net {


class Linker {
public:
    Linker();

    ~Linker();

    static void funcOnLink(net::RequestTCP* it) {
        //it->mUser;
        Linker* con = new Linker();
        con->onLink(it); //Linker×Ô»Ù¶ÔÏó
    }

private:

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);

    void onLink(net::RequestTCP* it);

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
    net::HandleTLS mTCP;
    Packet mPack;
};


} //namespace net
} //namespace app


#endif //APP_LINKER_H
