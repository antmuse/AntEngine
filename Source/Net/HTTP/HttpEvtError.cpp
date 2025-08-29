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
    msg->getCacheOut().reset();
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
    ebody += R"(</h1><hr><br><p>Not supported currently, pls wait for more.</p><br><hr>
           </body></html>)";
    s8 tmp[128];
    msg->setStatus(mErr);
    msg->writeStatus(msg->getStatus(), "ERR");
    msg->dumpHeadOut();
    msg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", ebody.size()));
    msg->writeOutBody(ebody.data(), ebody.size());
    return msg->getHttpLayer()->sendResp(msg);
}


s32 HttpEvtError::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtError::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}

} // namespace app
