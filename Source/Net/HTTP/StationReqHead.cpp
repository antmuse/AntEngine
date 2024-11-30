#include "Net/HTTP/StationReqHead.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpFileRead.h"
#include "Net/HTTP/HttpFileSave.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Net/HTTP/Website.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


s32 StationReqHead::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    msg->getHeadOut().writeKeepAlive(msg->isKeepAlive());

    if (msg->isChunked()) {
        // msg->getHeadIn().clear();
    } else {
    }

    return EE_OK;
}

} // namespace net
} // namespace app
