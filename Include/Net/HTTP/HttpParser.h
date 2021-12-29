#ifndef APP_HTTP_PARSER_H
#define APP_HTTP_PARSER_H

#include "Strings.h"

namespace app {
namespace net {

/* Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run faster
 */
#ifndef DHTTP_PARSE_STRICT
# define DHTTP_PARSE_STRICT 1
#endif

 /* Maximium header size allowed. If the macro is not defined
  * before including this header then the default is used. To
  * change the maximum header size, define the macro in the build
  * environment (e.g. -DHTTP_MAX_HEADER_SIZE=<value>). To remove
  * the effective limit on the size of the header, define the macro
  * to a very large number (e.g. -DHTTP_MAX_HEADER_SIZE=0x7fffffff)
  */
#ifndef HTTP_MAX_HEADER_SIZE
# define HTTP_MAX_HEADER_SIZE (80*1024)
#endif


  /* Callbacks should return non-zero to indicate an error. The parser will
   * then halt execution.
   *
   * The one exception is mCallHeadComplete. In a EHTTP_RESPONSE parser
   * returning '1' from mCallHeadComplete will tell the parser that it
   * should not expect a body. This is used when receiving a response to a
   * HEAD request which may contain 'Content-Length' or 'Transfer-Encoding:
   * chunked' headers that indicate the presence of a body.
   *
   * Returning `2` from mCallHeadComplete will tell parser that it should not
   * expect neither a body nor any futher responses on this connection. This is
   * useful for handling responses to a CONNECT request which may not contain
   * `Upgrade` or `Connection: upgrade` headers.
   *
   * FuncHttpData does not return data chunks. It will be called arbitrarily
   * many times for each string. E.G. you might get 10 callbacks for "mCallURL"
   * each providing just a few characters more data.
   */
class HttpParser;
typedef s32(*FuncHttpData) (HttpParser&, const s8* at, usz length);
typedef s32(*FuncHttpHeader) (HttpParser&, const StringView& key, const StringView& val);
typedef s32(*FuncHttpParser) (HttpParser&);


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
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
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


/* Request Methods */
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


enum EHttpParserType {
    EHTTP_BOTH = 0,
    EHTTP_REQUEST,
    EHTTP_RESPONSE
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
  XX(CB_MsgBegin, "the mCallMsgBegin callback failed")               \
  XX(CB_URL, "the mCallURL callback failed")                         \
  XX(CB_HeaderField, "the mCallHeaderField callback failed")         \
  XX(CB_HeaderValue, "the mCallHeaderValue callback failed")         \
  XX(CB_HeadersComplete, "the mCallHeadComplete callback failed")    \
  XX(CB_Body, "the mCallBody callback failed")                       \
  XX(CB_MsgComplete, "the mCallMsgComplete callback failed")         \
  XX(CB_Status, "the mCallURL callback failed")                      \
  XX(CB_ChunkHeader, "the mCallChunkHeader callback failed")         \
  XX(CB_ChunkComplete, "the mCallChunkComplete callback failed")     \
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
    F_HEAD_DONE = 1 << 0              //header done
    , F_CONNECTION_KEEP_ALIVE = 1 << 1
    , F_CONNECTION_CLOSE = 1 << 2
    , F_CONNECTION_UPGRADE = 1 << 3
    , F_TAILING = 1 << 4                 //last chunk
    , F_UPGRADE = 1 << 5
    , F_SKIPBODY = 1 << 6
    , F_CONTENTLENGTH = 1 << 7          //header content_length repeated
    , F_CHUNKED = 1 << 9                //
    , F_BOUNDARY = 1 << 8               //Boundary
    , F_BOUNDARY_CMP = 1 << 10          //Boundary must been compared
};

enum EHttpUrlFields {
    UF_SCHEMA = 0
    , UF_HOST = 1
    , UF_PORT = 2
    , UF_PATH = 3
    , UF_QUERY = 4
    , UF_FRAGMENT = 5
    , UF_USERINFO = 6
    , UF_MAX = 7
};


/**
 * @brief Result structure for http parse url.
 *
 * Callers should index into field_data[] with UF_* values iff field_set
 * has the relevant (1 << UF_*) bit set. As a courtesy to clients (and
 * because we probably have padding left over), we convert any port to
 * a u16.
 */
class HttpParserURL {
public:
    u16 mFieldSet;           /* Bitmask of (1 << UF_*) values */
    u16 mPort;                /* Converted UF_PORT string */

    struct {
        u16 mOffset;          /* Offset into buffer in which field starts */
        u16 mLen;             /* Length of run in buffer */
    } mFieldData[UF_MAX];

    HttpParserURL() {
        init();
    }

    void init() {
        memset(this, 0, sizeof(*this));
    }

    /* Parse a URL; return nonzero on failure */
    s32 parseURL(const s8* buf, usz buflen, s32 is_connect);

    s32 parseHost(const s8* buf, s32 found_at);
};


/**
 * @breif 除body回调，其它皆在消息完整时回调(消息最大长受到TcpHandle中接收缓存区限制)
 *  body回调不受缓存区限制，其它消息如在缓存区满时仍然不完整，将认为受到攻击并需切断连接。
 */
class HttpParser {
public:
    HttpParser() {
        memset(this, 0, sizeof(*this));
    }

    void init(EHttpParserType type, void* user);

    /* Executes the parser. Returns number of parsed bytes. Sets
     * `parser->EHttpError` on error. */
    usz parseBuf(const s8* data, usz len);

    /* Checks if this is the final chunk of the body. */
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
        return (EHttpError)mHttpError;
    }

    //Does the parser need to see an EOF to find the end of the message?
    bool needEOF()const;

    static void setMaxHeaderSize(u32 size) {
        GMAX_HEAD_SIZE = size;
    }

    static const s8* getMethodStr(EHttpMethod m);

    static const s8* getStatusStr(EHttpStatus s);

    static const s8* getErrorStr(EHttpError err);

    /** eg:
        Content-Type: multipart/form-data; boundary="---soun"
        out = {"multipart"="form-data","boundary"="---soun"};

        Content-Disposition: form-data; name="field"; filename="filename.jpg"
        out = {"form-data"="","name"="field","filename"="filename.jpg"};
    */
    static const s8* parseValue(const s8* curr, const s8* end,
        StringView* out, s32& omax, s32& vflag);

    const s8* getErrorBrief(EHttpError err);

protected:
    friend class HttpLayer;

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
    u16 mStatusCode; // responses only
    u8 mMethod;      // requests only
    u8 mHttpError;

    u8 mBoundaryLen;
    s8 mBoundary[256];

    u8 mFormNameLen;
    s8 mFormName[256];

    u8 mFileNameLen;
    s8 mFileName[256];

    /* A pointer to get hook to the "connection" or "socket" object */
    void* mUserData;

    //callbacks
    FuncHttpParser mCallMsgBegin;
    FuncHttpData mCallURL;
    FuncHttpData mCallStatus;
    FuncHttpHeader mCallHeader;
    FuncHttpParser mCallHeadComplete;
    FuncHttpData mCallBody;
    FuncHttpParser mCallMsgComplete;
    /*
     * When mCallChunkHeader is called, the current chunk length is stored
     * in parser->content_length.
     */
    FuncHttpParser mCallChunkHeader;
    FuncHttpParser mCallChunkComplete;

    void setError(EHttpError it) {
        mHttpError = it;
    }

private:
    static u32 GMAX_HEAD_SIZE;

    void reset();
    const s8* parseBoundBody(const s8* curr, const s8* end, StringView& tbody);
};



}//namespace app
}//namespace net

#endif //APP_HTTP_PARSER_H
