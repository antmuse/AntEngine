#include "HttpsClient.h"
#include "Engine.h"
#include "AppTicker.h"

namespace app {


HttpsClient::HttpsClient() {
    mTCP.setClose(EHT_TCP_CONNECT, HttpsClient::funcOnClose, this);
    mTCP.setTime(HttpsClient::funcOnTime, 15 * 1000, 20 * 1000, -1);
}


HttpsClient::~HttpsClient() {
}


s32 HttpsClient::open(const String& addr) {
    RequestFD* it = RequestFD::newRequest(4 * 1024);
    it->mUser = this;
    it->mCall = HttpsClient::funcOnConnect;
    s32 ret = mTCP.open(addr, it);
    mTCP.setHost("www.baidu.com", sizeof("www.baidu.com") - 1);
    if (EE_OK != ret) {
        RequestFD::delRequest(it);
    }
    return ret;
}


s32 HttpsClient::onTimeout(HandleTime& it) {
    DASSERT(mTCP.getGrabCount() > 0);
    return EE_ERROR;
}


void HttpsClient::onClose(Handle* it) {
    DASSERT(&mTCP == it && "HttpsClient::onClose what handle?");
    //delete this;
    mFile.close();
}


void HttpsClient::onWrite(RequestFD* it) {
    if (0 == it->mError) {
        //printf("HttpsClient::onWrite>>success = %u\n", it->mUsed);
    } else {
        Logger::log(ELL_ERROR, "HttpsClient::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestFD::delRequest(it);
}


void HttpsClient::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        StringView dat = it->getReadBuf();
        printf("%.*s\n", (s32)dat.mLen, dat.mData);
        usz got = mFile.write(dat.mData, dat.mLen);
        it->mUsed = 0;
        if (EE_OK == mTCP.read(it)) {
            return;
        }
    }

    //stop read
    Logger::log(ELL_INFO, "HttpsClient::onRead>>read 0, ecode=%d", it->mError);
    RequestFD::delRequest(it);
}


void HttpsClient::onConnect(RequestFD* it) {
    if (0 == it->mError) {
        s32 ret = mTCP.verify(net::ETLS_VERIFY_CERT_HOST);
        if (EE_OK != ret) {
            Logger::log(ELL_ERROR, "HttpsClient::onConnect>>verify fail, ecode=%d", ret);
        }
        String fnm = "Log/";
        fnm += mTCP.getRemote().getStr();
        s64 len = fnm.findFirst(':');
        fnm.setLen(len > 0 ? len : fnm.getLen());
        fnm += ".html";
        mFile.openFile(fnm, false);
        it->mCall = HttpsClient::funcOnRead;
        if (0 == mTCP.read(it)) {
            RequestFD* get = RequestFD::newRequest(1 * 1024);
            get->mUsed = snprintf(get->mData, get->mAllocated,
                "GET / HTTP/1.1\r\nAccept:*/*\r\nConnection:close\r\n\r\n");
            get->mUser = this;
            get->mCall = HttpsClient::funcOnWrite;
            if (0 != mTCP.write(get)) {
                RequestFD::delRequest(get);
            }
            return;
        }
    }

    Logger::log(ELL_ERROR, "HttpsClient::onConnect>>ecode=%d", it->mError);
    RequestFD::delRequest(it);
}


} //namespace app
