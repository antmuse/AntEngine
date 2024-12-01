#include "Net/HTTP/HttpEvtFileSave.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

#define DSTRV(V) V, sizeof(V) - 1

namespace app {

HttpEvtFileSave::HttpEvtFileSave() : mWrited(0), mBody(nullptr), mMsg(nullptr), mFinish(false) {
    mReqs.mCall = HttpEvtFileSave::funcOnWrite;
    mReqs.mUser = this;
    mFile.setClose(EHT_FILE, HttpEvtFileSave::funcOnClose, this);
}

HttpEvtFileSave::~HttpEvtFileSave() {
}

s32 HttpEvtFileSave::onSent(net::HttpMsg* msg) {
    // DASSERT(0);
    return EE_CLOSING;
}

s32 HttpEvtFileSave::onOpen(net::HttpMsg* msg) {
    DASSERT(msg);
    mFinish = false;
    mBody = &msg->getCacheIn();
    s32 ret = mFile.open(msg->getRealPath(), 4);
    if (EE_OK == ret) {
        mWrited = mFile.getFileSize();
        msg->grab();
        mMsg = msg;
        grab();
    }
    return ret;
}

s32 HttpEvtFileSave::onClose() {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
        mFile.launchClose();
    }
    return EE_OK;
}

void HttpEvtFileSave::onFileClose(Handle* it) {
    if (!mMsg) { // error
        DLOG(ELL_ERROR, "onFileWrite>>fail msg, file=%p", mFile.getFileName().c_str());
        drop();
        return;
    }
    if (!mMsg->getHttpLayer()->getWebsite()) { // error
        DLOG(ELL_ERROR, "onFileWrite>>fail net, file=%p", mFile.getFileName().c_str());
        mMsg->drop();
        mMsg = nullptr;
        drop();
        return;
    }
    net::HttpHead& hed = mMsg->getHeadOut();
    const String& host = mMsg->getHttpLayer()->getWebsite()->getConfig().mHost;
    StringView key(DSTRV("Host"));
    StringView val(host.c_str(), host.size());
    hed.add(key, val);

    key.set(DSTRV("Access-Control-Allow-Origin"));
    val.set(DSTRV("*"));
    hed.add(key, val);

    val.set(DSTRV("application/json; charset=utf-8"));
    hed.writeContentType(val);

    const s8* resp = "{\"ecode\":0,\"emsg\":\"success\"}";
    if (mFinish && mBody->getSize() == 0) {
        mMsg->writeStatus(200);
        DLOG(ELL_INFO, "onFileWrite>>success up, file=%p", mFile.getFileName().c_str());
    } else {
        mMsg->writeStatus(500);
        resp = "{\"ecode\":500,\"emsg\":\"fail\"}";
    }
    // hed.writeChunked();
    usz rlen = strlen(resp);
    hed.writeLength(rlen);
    mMsg->dumpHeadOut();
    // mMsg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", strlen(resp)));
    mMsg->writeOutBody("\r\n", 2);
    mMsg->writeOutBody(resp, rlen);
    mMsg->getHttpLayer()->sendResp(mMsg);
    mMsg->drop();
    mMsg = nullptr;
    drop();
}

void HttpEvtFileSave::onFileWrite(RequestFD* it) {
    if (it->mError) {
        DLOG(ELL_ERROR, "onFileWrite>>err=%d, file=%p", it->mError, mFile.getFileName().c_str());
        mFile.launchClose();
        return;
    }
    mBody->commitHead(it->mUsed);
    mWrited += it->mUsed;
    it->mUsed = 0;

    if (!mMsg) {
        DLOG(ELL_ERROR, "onFileWrite>>msg closed, file=%p", mFile.getFileName().c_str());
        mFile.launchClose();
        return;
    }
    if (mBody->getSize() > 0) {
        launchWrite();
        return;
    }
    if (mFinish) {
        mFile.launchClose();
    }
}


s32 HttpEvtFileSave::onFinish(net::HttpMsg* msg) {
    mFinish = true;
    return onBodyPart(msg);
}


s32 HttpEvtFileSave::onBodyPart(net::HttpMsg* msg) {
    if (!mMsg) {
        return EE_CLOSING;
    }
    return launchWrite();
}


s32 HttpEvtFileSave::launchWrite() {
    if (mReqs.mUsed > 0) {
        return EE_OK;
    }
    if (mBody->getSize() <= 0) {
        return EE_OK;
    }
    StringView buf = mBody->peekHead();
    mReqs.mData = buf.mData;
    mReqs.mAllocated = static_cast<u32>(buf.mLen);
    mReqs.mUsed = mReqs.mAllocated;
    if (EE_OK != mFile.write(&mReqs, mWrited)) {
        DLOG(ELL_ERROR, "launchWrite>>err=%d, file=%p", mReqs.mError, mFile.getFileName().c_str());
        return mFile.launchClose();
    }
    return EE_OK;
}


} // namespace app
