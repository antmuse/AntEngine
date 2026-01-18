#include "Net/HTTP/HttpEvtShow.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

namespace app {


HttpEvtShow::HttpEvtShow() {
}

HttpEvtShow::~HttpEvtShow() {
}

s32 HttpEvtShow::onRespWrite(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtShow::onReqHeadDone(net::HttpMsg* msg) {
    DASSERT(msg);
    msg->getHttpLayer()->getHandle().getTlsSession()->showPeerCert();
    printf("-----------------head-----------------\n");
    net::HttpHead& hed = msg->getHead();
    for (usz i = 0; i < hed.size(); ++i) {
        printf("%s : %s\n", hed[i].mKey.data(), hed[i].mVal.data());
    }
    return EE_OK;
}

s32 HttpEvtShow::onLayerClose(net::HttpMsg* msg) {

    return EE_OK;
}

s32 HttpEvtShow::onReqBody(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtShow::onReadError(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtShow::onRespWriteError(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtShow::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtShow::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtShow::onReqBodyDone(net::HttpMsg* msg) {
    DASSERT(msg);
    printf("-----------------body-----------------\n");
    Packet& body = msg->getBody();
    printf("%.*s", static_cast<s32>(body.size()), body.data());
    body.clear();
    printf("\n-----------------body-end-------------\n");
    return EE_OK;
}


} // namespace app
