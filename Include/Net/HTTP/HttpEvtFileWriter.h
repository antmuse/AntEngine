#pragma once

#include "TList.h"
#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtFileWriter : public net::HttpEventer {
public:
    HttpEvtFileWriter();
    virtual ~HttpEvtFileWriter();

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
    TList<Packet> mCache;
    HandleFile* mFile = nullptr; // backend
    net::HttpMsg* mMsg = nullptr;
    net::HttpMsg* mMsgResp = nullptr; // back msg
    usz mOffset = 0;
    bool mReqBodyFinish = true;

    s32 sendRespHead(net::HttpMsg* req, s32 err, const s8* body, bool chunk, bool send);
    void onFileWrite(RequestFD* it);
    void onFileClose(Handle* it);

    s32 launchWrite();
    s32 postResp();

    static void funcOnWrite(RequestFD* it) {
        HttpEvtFileWriter& nd = *(HttpEvtFileWriter*)it->mUser;
        nd.onFileWrite(it);
    }
    static void funcOnClose(Handle* it) {
        HttpEvtFileWriter& nd = *(HttpEvtFileWriter*)it->getUser();
        nd.onFileClose(it);
    }
};

} // namespace app
