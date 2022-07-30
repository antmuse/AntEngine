#include "Net/HTTP/HttpFileRead.h"
#include "RingBuffer.h"

namespace app {

HttpFileRead::HttpFileRead()
    :mReaded(0)
    , mMsg(nullptr)
    , mBody(nullptr)
    , mDone(false) {

    mReqs.mCall = HttpFileRead::funcOnRead;
    //mReqs.mUser = this;

    mFile.setClose(EHT_FILE, HttpFileRead::funcOnClose, this);
}

HttpFileRead::~HttpFileRead() {
}

s32 HttpFileRead::onSent(net::HttpMsg& msg) {
    //go on to send body here
    return EE_OK;
}

s32 HttpFileRead::onOpen(net::HttpMsg& msg) {
    mDone = false;
    mReaded = 0;
    mBody = &msg.getCacheOut();

    String fnm(msg.getURL().getPath());

    s32 ret = mFile.open(fnm, 1);
    if (EE_OK == ret) {
        msg.grab();
        mMsg = &msg;
    }
    return ret;
}

s32 HttpFileRead::onClose() {
    mDone = true;
    if (mFile.isClose()) {
        delete this;
    } else {
        mFile.launchClose();
    }
    return EE_OK;
}

s32 HttpFileRead::onFinish(net::HttpMsg& msg) {
    return onBodyPart(msg);
}

void HttpFileRead::onFileClose(Handle* it) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mDone) {
        delete this;
    }
}

void HttpFileRead::onFileRead(net::RequestTCP* it) {
    if (it->mError) {
        return;
    }
    mBody->commitHead(it->mUsed);
    mReaded += it->mUsed;
    it->mUsed = 0;
    it->mUser = nullptr;
    if (mDone) {
        mFile.launchClose();
    } else {
        launchRead();
    }
}

s32 HttpFileRead::onBodyPart(net::HttpMsg& msg) {
    if (!mFile.isOpen()) {
        return EE_ERROR;
    }
    return launchRead();
}


s32 HttpFileRead::launchRead() {
    if (mReqs.mUser) {
        return EE_OK;
    }
    if (0 == mReqs.mUsed && mBody->getSize() > 0) {
        StringView buf = mBody->peekHead();
        mReqs.mData = buf.mData;
        mReqs.mAllocated = static_cast<u32>(buf.mLen);
        mReqs.mUsed = mReqs.mAllocated;
        mReqs.mUser = this;
        if (EE_OK != mFile.read(&mReqs, mReaded)) {
            return mFile.launchClose();
        }
    }
    return EE_OK;
}


}//namespace app
