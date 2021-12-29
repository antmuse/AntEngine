/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#include "Net/HTTP/HttpLayer.h"
#include "Net/Acceptor.h"
#include "Loop.h"
#include "Timer.h"

namespace app {
namespace net {

const static s32 G_CHUNK_SIZE = 2 * 1024;       //0x800
const static s8* G_CHUNK_REFMT = "%.3llx";      //3=strlen(800),   0x800

s32 HttpLayer::funcHttpHeader(HttpParser& pars, const StringView& key, const StringView& val) {
    //printf("%.*s : %.*s", (s32)key.mLen, key.mData,(s32)val.mLen, val.mData);
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;

    if (EHTTP_RESPONSE == pars.mType) {
        nd.getResp().onHeader(key, val);
    } else if (EHTTP_REQUEST == pars.mType) {
        nd.getRequest().onHeader(key, val);
    }
    return 0;
}

s32 HttpLayer::funcHttpBegin(HttpParser& pars) {
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    if (EHTTP_REQUEST == pars.mType) {
        nd.getRequest().setMethod((EHttpMethod)pars.mMethod);
    }
    nd.onHttpStart();
    return 0;
}

s32 HttpLayer::funcHttpStatus(HttpParser& pars, const s8* at, usz length) {
    //printf("funcHttpStatus>>status=%u, str=%.*s\n", pars.status_code, (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    if (EHTTP_RESPONSE == pars.mType) {
        nd.getResp().writeStatus(pars.mStatusCode);
        nd.getResp().writeBrief(at, length);
    } else if (EHTTP_REQUEST == pars.mType) {
        nd.getRequest().setMethod((EHttpMethod)pars.mMethod);
    }
    return 0;
}

s32 HttpLayer::funcHttpHeadFinish(HttpParser& pars) {
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    if (EHTTP_RESPONSE == pars.mType) {
        return nd.getResp().onHeadFinish(pars.mFlags, pars.mContentLen);
    } else {
        return nd.getRequest().onHeadFinish(pars.mFlags, pars.mContentLen);
    }
    return 0;
}

s32 HttpLayer::funcHttpFinish(HttpParser& pars) {
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    nd.onHttpFinish();
    return 0;
}

s32 HttpLayer::funcHttpPath(HttpParser& pars, const s8* at, usz length) {
    //printf("URL=%.*s\n", (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    HttpRequest& req = nd.getRequest();
    req.getURL().decode(at, length);
    req.setMethod((EHttpMethod)pars.mMethod);
    return 0;
}

s32 HttpLayer::funcHttpBody(HttpParser& pars, const s8* at, usz length) {
    //printf("%.*s\n", (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    if (EHTTP_RESPONSE == pars.mType) {
        nd.getResp().writeBody(at, length);
    } else if (EHTTP_REQUEST == pars.mType) {
        nd.getRequest().writeBody(at, length);
    }
    nd.onHttpBody();
    return 0;
}

//Transfer-Encoding : chunked
s32 HttpLayer::funcHttpChunkHead(HttpParser& pars) {
    //printf("funcHttpChunkHead>>chunk size = %llu = 0X%llx\n", pars.mContentLen, pars.mContentLen);
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    if (EHTTP_RESPONSE == pars.mType) {
        //nd.getResp().onChunkHead(pars.mContentLen);
    } else if (EHTTP_REQUEST == pars.mType) {
        //nd.getRequest().onChunkHead(pars.mContentLen);
    }
    return 0;
}

s32 HttpLayer::funcHttpChunkFinish(HttpParser& pars) {
    HttpLayer& nd = *(HttpLayer*)pars.mUserData;
    nd.onChunkFinish();
    return 0;
}





HttpLayer::HttpLayer(EHttpParserType tp) :
    mPType(tp),
    mEvent(nullptr),
    mHTTPS(true),
    mKeepAlive(false) {
    initParser(tp);
    mTCP.setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
    mTCP.setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
}


HttpLayer::~HttpLayer() {
    //onClose();
}


void HttpLayer::initParser(EHttpParserType tp) {
    mParser.init(tp, this);

    mParser.mCallMsgBegin = HttpLayer::funcHttpBegin;
    mParser.mCallURL = HttpLayer::funcHttpPath;
    mParser.mCallStatus = HttpLayer::funcHttpStatus;
    mParser.mCallHeader = HttpLayer::funcHttpHeader;
    mParser.mCallHeadComplete = HttpLayer::funcHttpHeadFinish;
    mParser.mCallBody = HttpLayer::funcHttpBody;
    mParser.mCallMsgComplete = HttpLayer::funcHttpFinish;
    mParser.mCallChunkHeader = HttpLayer::funcHttpChunkHead;
    mParser.mCallChunkComplete = HttpLayer::funcHttpChunkFinish;
}



s32 HttpLayer::get(const String& gurl) {
    mResp.clear();
    if (EE_OK != mRequest.writeGet(gurl)) {
        return EE_ERROR;
    }

    mHTTPS = mRequest.getURL().isHttps();
    if (!mHTTPS) {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.getHandleTCP().setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    } else {
        mTCP.setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    }

    StringView host = mRequest.getURL().getHost();
    NetAddress addr(mRequest.getURL().getPort());
    s8 shost[256];  //255, Maximum host name defined in RFC 1035
    snprintf(shost, sizeof(shost), "%.*s", (s32)(host.mLen), host.mData);
    addr.setDomain(shost);

    net::RequestTCP* nd = net::RequestTCP::newRequest(4 * 1024);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnConnect;
    s32 ret = mHTTPS ? mTCP.open(addr, nd) : mTCP.getHandleTCP().open(addr, nd);
    if (EE_OK != ret) {
        net::RequestTCP::delRequest(nd);
        return ret;
    }
    return EE_OK;
}

s32 HttpLayer::post(const String& gurl) {
    return EE_ERROR;
}

void HttpLayer::onChunkFinish() {
    onHttpBody();
}

void HttpLayer::writeContentType(const String& pat) {
    const StringView str = mResp.getMimeType(pat.c_str(), pat.getLen());
    mResp.writeHead("Content-Type", str.mData);

    StringView svv("If-Range", sizeof("If-Range") - 1);
    svv = mRequest.getHead().get(svv);
    if (svv.mLen > 0) {

    }

    svv.set("Range", sizeof("Range") - 1);
    svv = mRequest.getHead().get(svv);
    if (svv.mLen > 0) {

    }
}


bool HttpLayer::onHttpBody() {
    s32 ret = EE_OK;
    if (mEvent) {
        if (EHTTP_REQUEST == mParser.mType) {
            ret = mEvent->onBodyPart(mRequest);
        } else {
            ret = mEvent->onBodyPart(mResp);
        }
    }
    return 0 == ret;
}

bool HttpLayer::onHttpStart() {
    s32 ret = EE_OK;
    if (mEvent) {
        if (EHTTP_REQUEST == mParser.mType) {
            ret = mEvent->onOpen(mRequest);
        } else {
            ret = mEvent->onOpen(mResp);
        }
    }
    return EE_OK == ret;
}

bool HttpLayer::onHttpFinish() {
    if (mEvent) {
        s32 ret;
        if (EHTTP_REQUEST == mParser.mType) {
            if (HTTP_OPTIONS == mRequest.getMethod()) { //跨域
                mResp.writeStatus(HTTP_STATUS_NO_CONTENT, "No Content");
                mResp.writeHead("Server", "AntEngine");

                mKeepAlive = mRequest.isKeepAlive();
                mResp.setKeepAlive(mKeepAlive);
                //mResp.writeHead("Connection", "keep-alive", 0);

                s8 gmtime[32];
                Timer::getTimeStr(gmtime, sizeof(gmtime), "%a, %d %b %Y %H:%M:%S GMT");
                mResp.writeHead("Date", gmtime);

                mResp.writeHead("Access-Control-Allow-Origin", "*");
                mResp.writeHead("Access-Control-Allow-Credentials", "true");
                mResp.writeHead("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                mResp.writeHead("Access-Control-Allow-Headers", "Content-Type,Content-Length,Content-Range");
                mResp.writeHead("Access-Control-Expose-Headers", "Content-Type,Content-Length,Content-Range");
                mResp.writeHead("Access-Control-Max-Age", "1728000");
                mResp.writeHead("Content-Type", "text/plain; charset=utf-8");
                mResp.writeHead("Content-Length", "0");
                mResp.writeHeadFinish();

                mRequest.clear();
                sendResp();
                return true;
            }
            ret = mEvent->onFinish(mRequest);
        } else {
            ret = mEvent->onFinish(mResp);
        }
        if (EE_OK != ret) {
            mHTTPS ? mTCP.launchClose() : mTCP.getHandleTCP().launchClose();
        }
        return EE_OK;
    }
    mHTTPS ? mTCP.launchClose() : mTCP.getHandleTCP().launchClose();
    return EE_OK;

    mReaded = 0;
    mResp.clear();

    mResp.writeStatus(HTTP_STATUS_OK);
    StringView svv = mRequest.getURL().getPath();
    svv.mLen = HttpURL::decodeURL(svv.mData, svv.mLen);
    //svv.toLower();
    String pat(mConfig->mRootPath.getLen() + 64);
    pat = mConfig->mRootPath;
    if (svv.mLen < 2) {
        pat += "/index.html";
    } else {
        pat += svv;
    }
    mResp.writeHead("Host", mTCP.getLocal().getStr());
    mKeepAlive = mRequest.isKeepAlive();
    mResp.setKeepAlive(mKeepAlive);

    s32 tp = System::isExist(pat);
    if (0 == tp) {
        if (mReadFile.openFile(pat)) {
            mResp.writeHead("Access-Control-Allow-Origin", "*");
            writeContentType(pat);
        } else {
            writeRespError(HTTP_STATUS_NOT_FOUND);
        }
    } else if (1 == tp) {
        mResp.writeHead("Content-Type", "text/html;charset=utf-8");
    }
    writeRespError(HTTP_STATUS_NOT_FOUND);
    sendResp();
    return true;
}

bool HttpLayer::sendReq() {
    RingBuffer& bufs = mRequest.getCache();
    if (bufs.getSize() > 0) {
        net::RequestTCP* nd = net::RequestTCP::newRequest(0);
        nd->mUser = this;
        nd->mCall = HttpLayer::funcOnWrite;
        StringView msg = bufs.peekHead();
        nd->mData = msg.mData;
        nd->mAllocated = (u32)msg.mLen;
        nd->mUsed = (u32)msg.mLen;
        s32 ret = writeIF(nd);
        if (0 != ret) {
            net::RequestTCP::delRequest(nd);
            return false;
        }
    }
    return true;
}

bool HttpLayer::sendResp() {
    RingBuffer& bufs = mResp.getCache();
    net::RequestTCP* nd = net::RequestTCP::newRequest(0);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnWrite;
    StringView msg = bufs.peekHead();
    nd->mData = msg.mData;
    nd->mAllocated = (u32)msg.mLen;
    nd->mUsed = (u32)msg.mLen;
    s32 ret = writeIF(nd);
    if (0 != ret) {
        net::RequestTCP::delRequest(nd);
        return false;
    }
    return true;
}


void HttpLayer::writeRespError(s32 ecode) {
    mKeepAlive = false;
    mResp.writeStatus(ecode);
    String pat = mConfig->mRootPath;
    pat += "/err/";
    pat += ecode;
    pat += ".html";
    mResp.writeHead("Content-Type", "text/html;charset=utf-8");
    if (mReadFile.openFile(pat)) {
        mResp.writeLength(mReadFile.getFileSize());
        mResp.writeHeadFinish();
        usz fsz = mReadFile.getFileSize();
        while (fsz > 0) {
            s8* buf;
            s32 bsz = mResp.getCache().peekTailNode(&buf, fsz);
            bsz = mReadFile.read(buf, bsz);
            mResp.getCache().commitTailPos(bsz);
            fsz -= bsz;
        }
        mReadFile.close();
    } else {
        mResp.writeStatus(500);
        mResp.writeBody(pat.c_str() + mConfig->mRootPath.getLen(),
            pat.getLen() - mConfig->mRootPath.getLen());
    }
}


#ifdef DDEBUG
s32 TestHttpReceive(HttpParser& mParser) {
    usz tlen;
    usz used;

    /*const s8* str0 =
        "POST /fs/upload HTTP/1.1\r\n"
        "Con";

    tlen = strlen(str0);
    used = mParser.parseBuf(str0, tlen);

    const s8* str1 = "Content-Length: 8";
    tlen = strlen(str1);
    used = mParser.parseBuf(str1, tlen);
    const s8* str2 = "Content-Length: 8\r\n";

    tlen = strlen(str2);
    used = mParser.parseBuf(str2, tlen);
    const s8* str3 =
        "Connection: Keep-Alive\r\n"
        "\r\n"
        "abcd";

    tlen = strlen(str3);
    used = mParser.parseBuf(str3, tlen);
    const s8* str4 = "len5";

    tlen = strlen(str4);
    used = mParser.parseBuf(str4, tlen);*/

    const s8* test =
        "POST /fs/upload HTTP/1.1\r\n"
        "Content-Length: 18\r\n"
        "Content-Type: multipart/form-data; boundary=vksoun\r\n"
        "Connection: Keep-Alive\r\n"
        "Cookie: Mailbox=yyyy@qq.com\r\n"
        "\r\n"
        "--vksoun\r\n" //boundary
        "Content-Disposition: form-data; name=\"fieldName\"; filename=\"filename.txt\"\r\n"
        "\r\n"
        "msgPart1"
        "\r\n"
        "--vksoun\r\n" //boundary
        "Content-Type: text/plain\r\n"
        "\r\n"
        "--vksoun-"
        "\r\n\r\n"
        "--vksoun--"  //boundary tail
        ;
    tlen = strlen(test);
    used = mParser.parseBuf(test, tlen);
    const s8* laststr = test + used;
    if (0 == mParser.getError()) {
        used++;
    }
    return 0;
}
#endif



s32 HttpLayer::onTimeout(HandleTime& it) {
    return mKeepAlive ? EE_OK : EE_ERROR;
}


void HttpLayer::onClose(Handle* it) {
    if (mEvent) {
        mEvent->onClose();
        mEvent = nullptr;
    }
    if (mHTTPS) {
        DASSERT((&mTCP == it) && "HttpLayer::onClose https handle?");
    } else {
        DASSERT((&mTCP.getHandleTCP() == it) && "HttpLayer::onClose http handle?");
    }
    delete this;
}


void HttpLayer::onLink(net::RequestTCP* it) {
    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    net::RequestAccept& req = *(net::RequestAccept*)it;
    mConfig = reinterpret_cast<EngineConfig::WebsiteCfg*>(accp->getUser());
    mHTTPS = 1 == mConfig->mType;
    if (!mHTTPS) {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.getHandleTCP().setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    }
    net::RequestTCP* nd = net::RequestTCP::newRequest(4 * 1024);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnRead;
    s32 ret = mHTTPS ? mTCP.open(req, nd) : mTCP.getHandleTCP().open(req, nd);
    if (0 == ret) {
        Logger::log(ELL_INFO, "HttpLayer::onLink>> [%s->%s]", mTCP.getRemote().getStr(), mTCP.getLocal().getStr());
    } else {
        net::RequestTCP::delRequest(nd);
        Logger::log(ELL_INFO, "HttpLayer::onLink>> [%s->%s], ecode=%d", mTCP.getRemote().getStr(), mTCP.getLocal().getStr(), ret);
        delete this;
    }
}


void HttpLayer::clear() {
    mReadFile.close();
    mRequest.clear();
    mFiles.clear();
    mReaded = 0;
}


void HttpLayer::onConnect(net::RequestTCP* it) {
    if (0 == it->mError && sendReq()) {
        it->mCall = HttpLayer::funcOnRead;
        if (EE_OK == readIF(it)) {
            return;
        }
    }
    net::RequestTCP::delRequest(it);
}

void HttpLayer::onWrite(net::RequestTCP* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "HttpLayer::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
        clear();
        return;
    }
    clear();
    net::RequestTCP::delRequest(it);
}


void HttpLayer::onRead(net::RequestTCP* it) {
    const s8* dat = it->getBuf();
    ssz datsz = it->mUsed;
    if (0 == datsz) {
        mParser.parseBuf(dat, 0); //make the http-msg-finish callback
    } else {
        ssz parsed = 0;
        ssz stepsz;
        while (datsz > 0 && 0 == mParser.getError()) {
            stepsz = mParser.parseBuf(dat + parsed, datsz);
            parsed += stepsz;
            if (stepsz < datsz) {
                break; //leftover
            }
            datsz -= stepsz;
        }
        it->clearData((u32)parsed);

        if (0 == it->getWriteSize()) {
            //可能受到超长header攻击或其它错误
            Logger::logError("HttpLayer::onRead>>remote=%p, msg overflow", mTCP.getRemote().getStr());
            mTCP.launchClose();
        } else if (0 == mParser.getError()) {
            if (EE_OK == readIF(it)) {
                return;
            }
        }
    }

    //stop mReadFile
    printf("HttpLayer::onRead>>read 0 bytes, ecode=%d\n", it->mError);
    net::RequestTCP::delRequest(it);
}

} //namespace net
}//namespace
