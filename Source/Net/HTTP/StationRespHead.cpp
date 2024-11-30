#include "Net/HTTP/StationRespHead.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {
namespace net {

s32 StationRespHead::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    s32 bsz = msg->getCacheOut().getSize();
    if (bsz > 0) {
        return msg->getHttpLayer()->sendResp(msg);
    }
    msg->setStationID(ES_RESP_BODY);
    return EE_RETRY;
}

} // namespace net
} // namespace app
