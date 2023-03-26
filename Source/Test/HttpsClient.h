#ifndef APP_HTTPSCLIENT_H
#define	APP_HTTPSCLIENT_H

#include "FileWriter.h"
#include "Net/HandleTLS.h"

namespace app {


class HttpsClient {
public:
    HttpsClient();

    ~HttpsClient();

    s32 open(const String& addr);

private:
    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it);

    void onRead(RequestFD* it);

    static s32 funcOnTime(HandleTime* it) {
        HttpsClient& nd = *(HttpsClient*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        HttpsClient& nd = *(HttpsClient*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(RequestFD* it) {
        HttpsClient& nd = *(HttpsClient*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(RequestFD* it) {
        HttpsClient& nd = *(HttpsClient*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        HttpsClient& nd = *(HttpsClient*)it->getUser();
        nd.onClose(it);
    }

    net::HandleTLS mTCP;
    FileWriter mFile;
};


} //namespace app


#endif //APP_HTTPSCLIENT_H
