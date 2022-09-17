#include "Net/HTTP/HttpFileRead.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

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
    net::Website* site = msg.getHttpLayer()->getWebsite();
    String fnm(site->getConfig().mRootPath);
    fnm += msg.getURL().getPath();
    if (EE_OK == mFile.open(fnm, 1) && EE_OK == launchRead()) {
        msg.grab();
        grab();
        mMsg = &msg;
        return EE_OK;
    }
    return EE_ERROR;
}

s32 HttpFileRead::onClose() {
    mDone = true;
    if (!mFile.isClose()) {
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
        drop();
    }
}

void HttpFileRead::onFileRead(net::RequestTCP* it) {
    if (it->mError) {
        return;
    }
    if (it->mUsed > 0) {
        s8 chunked[8];
        snprintf(chunked, sizeof(chunked), "%04X\r\n", it->mUsed);
        mBody->rewrite(mChunkPos, chunked, G_BLOCK_HEAD_SIZE);
        mBody->commitTailPos(it->mUsed);
        //mBody->write("\r\n", 2);
        if (it->mUsed < it->mAllocated) {
            mDone = true;
            mBody->write("0\r\n\r\n", 5);
        }
    } else {
        mDone = true;
        mBody->write("0\r\n\r\n", 5);
    }

    net::Website* site = mMsg->getHttpLayer()->getWebsite();
    if (site) {
        site->stepMsg(mMsg);
    }

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
        return EE_RETRY;
    }
    if (0 == mReqs.mUsed) {
        mChunkPos = mBody->getTail();
        mReqs.mAllocated = mBody->peekTailNode(G_BLOCK_HEAD_SIZE, &mReqs.mData, 4 * 1024);
        mReqs.mUser = this;
        if (EE_OK != mFile.read(&mReqs, mReaded)) {
            mReqs.mUser = nullptr;
            return mFile.launchClose();
        }
        return EE_OK;
    }
    return EE_ERROR;
}


}//namespace app