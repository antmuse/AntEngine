#include "Net/HTTP/StationReqBody.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpFileRead.h"
#include "Net/HTTP/HttpFileSave.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Net/HTTP/Website.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


s32 StationReqBody::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    if (msg->getEvent()) {
        msg->getEvent()->onBodyPart(msg);
    }

    if (msg->isChunked()) {
    }
    return EE_OK;
}

} // namespace net
} // namespace app
