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
#include "Net/HTTP/Website.h"
#include "Net/Acceptor.h"
#include "Loop.h"
#include "Timer.h"

namespace app {
namespace net {


HttpLayer::HttpLayer(EHttpParserType tp) :
    mPType(tp),
    mWebsite(nullptr),
    mMsg(nullptr),
    mHttpError(HPE_OK),
    mHTTPS(true) {
    clear();
}


HttpLayer::~HttpLayer() {
    //onClose();
}


s32 HttpLayer::get(const String& gurl) {
    mMsg->clear();
    if (EE_OK != mMsg->writeGet(gurl)) {
        return EE_ERROR;
    }

    mHTTPS = mMsg->getURL().isHttps();
    if (!mHTTPS) {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.getHandleTCP().setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    } else {
        mTCP.setClose(EHT_TCP_LINK, HttpLayer::funcOnClose, this);
        mTCP.setTime(HttpLayer::funcOnTime, 20 * 1000, 30 * 1000, -1);
    }

    StringView host = mMsg->getURL().getHost();
    NetAddress addr(mMsg->getURL().getPort());
    s8 shost[256];  //255, Maximum host name defined in RFC 1035
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
    return EE_OK;
}

s32 HttpLayer::post(const String& gurl) {
    return EE_ERROR;
}


void HttpLayer::msgBegin() {
    if (mMsg) {
        mHttpError = HPE_CB_MsgBegin;
        postClose();
        return; //error
    }

    mMsg = new HttpMsg(this);
    mMsg->mStationID = ES_INIT;
    msgStep();
}

void HttpLayer::msgEnd() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mStationID = ES_BODY_DONE;
        msgStep();
        mMsg->drop();
        mMsg = nullptr;
    }
}


void HttpLayer::msgPath() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mType = (EHttpParserType)mType;
        mMsg->mFlags = mFlags;
        mMsg->mStationID = ES_PATH;
        msgStep();
    }
}

s32 HttpLayer::headDone() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mFlags = mFlags;
        mMsg->mStationID = ES_HEAD;
        msgStep();
    }
    return 0;
}

void HttpLayer::chunkHeadDone() {
    DASSERT(mMsg);
    if (mMsg) {
        mMsg->mFlags = mFlags;
        mMsg->mStationID = ES_HEAD;
        msgStep();
    }
}

void HttpLayer::msgStep() {
    if (EE_OK != mWebsite->stepMsg(mMsg)) {
        postClose();
    }
}

void HttpLayer::msgBody() {
    if (mMsg) {
        mMsg->mStationID = ES_BODY;
        msgStep();
    }
}

void HttpLayer::msgError() {
    if (mMsg) {
        mMsg->mStationID = ES_ERROR;
        msgStep();
    }
}

void HttpLayer::chunkMsg() {
    if (mMsg) {
        mMsg->mStationID = ES_BODY;
        msgStep();
    }
}

void HttpLayer::chunkDone() {
    if (mMsg) {
        mMsg->mStationID = ES_BODY_DONE;
        mWebsite->stepMsg(mMsg);
        mMsg->drop();
        mMsg = nullptr;
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
    }
    return true;
}

bool HttpLayer::sendResp(HttpMsg* msg) {
    DASSERT(msg);

    if (msg->getRespStatus() > 0) {
        return true;
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
        return false;
    }
    msg->grab();
    msg->setRespStatus(1);
    return true;
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

    if (mMsg) {
        mMsg->mStationID = ES_CLOSE;
        mWebsite->stepMsg(mMsg);
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mWebsite) {
        Website* site = mWebsite;
        mWebsite = nullptr;
        site->unbind(this);
    } else {
        drop();
        Logger::log(ELL_ERROR, "HttpLayer::onClose>>null website");
    }
}


bool HttpLayer::onLink(RequestFD* it) {
    DASSERT(nullptr == mWebsite);
    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    RequestAccept& req = *(RequestAccept*)it;
    mWebsite = reinterpret_cast<Website*>(accp->getUser());
    DASSERT(mWebsite);
    mHTTPS = 1 == mWebsite->getConfig().mType;
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
    s32 ret = mHTTPS ? mTCP.open(req, nd, &mWebsite->getTlsContext()) : mTCP.getHandleTCP().open(req, nd);
    if (0 == ret) {
        mWebsite->bind(this);
        Logger::log(ELL_INFO, "HttpLayer::onLink>> [%s->%s]", mTCP.getRemote().getStr(), mTCP.getLocal().getStr());
    } else {
        mWebsite = nullptr;
        RequestFD::delRequest(nd);
        Logger::log(ELL_ERROR, "HttpLayer::onLink>> [%s->%s], ecode=%d", mTCP.getRemote().getStr(), mTCP.getLocal().getStr(), ret);
    }
    return EE_OK == ret;
}

void HttpLayer::onConnect(RequestFD* it) {
    if (0 == it->mError && sendReq()) {
        it->mCall = HttpLayer::funcOnRead;
        if (EE_OK == readIF(it)) {
            return;
        }
    }
    RequestFD::delRequest(it);
}

void HttpLayer::onWrite(RequestFD* it, HttpMsg* msg) {
    if (EE_OK != it->mError) {
        Logger::log(ELL_ERROR, "HttpLayer::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    } else {
        msg->setRespStatus(0);
        msg->getCacheOut().commitHead(static_cast<s32>(it->mUsed));
        if (msg->getCacheOut().getSize() > 0) {
            sendResp(msg);
        } else {
            if (EE_OK != mWebsite->stepMsg(msg)) {
                postClose();
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
        parseBuf(dat, 0); //make the http-msg-finish callback
    } else {
        ssz parsed = 0;
        ssz stepsz;
        while (datsz > 0 && 0 == mHttpError) {
            stepsz = parseBuf(dat + parsed, datsz);
            parsed += stepsz;
            if (stepsz < datsz) {
                break; //leftover
            }
            datsz -= stepsz;
        }
        it->clearData((u32)parsed);

        if (0 == it->getWriteSize()) {
            //可能受到超长header攻击或其它错误
            Logger::logError("HttpLayer::onRead>>remote=%s, msg overflow", mTCP.getRemote().getStr());
            postClose();
        } else {
            if (0 == mHttpError && EE_OK == readIF(it)) {
                return;
            }
            Logger::logError("HttpLayer::onRead>>remote=%s, parser err=%d=%s", mTCP.getRemote().getStr(), mHttpError, getErrStr());
        }
    }

    //del req
#if defined(DDEBUG)
    printf("HttpLayer::onRead>>del req, read=%lld, ecode=%d, parser err=%d=%s\n", datsz, it->mError, mHttpError, getErrStr());
#endif
    RequestFD::delRequest(it);
}


void HttpLayer::postClose() {
    mHTTPS ? mTCP.launchClose() : mTCP.getHandleTCP().launchClose();
}











/////////////////////////
u32 HttpLayer::GMAX_HEAD_SIZE = HTTP_MAX_HEADER_SIZE;

#ifndef ULLONG_MAX
#define ULLONG_MAX ((u64) -1) /* 2^64-1 */
#endif

#define COUNT_HEADER_SIZE(V)                                         \
do {                                                                 \
  nread += (u32)(V);                                                 \
  if (UNLIKELY(nread > GMAX_HEAD_SIZE)) {                            \
    mHttpError = (HPE_HEADER_OVERFLOW);                              \
    goto GT_ERROR;                                                   \
  }                                                                  \
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
#define MLT_PART_FORM  "multipart/form-data"


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
    'x', 'y', 'z', 0, '|', 0, '~', 0
};


static const u8 GMAP_UN_HEX[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 10, 11, 12, 13, 14, 15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 10, 11, 12, 13, 14, 15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


#if DHTTP_PARSE_STRICT
# define T(v) 0
#else
# define T(v) v
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
    1 | 2 | 4 | 8 | 16 | 32 | 64 | 0, };

#undef T
//DHTTP_PARSE_STRICT

enum EPareState {
    s_dead = 1 //important that this is > 0

    , s_start_resp_or_req
    , s_res_or_resp_H
    , s_start_resp //response start
    , s_res_H
    , s_res_HT
    , s_res_HTT
    , s_res_HTTP
    , s_res_http_major
    , s_res_http_dot
    , s_res_http_minor
    , s_res_http_end
    , s_res_first_status_code
    , s_res_status_code
    , s_res_status_start
    , s_res_status
    , s_res_line_almost_done

    , s_start_req //request start

    , s_req_method
    , s_req_spaces_before_url
    , s_req_schema
    , s_req_schema_slash
    , s_req_schema_slash2
    , s_req_server_start
    , s_req_server
    , s_req_server_with_at
    , s_req_path
    , s_req_query_string_start
    , s_req_query_string
    , s_req_fragment_start
    , s_req_fragment
    , s_req_http_start
    , s_req_http_H
    , s_req_http_HT
    , s_req_http_HTT
    , s_req_http_HTTP
    , s_req_http_I
    , s_req_http_IC
    , s_req_http_major
    , s_req_http_dot
    , s_req_http_minor
    , s_req_http_end
    , s_req_line_almost_done

    , s_header_field_start
    , s_header_field
    , s_header_value_discard_ws             //丢弃head field & value 之间的间隔字符
    , s_header_value_discard_ws_almost_done
    , s_header_value_discard_lws            //丢弃value末尾的间隔字符
    , s_header_value_start
    , s_header_value
    , s_header_value_lws

    , s_header_almost_done

    , s_chunk_size_start
    , s_chunk_size
    , s_chunk_parameters
    , s_chunk_size_almost_done

    , s_headers_almost_done
    , s_headers_done
    /*@note 's_headers_done' must be the last 'header' state. All
     * states beyond this must be 'body' states. It is used for overflow
     * checking. See the @note PARSING_HEADER.
     */

    , s_chunk_data              //只要有数据就需要回调，避免缓存区满时仍然不能触发回调
    , s_chunk_data_almost_done
    , s_chunk_data_done

    , s_boundary_body          //只要有数据就需要回调，避免缓存区满时仍然不能触发回调

    , s_body_identity          //content_length有效
    , s_body_identity_eof      //无content-length/chunck等信息，靠断开连接表示body结束

    , s_message_done
};

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

    //for value
    EHV_START_NAME,
    EHV_PRE_NAME,
    EHV_NAME,
    EHV_PRE_VALUE,
    EHV_VALUE,

    //for boundary body
    EHV_BOUNDARY_CMP_PRE,     //\n in body
    EHV_BOUNDARY_CMP_PRE1,    //first "-"
    EHV_BOUNDARY_CMP_PRE2,    //2nd "-"
    EHV_BOUNDARY_CMP,         //compare multipart's boundary
    EHV_BOUNDARY_CMP_DONE,
    EHV_BOUNDARY_CMP_TAIL,    //compare the tail "--"
    EHV_BOUNDARY_BODY_PRE,    //body start
    EHV_BOUNDARY_BODY,

    //for flags
    EHV_MATCH_TYPE_MULTI = 0x10000000,    //Content-Type: multipart/
    EHV_CONTENT_TYPE_MULTI = 0x20000000,  //Content-Type: multipart/...
    EHV_DONE = 0x80000000,
    //EHV_CONTENT_DISP = 0x20000000       //Content-Disposition
};

enum EHttpHeaderState {
    h_general = 0
    , h_C
    , h_CO
    , h_CON

    , h_matching_connection
    , h_matching_proxy_connection
    , h_matching_content_           //Content-
    , h_matching_content_type       //Content-Type
    , h_matching_content_disp       //content-disposition
    , h_matching_content_length
    , h_matching_transfer_encoding
    , h_matching_upgrade

    , h_connection
    , h_content_type        //Content-Type
    , h_content_disp        //content-disposition
    , h_content_length
    , h_content_length_num
    , h_content_length_ws
    , h_transfer_encoding
    , h_upgrade

    , h_matching_transfer_encoding_token_start
    , h_matching_transfer_encoding_chunked
    , h_matching_transfer_encoding_token

    , h_matching_connection_token_start
    , h_matching_connection_keep_alive
    , h_matching_connection_close
    , h_matching_connection_upgrade
    , h_matching_connection_token

    , h_transfer_encoding_chunked
    , h_connection_keep_alive
    , h_connection_close
    , h_connection_upgrade
};

enum EHttpHostState {
    s_http_host_dead = 1
    , s_http_userinfo_start
    , s_http_userinfo
    , s_http_host_start
    , s_http_host_v6_start
    , s_http_host
    , s_http_host_v6
    , s_http_host_v6_end
    , s_http_host_v6_zone_start
    , s_http_host_v6_zone
    , s_http_host_port_start
    , s_http_host_port
};

/* Macros for character classes; depends on strict-mode  */
#define CR                  '\r'
#define LF                  '\n'
#define LOWER(c)            (u8)(c | 0x20)
#define IS_ALPHA(c)         (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c)      (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c)           (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))
#define IS_MARK(c)          ((c) == '-' || (c) == '_' || (c) == '.' || \
  (c) == '!' || (c) == '~' || (c) == '*' || (c) == '\'' || (c) == '(' || \
  (c) == ')')

#define IS_USERINFO_CHAR(c) (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || \
  (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+' || \
  (c) == '$' || (c) == ',')

#define STRICT_TOKEN(c)     ((c == ' ') ? 0 : GTokens[(u8)c])

#if DHTTP_PARSE_STRICT
#define TOKEN(c)            STRICT_TOKEN(c)
#define IS_URL_CHAR(c)      (AppIsBitON(G_NORMAL_URL_CHAR, (u8)c))
#define IS_HOST_CHAR(c)     (IS_ALPHANUM(c) || (c) == '.' || (c) == '-')
#else
#define TOKEN(c)            GTokens[(u8)c]
#define IS_URL_CHAR(c)                                                         \
  (AppIsBitON(G_NORMAL_URL_CHAR, (u8)c) || ((c) & 0x80))
#define IS_HOST_CHAR(c)                                                        \
  (IS_ALPHANUM(c) || (c) == '.' || (c) == '-' || (c) == '_')
#endif

//Verify that a char is a valid visible (printable) US-ASCII character or %x80-FF
#define IS_HEADER_CHAR(ch) (ch == CR || ch == LF || ch == 9 || ((u8)ch > 31 && ch != 127))


#if DHTTP_PARSE_STRICT
# define STRICT_CHECK(cond)                                          \
do {                                                                 \
  if (cond) {                                                        \
    mHttpError = HPE_STRICT;                                         \
    goto GT_ERROR;                                                   \
  }                                                                  \
} while (0)

#define NEW_MESSAGE() (shouldKeepAlive() ? (mType == EHTTP_REQUEST ? s_start_req : s_start_resp) : s_dead)

#else
# define STRICT_CHECK(cond)
# define NEW_MESSAGE() (mType == EHTTP_REQUEST ? s_start_req : s_start_resp)
#endif


 /* Map errno values to strings for human-readable output */
#define HTTP_STRERROR_GEN(n, s) { "HPE_" #n, s },
static struct {
    const s8* name;
    const s8* description;
} HttpStrErrorTab[] = {
    HTTP_ERRNO_MAP(HTTP_STRERROR_GEN)
};
#undef HTTP_STRERROR_GEN



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
        return s_dead;
    }

#if DHTTP_PARSE_STRICT
    if (ch == '\t' || ch == '\f') {
        return s_dead;
    }
#endif

    switch (s) {
    case s_req_spaces_before_url:
        /* Proxied requests are followed by scheme of an absolute URI (alpha).
         * All methods except CONNECT are followed by '/' or '*'.
         */
        if (ch == '/' || ch == '*') {
            return s_req_path;
        }
        if (IS_ALPHA(ch)) {
            return s_req_schema;
        }
        break;

    case s_req_schema:
        if (IS_ALPHA(ch)) {
            return s;
        }
        if (ch == ':') {
            return s_req_schema_slash;
        }
        break;

    case s_req_schema_slash:
        if (ch == '/') {
            return s_req_schema_slash2;
        }
        break;

    case s_req_schema_slash2:
        if (ch == '/') {
            return s_req_server_start;
        }
        break;

    case s_req_server_with_at:
        if (ch == '@') {
            return s_dead;
        }

        /* fall through */
    case s_req_server_start:
    case s_req_server:
        if (ch == '/') {
            return s_req_path;
        }
        if (ch == '?') {
            return s_req_query_string_start;
        }
        if (ch == '@') {
            return s_req_server_with_at;
        }
        if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
            return s_req_server;
        }
        break;

    case s_req_path:
        if (IS_URL_CHAR(ch)) {
            return s;
        }
        switch (ch) {
        case '?':
            return s_req_query_string_start;
        case '#':
            return s_req_fragment_start;
        }
        break;

    case s_req_query_string_start:
    case s_req_query_string:
        if (IS_URL_CHAR(ch)) {
            return s_req_query_string;
        }
        switch (ch) {
        case '?':
            /* allow extra '?' in query string */
            return s_req_query_string;
        case '#':
            return s_req_fragment_start;
        }
        break;

    case s_req_fragment_start:
        if (IS_URL_CHAR(ch)) {
            return s_req_fragment;
        }
        switch (ch) {
        case '?':
            return s_req_fragment;
        case '#':
            return s;
        }
        break;

    case s_req_fragment:
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
    return s_dead;
}

//not strict check
static EPareState AppParseUrlChar2(EPareState s, const s8 ch) {
    if (ch == ' ' || ch == '\r' || ch == '\n') {
        return s_dead;
    }

#if DHTTP_PARSE_STRICT
    if (ch == '\t' || ch == '\f') {
        return s_dead;
    }
#endif

    switch (s) {
    case s_req_spaces_before_url:
        /* Proxied requests are followed by scheme of an absolute URI (alpha).
         * All methods except CONNECT are followed by '/' or '*'.
         */
        if (ch == '/' || ch == '*') {
            return s_req_path;
        }
        if (IS_ALPHA(ch)) {
            return s_req_schema;
        }
        break;

    case s_req_schema:
        if (IS_ALPHA(ch)) {
            return s;
        }
        if (ch == ':') {
            return s_req_schema_slash;
        }
        break;

    case s_req_schema_slash:
        if (ch == '/') {
            return s_req_schema_slash2;
        }
        break;

    case s_req_schema_slash2:
        if (ch == '/') {
            return s_req_server_start;
        }
        break;

    case s_req_server_with_at:
        if (ch == '@') {
            return s_dead;
        }

        /* fall through */
    case s_req_server_start:
    case s_req_server:
        if (ch == '/') {
            return s_req_path;
        }
        if (ch == '?') {
            return s_req_query_string_start;
        }
        if (ch == '@') {
            return s_req_server_with_at;
        }
        if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
            return s_req_server;
        }
        break;

    case s_req_path:
        switch (ch) {
        case '?':
            return s_req_query_string_start;
        case '#':
            return s_req_fragment_start;
        default:
            return s;
        }
        break;

    case s_req_query_string_start:
    case s_req_query_string:
        switch (ch) {
        case '?':
            // allow extra '?' in query string
            return s_req_query_string;
        case '#':
            return s_req_fragment_start;
        default:
            return s_req_query_string;
        }
        break;

    case s_req_fragment_start:
        switch (ch) {
        case '?':
            return s_req_fragment;
        case '#':
            return s;
        default:
            return s_req_fragment;
        }
        break;

    case s_req_fragment:
        switch (ch) {
        case '?':
        case '#':
        default:
            return s;
        }
        break;

    default:
        break;
    }

    return s_dead;
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
                kval.set(curr, 0); //empty value
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
            //break; fall

        case EHV_NAME:
            if (' ' == *curr || '=' == *curr) {
                vstat = EHV_PRE_VALUE;
                break;
            }
            ++kname.mLen;
            if ('/' == *curr) {
                vstat = EHV_PRE_VALUE;
            } else if (';' == *curr) {
                kval.set(curr, 0); //empty value
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
            //no break

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

        default:break;
        }//switch
    }//for


    GT_RET:
    omax = used;
    return curr;
}


void HttpLayer::clear() {
    mType = mPType;
    mIndex = 0;
    mVersionMajor = 0;
    mVersionMinor = 0;
    mStatusCode = 0;
    mMethod = HTTP_GET;
    mState = (mType == EHTTP_REQUEST ? s_start_req : (mType == EHTTP_RESPONSE ? s_start_resp : s_start_resp_or_req));
    mUpgrade = 0;
    mUseTransferEncode = 0;
    mAllowChunkedLen = 0;
    mLenientHeaders = 0;
    if (EHTTP_REQUEST == mPType) {
        msgBegin();
    }
    reset();
}

void HttpLayer::reset() {
    mHttpError = HPE_OK;
    mReadSize = 0;
    mFlags = 0;
    mUseTransferEncode = 0;
    mContentLen = ULLONG_MAX;
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

    s8 ch; //raw byte
    s8 c;  //low char of ch
    u8 unhex_val;
    StringView headkey;
    StringView headval;
    StringView tbody;
    StringView tstat;
    StringView turl;
    const s8* p = data;
    const s8* const end = data + len;
    EPareState p_state = (EPareState)mState;
    const u32 lenient = mLenientHeaders;
    const u32 allow_chunked_length = mAllowChunkedLen;

    u32 nread = mReadSize;

    if (len == 0) {
        switch (p_state) {
        case s_body_identity_eof:
            msgEnd();
            return 0;

        case s_dead:
        case s_start_resp_or_req:
        case s_start_resp:
        case s_start_req:
            return 0;

        default:
            mReadSize = nread;
            mHttpError = HPE_INVALID_EOF_STATE;
            return 1;
        }
    }

    if (p_state == s_header_field_start) {
        headkey.set(data, 0);
    }

    switch (p_state) {
    case s_req_path:
    case s_req_schema:
    case s_req_schema_slash:
    case s_req_schema_slash2:
    case s_req_server_start:
    case s_req_server:
    case s_req_server_with_at:
    case s_req_query_string_start:
    case s_req_query_string:
    case s_req_fragment_start:
    case s_req_fragment:
        turl.set(p, 0);
        break;
    case s_res_status:
        tstat.set(p, 0);
        break;
    default:
        break;
    }//switch

    for (p = data; p != end; ++p) {
        ch = *p;
        if (p_state <= s_headers_done) {//@note PARSING_HEADER
            COUNT_HEADER_SIZE(1);
        }

        GT_REPARSE: //recheck current byte
        switch (p_state) {
        case s_dead:
            /* this state is used after a 'Connection: close' message
             * the parser will error out if it reads another message
             */
            if (LIKELY(ch == CR || ch == LF)) {
                break;
            }
            mHttpError = (HPE_CLOSED_CONNECTION);
            goto GT_ERROR;

        case s_start_resp_or_req:
        {
            if (ch == CR || ch == LF) {
                break;
            }
            reset();
            if (ch == 'H') {
                p_state = (s_res_or_resp_H);
                msgBegin();
                mState = p_state;
                if (HPE_OK != mHttpError) {
                    return p - data + 1;
                }
            } else {
                mType = EHTTP_REQUEST;
                p_state = (s_start_req);
                goto GT_REPARSE;
            }
            break;
        }

        case s_res_or_resp_H:
            if (ch == 'T') {
                mType = EHTTP_RESPONSE;
                p_state = (s_res_HT);
            } else {
                if (UNLIKELY(ch != 'E')) {
                    mHttpError = (HPE_INVALID_CONSTANT);
                    goto GT_ERROR;
                }
                mType = EHTTP_REQUEST;
                mMethod = HTTP_HEAD;
                mIndex = 2;
                p_state = (s_req_method);
            }
            break;

        case s_start_resp:
        {
            if (ch == CR || ch == LF) {
                break;
            }
            reset();
            if (ch == 'H') {
                p_state = s_res_H;
            } else {
                mHttpError = (HPE_INVALID_CONSTANT);
                goto GT_ERROR;
            }

            msgBegin();
            mState = p_state;
            if (HPE_OK != mHttpError) {
                return p - data + 1;
            }
            break;
        }

        case s_res_H:
            STRICT_CHECK(ch != 'T');
            p_state = (s_res_HT);
            break;

        case s_res_HT:
            STRICT_CHECK(ch != 'T');
            p_state = (s_res_HTT);
            break;

        case s_res_HTT:
            STRICT_CHECK(ch != 'P');
            p_state = (s_res_HTTP);
            break;

        case s_res_HTTP:
            STRICT_CHECK(ch != '/');
            p_state = (s_res_http_major);
            break;

        case s_res_http_major:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            mVersionMajor = ch - '0';
            p_state = (s_res_http_dot);
            break;

        case s_res_http_dot:
        {
            if (UNLIKELY(ch != '.')) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            p_state = (s_res_http_minor);
            break;
        }

        case s_res_http_minor:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            mVersionMinor = ch - '0';
            p_state = (s_res_http_end);
            break;

        case s_res_http_end:
        {
            if (UNLIKELY(ch != ' ')) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            p_state = (s_res_first_status_code);
            break;
        }

        case s_res_first_status_code:
        {
            if (!IS_NUM(ch)) {
                if (ch == ' ') {
                    break;
                }
                mHttpError = HPE_INVALID_STATUS;
                goto GT_ERROR;
            }
            mStatusCode = ch - '0';
            p_state = s_res_status_code;
            break;
        }

        case s_res_status_code:
        {
            if (!IS_NUM(ch)) {
                switch (ch) {
                case ' ':
                    p_state = (s_res_status_start);
                    break;
                case CR:
                case LF:
                    p_state = (s_res_status_start);
                    goto GT_REPARSE;
                    break;
                default:
                    mHttpError = (HPE_INVALID_STATUS);
                    goto GT_ERROR;
                }
                break;
            }

            mStatusCode *= 10;
            mStatusCode += ch - '0';

            if (UNLIKELY(mStatusCode > 999)) {
                mHttpError = (HPE_INVALID_STATUS);
                goto GT_ERROR;
            }
            break;
        }

        case s_res_status_start:
        {
            mMsg->setStatus(mStatusCode);
            if (!tstat.mData) {
                tstat.set(p, 0);
            }
            p_state = (s_res_status);
            mIndex = 0;
            if (ch == CR || ch == LF) {
                goto GT_REPARSE;
            }
            break;
        }

        case s_res_status:
            if (ch == CR || ch == LF) {
                p_state = (CR == ch ? s_res_line_almost_done : s_header_field_start);
                //CALLBACK_DATA(Status);
                DASSERT(mHttpError == HPE_OK);
                if (tstat.mData) {
                    if (LIKELY(mMsg)) {
                        tstat.mLen = p - tstat.mData;
                        mState = p_state;
                        mMsg->setBrief(tstat.mData, tstat.mLen);
                    }
                    tstat.set(nullptr, 0);
                }
                break;
            }
            break;

        case s_res_line_almost_done:
            STRICT_CHECK(ch != LF);
            p_state = (s_header_field_start);
            break;

        case s_start_req:
        {
            if (ch == CR || ch == LF) {
                break;
            }
            reset();
            if (UNLIKELY(!IS_ALPHA(ch))) {
                mHttpError = (HPE_INVALID_METHOD);
                goto GT_ERROR;
            }
            mMethod = (enum EHttpMethod) 0;
            mIndex = 1;
            switch (ch) {
            case 'A': mMethod = HTTP_ACL; break;
            case 'B': mMethod = HTTP_BIND; break;
            case 'C': mMethod = HTTP_CONNECT; /* or COPY, CHECKOUT */ break;
            case 'D': mMethod = HTTP_DELETE; break;
            case 'G': mMethod = HTTP_GET; break;
            case 'H': mMethod = HTTP_HEAD; break;
            case 'L': mMethod = HTTP_LOCK; /* or LINK */ break;
            case 'M': mMethod = HTTP_MKCOL; /* or MOVE, MKACTIVITY, MERGE, M-SEARCH, MKCALENDAR */ break;
            case 'N': mMethod = HTTP_NOTIFY; break;
            case 'O': mMethod = HTTP_OPTIONS; break;
            case 'P': mMethod = HTTP_POST;
                /* or PROPFIND|PROPPATCH|PUT|PATCH|PURGE */
                break;
            case 'R': mMethod = HTTP_REPORT; /* or REBIND */ break;
            case 'S': mMethod = HTTP_SUBSCRIBE; /* or SEARCH, SOURCE */ break;
            case 'T': mMethod = HTTP_TRACE; break;
            case 'U': mMethod = HTTP_UNLOCK; /* or UNSUBSCRIBE, UNBIND, UNLINK */ break;
            default:
                mHttpError = (HPE_INVALID_METHOD);
                goto GT_ERROR;
            }
            p_state = s_req_method;
            msgBegin();
            mState = p_state;
            if (HPE_OK != mHttpError) {
                return p - data + 1;
            }
            break;
        }

        case s_req_method:
        {
            const s8* matcher;
            if (UNLIKELY(ch == '\0')) {
                mHttpError = (HPE_INVALID_METHOD);
                goto GT_ERROR;
            }

            matcher = GHTTP_METHOD_STRS[mMethod];
            if (ch == ' ' && matcher[mIndex] == '\0') {
                p_state = (s_req_spaces_before_url);
                mMsg->setMethod(mMethod);
            } else if (ch == matcher[mIndex]) {
                ; /* nada */
            } else if ((ch >= 'A' && ch <= 'Z') || ch == '-') {

                switch (mMethod << 16 | mIndex << 8 | ch) {
#define XX(meth, pos, ch, new_meth) \
            case (HTTP_##meth << 16 | pos << 8 | ch): \
              mMethod = HTTP_##new_meth; break;

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
                    mHttpError = (HPE_INVALID_METHOD);
                    goto GT_ERROR;
                }
            } else {
                mHttpError = (HPE_INVALID_METHOD);
                goto GT_ERROR;
            }
            ++mIndex;
            break;
        }

        case s_req_spaces_before_url:
        {
            if (ch == ' ') {
                break;
            }
            if (!turl.mData) {
                turl.set(p, 0);
            }
            if (mMethod == HTTP_CONNECT) {
                p_state = (s_req_server_start);
            }
            p_state = (AppParseUrlChar(p_state, ch));
            if (UNLIKELY(p_state == s_dead)) {
                mHttpError = (HPE_INVALID_URL);
                goto GT_ERROR;
            }
            break;
        }

        case s_req_schema:
        case s_req_schema_slash:
        case s_req_schema_slash2:
        case s_req_server_start:
        {
            switch (ch) {
            case ' ': //No whitespace allowed here
            case CR:
            case LF:
                mHttpError = (HPE_INVALID_URL);
                goto GT_ERROR;
            default:
                p_state = (AppParseUrlChar(p_state, ch));
                if (UNLIKELY(p_state == s_dead)) {
                    mHttpError = (HPE_INVALID_URL);
                    goto GT_ERROR;
                }
            }
            break;
        }

        case s_req_server:
        case s_req_server_with_at:
        case s_req_path:
        case s_req_query_string_start:
        case s_req_query_string:
        case s_req_fragment_start:
        case s_req_fragment:
        {
            switch (ch) {
            case ' ':
                p_state = (s_req_http_start);
                turl.mLen = 1; //turl.mLen>0表示需要回调
                break;
            case CR:
            case LF:
                mVersionMajor = 0;
                mVersionMinor = 9;
                p_state = (ch == CR ? s_req_line_almost_done : s_header_field_start);
                turl.mLen = 1; //turl.mLen>0表示需要回调
                break;
            default:
                p_state = (AppParseUrlChar(p_state, ch));
                if (UNLIKELY(p_state == s_dead)) {
                    mHttpError = (HPE_INVALID_URL);
                    goto GT_ERROR;
                }
            }
            if (1 == turl.mLen && turl.mData) {
                DASSERT(mHttpError == HPE_OK);
                turl.mLen = p - turl.mData; //重新计算长度
                mState = p_state;
                if (UNLIKELY(!mMsg->mURL.decode(turl.mData, turl.mLen))) {
                    mReadSize = nread;
                    mHttpError = HPE_CB_URL;
                    return (p - data + 1);
                }
                turl.set(nullptr, 0);
                msgPath();
            }
            break;
        }

        case s_req_http_start:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                p_state = s_req_http_H;
                break;
            case 'I':
                if (mMethod == HTTP_SOURCE) {
                    p_state = s_req_http_I;
                    break;
                }
                /* fall through */
            default:
                mHttpError = HPE_INVALID_CONSTANT;
                goto GT_ERROR;
            }
            break;

        case s_req_http_H:
            STRICT_CHECK(ch != 'T');
            p_state = s_req_http_HT;
            break;

        case s_req_http_HT:
            STRICT_CHECK(ch != 'T');
            p_state = s_req_http_HTT;
            break;

        case s_req_http_HTT:
            STRICT_CHECK(ch != 'P');
            p_state = s_req_http_HTTP;
            break;

        case s_req_http_I:
            STRICT_CHECK(ch != 'C');
            p_state = s_req_http_IC;
            break;

        case s_req_http_IC:
            STRICT_CHECK(ch != 'E');
            p_state = s_req_http_HTTP;  // Treat "ICE" as "HTTP"
            break;

        case s_req_http_HTTP:
            STRICT_CHECK(ch != '/');
            p_state = (s_req_http_major);
            break;

        case s_req_http_major:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            mVersionMajor = ch - '0';
            p_state = (s_req_http_dot);
            break;

        case s_req_http_dot:
        {
            if (UNLIKELY(ch != '.')) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            p_state = (s_req_http_minor);
            break;
        }

        case s_req_http_minor:
            if (UNLIKELY(!IS_NUM(ch))) {
                mHttpError = (HPE_INVALID_VERSION);
                goto GT_ERROR;
            }
            mVersionMinor = ch - '0';
            p_state = (s_req_http_end);
            break;

        case s_req_http_end:
        {
            if (ch == CR) {
                p_state = (s_req_line_almost_done);
                break;
            }
            if (ch == LF) {
                p_state = (s_header_field_start);
                break;
            }
            mHttpError = (HPE_INVALID_VERSION);
            goto GT_ERROR;
            break;
        }

        /* end of request line */
        case s_req_line_almost_done:
        {
            if (UNLIKELY(ch != LF)) {
                mHttpError = (HPE_LF_EXPECTED);
                goto GT_ERROR;
            }
            p_state = (s_header_field_start);
            break;
        }

        case s_header_field_start:
        {
            if (ch == CR) {
                p_state = (s_headers_almost_done);
                break;
            }
            if (ch == LF) {
                /* they might be just sending \n instead of \r\n so this would be
                 * the second \n to denote the end of headers*/
                p_state = (s_headers_almost_done);
                goto GT_REPARSE;
            }

            c = TOKEN(ch);
            if (UNLIKELY(!c)) {
                mHttpError = (HPE_INVALID_HEADER_TOKEN);
                goto GT_ERROR;
            }

            if (!headkey.mData) {
                headkey.set(p, 0);
            }

            mIndex = 0;
            p_state = (s_header_field);

            switch (c) {
            case 'c':
                mHeaderState = h_C;
                break;

            case 'p':
                mHeaderState = h_matching_proxy_connection;
                break;

            case 't':
                mHeaderState = h_matching_transfer_encoding;
                break;

            case 'u':
                mHeaderState = h_matching_upgrade;
                break;

            default:
                mHeaderState = h_general;
                break;
            }
            break;
        }

        case s_header_field:
        {
            const s8* start = p;
            for (; p != end; p++) {
                ch = *p;
                c = TOKEN(ch);

                if (!c) {
                    break;
                }
                switch (mHeaderState) {
                case h_general:
                {
                    usz left = end - p;
                    const s8* pe = p + DMIN(left, GMAX_HEAD_SIZE);
                    while (p + 1 < pe && TOKEN(p[1])) {
                        p++;
                    }
                    break;
                }

                case h_C:
                    mIndex++;
                    mHeaderState = (c == 'o' ? h_CO : h_general);
                    break;

                case h_CO:
                    mIndex++;
                    mHeaderState = (c == 'n' ? h_CON : h_general);
                    break;

                case h_CON:
                    mIndex++;
                    switch (c) {
                    case 'n':
                        mHeaderState = h_matching_connection;
                        break;
                    case 't':
                        mHeaderState = h_matching_content_;
                        break;
                    default:
                        mHeaderState = h_general;
                        break;
                    }
                    break;

                case h_matching_connection:
                    mIndex++;
                    if (mIndex > sizeof(CONNECTION) - 1 || c != CONNECTION[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(CONNECTION) - 2) {
                        mHeaderState = h_connection;
                    }
                    break;

                case h_matching_proxy_connection:
                    mIndex++;
                    if (mIndex > sizeof(PROXY_CONNECTION) - 1 || c != PROXY_CONNECTION[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(PROXY_CONNECTION) - 2) {
                        mHeaderState = h_connection;
                    }
                    break;

                case h_matching_content_:
                    mIndex++;
                    if (mIndex == sizeof(CONTENT_) - 1) {
                        if ('t' == c) {
                            mHeaderState = h_matching_content_type;
                        } else if ('l' == c) {
                            mHeaderState = h_matching_content_length;
                        } else if ('d' == c) {
                            mHeaderState = h_matching_content_disp;
                        } else {
                            mHeaderState = h_general;
                        }
                    } else if (mIndex > sizeof(CONTENT_) - 1 || c != CONTENT_[mIndex]) {
                        mHeaderState = h_general;
                    }
                    break;

                case h_matching_content_disp:
                    mIndex++;
                    if (mIndex > sizeof(CONTENT_DISP) - 1 || c != CONTENT_DISP[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(CONTENT_DISP) - 2) {
                        mHeaderState = h_content_disp;
                    }
                    break;

                case h_matching_content_type:
                    mIndex++;
                    if (mIndex > sizeof(CONTENT_TYPE) - 1 || c != CONTENT_TYPE[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(CONTENT_TYPE) - 2) {
                        mHeaderState = h_content_type;
                    }
                    break;

                case h_matching_content_length:
                    mIndex++;
                    if (mIndex > sizeof(CONTENT_LENGTH) - 1 || c != CONTENT_LENGTH[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(CONTENT_LENGTH) - 2) {
                        mHeaderState = h_content_length;
                    }
                    break;

                case h_matching_transfer_encoding:
                    mIndex++;
                    if (mIndex > sizeof(TRANSFER_ENCODING) - 1 || c != TRANSFER_ENCODING[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(TRANSFER_ENCODING) - 2) {
                        mHeaderState = h_transfer_encoding;
                        mUseTransferEncode = 1;
                    }
                    break;

                case h_matching_upgrade:
                    mIndex++;
                    if (mIndex > sizeof(UPGRADE) - 1 || c != UPGRADE[mIndex]) {
                        mHeaderState = h_general;
                    } else if (mIndex == sizeof(UPGRADE) - 2) {
                        mHeaderState = h_upgrade;
                    }
                    break;

                case h_connection:
                case h_content_type:
                case h_content_disp:
                case h_content_length:
                case h_transfer_encoding:
                case h_upgrade:
                    if (ch != ' ') {
                        mHeaderState = h_general;
                    }
                    break;

                default:
                    DASSERT(0 && "Unknown header_state");
                    break;
                }
            }

            if (p == end) {
                --p;
                COUNT_HEADER_SIZE(p - start);
                break;
            }

            COUNT_HEADER_SIZE(p - start);

            if (ch == ':') {
                p_state = (s_header_value_discard_ws);
                headkey.mLen = p - headkey.mData;
                break;
            }

            mHttpError = (HPE_INVALID_HEADER_TOKEN);
            goto GT_ERROR;
        }//s_header_field

        case s_header_value_discard_ws:
        {
            if (ch == ' ' || ch == '\t') {
                break;
            }
            if (ch == CR) {
                p_state = (s_header_value_discard_ws_almost_done);
                break;
            }
            if (ch == LF) {
                p_state = (s_header_value_discard_lws);
                break;
            }
            //break; fall through
        }

        case s_header_value_start:
        {
            if (!headval.mData) {
                headval.set(p, 0);
            }

            p_state = (s_header_value);
            mIndex = 0;

            c = LOWER(ch);

            switch (mHeaderState) {
            case h_upgrade:
                mFlags |= F_UPGRADE;
                mHeaderState = h_general;
                break;

            case h_transfer_encoding:
                /* looking for 'Transfer-Encoding: chunked' */
                if ('c' == c) {
                    mHeaderState = h_matching_transfer_encoding_chunked;
                } else {
                    mHeaderState = h_matching_transfer_encoding_token;
                }
                break;

                /* Multi-value `Transfer-Encoding` header */
            case h_matching_transfer_encoding_token_start:
                break;

            case h_content_disp:
            {
                StringView vals[6];
                s32 vcnt = (s32)DSIZEOF(vals);
                s32 vflags;
                p = parseValue(p, end, vals, vcnt, vflags);
                if (vflags & EHV_DONE) {
                    if (vcnt >= 4) {
                        //name of form
                        if (4 == vals[2].mLen && App4Char2S32(vals[2].mData) == App4Char2S32("name")) {
                            mFormNameLen = (u8)vals[3].mLen;
                            memcpy(mFormName, vals[3].mData, mFormNameLen);
                        }

                        //filename
                        if (vcnt >= 6) {
                            if (8 == vals[4].mLen && App4Char2S32(vals[4].mData) == App4Char2S32("file")) {
                                mFileNameLen = (u8)vals[5].mLen;
                                memcpy(mFileName, vals[5].mData, mFileNameLen);
                            }
                        }
                    }
                    mHeaderState = h_general;
                }
                break;
            }
            case h_content_type:
            {
                if ('m' == c) {
                    StringView vals[6];
                    s32 vcnt = (s32)DSIZEOF(vals);
                    s32 vflags;
                    p = parseValue(p, end, vals, vcnt, vflags);
                    if ((vflags & EHV_DONE) && (vflags & EHV_CONTENT_TYPE_MULTI)) {
                        if (4 == vcnt && 8 == vals[2].mLen) {
                            if ((mFlags & F_BOUNDARY) || vals[3].mLen > sizeof(mBoundary)) {
                                mHttpError = (HPE_UNKNOWN);
                                goto GT_ERROR;
                            }
                            mFlags |= F_BOUNDARY;
                            memcpy(mBoundary, vals[3].mData, vals[3].mLen);
                            mBoundaryLen = (u8)vals[3].mLen;
                            mHeaderState = h_general;
                        }
                    }
                } else {
                    mHeaderState = h_general;
                }
                break;
            }
            case h_content_length:
                if (UNLIKELY(!IS_NUM(ch))) {
                    mHttpError = HPE_INVALID_CONTENT_LENGTH;
                    goto GT_ERROR;
                }

                if (mFlags & F_CONTENTLENGTH) {
                    mHttpError = HPE_UNEXPECTED_CONTENT_LENGTH;
                    goto GT_ERROR;
                }

                mFlags |= F_CONTENTLENGTH;
                mContentLen = ch - '0';
                mHeaderState = h_content_length_num;
                break;

                /* when obsolete line folding is encountered for content length
                 * continue to the s_header_value state */
            case h_content_length_ws:
                break;

            case h_connection:
                /* looking for 'Connection: keep-alive' */
                if (c == 'k') {
                    mHeaderState = h_matching_connection_keep_alive;
                    /* looking for 'Connection: close' */
                } else if (c == 'c') {
                    mHeaderState = h_matching_connection_close;
                } else if (c == 'u') {
                    mHeaderState = h_matching_connection_upgrade;
                } else {
                    mHeaderState = h_matching_connection_token;
                }
                break;

                /* Multi-value `Connection` header */
            case h_matching_connection_token_start:
                break;

            default:
                mHeaderState = h_general;
                break;
            }
            break;
        }

        case s_header_value:
        {
            const s8* start = p;
            EHttpHeaderState h_state = (EHttpHeaderState)mHeaderState;
            for (; p < end; p++) {
                ch = *p;
                if (ch == CR) {
                    p_state = (s_header_almost_done);
                    mHeaderState = h_state;

                    DASSERT(mHttpError == HPE_OK);
                    if (headkey.mData && headval.mData) {
                        headval.mLen = p - headval.mData;
                        mState = p_state;
                        mMsg->mHeadIn.add(headkey, headval);
                        headkey.set(nullptr, 0);
                        headval.set(nullptr, 0);
                    }
                    break;
                }

                if (ch == LF) {
                    p_state = (s_header_almost_done);
                    COUNT_HEADER_SIZE(p - start);
                    mHeaderState = h_state;
                    DASSERT(mHttpError == HPE_OK);
                    if (headkey.mData && headval.mData) {
                        headval.mLen = p - headval.mData;
                        mState = p_state;
                        mMsg->mHeadIn.add(headkey, headval);
                        headkey.set(nullptr, 0);
                        headval.set(nullptr, 0);
                    }
                    goto GT_REPARSE;
                }

                if (!lenient && !IS_HEADER_CHAR(ch)) {
                    mHttpError = (HPE_INVALID_HEADER_TOKEN);
                    goto GT_ERROR;
                }

                c = LOWER(ch);

                switch (h_state) {
                case h_general:
                {
                    usz left = end - p;
                    const s8* pe = p + DMIN(left, GMAX_HEAD_SIZE);
                    for (; p != pe; p++) {
                        ch = *p;
                        if (ch == CR || ch == LF) {
                            --p;
                            break;
                        }
                        if (!lenient && !IS_HEADER_CHAR(ch)) {
                            mHttpError = (HPE_INVALID_HEADER_TOKEN);
                            goto GT_ERROR;
                        }
                    }
                    if (p == end) {
                        --p;
                    }
                    break;
                }

                case h_connection:
                case h_transfer_encoding:
                    DASSERT(0 && "Shouldn't get here.");
                    break;

                case h_content_length:
                    if (ch == ' ') {
                        break;
                    }
                    h_state = h_content_length_num;
                    //break; fall through

                case h_content_length_num:
                {
                    if (ch == ' ') {
                        h_state = h_content_length_ws;
                        break;
                    }

                    if (UNLIKELY(!IS_NUM(ch))) {
                        mHttpError = (HPE_INVALID_CONTENT_LENGTH);
                        mHeaderState = h_state;
                        goto GT_ERROR;
                    }

                    u64 t = mContentLen;
                    t *= 10;
                    t += ch - '0';

                    /* Overflow? Test against a conservative limit for simplicity. */
                    if (UNLIKELY((ULLONG_MAX - 10) / 10 < mContentLen)) {
                        mHttpError = (HPE_INVALID_CONTENT_LENGTH);
                        mHeaderState = h_state;
                        goto GT_ERROR;
                    }

                    mContentLen = t;
                    break;
                }

                case h_content_length_ws:
                    if (ch == ' ') { break; }
                    mHttpError = (HPE_INVALID_CONTENT_LENGTH);
                    mHeaderState = h_state;
                    goto GT_ERROR;

                case h_matching_transfer_encoding_token_start:
                {
                    /* looking for 'Transfer-Encoding: chunked' */
                    if ('c' == c) {
                        h_state = h_matching_transfer_encoding_chunked;
                    } else if (STRICT_TOKEN(c)) {
                        /* TODO(indutny): similar code below does this, but why?
                         * At the very least it seems to be inconsistent given that
                         * h_matching_transfer_encoding_token does not check for
                         * `STRICT_TOKEN`
                         */
                        h_state = h_matching_transfer_encoding_token;
                    } else if (c == ' ' || c == '\t') {
                        //Skip lws
                    } else {
                        h_state = h_general;
                    }
                    break;
                }

                case h_matching_transfer_encoding_chunked:
                    mIndex++;
                    if (mIndex > sizeof(CHUNKED) - 1 || c != CHUNKED[mIndex]) {
                        h_state = h_matching_transfer_encoding_token;
                    } else if (mIndex == sizeof(CHUNKED) - 2) {
                        h_state = h_transfer_encoding_chunked;
                    }
                    break;

                case h_matching_transfer_encoding_token:
                    if (ch == ',') {
                        h_state = h_matching_transfer_encoding_token_start;
                        mIndex = 0;
                    }
                    break;

                case h_matching_connection_token_start:
                {
                    /* looking for 'Connection: keep-alive' */
                    if (c == 'k') {
                        h_state = h_matching_connection_keep_alive;
                        /* looking for 'Connection: close' */
                    } else if (c == 'c') {
                        h_state = h_matching_connection_close;
                    } else if (c == 'u') {
                        h_state = h_matching_connection_upgrade;
                    } else if (STRICT_TOKEN(c)) {
                        h_state = h_matching_connection_token;
                    } else if (c == ' ' || c == '\t') {
                        /* Skip lws */
                    } else {
                        h_state = h_general;
                    }
                    break;
                }
                case h_matching_connection_keep_alive:
                    mIndex++;
                    if (mIndex > sizeof(KEEP_ALIVE) - 1 || c != KEEP_ALIVE[mIndex]) {
                        h_state = h_matching_connection_token;
                    } else if (mIndex == sizeof(KEEP_ALIVE) - 2) {
                        h_state = h_connection_keep_alive;
                    }
                    break;

                    //looking for 'Connection: close'
                case h_matching_connection_close:
                    mIndex++;
                    if (mIndex > sizeof(CLOSE) - 1 || c != CLOSE[mIndex]) {
                        h_state = h_matching_connection_token;
                    } else if (mIndex == sizeof(CLOSE) - 2) {
                        h_state = h_connection_close;
                    }
                    break;

                case h_matching_connection_upgrade:
                    mIndex++;
                    if (mIndex > sizeof(UPGRADE) - 1 || c != UPGRADE[mIndex]) {
                        h_state = h_matching_connection_token;
                    } else if (mIndex == sizeof(UPGRADE) - 2) {
                        h_state = h_connection_upgrade;
                    }
                    break;

                case h_matching_connection_token:
                    if (ch == ',') {
                        h_state = h_matching_connection_token_start;
                        mIndex = 0;
                    }
                    break;

                case h_transfer_encoding_chunked:
                    if (ch != ' ') {
                        h_state = h_matching_transfer_encoding_token;
                    }
                    break;

                case h_connection_keep_alive:
                case h_connection_close:
                case h_connection_upgrade:
                {
                    if (ch == ',') {
                        if (h_state == h_connection_keep_alive) {
                            mFlags |= F_CONNECTION_KEEP_ALIVE;
                        } else if (h_state == h_connection_close) {
                            mFlags |= F_CONNECTION_CLOSE;
                        } else if (h_state == h_connection_upgrade) {
                            mFlags |= F_CONNECTION_UPGRADE;
                        }
                        h_state = h_matching_connection_token_start;
                        mIndex = 0;
                    } else if (ch != ' ') {
                        h_state = h_matching_connection_token;
                    }
                    break;
                }
                default:
                    p_state = (s_header_value);
                    h_state = h_general;
                    break;
                }//switch(h_state)
            }//for in val
            mHeaderState = h_state;

            if (p == end) {
                --p;
            }
            COUNT_HEADER_SIZE(p - start);
            break;
        }

        case s_header_almost_done:
        {
            if (UNLIKELY(ch != LF)) {
                mHttpError = (HPE_LF_EXPECTED);
                goto GT_ERROR;
            }
            p_state = s_header_value_lws;
            break;
        }

        case s_header_value_lws:
        {
            if (ch == ' ' || ch == '\t') {
                if (mHeaderState == h_content_length_num) {
                    /* treat obsolete line folding as space */
                    mHeaderState = h_content_length_ws;
                }
                p_state = (s_header_value_start);
                goto GT_REPARSE;
            }

            /* finished the header */
            switch (mHeaderState) {
            case h_connection_keep_alive:
                mFlags |= F_CONNECTION_KEEP_ALIVE;
                break;
            case h_connection_close:
                mFlags |= F_CONNECTION_CLOSE;
                break;
            case h_transfer_encoding_chunked:
                mFlags |= F_CHUNKED;
                break;
            case h_connection_upgrade:
                mFlags |= F_CONNECTION_UPGRADE;
                break;
            default: break;
            }

            p_state = (s_header_field_start);
            goto GT_REPARSE;
        }

        case s_header_value_discard_ws_almost_done:
        {
            STRICT_CHECK(ch != LF);
            p_state = (s_header_value_discard_lws);
            break;
        }

        case s_header_value_discard_lws:
        {
            if (ch == ' ' || ch == '\t') {
                p_state = (s_header_value_discard_ws);
                break;
            } else {
                switch (mHeaderState) {
                case h_connection_keep_alive:
                    mFlags |= F_CONNECTION_KEEP_ALIVE;
                    break;
                case h_connection_close:
                    mFlags |= F_CONNECTION_CLOSE;
                    break;
                case h_connection_upgrade:
                    mFlags |= F_CONNECTION_UPGRADE;
                    break;
                case h_transfer_encoding_chunked:
                    mFlags |= F_CHUNKED;
                    break;
                case h_content_length:
                    /* do not allow empty content length */
                    mHttpError = (HPE_INVALID_CONTENT_LENGTH);
                    goto GT_ERROR;
                    break;
                default:
                    break;
                }

                // empty header value
                if (!headval.mData) {
                    headval.set(p, 0);
                }
                p_state = (s_header_field_start);
                DASSERT(mHttpError == HPE_OK);
                if (headkey.mData && headval.mData) {
                    headval.mLen = p - headval.mData;
                    mState = p_state;
                    mMsg->mHeadIn.add(headkey, headval);
                    headkey.set(nullptr, 0);
                    headval.set(nullptr, 0);
                }
                goto GT_REPARSE;
            }
        }

        case s_headers_almost_done:
        {
            STRICT_CHECK(ch != LF);
            if (mFlags & F_HEAD_DONE) {
                if (mFlags & F_BOUNDARY) {
                    p_state = s_boundary_body;
                    mValueState = EHV_BOUNDARY_BODY_PRE;
                    mFlags &= ~F_BOUNDARY_CMP;
                    break;
                }
                if (mFlags & (F_CHUNKED | F_TAILING)) {
                    //End of a chunked request
                    p_state = s_message_done;
                    chunkDone();
                    mState = p_state;
                    goto GT_REPARSE;
                }
            }

            /* Cannot use transfer-encoding and a content-length header together
               per the HTTP specification. (RFC 7230 Section 3.3.3) */
            if ((mUseTransferEncode == 1) &&
                (mFlags & F_CONTENTLENGTH)) {
                /* Allow it for lenient parsing as long as `Transfer-Encoding` is
                 * not `chunked` or allow_length_with_encoding is set */
                if (mFlags & F_CHUNKED) {
                    if (!allow_chunked_length) {
                        mHttpError = (HPE_UNEXPECTED_CONTENT_LENGTH);
                        goto GT_ERROR;
                    }
                } else if (!lenient) {
                    mHttpError = (HPE_UNEXPECTED_CONTENT_LENGTH);
                    goto GT_ERROR;
                }
            }

            p_state = s_headers_done;

            /* Set this here so that headDone() can see it */
            if ((mFlags & F_UPGRADE) &&
                (mFlags & F_CONNECTION_UPGRADE)) {
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
                mHttpError = (HPE_CB_HeadersComplete);
                mState = p_state;
                return (p - data); //Error
            }

            if (mHttpError != HPE_OK) {
                mReadSize = nread;
                mState = p_state;
                return (p - data);
            }
            goto GT_REPARSE;
        }

        case s_boundary_body:
        {
            p = parseBoundBody(p, end, tbody);
            p_state = (EPareState)(mState);
            if (UNLIKELY(mHttpError != HPE_OK)) {
                return (p - data);
            }
            break;
        }
        case s_headers_done:
        {
            STRICT_CHECK(ch != LF);
            DASSERT(0 == (F_HEAD_DONE & mFlags));
            mFlags |= F_HEAD_DONE;
            mReadSize = 0;
            nread = 0;

            s32 hasBody = (mFlags & (F_CHUNKED | F_BOUNDARY))
                || (mContentLen > 0 && mContentLen != ULLONG_MAX);

            if (mUpgrade && (mMethod == HTTP_CONNECT || (mFlags & F_SKIPBODY) || !hasBody)) {
                //Exit, the rest of the message is in a different protocol
                p_state = (NEW_MESSAGE());
                msgEnd();
                mReadSize = nread;
                mState = p_state;
                return (p - data + 1);
            }

            if (mFlags & F_SKIPBODY) {
                p_state = (NEW_MESSAGE());
                //CALLBACK_NOTIFY(MsgComplete);
                msgEnd();
                mState = p_state;
            } else if (mFlags & F_CHUNKED) {
                //chunked encoding, ignore Content-Length header
                p_state = s_chunk_size_start;
            } else if (F_BOUNDARY & mFlags) {
                mIndex = 0;
                p_state = s_boundary_body;
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
                    mHttpError = (HPE_INVALID_TRANSFER_ENCODING);
                    mState = p_state;
                    return (p - data); /* Error */
                } else {
                    /* RFC 7230 3.3.3,
                     * If a Transfer-Encoding header field is present in a response and
                     * the chunked transfer coding is not the final encoding, the
                     * message body length is determined by reading the connection until
                     * it is closed by the server.
                     */
                    p_state = (s_body_identity_eof);
                }
            } else {
                if (mContentLen == 0) {
                    /* Content-Length header given but zero: Content-Length: 0\r\n */
                    p_state = (NEW_MESSAGE());
                    msgEnd();
                    mState = p_state;
                } else if (mContentLen != ULLONG_MAX) {
                    /* Content-Length header given and non-zero */
                    p_state = s_body_identity;
                } else {
                    if (!needEOF()) {
                        //Assume content-length 0 - read the next
                        p_state = (NEW_MESSAGE());
                        msgEnd();
                        mState = p_state;
                    } else {
                        /* Read body until EOF */
                        p_state = (s_body_identity_eof);
                    }
                }
            }
            break;
        }

        case s_body_identity:
        {
            u64 to_read = DMIN(mContentLen, (u64)(end - p));
            DASSERT(mContentLen != 0 && mContentLen != ULLONG_MAX);

            /* The difference between advancing content_length and p is because
             * the latter will automaticaly advance on the next loop iteration.
             * Further, if content_length ends up at 0, we want to see the last
             * byte again for our message complete callback.
             */
            if (!tbody.mData) {
                tbody.set(p, 0);
            }
            mContentLen -= to_read;
            p += to_read - 1;

            if (mContentLen == 0) {
                p_state = (s_message_done);

                /* Mimic CALLBACK_DATA_NOADVANCE() but with one extra byte.
                 *
                 * The alternative to doing this is to wait for the next byte to
                 * trigger the data callback, just as in every other case. The
                 * problem with this is that this makes it difficult for the test
                 * harness to distinguish between complete-on-EOF and
                 * complete-on-length. It's not clear that this distinction is
                 * important for applications, but let's keep it for now.
                 */

                 //CALLBACK_DATA_(tbody.mData, p - tbody.mData + 1, p - data);
                DASSERT(mHttpError == HPE_OK);
                if (tbody.mData) {
                    tbody.mLen = p - tbody.mData + 1;
                    mState = p_state;
                    mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
                    //if (UNLIKELY(mHttpError != HPE_OK)) {
                    //    return (p - data);
                    //}
                    tbody.set(nullptr, 0);
                }
                goto GT_REPARSE;
            }

            break;
        }

        //read until EOF
        case s_body_identity_eof:
            if (!tbody.mData) {
                tbody.set(p, 0);
            }
            p = end - 1;
            break;

        case s_message_done:
            p_state = (NEW_MESSAGE());
            msgEnd();
            mState = p_state;
            mReadSize = nread;
            if (mUpgrade) {
                //Exit, the rest of the message is in a different protocol
                return (p - data + 1);
            }
            break;

        case s_chunk_size_start:
        {
            DASSERT(nread == 1);
            DASSERT(mFlags & F_CHUNKED);

            unhex_val = GMAP_UN_HEX[(u8)ch];
            if (UNLIKELY(unhex_val == 0xFF)) {
                mHttpError = (HPE_INVALID_CHUNK_SIZE);
                goto GT_ERROR;
            }

            mContentLen = unhex_val;
            p_state = (s_chunk_size);
            break;
        }

        case s_chunk_size:
        {
            DASSERT(mFlags & F_CHUNKED);
            if (ch == CR) {
                p_state = (s_chunk_size_almost_done);
                break;
            }

            unhex_val = GMAP_UN_HEX[(u8)ch];
            if (unhex_val == 0xFF) {
                if (ch == ';' || ch == ' ') {
                    p_state = (s_chunk_parameters);
                    break;
                }
                mHttpError = (HPE_INVALID_CHUNK_SIZE);
                goto GT_ERROR;
            }

            u64 t = mContentLen;
            t *= 16;
            t += unhex_val;

            /* Overflow? Test against a conservative limit for simplicity. */
            if (UNLIKELY((ULLONG_MAX - 16) / 16 < mContentLen)) {
                mHttpError = (HPE_INVALID_CONTENT_LENGTH);
                goto GT_ERROR;
            }

            mContentLen = t;
            break;
        }

        case s_chunk_parameters:
        {
            DASSERT(mFlags & F_CHUNKED);
            /* just ignore this shit. TODO check for overflow */
            if (ch == CR) {
                p_state = (s_chunk_size_almost_done);
                break;
            }
            break;
        }

        case s_chunk_size_almost_done:
        {
            DASSERT(mFlags & F_CHUNKED);
            STRICT_CHECK(ch != LF);

            mReadSize = 0;
            nread = 0;
            if (mContentLen == 0) {
                mFlags |= F_TAILING;
                p_state = (s_header_field_start);
            } else {
                p_state = (s_chunk_data);
            }
            chunkHeadDone();
            mState = p_state;
            break;
        }

        case s_chunk_data:
        {
            u64 to_read = DMIN(mContentLen, (u64)(end - p));

            DASSERT(mFlags & F_CHUNKED);
            DASSERT(mContentLen != 0 && mContentLen != ULLONG_MAX);

            /* See the explanation in s_body_identity for why the content
             * length and data pointers are managed this way.
             */
            if (!tbody.mData) {
                tbody.set(p, 0);
            }
            mContentLen -= to_read;
            p += to_read - 1;

            if (mContentLen == 0) {
                p_state = (s_chunk_data_almost_done);
            }

            break;
        }

        case s_chunk_data_almost_done:
            DASSERT(mFlags & F_CHUNKED);
            DASSERT(mContentLen == 0);
            STRICT_CHECK(ch != CR);
            p_state = (s_chunk_data_done);

            DASSERT(mHttpError == HPE_OK);
            if (tbody.mData) {
                tbody.mLen = p - tbody.mData;
                mState = p_state;
                mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
                //if (UNLIKELY(mHttpError != HPE_OK)) {
                //    return (p - data+1);
                //}
                tbody.set(nullptr, 0);
            }
            break;

        case s_chunk_data_done:
            DASSERT(mFlags & F_CHUNKED);
            STRICT_CHECK(ch != LF);
            mReadSize = 0;
            nread = 0;
            p_state = (s_chunk_size_start);
            chunkMsg();
            mState = p_state;
            break;

        default:
            DASSERT(0 && "unhandled state");
            mHttpError = (HPE_INVALID_INTERNAL_STATE);
            goto GT_ERROR;
        }//switch
    }//for -------------------------------------------------


    //check the leftover, @note p is out span now
    if (turl.mData) {
        len = turl.mData - data;
        mHeaderState = h_general;
    } else if (tstat.mData) {
        len = tstat.mData - data;
        mHeaderState = h_general;
        //mState = s_header_field_start;
    } else if (headkey.mData) {
        DASSERT(headkey.mData >= data);
        len = headkey.mData - data;
        mReadSize = nread - (u32)(p - headkey.mData);
        mState = s_header_field_start;

        //清除不能重入的状态
        //判断是不是body中的header
        if (0 == (mFlags & F_HEAD_DONE)) {
            switch (mHeaderState) {
            case h_content_length:
            case h_content_length_num:
                mFlags &= (~(u32)(F_CONTENTLENGTH));
                break;
            case h_content_type:
                mFlags &= (~(u32)(F_BOUNDARY));
                break;
            default:break;
            }
        }
        mHeaderState = h_general;
    } else if (tbody.mData) {
        tbody.mLen = p - tbody.mData;
        mState = p_state;
        mMsg->mCacheIn.write(tbody.mData, tbody.mLen);
        //if (UNLIKELY(mHttpError != HPE_OK)) {
        //    return (p - data);
        //}
        //tbody.set(nullptr, 0);
    }

    mReadSize = nread;
    mState = p_state;
    return len;


    GT_ERROR:
    if (HPE_OK == mHttpError) {
        mHttpError = HPE_UNKNOWN;
    }
    mReadSize = nread;
    mState = p_state;
    msgError();
    return (p - data);
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
            //break; fall
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
            //partly callback
            //curr - tbody.mData
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
                mState = s_header_field_start;
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
                mState = s_message_done;
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
        } //switch
    }//for

    mValueState = vstat;
    return curr;
}


bool HttpLayer::needEOF()const {
    if (mType == EHTTP_REQUEST) {
        return false;
    }

    /* See RFC 2616 section 4.4 */
    if (mStatusCode / 100 == 1 || /* 1xx e.g. Continue */
        mStatusCode == 204 ||     /* No Content */
        mStatusCode == 304 ||     /* Not Modified */
        mFlags & F_SKIPBODY) {     /* response to a HEAD request */
        return false;
    }

    /* RFC 7230 3.3.3, see `s_headers_almost_done` */
    if ((mUseTransferEncode == 1) && (mFlags & F_CHUNKED) == 0) {
        return true;
    }

    if ((mFlags & F_CHUNKED) || mContentLen != ULLONG_MAX) {
        return false;
    }

    return true;
}


bool HttpLayer::shouldKeepAlive()const {
    if (mVersionMajor > 0 && mVersionMinor > 0) {
        /* HTTP/1.1 */
        if (mFlags & F_CONNECTION_CLOSE) {
            return false;
        }
    } else {
        /* HTTP/1.0 or earlier */
        if (!(mFlags & F_CONNECTION_KEEP_ALIVE)) {
            return false;
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
    return mState == s_message_done;
}

const s8* HttpLayer::getErrStr(EHttpError it) {
    return HttpStrErrorTab[it].description;
}

StringView HttpLayer::getMethodStr(EHttpMethod it) {
#define DCASE(num, name, str) case HTTP_##name: ret.set(#str,sizeof(#str)-1);break;

    StringView ret;
    switch (it) {
        HTTP_METHOD_MAP(DCASE)
    default:
        ret.set("NULL", 4);
        break;
    }

#undef DCASE
    return ret;
}


//////////////////////////////
static EHttpHostState parseHostChar(EHttpHostState s, const s8 ch) {
    switch (s) {
    case s_http_userinfo:
    case s_http_userinfo_start:
        if (ch == '@') {
            return s_http_host_start;
        }
        if (IS_USERINFO_CHAR(ch)) {
            return s_http_userinfo;
        }
        break;

    case s_http_host_start:
        if (ch == '[') {
            return s_http_host_v6_start;
        }
        if (IS_HOST_CHAR(ch)) {
            return s_http_host;
        }
        break;

    case s_http_host:
        if (IS_HOST_CHAR(ch)) {
            return s_http_host;
        }
        //break; fall

    case s_http_host_v6_end:
        if (ch == ':') {
            return s_http_host_port_start;
        }
        break;

    case s_http_host_v6:
        if (ch == ']') {
            return s_http_host_v6_end;
        }
        //break; fall

    case s_http_host_v6_start:
        if (IS_HEX(ch) || ch == ':' || ch == '.') {
            return s_http_host_v6;
        }
        if (s == s_http_host_v6 && ch == '%') {
            return s_http_host_v6_zone_start;
        }
        break;

    case s_http_host_v6_zone:
        if (ch == ']') {
            return s_http_host_v6_end;
        }
        //break; fall

    case s_http_host_v6_zone_start:
        /* RFC 6874 Zone ID consists of 1*( unreserved / pct-encoded) */
        if (IS_ALPHANUM(ch) || ch == '%' || ch == '.' || ch == '-' || ch == '_' ||
            ch == '~') {
            return s_http_host_v6_zone;
        }
        break;

    case s_http_host_port:
    case s_http_host_port_start:
        if (IS_NUM(ch)) {
            return s_http_host_port;
        }
        break;

    default:
        break;
    }
    return s_http_host_dead;
}


s32 HttpURL::parseHost(const s8* buf, s32 found_at) {
    usz buflen = mFieldData[UF_HOST].mOffset + mFieldData[UF_HOST].mLen;
    DASSERT(mFieldSet & (1 << UF_HOST));
    mFieldData[UF_HOST].mLen = 0;

    EHttpHostState s = found_at ? s_http_userinfo_start : s_http_host_start;

    for (const s8* p = buf + mFieldData[UF_HOST].mOffset; p < buf + buflen; p++) {
        EHttpHostState new_s = parseHostChar(s, *p);

        if (new_s == s_http_host_dead) {
            return 1;
        }

        switch (new_s) {
        case s_http_host:
            if (s != s_http_host) {
                mFieldData[UF_HOST].mOffset = (u16)(p - buf);
            }
            mFieldData[UF_HOST].mLen++;
            break;

        case s_http_host_v6:
            if (s != s_http_host_v6) {
                mFieldData[UF_HOST].mOffset = (u16)(p - buf);
            }
            mFieldData[UF_HOST].mLen++;
            break;

        case s_http_host_v6_zone_start:
        case s_http_host_v6_zone:
            mFieldData[UF_HOST].mLen++;
            break;

        case s_http_host_port:
            if (s != s_http_host_port) {
                mFieldData[UF_PORT].mOffset = (u16)(p - buf);
                mFieldData[UF_PORT].mLen = 0;
                mFieldSet |= (1 << UF_PORT);
            }
            mFieldData[UF_PORT].mLen++;
            break;

        case s_http_userinfo:
            if (s != s_http_userinfo) {
                mFieldData[UF_USERINFO].mOffset = (u16)(p - buf);
                mFieldData[UF_USERINFO].mLen = 0;
                mFieldSet |= (1 << UF_USERINFO);
            }
            mFieldData[UF_USERINFO].mLen++;
            break;

        default:
            break;
        }
        s = new_s;
    }

    /* Make sure we don't end somewhere unexpected */
    switch (s) {
    case s_http_host_start:
    case s_http_host_v6_start:
    case s_http_host_v6:
    case s_http_host_v6_zone_start:
    case s_http_host_v6_zone:
    case s_http_host_port_start:
    case s_http_userinfo:
    case s_http_userinfo_start:
        return 1;
    default:
        break;
    }

    return 0;
}


s32 HttpURL::parseURL(const s8* buf, usz buflen, s32 is_connect) {
    if (buflen == 0) {
        return 1;
    }

    s32 found_at = 0;
    mPort = mFieldSet = 0;
    EPareState s = is_connect ? s_req_server_start : s_req_spaces_before_url;
    EHttpUrlFields uf;
    EHttpUrlFields old_uf = UF_MAX;
    for (const s8* p = buf; p < buf + buflen; p++) {
        s = AppParseUrlChar2(s, *p);

        /* Figure out the next field that we're operating on */
        switch (s) {
        case s_dead:
            return 1;

            /* Skip delimeters */
        case s_req_schema_slash:
        case s_req_schema_slash2:
        case s_req_server_start:
        case s_req_query_string_start:
        case s_req_fragment_start:
            continue;

        case s_req_schema:
            uf = UF_SCHEMA;
            break;

        case s_req_server_with_at:
            found_at = 1;

            /* fall through */
        case s_req_server:
            uf = UF_HOST;
            break;

        case s_req_path:
            uf = UF_PATH;
            break;

        case s_req_query_string:
            uf = UF_QUERY;
            break;

        case s_req_fragment:
            uf = UF_FRAGMENT;
            break;

        default:
            DASSERT(!"Unexpected state");
            return 1;
        }

        /* Nothing's changed; soldier on */
        if (uf == old_uf) {
            mFieldData[uf].mLen++;
            continue;
        }

        mFieldData[uf].mOffset = (u16)(p - buf);
        mFieldData[uf].mLen = 1;

        mFieldSet |= (1 << uf);
        old_uf = uf;
    }

    /* host must be present if there is a schema */
    /* parsing http:///toto will fail */
    if ((mFieldSet & (1 << UF_SCHEMA)) &&
        (mFieldSet & (1 << UF_HOST)) == 0) {
        return 1;
    }

    if (mFieldSet & (1 << UF_HOST)) {
        if (parseHost(buf, found_at) != 0) {
            return 1;
        }
    }

    /* CONNECT requests can only contain "hostname:port" */
    if (is_connect && mFieldSet != ((1 << UF_HOST) | (1 << UF_PORT))) {
        return 1;
    }

    if (mFieldSet & (1 << UF_PORT)) {
        u16 off = mFieldData[UF_PORT].mOffset;
        u16 len = mFieldData[UF_PORT].mLen;
        const s8* end = buf + off + len;

        /* NOTE: The characters are already validated and are in the [0-9] range */
        DASSERT((usz)(off + len) <= buflen && "Port number overflow");
        usz v = 0;
        for (const s8* p = buf + off; p < end; p++) {
            v *= 10;
            v += *p - '0';
            if (v > 0xffff) {
                return 1;
            }
        }

        mPort = (u16)v;
    }

    return 0;
}




} //namespace net
}//namespace
