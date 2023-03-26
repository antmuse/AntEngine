#ifndef APP_ASYNCFILE_H
#define	APP_ASYNCFILE_H

#include "Engine.h"
#include "Packet.h"
#include "HandleFile.h"

namespace app {


class AsyncFile {
public:
    AsyncFile();

    ~AsyncFile();

    s32 open(const String& addr, s32 flag);

private:
    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(RequestFD* it);

    void onRead(RequestFD* it);

    static s32 funcOnTime(HandleTime* it) {
        AsyncFile& nd = *(AsyncFile*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        AsyncFile& nd = *(AsyncFile*)it->mUser;
        nd.onWrite((RequestFD*)it);
    }

    static void funcOnRead(RequestFD* it) {
        AsyncFile& nd = *(AsyncFile*)it->mUser;
        nd.onRead((RequestFD*)it);
    }

    static void funcOnClose(Handle* it) {
        AsyncFile& nd = *(AsyncFile*)it->getUser();
        nd.onClose(it);
    }

    Loop* mLoop;
    HandleFile mHandleFile;
    usz mAsyncSize;
    Packet mPack;
};


} //namespace app


#endif //APP_ASYNCFILE_H
