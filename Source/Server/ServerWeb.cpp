#include "ServerWeb.h"
#include "Logger.h"
#include "Net/HTTP/HttpEvtPath.h"
#include "Net/HTTP/HttpEvtFile.h"
#include "Net/HTTP/HttpEvtError.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Script/ScriptManager.h"



namespace app {
namespace net {

ServerWeb::ServerWeb(WebsiteCfg& cfg) : Website(cfg) {
}

ServerWeb::~ServerWeb() {
}

s32 ServerWeb::createMsgEvent(HttpMsg* msg) {
    if (!msg) {
        DLOG(ELL_ERROR, "HttpLayer::createMsgEvent>> msg=null");
        return EE_ERROR;
    }
    s32 ret = EE_OK;

    DLOG(ELL_INFO, "ip= %s, url= %s", msg->getHttpLayer()->getHandle().getRemote().getStr(),
        msg->getURL().data().c_str());

    // reset url
    String real(getConfig().mRootPath);
    if (msg->getURL().data() == "/") {
        real += "/index.html";
        // msg->getURL().append("index.html", sizeof("index.html") - 1);
        // msg->getURL().parser();
    } else {
        StringView path = msg->getURL().getPath();
        path.simplifyPath();
        real += path;
    }
    msg->setRealPath(real);
    StringView requrl(real.data() + getConfig().mRootPath.size(), real.size() - getConfig().mRootPath.size());

    net::HttpEventer* evt = nullptr;
    net::EHttpMethod cmd = msg->getMethod();
    const s32 checkDisk = System::isExist(real);

    if (requrl.equalsn("/lua/", sizeof("/lua/") - 1)) {
        if (1 == checkDisk) {
            evt = new HttpEvtLua();
        } else {
            evt = new HttpEvtError(0 == checkDisk ? 404 : 403);
        }
    } else if (requrl.equalsn("/fs/", sizeof("/fs/") - 1)) {
        if (1 == checkDisk) {
            if (net::HTTP_GET == cmd) {
                evt = new HttpEvtFile(true);
            } else {
                evt = new HttpEvtError(401);
            }
        } else if (2 == checkDisk) {
            if (net::HTTP_GET == cmd) {
                evt = new HttpEvtPath();
            } else {
                evt = new HttpEvtError(401);
            }
        } else {
            if (net::HTTP_POST == cmd || net::HTTP_PUT == cmd) {
                evt = new HttpEvtFile(false); // upload
            } else {
                evt = new HttpEvtError(0 == checkDisk ? 404 : 403);
            }
        }
    } else { // readonly
        if (net::HTTP_GET == cmd && 1 == checkDisk) {
            evt = new HttpEvtFile(true);
        } else {
            evt = new HttpEvtError(0 == checkDisk ? 404 : 403);
        }
    }

    msg->setEvent(evt);
    evt->drop();
    return ret;
}


} // namespace net
} // namespace app