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


#ifndef APP_HTTPLAYER_H
#define	APP_HTTPLAYER_H


#include "EngineConfig.h"
#include "Loop.h"
#include "System.h"
#include "FileReader.h"
#include "Net/HandleTLS.h"
#include "Net/HTTP/HttpRequest.h"
#include "Net/HTTP/HttpResponse.h"

namespace app {
namespace net {


class HttpLayer {
public:
    HttpLayer(http_parser_type tp = HTTP_BOTH);

    ~HttpLayer();

    static void funcOnLink(net::RequestTCP* it) {
        HttpLayer* con = new HttpLayer();
        con->onLink(it); //HttpLayer×Ô»Ù¶ÔÏó
    }

    http_parser_type getType()const {
        return (http_parser_type)(mParser.type);
    }

    HttpResponse& getResp() {
        return mResp;
    }

    HttpRequest& getRequest() {
        return mRequest;
    }

private:

    bool onHttpFinish();

    void onChunkFinish();

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onWrite(net::RequestTCP* it);

    void onRead(net::RequestTCP* it);

    void onLink(net::RequestTCP* it);

    DFINLINE s32 writeIF(net::RequestTCP* it) {
        return mHTTPS ? mTCP.write(it) : mTCP.getHandleTCP().write(it);
    }

    DFINLINE s32 readIF(net::RequestTCP* it) {
        return mHTTPS ? mTCP.read(it) : mTCP.getHandleTCP().read(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        HttpLayer& nd = *(HttpLayer*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(net::RequestTCP* it) {
        HttpLayer& nd = *(HttpLayer*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(net::RequestTCP* it) {
        HttpLayer& nd = *(HttpLayer*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnClose(Handle* it) {
        HttpLayer& nd = *(HttpLayer*)it->getUser();
        nd.onClose(it);
    }


    /**
     * @return 0=OK, 1=F_SKIPBODY, 2=upgrade
    */
    static s32 funcHttpHeadFinish(http_parser* iContext);
    static s32 funcHttpBegin(http_parser* iContext);
    static s32 funcHttpStatus(http_parser* iContext, const s8* at, usz length);
    static s32 funcHttpFinish(http_parser* iContext);
    static s32 funcHttpChunkHead(http_parser* iContext);
    static s32 funcHttpChunkFinish(http_parser* iContext);
    static s32 funcHttpPath(http_parser* iContext, const s8* at, usz length);
    static s32 funcHttpHeadName(http_parser* iContext, const s8* at, usz length);
    static s32 funcHttpHeadValue(http_parser* iContext, const s8* at, usz length);
    static s32 funcHttpBody(http_parser* iContext, const s8* at, usz length);


    void clear();

    bool onReceive(s8* buf, usz sz);
    void show(HttpRequest& req, HttpResponse& resp);
    void initParser(http_parser_type tp);
    void writeRespError(s32 err);
    void writeContentType(const String& path);

    bool mKeepAlive;
    bool mHTTPS;
    FileReader mReadFile;
    usz mReaded;
    EngineConfig::WebsiteCfg* mConfig;
    http_parser_settings mSets;
    http_parser mParser;
    HandleTLS mTCP;
    HttpRequest mRequest;
    HttpResponse mResp;
    TVector<FileInfo> mFiles;
};

}//namespace net
}//namespace app

#endif //APP_HTTPLAYER_H