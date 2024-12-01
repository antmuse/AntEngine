#pragma once

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtFileSave : public net::HttpEventer {
public:
    HttpEvtFileSave();
    virtual ~HttpEvtFileSave();

    virtual s32 onSent(net::HttpMsg* msg) override;
    virtual s32 onFinish(net::HttpMsg* msg) override;
    virtual s32 onBodyPart(net::HttpMsg* msg) override;
    virtual s32 onOpen(net::HttpMsg* msg) override;
    virtual s32 onClose() override;

private:
    RingBuffer* mBody;
    RequestFD mReqs;
    HandleFile mFile;
    usz mWrited;
    bool mFinish;
    net::HttpMsg* mMsg;

    void onFileWrite(RequestFD* it);
    void onFileClose(Handle* it);

    s32 launchWrite();

    static void funcOnWrite(RequestFD* it) {
        HttpEvtFileSave& nd = *(HttpEvtFileSave*)it->mUser;
        nd.onFileWrite(it);
    }
    static void funcOnClose(Handle* it) {
        HttpEvtFileSave& nd = *(HttpEvtFileSave*)it->getUser();
        nd.onFileClose(it);
    }
};

} // namespace app
