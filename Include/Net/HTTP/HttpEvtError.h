#pragma once

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtError : public net::HttpEventer {
public:
    HttpEvtError(s32 err);
    virtual ~HttpEvtError();

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
    s32 mErr;
};

} // namespace app
