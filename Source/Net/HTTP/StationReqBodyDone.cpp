#include "Net/HTTP/StationReqBodyDone.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpFileRead.h"
#include "Net/HTTP/HttpFileSave.h"
#include "Net/HTTP/HttpEvtFilePath.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Net/HTTP/Website.h"
#include "System.h"

// def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {


HttpEventer* StationReqBodyDone::createEvt(HttpMsg* msg) {
    HttpEventer* evt = nullptr;
    net::Website* site = msg->getHttpLayer()->getWebsite();
    const String& real = msg->getRealPath();
    StringView requrl(
        real.c_str() + site->getConfig().mRootPath.size(), real.size() - site->getConfig().mRootPath.size());
    if (requrl.equalsn("/lua/", sizeof("/lua/") - 1)) {
        evt = new HttpEvtLua(requrl);
    } else if (requrl.equalsn("/api/", sizeof("/api/") - 1)) {
        // TODO
        msg->setStatus(500);
    } else if (requrl.equalsn("/fs/", sizeof("/fs/") - 1)) {
        s32 ck = System::isExist(real);
        if (0 == ck) {
            evt = new HttpFileRead();
        } else if (1 == ck) {
            evt = new HttpEvtFilePath();
        } else {
            msg->setStatus(404);
        }
    } else if (requrl.equalsn("/up/", sizeof("/up/") - 1)) {
        evt = new HttpFileSave();
    } else { // readonly
        if (0 == System::isExist(real)) {
            evt = new HttpFileRead();
        } else {
            msg->setStatus(404);
        }
    }

    const StringView str = HttpMsg::getMimeType(requrl.mData, requrl.mLen);
    msg->getHeadOut().writeContentType(str);

    StringView svv("If-Range", sizeof("If-Range") - 1);
    svv = msg->getHeadIn().get(svv);
    if (svv.mLen > 0) {
        svv.set("Range", sizeof("Range") - 1);
        // msg->getHeadOut().add()
    }

    return evt;
}

s32 StationReqBodyDone::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    HttpHead& hed = msg->getHeadOut();
    const String& host = msg->getHttpLayer()->getWebsite()->getConfig().mHost;
    StringView key(DSTRV("Host"));
    StringView val(host.c_str(), host.size());
    hed.add(key, val);

    HttpEventer* evt = createEvt(msg);
    if (!evt) {
        msg->setStationID(ES_ERROR);
        return EE_RETRY;
    }


    hed.writeChunked();

    key.set(DSTRV("Access-Control-Allow-Origin"));
    val.set(DSTRV("*"));
    hed.add(key, val);

    // key.set(DSTRV("Content-Type"));
    // val.set(DSTRV("text/html;charset=utf-8"));
    // hed.add(key, val);

    msg->writeStatus(200);
    msg->dumpHeadOut();
    msg->writeOutBody("\r\n", 2);
    // msg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", esz));

    if (EE_OK != evt->onOpen(msg)) {
        msg->setStationID(ES_ERROR);
        msg->setStatus(404);
        evt->drop();
        return EE_RETRY;
    }

    msg->setEvent(evt);
    evt->drop();
    msg->setStationID(ES_RESP_HEAD);
    return msg->getHttpLayer()->sendResp(msg);
}

} // namespace net
} // namespace app
