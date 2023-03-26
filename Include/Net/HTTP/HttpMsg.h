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


#ifndef APP_HTTPMSG_H
#define	APP_HTTPMSG_H

#include "RefCount.h"
#include "RingBuffer.h"
#include "Net/HTTP/HttpURL.h"
#include "Net/HTTP/HttpHead.h"

namespace app {
namespace net {

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad RequestFD)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 RequestFD Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected RequestFD)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, RequestFD Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

enum EHttpStatus {
#define XX(num, name, string) HTTP_STATUS_##name = num,
    HTTP_STATUS_MAP(XX)
#undef XX
};


/* RequestFD Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

enum EHttpMethod {
#define XX(num, name, string) HTTP_##name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
};



/* Map for errno-related constants
 *
 * The provided argument should be a macro that takes 2 arguments.
 */
#define HTTP_ERRNO_MAP(XX)                                           \
  /* No error */                                                     \
  XX(OK, "success")                                                  \
                                                                     \
  /* Callback-related errors */                                      \
  XX(CB_MsgBegin, "the CallMsgBegin callback failed")                \
  XX(CB_URL, "the CallURL callback failed")                          \
  XX(CB_HeaderField, "the CallHeaderField callback failed")          \
  XX(CB_HeaderValue, "the CallHeaderValue callback failed")          \
  XX(CB_HeadersComplete, "the CallHeadComplete callback failed")     \
  XX(CB_Body, "the CallBody callback failed")                        \
  XX(CB_MsgComplete, "the CallMsgComplete callback failed")          \
  XX(CB_Status, "the CallURL callback failed")                       \
  XX(CB_ChunkHeader, "the CallChunkHeader callback failed")          \
  XX(CB_ChunkComplete, "the CallChunkComplete callback failed")      \
                                                                     \
  /* Parsing-related errors */                                       \
  XX(INVALID_EOF_STATE, "stream ended at an unexpected time")        \
  XX(HEADER_OVERFLOW,                                                \
     "too many header bytes seen; overflow detected")                \
  XX(CLOSED_CONNECTION,                                              \
     "data received after completed connection: close message")      \
  XX(INVALID_VERSION, "invalid HTTP version")                        \
  XX(INVALID_STATUS, "invalid HTTP status code")                     \
  XX(INVALID_METHOD, "invalid HTTP method")                          \
  XX(INVALID_URL, "invalid URL")                                     \
  XX(INVALID_HOST, "invalid host")                                   \
  XX(INVALID_PORT, "invalid port")                                   \
  XX(INVALID_PATH, "invalid path")                                   \
  XX(INVALID_QUERY_STRING, "invalid query string")                   \
  XX(INVALID_FRAGMENT, "invalid fragment")                           \
  XX(LF_EXPECTED, "LF character expected")                           \
  XX(INVALID_HEADER_TOKEN, "invalid character in header")            \
  XX(INVALID_CONTENT_LENGTH,                                         \
     "invalid character in content-length header")                   \
  XX(UNEXPECTED_CONTENT_LENGTH,                                      \
     "unexpected content-length header")                             \
  XX(INVALID_CHUNK_SIZE,                                             \
     "invalid character in chunk size header")                       \
  XX(INVALID_CONSTANT, "invalid constant string")                    \
  XX(INVALID_INTERNAL_STATE, "encountered unexpected internal state")\
  XX(STRICT, "strict mode assertion failed")                         \
  XX(PAUSED, "parser is paused")                                     \
  XX(UNKNOWN, "an unknown error occurred")                           \
  XX(INVALID_TRANSFER_ENCODING,                                      \
     "request has invalid transfer-encoding")                        \


 /* Define HPE_* values for each errno value above */
#define HTTP_ERRNO_GEN(n, s) HPE_##n,
enum EHttpError {
    HTTP_ERRNO_MAP(HTTP_ERRNO_GEN)
};
#undef HTTP_ERRNO_GEN


//Flag values for HttpParser.mFlags
enum EHttpFlags {
    F_HEAD_DONE = 1 << 0,
    F_CONNECTION_KEEP_ALIVE = 1 << 1,
    F_CONNECTION_CLOSE = 1 << 2,
    F_CONNECTION_UPGRADE = 1 << 3,
    F_TAILING = 1 << 4,                 //last chunk
    F_UPGRADE = 1 << 5,
    F_SKIPBODY = 1 << 6,
    F_CONTENTLENGTH = 1 << 7,           //header content_length repeated
    F_CHUNKED = 1 << 9,
    F_BOUNDARY = 1 << 8,                //Boundary
    F_BOUNDARY_CMP = 1 << 10            //Boundary must been compared
};



enum EHttpParserType {
    EHTTP_BOTH = 0,
    EHTTP_REQUEST,
    EHTTP_RESPONSE
};


class HttpMsg;
class HttpLayer;


class HttpEventer : public RefCount {
public:
    HttpEventer() { }
    virtual ~HttpEventer() { }
    virtual s32 onClose() = 0;
    virtual s32 onOpen(HttpMsg& msg) = 0;
    virtual s32 onSent(HttpMsg& req) = 0;
    virtual s32 onFinish(HttpMsg& resp) = 0;
    virtual s32 onBodyPart(HttpMsg& resp) = 0;
};


class HttpMsg : public RefCount {
public:
    HttpMsg(HttpLayer* it);

    virtual ~HttpMsg();


    /**
    * @brief 支持<=18的后缀
    * @param filename, 文件名, 可以不以\0结尾，不用在意大小写。
    * @return mime if success, else "application/octet-stream" as default.
    */
    static const StringView getMimeType(const s8* filename, usz iLen);

    void setStationID(u8 id) {
        mStationID = id;
    }

    s32 getStationID()const {
        return mStationID;
    }

    void setEvent(HttpEventer* it) {
        if (mEvent) {
            mEvent->onClose();
            mEvent->drop();
        }
        if (it) {
            it->grab();
        }
        mEvent = it;
    }

    HttpEventer* getEvent()const {
        return mEvent;
    }

    void writeOutHead(const s8* name, const s8* value);

    void writeOutBody(const void* buf, usz bsz);

    RingBuffer& getCacheIn() {
        return mCacheIn;
    }

    RingBuffer& getCacheOut() {
        return mCacheOut;
    }

    HttpHead& getHeadIn() {
        return mHeadIn;
    }

    HttpHead& getHeadOut() {
        return mHeadOut;
    }

    HttpLayer* getHttpLayer() {
        return mLayer;
    }

    void clear() {
        mFlags = 0;
        mStatusCode = 0;
        mHeadIn.clear();
        mHeadOut.clear();
        mCacheIn.reset();
        mCacheOut.reset();
        mURL.clear();
    }

    /*
     @return 0=success, 1=skip body, 2=upgrade, else error
    */
    //s32 onHeadFinish(s32 flag, ssz bodySize);

    bool isKeepAlive()const {
        return (F_CONNECTION_KEEP_ALIVE & mFlags) > 0;
    }

    //test page: http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx
    //http://www.xingkong.com/
    bool isChunked()const {
        return (F_CHUNKED & mFlags) > 0;
    }

    void clearInBody() {
        mCacheIn.reset();
    }

    void clearOutBody() {
        mCacheOut.reset();
    }

    usz getInBodySize()const {
        return mCacheIn.getSize();
    }

    usz getOutBodySize()const {
        return mCacheOut.getSize();
    }

    /**request func*/
    s32 writeGet(const String& req);

    /**request func*/
    void setURL(const HttpURL& it) {
        mURL = it;
    }

    /**request func*/
    HttpURL& getURL() {
        return mURL;
    }

    /**request func*/
    void setMethod(EHttpMethod it) {
        mMethod = it;
    }

    /**request func*/
    EHttpMethod getMethod()const {
        return mMethod;
    }

    /**resp func*/
    void setStatus(u16 it) {
        mStatusCode = it;
    }
    u16 getStatus()const {
        return mStatusCode;
    }

    /**
     * resp func
    * @param val range[0-999]
    */
    void writeStatus(s32 val, const s8* str = "OK");

    /**resp func*/
    const String& getBrief()const {
        return mBrief;
    }

    /**resp func*/
    void setBrief(const s8* buf, usz len) {
        mBrief.setLen(0);
        mBrief.append(buf, len);
    }

    EHttpParserType getType()const {
        return mType;
    }


    void dumpHeadIn() {
        dumpHead(mHeadIn, mCacheIn);
    }

    void dumpHeadOut() {
        dumpHead(mHeadOut, mCacheOut);
    }


protected:
    void dumpHead(const HttpHead& hds, RingBuffer& out);

    u8 getRespStatus()const {
        return mRespStatus;
    }
    void setRespStatus(u8 it) {
        mRespStatus = it;
    }

    friend class HttpLayer;
    u8 mRespStatus; //for HttpLayer
    u8 mStationID;
    u16 mStatusCode;
    u16 mFlags;
    EHttpParserType mType;
    EHttpMethod mMethod;

    HttpHead mHeadIn;
    RingBuffer mCacheIn;

    HttpHead mHeadOut;
    RingBuffer mCacheOut;

    HttpURL mURL;   //request only
    String mBrief;  //response only

    HttpLayer* mLayer;
    HttpEventer* mEvent;
};


}//namespace net
}//namespace app


#endif //APP_HTTPMSG_H