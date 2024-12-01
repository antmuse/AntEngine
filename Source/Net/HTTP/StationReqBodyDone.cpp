#include "Net/HTTP/StationReqBodyDone.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/Website.h"



namespace app {
namespace net {


s32 StationReqBodyDone::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    HttpEventer* evt = msg->getEvent();
    if (!evt || EE_OK != evt->onFinish(msg)) {
        msg->setStationID(ES_ERROR);
        return EE_RETRY;
    }
    msg->setStationID(ES_RESP_HEAD);
    return EE_OK;
}

} // namespace net
} // namespace app
