#include "Net/HTTP/StationRespBodyDone.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpFileRead.h"
#include "Net/HTTP/HttpFileSave.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Net/HTTP/Website.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


//wait to send out the leftover
s32 StationRespBodyDone::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    if (msg->getCacheOut().getSize() > 0) {
        return msg->getHttpLayer()->sendResp(msg);
    }

    msg->setStationID(ES_CLOSE);
    return EE_RETRY;
}


} // namespace net
} // namespace app
