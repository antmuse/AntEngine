#include "Net/HTTP/StationRespBody.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/Website.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


s32 StationRespBody::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    s32 bsz = msg->getCacheOut().getSize();
    if (bsz > 0) {
        return msg->getHttpLayer()->sendResp(msg);
    }

    HttpEventer* evt = msg->getEvent();
    if (!evt) {
        msg->setStationID(ES_ERROR);
        return EE_RETRY;
    }

    s32 ret = evt->onSent(msg);
    if (EE_OK == ret) {
        return EE_OK;
    }

    if (EE_CLOSING == ret) {
        msg->setStationID(ES_RESP_BODY_DONE);
    } else {
        //msg->setStationID(ES_ERROR);
        msg->setStationID(ES_CLOSE);
    }
    return EE_RETRY;
}

} // namespace net
} // namespace app
