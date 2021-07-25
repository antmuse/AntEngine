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


#include "Net/HTTP/HttpResponse.h"

namespace app {
namespace net {

HttpResponse::HttpResponse() :
    mStatusCode(HTTP_STATUS_OK){
    //memcpy(mCache.getPointer(), "HTTP/1.1 200 OK\r\n", sizeof("HTTP/1.1 200 OK\r\n") - 1);
    //mCache.resize(sizeof("HTTP/1.1 200 OK\r\n") - 1);
}

HttpResponse::~HttpResponse() {
}


void HttpResponse::writeStatus(s32 val) {
    DASSERT(mCache.capacity() > 18);//18="HTTP/1.1 %d OK\r\n"
    mStatusCode = val; // AppClamp(val, 0, 999);
    usz tsz = snprintf(mCache.getPointer(), mCache.capacity(), "HTTP/1.1 %d OK\r\n", val);
    mCache.resize(tsz);
}

void HttpResponse::rewriteStatus(s32 val) {
    DASSERT(mCache.capacity() > 18);//18="HTTP/1.1 %d OK\r\n"
    mStatusCode = AppClamp(val, 0, 999);//TODO>>
    usz tsz = snprintf(mCache.getPointer(), mCache.capacity(), "HTTP/1.1 %d OK\r", mStatusCode);
    mCache[tsz] = '\n';
}

}//namespace net
}//namespace app
