#include "Net/HTTP/HttpEvtPath.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtPath::HttpEvtPath() : mOffset(0), mMsg(nullptr) {
}

HttpEvtPath::~HttpEvtPath() {
    DASSERT(mMsg == nullptr);
}

s32 HttpEvtPath::onReqBody(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtPath::onReadError(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtPath::onRespWriteError(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtPath::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtPath::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtPath::onReqHeadDone(net::HttpMsg* msg) {
    net::Website* site = msg->getHttpLayer()->getWebsite();
    if (!site) {
        return EE_ERROR;
    }
    mList.clear();
    System::getPathNodes(msg->getRealPath(), site->getConfig().mRootPath.size(), mList);
    mOffset = 0;
    //mMsg = msg;
    //msg->grab();
    return EE_OK;
}


s32 HttpEvtPath::onLayerClose(net::HttpMsg* msg) {
    //if (mMsg) {
    //    mMsg->drop();
    //    mMsg = nullptr;
    //    mOffset = mList.size() + 1;
    //}
    return EE_OK;
}


s32 HttpEvtPath::onReqBodyDone(net::HttpMsg* msg) {
    net::Website* site = msg->getHttpLayer()->getWebsite();
    net::HttpHead& hed = msg->getHeadOut();
    const String& host = site->getConfig().mHost;
    StringView key("Host", 4);
    StringView val(host.c_str(), host.size());
    hed.add(key, val);

    key.set("Access-Control-Allow-Origin", sizeof("Access-Control-Allow-Origin") - 1);
    val.set("*", 1);
    hed.add(key, val);

    // key.set(DSTRV("Content-Type"));
    // val.set(DSTRV("text/html;charset=utf-8"));
    // hed.add(key, val);

    hed.setChunked();

    msg->writeStatus(200);
    msg->dumpHeadOut();
    msg->writeOutBody("\r\n", 2);
    // msg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", esz));
    writeStr(msg, "<htm><head><title>simple path resp</title></head><body>Path List:<br><hr><br><ol>");
    return msg->getHttpLayer()->sendResp(msg);
}


void HttpEvtPath::writeStr(net::HttpMsg* msg, const s8* it) {
    DASSERT(it);
    s32 len = static_cast<s32>(strlen(it));
    s8 chunked[8];
    snprintf(chunked, sizeof(chunked), "%04x\r\n", len);
    msg->getCacheOut().write(chunked, G_BLOCK_HEAD_SIZE);
    msg->getCacheOut().write(it, len);
    msg->getCacheOut().write("\r\n", 2);
    postResp(msg);
}


s32 HttpEvtPath::onRespWrite(net::HttpMsg* msg) {
    if (mOffset > mList.size()) {
        return EE_OK;
    }
    if (mOffset < mList.size()) {
        SRingBufPos chunkPos = msg->getCacheOut().getTail();
        s8 chunked[256];
        msg->getCacheOut().write(chunked, G_BLOCK_HEAD_SIZE);
        s32 len = 0;
        for (; msg->getCacheOut().getSize() < D_RBUF_BLOCK_SIZE && mOffset < mList.size(); ++mOffset) {
            s32 add = snprintf(chunked, sizeof(chunked), "<li><a href=\"%s\">%s</a></li>", mList[mOffset].mFileName,
                mList[mOffset].mFileName);
            if (add < sizeof(chunked)) {
                msg->getCacheOut().write(chunked, add);
                len += add;
            } else {
                DLOG(ELL_ERROR, "HttpEvtPath::writeChunk too long, len=%d", add);
            }
        }
        snprintf(chunked, sizeof(chunked), "%04x\r\n", len);
        msg->getCacheOut().rewrite(chunkPos, chunked, -G_BLOCK_HEAD_SIZE);
        msg->getCacheOut().write("\r\n", 2);
        return postResp(msg);
    }

    // if (mOffset == mList.size()) :
    ++mOffset;
    writeStr(msg, "</ol><br><hr><br>the end<br><hr></body></html>");
    msg->getCacheOut().write("0\r\n\r\n", 5);
    return postResp(msg);
}


s32 HttpEvtPath::postResp(net::HttpMsg* msg) {
    s32 ret = msg->getHttpLayer()->sendResp(msg);
    if (EE_OK != ret) {
        DLOG(ELL_ERROR, "HttpEvtPath::postResp fail, status=%d", msg->getStatus());
        msg->drop();
        msg = nullptr;
    }
    return ret;
}


} // namespace app
