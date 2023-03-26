#include "HttpFileSave.h"
#include "RingBuffer.h"

namespace app {

HttpFileSave::HttpFileSave()
    :mWrited(0)
    , mBody(nullptr)
    , mDone(false) {

    mReqs.mCall = HttpFileSave::funcOnWrite;
    mReqs.mUser = this;

    mFile.setClose(EHT_FILE, HttpFileSave::funcOnClose, this);
}

HttpFileSave::~HttpFileSave() {
}

s32 HttpFileSave::onSent(net::HttpMsg& msg) {
    //go on to send body here
    return EE_OK;
}

s32 HttpFileSave::onOpen(net::HttpMsg& msg) {
    mDone = false;
    mBody = &msg.getCacheOut();
    return mFile.open("Log/httpfile.html", 4);
}

s32 HttpFileSave::onClose() {
    mDone = true;
    if (mFile.isClose()) {
        delete this;
    } else {
        mFile.launchClose();
    }
    return EE_OK;
}

s32 HttpFileSave::onFinish(net::HttpMsg& msg) {
    return onBodyPart(msg);
}

void HttpFileSave::onFileClose(Handle* it) {
    if (mDone) {
        delete this;
    }
}

void HttpFileSave::onFileWrite(RequestFD* it) {
    if (it->mError) {
        return;
    }
    mBody->commitHead(it->mUsed);
    mWrited += it->mUsed;
    it->mUsed = 0;
    if (mDone) {
        mFile.launchClose();
    } else {
        launchWrite();
    }
}

s32 HttpFileSave::onBodyPart(net::HttpMsg& msg) {
    if (!mFile.isOpen()) {
        return 0;
    }
    return launchWrite();
}


s32 HttpFileSave::launchWrite() {
    if (0 == mReqs.mUsed && mBody->getSize() > 0) {
        StringView buf = mBody->peekHead();
        mReqs.mData = buf.mData;
        mReqs.mAllocated = buf.mLen;
        mReqs.mUsed = mReqs.mAllocated;
        if (EE_OK != mFile.write(&mReqs, mWrited)) {
            return mFile.launchClose();
        }
    }
    return EE_OK;
}


}//namespace app
