#include "Net/HTTP/MsgStation.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Net/HTTP/Website.h"


namespace app {
namespace net {

s32 StationInit::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    return 0;
}


} // namespace net
} // namespace app
