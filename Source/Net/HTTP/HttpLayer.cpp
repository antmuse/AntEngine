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

s32 HttpLayer::funcHttpBegin(http_parser* iContext) {
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    HttpRequest& req = nd.getRequest();
    req.setMethod((http_method)iContext->method);
    return 0;
}

s32 HttpLayer::funcHttpStatus(http_parser* iContext, const s8* at, usz length) {
    //printf("funcHttpStatus>>status=%u, str=%.*s\n", iContext->status_code, (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    HttpResponse& resp = nd.getResp();
    resp.writeStatus(iContext->status_code);
    return 0;
}

s32 HttpLayer::funcHttpHeadFinish(http_parser* iContext) {
    HttpLayer& nd = *(HttpLayer*)iContext->data;

    if (HTTP_RESPONSE == iContext->type) {
        nd.getResp().writeHead(nullptr, nullptr, true);
        return nd.getResp().onHeadFinish(iContext->flags, iContext->content_length);
    } else {
        return nd.getRequest().onHeadFinish(iContext->flags, iContext->content_length);
    }
    return 0;
}

s32 HttpLayer::funcHttpFinish(http_parser* iContext) {
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    nd.onHttpFinish();
    return 0;
}

s32 HttpLayer::funcHttpPath(http_parser* iContext, const s8* at, usz length) {
    //printf("URL=%.*s\n", (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    HttpRequest& req = nd.getRequest();
    req.getURL().decode(at, length);
    req.setMethod((http_method)iContext->method);
    return 0;
}

s32 HttpLayer::funcHttpHeadName(http_parser* iContext, const s8* at, usz length) {
    //printf("%.*s : ", (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)iContext->data;

    if (HTTP_RESPONSE == iContext->type) {
        nd.getResp().onHeadName(at, length);
    } else if (HTTP_REQUEST == iContext->type) {
        nd.getRequest().onHeadName(at, length);
    }
    return 0;
}

s32 HttpLayer::funcHttpHeadValue(http_parser* iContext, const s8* at, usz length) {
    //printf("%.*s\n", (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    if (HTTP_RESPONSE == iContext->type) {
        nd.getResp().onHeadValue(at, length);
    } else {
        nd.getRequest().onHeadValue(at, length);
    }
    return 0;
}

s32 HttpLayer::funcHttpBody(http_parser* iContext, const s8* at, usz length) {
    //printf("%.*s\n", (s32)length, at);
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    if (HTTP_RESPONSE == iContext->type) {
        nd.getResp().writeBody(at, length);
    } else if (HTTP_REQUEST == iContext->type) {
        nd.getRequest().writeBody(at, length);
    }
    return 0;
}

//Transfer-Encoding : chunked
s32 HttpLayer::funcHttpChunkHead(http_parser* iContext) {
    //printf("funcHttpChunkHead>>chunk size = %llu = 0X%llx\n", iContext->content_length, iContext->content_length);
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    if (HTTP_RESPONSE == iContext->type) {
        nd.getResp().onChunkHead(iContext->content_length);
    } else if (HTTP_REQUEST == iContext->type) {
        nd.getRequest().onChunkHead(iContext->content_length);
    }
    return 0;
}

s32 HttpLayer::funcHttpChunkFinish(http_parser* iContext) {
    HttpLayer& nd = *(HttpLayer*)iContext->data;
    nd.onChunkFinish();
    return 0;
}





HttpLayer::HttpLayer(http_parser_type tp) :
    mHTTPS(true),
    mKeepAlive(false) {
    initParser(tp);
    mTCP.setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
    mTCP.setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
}


HttpLayer::~HttpLayer() {
    //onClose();
}


void HttpLayer::initParser(http_parser_type tp) {
    http_parser_settings_init(&mSets);
    mSets.on_message_begin = HttpLayer::funcHttpBegin;
    mSets.on_url = HttpLayer::funcHttpPath;
    mSets.on_status = HttpLayer::funcHttpStatus;
    mSets.on_header_field = HttpLayer::funcHttpHeadName;
    mSets.on_header_value = HttpLayer::funcHttpHeadValue;
    mSets.on_headers_complete = HttpLayer::funcHttpHeadFinish;
    mSets.on_body = HttpLayer::funcHttpBody;
    mSets.on_message_complete = HttpLayer::funcHttpFinish;
    mSets.on_chunk_header = HttpLayer::funcHttpChunkHead;
    mSets.on_chunk_complete = HttpLayer::funcHttpChunkFinish;

    http_parser_init(&mParser, tp);
    mParser.data = this;
}


//s8 shost[256];//255, Maximum host name defined in RFC 1035
//snprintf(shost, sizeof(shost), "%.*s", (s32)(host.mLen), host.mData);


void HttpLayer::onChunkFinish() {
    mResp.clearBody();
    //printf("HttpLayer::onChunkFinish>>chunk size = %llu = 0X%llx\n", bsz, bsz);
}

void HttpLayer::writeContentType(const String& pat) {
    if ('/' == pat.lastChar() || pat.isFileExtension("html")) {
        mResp.writeHead("Content-Type", "text/html;charset=utf-8", 0);
    } else if (pat.isFileExtension("ico")) {
        mResp.writeHead("Content-Type", "image/x-icon;charset=utf-8", 0);
    } else {
        mResp.writeHead("Content-Type", "application/octet-stream;charset=utf-8", 0);
        StringView svv("If-Range", sizeof("If-Range") - 1);
        svv = mRequest.getHead().get(svv);
        if (svv.mLen > 0) {

        }

        svv.set("Range", sizeof("Range") - 1);
        svv = mRequest.getHead().get(svv);
        if (svv.mLen > 0) {

        }
    }
}

bool HttpLayer::onHttpFinish() {
    http_parser_init(&mParser, (http_parser_type)mParser.type);
    mParser.data = this;
    mReaded = 0;
    mResp.clear();

    if (HTTP_OPTIONS == mRequest.getMethod()) { //跨域
        mResp.writeStatus(HTTP_STATUS_NO_CONTENT, "No Content");
        mResp.writeHead("Server", "AntEngine", 0);

        mKeepAlive = mRequest.isKeepAlive();
        mResp.setKeepAlive(mKeepAlive);
        //mResp.writeHead("Connection", "keep-alive", 0);

        s8 gmtime[32];
        Timer::getTimeStr(gmtime, sizeof(gmtime), "%a, %d %b %Y %H:%M:%S GMT");
        mResp.writeHead("Date", gmtime, 0);

        mResp.writeHead("Access-Control-Allow-Origin", "*", 0);
        mResp.writeHead("Access-Control-Allow-Credentials", "true", 0);
        mResp.writeHead("Access-Control-Allow-Methods", "GET, POST, OPTIONS", 0);
        mResp.writeHead("Access-Control-Allow-Headers", "Content-Type,Content-Length,Content-Range", 0);
        mResp.writeHead("Access-Control-Expose-Headers", "Content-Type,Content-Length,Content-Range", 0);
        mResp.writeHead("Access-Control-Max-Age", "1728000", 0);
        mResp.writeHead("Content-Type", "text/plain; charset=utf-8", 0);
        mResp.writeHead("Content-Length", "0", 2);
        sendResp();
        return true;
    }


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
    writeContentType(pat);
    mResp.writeHead("Host", mTCP.getLocal().getStr(), 0);
    mKeepAlive = mRequest.isKeepAlive();
    mResp.setKeepAlive(mKeepAlive);

    s32 tp = System::isExist(pat);
    if (0 == tp) {
        if (mReadFile.openFile(pat)) {
            if (mReadFile.getFileSize() > G_CHUNK_SIZE) {
                mResp.writeHead("Accept-Ranges", "bytes", 0);
                mResp.writeContentRange(mReadFile.getFileSize(), mReaded, G_CHUNK_SIZE - 1, 0);
                mResp.setChunked(2);
                usz usd = mResp.getCache().size();
                mResp.getCache().reallocate(usd + G_CHUNK_SIZE + 32);
                s8* buf = mResp.getCache().getPointer() + usd;
                usz cksz = snprintf(buf, 32, "%x\r\n", G_CHUNK_SIZE);
                usz rds = mReadFile.read(buf + cksz, G_CHUNK_SIZE);
                if (G_CHUNK_SIZE == rds) {
                    mReaded = rds;
                    mResp.getCache().resize(usd + cksz + rds);
                    mResp.getCache().writeU16(App2Char2S16("\r\n"));
                } else {
                    writeRespError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                    mReadFile.close();
                }
            } else {
                mResp.writeContentRange(mReadFile.getFileSize(), mReaded, mReadFile.getFileSize() - 1, 0);
                mResp.writeLength(mReadFile.getFileSize(), 2);
                usz usd = mResp.getCache().size();
                mResp.getCache().resize(usd + mReadFile.getFileSize());
                s8* buf = mResp.getCache().getPointer() + usd;
                if (mReadFile.read(buf, mReadFile.getFileSize()) != mReadFile.getFileSize()) {
                    writeRespError(HTTP_STATUS_NOT_FOUND);
                }
                mReadFile.close();
            }
        } else {
            writeRespError(HTTP_STATUS_NOT_FOUND);
        }
    } else if (1 == tp) {
        System::getPathNodes(pat, mConfig->mRootPath.getLen(), mFiles);
        pat = mConfig->mRootPath;
        pat += "/files.html";
        if (mReadFile.openFile(pat)) {
            mResp.setChunked(2);
            usz usd = mResp.getCache().size();
            mResp.getCache().reallocate(usd + 32 + AppMax((s64)G_CHUNK_SIZE, mReadFile.getFileSize()));
            s8* buf = mResp.getCache().getPointer() + usd;
            usz cksz = snprintf(buf, 32, "%llx\r\n", mReadFile.getFileSize());
            usz rds = mReadFile.read(buf + cksz, mReadFile.getFileSize());
            if (mReadFile.getFileSize() == rds) {
                mResp.getCache().resize(usd + cksz + rds);
                mResp.getCache().writeU16(App2Char2S16("\r\n"));
            } else {
                writeRespError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
            mReadFile.close();
        } else {
            writeRespError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
    } else {
        writeRespError(HTTP_STATUS_NOT_FOUND);
    }

    sendResp();
    //show(mRequest, mResp);
    return true;
}

void HttpLayer::sendResp() {
    net::RequestTCP* nd = net::RequestTCP::newRequest(0);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnWrite;
    nd->mData = mResp.getCache().getPointer();
    nd->mUsed = (u32)mResp.getCache().size();

    s32 ret = writeIF(nd);
    if (0 != ret) {
        net::RequestTCP::delRequest(nd);
    }
}


void HttpLayer::writeRespError(s32 ecode) {
    mKeepAlive = false;
    mResp.writeStatus(ecode);
    String pat = mConfig->mRootPath;
    pat += "/err/";
    pat += ecode;
    pat += ".html";
    mResp.writeHead("Content-Type", "text/html;charset=utf-8", 0);
    if (mReadFile.openFile(pat)) {
        mResp.writeLength(mReadFile.getFileSize(), 2);
        usz usd = mResp.getCache().size();
        mResp.getCache().resize(usd + mReadFile.getFileSize());
        s8* buf = mResp.getCache().getPointer() + usd;
        mReadFile.read(buf, mReadFile.getFileSize());
        mReadFile.close();
    } else {
        mResp.writeStatus(500);
        mResp.writeBody(pat.c_str() + mConfig->mRootPath.getLen(),
            pat.getLen() - mConfig->mRootPath.getLen());
    }
}


bool HttpLayer::onReceive(s8* buf, usz sz) {
    usz parsed = 0;
    while (sz > 0 && 0 == mParser.http_errno) {
        parsed += http_parser_execute(&mParser, &mSets, buf + parsed, sz);
        sz -= parsed;
    }
    return 0 == mParser.http_errno;
}


void HttpLayer::show(HttpRequest& req, HttpResponse& resp) {
    usz max = req.getCache().size();
    const s8* start = req.getCache().getPointer();
    for (usz i = 0; i < max; i += 1024) {
        if (max - i > 1024) {
            printf("%.*s", 1024, start + i);
        } else {
            printf("%.*s\n\n", (s32)(max - i), start + i);
            break;
        }
    }

    max = resp.getCache().size();
    start = resp.getCache().getPointer();
    for (usz i = 0; i < max; i += 1024) {
        if (max - i > 1024) {
            printf("%.*s", 1024, start + i);
        } else {
            printf("%.*s\n\n", (s32)(max - i), start + i);
            break;
        }
    }
}


s32 HttpLayer::onTimeout(HandleTime& it) {
    return mKeepAlive ? EE_OK : EE_ERROR;
}


void HttpLayer::onClose(Handle* it) {
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


void HttpLayer::onWrite(net::RequestTCP* it) {
    if (0 != it->mError) {
        Logger::log(ELL_ERROR, "HttpLayer::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
        clear();
        return;
    }

    if (mReadFile.isOpen()) {
        DASSERT(mResp.isChunked());
        mResp.getCache().resize(0);
        s8* buf = mResp.getCache().getPointer();
        usz cksz = snprintf(buf, 32, "%x\r\n", G_CHUNK_SIZE);
        DASSERT(mReadFile.getPos() == mReaded);
        usz rds = mReadFile.read(buf + cksz, G_CHUNK_SIZE);
        mReaded += rds;
        if (rds > 0) {
            if (rds < G_CHUNK_SIZE) {
                usz rsz = snprintf(buf, 32, G_CHUNK_REFMT, rds);
                buf[rsz] = '\r';
            }
            mResp.getCache().resize(cksz + rds);
            mResp.getCache().writeU16(App2Char2S16("\r\n"));
        }
        if (mReaded == mReadFile.getFileSize()) {
            mResp.getCache().write("0\r\n\r\n", 5);
            mReadFile.close();
        }

        it->mData = mResp.getCache().getPointer();
        it->mUsed = (u32)mResp.getCache().size();
        it->setStepSize(0);
        s32 ret = writeIF(it);
        if (0 == ret) {
            return;
        }
    } else if (mFiles.size() > 0) {
        if (mReaded < mFiles.size()) {
            s8* buf = mResp.getCache().getPointer();
            usz cksz = snprintf(buf, 32, "%x\r\n", G_CHUNK_SIZE);
            mResp.getCache().resize(cksz);
            for (; mReaded < mFiles.size(); ++mReaded) {
                usz leftover = G_CHUNK_SIZE - mResp.getCache().size();
                usz uss = snprintf(mResp.getCache().getWritePointer(),
                    leftover,
                    "<tr><td>%s</td><td><a href=\"%s\">%s</a></td></tr>\r\n",
                    1 & mFiles[mReaded].mFlag ? u8"文件夹" : u8"文件",
                    mFiles[mReaded].mFileName,
                    mFiles[mReaded].mFileName
                    );
                if (uss <= leftover) {
                    mResp.getCache().resize(mResp.getCache().size() + uss);
                } else {
                    break;
                }
            }
            cksz = snprintf(buf, cksz, G_CHUNK_REFMT, mResp.getCache().size() - cksz);
            buf[cksz] = '\r';
            mResp.getCache().writeU16(App2Char2S16("\r\n"));
            it->mData = mResp.getCache().getPointer();
            it->mUsed = (u32)mResp.getCache().size();
            it->setStepSize(0);
            s32 ret = writeIF(it);
            if (0 == ret) {
                return;
            }
        } else if (mReaded == mFiles.size()) {
            ++mReaded;
            mFiles.clear();
            s8* buf = mResp.getCache().getPointer();
            usz cksz = snprintf(buf, 32, "%llx\r\n", sizeof("</TABLE></DIV></UL><hr></BODY></HTML>") - 1);
            cksz += snprintf(buf + cksz, G_CHUNK_SIZE, "%s\r\n0\r\n\r\n",
                "</TABLE></DIV></UL><hr></BODY></HTML>");
            mResp.getCache().resize(cksz);
            it->mData = mResp.getCache().getPointer();
            it->mUsed = (u32)mResp.getCache().size();
            it->setStepSize(0);
            s32 ret = writeIF(it);
            if (0 == ret) {
                return;
            }
        }
    }

    clear();
    net::RequestTCP::delRequest(it);
}


void HttpLayer::onRead(net::RequestTCP* it) {
    if (it->mUsed > 0) {
        StringView dat = it->getReadBuf();
        if (onReceive(dat.mData, dat.mLen)) {
            it->mUsed = 0;
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
