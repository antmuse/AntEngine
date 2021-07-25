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


#ifndef APP_HTTPCOOKIE_H
#define	APP_HTTPCOOKIE_H

#include "Strings.h"
#include "Packet.h"

namespace app {
namespace net {

/**
* @brief http Cookie
*
* Set-Cookie : BAIDUID=284AECB0B263FB20520C210E0CE9B0E0:FG=1; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com
*/
class HttpCookie {
public:
    HttpCookie();

    ~HttpCookie();

    void setSecure(bool it) {
        if (it) {
            mFlags |= 1;
        } else {
            mFlags &= ~1;
        }
    }

    bool getSecure()const {
        return (mFlags & 1) > 0;
    }

    void setHttpOnly(bool it) {
        if (it) {
            mFlags |= 2;
        } else {
            mFlags &= ~2;
        }
    }

    bool getHttpOnly()const {
        return (mFlags & 2) > 0;
    }


private:
    String mDomain;
    String mPath;
    String mValue;
    s64 mExpireTime;
    Packet mBuf;
    s8 mFlags;
};


} //namespace net
} //namespace app

#endif //APP_HTTPCOOKIE_H