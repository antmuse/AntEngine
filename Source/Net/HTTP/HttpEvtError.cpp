#include "Net/HTTP/HttpEvtError.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"


namespace app {

HttpEvtError::HttpEvtError(s32 err) : mErr(err) {
}

HttpEvtError::~HttpEvtError() {
}


s32 HttpEvtError::onLayerClose(net::HttpMsg* msg) {
    return EE_OK;
}

// req parse err
s32 HttpEvtError::onReadError(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtError::onRespWrite(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtError::onRespWriteError(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtError::onReqHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtError::onReqBody(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtError::onReqBodyDone(net::HttpMsg* msg) {
    String ebody = R"(<html>
<head><title>ERROR</title>
<style>
h1 {
font-size: 32px;
text-align: center;
}
p {
font-size: 24px;
text-align: center;
}
</style>
</head><body>
<h1>ERROR )";
    ebody += mErr;
    ebody += R"(</h1><hr><br><p>file or not supported, pls wait for more.</p><br><hr></body></html>)";
    net::HttpMsg* resp = new net::HttpMsg(msg->getHttpLayer());
    resp->setStatus(mErr, "ERR");
    resp->getHead().setLength(ebody.size());
    resp->getHead().setDefaultContentType();
    resp->getBody().write(ebody.data(), ebody.size());
    s32 ret = msg->getHttpLayer()->sendOut(resp);
    resp->drop();
    return ret;
}


s32 HttpEvtError::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtError::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}

} // namespace app
