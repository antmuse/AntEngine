#include "Net/HTTP/Website.h"
#include "Logger.h"
#include "Net/HTTP/HttpEvtPath.h"
#include "Net/HTTP/HttpEvtFile.h"
#include "Net/HTTP/HttpEvtError.h"
#include "Net/HTTP/HttpEvtLua.h"



namespace app {
namespace net {

Website::Website(EngineConfig::WebsiteCfg& cfg) : mConfig(cfg) {
    init();
}

Website::~Website() {
    clear();
}

s32 Website::createMsgEvent(HttpMsg* msg) {
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

    if (requrl.equalsn("/lua/", sizeof("/lua/") - 1)) {
        evt = new HttpEvtLua(requrl);
    } else if (requrl.equalsn("/fs/", sizeof("/fs/") - 1)) {
        s32 ck = System::isExist(real);
        if (1 == ck) {
            if (net::HTTP_GET == cmd) {
                evt = new HttpEvtFile();
            } else {
                evt = new HttpEvtError(401);
            }
        } else if (2 == ck) {
            if (net::HTTP_GET == cmd) {
                evt = new HttpEvtPath();
            } else {
                evt = new HttpEvtError(401);
            }
        } else {
            if (net::HTTP_POST == cmd || net::HTTP_PUT == cmd) {
                evt = new HttpEvtFile(); // upload
            } else {
                evt = new HttpEvtError(404);
            }
        }
    } else { // readonly
        if (1 == System::isExist(real)) {
            evt = new HttpEvtFile();
        } else {
            evt = new HttpEvtError(404);
        }
    }

    const StringView str = HttpMsg::getMimeType(requrl.mData, requrl.mLen);
    msg->getHeadOut().setContentType(str);

    StringView svv("If-Range", sizeof("If-Range") - 1);
    svv = msg->getHeadIn().get(svv);
    if (svv.mLen > 0) {
        svv.set("Range", sizeof("Range") - 1);
        // msg->getHeadOut().add()
    }

    msg->setEvent(evt);
    evt->drop();
    return ret;
}


void Website::clear() {
    if (1 != mConfig.mType) { // not TLS
        return;
    }
    mTlsContext.uninit();
}

void Website::init() {
    if (1 != mConfig.mType) { // not TLS
        return;
    }
    mTlsContext.init(mConfig.mTLS);
}



} // namespace net
} // namespace app