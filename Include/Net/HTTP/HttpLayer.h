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
#include "Net/HTTP/HttpMsg.h"

namespace app {
namespace net {

//Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run faster
#ifndef DHTTP_PARSE_STRICT
# define DHTTP_PARSE_STRICT 1
#endif

//Maximium header size allowed. 
#ifndef HTTP_MAX_HEADER_SIZE
# define HTTP_MAX_HEADER_SIZE (80*1024)
#endif




class Website;

class HttpLayer : public RefCount {
public:
    HttpLayer(EHttpParserType tp = EHTTP_BOTH);

    virtual ~HttpLayer();

    HttpMsg* getMsg() const {
        return mMsg;
    }

    Website* getWebsite() const {
        return mWebsite;
    }

    s32 get(const String& gurl);

    s32 post(const String& gurl);

    EHttpParserType getType()const {
        return mPType;
    }

    const HandleTLS& getHandle()const {
        return mTCP;
    }

    bool onLink(RequestFD* it);

    bool sendReq();
    bool sendResp(HttpMsg* msg);

    /* Executes the parser. Returns number of parsed bytes. Sets
     * `parser->EHttpError` on error. */
    usz parseBuf(const s8* data, usz len);

    static void setMaxHeaderSize(u32 size) {
        GMAX_HEAD_SIZE = size;
    }
    static StringView getMethodStr(EHttpMethod it);

    static const s8* getErrStr(EHttpError it);

    const s8* getErrStr() const {
        return getErrStr(mHttpError);
    }

private:
    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it, HttpMsg* msg);

    void onRead(RequestFD* it);

    void postClose();

    DFINLINE s32 writeIF(RequestFD* it) {
        return mHTTPS ? mTCP.write(it) : mTCP.getHandleTCP().write(it);
    }

    DFINLINE s32 readIF(RequestFD* it) {
        return mHTTPS ? mTCP.read(it) : mTCP.getHandleTCP().read(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        HttpLayer& nd = *(HttpLayer*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        HttpMsg* msg = reinterpret_cast<HttpMsg*>(it->mUser);
        HttpLayer* nd = msg->getHttpLayer();
        DASSERT(msg && nd);
        nd->onWrite(it, msg);
    }

    static void funcOnRead(RequestFD* it) {
        HttpLayer& nd = *(HttpLayer*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(RequestFD* it) {
        HttpLayer& nd = *(HttpLayer*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        HttpLayer& nd = *(HttpLayer*)it->getUser();
        nd.onClose(it);
    }

    void clear();

    bool mHTTPS;
    EHttpParserType mPType;
    Website* mWebsite;
    HandleTLS mTCP;
    HttpMsg* mMsg;

    //parser
private:

    //Checks if this is the final chunk of the body
    bool isBodyFinal()const;

    bool isBodyHeader()const {
        return 0 != (mFlags & F_HEAD_DONE);
    }

    /* If http_should_keep_alive() in the mCallHeadComplete or
     * mCallMsgComplete callback returns 0, then this should be
     * the last message on the connection.
     * If you are the server, respond with the "Connection: close" header.
     * If you are the client, close the connection.
     */
    bool shouldKeepAlive()const;

    /* Pause or un-pause the parser; a nonzero value pauses
     * Users should only be pausing/unpausing a parser that is not in an error
     * state. In non-debug builds, there's not much that we can do about this
     * other than ignore it.
     */
    void pauseParse(bool paused);


    /* Get an EHttpError value from an HttpParser */
    EHttpError getError()const {
        return mHttpError;
    }

    void setError(EHttpError it) {
        mHttpError = it;
    }

    //Does the parser need to see an EOF to find the end of the message?
    bool needEOF()const;


    /** eg:
        Content-Type: multipart/form-data; boundary="---soun"
        out = {"multipart"="form-data","boundary"="---soun"};

        Content-Disposition: form-data; name="field"; filename="filename.jpg"
        out = {"form-data"="","name"="field","filename"="filename.jpg"};
    */
    static const s8* parseValue(const s8* curr, const s8* end,
        StringView* out, s32& omax, s32& vflag);

    u16 mFlags;            // F_* values from 'flags' enum; semi-public
    u8 mIndex;             // index into current matcher
    u8 mState;
    u8 mHeaderState;
    u8 mValueState;

    u32 mType : 2;         // enum EHttpParserType

    //Transfer-Encoding header is present
    u32 mUseTransferEncode : 1;

    //Allow headers with both `Content-Length` and `Transfer-Encoding: chunked`
    u32 mAllowChunkedLen : 1;

    u32 mLenientHeaders : 1;

    /* 1 = Upgrade header was present and the parser has exited because of that.
     * 0 = No upgrade header present.
     * Should be checked when http_parser_execute() returns in addition to
     * error checking.
     */
    u32 mUpgrade : 1;

    //bytes read in various scenarios
    u32 mReadSize;

    /* bytes in body.
     * `(u64) -1` (all bits one) if no Content-Length header.
     */
    u64 mContentLen;

    //READ-ONLY
    u16 mVersionMajor;
    u16 mVersionMinor;
    u16 mStatusCode;     // responses only
    EHttpMethod mMethod; // requests only
    EHttpError mHttpError;

    u8 mBoundaryLen;
    s8 mBoundary[256];

    u8 mFormNameLen;
    s8 mFormName[256];

    u8 mFileNameLen;
    s8 mFileName[256];

    static u32 GMAX_HEAD_SIZE;

    //reset parser
    void reset();

    const s8* parseBoundBody(const s8* curr, const s8* end, StringView& tbody);


    /**
     * @return 0=OK, 1=F_SKIPBODY, 2=upgrade
     */
    s32 headDone();
    void chunkHeadDone();
    void chunkMsg();
    void chunkDone();
    void msgBegin();
    void msgPath();
    void msgEnd();
    void msgBody();
    void msgError();
    void msgStep();
};

}//namespace net
}//namespace app

#endif //APP_HTTPLAYER_H