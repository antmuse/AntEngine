#include "Net/HTTP/HttpEvtFile.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

namespace app {
#define DSTRV(V) V, sizeof(V) - 1

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtFile::HttpEvtFile(bool readonly) :
    mOffset(0), mReadOnly(readonly), mReqBodyFinish(true), mMsg(nullptr), mNoBackend(false), mLayerClosed(false),
    mBody(nullptr) {
    mFile.setClose(EHT_FILE, HttpEvtFile::funcOnClose, this);
}

HttpEvtFile::~HttpEvtFile() {
    DASSERT(mMsg == nullptr);
}

s32 HttpEvtFile::onReadError(net::HttpMsg* msg) {
    return EE_ERROR;
}

s32 HttpEvtFile::onRespWriteError(net::HttpMsg* msg) {
    return EE_ERROR;
}

s32 HttpEvtFile::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtFile::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtFile::onRespWrite(net::HttpMsg* msg) {
    if (mNoBackend) {
        return EE_OK;
    }
    if (mReqs.mError) {
        return EE_ERROR;
    }
    if (mFile.isClosing()) {
        return EE_CLOSING;
    }
    return mReadOnly ? launchRead() : launchWrite();
}


void HttpEvtFile::onFileClose(Handle* it) {
    mReqs.mUser = nullptr;
    if (!mMsg) { // error
        DLOG(ELL_INFO, "onFileClose: msg= 0, drop file= %s", mFile.getFileName().data());
        // drop();
        return;
    }
    if (!mReadOnly) {
        if (mLayerClosed) {
            s32 ret = System::removeFile(mFile.getFileName());
            DLOG(ELL_INFO, "onFileClose: %s to remove file= %s", EE_OK == ret ? "success" : "fail",
                mFile.getFileName().data());
        } else {
            net::Website* site = mMsg->getHttpLayer()->getWebsite();
            net::HttpHead& hed = mMsg->getHeadOut();
            const String& host = site->getConfig().mHost;
            StringView key(DSTRV("Host"));
            StringView val(host.data(), host.size());
            hed.add(key, val);

            key.set(DSTRV("Access-Control-Allow-Origin"));
            val.set(DSTRV("*"));
            hed.add(key, val);

            val.set(DSTRV("application/json; charset=utf-8"));
            hed.setContentType(val);

            const s8* resp = R"({"ecode":0,"emsg":"success"})";
            if (mMsg->getCacheIn().getSize() == 0) {
                mMsg->setStatus(200);
                DLOG(ELL_INFO, "onFileWrite>>success up, file=%s", mFile.getFileName().data());
            } else {
                mMsg->setStatus(400, "ERR");
                resp = R"({"ecode":400,"emsg":"fail"})";
            }
            // hed.writeChunked();
            usz rlen = strlen(resp);
            hed.setLength(rlen);
            mMsg->buildResp();
            mMsg->writeOutBody(resp, rlen);
            mMsg->getHttpLayer()->sendResp(mMsg);
            DLOG(ELL_INFO, "onFileClose: save success, file= %s", mFile.getFileName().data());
        }
    }

    mMsg->drop();
    mMsg = nullptr;
    drop();
}


s32 HttpEvtFile::onLayerClose(net::HttpMsg* msg) {
    if (mMsg) {
        DASSERT(!mNoBackend);
        if (mFile.isClose()) {
            mMsg->drop();
            mMsg = nullptr;
            DLOG(ELL_INFO, "onLayerClose: msg drop");
        } else {
            s32 ret = mFile.launchClose();
            DLOG(ELL_INFO, "onLayerClose: delay msg drop, fclose ret=%d", ret);
        }
    } else {
        DLOG(ELL_INFO, "onLayerClose: msg=0, backend=%s", mNoBackend ? "none" : "yes");
    }
    mLayerClosed = true;
    return EE_OK;
}


s32 HttpEvtFile::onReqHeadDone(net::HttpMsg* msg) {
    mMsg = nullptr;
    mLayerClosed = false;
    mReqBodyFinish = false;
    mNoBackend = false;

    net::EHttpMethod cmd = msg->getMethod();
    switch (cmd) {
    case net::HTTP_GET:
    {
        s32 ret = mFile.open(msg->getRealPath(), 1);
        if (EE_OK != ret) {
            mReqs.mError = ret;
            return ret;
        }
        mReqs.mCall = HttpEvtFile::funcOnRead;
        break;
    }
    case net::HTTP_PUT:
    case net::HTTP_POST:
    {
        System::createPath(msg->getRealPath());
        if (AppIsPathDelimiter(msg->getRealPath().lastChar())) {
            // just create a path don't req on HandleFile
            mNoBackend = true;
            return sendRespHead(msg);
        }
        s32 ret = mFile.open(msg->getRealPath(), 2 | 4);
        if (EE_OK != ret) {
            return EE_NO_WRITEABLE;
        }
        mReqs.mCall = HttpEvtFile::funcOnWrite;
        break;
    }
    case net::HTTP_DELETE:
        // don't req on HandleFile
        mNoBackend = true;
        System::removeFile(msg->getRealPath());
        return sendRespHead(msg);
    default:
        return EE_ERROR;
    }

    // grab for backend: HandleFile
    grab();
    msg->grab();

    mMsg = msg;
    mOffset = 0;
    mReqs.mError = 0;
    mReqs.mUser = nullptr;
    if (mReadOnly) {
        mBody = &msg->getCacheOut();
        return sendRespHead(msg);
    } else {
        mBody = &msg->getCacheIn();
    }
    return EE_OK;
}


s32 HttpEvtFile::onReqBody(net::HttpMsg* msg) {
    if (mReadOnly || mNoBackend) {
        return EE_OK; // skip body if
    }
    return launchWrite();
}


s32 HttpEvtFile::onReqBodyDone(net::HttpMsg* msg) {
    mReqBodyFinish = true;
    if (mNoBackend) {
        return EE_OK;
    }
    if (!mReadOnly) {
        return launchWrite();
    }
    return EE_OK;
}


s32 HttpEvtFile::sendRespHead(net::HttpMsg* msg) {
    net::Website* site = msg->getHttpLayer()->getWebsite();
    net::HttpHead& hed = msg->getHeadOut();
    const String& host = site->getConfig().mHost;
    StringView key("Host", 4);
    StringView val(host.data(), host.size());
    hed.add(key, val);
    hed.setChunked();

    key.set("Access-Control-Allow-Origin", sizeof("Access-Control-Allow-Origin") - 1);
    val.set("*", 1);
    hed.add(key, val);

    // key.set(DSTRV("Content-Type"));
    // val.set(DSTRV("text/html;charset=utf-8"));
    // hed.add(key, val);

    msg->setStatus(200);
    msg->buildResp();

    if (mFile.getFileSize() > 0) {
        s32 ret = launchRead();
        if (EE_OK != ret) {
            mFile.launchClose();
            return ret;
        }
    } else {
        msg->writeOutBody("0\r\n\r\n", 5);
        mFile.launchClose();
    }
    return msg->getHttpLayer()->sendResp(msg);
}


s32 HttpEvtFile::launchWrite() {
    if (mReqs.mUsed > 0 || mReqs.mUser) {
        return EE_OK;
    }
    if (mMsg->getCacheIn().getSize() <= 0) {
        DLOG(ELL_INFO, "launchWrite: empty req_body %s, file=%s", mReqBodyFinish ? "finish" : "unfinish",
            mFile.getFileName().data());
        if (mReqBodyFinish) {
            return mFile.launchClose();
        }
        return EE_OK;
    }
    StringView buf = mMsg->getCacheIn().peekHead();
    mReqs.mData = buf.mData;
    mReqs.mAllocated = static_cast<u32>(buf.mLen);
    mReqs.mUsed = mReqs.mAllocated;
    mReqs.mUser = this;
    if (EE_OK != mFile.write(&mReqs, mOffset)) {
        DLOG(ELL_ERROR, "launchWrite: err=%d, file=%s", mReqs.mError, mFile.getFileName().data());
        return mFile.launchClose();
    }
    return EE_OK;
}


void HttpEvtFile::onFileWrite(RequestFD* it) {
    if (it->mError) {
        DLOG(ELL_ERROR, "onFileWrite>>err=%d, file=%s", it->mError, mFile.getFileName().data());
        mFile.launchClose();
        return;
    }
    mMsg->getCacheIn().commitHead(it->mUsed);
    mOffset += it->mUsed;
    it->mUser = nullptr;
    it->mUsed = 0;
    if (mMsg->getCacheIn().getSize() > 0) {
        launchWrite();
        return;
    }
    if (mReqBodyFinish) {
        mFile.launchClose();
        DLOG(ELL_INFO, "onFileWrite: finish file=%s", mFile.getFileName().data());
    } else {
        DLOG(ELL_INFO, "onFileWrite: body empty, file=%s", mFile.getFileName().data());
    }
}


void HttpEvtFile::onFileRead(RequestFD* it) {
    if (it->mError || mLayerClosed) {
        DLOG(ELL_ERROR, "onFileRead: err=%d, file=%s", it->mError, mFile.getFileName().data());
        mFile.launchClose();
        return;
    }
    if (it->mUsed > 0) {
        s8 chunked[8];
        snprintf(chunked, sizeof(chunked), "%04x\r\n", it->mUsed);
        mBody->rewrite(mChunkPos, chunked, G_BLOCK_HEAD_SIZE);
        mBody->commitTailPos(it->mUsed);
        mBody->write("\r\n", 2);
        mOffset += it->mUsed;
    }
    if (mOffset >= mFile.getFileSize()) {
        mBody->write("0\r\n\r\n", 5);
        mFile.launchClose();
        DLOG(ELL_INFO, "onFileRead: finish file=%s, size=%lu", mFile.getFileName().data(), mOffset);
    }
    it->mUser = nullptr;
    it->mUsed = 0;
    if (!mMsg || EE_OK != mMsg->getHttpLayer()->sendResp(mMsg)) {
        DLOG(ELL_ERROR, "onFileRead: post resp err file=%s", mFile.getFileName().data());
        mFile.launchClose();
    }
}


s32 HttpEvtFile::launchRead() {
    if (EE_OK != mReqs.mError || mLayerClosed || mReqs.mUser) {
        return mReqs.mError;
    }
    if (mOffset >= mFile.getFileSize()) {
        mBody->write("0\r\n\r\n", 5);
        mFile.launchClose();
        DLOG(ELL_ERROR, "onFileRead: finish file=%s", mFile.getFileName().data());
        return EE_OK;
    }
    mChunkPos = mBody->getTail();
    mReqs.mAllocated = mBody->peekTailNode(G_BLOCK_HEAD_SIZE, &mReqs.mData, 4 * 1024);
    mReqs.mUser = this;
    if (EE_OK != mFile.read(&mReqs, mOffset)) {
        return mFile.launchClose();
    }
    return EE_OK;
}


} // namespace app
