#include "Net/HTTP/HttpEvtPath.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtPath::HttpEvtPath() {
}

HttpEvtPath::~HttpEvtPath() {
    // DASSERT(mMsg == nullptr);
    // DASSERT(mMsgResp == nullptr);
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mMsgResp) {
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
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
    return EE_OK;
}


s32 HttpEvtPath::onLayerClose(net::HttpMsg* msg) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    return EE_OK;
}


s32 HttpEvtPath::onReqBodyDone(net::HttpMsg* msg) {
    msg->grab();
    mMsg = msg;

    net::Website* site = msg->getHttpLayer()->getWebsite();
    mList.clear();
    System::getPathNodes(msg->getRealPath(), site->getConfig().mRootPath.size(), mList);
    mOffset = 0;
    mMsgResp = new net::HttpMsg(msg->getHttpLayer());
    mMsgResp->setEvent(this);

    net::HttpHead& hed = mMsgResp->getHead();
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
    hed.setDefaultContentType();

    hed.setChunked();

    mMsgResp->setStatus(200);
    mMsgResp->writeChunk("<htm><head><title>simple path resp</title></head><body>Path List:<br><hr><br><ol>");
    return postResp();
}


s32 HttpEvtPath::onRespWrite(net::HttpMsg* msg) {
    if (!mMsgResp) {
        DLOG(ELL_INFO, "onRespWrite, not any more data");
        return EE_OK;
    }
    if (!mMsg) {
        DLOG(ELL_ERROR, "onRespWrite, disconnection when resp");
        return EE_OK;
    }
    DASSERT(msg == mMsgResp);
    mMsgResp->clearBody();
    if (mOffset < mList.size()) {
        mMsgResp->getBody().reallocate(D_RBUF_BLOCK_SIZE);
        s8 chunked[256];
        mMsgResp->writeBody(chunked, G_BLOCK_HEAD_SIZE);
        s32 len = 0;
        for (; mMsgResp->getBody().size() < D_RBUF_BLOCK_SIZE && mOffset < mList.size(); ++mOffset) {
            s32 add = snprintf(chunked, sizeof(chunked), "<li><a href=\"%s\">%s</a></li>", mList[mOffset].mFileName,
                mList[mOffset].mFileName);
            if (add < sizeof(chunked)) {
                mMsgResp->writeBody(chunked, add);
                len += add;
            } else {
                DLOG(ELL_ERROR, "HttpEvtPath::writeChunk too long, len=%d", add);
            }
        }
        snprintf(chunked, sizeof(chunked), "%04x\r\n", len);
        memcpy(mMsgResp->getBody().data(), chunked, G_BLOCK_HEAD_SIZE);
        mMsgResp->writeBody("\r\n", 2);
        if (mOffset < mList.size()) {
            return postResp();
        }
    }

    ++mOffset;
    mMsgResp->writeChunk("</ol><br><hr><br>the end<br><hr></body></html>");
    mMsgResp->writeLastChunk();
    s32 ret = postResp();
    if (EE_OK == ret) {
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
    return ret;
}


s32 HttpEvtPath::postResp() {
    s32 ret = mMsgResp->getHttpLayer()->sendOut(mMsgResp);
    if (EE_OK != ret) {
        DLOG(ELL_ERROR, "HttpEvtPath::postResp fail, status=%d", mMsgResp->getStatus());
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
    return ret;
}


} // namespace app
