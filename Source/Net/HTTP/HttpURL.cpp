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


#include "Net/HTTP/HttpURL.h"
#include "Converter.h"

namespace app {
namespace net {

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
    mData.setLen(0);
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
            } else if ((ch < '0' && ch != '-' && ch != '.') ||
                (ch < 'A' && ch > '9') ||
                (ch > 'Z' && ch < 'a' && ch != '_') ||
                (ch > 'z')) {
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
            } else if (*data == '%' && len >= 2 &&
                isxdigit(data[1]) && isxdigit(data[2])) {
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
    mData.setLen(0);
    mData.reserve(len * 3 + 4);
    mData.setLen(encodeURL(uri, len, (s8*)mData.c_str(), mData.getAllocated()));
    return parser();
}


bool HttpURL::decode(const s8* uri, usz len) {
    if (!uri) {
        return false;
    }
    mData.setLen(0);
    mData.append(uri, len);
    mData.setLen(decodeURL((s8*)mData.c_str(), mData.getLen()));
    return parser();
}

bool HttpURL::isHttps()const {
    StringView ret = getNode(UF_SCHEMA);
    if (ret.mData && 5 == ret.mLen) {
        return (App4Char2S32(ret.mData) == App4Char2S32("http") && 's' == ret.mData[4]);
    }
    return false;
}

StringView HttpURL::getNode(u32 idx)const {
    StringView ret;
    if (idx < UF_MAX) {
        ret.set((s8*)mData.c_str() + mFieldData[idx].mOffset, mFieldData[idx].mLen);
    }
    return ret;
}

bool HttpURL::parser() {
    s32 ret = parseURL(mData.c_str(), mData.getLen(), 0);
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

u16 HttpURL::getPort()const {
    return mPort;
}












/////////

}//namespace net
}//namespace app
