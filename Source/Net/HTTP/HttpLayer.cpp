#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpParserDef.h"
#include "Net/HTTP/Website.h"
#include "Net/Acceptor.h"
#include "Loop.h"
#include "Timer.h"

namespace app {
namespace net {

u32 HttpLayer::GMAX_HEAD_SIZE = HTTP_MAX_HEADER_SIZE;

HttpLayer::HttpLayer(EHttpParserType tp, bool https, TlsContext* tlsContext) :
    mPType(tp), mWebSite(nullptr), mMsg(nullptr), mHttpError(HPE_OK), mHTTPS(https), mTlsContext(tlsContext) {
    clear();
}


HttpLayer::~HttpLayer() {
    if (mWebSite) {
        mWebSite->drop();
        mWebSite = nullptr;
    }
}


s32 HttpLayer::launch(HttpMsg* msg) {
    if (mMsg) {
        return EE_ERROR;
    }
    mHTTPS = msg->getURL().isHttps();
    if (!mHTTPS) {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.getHandleTCP().setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    } else {
        mTCP.setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    }
    StringView host = msg->getURL().getHost();
    NetAddress addr(msg->getURL().getPort());
    s8 shost[256]; // 255, Maximum host name defined in RFC 1035
    snprintf(shost, sizeof(shost), "%.*s", (s32)(host.mLen), host.mData);
    addr.setDomain(shost);

    RequestFD* nd = RequestFD::newRequest(4 * 1024);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnConnect;
    s32 ret = mHTTPS ? mTCP.open(addr, nd) : mTCP.getHandleTCP().open(addr, nd);
    if (EE_OK != ret) {
        RequestFD::delRequest(nd);
        return ret;
    }
    mMsg = msg;
    msg->grab();
    grab();
    return EE_OK;
}


void HttpLayer::msgBegin() {
    if (mWebSite) {
        if (mMsg) {
            mHttpError = HPE_CB_MsgBegin;
            postClose();
            DLOG(ELL_ERROR, "HttpLayer::msgBegin>> fail pre_msg check");
            return; // error
        }
        mMsg = new HttpMsg(this);
    }
}

void HttpLayer::msgEnd() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->getEvent()->onReqBodyDone(mMsg);
        mMsg->drop();
        mMsg = nullptr;
    }
}


void HttpLayer::msgPath() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mType = (EHttpParserType)mType;
        mMsg->mFlags = mFlags;
    }
}

s32 HttpLayer::headDone() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mFlags = mFlags;
        if (!mMsg->getEvent()) {
            mWebSite->createMsgEvent(mMsg);
        }
        if (EE_OK != mMsg->getEvent()->onReqHeadDone(mMsg)) {
            postClose();
        }
    }
    return 0;
}

void HttpLayer::chunkHeadDone() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mFlags = mFlags;
        mMsg->getEvent()->onReqChunkHeadDone(mMsg);
    }
}


void HttpLayer::msgBody() {
    if (mMsg) {
        mMsg->getEvent()->onReqBody(mMsg);
    }
}

void HttpLayer::msgError() {
    if (mMsg) {
        if (mMsg->getEvent()) {
            mMsg->getEvent()->onReadError(mMsg);
        }
        mMsg->drop();
        mMsg = nullptr;
    }
}

void HttpLayer::chunkMsg() {
    if (mMsg) {
        mMsg->getEvent()->onReqBody(mMsg);
    }
}

void HttpLayer::chunkDone() {
    if (mMsg) {
        mMsg->getEvent()->onReqChunkBodyDone(mMsg);
    }
}


bool HttpLayer::sendReq() {
    RingBuffer& bufs = mMsg->getCacheOut();
    if (bufs.getSize() > 0) {
        RequestFD* nd = RequestFD::newRequest(0);
        nd->mUser = mMsg;
        nd->mCall = HttpLayer::funcOnWrite;
        StringView msg = bufs.peekHead();
        nd->mData = msg.mData;
        nd->mAllocated = (u32)msg.mLen;
        nd->mUsed = (u32)msg.mLen;
        s32 ret = writeIF(nd);
        if (0 != ret) {
            RequestFD::delRequest(nd);
            return false;
        }
        mMsg->grab();
    }
    return true;
}

s32 HttpLayer::sendResp(HttpMsg* msg) {
    DASSERT(msg);

    if (msg->getRespStatus() > 0) {
        return EE_OK;
    }

    RingBuffer& bufs = msg->getCacheOut();
    RequestFD* nd = RequestFD::newRequest(0);
    nd->mUser = msg;
    nd->mCall = HttpLayer::funcOnWrite;
    StringView pack = bufs.peekHead();
    nd->mData = pack.mData;
    nd->mAllocated = (u32)pack.mLen;
    nd->mUsed = (u32)pack.mLen;
    s32 ret = writeIF(nd);
    if (EE_OK != ret) {
        RequestFD::delRequest(nd);
        return ret;
    }
    msg->grab();
    msg->setRespStatus(1);
    return EE_OK;
}



#ifdef DDEBUG
s32 TestHttpReceive(HttpLayer& mMsg) {
    usz tlen;
    usz used;

    /*const s8* str0 =
        "POST /fs/upload HTTP/1.1\r\n"
        "Con";

    tlen = strlen(str0);
    used = parseBuf(str0, tlen);

    const s8* str1 = "Content-Length: 8";
    tlen = strlen(str1);
    used = parseBuf(str1, tlen);
    const s8* str2 = "Content-Length: 8\r\n";

    tlen = strlen(str2);
    used = parseBuf(str2, tlen);
    const s8* str3 =
        "Connection: Keep-Alive\r\n"
        "\r\n"
        "abcd";

    tlen = strlen(str3);
    used = parseBuf(str3, tlen);
    const s8* str4 = "len5";

    tlen = strlen(str4);
    used = parseBuf(str4, tlen);*/

    const s8* test = "POST /fs/upload HTTP/1.1\r\n"
                     "Content-Length: 18\r\n"
                     "Content-Type: multipart/form-data; boundary=vksoun\r\n"
                     "Connection: Keep-Alive\r\n"
                     "Cookie: Mailbox=yyyy@qq.com\r\n"
                     "\r\n"
                     "--vksoun\r\n" // boundary
                     "Content-Disposition: form-data; name=\"fieldName\"; filename=\"filename.txt\"\r\n"
                     "\r\n"
                     "msgPart1"
                     "\r\n"
                     "--vksoun\r\n" // boundary
                     "Content-Type: text/plain\r\n"
                     "\r\n"
                     "--vksoun-"
                     "\r\n\r\n"
                     "--vksoun--" // boundary tail
        ;
    tlen = strlen(test);
    used = mMsg.parseBuf(test, tlen);
    const s8* laststr = test + used;

    return 0;
}
#endif



s32 HttpLayer::onTimeout(HandleTime& it) {
    return shouldKeepAlive() ? EE_OK : EE_ERROR;
}


void HttpLayer::onClose(Handle* it) {
#ifdef DDEBUG
    if (mHTTPS) {
        DASSERT((&mTCP == it) && "HttpLayer::onClose https handle?");
    } else {
        DASSERT((&mTCP.getHandleTCP() == it) && "HttpLayer::onClose http handle?");
    }
#endif
    s32 cnt = -99;
    if (mMsg) {
        if (mMsg->getEvent()) {
            mMsg->getEvent()->onLayerClose(mMsg);
        }
        cnt = mMsg->drop();
        mMsg = nullptr;
    }
    s32 my = drop();
    DLOG(ELL_INFO, "onClose>> my_grab= %d, msg_grab= %d", my, cnt);
}


bool HttpLayer::onLink(RequestFD* it) {
    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    RequestAccept& req = *(RequestAccept*)it;
    mWebSite = reinterpret_cast<Website*>(accp->getUser());
    if (!mWebSite) {
        DLOG(ELL_ERROR, "HttpLayer::onLink>> invalid website");
        return false;
    }
    if (mHTTPS) {
        mTCP.setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    } else {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.getHandleTCP().setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    }
    RequestFD* nd = RequestFD::newRequest(4 * 1024);
    nd->mUser = this;
    nd->mCall = HttpLayer::funcOnRead;
    s32 ret = mHTTPS ? mTCP.open(req, nd, mTlsContext) : mTCP.getHandleTCP().open(req, nd);
    if (0 == ret) {
        mWebSite->grab();
        grab();
        Logger::log(ELL_INFO, "HttpLayer::onLink>> [%s->%s]", mTCP.getRemote().getStr(), mTCP.getLocal().getStr());
    } else {
        mWebSite = nullptr;
        RequestFD::delRequest(nd);
        Logger::log(ELL_ERROR, "HttpLayer::onLink>> [%s->%s], ecode=%d", mTCP.getRemote().getStr(),
            mTCP.getLocal().getStr(), ret);
    }
    return EE_OK == ret;
}

void HttpLayer::onConnect(RequestFD* it) {
    if (EE_OK == it->mError) {
        mMsg->buildReq();
        if (sendReq()) {
            it->mCall = HttpLayer::funcOnRead;
            if (EE_OK == readIF(it)) {
                return;
            }
        }
    }
    DLOG(ELL_ERROR, "HttpLayer::onConnect>> [url=%d] [ip=%s], ecode=%d", mMsg->getURL().data().data(),
        mTCP.getRemote().getStr(), it->mError);
    RequestFD::delRequest(it);
}

void HttpLayer::onWrite(RequestFD* it, HttpMsg* msg) {
    if (EE_OK != it->mError) {
        DLOG(ELL_ERROR, "onWrite>>size=%u, ecode=%d, msg=%s", it->mUsed, it->mError, msg->getRealPath().data());
        msg->getEvent()->onRespWriteError(msg);
    } else {
        msg->setRespStatus(0);
        msg->getCacheOut().commitHead(static_cast<s32>(it->mUsed));
        if (msg->getCacheOut().getSize() > 0) {
            sendResp(msg);
        } else {
            if (EE_OK != msg->getEvent()->onRespWrite(msg)) {
                if (!msg->isKeepAlive()) {
                    // mTCP.setTimeout(10000);  TODO
                    // postClose();  //delay close on timeout
                }
            }
        }
    }
    RequestFD::delRequest(it);
    msg->drop();
}


void HttpLayer::onRead(RequestFD* it) {
    const s8* dat = it->getBuf();
    ssz datsz = it->mUsed;
    if (0 == datsz) {
        parseBuf(dat, 0); // make the http-msg-finish callback
    } else {
        ssz parsed = 0;
        ssz stepsz;
        while (datsz > 0 && 0 == mHttpError) {
            stepsz = parseBuf(dat + parsed, datsz);
            parsed += stepsz;
            if (stepsz < datsz) {
                break; // leftover
            }
            datsz -= stepsz;
        }
        it->clearData((u32)parsed);

        if (0 == it->getWriteSize()) {
            // 可能受到超长header攻击或其它错误
            Logger::logError("HttpLayer::onRead>>remote=%s, msg overflow", mTCP.getRemote().getStr());
            postClose();
        } else {
            if (0 == mHttpError && EE_OK == readIF(it)) {
                return;
            }
            postClose();
            Logger::logError(
                "HttpLayer::onRead>>remote=%s, parser err=%d=%s", mTCP.getRemote().getStr(), mHttpError, getErrStr());
        }
    }

    // del req
#if defined(DDEBUG)
    printf("HttpLayer::onRead>>del req, read=%lld, ecode=%d, parser err=%d=%s\n", datsz, it->mError, mHttpError,
        getErrStr());
#endif
    RequestFD::delRequest(it);
}


void HttpLayer::postClose() {
    DLOG(ELL_INFO, "HttpLayer::postClose remote = %s", mTCP.getHandleTCP().getRemote().getStr());
    mTCP.getHandleTCP().launchClose();
}



/////////////////////////

#define COUNT_HEADER_SIZE(V)                                                                                           \
    do {                                                                                                               \
        nread += (u32)(V);                                                                                             \
        if (UNLIKELY(nread > GMAX_HEAD_SIZE)) {                                                                        \
            mHttpError = HPE_HEADER_OVERFLOW;                                                                          \
            goto GT_ERROR;                                                                                             \
        }                                                                                                              \
    } while (0)


#define PROXY_CONNECTION "proxy-connection"
#define CONNECTION "connection"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define UPGRADE "upgrade"
#define CHUNKED "chunked"
#define KEEP_ALIVE "keep-alive"
#define CLOSE "close"

#define CONTENT_ "content-"
#define CONTENT_TYPE "content-type"
#define CONTENT_DISP "content-disposition"


#define MLT_PART "multipart"
#define MLT_PART_MIXED "multipart/mixed"
#define MLT_PART_FORM "multipart/form-data"


static const s8* GHTTP_METHOD_STRS[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};


/* Tokens as defined by rfc 2616. Also lowercases them.
 *        token       = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 */
static const s8 GTokens[256] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
    0, 0, 0, 0, 0, 0, 0, 0,
    /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
    0, 0, 0, 0, 0, 0, 0, 0,
    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
    0, 0, 0, 0, 0, 0, 0, 0,
    /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
    0, 0, 0, 0, 0, 0, 0, 0,
    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
    ' ', '!', 0, '#', '$', '%', '&', '\'',
    /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
    0, 0, '*', '+', 0, '-', '.', 0,
    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
    '0', '1', '2', '3', '4', '5', '6', '7',
    /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
    '8', '9', 0, 0, 0, 0, 0, 0,
    /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
    0, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
    'x', 'y', 'z', 0, 0, 0, '^', '_',
    /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
    'x', 'y', 'z', 0, '|', 0, '~', 0};


static const u8 GMAP_UN_HEX[256] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 10, 11, 12, 13, 14, 15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 10, 11,
    12, 13, 14, 15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


#if DHTTP_PARSE_STRICT
#define T(v) 0
#else
#define T(v) v
#endif


static const u8 G_NORMAL_URL_CHAR[32] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
    /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
    0 | T(2) | 0 | 0 | T(16) | 0 | 0 | 0,
    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
    /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
    0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
    0 | 2 | 4 | 0 | 16 | 32 | 64 | 128,
    /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
    /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
    /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
};

#undef T
// DHTTP_PARSE_STRICT



/**
 * @brief used for parse value
 *   Content-Type: multipart/mixed; boundary="yyds"
 *   Content-Type: multipart/form-data; boundary="yyds"
 *
 *   Content-Disposition: inline
 *   Content-Disposition: attachment
 *   Content-Disposition: attachment; filename="filename.jpg"
 *
 *   Content-Disposition: form-data
 *   Content-Disposition: form-data; name="fieldName"
 *   Content-Disposition: form-data; name="fieldName"; filename="filename.jpg"
 */
enum EHeadValueState {
    EHV_ERROR = 0,

    // for value
    EHV_START_NAME,
    EHV_PRE_NAME,
    EHV_NAME,
    EHV_PRE_VALUE,
    EHV_VALUE,

    // for boundary body
    EHV_BOUNDARY_CMP_PRE,  // \n in body
    EHV_BOUNDARY_CMP_PRE1, // first "-"
    EHV_BOUNDARY_CMP_PRE2, // 2nd   "-"
    EHV_BOUNDARY_CMP,      // compare multipart's boundary
    EHV_BOUNDARY_CMP_DONE,
    EHV_BOUNDARY_CMP_TAIL, // compare the tail "--"
    EHV_BOUNDARY_BODY_PRE, // body start
    EHV_BOUNDARY_BODY,

    // for flags
    EHV_MATCH_TYPE_MULTI = 0x10000000,   // Content-Type: multipart/
    EHV_CONTENT_TYPE_MULTI = 0x20000000, // Content-Type: multipart/...
    EHV_DONE = 0x80000000,
    // EHV_CONTENT_DISP = 0x20000000       //Content-Disposition
};

enum EHttpHeaderState {
    EH_NORMAL = 0,
    EH_C,
    EH_CO,
    EH_CON,

    EH_CMP_CONNECTION,
    EH_CMP_PROXY_CONNECTION,
    EH_CMP_CONTENT,      // Content-
    EH_CMP_CONTENT_TYPE, // Content-Type
    EH_CMP_CONTENT_DISP, // content-disposition
    EH_CMP_CONTENT_LEN,
    EH_CMP_TRANSFER_ENCODING,
    EH_CMP_UPGRADE,

    EH_CONNECTION,
    EH_CONTENT_TYPE, // Content-Type
    EH_CONTENT_TYPE_BOUNDARY,
    EH_CONTENT_DISP, // content-disposition
    EH_CONTENT_LEN,
    EH_CONTENT_LEN_NUM,
    EH_CONTENT_LEN_WS,
    EH_TRANSFER_ENCODING,
    EH_UPGRADE,
    EH_CMP_TRANSFER_ENCODING_TOKEN_START,
    EH_CMP_TRANSFER_ENCODING_TOKEN,
    EH_CMP_TRANSFER_ENCODING_CHUNCKED,
    EH_CMP_CONNECTION_TOKEN_START,
    EH_CMP_CONNECTION_KEEP_ALIVE,
    EH_CMP_CONNECTION_CLOSE,
    EH_CMP_CONNECTION_UPGRADE,
    EH_CMP_CONNECTION_TOKEN,
    EH_TRANSFER_ENCODING_CHUNCKED,
    EH_CONNECTION_KEEP_ALIVE,
    EH_CONNECTION_CLOSE,
    EH_CONNECTION_UPGRADE
};


// Verify that a char is a valid visible (printable) US-ASCII character or %x80-FF
#define IS_HEADER_CHAR(ch) (ch == CR || ch == LF || ch == 9 || ((u8)ch > 31 && ch != 127))


#if DHTTP_PARSE_STRICT
#define STRICT_CHECK(cond)                                                                                             \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            mHttpError = HPE_STRICT;                                                                                   \
            goto GT_ERROR;                                                                                             \
        }                                                                                                              \
    } while (0)

#define NEW_MESSAGE() (shouldKeepAlive() ? (mType == EHTTP_REQUEST ? PS_REQ_START : PS_RESP_START) : PS_DEAD)

#else
#define STRICT_CHECK(cond)
#define NEW_MESSAGE() (mType == EHTTP_REQUEST ? PS_REQ_START : PS_RESP_START)
#endif


/* Our URL parser.
 *
 * This is designed to be shared by parseBuf() for URL validation,
 * hence it has a state transition + byte-for-byte interface. In addition, it
 * is meant to be embedded in parseURL(), which does the dirty
 * work of turning state transitions URL components for its API.
 *
 * This function should only be invoked with non-space characters. It is
 * assumed that the caller cares about (and can detect) the transition between
 * URL and non-URL states by looking for these.
 */
static EPareState AppParseUrlChar(EPareState s, const s8 ch) {
    if (ch == ' ' || ch == '\r' || ch == '\n') {
        return PS_DEAD;
    }

#if DHTTP_PARSE_STRICT
    if (ch == '\t' || ch == '\f') {
        return PS_DEAD;
    }
#endif

    switch (s) {
    case PS_REQ_URL_PRE:
        /* Proxied requests are followed by scheme of an absolute URI (alpha).
         * All methods except CONNECT are followed by '/' or '*'.
         */
        if (ch == '/' || ch == '*') {
            return PS_REQ_URL_PATH;
        }
        if (IS_ALPHA(ch)) {
            return PS_REQ_URL_SCHEMA;
        }
        break;

    case PS_REQ_URL_SCHEMA:
        if (IS_ALPHA(ch)) {
            return s;
        }
        if (ch == ':') {
            return PS_REQ_URL_SLASH;
        }
        break;

    case PS_REQ_URL_SLASH:
        if (ch == '/') {
            return PS_REQ_URL_SLASH2;
        }
        break;

    case PS_REQ_URL_SLASH2:
        if (ch == '/') {
            return PS_REQ_URL_HOST;
        }
        break;

    case PS_REQ_SERVER_AT:
        if (ch == '@') {
            return PS_DEAD;
        }

        /* fall through */
    case PS_REQ_URL_HOST:
    case PS_REQ_SERVER:
        if (ch == '/') {
            return PS_REQ_URL_PATH;
        }
        if (ch == '?') {
            return PS_REQ_URL_QUERY;
        }
        if (ch == '@') {
            return PS_REQ_SERVER_AT;
        }
        if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
            return PS_REQ_SERVER;
        }
        break;

    case PS_REQ_URL_PATH:
        if (IS_URL_CHAR(ch)) {
            return s;
        }
        switch (ch) {
        case '?':
            return PS_REQ_URL_QUERY;
        case '#':
            return PS_REQ_URL_FRAG;
        }
        break;

    case PS_REQ_URL_QUERY:
    case PS_REQ_URL_QUERY_KEY:
        if (IS_URL_CHAR(ch)) {
            return PS_REQ_URL_QUERY_KEY;
        }
        switch (ch) {
        case '?':
            /* allow extra '?' in query string */
            return PS_REQ_URL_QUERY_KEY;
        case '#':
            return PS_REQ_URL_FRAG;
        }
        break;

    case PS_REQ_URL_FRAG:
        if (IS_URL_CHAR(ch)) {
            return PS_REQ_URL_FRAGMENT;
        }
        switch (ch) {
        case '?':
            return PS_REQ_URL_FRAGMENT;
        case '#':
            return s;
        }
        break;

    case PS_REQ_URL_FRAGMENT:
        if (IS_URL_CHAR(ch)) {
            return s;
        }
        switch (ch) {
        case '?':
        case '#':
            return s;
        }
        break;

    default:
        break;
    }

    /* We should never fall out of the switch above unless there's an error */
    return PS_DEAD;
}

const s8* HttpLayer::parseValue(const s8* curr, const s8* end, StringView* out, s32& omax, s32& vflag) {
    vflag = 0;
    s32 used = 0;
    StringView kname;
    StringView kval;
    EHeadValueState vstat = EHV_START_NAME;
    s8 ch;
    for (; curr < end; ++curr) {
        if (CR == *curr || LF == *curr) {
            if (kname.mData && used + 1 < omax) {
                if (!kval.mData) {
                    kval.set(curr, 0);
                }
                out[used++] = kname;
                out[used++] = kval;
            }
            vflag |= EHV_DONE;
            goto GT_RET;
        }

        switch (vstat) {
        case EHV_START_NAME:
        {
            kname.set(curr, 0);
            ch = LOWER(*curr);
            if ('m' == ch) {
                vstat = EHV_MATCH_TYPE_MULTI;
                break;
            }
            vstat = EHV_NAME;
            break;
        }
        case EHV_MATCH_TYPE_MULTI:
        {
            ++kname.mLen;
            if (';' == *curr) {
                kval.set(curr, 0); // empty value
                if (used + 1 < omax) {
                    out[used++] = kname;
                    out[used++] = kval;
                }
                kname.set(nullptr, 0);
                kval.set(nullptr, 0);
                vstat = EHV_PRE_NAME;
                break;
            }
            if ('/' == *curr) {
                if (kname.mLen == sizeof(MLT_PART) - 1) {
                    vflag |= EHV_CONTENT_TYPE_MULTI;
                }
                vstat = EHV_PRE_VALUE;
                break;
            }
            ch = LOWER(*curr);
            if (kname.mLen > sizeof(MLT_PART) - 1 || ch != MLT_PART[kname.mLen]) {
                vstat = EHV_NAME;
            }
            break;
        }

        case EHV_PRE_NAME:
            if (';' == *curr || ' ' == *curr) {
                break;
            }
            vstat = EHV_NAME;
            kname.set(curr, 0);
            // break; fall

        case EHV_NAME:
            if (' ' == *curr || '=' == *curr) {
                vstat = EHV_PRE_VALUE;
                break;
            }
            ++kname.mLen;
            if ('/' == *curr) {
                vstat = EHV_PRE_VALUE;
            } else if (';' == *curr) {
                kval.set(curr, 0); // empty value
                if (used + 1 < omax) {
                    out[used++] = kname;
                    out[used++] = kval;
                }
                kname.set(nullptr, 0);
                kval.set(nullptr, 0);
                vstat = EHV_PRE_NAME;
            }
            break;

        case EHV_PRE_VALUE:
            if (' ' == *curr || '=' == *curr || '"' == *curr) {
                break;
            }
            vstat = EHV_VALUE;
            kval.set(curr, 0);
            // no break

        case EHV_VALUE:
            if ('"' == *curr || ';' == *curr || ' ' == *curr) {
                vstat = EHV_PRE_NAME;
                if (used + 1 < omax) {
                    out[used++] = kname;
                    out[used++] = kval;
                }
                kname.set(nullptr, 0);
                kval.set(nullptr, 0);
                break;
            }
            ++kval.mLen;
            break;

        default:
            break;
        } // switch
    }     // for


GT_RET:
    omax = used;
    return curr;
}


void HttpLayer::clear() {
    mType = mPType;
    mVersionMajor = 0;
    mVersionMinor = 0;
    mStatusCode = 0;
    mMethod = HTTP_GET;
    mState = (mType == EHTTP_REQUEST ? PS_REQ_START : (mType == EHTTP_RESPONSE ? PS_RESP_START : PS_START_MSG));
    mUpgrade = 0;
    mAllowChunkedLen = 0;
    mLenientHeaders = 0;
    reset();
}

void HttpLayer::reset() {
    mHttpError = HPE_OK;
    mReadSize = 0;
    mFlags = 0;
    mIndex = 0;
    mUseTransferEncode = 0;
    mContentLen = GMAX_USIZE;
    mBoundaryLen = 0;
    mBoundary[0] = 0;

    mFormNameLen = 0;
    mFormName[0] = 0;

    mFileNameLen = 0;
    mFileName[0] = 0;
}


usz HttpLayer::parseBuf(const s8* data, usz len) {
    if (mHttpError != HPE_OK) {
        return 0;
    }

    s8 ch;   // raw byte
    s8 lowc; // low char of ch
    u8 unhex_val;
    StringView tmpkey(data, 0);
    StringView tmpval(data, 0);
    StringView tbody(data, 0);
    const s8* pp = data;
    const s8* pe = data; // for return len
    const s8* const end = data + len;
    mHeaderState = EH_NORMAL;
    EPareState tmpstate = (EPareState)mState;
    const u32 lenient = mLenientHeaders;
    const u32 allow_chunked_length = mAllowChunkedLen;

    u32 nread = mReadSize;

    if (len == 0) {
        switch (tmpstate) {
        case PS_BODY_EOF:
            msgEnd();
            return 0;

        case PS_DEAD:
        case PS_START_MSG:
        case PS_RESP_START:
        case PS_REQ_START:
            return 0;

        default:
            mReadSize = nread;
            mHttpError = HPE_INVALID_EOF_STATE;
            return 0;
        }
    }

    for (pp = data; pp < end; ++pp) {
        ch = *pp;
        if (tmpstate <= PS_HEAD_DONE) { //@note PARSING_HEADER
            COUNT_HEADER_SIZE(1);
        }

GT_REPARSE: // recheck current byte
        switch (tmpstate) {
        case PS_DEAD:
            /* this state is used after a 'Connection: close' message
             * the parser will error out if it reads another message */
            if (LIKELY(ch == CR || ch == LF)) {
                break;
            }
            mHttpError = HPE_CLOSED_CONNECTION;
            goto GT_ERROR;

        case PS_START_MSG:
        {
            if (ch == CR || ch == LF) {
                break;
            }
            if (ch == 'H') {
                tmpstate = PS_START_H;
            } else {
                mType = EHTTP_REQUEST;
                tmpstate = PS_REQ_START;
                goto GT_REPARSE;
            }
            reset();
            mState = tmpstate;
            pe = pp + 1; // steped
            break;
        }

        case PS_START_H:
            if (ch == 'T') {
                mType = EHTTP_RESPONSE;
                tmpstate = PS_RESP_HT;
            } else {
                if (UNLIKELY(ch != 'E')) {
                    mHttpError = HPE_INVALID_CONSTANT;
                    goto GT_ERROR;
                }
                mType = EHTTP_REQUEST;
                mMethod = HTTP_HEAD;
                mIndex = 2;
                tmpstate = PS_REQ_METHOD;
            }
            mState = tmpstate;
            pe = pp + 1; // steped
            break;

        case PS_RESP_START:
        {
            if (ch == CR || ch == LF) {
                break;
            }
            reset();
            if (ch == 'H') {
                tmpstate = PS_RESP_H;
            } else {
                mHttpError = HPE_INVALID_CONSTANT;
                goto GT_ERROR;
            }

            msgBegin();
            mState = tmpstate;
            pe = pp + 1; // steped
            if (HPE_OK != mHttpError) {
                return pe - data;
            }
            break;
        }

        case PS_RESP_H:
            STRICT_CHECK(ch != 'T');
            tmpstate = (PS_RESP_HT);
            break;

        case PS_RESP_HT:
            STRICT_CHECK(ch != 'T');
            tmpstate = PS_RESP_HTT;
            break;

        case PS_RESP_HTT:
            STRICT_CHECK(ch != 'P');
            tmpstate = PS_RESP_HTTP;
            break;

        case PS_RESP_HTTP:
            STRICT_CHECK(ch != '/');
            tmpstate = PS_RESP_VER_MAJOR;
            break;

        case PS_RESP_VER_MAJOR:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            mVersionMajor = ch - '0';
            tmpstate = PS_RESP_VER_DOT;
            break;

        case PS_RESP_VER_DOT:
        {
            if (UNLIKELY(ch != '.')) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            tmpstate = PS_RESP_VER_MINOR;
            break;
        }

        case PS_RESP_VER_MINOR:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            mVersionMinor = ch - '0';
            tmpstate = PS_RESP_VER_END;
            break;

        case PS_RESP_VER_END:
        {
            if (UNLIKELY(ch != ' ')) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            tmpstate = PS_RESP_CODE_PRE;
            break;
        }

        case PS_RESP_CODE_PRE:
        {
            if (!IS_NUM(ch)) {
                if (ch == ' ') {
                    break;
                }
                mHttpError = HPE_INVALID_STATUS;
                goto GT_ERROR;
            }
            mStatusCode = ch - '0';
            tmpstate = PS_RESP_CODE;
            mState = PS_RESP_CODE; // steped
            break;
        }

        case PS_RESP_CODE:
        {
            if (!IS_NUM(ch)) {
                switch (ch) {
                case ' ':
                    tmpstate = PS_RESP_DESC_PRE;
                    break;
                case CR:
                case LF:
                    tmpstate = PS_RESP_DESC_PRE; // empty desc
                    goto GT_REPARSE;
                    break;
                default:
                    mHttpError = HPE_INVALID_STATUS;
                    goto GT_ERROR;
                }
                break;
            }

            mStatusCode *= 10;
            mStatusCode += ch - '0';
            if (UNLIKELY(mStatusCode > 999)) {
                mHttpError = HPE_INVALID_STATUS;
                goto GT_ERROR;
            }
            break;
        }

        case PS_RESP_DESC_PRE:
        {
            mMsg->setStatus(mStatusCode);
            tmpval.set(pp, 0);
            tmpstate = PS_RESP_DESC;
            mIndex = 0;
            if (ch == CR || ch == LF) {
                goto GT_REPARSE;
            }
            break;
        }

        case PS_RESP_DESC:
            if (ch == CR || ch == LF) {
                tmpstate = (CR == ch ? PS_RESP_LINE_END : PS_HEAD_FIELD_PRE);
                mState = tmpstate;
                pe = pp + 1; // steped
                DASSERT(mHttpError == HPE_OK);
                if (LIKELY(mMsg)) {
                    tmpval.mLen = pp - tmpval.mData;
                    mMsg->setBrief(tmpval);
                }
                break;
            }
            break;

        case PS_RESP_LINE_END:
            STRICT_CHECK(ch != LF);
            tmpstate = PS_HEAD_FIELD_PRE;
            mState = tmpstate;
            pe = pp + 1; // steped
            break;

        case PS_REQ_START:
        {
            if (ch == CR || ch == LF) {
                break;
            }
            reset();
            if (UNLIKELY(!IS_ALPHA(ch))) {
                mHttpError = HPE_INVALID_METHOD;
                goto GT_ERROR;
            }
            mMethod = (enum EHttpMethod)0;
            mIndex = 1;
            switch (ch) {
            case 'A':
                mMethod = HTTP_ACL;
                break;
            case 'B':
                mMethod = HTTP_BIND;
                break;
            case 'C':
                mMethod = HTTP_CONNECT; /* or COPY, CHECKOUT */
                break;
            case 'D':
                mMethod = HTTP_DELETE;
                break;
            case 'G':
                mMethod = HTTP_GET;
                break;
            case 'H':
                mMethod = HTTP_HEAD;
                break;
            case 'L':
                mMethod = HTTP_LOCK; /* or LINK */
                break;
            case 'M':
                mMethod = HTTP_MKCOL; /* or MOVE, MKACTIVITY, MERGE, M-SEARCH, MKCALENDAR */
                break;
            case 'N':
                mMethod = HTTP_NOTIFY;
                break;
            case 'O':
                mMethod = HTTP_OPTIONS;
                break;
            case 'P':
                mMethod = HTTP_POST;
                /* or PROPFIND|PROPPATCH|PUT|PATCH|PURGE */
                break;
            case 'R':
                mMethod = HTTP_REPORT; /* or REBIND */
                break;
            case 'S':
                mMethod = HTTP_SUBSCRIBE; /* or SEARCH, SOURCE */
                break;
            case 'T':
                mMethod = HTTP_TRACE;
                break;
            case 'U':
                mMethod = HTTP_UNLOCK; /* or UNSUBSCRIBE, UNBIND, UNLINK */
                break;
            default:
                mHttpError = HPE_INVALID_METHOD;
                goto GT_ERROR;
            }
            tmpstate = PS_REQ_METHOD;
            mState = tmpstate;
            pe = pp + 1; // steped
            break;
        }

        case PS_REQ_METHOD:
        {
            if (UNLIKELY(ch == '\0')) {
                mHttpError = HPE_INVALID_METHOD;
                goto GT_ERROR;
            }
            const s8* matcher = GHTTP_METHOD_STRS[mMethod];
            if (ch == ' ' && matcher[mIndex] == '\0') {
                tmpstate = PS_REQ_URL_PRE;
                mState = tmpstate;
                pe = pp + 1; // steped
                msgBegin();
                mMsg->setMethod(mMethod);
            } else if (ch == matcher[mIndex]) {
                // go
            } else if ((ch >= 'A' && ch <= 'Z') || ch == '-') {
                switch (mMethod << 16 | mIndex << 8 | ch) {
#define XX(meth, pos, ch, new_meth)                                                                                    \
    case (HTTP_##meth << 16 | pos << 8 | ch):                                                                          \
        mMethod = HTTP_##new_meth;                                                                                     \
        break;

                    XX(POST, 1, 'U', PUT)
                    XX(POST, 1, 'A', PATCH)
                    XX(POST, 1, 'R', PROPFIND)
                    XX(PUT, 2, 'R', PURGE)
                    XX(CONNECT, 1, 'H', CHECKOUT)
                    XX(CONNECT, 2, 'P', COPY)
                    XX(MKCOL, 1, 'O', MOVE)
                    XX(MKCOL, 1, 'E', MERGE)
                    XX(MKCOL, 1, '-', MSEARCH)
                    XX(MKCOL, 2, 'A', MKACTIVITY)
                    XX(MKCOL, 3, 'A', MKCALENDAR)
                    XX(SUBSCRIBE, 1, 'E', SEARCH)
                    XX(SUBSCRIBE, 1, 'O', SOURCE)
                    XX(REPORT, 2, 'B', REBIND)
                    XX(PROPFIND, 4, 'P', PROPPATCH)
                    XX(LOCK, 1, 'I', LINK)
                    XX(UNLOCK, 2, 'S', UNSUBSCRIBE)
                    XX(UNLOCK, 2, 'B', UNBIND)
                    XX(UNLOCK, 3, 'I', UNLINK)
#undef XX
                default:
                    mHttpError = HPE_INVALID_METHOD;
                    goto GT_ERROR;
                }
            } else {
                mHttpError = HPE_INVALID_METHOD;
                goto GT_ERROR;
            }
            ++mIndex;
            pe = pp + 1; // mState = not changed;
            break;
        }

        case PS_REQ_URL_PRE:
        {
            if (ch == ' ') {
                break;
            }
            tmpval.set(pp, 0);
            if (mMethod == HTTP_CONNECT) {
                tmpstate = PS_REQ_URL_HOST;
            }
            tmpstate = AppParseUrlChar(tmpstate, ch);
            if (UNLIKELY(tmpstate == PS_DEAD)) {
                mHttpError = HPE_INVALID_URL;
                goto GT_ERROR;
            }
            break;
        }

        case PS_REQ_URL_SCHEMA:
        case PS_REQ_URL_SLASH:
        case PS_REQ_URL_SLASH2:
        case PS_REQ_URL_HOST:
        {
            switch (ch) {
            case ' ': // No whitespace allowed here
            case CR:
            case LF:
                mHttpError = HPE_INVALID_URL;
                goto GT_ERROR;
            default:
                tmpstate = AppParseUrlChar(tmpstate, ch);
                if (UNLIKELY(tmpstate == PS_DEAD)) {
                    mHttpError = HPE_INVALID_URL;
                    goto GT_ERROR;
                }
            }
            break;
        }

        case PS_REQ_SERVER:
        case PS_REQ_SERVER_AT:
        case PS_REQ_URL_PATH:
        case PS_REQ_URL_QUERY:
        case PS_REQ_URL_QUERY_KEY:
        case PS_REQ_URL_FRAG:
        case PS_REQ_URL_FRAGMENT:
        {
            switch (ch) {
            case ' ':
                tmpstate = PS_REQ_HTTP_START;
                tmpval.mLen = 1; // tmpval.mLen>0表示需要回调
                break;
            case CR:
            case LF:
                mVersionMajor = 1;
                mVersionMinor = 0;
                tmpstate = (ch == CR ? PS_REQ_LINE_END : PS_HEAD_FIELD_PRE);
                tmpval.mLen = 1; // tmpval.mLen>0表示需要回调
                break;
            default:
                tmpstate = AppParseUrlChar(tmpstate, ch);
                if (UNLIKELY(tmpstate == PS_DEAD)) {
                    mHttpError = HPE_INVALID_URL;
                    goto GT_ERROR;
                }
            }
            if (1 == tmpval.mLen) {
                DASSERT(mHttpError == HPE_OK);
                tmpval.mLen = pp - tmpval.mData; // 重新计算长度
                mState = tmpstate;
                pe = pp + 1; // steped
                if (UNLIKELY(!mMsg->mURL.decode(tmpval.mData, tmpval.mLen))) {
                    mReadSize = nread;
                    mHttpError = HPE_CB_URL;
                    return (pe - data);
                }
                msgPath();
            }
            break;
        }

        case PS_REQ_HTTP_START:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                tmpstate = PS_REQ_H;
                break;
            case 'I':
                if (mMethod == HTTP_SOURCE) {
                    tmpstate = PS_REQ_I;
                    break;
                }
                /* fall through */
            default:
                mHttpError = HPE_INVALID_CONSTANT;
                goto GT_ERROR;
            }
            break;

        case PS_REQ_H:
            STRICT_CHECK(ch != 'T');
            tmpstate = PS_REQ_HT;
            break;

        case PS_REQ_HT:
            STRICT_CHECK(ch != 'T');
            tmpstate = PS_REQ_HTT;
            break;

        case PS_REQ_HTT:
            STRICT_CHECK(ch != 'P');
            tmpstate = PS_REQ_HTTP;
            break;

        case PS_REQ_I:
            STRICT_CHECK(ch != 'C');
            tmpstate = PS_REQ_IC;
            break;

        case PS_REQ_IC:
            STRICT_CHECK(ch != 'E');
            tmpstate = PS_REQ_HTTP; // Treat "ICE" as "HTTP"
            break;

        case PS_REQ_HTTP:
            STRICT_CHECK(ch != '/');
            tmpstate = PS_REQ_VER_MAJOR;
            break;

        case PS_REQ_VER_MAJOR:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            mVersionMajor = ch - '0';
            tmpstate = PS_REQ_VER_DOT;
            break;

        case PS_REQ_VER_DOT:
        {
            if (UNLIKELY(ch != '.')) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            tmpstate = PS_REQ_VER_MINOR;
            break;
        }

        case PS_REQ_VER_MINOR:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = HPE_INVALID_VERSION;
                goto GT_ERROR;
            }
            mVersionMinor = ch - '0';
            tmpstate = PS_REQ_VER_END;
            break;

        case PS_REQ_VER_END:
        {
            if (ch == CR) {
                tmpstate = PS_REQ_LINE_END;
                break;
            }
            if (ch == LF) {
                tmpstate = PS_HEAD_FIELD_PRE;
                mState = tmpstate;
                pe = pp + 1; // steped
                break;
            }
            mHttpError = HPE_INVALID_VERSION;
            goto GT_ERROR;
            break;
        }

        /* end of request line */
        case PS_REQ_LINE_END:
        {
            if (UNLIKELY(ch != LF)) {
                mHttpError = HPE_LF_EXPECTED;
                goto GT_ERROR;
            }
            tmpstate = PS_HEAD_FIELD_PRE;
            mState = tmpstate;
            pe = pp + 1; // steped
            break;
        }

        case PS_HEAD_FIELD_PRE:
        {
            if (ch == CR) {
                tmpstate = PS_HEAD_WILL_END;
                break;
            }
            if (ch == LF) {
                /* they might be just sending \n instead of \r\n so this would be
                 * the second \n to denote the end of headers*/
                tmpstate = PS_HEAD_WILL_END;
                goto GT_REPARSE;
            }

            lowc = TOKEN(ch);
            if (UNLIKELY(!lowc)) {
                mHttpError = HPE_INVALID_HEADER_TOKEN;
                goto GT_ERROR;
            }

            tmpkey.set(pp, 0);
            mIndex = 0;
            tmpstate = PS_HEAD_FIELD;

            switch (lowc) {
            case 'c':
                mHeaderState = EH_C;
                break;

            case 'p':
                mHeaderState = EH_CMP_PROXY_CONNECTION;
                break;

            case 't':
                mHeaderState = EH_CMP_TRANSFER_ENCODING;
                break;

            case 'u':
                mHeaderState = EH_CMP_UPGRADE;
                break;

            default:
                mHeaderState = EH_NORMAL;
                break;
            }
            break;
        }

        case PS_HEAD_FIELD:
        {
            const s8* start = pp;
            for (; pp != end; pp++) {
                ch = *pp;
                lowc = TOKEN(ch);
                if (!lowc) {
                    break;
                }
                switch (mHeaderState) {
                case EH_NORMAL:
                {
                    usz left = end - pp;
                    const s8* pe2 = pp + DMIN(left, GMAX_HEAD_SIZE);
                    while (pp + 1 < pe2 && TOKEN(pp[1])) {
                        pp++;
                    }
                    break;
                }

                case EH_C:
                    mIndex++;
                    mHeaderState = (lowc == 'o' ? EH_CO : EH_NORMAL);
                    break;

                case EH_CO:
                    mIndex++;
                    mHeaderState = (lowc == 'n' ? EH_CON : EH_NORMAL);
                    break;

                case EH_CON:
                    mIndex++;
                    switch (lowc) {
                    case 'n':
                        mHeaderState = EH_CMP_CONNECTION;
                        break;
                    case 't':
                        mHeaderState = EH_CMP_CONTENT;
                        break;
                    default:
                        mHeaderState = EH_NORMAL;
                        break;
                    }
                    break;

                case EH_CMP_CONNECTION:
                    mIndex++;
                    if (mIndex > sizeof(CONNECTION) - 1 || lowc != CONNECTION[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(CONNECTION) - 2) {
                        mHeaderState = EH_CONNECTION;
                    }
                    break;

                case EH_CMP_PROXY_CONNECTION:
                    mIndex++;
                    if (mIndex > sizeof(PROXY_CONNECTION) - 1 || lowc != PROXY_CONNECTION[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(PROXY_CONNECTION) - 2) {
                        mHeaderState = EH_CONNECTION;
                    }
                    break;

                case EH_CMP_CONTENT:
                    mIndex++;
                    if (mIndex == sizeof(CONTENT_) - 1) {
                        if ('t' == lowc) {
                            mHeaderState = EH_CMP_CONTENT_TYPE;
                        } else if ('l' == lowc) {
                            mHeaderState = EH_CMP_CONTENT_LEN;
                        } else if ('d' == lowc) {
                            mHeaderState = EH_CMP_CONTENT_DISP;
                        } else {
                            mHeaderState = EH_NORMAL;
                        }
                    } else if (mIndex > sizeof(CONTENT_) - 1 || lowc != CONTENT_[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    }
                    break;

                case EH_CMP_CONTENT_DISP:
                    mIndex++;
                    if (mIndex > sizeof(CONTENT_DISP) - 1 || lowc != CONTENT_DISP[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(CONTENT_DISP) - 2) {
                        mHeaderState = EH_CONTENT_DISP;
                    }
                    break;

                case EH_CMP_CONTENT_TYPE:
                    mIndex++;
                    if (mIndex > sizeof(CONTENT_TYPE) - 1 || lowc != CONTENT_TYPE[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(CONTENT_TYPE) - 2) {
                        mHeaderState = EH_CONTENT_TYPE;
                    }
                    break;

                case EH_CMP_CONTENT_LEN:
                    mIndex++;
                    if (mIndex > sizeof(CONTENT_LENGTH) - 1 || lowc != CONTENT_LENGTH[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(CONTENT_LENGTH) - 2) {
                        mHeaderState = EH_CONTENT_LEN;
                    }
                    break;

                case EH_CMP_TRANSFER_ENCODING:
                    mIndex++;
                    if (mIndex > sizeof(TRANSFER_ENCODING) - 1 || lowc != TRANSFER_ENCODING[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(TRANSFER_ENCODING) - 2) {
                        mHeaderState = EH_TRANSFER_ENCODING;
                        mUseTransferEncode = 1;
                    }
                    break;

                case EH_CMP_UPGRADE:
                    mIndex++;
                    if (mIndex > sizeof(UPGRADE) - 1 || lowc != UPGRADE[mIndex]) {
                        mHeaderState = EH_NORMAL;
                    } else if (mIndex == sizeof(UPGRADE) - 2) {
                        mHeaderState = EH_UPGRADE;
                    }
                    break;

                case EH_CONNECTION:
                case EH_CONTENT_TYPE:
                case EH_CONTENT_DISP:
                case EH_CONTENT_LEN:
                case EH_TRANSFER_ENCODING:
                case EH_UPGRADE:
                    if (ch != ' ') {
                        mHeaderState = EH_NORMAL;
                    }
                    break;

                default:
                    DASSERT(0 && "Unknown header_state");
                    break;
                }
            }

            if (pp == end) {
                --pp;
                COUNT_HEADER_SIZE(pp - start);
                break;
            }

            COUNT_HEADER_SIZE(pp - start);

            if (ch == ':') {
                tmpstate = PS_HEAD_2DOT;
                tmpkey.mLen = pp - tmpkey.mData;
                break;
            }

            mHttpError = HPE_INVALID_HEADER_TOKEN;
            goto GT_ERROR;
        }

        case PS_HEAD_2DOT:
        {
            if (ch == ' ' || ch == '\t') {
                break;
            }
            if (ch == CR) {
                tmpstate = PS_HEAD_VALUE_WILL_END_X;
                break;
            }
            if (ch == LF) {
                tmpstate = PS_HEAD_VALUE_END_X;
                break;
            }
            // break; fall through
        }

        case PS_HEAD_VALUE_PRE:
        {
            tmpval.set(pp, 0);
            tmpstate = PS_HEAD_VALUE;
            mIndex = 0;

            lowc = LOWER(ch);

            switch (mHeaderState) {
            case EH_UPGRADE:
                mFlags |= F_UPGRADE;
                mHeaderState = EH_NORMAL;
                break;

            case EH_TRANSFER_ENCODING:
                /* looking for 'Transfer-Encoding: chunked' */
                if ('c' == lowc) {
                    mHeaderState = EH_CMP_TRANSFER_ENCODING_CHUNCKED;
                } else {
                    mHeaderState = EH_CMP_TRANSFER_ENCODING_TOKEN;
                }
                break;

                /* Multi-value `Transfer-Encoding` header */
            case EH_CMP_TRANSFER_ENCODING_TOKEN_START:
                break;

            case EH_CONTENT_DISP:
            {
                StringView vals[6];
                s32 vcnt = (s32)DSIZEOF(vals);
                s32 vflags;
                pp = parseValue(pp, end, vals, vcnt, vflags);
                if (vflags & EHV_DONE) {
                    if (vcnt >= 4) {
                        // name of form
                        if (4 == vals[2].mLen && App4Char2S32(vals[2].mData) == App4Char2S32("name")) {
                            mFormNameLen = (u8)vals[3].mLen;
                            memcpy(mFormName, vals[3].mData, mFormNameLen);
                        }

                        // filename
                        if (vcnt >= 6) {
                            if (8 == vals[4].mLen && App4Char2S32(vals[4].mData) == App4Char2S32("file")) {
                                mFileNameLen = (u8)vals[5].mLen;
                                memcpy(mFileName, vals[5].mData, mFileNameLen);
                            }
                        }
                    }
                    mHeaderState = EH_NORMAL;
                }
                break;
            }
            case EH_CONTENT_TYPE:
            {
                if ('m' == lowc) {
                    StringView vals[6];
                    s32 vcnt = (s32)DSIZEOF(vals);
                    s32 vflags;
                    pp = parseValue(pp, end, vals, vcnt, vflags);
                    if ((vflags & EHV_DONE) && (vflags & EHV_CONTENT_TYPE_MULTI)) {
                        if (4 == vcnt && 8 == vals[2].mLen) {
                            if (vals[3].mLen > sizeof(mBoundary)) {
                                mHttpError = HPE_UNKNOWN;
                                goto GT_ERROR;
                            }
                            memcpy(mBoundary, vals[3].mData, vals[3].mLen);
                            mBoundaryLen = (u8)vals[3].mLen;
                            mHeaderState = EH_CONTENT_TYPE_BOUNDARY;
                        }
                    }
                } else {
                    mHeaderState = EH_NORMAL;
                }
                break;
            }
            case EH_CONTENT_LEN:
                if (UNLIKELY(!IS_NUM(ch))) {
                    mHttpError = HPE_INVALID_CONTENT_LENGTH;
                    goto GT_ERROR;
                }
                mContentLen = ch - '0';
                mHeaderState = EH_CONTENT_LEN_NUM;
                break;

                /* when obsolete line folding is encountered for content length
                 * continue to the PS_HEAD_VALUE state */
            case EH_CONTENT_LEN_WS:
                break;

            case EH_CONNECTION:
                /* looking for 'Connection: keep-alive' */
                if (lowc == 'k') {
                    mHeaderState = EH_CMP_CONNECTION_KEEP_ALIVE;
                    /* looking for 'Connection: close' */
                } else if (lowc == 'c') {
                    mHeaderState = EH_CMP_CONNECTION_CLOSE;
                } else if (lowc == 'u') {
                    mHeaderState = EH_CMP_CONNECTION_UPGRADE;
                } else {
                    mHeaderState = EH_CMP_CONNECTION_TOKEN;
                }
                break;

                /* Multi-value `Connection` header */
            case EH_CMP_CONNECTION_TOKEN_START:
                break;

            default:
                mHeaderState = EH_NORMAL;
                break;
            }
            break;
        }

        case PS_HEAD_VALUE:
        {
            const s8* start = pp;
            EHttpHeaderState h_state = (EHttpHeaderState)mHeaderState;
            for (; pp < end; pp++) {
                ch = *pp;
                if (ch == CR) {
                    tmpstate = PS_HEAD_VALUE_WILL_END;
                    mState = tmpstate;
                    pe = pp + 1; // steped
                    mHeaderState = h_state;
                    DASSERT(mHttpError == HPE_OK);
                    tmpval.mLen = pp - tmpval.mData;
                    mMsg->mHeadIn.add(tmpkey, tmpval);
                    break;
                }

                if (ch == LF) {
                    tmpstate = PS_HEAD_VALUE_WILL_END;
                    COUNT_HEADER_SIZE(pp - start);
                    mState = tmpstate;
                    pe = pp + 1; // steped
                    mHeaderState = h_state;
                    DASSERT(mHttpError == HPE_OK);
                    tmpval.mLen = pp - tmpval.mData;
                    mMsg->mHeadIn.add(tmpkey, tmpval);
                    goto GT_REPARSE;
                }

                if (!lenient && !IS_HEADER_CHAR(ch)) {
                    mHttpError = HPE_INVALID_HEADER_TOKEN;
                    goto GT_ERROR;
                }

                lowc = LOWER(ch);

                switch (h_state) {
                case EH_NORMAL:
                {
                    usz left = end - pp;
                    const s8* pe2 = pp + DMIN(left, GMAX_HEAD_SIZE);
                    for (; pp != pe2; pp++) {
                        ch = *pp;
                        if (ch == CR || ch == LF) {
                            --pp;
                            break;
                        }
                        if (!lenient && !IS_HEADER_CHAR(ch)) {
                            mHttpError = (HPE_INVALID_HEADER_TOKEN);
                            goto GT_ERROR;
                        }
                    }
                    if (pp == end) {
                        --pp;
                    }
                    break;
                }

                case EH_CONNECTION:
                case EH_TRANSFER_ENCODING:
                    DASSERT(0 && "Shouldn't get here.");
                    break;

                case EH_CONTENT_LEN:
                    if (ch == ' ') {
                        break;
                    }
                    h_state = EH_CONTENT_LEN_NUM;
                    // break; fall through

                case EH_CONTENT_LEN_NUM:
                {
                    if (ch == ' ') {
                        h_state = EH_CONTENT_LEN_WS;
                        break;
                    }

                    if (UNLIKELY(!IS_NUM(ch))) {
                        mHttpError = HPE_INVALID_CONTENT_LENGTH;
                        mHeaderState = h_state;
                        goto GT_ERROR;
                    }

                    u64 tlen = mContentLen * 10;
                    tlen += ch - '0';
                    /* Overflow? Test against a conservative limit for simplicity. */
                    if (UNLIKELY((GMAX_USIZE - 10) / 10 < tlen)) {
                        mHttpError = HPE_INVALID_CONTENT_LENGTH;
                        mHeaderState = h_state;
                        goto GT_ERROR;
                    }

                    mContentLen = tlen;
                    break;
                }

                case EH_CONTENT_LEN_WS:
                    if (ch == ' ') {
                        break;
                    }
                    mHttpError = HPE_INVALID_CONTENT_LENGTH;
                    mHeaderState = h_state;
                    goto GT_ERROR;

                case EH_CMP_TRANSFER_ENCODING_TOKEN_START:
                {
                    /* looking for 'Transfer-Encoding: chunked' */
                    if ('c' == lowc) {
                        h_state = EH_CMP_TRANSFER_ENCODING_CHUNCKED;
                    } else if (STRICT_TOKEN(lowc)) {
                        /* TODO(indutny): similar code below does this, but why?
                         * At the very least it seems to be inconsistent given that
                         * EH_CMP_TRANSFER_ENCODING_TOKEN does not check for
                         * `STRICT_TOKEN`
                         */
                        h_state = EH_CMP_TRANSFER_ENCODING_TOKEN;
                    } else if (lowc == ' ' || lowc == '\t') {
                        // Skip lws
                    } else {
                        h_state = EH_NORMAL;
                    }
                    break;
                }

                case EH_CMP_TRANSFER_ENCODING_CHUNCKED:
                    mIndex++;
                    if (mIndex > sizeof(CHUNKED) - 1 || lowc != CHUNKED[mIndex]) {
                        h_state = EH_CMP_TRANSFER_ENCODING_TOKEN;
                    } else if (mIndex == sizeof(CHUNKED) - 2) {
                        h_state = EH_TRANSFER_ENCODING_CHUNCKED;
                    }
                    break;

                case EH_CMP_TRANSFER_ENCODING_TOKEN:
                    if (ch == ',') {
                        h_state = EH_CMP_TRANSFER_ENCODING_TOKEN_START;
                        mIndex = 0;
                    }
                    break;

                case EH_CMP_CONNECTION_TOKEN_START:
                {
                    /* looking for 'Connection: keep-alive' */
                    if (lowc == 'k') {
                        h_state = EH_CMP_CONNECTION_KEEP_ALIVE;
                        /* looking for 'Connection: close' */
                    } else if (lowc == 'c') {
                        h_state = EH_CMP_CONNECTION_CLOSE;
                    } else if (lowc == 'u') {
                        h_state = EH_CMP_CONNECTION_UPGRADE;
                    } else if (STRICT_TOKEN(lowc)) {
                        h_state = EH_CMP_CONNECTION_TOKEN;
                    } else if (lowc == ' ' || lowc == '\t') {
                        /* Skip lws */
                    } else {
                        h_state = EH_NORMAL;
                    }
                    break;
                }
                case EH_CMP_CONNECTION_KEEP_ALIVE:
                    mIndex++;
                    if (mIndex > sizeof(KEEP_ALIVE) - 1 || lowc != KEEP_ALIVE[mIndex]) {
                        h_state = EH_CMP_CONNECTION_TOKEN;
                    } else if (mIndex == sizeof(KEEP_ALIVE) - 2) {
                        h_state = EH_CONNECTION_KEEP_ALIVE;
                    }
                    break;

                    // looking for 'Connection: close'
                case EH_CMP_CONNECTION_CLOSE:
                    mIndex++;
                    if (mIndex > sizeof(CLOSE) - 1 || lowc != CLOSE[mIndex]) {
                        h_state = EH_CMP_CONNECTION_TOKEN;
                    } else if (mIndex == sizeof(CLOSE) - 2) {
                        h_state = EH_CONNECTION_CLOSE;
                    }
                    break;

                case EH_CMP_CONNECTION_UPGRADE:
                    mIndex++;
                    if (mIndex > sizeof(UPGRADE) - 1 || lowc != UPGRADE[mIndex]) {
                        h_state = EH_CMP_CONNECTION_TOKEN;
                    } else if (mIndex == sizeof(UPGRADE) - 2) {
                        h_state = EH_CONNECTION_UPGRADE;
                    }
                    break;

                case EH_CMP_CONNECTION_TOKEN:
                    if (ch == ',') {
                        h_state = EH_CMP_CONNECTION_TOKEN_START;
                        mIndex = 0;
                    }
                    break;

                case EH_TRANSFER_ENCODING_CHUNCKED:
                    if (ch != ' ') {
                        h_state = EH_CMP_TRANSFER_ENCODING_TOKEN;
                    }
                    break;

                case EH_CONNECTION_KEEP_ALIVE:
                case EH_CONNECTION_CLOSE:
                case EH_CONNECTION_UPGRADE:
                {
                    if (ch == ',') {
                        if (h_state == EH_CONNECTION_KEEP_ALIVE) {
                            mFlags |= F_CONNECTION_KEEP_ALIVE;
                        } else if (h_state == EH_CONNECTION_CLOSE) {
                            mFlags |= F_CONNECTION_CLOSE;
                        } else if (h_state == EH_CONNECTION_UPGRADE) {
                            mFlags |= F_CONNECTION_UPGRADE;
                        }
                        h_state = EH_CMP_CONNECTION_TOKEN_START;
                        mIndex = 0;
                    } else if (ch != ' ') {
                        h_state = EH_CMP_CONNECTION_TOKEN;
                    }
                    break;
                }
                default:
                    tmpstate = (PS_HEAD_VALUE);
                    h_state = EH_NORMAL;
                    break;
                } // switch(h_state)
            }     // for in val
            mHeaderState = h_state;

            if (pp == end) {
                --pp;
            }
            COUNT_HEADER_SIZE(pp - start);
            break;
        }

        case PS_HEAD_VALUE_WILL_END:
        {
            if (UNLIKELY(ch != LF)) {
                mHttpError = HPE_LF_EXPECTED;
                goto GT_ERROR;
            }
            tmpstate = PS_HEAD_VALUE_END;
            break;
        }

        case PS_HEAD_VALUE_END:
        {
            if (ch == ' ' || ch == '\t') {
                if (mHeaderState == EH_CONTENT_LEN_NUM) {
                    // treat obsolete line folding as space
                    mHeaderState = EH_CONTENT_LEN_WS;
                }
                tmpstate = PS_HEAD_VALUE_PRE;
                goto GT_REPARSE;
            }

            /* finished the header */
            switch (mHeaderState) {
            case EH_CONTENT_LEN_NUM:
            {
                if (mFlags & F_CONTENTLENGTH) {
                    mHttpError = HPE_UNEXPECTED_CONTENT_LENGTH;
                    goto GT_ERROR;
                }
                mFlags |= F_CONTENTLENGTH;
                break;
            }
            case EH_CONTENT_TYPE_BOUNDARY:
            {
                if (mFlags & F_BOUNDARY) {
                    mHttpError = HPE_UNKNOWN;
                    goto GT_ERROR;
                }
                mFlags |= F_BOUNDARY;
                break;
            }
            case EH_CONNECTION_KEEP_ALIVE:
                mFlags |= F_CONNECTION_KEEP_ALIVE;
                break;
            case EH_CONNECTION_CLOSE:
                mFlags |= F_CONNECTION_CLOSE;
                break;
            case EH_TRANSFER_ENCODING_CHUNCKED:
                mFlags |= F_CHUNKED;
                break;
            case EH_CONNECTION_UPGRADE:
                mFlags |= F_CONNECTION_UPGRADE;
                break;
            default:
                break;
            }

            tmpstate = PS_HEAD_FIELD_PRE;
            mState = tmpstate;
            pe = pp; // steped
            goto GT_REPARSE;
        }

        case PS_HEAD_VALUE_WILL_END_X:
        {
            STRICT_CHECK(ch != LF);
            tmpstate = PS_HEAD_VALUE_END_X;
            break;
        }

        case PS_HEAD_VALUE_END_X:
        {
            if (ch == ' ' || ch == '\t') {
                tmpstate = PS_HEAD_2DOT;
                break;
            } else {
                switch (mHeaderState) {
                case EH_CONNECTION_KEEP_ALIVE:
                    mFlags |= F_CONNECTION_KEEP_ALIVE;
                    break;
                case EH_CONNECTION_CLOSE:
                    mFlags |= F_CONNECTION_CLOSE;
                    break;
                case EH_CONNECTION_UPGRADE:
                    mFlags |= F_CONNECTION_UPGRADE;
                    break;
                case EH_TRANSFER_ENCODING_CHUNCKED:
                    mFlags |= F_CHUNKED;
                    break;
                case EH_CONTENT_LEN:
                    /* do not allow empty content length */
                    mHttpError = HPE_INVALID_CONTENT_LENGTH;
                    goto GT_ERROR;
                    break;
                default:
                    break;
                }

                tmpval.set(pp, 0); // empty header value
                
                DASSERT(mHttpError == HPE_OK);
                if (tmpkey.mLen) {
                    mMsg->mHeadIn.add(tmpkey, tmpval);
                }
                tmpstate = PS_HEAD_FIELD_PRE;
                mState = tmpstate;
                pe = pp; // steped
                goto GT_REPARSE;
            }
        }

        case PS_HEAD_WILL_END:
        {
            STRICT_CHECK(ch != LF);
            if (mFlags & F_HEAD_DONE) {
                if (mFlags & F_BOUNDARY) {
                    tmpstate = PS_BODY_BOUNDARY;
                    mState = tmpstate;
                    pe = pp + 1; // steped
                    mValueState = EHV_BOUNDARY_BODY_PRE;
                    mFlags &= ~F_BOUNDARY_CMP;
                    break;
                }
                if (mFlags & (F_CHUNKED | F_TAILING)) {
                    // End of a chunked request
                    tmpstate = PS_MSG_DONE;
                    chunkDone();
                    mState = tmpstate;
                    pe = pp + 1; // steped
                    goto GT_REPARSE;
                }
            }

            /* Cannot use transfer-encoding and a content-length header together
               per the HTTP specification. (RFC 7230 Section 3.3.3) */
            if ((mUseTransferEncode == 1) && (mFlags & F_CONTENTLENGTH)) {
                /* Allow it for lenient parsing as long as `Transfer-Encoding` is
                 * not `chunked` or allow_length_with_encoding is set */
                if (mFlags & F_CHUNKED) {
                    if (!allow_chunked_length) {
                        mHttpError = HPE_UNEXPECTED_CONTENT_LENGTH;
                        goto GT_ERROR;
                    }
                } else if (!lenient) {
                    mHttpError = HPE_UNEXPECTED_CONTENT_LENGTH;
                    goto GT_ERROR;
                }
            }

            tmpstate = PS_HEAD_DONE;
            mState = tmpstate;
            pe = pp + 1; // steped

            /* Set this here so that headDone() can see it */
            if ((mFlags & F_UPGRADE) && (mFlags & F_CONNECTION_UPGRADE)) {
                /* For responses, "Upgrade: foo" and "Connection: upgrade" are
                 * mandatory only when it is a 101 Switching Protocols response,
                 * otherwise it is purely informational, to announce support.
                 */
                mUpgrade = (mType == EHTTP_REQUEST || mStatusCode == 101);
            } else {
                mUpgrade = (mMethod == HTTP_CONNECT);
            }

            /* Here we call the headers_complete callback. This is somewhat
             * different than other callbacks because if the user returns 1, we
             * will interpret that as saying that this message has no body. This
             * is needed for the annoying case of recieving a response to a HEAD
             * request.
             *
             * We'd like to use CALLBACK_NOTIFY_NOADVANCE() here but we cannot, so
             * we have to simulate it by handling a change in errno below.
             */
            switch (headDone()) {
            case 0:
                break;

            case 2:
                mUpgrade = 1;

                // fall through
            case 1:
                mFlags |= F_SKIPBODY;
                break;

            default:
                mReadSize = nread;
                mHttpError = HPE_CB_HeadersComplete;
                mState = tmpstate;
                return (pp - data); // Error
            }

            if (mHttpError != HPE_OK) {
                mReadSize = nread;
                mState = tmpstate;
                return (pp - data);
            }
            goto GT_REPARSE;
        }

        case PS_BODY_BOUNDARY:
        {
            pp = parseBoundBody(pp, end, tbody);
            tmpstate = (EPareState)(mState);
            if (UNLIKELY(mHttpError != HPE_OK)) {
                return (pp - data);
            }
            break;
        }
        case PS_HEAD_DONE:
        {
            STRICT_CHECK(ch != LF);
            DASSERT(0 == (F_HEAD_DONE & mFlags));
            mFlags |= F_HEAD_DONE;
            mReadSize = 0;
            nread = 0;

            s32 hasBody = (mFlags & (F_CHUNKED | F_BOUNDARY)) || (mContentLen > 0 && mContentLen != GMAX_USIZE);

            if (mUpgrade && (mMethod == HTTP_CONNECT || (mFlags & F_SKIPBODY) || !hasBody)) {
                // Exit, the rest of the message is in a different protocol
                tmpstate = (NEW_MESSAGE());
                msgEnd();
                mReadSize = nread;
                mState = tmpstate;
                return (pp - data + 1);
            }

            if (mFlags & F_SKIPBODY) {
                tmpstate = (NEW_MESSAGE());
                msgEnd();
                mState = tmpstate;
                pe = pp + 1; // steped
            } else if (mFlags & F_CHUNKED) {
                // chunked encoding, ignore Content-Length header
                tmpstate = PS_CHUNK_SIZE_START;
                mState = tmpstate;
                pe = pp + 1; // steped
            } else if (F_BOUNDARY & mFlags) {
                mIndex = 0;
                tmpstate = PS_BODY_BOUNDARY;
                mState = tmpstate;
                pe = pp + 1; // steped
                mValueState = EHV_BOUNDARY_CMP_PRE1;
                mFlags |= F_BOUNDARY_CMP;
            } else if (mUseTransferEncode == 1) {
                if (mType == EHTTP_REQUEST && !lenient) {
                    /* RFC 7230 3.3.3,
                     * If a Transfer-Encoding header field
                     * is present in a request and the chunked transfer coding is not
                     * the final encoding, the message body length cannot be determined
                     * reliably; the server MUST respond with the 400 (Bad RequestFD)
                     * status code and then close the connection.
                     */
                    mReadSize = nread;
                    mHttpError = HPE_INVALID_TRANSFER_ENCODING;
                    mState = tmpstate;
                    return (pp - data); /* Error */
                } else {
                    /* RFC 7230 3.3.3,
                     * If a Transfer-Encoding header field is present in a response and
                     * the chunked transfer coding is not the final encoding, the
                     * message body length is determined by reading the connection until
                     * it is closed by the server.
                     */
                    tmpstate = PS_BODY_EOF;
                    mState = tmpstate;
                    pe = pp + 1; // steped
                }
            } else {
                if (mContentLen == 0) {
                    /* Content-Length header given but zero: Content-Length: 0\r\n */
                    tmpstate = (NEW_MESSAGE());
                    msgEnd();
                } else if (mContentLen != GMAX_USIZE) {
                    tmpstate = PS_BODY_HAS_LEN;
                } else {
                    if (!needEOF()) {
                        // Assume content-length 0 - read the next
                        tmpstate = (NEW_MESSAGE());
                        msgEnd();
                        mState = tmpstate;
                    } else {
                        tmpstate = PS_BODY_EOF;
                    }
                }
                mState = tmpstate;
                pe = pp + 1; // steped
            }
            break;
        }

        case PS_BODY_HAS_LEN:
        {
            DASSERT(mContentLen != 0 && mContentLen != GMAX_USIZE);
            u64 to_read = end - pp;
            to_read = AppMin(mContentLen, to_read);
            /* The difference between advancing content_length and p is because
             * the latter will automaticaly advance on the next loop iteration.
             * Further, if content_length ends up at 0, we want to see the last
             * byte again for our message complete callback.
             */
            tbody.set(pp, to_read);
            mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
            mContentLen -= to_read;
            pe = pp + to_read; // steped
            pp = pe - 1;

            if (mContentLen == 0) {
                DASSERT(mHttpError == HPE_OK);
                tmpstate = PS_MSG_DONE;
                mState = tmpstate;
                /*
                 * The alternative to doing this is to wait for the next byte to
                 * trigger the data callback, just as in every other case. The
                 * problem with this is that this makes it difficult for the test
                 * harness to distinguish between complete-on-EOF and
                 * complete-on-length. It's not clear that this distinction is
                 * important for applications, but let's keep it for now.
                 */
                goto GT_REPARSE;
            }
            break;
        }

        // read until EOF
        case PS_BODY_EOF:
            tbody.set(pp, end - pp);
            mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
            pp = end - 1;
            pe = end; // steped
            break;

        case PS_MSG_DONE:
            tmpstate = (NEW_MESSAGE());
            msgEnd();
            mState = tmpstate;
            mReadSize = nread;
            if (mUpgrade) {
                // Exit, the rest of the message is in a different protocol
                return (pp - data + 1);
            }
            break;

        case PS_CHUNK_SIZE_START:
        {
            DASSERT(nread == 1);
            DASSERT(mFlags & F_CHUNKED);

            unhex_val = GMAP_UN_HEX[(u8)ch];
            if (UNLIKELY(unhex_val == 0xFF)) {
                mHttpError = HPE_INVALID_CHUNK_SIZE;
                goto GT_ERROR;
            }

            mContentLen = unhex_val;
            tmpstate = PS_CHUNK_SIZE;
            break;
        }

        case PS_CHUNK_SIZE:
        {
            DASSERT(mFlags & F_CHUNKED);
            if (ch == CR) {
                tmpstate = PS_CHUNK_SIZE_WILL_END;
                mState = tmpstate;
                pe = pp + 1; // steped
                break;
            }

            unhex_val = GMAP_UN_HEX[(u8)ch];
            if (unhex_val == 0xFF) {
                if (ch == ';' || ch == ' ') {
                    tmpstate = PS_CHUNK_SIZE_PARAM;
                    mState = tmpstate;
                    pe = pp + 1; // steped
                    break;
                }
                mHttpError = (HPE_INVALID_CHUNK_SIZE);
                goto GT_ERROR;
            }

            u64 tx = mContentLen * 16;
            tx += unhex_val;
            // Overflow? Test against a conservative limit for simplicity
            if (UNLIKELY((GMAX_USIZE - 16) / 16 < tx)) {
                mHttpError = (HPE_INVALID_CONTENT_LENGTH);
                goto GT_ERROR;
            }
            mContentLen = tx;
            break;
        }

        case PS_CHUNK_SIZE_PARAM:
        {
            DASSERT(mFlags & F_CHUNKED);
            // just ignore this shit. TODO check for overflow
            if (ch == CR) {
                tmpstate = PS_CHUNK_SIZE_WILL_END;
                mState = tmpstate;
                pe = pp + 1; // steped
                break;
            }
            break;
        }

        case PS_CHUNK_SIZE_WILL_END:
        {
            DASSERT(mFlags & F_CHUNKED);
            STRICT_CHECK(ch != LF);

            mReadSize = 0;
            nread = 0;
            if (mContentLen == 0) {
                mFlags |= F_TAILING;
                tmpstate = PS_HEAD_FIELD_PRE;
            } else {
                tmpstate = PS_CHUNK_DATA;
            }
            chunkHeadDone();
            mState = tmpstate;
            pe = pp + 1; // steped
            break;
        }

        case PS_CHUNK_DATA:
        {

            DASSERT(mFlags & F_CHUNKED);
            DASSERT(mContentLen != 0 && mContentLen != GMAX_USIZE);
            u64 to_read = end - pp;
            to_read = AppMin(mContentLen, to_read);
            tbody.set(pp, to_read);
            mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
            mContentLen -= to_read;
            pp += to_read - 1;
            if (mContentLen == 0) {
                tmpstate = PS_CHUNK_DATA_WILL_END;
                mState = tmpstate;
                pe = pp + 1; // steped
            }
            break;
        }

        case PS_CHUNK_DATA_WILL_END:
            DASSERT(mFlags & F_CHUNKED);
            DASSERT(mContentLen == 0);
            STRICT_CHECK(ch != CR);
            DASSERT(mHttpError == HPE_OK);
            tmpstate = PS_CHUNK_DATA_END;
            mState = tmpstate;
            pe = pp + 1; // steped
            break;

        case PS_CHUNK_DATA_END:
            DASSERT(mFlags & F_CHUNKED);
            STRICT_CHECK(ch != LF);
            mReadSize = 0;
            nread = 0;
            tmpstate = PS_CHUNK_SIZE_START;
            chunkMsg();
            mState = tmpstate;
            pe = pp + 1; // steped
            break;

        default:
            DASSERT(0 && "unhandled state");
            mHttpError = (HPE_INVALID_INTERNAL_STATE);
            goto GT_ERROR;
        } // switch
    }     // for

    // mState = tmpstate;  don't step here
    mReadSize = nread;
    return pe - data;


GT_ERROR:
    if (HPE_OK == mHttpError) {
        mHttpError = HPE_UNKNOWN;
    }
    mReadSize = nread;
    mState = tmpstate;
    msgError();
    return (pp - data);
}


const s8* HttpLayer::parseBoundBody(const s8* curr, const s8* end, StringView& tbody) {
    u32 cmpbyte = 0;
    bool mustCMP = mFlags & F_BOUNDARY_CMP;
    u32 vstat = mValueState;

    for (; curr < end; ++curr) {
        switch (vstat) {
        case EHV_BOUNDARY_BODY_PRE:
        {
            tbody.set(curr, 0);
            vstat = EHV_BOUNDARY_BODY;
            // break; fall
        }
        case EHV_BOUNDARY_BODY:
        {
            cmpbyte = 0;
            for (; curr < end; ++curr) {
                if (CR == *curr) {
                    cmpbyte = 1;
                    vstat = EHV_BOUNDARY_CMP_PRE;
                    break;
                }
                if (LF == *curr) {
                    ++cmpbyte;
                    vstat = EHV_BOUNDARY_CMP_PRE1;
                    break;
                }
            }
            // partly callback
            // curr - tbody.mData
            break;
        }
        case EHV_BOUNDARY_CMP_PRE:
        {
            if (LF == *curr) {
                ++cmpbyte;
                vstat = EHV_BOUNDARY_CMP_PRE1;
                break;
            } else {
                vstat = mustCMP ? EHV_ERROR : EHV_BOUNDARY_BODY;
            }
            break;
        }
        case EHV_BOUNDARY_CMP_PRE1:
            if ('-' == *curr) {
                ++cmpbyte;
                vstat = EHV_BOUNDARY_CMP_PRE2;
            } else {
                vstat = mustCMP ? EHV_ERROR : EHV_BOUNDARY_BODY;
            }
            break;

        case EHV_BOUNDARY_CMP_PRE2:
            if ('-' == *curr) {
                ++cmpbyte;
                mIndex = 0;
                vstat = EHV_BOUNDARY_CMP;
            } else {
                vstat = mustCMP ? EHV_ERROR : EHV_BOUNDARY_BODY;
            }
            break;

        case EHV_BOUNDARY_CMP:
        {
            if (mIndex == mBoundaryLen) {
                cmpbyte += mIndex;
                if (CR == *curr) {
                    ++cmpbyte;
                    vstat = EHV_BOUNDARY_CMP_DONE;
                    break;
                }
                if (LF == *curr) {
                    --curr;
                    vstat = EHV_BOUNDARY_CMP_DONE;
                    break;
                }
                if (mustCMP) {
                    vstat = EHV_ERROR;
                } else {
                    if ('-' == *curr) {
                        ++cmpbyte;
                        vstat = EHV_BOUNDARY_CMP_TAIL;
                    } else {
                        vstat = EHV_BOUNDARY_BODY;
                    }
                }
            } else if (mIndex < mBoundaryLen && *curr == mBoundary[mIndex]) {
                ++mIndex;
            } else {
                vstat = mustCMP ? EHV_ERROR : EHV_BOUNDARY_BODY;
            }
            break;
        }
        case EHV_BOUNDARY_CMP_DONE:
        {
            if (LF == *curr) {
                DASSERT(mHttpError == HPE_OK);
                if (tbody.mData) {
                    tbody.mLen = curr - tbody.mData - cmpbyte;
                    mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
                    tbody.set(nullptr, 0);
                }
                end = --curr;
                mState = PS_HEAD_FIELD_PRE;
            } else {
                vstat = mustCMP ? EHV_ERROR : EHV_BOUNDARY_BODY;
            }
            break;
        }
        case EHV_BOUNDARY_CMP_TAIL:
        {
            if ('-' == *curr) {
                ++cmpbyte;
                DASSERT(mHttpError == HPE_OK);
                if (tbody.mData) {
                    tbody.mLen = curr - tbody.mData - cmpbyte;
                    mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
                    tbody.set(nullptr, 0);
                }
                mState = PS_MSG_DONE;
                end = --curr;
            } else {
                vstat = EHV_BOUNDARY_BODY;
            }
            break;
        }
        case EHV_ERROR:
        {
            mHttpError = HPE_CB_Body;
            end = --curr;
            break;
        }
        default:
            DASSERT(false && "unknown status");
            mHttpError = HPE_CB_Body;
            end = --curr;
            break;
        } // switch
    }     // for

    mValueState = vstat;
    return curr;
}


bool HttpLayer::needEOF() const {
    if (mType == EHTTP_REQUEST) {
        return false;
    }

    /* See RFC 2616 section 4.4 */
    if (mStatusCode / 100 == 1 || /* 1xx e.g. Continue */
        mStatusCode == 204 ||     /* No Content */
        mStatusCode == 304 ||     /* Not Modified */
        mFlags & F_SKIPBODY) {    /* response to a HEAD request */
        return false;
    }

    /* RFC 7230 3.3.3, see `PS_HEAD_WILL_END` */
    if ((mUseTransferEncode == 1) && (mFlags & F_CHUNKED) == 0) {
        return true;
    }

    if ((mFlags & F_CHUNKED) || mContentLen != GMAX_USIZE) {
        return false;
    }

    return true;
}


bool HttpLayer::shouldKeepAlive() const {
    if (mVersionMajor > 0 && mVersionMinor > 0) {
        // HTTP/1.1
        if (mFlags & F_CONNECTION_CLOSE) {
            return false;
        }
    } else {
        // HTTP/1.0 or earlier
        if (mFlags & F_CONNECTION_KEEP_ALIVE) {
            return true;
        }
    }
    return !needEOF();
}


void HttpLayer::pauseParse(bool paused) {
    if (mHttpError == HPE_OK || mHttpError == HPE_PAUSED) {
        mHttpError = (paused ? HPE_PAUSED : HPE_OK);
    } else {
        DASSERT(0 && "Attempting to pause parser in error state");
    }
}


bool HttpLayer::isBodyFinal() const {
    return mState == PS_MSG_DONE;
}

const s8* HttpLayer::getErrStr(EHttpError it) {
#define HTTP_STRERROR_GEN(n, s) {"HPE_" #n, s},
    const static struct {
        const s8* name;
        const s8* description;
    } HttpStrErrorTab[] = {HTTP_ERRNO_MAP(HTTP_STRERROR_GEN)};
#undef HTTP_STRERROR_GEN

    return HttpStrErrorTab[it].description;
}


} // namespace net
} // namespace app
