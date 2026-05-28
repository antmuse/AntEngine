#include "Net/HTTP/HttpEvtFileReader.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"


namespace app {
#define DSTRV(V) V, sizeof(V) - 1

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtFileReader::HttpEvtFileReader() {
}

HttpEvtFileReader::~HttpEvtFileReader() {
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

s32 HttpEvtFileReader::onReadError(net::HttpMsg* msg) {
    return EE_ERROR;
}

s32 HttpEvtFileReader::onRespWriteError(net::HttpMsg* msg) {
    return EE_ERROR;
}

s32 HttpEvtFileReader::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtFileReader::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtFileReader::onRespWrite(net::HttpMsg* msg) {
    if (!mMsgResp) {
        return EE_OK;
    }
    if (mReqs.mError) {
        return EE_ERROR;
    }
    if (mFile->isClosing()) {
        return EE_CLOSING;
    }
    return launchRead();
}


void HttpEvtFileReader::onFileClose(Handle* it) {
    // mReqs.mUser = nullptr;
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


s32 HttpEvtFileReader::onLayerClose(net::HttpMsg* msg) {
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


s32 HttpEvtFileReader::onReqHeadDone(net::HttpMsg* msg) {
    msg->grab();
    mMsg = msg;
    mReqBodyFinish = false;

    net::EHttpMethod cmd = msg->getMethod();
    if (net::HTTP_GET == cmd) {
        mFile = new HandleFile();
        mFile->setClose(EHT_FILE, HttpEvtFileReader::funcOnClose, this);
        s32 ret = mFile->open(msg->getRealPath(), 1);
        if (EE_OK != ret) {
            mReqs.mError = ret;
            delete mFile;
            mFile = nullptr;
            return sendRespHead(msg, net::HTTP_STATUS_NOT_FOUND, "open fail", false, true);
        }
        mReqs.mCall = HttpEvtFileReader::funcOnRead;
        grab();
    } else {
        mMsg->drop();
        mMsg = nullptr;
        return sendRespHead(msg, net::HTTP_STATUS_METHOD_NOT_ALLOWED, "", false, true);
    }

    mOffset = 0;
    mReqs.mError = 0;
    mReqs.mUser = nullptr;
    sendRespHead(msg, net::HTTP_STATUS_OK, "", true, false);
    return launchRead();
}


s32 HttpEvtFileReader::onReqBody(net::HttpMsg* msg) {
    return EE_OK; // skip body if
}


s32 HttpEvtFileReader::onReqBodyDone(net::HttpMsg* msg) {
    mReqBodyFinish = true;
    return EE_OK; // skip body if
}


s32 HttpEvtFileReader::sendRespHead(net::HttpMsg* msg, s32 err, const s8* body, bool chunk, bool send) {
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


void HttpEvtFileReader::onFileRead(RequestFD* it) {
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


s32 HttpEvtFileReader::launchRead() {
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


s32 HttpEvtFileReader::postResp() {
    s32 ret = mMsgResp->getHttpLayer()->sendOut(mMsgResp);
    if (EE_OK != ret) {
        DLOG(ELL_ERROR, "postResp fail, status=%d", mMsgResp->getStatus());
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
    return ret;
}

} // namespace app
