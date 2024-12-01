#include "Net/HTTP/StationClose.h"
#include "Net/HTTP/HttpLayer.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


s32 StationClose::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    HttpEventer* evt = msg->getEvent();
    if (evt) {
        evt->onClose();
        msg->setEvent(nullptr);
    }
    return EE_OK;
}


} // namespace net
} // namespace app
