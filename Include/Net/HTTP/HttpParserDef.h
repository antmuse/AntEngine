#pragma once



namespace app {
namespace net {


// Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run faster
#ifndef DHTTP_PARSE_STRICT
#define DHTTP_PARSE_STRICT 1
#endif

// Maximium header size allowed.
#ifndef HTTP_MAX_HEADER_SIZE
#define HTTP_MAX_HEADER_SIZE (80 * 1024)
#endif



/* Macros for character classes; depends on strict-mode  */
#define CR '\r'
#define LF '\n'
#define LOWER(c) (u8)(c | 0x20)
#define IS_ALPHA(c) (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c) (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c) (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))
#define IS_MARK(c)                                                                                                     \
    ((c) == '-' || (c) == '_' || (c) == '.' || (c) == '!' || (c) == '~' || (c) == '*' || (c) == '\'' || (c) == '('     \
        || (c) == ')')

#define IS_USERINFO_CHAR(c)                                                                                            \
    (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+'  \
        || (c) == '$' || (c) == ',')

#define STRICT_TOKEN(c) ((c == ' ') ? 0 : GTokens[(u8)c])

#if DHTTP_PARSE_STRICT
#define TOKEN(c) STRICT_TOKEN(c)
#define IS_URL_CHAR(c) (AppIsBitON(G_NORMAL_URL_CHAR, (u8)c))
#define IS_HOST_CHAR(c) (IS_ALPHANUM(c) || (c) == '.' || (c) == '-')
#else
#define TOKEN(c) GTokens[(u8)c]
#define IS_URL_CHAR(c) (AppIsBitON(G_NORMAL_URL_CHAR, (u8)c) || ((c) & 0x80))
#define IS_HOST_CHAR(c) (IS_ALPHANUM(c) || (c) == '.' || (c) == '-' || (c) == '_')
#endif


enum EPareState {
    PS_DEAD = 1, // important that this is > 0

    PS_START_MSG,
    PS_START_H,

    PS_RESP_START, // response start
    PS_RESP_H,
    PS_RESP_HT,
    PS_RESP_HTT,
    PS_RESP_HTTP,
    PS_RESP_VER_MAJOR,
    PS_RESP_VER_DOT,
    PS_RESP_VER_MINOR,
    PS_RESP_VER_END,
    PS_RESP_CODE_PRE,
    PS_RESP_CODE,
    PS_RESP_DESC_PRE,
    PS_RESP_DESC,
    PS_RESP_LINE_END,

    PS_REQ_START, // request start
    PS_REQ_METHOD,
    PS_REQ_URL_PRE,
    PS_REQ_URL_SCHEMA, // http
    PS_REQ_URL_SLASH,  // http:/
    PS_REQ_URL_SLASH2, // http://
    PS_REQ_URL_HOST,
    PS_REQ_SERVER,
    PS_REQ_SERVER_AT, //@
    PS_REQ_URL_PATH,
    PS_REQ_URL_QUERY, //?
    PS_REQ_URL_QUERY_KEY,
    PS_REQ_URL_FRAG, // #
    PS_REQ_URL_FRAGMENT,
    PS_REQ_HTTP_START, // HTTP/1.1
    PS_REQ_H,
    PS_REQ_HT,
    PS_REQ_HTT,
    PS_REQ_HTTP,
    PS_REQ_I,  // ICE
    PS_REQ_IC, // ICE
    PS_REQ_VER_MAJOR,
    PS_REQ_VER_DOT,
    PS_REQ_VER_MINOR,
    PS_REQ_VER_END,
    PS_REQ_LINE_END,

    PS_HEAD_FIELD_PRE,
    PS_HEAD_FIELD,
    PS_HEAD_2DOT,             // : head field & value 之间的间隔字符
    PS_HEAD_VALUE_WILL_END_X, // will empty value
    PS_HEAD_VALUE_END_X,      // empty value
    PS_HEAD_VALUE_PRE,
    PS_HEAD_VALUE,
    PS_HEAD_VALUE_WILL_END,
    PS_HEAD_VALUE_END,

    PS_CHUNK_SIZE_START,
    PS_CHUNK_SIZE,
    PS_CHUNK_SIZE_PARAM, // discard
    PS_CHUNK_SIZE_WILL_END,

    PS_HEAD_WILL_END, // \r\n\r

    /* @note 'PS_HEAD_DONE' must be the last 'header' state. All
     * states beyond this must be 'body' states. It is used for overflow
     * checking. See the @note PARSING_HEADER.
     */
    PS_HEAD_DONE,

    PS_CHUNK_DATA, // 只要有数据就需要回调，避免缓存区满时仍然不能触发回调
    PS_CHUNK_DATA_WILL_END,
    PS_CHUNK_DATA_END,
    PS_BODY_BOUNDARY, // 只要有数据就需要回调，避免缓存区满时仍然不能触发回调

    PS_BODY_HAS_LEN, // Content-Length header given and non-zero

    PS_BODY_EOF, // 无content-length/chunck等信息，靠断开连接表示body结束

    PS_MSG_DONE
};

enum EHttpHostState {
    s_http_host_dead = 1,
    s_http_userinfo_start,
    s_http_userinfo,
    s_http_host_start,
    s_http_host_v6_start,
    s_http_host,
    s_http_host_v6,
    s_http_host_v6_end,
    s_http_host_v6_zone_start,
    s_http_host_v6_zone,
    s_http_host_port_start,
    s_http_host_port
};


} // namespace net
} // namespace app
