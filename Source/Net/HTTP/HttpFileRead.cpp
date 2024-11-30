#include "Net/HTTP/HttpFileRead.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Net/HTTP/MsgStation.h"

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpFileRead::HttpFileRead() : mReaded(0), mMsg(nullptr), mBody(nullptr) {
    mReqs.mCall = HttpFileRead::funcOnRead;
    // mReqs.mUser = this;
    mFile.setClose(EHT_FILE, HttpFileRead::funcOnClose, this);
}

HttpFileRead::~HttpFileRead() {
    DASSERT(mMsg == nullptr);
}

s32 HttpFileRead::onSent(net::HttpMsg* msg) {
    if (mReqs.mError) {
        return EE_ERROR;
    }
    if (mFile.isClosing()) {
        return EE_CLOSING;
    }
    return launchRead();
}

s32 HttpFileRead::onOpen(net::HttpMsg* msg) {
    net::Website* site = msg->getHttpLayer()->getWebsite();
    if (!site) {
        return EE_ERROR;
    }
    mMsg = nullptr;
    String fnm(site->getConfig().mRootPath);
    fnm += msg->getURL().getPath();
    s32 ret = mFile.open(fnm, 1);
    if (EE_OK != ret) {
        mReqs.mError = ret;
        return ret;
    }
    grab(); // opened success, wait close callback

    mReaded = 0;
    mReqs.mError = 0;
    mReqs.mUser = nullptr;
    mBody = &msg->getCacheOut();
    ret = launchRead();
    if (EE_OK != ret) {
        mFile.launchClose();
        return ret;
    }

    msg->grab();
    mMsg = msg;
    return EE_OK;
}

s32 HttpFileRead::onClose() {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
        mFile.launchClose();
    }
    return EE_OK;
}

s32 HttpFileRead::onFinish(net::HttpMsg* msg) {
    return onBodyPart(msg);
}

void HttpFileRead::onFileClose(Handle* it) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    drop();
}

void HttpFileRead::onFileRead(RequestFD* it) {
    if (it->mError) {
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
        mFile.launchClose();
        Logger::log(ELL_ERROR, "HttpFileRead::onFileRead>>err file=%s", mFile.getFileName().c_str());
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
    if (mMsg) {
        mMsg->getHttpLayer()->sendResp(mMsg);
    } else {
        mFile.launchClose();
    }
}

s32 HttpFileRead::onBodyPart(net::HttpMsg* msg) {
    if (!mFile.isOpen()) {
        return EE_ERROR;
    }
    return launchRead();
}


s32 HttpFileRead::launchRead() {
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
