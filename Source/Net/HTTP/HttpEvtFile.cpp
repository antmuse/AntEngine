#include "Net/HTTP/HttpEvtFile.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"

// TODO: fix file upload

namespace app {
#define DSTRV(V) V, sizeof(V) - 1

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtFile::HttpEvtFile(bool readonly) : mReadOnly(readonly) {
}

HttpEvtFile::~HttpEvtFile() {
    DASSERT(mMsg == nullptr);
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mMsgResp) {
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
    if (mFile) {
        delete mFile;
        mFile = nullptr;
    }
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
    if (!mMsgResp) {
        return EE_OK;
    }
    if (mReqs.mError) {
        return EE_ERROR;
    }
    if (mFile->isClosing()) {
        return EE_CLOSING;
    }
    return mReadOnly ? launchRead() : launchWrite();
}


void HttpEvtFile::onFileClose(Handle* it) {
    // mReqs.mUser = nullptr;
    if (!mReadOnly) {
        if (mMsg) {
            if (mReqBodyFinish && mCache.empty()) {
                DLOG(ELL_INFO, "onFileWrite>>success up, file=%s", mFile->getFileName().data());
                const s8* resp = R"({"ecode":0,"emsg":"success"})";
                sendRespHead(mMsg, net::HTTP_STATUS_OK, resp, true, false);
            } else {
                s32 ret = System::removeFile(mFile->getFileName());
                DLOG(ELL_INFO, "onFileClose: frontend closed, %s to remove file= %s", EE_OK == ret ? "success" : "fail",
                    mFile->getFileName().data());
                const s8* resp = R"({"ecode":400,"emsg":"fail"})";
                DLOG(ELL_INFO, "onFileWrite>>fail up, file=%s", mFile->getFileName().data());
                sendRespHead(mMsg, net::HTTP_STATUS_FORBIDDEN, resp, true, false);
            }

            net::HttpHead& hed = mMsgResp->getHead();

            StringView key(DSTRV("Access-Control-Allow-Origin"));
            StringView val(DSTRV("*"));
            hed.add(key, val);

            val.set(DSTRV("application/json; charset=utf-8"));
            hed.setContentType(val);

            mMsgResp->writeLastChunk();
            s32 ret = mMsgResp->getHttpLayer()->sendOut(mMsgResp);
            DLOG(ELL_INFO, "onFileClose: file= %s, last resp = %d", mFile->getFileName().data(), ret);
        } else {
            DLOG(ELL_INFO, "onFileClose: file= %s, need remove", mFile->getFileName().data());
        }
    }

    // mReqBodyFinish
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mMsgResp) {
        mMsgResp->drop();
        mMsgResp = nullptr;
    }

    DASSERT(mFile);
    delete mFile;
    mFile = nullptr;
    drop(); // drop for HandleFile
}


s32 HttpEvtFile::onLayerClose(net::HttpMsg* msg) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
        DLOG(ELL_INFO, "onLayerClose: msg drop");
    } else {
        DLOG(ELL_INFO, "onLayerClose: msg = 0, %p", msg);
    }
    if (mFile) {
        s32 ret = mFile->launchClose();
        DLOG(ELL_INFO, "onLayerClose: delay msg drop, fclose ret=%d, fpath = %s", ret, mFile->getFileName().data());
    }
    return EE_OK;
}


s32 HttpEvtFile::onReqHeadDone(net::HttpMsg* msg) {
    msg->grab();
    mMsg = msg;
    mReqBodyFinish = false;

    net::EHttpMethod cmd = msg->getMethod();
    switch (cmd) {
    case net::HTTP_GET:
    {
        mFile = new HandleFile();
        mFile->setClose(EHT_FILE, HttpEvtFile::funcOnClose, this);
        s32 ret = mFile->open(msg->getRealPath(), 1);
        if (EE_OK != ret) {
            mReqs.mError = ret;
            delete mFile;
            mFile = nullptr;
            return sendRespHead(msg, net::HTTP_STATUS_NOT_FOUND, "open fail", false, true);
        }
        mReqs.mCall = HttpEvtFile::funcOnRead;
        grab();
        break;
    }
    case net::HTTP_PUT:
    case net::HTTP_POST:
    {
        s32 ret = System::createPath(msg->getRealPath());
        if (AppIsPathDelimiter(msg->getRealPath().lastChar())) {
            // just create a path don't req on HandleFile
            if (EE_OK != ret) {
                return sendRespHead(msg, net::HTTP_STATUS_SERVICE_UNAVAILABLE, "create path fail", false, true);
            }
            return sendRespHead(msg, net::HTTP_STATUS_OK, "", false, true);
        }
        mFile = new HandleFile();
        mFile->setClose(EHT_FILE, HttpEvtFile::funcOnClose, this);
        ret = mFile->open(msg->getRealPath(), 2 | 4);
        if (EE_OK != ret) {
            mReqs.mError = ret;
            delete mFile;
            mFile = nullptr;
            return sendRespHead(msg, net::HTTP_STATUS_SERVICE_UNAVAILABLE, "create file fail", false, true);
        }
        mReqs.mCall = HttpEvtFile::funcOnWrite;
        grab();
        break;
    }
    case net::HTTP_DELETE:
    {
        // don't req on HandleFile
        s32 ret = System::removeFile(msg->getRealPath());
        if (EE_OK != ret) {
            return sendRespHead(msg, net::HTTP_STATUS_SERVICE_UNAVAILABLE, "delete fail", false, true);
        }
        return sendRespHead(msg, net::HTTP_STATUS_OK, "", false, true);
    }
    default:
        mMsg->drop();
        mMsg = nullptr;
        return sendRespHead(msg, net::HTTP_STATUS_METHOD_NOT_ALLOWED, "", false, true);
    }


    mOffset = 0;
    mReqs.mError = 0;
    mReqs.mUser = nullptr;
    sendRespHead(msg, net::HTTP_STATUS_OK, "", true, false);
    return mReadOnly ? launchRead() : launchWrite();
}


s32 HttpEvtFile::onReqBody(net::HttpMsg* msg) {
    if (mReadOnly || !mFile || 0 == msg->getBody().size()) {
        return EE_OK; // skip body if
    }
    Packet tmp;
    tmp.swap(msg->getBody());
    mCache.moveBack(std::move(tmp));
    return launchWrite();
}


s32 HttpEvtFile::onReqBodyDone(net::HttpMsg* msg) {
    mReqBodyFinish = true;
    s32 ret = onReqBody(msg);
    // if (mMsg) {
    //     mMsg->drop();
    //     mMsg = nullptr;
    // }
    return ret;
}


s32 HttpEvtFile::sendRespHead(net::HttpMsg* msg, s32 err, const s8* body, bool chunk, bool send) {
    net::HttpMsg* omsg = new net::HttpMsg(msg->getHttpLayer());
    omsg->setEvent(this);

    usz olen = strlen(body);

    omsg->setStatus(err);

    net::Website* site = msg->getHttpLayer()->getWebsite();
    net::HttpHead& hed = omsg->getHead();
    const StringView str = net::HttpMsg::getMimeType(msg->getRealPath().data(), msg->getRealPath().size());
    hed.setContentType(str);

    StringView svv("If-Range", sizeof("If-Range") - 1);
    svv = msg->getHead().get(svv);
    if (svv.mLen > 0) {
        svv.set("Range", sizeof("Range") - 1);
        // hed.add(svv);
    }

    hed.add("Cache-Control", "public, max-age=6000"); // TODO config cache time

    const String& host = site->getConfig().mHost;
    StringView key("Host", 4);
    StringView val(host.data(), host.size());
    hed.add(key, val);

    key.set("Access-Control-Allow-Origin", sizeof("Access-Control-Allow-Origin") - 1);
    val.set("*", 1);
    hed.add(key, val);

    // key.set(DSTRV("Content-Type"));
    // val.set(DSTRV("text/html;charset=utf-8"));
    // hed.add(key, val);
    if (chunk) {
        hed.setChunked();
        if (olen > 0) {
            omsg->writeChunk(body, olen);
        }
    } else {
        hed.setLength(olen);
        omsg->writeBody(body, olen);
    }

    if (send) {
        if (chunk) {
            omsg->writeLastChunk();
        }
        s32 ret = msg->getHttpLayer()->sendOut(omsg);
        omsg->drop();
        return ret;
    }

    mMsgResp = omsg;
    return EE_OK;
}


s32 HttpEvtFile::launchWrite() {
    if (mReqs.mUsed > 0 || mReqs.mUser || !mMsgResp || 0 == mCache.size()) {
        return EE_OK;
    }
    if (0 == mCache.size()) {
        if (mReqBodyFinish && mFile) {
            return mFile->launchClose();
        }
        return EE_OK;
    }
    Packet& pack = (*mCache.begin());
    mReqs.mData = pack.data();
    mReqs.mAllocated = (u32)pack.capacity();
    mReqs.mUsed = (u32)pack.size();
    mReqs.mUser = this;
    mReqs.mError = mFile->write(&mReqs, mOffset);
    if (EE_OK != mReqs.mError) {
        return mFile->launchClose();
    }
    return EE_OK;
}


void HttpEvtFile::onFileWrite(RequestFD* it) {
    if (it->mError) {
        DLOG(ELL_ERROR, "onFileWrite>>err=%d, file=%s", it->mError, mFile->getFileName().data());
        mFile->launchClose();
        return;
    }
    mOffset += it->mUsed;
    it->mUser = nullptr;
    it->mUsed = 0;
    auto nd = mCache.begin();
    mCache.erase(nd);
    launchWrite();
}


void HttpEvtFile::onFileRead(RequestFD* it) {
    if (it->mError || !mMsg) {
        DLOG(ELL_ERROR, "onFileRead: err=%d, file=%s", it->mError, mFile->getFileName().data());
        mFile->launchClose();
        return;
    }

    Packet& pack = mMsgResp->getBody();
    if (it->mUsed > G_BLOCK_HEAD_SIZE) {
        u32 readlen = it->mUsed - G_BLOCK_HEAD_SIZE;
        s8 chunked[8];
        snprintf(chunked, sizeof(chunked), "%04x\r\n", readlen);
        memcpy(pack.data(), chunked, G_BLOCK_HEAD_SIZE);
        pack.resize(it->mUsed);
        pack.write("\r\n", 2);
        mOffset += readlen;
    }
    if (mOffset >= mFile->getFileSize()) {
        mMsgResp->writeLastChunk();
        mFile->launchClose();
        DLOG(ELL_INFO, "onFileRead: finish file=%s, size=%lu", mFile->getFileName().data(), mOffset);
    }
    it->mUser = nullptr;
    it->mUsed = 0;
    if (!mMsgResp || EE_OK != mMsgResp->getHttpLayer()->sendOut(mMsgResp)) {
        DLOG(ELL_ERROR, "onFileRead: post resp err file=%s", mFile->getFileName().data());
        mFile->launchClose();
    }
}


s32 HttpEvtFile::launchRead() {
    if (EE_OK != mReqs.mError || !mFile || !mMsgResp) {
        return mReqs.mError;
    }
    if (mOffset >= mFile->getFileSize()) {
        mMsgResp->writeLastChunk();
        DLOG(ELL_INFO, "onFileRead: finish file=%s", mFile->getFileName().data());
        mFile->launchClose();
        return postResp();
    }
    Packet& pack = mMsgResp->getBody();
    pack.reallocate(4 * 1024);
    mReqs.mData = pack.data();
    mReqs.mAllocated = (u32)pack.capacity() - 2; // @note should be <= 0xFFFF, 2 for tail
    mReqs.mUsed = G_BLOCK_HEAD_SIZE;
    mReqs.mUser = this;
    mReqs.mError = mFile->read(&mReqs, mOffset);
    if (EE_OK != mReqs.mError) {
        return mFile->launchClose();
    }
    return EE_OK;
}


s32 HttpEvtFile::postResp() {
    s32 ret = mMsgResp->getHttpLayer()->sendOut(mMsgResp);
    if (EE_OK != ret) {
        DLOG(ELL_ERROR, "postResp fail, status=%d", mMsgResp->getStatus());
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
    return ret;
}

} // namespace app
