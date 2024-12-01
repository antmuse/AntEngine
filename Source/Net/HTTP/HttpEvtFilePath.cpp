#include "Net/HTTP/HttpEvtFilePath.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Net/HTTP/MsgStation.h"

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtFilePath::HttpEvtFilePath() : mOffset(0), mMsg(nullptr), mBody(nullptr) {
}

HttpEvtFilePath::~HttpEvtFilePath() {
    DASSERT(mMsg == nullptr);
}

s32 HttpEvtFilePath::onSent(net::HttpMsg* msg) {
    if (mOffset > mList.size()) {
        // finished
        return EE_CLOSING;
    }
    writeChunk();
    return EE_OK;
}

s32 HttpEvtFilePath::onOpen(net::HttpMsg* msg) {
    net::Website* site = msg->getHttpLayer()->getWebsite();
    if (!site) {
        return EE_ERROR;
    }
    mList.clear();
    System::getPathNodes(msg->getRealPath(), site->getConfig().mRootPath.size(), mList);
    mOffset = 0;
    mBody = &msg->getCacheOut();
    mMsg = msg;
    msg->grab();

    writeStr("<htm><head><title>simple path resp</title></head><body>Path List:<br><hr><br><ol>");
    // postResp();
    return EE_OK;
}

s32 HttpEvtFilePath::onClose() {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
        mOffset = mList.size() + 1;
    }
    return EE_OK;
}

s32 HttpEvtFilePath::onFinish(net::HttpMsg* msg) {
    return onBodyPart(msg);
}

void HttpEvtFilePath::writeStr(const s8* it) {
    DASSERT(it);
    s32 len = static_cast<s32>(strlen(it));
    s8 chunked[8];
    snprintf(chunked, sizeof(chunked), "%04x\r\n", len);
    mBody->write(chunked, G_BLOCK_HEAD_SIZE);
    mBody->write(it, len);
    mBody->write("\r\n", 2);
    postResp();
}

void HttpEvtFilePath::writeChunk() {
    if (mOffset < mList.size()) {
        mChunkPos = mBody->getTail();
        s8 chunked[256];
        mBody->write(chunked, G_BLOCK_HEAD_SIZE);
        s32 len = 0;
        for (; mBody->getSize() < D_RBUF_BLOCK_SIZE && mOffset < mList.size(); ++mOffset) {
            s32 add = snprintf(chunked, sizeof(chunked), "<li><a href=\"%s\">%s</a></li>", mList[mOffset].mFileName,
                mList[mOffset].mFileName);
            if (add < sizeof(chunked)) {
                mBody->write(chunked, add);
                len += add;
            } else {
                DLOG(ELL_ERROR, "HttpEvtFilePath::writeChunk too long, len=%d", add);
            }
        }
        snprintf(chunked, sizeof(chunked), "%04x\r\n", len);
        mBody->rewrite(mChunkPos, chunked, -G_BLOCK_HEAD_SIZE);
        mBody->write("\r\n", 2);
        postResp();
    } else if (mOffset == mList.size()) {
        ++mOffset;
        writeStr("</ol><br><hr><br>the end<br><hr></body></html>");
        mBody->write("0\r\n\r\n", 5);
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
        if (mMsg) {
            mMsg->getHttpLayer()->sendResp(mMsg);
            mMsg->drop();
            mMsg = nullptr;
        }
    }
}

void HttpEvtFilePath::postResp() {
    if (EE_OK != mMsg->getHttpLayer()->sendResp(mMsg)) {
        DLOG(ELL_ERROR, "HttpEvtFilePath::postResp fail, station=%d", mMsg->getStationID());
        mMsg->drop();
        mMsg = nullptr;
    }
}

s32 HttpEvtFilePath::onBodyPart(net::HttpMsg* msg) {
    return EE_OK;
}


} // namespace app
