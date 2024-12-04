#include "Net/HTTP/HttpURL.h"
#include "Net/HTTP/HttpParserDef.h"
#include "Converter.h"

namespace app {
namespace net {

// not strict check
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
        // break; fall

    case s_http_host_v6_end:
        if (ch == ':') {
            return s_http_host_port_start;
        }
        break;

    case s_http_host_v6:
        if (ch == ']') {
            return s_http_host_v6_end;
        }
        // break; fall

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
        // break; fall

    case s_http_host_v6_zone_start:
        /* RFC 6874 Zone ID consists of 1*( unreserved / pct-encoded) */
        if (IS_ALPHANUM(ch) || ch == '%' || ch == '.' || ch == '-' || ch == '_' || ch == '~') {
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


HttpURL::HttpURL() {
    clear();
}

HttpURL::~HttpURL() {
}

void HttpURL::append(const s8* buf, size_t sz) {
    mData.append(buf, sz);
}


void HttpURL::clear() {
    mFieldSet = 0;
    memset(&mFieldData, 0, sizeof(mFieldData));
    mData.resize(0);
}


usz HttpURL::encodeURL(const s8* from, usz len, s8* out, usz olen) {
    const static u8 hexchars[] = "0123456789ABCDEF";
    register u8 ch;
    s8* curr = out;
    const s8* const end = from + len;
    const s8* const oend = out + olen;
    if (from && out && olen > len) {
        while (from < end && curr + 3 < oend) {
            ch = *from++;
            if (ch == ' ') {
                *curr++ = '+';
            } else if ((ch < '0' && ch != '-' && ch != '.') || (ch < 'A' && ch > '9')
                       || (ch > 'Z' && ch < 'a' && ch != '_') || (ch > 'z')) {
                curr[0] = '%';
                curr[1] = hexchars[ch >> 4];
                curr[2] = hexchars[ch & 0xF];
                curr += 3;
            } else {
                *curr++ = ch;
            }
        }
        *curr = 0;
    }
    return (usz)(curr - out);
}


usz HttpURL::decodeURL(s8* str, usz len) {
    s8* dest = str;
    s8* data = str;
    if (str) {
        while (len--) {
            if (*data == '+') {
                *dest = ' ';
            } else if (*data == '%' && len >= 2 && isxdigit(data[1]) && isxdigit(data[2])) {
                *dest = (s8)((App16CharToU32(data[1]) << 4) | App16CharToU32(data[2]));
                data += 2;
                len -= 2;
            } else {
                *dest = *data;
            }
            data++;
            dest++;
        }
        *dest = '\0';
    }
    return dest - str;
}


bool HttpURL::encode(const s8* uri, usz len) {
    if (!uri || 0 == len) {
        return false;
    }
    mData.resize(0);
    mData.reserve(len * 3 + 4);
    mData.resize(encodeURL(uri, len, (s8*)mData.c_str(), mData.capacity()));
    return parser();
}


bool HttpURL::decode(const s8* uri, usz len) {
    if (!uri) {
        return false;
    }
    mData.resize(0);
    mData.append(uri, len);
    mData.resize(decodeURL((s8*)mData.c_str(), mData.size()));
    return parser();
}

bool HttpURL::isHttps() const {
    StringView ret = getNode(UF_SCHEMA);
    if (ret.mData && 5 == ret.mLen) {
        return (App4Char2S32(ret.mData) == App4Char2S32("http") && 's' == ret.mData[4]);
    }
    return false;
}

StringView HttpURL::getNode(u32 idx) const {
    StringView ret;
    if (idx < UF_MAX) {
        ret.set((s8*)mData.c_str() + mFieldData[idx].mOffset, mFieldData[idx].mLen);
    }
    return ret;
}

bool HttpURL::parser() {
    s32 ret = parseURL(mData.c_str(), mData.size(), 0);
    if (0 == ret) {
        StringView ret = getNode(UF_SCHEMA);
        ret.toLower();
        if (0 == mPort) {
            ret = getNode(UF_PORT);
            mPort = ret.mLen > 0 ? App10StrToU32(ret.mData) : (isHttps() ? 443 : 80);
        }
    } else {
        clear();
    }
    return 0 == ret;
}

u16 HttpURL::getPort() const {
    return mPort;
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

    mPort = 0;
    mFieldSet = 0;
    memset(&mFieldData, 0, sizeof(mFieldData));

    s32 found_at = 0;
    EPareState s = is_connect ? s_req_server_start : s_req_spaces_before_url;
    EHttpUrlFields uf;
    EHttpUrlFields old_uf = UF_MAX;
    for (const s8* p = buf; p < buf + buflen; p++) {
        s = AppParseUrlChar2(s, *p);

        // Figure out the next field that we're operating on
        switch (s) {
        case s_dead:
            return 1;

            // Skip delimeters
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
            // break; fall through
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
    if ((mFieldSet & (1 << UF_SCHEMA)) && (mFieldSet & (1 << UF_HOST)) == 0) {
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

    // make safe path
    s8* pos = const_cast<s8*>(mData.c_str()) + mFieldData[UF_PATH].mOffset;
    mFieldData[UF_PATH].mLen = static_cast<u16>(AppSimplifyPath(pos, pos + mFieldData[UF_PATH].mLen));
    return 0;
}


} // namespace net
} // namespace app
