#ifndef APP_HTTPFILEREAD_H
#define	APP_HTTPFILEREAD_H

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpFileRead :public net::HttpEventer {
public:
    HttpFileRead();
    virtual ~HttpFileRead();

    virtual s32 onSent(net::HttpMsg& req)override;
    virtual s32 onFinish(net::HttpMsg& resp)override;
    virtual s32 onBodyPart(net::HttpMsg& resp)override;
    virtual s32 onOpen(net::HttpMsg& msg)override;
    virtual s32 onClose()override;

private:
    //RingBuffer mBuf;
    SRingBufPos mChunkPos;
    RingBuffer* mBody;
    RequestFD mReqs;
    HandleFile mFile;
    net::HttpMsg* mMsg;
    usz mReaded;
    bool mDone;

    void onFileRead(RequestFD* it);
    void onFileClose(Handle* it);

    s32 launchRead();

    static void funcOnRead(RequestFD* it) {
        HttpFileRead& nd = *(HttpFileRead*)it->mUser;
        nd.onFileRead(it);
    }
    static void funcOnClose(Handle* it) {
        HttpFileRead& nd = *(HttpFileRead*)it->getUser();
        nd.onFileClose(it);
    }
};

}//namespace app
#endif //APP_HTTPFILEREAD_H