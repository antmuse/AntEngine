#include "Net/HTTP/MsgStation.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpFileRead.h"
#include "Net/HTTP/HttpFileSave.h"

//def str view
#define DSTRV(V) V, sizeof(V) - 1

namespace app {
namespace net {

s32 StationInit::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    return 0;
}

s32 StationPath::check(HttpMsg* msg) {
    DASSERT(msg);

    String& requrl = const_cast<String&>(msg->getURL().get());
    requrl.replace('\\', '/');

    if (requrl.find("/../") >= 0) {
        //TODO fix this path
        msg->setStatus(403);
        msg->setStationID(ES_ERROR);
        return ES_ERROR;
    }

    
    return EE_OK;
}

s32 StationPath::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    if (EE_OK != check(msg)) {
        return EE_ERROR;
    }

    const String& requrl = msg->getURL().get();
    const StringView str = HttpMsg::getMimeType(requrl.c_str(), requrl.getLen());
    msg->getHeadOut().writeContentType(str);

    StringView svv("If-Range", sizeof("If-Range") - 1);
    svv = msg->getHeadIn().get(svv);
    if (svv.mLen > 0) {
        svv.set("Range", sizeof("Range") - 1);
        //msg->getHeadOut().add()
    }

    if (requrl == "/") {
        msg->getURL().append("index.html", sizeof("index.html") - 1);
    }

    if (requrl.equalsn("/api/", sizeof("/api/") - 1)) {

    } else if (requrl.equalsn("/fs/", sizeof("/fs/") - 1)) {
        HttpEventer* evt = new HttpFileRead();
        msg->setEvent(evt);
        evt->drop();
    } else {
        HttpEventer* evt = new HttpFileRead();
        msg->setEvent(evt);
        evt->drop();
    }

    return EE_OK;
}


s32 StationReqHead::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    msg->getHeadOut().writeKeepAlive(msg->isKeepAlive());

    if (msg->isChunked()) {
        //msg->getHeadIn().clear();
    } else {

    }

    return EE_OK;
}

s32 StationBody::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    if (msg->getEvent()) {
        msg->getEvent()->onBodyPart(*msg);
    }

    if (msg->isChunked()) {

    }
    return EE_OK;
}

s32 StationBodyDone::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    HttpHead& hed = msg->getHeadOut();
    StringView key(DSTRV("Host"));
    StringView val(DSTRV("127.0.0.1"));
    hed.add(key, val);

    key.set(DSTRV("Access-Control-Allow-Origin"));
    val.set(DSTRV("*"));
    hed.add(key, val);

    //key.set(DSTRV("Content-Type"));
    //val.set(DSTRV("text/html;charset=utf-8"));
    //hed.add(key, val);

    hed.writeChunked();

    msg->writeStatus(200);
    msg->dumpHeadOut();
    msg->writeOutBody("\r\n", 2);
    //msg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", esz));

    HttpEventer* evt = msg->getEvent();
    if (!evt || EE_OK != evt->onOpen(*msg)) {
        return EE_ERROR;
    }

    msg->setStationID(ES_RESP_HEAD);
    return EE_OK;
}


//+public headers
s32 StationRespHead::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    
    //s32 bsz = msg->getCacheOut().getSize();
    if (!msg->getHttpLayer()->sendResp(msg)) {
        return EE_ERROR;
    }

    msg->setStationID(ES_RESP_BODY);
    return EE_OK;
}

s32 StationRespBody::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    s32 bsz = msg->getCacheOut().getSize();
    if (0 == bsz) {
        msg->setStationID(ES_RESP_BODY_DONE);
        return EE_OK;
    }

    if (!msg->getHttpLayer()->sendResp(msg)) {
        return EE_ERROR;
    }

    return EE_OK;
}

s32 StationRespBodyDone::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    if (msg->getCacheOut().getSize() > 0) {
        msg->getHttpLayer()->sendResp(msg);
    }

    msg->setStationID(ES_CLOSE);
    return EE_OK;
}


s32 StationClose::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    msg->setEvent(nullptr);
    return EE_OK;
}


s32 StationError::onMsg(HttpMsg* msg) {
    DASSERT(msg);

    msg->getHeadOut().clear();
    static const s8* ebody = "<html>\n"
        "<head>\n"
        u8"<title> ���� </title>\n"
        "</head>\n"
        "<body>\n"
        u8"<hr><br>*_*<br><hr>"
        "</body>\n"
        "</html>";
    static const usz esz = strlen(ebody);

    s8 tmp[128];
    msg->writeStatus(msg->getStatus(), "ERR");
    msg->dumpHeadOut();
    msg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", esz));
    msg->writeOutBody(ebody, esz);

    msg->getHttpLayer()->sendResp(msg);

    return EE_OK;
}

} //namespace net
} //namespace app