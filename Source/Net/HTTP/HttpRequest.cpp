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


#include "Net/HTTP/HttpRequest.h"
#include "Logger.h"
#include "Net/NetAddress.h"

namespace app {
namespace net {

HttpRequest::HttpRequest() :
    mMethod(HTTP_GET) {
    //writeHead("Connection", "keep-alive");
    //writeHead("Connection", "close");
}

HttpRequest::~HttpRequest() {
}


s32 HttpRequest::writeGet(const String& req) {
    mCache.reset();
    mURL.append(req.c_str(), req.getLen());
    if (!mURL.parser()) {
        return EE_ERROR;
    }
    StringView buf = mURL.getPath();
    mCache.write("GET ", sizeof("GET ") - 1);
    mCache.write(buf.mData, mURL.get().getLen() - (buf.mData - mURL.get().c_str()));
    mCache.write(" HTTP/1.1\r\n", sizeof(" HTTP/1.1\r\n") - 1);

    buf = mURL.getHost();
    mCache.write("Host:", sizeof("Host:") - 1);
    mCache.write(buf.mData,buf.mLen);
    mCache.write("\r\n", sizeof("\r\n") - 1);

    usz mx = mHead.size();
    for (usz i = 0; i < mx; ++i) {
        const HeadLine& line = mHead[i];
        mCache.write(line.mKey.c_str(), line.mKey.getLen());
        mCache.write(":", 1);
        mCache.write(line.mVal.c_str(), line.mVal.getLen());
        mCache.write("\r\n", sizeof("\r\n") - 1);
    }
    mCache.write("\r\n", sizeof("\r\n") - 1);
    return EE_OK;
}

}//namespace net
}//namespace app
