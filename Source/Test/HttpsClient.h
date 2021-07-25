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

    void onConnect(net::RequestTCP* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);

    static s32 funcOnTime(HandleTime* it) {
        HttpsClient& nd = *(HttpsClient*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(net::RequestTCP* it) {
        HttpsClient& nd = *(HttpsClient*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(net::RequestTCP* it) {
        HttpsClient& nd = *(HttpsClient*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(net::RequestTCP* it) {
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
