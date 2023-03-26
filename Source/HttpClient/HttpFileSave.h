#ifndef APP_HTTPFILESAVE_H
#define	APP_HTTPFILESAVE_H

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpFileSave :public net::HttpEventer {
public:
    HttpFileSave();
    virtual ~HttpFileSave();

    virtual s32 onSent(net::HttpMsg& req)override;
    virtual s32 onFinish(net::HttpMsg& resp)override;
    virtual s32 onBodyPart(net::HttpMsg& resp)override;
    virtual s32 onOpen(net::HttpMsg& msg)override;
    virtual s32 onClose()override;

private:
    RingBuffer mBuf;
    RingBuffer* mBody;
    RequestFD mReqs;
    HandleFile mFile;
    usz mWrited;
    bool mDone;
    void onFileWrite(RequestFD* it);
    void onFileClose(Handle* it);

    s32 launchWrite();

    static void funcOnWrite(RequestFD* it) {
        HttpFileSave& nd = *(HttpFileSave*)it->mUser;
        nd.onFileWrite(it);
    }
    static void funcOnClose(Handle* it) {
        HttpFileSave& nd = *(HttpFileSave*)it->getUser();
        nd.onFileClose(it);
    }
};

}//namespace app
#endif //APP_HTTPFILESAVE_H