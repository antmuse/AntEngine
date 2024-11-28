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
    s_dead = 1, // important that this is > 0

    s_start_resp_or_req,
    s_res_or_resp_H,
    s_start_resp, // response start
    s_res_H,
    s_res_HT,
    s_res_HTT,
    s_res_HTTP,
    s_res_http_major,
    s_res_http_dot,
    s_res_http_minor,
    s_res_http_end,
    s_res_first_status_code,
    s_res_status_code,
    s_res_status_start,
    s_res_status,
    s_res_line_almost_done,

    s_start_req, // request start

    s_req_method,
    s_req_spaces_before_url,
    s_req_schema,
    s_req_schema_slash,
    s_req_schema_slash2,
    s_req_server_start,
    s_req_server,
    s_req_server_with_at,
    s_req_path,
    s_req_query_string_start,
    s_req_query_string,
    s_req_fragment_start,
    s_req_fragment,
    s_req_http_start,
    s_req_http_H,
    s_req_http_HT,
    s_req_http_HTT,
    s_req_http_HTTP,
    s_req_http_I,
    s_req_http_IC,
    s_req_http_major,
    s_req_http_dot,
    s_req_http_minor,
    s_req_http_end,
    s_req_line_almost_done,

    s_header_field_start,
    s_header_field,
    s_header_value_discard_ws, // 丢弃head field & value 之间的间隔字符
    s_header_value_discard_ws_almost_done,
    s_header_value_discard_lws, // 丢弃value末尾的间隔字符
    s_header_value_start,
    s_header_value,
    s_header_value_lws,

    s_header_almost_done,
    s_chunk_size_start,
    s_chunk_size,
    s_chunk_parameters,
    s_chunk_size_almost_done,

    s_headers_almost_done,
    s_headers_done,
    /*@note 's_headers_done' must be the last 'header' state. All
     * states beyond this must be 'body' states. It is used for overflow
     * checking. See the @note PARSING_HEADER.
     */

    s_chunk_data, // 只要有数据就需要回调，避免缓存区满时仍然不能触发回调
    s_chunk_data_almost_done,
    s_chunk_data_done,

    s_boundary_body, // 只要有数据就需要回调，避免缓存区满时仍然不能触发回调

    s_body_identity,     // content_length有效
    s_body_identity_eof, // 无content-length/chunck等信息，靠断开连接表示body结束

    s_message_done
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
