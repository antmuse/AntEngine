#include "Net/HTTP/StationReqPath.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpFileRead.h"
#include "Net/HTTP/HttpFileSave.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Net/HTTP/Website.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


s32 StationReqPath::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    DLOG(ELL_INFO, "ip= %s, url= %s", msg->getHttpLayer()->getHandle().getRemote().getStr(),
        msg->getURL().get().c_str());

    // reset url

    // default page
    if (msg->getURL().get() == "/") {
        msg->getURL().append("index.html", sizeof("index.html") - 1);
        msg->getURL().parser();
    }
    return EE_OK;
}

} // namespace net
} // namespace app
