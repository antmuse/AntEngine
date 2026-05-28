#pragma once

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtFileReader : public net::HttpEventer {
public:
    HttpEvtFileReader();
    virtual ~HttpEvtFileReader();

    virtual s32 onLayerClose(net::HttpMsg* msg) override;

    // req parse err
    virtual s32 onReadError(net::HttpMsg* msg) override;

    virtual s32 onRespWrite(net::HttpMsg* msg) override;
    virtual s32 onRespWriteError(net::HttpMsg* msg) override;

    virtual s32 onReqHeadDone(net::HttpMsg* msg) override;
    virtual s32 onReqChunkHeadDone(net::HttpMsg* msg) override;
    virtual s32 onReqBody(net::HttpMsg* msg) override;
    virtual s32 onReqChunkBodyDone(net::HttpMsg* msg) override;
    virtual s32 onReqBodyDone(net::HttpMsg* msg) override;


private:
    RequestFD mReqs;
    HandleFile* mFile = nullptr; // backend
    net::HttpMsg* mMsg = nullptr;
    net::HttpMsg* mMsgResp = nullptr; // back msg
    usz mOffset = 0;
    bool mReqBodyFinish = true;

    s32 sendRespHead(net::HttpMsg* req, s32 err, const s8* body, bool chunk, bool send);
    void onFileRead(RequestFD* it);
    void onFileClose(Handle* it);

    s32 launchRead();
    s32 postResp();

    static void funcOnRead(RequestFD* it) {
        HttpEvtFileReader& nd = *(HttpEvtFileReader*)it->mUser;
        nd.onFileRead(it);
    }

    static void funcOnClose(Handle* it) {
        HttpEvtFileReader& nd = *(HttpEvtFileReader*)it->getUser();
        nd.onFileClose(it);
    }
};

} // namespace app
