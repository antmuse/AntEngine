#include "Net/HTTP/Website.h"
#include "Logger.h"
#include "Net/HTTP/HttpEvtPath.h"
#include "Net/HTTP/HttpEvtFileReader.h"
#include "Net/HTTP/HttpEvtFileWriter.h"
#include "Net/HTTP/HttpEvtError.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Script/ScriptManager.h"



namespace app {
namespace net {

Website::Website(WebsiteCfg& cfg) : mConfig(cfg) {
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
    const s32 checkDisk = System::isExist(real);

    if (requrl.equalsn("/lua/", sizeof("/lua/") - 1)) {
        if (EDPF_FILE == checkDisk) {
            evt = new HttpEvtLua();
        } else {
            evt = new HttpEvtError(0 == checkDisk ? 404 : 403);
        }
    } else if (requrl.equalsn("/fs/", sizeof("/fs/") - 1)) {
        if (EDPF_FILE == checkDisk) {
            if (net::HTTP_GET == cmd) {
                evt = new HttpEvtFileReader();
            } else {
                evt = new HttpEvtError(401);
            }
        } else if (EDPF_PATH == checkDisk) {
            if (net::HTTP_GET == cmd) {
                evt = new HttpEvtPath();
            } else {
                evt = new HttpEvtError(401);
            }
        } else {
            if (net::HTTP_POST == cmd || net::HTTP_PUT == cmd) {
                evt = new HttpEvtFileWriter(); // upload
            } else {
                evt = new HttpEvtError(0 == checkDisk ? 404 : 403);
            }
        }
    } else { // readonly
        if (net::HTTP_GET == cmd && 1 == checkDisk) {
            evt = new HttpEvtFileReader();
        } else {
            evt = new HttpEvtError(0 == checkDisk ? 404 : 403);
        }
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
    if (1 == mConfig.mType) { // TLS
        mTlsContext.init(mConfig.mTLS);
    }
    script::ScriptManager& mng = script::ScriptManager::getInstance();
    script::Script nd;
    if (!nd.load(mng.getRootVM(), mConfig.mRootPath, "/lua/InitWebsite.lua", true, true)) {
        return;
    }
    bool ret = nd.exec(mng.getRootVM());
    nd.unload(mng.getRootVM());
    DLOG(ELL_INFO, "/lua/InitWebsite.lua init = %d", ret);
}



} // namespace net
} // namespace app