#include "Net/HTTP/HttpURL.h"
#include "Net/HTTP/HttpParserDef.h"
#include "Converter.h"
#include "TMap.h"
#include "Logger.h"

namespace app {
namespace net {


static EHttpHostState parseHostChar(EHttpHostState s, const s8 ch) {
    switch (s) {
    case EHTTP_HOST_USER:
    case EHTTP_HOST_USER_START:
        if (ch == '@') {
            return EHTTP_HOST_START;
        }
        if (IS_USERINFO_CHAR(ch)) {
            return EHTTP_HOST_USER;
        }
        break;

    case EHTTP_HOST_START:
        if (ch == '[') {
            return EHTTP_HOST_V6_START;
        }
        if (IS_HOST_CHAR(ch)) {
            return EHTTP_HOST;
        }
        break;

    case EHTTP_HOST:
        if (IS_HOST_CHAR(ch)) {
            return EHTTP_HOST;
        }
        // break; fall

    case EHTTP_HOST_V6_END:
        if (ch == ':') {
            return EHTTP_HOST_PORT_START;
        }
        break;

    case EHTTP_HOST_V6:
        if (ch == ']') {
            return EHTTP_HOST_V6_END;
        }
        // break; fall

    case EHTTP_HOST_V6_START:
        if (IS_HEX(ch) || ch == ':' || ch == '.') {
            return EHTTP_HOST_V6;
        }
        if (s == EHTTP_HOST_V6 && ch == '%') {
            return EHTTP_HOST_V6_ZONE_START;
        }
        break;

    case EHTTP_HOST_V6_ZONE:
        if (ch == ']') {
            return EHTTP_HOST_V6_END;
        }
        // break; fall

    case EHTTP_HOST_V6_ZONE_START:
        /* RFC 6874 Zone ID consists of 1*( unreserved / pct-encoded) */
        if (IS_ALPHANUM(ch) || ch == '%' || ch == '.' || ch == '-' || ch == '_' || ch == '~') {
            return EHTTP_HOST_V6_ZONE;
        }
        break;

    case EHTTP_HOST_PORT:
    case EHTTP_HOST_PORT_START:
        if (IS_NUM(ch)) {
            return EHTTP_HOST_PORT;
        }
        break;

    default:
        break;
    }
    return EHTTP_HOST_DEAD;
}


HttpURL::HttpURL() {
    clear();
}


HttpURL::~HttpURL() {
}



bool HttpURL::assign(const s8* buf, usz sz) {
    mData.assign(buf, sz);
    return parser(0);
}


void HttpURL::append(const s8* buf, usz sz) {
    mData.append(buf, sz);
}


bool HttpURL::getParam(const String& key, String& val) {
    TMap<String, String>::Node* nd = mParams.find(key);
    if (!nd) {
        // val.resize();
        return false;
    }
    val = nd->getValue();
    return true;
}

usz HttpURL::sumCacheSize() const {
    usz ret = mData.size();
    if (!mParams.empty()) {
        // ret*3 for encodeURL
        for (TMap<String, String>::ConstIterator it = mParams.getConstIterator(); !it.atEnd(); ++it) {
            ret += it->getKey().size() * 3;
            ret += it->getValue().size() * 3;
            ++ret;
        }
    }
    return ret + 2;
}


void HttpURL::clear() {
    mPort = 0;
    mFieldSet = 0;
    memset(&mFieldData, 0, sizeof(mFieldData));
    mData.resize(0);
    mParams.clear();
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
    //TODO: should encode part by part
    mData.resize(encodeURL(uri, len, (s8*)mData.data(), mData.capacity()));
    return parser(0);
}


bool HttpURL::decode(const s8* uri, usz len) {
    if (!uri) {
        return false;
    }
    mData.assign(uri, len);
    bool ret = parser(0);
    // should parse before decode to avoid read UTF8 bytes.
    if (ret) {
        mFieldData[UF_PATH].mLen = decodeURL(mData.data() + mFieldData[UF_PATH].mOffset, mFieldData[UF_PATH].mLen);
        // mData.resize(); // TODO: shrink data len;
    }
    return ret;
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
        ret.set((s8*)mData.data() + mFieldData[idx].mOffset, mFieldData[idx].mLen);
    }
    return ret;
}

bool HttpURL::parser(s32 is_connect) {
    s32 ret = parseURL(mData.data(), mData.size(), is_connect);
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

    EHttpHostState s = found_at ? EHTTP_HOST_USER_START : EHTTP_HOST_START;

    for (const s8* p = buf + mFieldData[UF_HOST].mOffset; p < buf + buflen; p++) {
        EHttpHostState new_s = parseHostChar(s, *p);

        if (new_s == EHTTP_HOST_DEAD) {
            return 1;
        }

        switch (new_s) {
        case EHTTP_HOST:
            if (s != EHTTP_HOST) {
                mFieldData[UF_HOST].mOffset = (u16)(p - buf);
            }
            mFieldData[UF_HOST].mLen++;
            break;

        case EHTTP_HOST_V6:
            if (s != EHTTP_HOST_V6) {
                mFieldData[UF_HOST].mOffset = (u16)(p - buf);
            }
            mFieldData[UF_HOST].mLen++;
            break;

        case EHTTP_HOST_V6_ZONE_START:
        case EHTTP_HOST_V6_ZONE:
            mFieldData[UF_HOST].mLen++;
            break;

        case EHTTP_HOST_PORT:
            if (s != EHTTP_HOST_PORT) {
                mFieldData[UF_PORT].mOffset = (u16)(p - buf);
                mFieldData[UF_PORT].mLen = 0;
                mFieldSet |= (1 << UF_PORT);
            }
            mFieldData[UF_PORT].mLen++;
            break;

        case EHTTP_HOST_USER:
            if (s != EHTTP_HOST_USER) {
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
    case EHTTP_HOST_START:
    case EHTTP_HOST_V6_START:
    case EHTTP_HOST_V6:
    case EHTTP_HOST_V6_ZONE_START:
    case EHTTP_HOST_V6_ZONE:
    case EHTTP_HOST_PORT_START:
    case EHTTP_HOST_USER:
    case EHTTP_HOST_USER_START:
        return 1;
    default:
        break;
    }

    return 0;
}


s32 HttpURL::parseURL(const s8* buf, usz buflen, s32 is_connect) {
    mPort = 0;
    mFieldSet = 0;
    memset(&mFieldData, 0, sizeof(mFieldData));

    if (buflen == 0) {
        return EE_ERROR;
    }

    s32 found_at = 0;
    EPareState s = is_connect ? PS_REQ_URL_HOST : PS_REQ_URL_PRE;
    EHttpUrlFields uf;
    EHttpUrlFields old_uf = UF_MAX;
    for (const s8* p = buf; p < buf + buflen; p++) {
        s = AppParseUrlChar(s, *p);

        // Figure out the next field that we're operating on
        switch (s) {
        case PS_DEAD:
            return EE_ERROR;

        // Skip delimeters
        case PS_REQ_URL_SLASH:
        case PS_REQ_URL_SLASH2:
        case PS_REQ_URL_HOST:
        case PS_REQ_URL_QUERY_KEY_PRE:
        case PS_REQ_URL_QUERY_VAL_PRE:
        case PS_REQ_URL_FRAG_PRE:
            continue;

        case PS_REQ_URL_SCHEMA:
            uf = UF_SCHEMA;
            break;

        case PS_REQ_SERVER_AT:
            found_at = 1;
            // break; fall through
        case PS_REQ_SERVER:
            uf = UF_HOST;
            break;

        case PS_REQ_URL_PATH:
            uf = UF_PATH;
            break;

        case PS_REQ_URL_QUERY_KEY:
        case PS_REQ_URL_QUERY_VAL:
            uf = UF_QUERY;
            break;
        
        case PS_REQ_URL_FRAG:
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
        return EE_ERROR;
    }

    if (mFieldSet & (1 << UF_HOST)) {
        if (parseHost(buf, found_at) != 0) {
            return EE_ERROR;
        }
    }

    /* CONNECT requests can only contain "hostname:port" */
    if (is_connect && mFieldSet != ((1 << UF_HOST) | (1 << UF_PORT))) {
        return EE_ERROR;
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
    s8* pos = const_cast<s8*>(mData.data()) + mFieldData[UF_PATH].mOffset;
    mFieldData[UF_PATH].mLen = static_cast<u16>(AppSimplifyPath(pos, pos + mFieldData[UF_PATH].mLen));
    return EE_OK;
}


} // namespace net
} // namespace app
