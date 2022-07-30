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


#ifndef APP_HTTPURL_H
#define	APP_HTTPURL_H


#include "Strings.h"

namespace app {
namespace net {

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


class HttpURL {
public:
    static usz decodeURL(s8* str, usz len);
    static usz encodeURL(const s8* from, usz len, s8* out, usz olength);

    HttpURL();

    ~HttpURL();

    const String& get()const {
        return mData;
    }

    bool encode(const s8* uri, usz len);

    bool decode(const s8* uri, usz len);

    void clear();

    bool parser();

    void append(const s8* buf, size_t sz);

    bool isHttps()const;

    u16 getPort()const;

    StringView getHost()const {
        return getNode(UF_HOST);
    }

    StringView getPath()const {
        return getNode(UF_PATH);
    }

    StringView getQuery()const {
        return getNode(UF_QUERY);
    }

    StringView getFragment()const {
        return getNode(UF_FRAGMENT);
    }

    StringView getNode(u32 idx)const;


private:
    String mData;

    u16 mFieldSet;           /* Bitmask of (1 << UF_*) values */
    u16 mPort;                /* Converted UF_PORT string */
    struct {
        u16 mOffset;          /* Offset into buffer in which field starts */
        u16 mLen;             /* Length of run in buffer */
    } mFieldData[UF_MAX];

    /* Parse a URL; return nonzero on failure */
    s32 parseURL(const s8* buf, usz buflen, s32 is_connect);

    s32 parseHost(const s8* buf, s32 found_at);
};

}//namespace net
}//namespace app

#endif //APP_HTTPURL_H
