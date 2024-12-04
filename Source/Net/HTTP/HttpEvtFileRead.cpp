#include "Net/HTTP/HttpEvtFileRead.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Net/HTTP/MsgStation.h"

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtFileRead::HttpEvtFileRead() : mReaded(0), mMsg(nullptr), mBody(nullptr) {
    mReqs.mCall = HttpEvtFileRead::funcOnRead;
    // mReqs.mUser = this;
    mFile.setClose(EHT_FILE, HttpEvtFileRead::funcOnClose, this);
}

HttpEvtFileRead::~HttpEvtFileRead() {
    DASSERT(mMsg == nullptr);
}

s32 HttpEvtFileRead::onSent(net::HttpMsg* msg) {
    if (mReqs.mError) {
        return EE_ERROR;
    }
    if (mFile.isClosing()) {
        return EE_CLOSING;
    }
    return launchRead();
}

s32 HttpEvtFileRead::onOpen(net::HttpMsg* msg) {
    mMsg = nullptr;
    s32 ret = mFile.open(msg->getRealPath(), 1);
    if (EE_OK != ret) {
        mReqs.mError = ret;
        return ret;
    }
    grab(); // opened success, wait close callback

    mReaded = 0;
    mReqs.mError = 0;
    mReqs.mUser = nullptr;
    mBody = &msg->getCacheOut();

    msg->grab();
    mMsg = msg;
    return EE_OK;
}

s32 HttpEvtFileRead::onClose() {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
        mFile.launchClose();
    }
    return EE_OK;
}


s32 HttpEvtFileRead::onBodyPart(net::HttpMsg* msg) {
    return EE_CLOSING;
}

s32 HttpEvtFileRead::onFinish(net::HttpMsg* msg) {
    if (!mMsg) {
        return ELL_ERROR;
    }
    net::Website* site = dynamic_cast<net::Website*>(msg->getHttpLayer()->getMsgReceiver());
    net::HttpHead& hed = msg->getHeadOut();
    const String& host = site->getConfig().mHost;
    StringView key("Host", 4);
    StringView val(host.c_str(), host.size());
    hed.add(key, val);
    hed.setChunked();

    key.set("Access-Control-Allow-Origin", sizeof("Access-Control-Allow-Origin") - 1);
    val.set("*", 1);
    hed.add(key, val);

    // key.set(DSTRV("Content-Type"));
    // val.set(DSTRV("text/html;charset=utf-8"));
    // hed.add(key, val);

    msg->writeStatus(200);
    msg->dumpHeadOut();
    msg->writeOutBody("\r\n", 2);

    if (mFile.getFileSize() > 0) {
        s32 ret = launchRead();
        if (EE_OK != ret) {
            mFile.launchClose();
            return ret;
        }
    } else {
        mBody->write("0\r\n\r\n", 5);
        mFile.launchClose();
    }
    return msg->getHttpLayer()->sendResp(msg);
}

void HttpEvtFileRead::onFileClose(Handle* it) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    drop();
}

void HttpEvtFileRead::onFileRead(RequestFD* it) {
    if (it->mError) {
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
        Logger::log(ELL_ERROR, "HttpEvtFileRead::onFileRead>>err=%d, file=%s", it->mError, mFile.getFileName().c_str());
        mFile.launchClose();
        return;
    }
    if (it->mUsed > 0) {
        s8 chunked[8];
        snprintf(chunked, sizeof(chunked), "%04x\r\n", it->mUsed);
        mBody->rewrite(mChunkPos, chunked, G_BLOCK_HEAD_SIZE);
        mBody->commitTailPos(it->mUsed);
        mBody->write("\r\n", 2);
        mReaded += it->mUsed;
    }
    if (it->mUsed < it->mAllocated) {
        mBody->write("0\r\n\r\n", 5);
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
        mFile.launchClose();
    }

    it->mUsed = 0;
    it->mUser = nullptr;
    if (!mMsg || EE_OK != mMsg->getHttpLayer()->sendResp(mMsg)) {
        DLOG(ELL_ERROR, "HttpEvtFileRead::onFileRead>>post resp err file=%s", mFile.getFileName().c_str());
        mFile.launchClose();
    }
}

s32 HttpEvtFileRead::launchRead() {
    if (EE_OK != mReqs.mError) {
        return mReqs.mError;
    }
    if (mReqs.mUser) {
        return EE_OK;
    }
    mChunkPos = mBody->getTail();
    mReqs.mAllocated = mBody->peekTailNode(G_BLOCK_HEAD_SIZE, &mReqs.mData, 4 * 1024);
    mReqs.mUser = this;
    if (EE_OK != mFile.read(&mReqs, mReaded)) {
        mReqs.mUser = nullptr;
        return mFile.launchClose();
    }
    return EE_OK;
}


} // namespace app
