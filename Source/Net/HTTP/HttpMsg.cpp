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


#include "Net/HTTP/HttpMsg.h"
#include <assert.h>

namespace app {
namespace net {
static const s32 G_MAX_BODY = 1024 * 1024 * 10;

HttpMsg::HttpMsg() :
    mFlag(0),
    mCache(512),
    mHeadSize(0){
}


HttpMsg::~HttpMsg() {
}


StringView HttpMsg::getMethodStr(http_method it) {
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



s32 HttpMsg::onHeadFinish(s32 flag, ssz bodySize) {
    mFlag = flag;
    mHeadSize = (u32)mCache.size();
    if (bodySize > 0) {
        if (bodySize < G_MAX_BODY) {
            mCache.reallocate(mCache.size() + bodySize);
        } else {
            return 1;
        }
    }//else no body
    return 0;
}


void HttpMsg::writeBody(const void* buf, usz bsz, s32 flag) {
    if (0 == flag) {
        mCache.reallocate(mCache.size() + 128);
        usz len = snprintf(mCache.getWritePointer(), mCache.getWriteSize(), "Content-Length:%llu\r\n\r\n", bsz);
        mCache.resize(mCache.size() + len);
    }
    mCache.write(buf, bsz);
}

void HttpMsg::writeHead(const s8* name, const s8* value, s32 flag) {
    if (name && value) {
        usz klen = strlen(name);
        usz vlen = strlen(value);
        if (klen > 0 && vlen > 0) {
            mCache.write(name, klen);
            mCache.writeU8(':');
            if (0x1 & flag) {
                StringView kk, vv;
                usz used = mCache.size();
                kk.mData = (s8*)(used - 1 - klen);
                kk.mLen = klen;
                vv.mData = (s8*)used;
                vv.mLen = vlen;
                mHead.add(kk, vv);
            }
            mCache.write(value, vlen);
            mCache.writeU16(App2Char2S16("\r\n"));
        }
    }
    if (0x3 & flag) {
        mHeadSize = (u32)mCache.size();
        mCache.writeU16(App2Char2S16("\r\n"));
    }
}

}//namespace net
}//namespace app
