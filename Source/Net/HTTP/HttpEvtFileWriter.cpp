#include "Net/HTTP/HttpEvtFileWriter.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"


namespace app {
#define DSTRV(V) V, sizeof(V) - 1

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtFileWriter::HttpEvtFileWriter() {
}

HttpEvtFileWriter::~HttpEvtFileWriter() {
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

s32 HttpEvtFileWriter::onReadError(net::HttpMsg* msg) {
    return EE_ERROR;
}

s32 HttpEvtFileWriter::onRespWriteError(net::HttpMsg* msg) {
    return EE_ERROR;
}

s32 HttpEvtFileWriter::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtFileWriter::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtFileWriter::onRespWrite(net::HttpMsg* msg) {
    if (!mMsgResp) {
        return EE_OK;
    }
    if (mReqs.mError) {
        return EE_ERROR;
    }
    if (mFile->isClosing()) {
        return EE_CLOSING;
    }
    return launchWrite();
}


void HttpEvtFileWriter::onFileClose(Handle* it) {
    if (mMsg) {
        if (mReqBodyFinish && mCache.empty()) {
            DLOG(ELL_INFO, "onFileWrite>>success up, file=%s", mFile->getFileName().data());
            const s8* resp = R"({"ecode":0,"emsg":"success"})";
            sendRespHead(mMsg, net::HTTP_STATUS_OK, resp, true, false);
        } else {
            s32 ret = System::removeFile(mFile->getFileName());
            DLOG(ELL_INFO, "onFileClose: frontend alive, %s to remove file= %s", EE_OK == ret ? "success" : "fail",
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
        s32 ret = System::removeFile(mFile->getFileName());
        DLOG(ELL_INFO, "onFileClose: frontend closed, %s to remove file= %s", EE_OK == ret ? "success" : "fail",
            mFile->getFileName().data());
    }

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


s32 HttpEvtFileWriter::onLayerClose(net::HttpMsg* msg) {
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


s32 HttpEvtFileWriter::onReqHeadDone(net::HttpMsg* msg) {
    mReqBodyFinish = false;
    mOffset = 0;
    mReqs.mError = 0;
    mReqs.mUser = nullptr;

    net::EHttpMethod cmd = msg->getMethod();
    switch (cmd) {
    case net::HTTP_PUT:
    case net::HTTP_POST:
    {
        s32 ret = System::createPath(msg->getRealPath());
        if (EE_OK != ret) {
            return sendRespHead(msg, net::HTTP_STATUS_METHOD_NOT_ALLOWED, "create_path_fail", false, true);
        }
        if (AppIsPathDelimiter(msg->getRealPath().lastChar())) {
            // just create a path don't req on HandleFile
            return sendRespHead(msg, net::HTTP_STATUS_OK, "", false, true);
        }
        msg->grab();
        mMsg = msg;
        mFile = new HandleFile();
        mFile->setClose(EHT_FILE, HttpEvtFileWriter::funcOnClose, this);
        ret = mFile->open(msg->getRealPath(), 2 | 4);
        if (EE_OK != ret) {
            mReqs.mError = ret;
            delete mFile;
            mFile = nullptr;
            return sendRespHead(msg, net::HTTP_STATUS_SERVICE_UNAVAILABLE, "createf_fail", false, true);
        }
        mReqs.mCall = HttpEvtFileWriter::funcOnWrite;
        grab();
        break;
    }
    case net::HTTP_DELETE:
    {
        s64 cnt = System::removeAll(msg->getRealPath());
        if (cnt < 0) {
            return sendRespHead(msg, net::HTTP_STATUS_NOT_FOUND, "del_fail", false, true);
        }
        DLOG(ELL_CRITICAL, "DeleteAll: %s", msg->getRealPath().data());
        return sendRespHead(msg, net::HTTP_STATUS_OK, "OK", false, true);
    }
    default:
        return sendRespHead(msg, net::HTTP_STATUS_METHOD_NOT_ALLOWED, "del_fail", false, true);
    }

    // sendRespHead(msg, net::HTTP_STATUS_OK, "OK", true, false);
    return EE_OK;
}


s32 HttpEvtFileWriter::onReqBody(net::HttpMsg* msg) {
    if (!mFile || 0 == msg->getBody().size()) {
        return EE_OK; // skip body
    }
    Packet tmp;
    tmp.swap(msg->getBody());
    mCache.moveBack(std::move(tmp));
    return launchWrite();
}


s32 HttpEvtFileWriter::onReqBodyDone(net::HttpMsg* msg) {
    mReqBodyFinish = true;
    return onReqBody(msg);
}


s32 HttpEvtFileWriter::sendRespHead(net::HttpMsg* msg, s32 err, const s8* body, bool chunk, bool send) {
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
    } else {
        mMsgResp = omsg;
    }

    return EE_OK;
}


s32 HttpEvtFileWriter::launchWrite() {
    if (0 == mCache.size() && mReqBodyFinish) {
        return mFile->launchClose(); // success, begin closing...
    }
    if (mReqs.mUser || 0 == mCache.size()) {
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


void HttpEvtFileWriter::onFileWrite(RequestFD* it) {
    if (it->mError) {
        DLOG(ELL_ERROR, "onFileWrite>>err=%d, file=%s", it->mError, mFile->getFileName().data());
        mFile->launchClose();
        return;
    }
    mOffset += it->mUsed;
    it->mUser = nullptr;
    // it->mUsed = 0;
    auto nd = mCache.begin();
    mCache.erase(nd);
    launchWrite();
}


s32 HttpEvtFileWriter::postResp() {
    s32 ret = mMsgResp->getHttpLayer()->sendOut(mMsgResp);
    if (EE_OK != ret) {
        DLOG(ELL_ERROR, "postResp fail, status=%d", mMsgResp->getStatus());
        mMsgResp->drop();
        mMsgResp = nullptr;
    }
    return ret;
}

} // namespace app
