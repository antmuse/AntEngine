#pragma once

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtFileRead : public net::HttpEventer {
public:
    HttpEvtFileRead();
    virtual ~HttpEvtFileRead();

    virtual s32 onSent(net::HttpMsg* req) override;
    virtual s32 onFinish(net::HttpMsg* resp) override;
    virtual s32 onBodyPart(net::HttpMsg* resp) override;
    virtual s32 onOpen(net::HttpMsg* msg) override;
    virtual s32 onClose() override;

private:
    // RingBuffer mBuf;
    SRingBufPos mChunkPos;
    RingBuffer* mBody;
    RequestFD mReqs;
    HandleFile mFile;
    net::HttpMsg* mMsg;
    usz mReaded;

    void onFileRead(RequestFD* it);
    void onFileClose(Handle* it);

    s32 launchRead();

    static void funcOnRead(RequestFD* it) {
        HttpEvtFileRead& nd = *(HttpEvtFileRead*)it->mUser;
        nd.onFileRead(it);
    }
    static void funcOnClose(Handle* it) {
        HttpEvtFileRead& nd = *(HttpEvtFileRead*)it->getUser();
        nd.onFileClose(it);
    }
};

} // namespace app
