#pragma once

#include "System.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtPath : public net::HttpEventer {
public:
    HttpEvtPath();
    virtual ~HttpEvtPath();

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
    s32 writeChunk(net::HttpMsg* msg);
    void writeStr(net::HttpMsg* msg, const s8* it);
    s32 postResp(net::HttpMsg* msg);

    net::HttpMsg* mMsg;
    usz mOffset;
    TVector<FileInfo> mList;
};

} // namespace app
