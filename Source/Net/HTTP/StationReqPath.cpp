#include "Net/HTTP/StationReqPath.h"
#include "Net/HTTP/HttpLayer.h"
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
    net::Website* site = msg->getHttpLayer()->getWebsite();
    String fnm(site->getConfig().mRootPath);

    // default page
    if (msg->getURL().get() == "/") {
        fnm += "/index.html";
        // msg->getURL().append("index.html", sizeof("index.html") - 1);
        // msg->getURL().parser();
    } else {
        fnm += msg->getURL().getPath();
    }
    msg->setRealPath(fnm);
    return EE_OK;
}

} // namespace net
} // namespace app
