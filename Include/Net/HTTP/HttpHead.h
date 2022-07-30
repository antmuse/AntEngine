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


#ifndef APP_HTTPHEAD_H
#define	APP_HTTPHEAD_H

#include "Strings.h"
#include "TVector.h"

namespace app {
namespace net {

class HeadLine {
public:
    String mKey;
    String mVal;
    HeadLine() { }
    ~HeadLine() { }
    HeadLine(const StringView& kk, const StringView& vv) :
        mKey(kk), mVal(vv) {
    }
    HeadLine(const String& kk, const String& vv) :
        mKey(kk), mVal(vv) {
    }
    HeadLine(const HeadLine& it) :
        mKey(it.mKey), mVal(it.mVal) {
    }
    HeadLine(HeadLine&& it)noexcept
        : mKey(std::move(it.mKey))
        , mVal(std::move(it.mVal)) {
    }
    HeadLine& operator=(HeadLine&& it) noexcept {
        if (&it != this) {
            mKey = std::move(it.mKey);
            mVal = std::move(it.mVal);
        }
        return *this;
    }
    HeadLine& operator=(const HeadLine& it) {
        if (&it != this) {
            mKey = it.mKey;
            mVal = it.mVal;
        }
        return *this;
    }
};



class HttpHead {
public:
    HttpHead();

    ~HttpHead();
    
    void writeLength(usz sz) {
        s8 tmp[128];
        StringView key("Content-Length",sizeof("Content-Length")-1);
        StringView val(tmp,snprintf(tmp, sizeof(tmp), "%llu", sz));
        add(key,val);
    }

    void writeContentRange(usz total, usz start, usz stop) {
        s8 tmp[128];
        StringView key("Content-Range",sizeof("Content-Range")-1);
        StringView val(tmp,snprintf(tmp, sizeof(tmp), "bytes %llu-%llu/%llu", start, stop, total));
        add(key,val);
    }

    //@brief default is "text/html; charset=utf-8"
    void writeDefaultContentType() {
        StringView key("Content-Type",sizeof("Content-Type")-1);
        StringView val("text/html;charset=utf-8",sizeof("text/html;charset=utf-8")-1);
        add(key,val);
    }

    void writeContentType(const StringView& val) {
        StringView key("Content-Type",sizeof("Content-Type")-1);
        add(key,val);
    }

    void writeKeepAlive(bool it) {
        StringView key("Connection",sizeof("Connection")-1);
        StringView val("keep-alive",sizeof("keep-alive")-1);
        if (!it) {
            val.set("close",sizeof("close")-1);
        }
        add(key,val);
    }

    void writeChunked() {
        StringView key("Transfer-Encoding",sizeof("Transfer-Encoding")-1);
        StringView val("chunked",sizeof("chunked")-1);
        add(key,val);
    }

    //Transfer-Encoding : chunked
    bool isChunked()const;

    void add(const StringView& key, const StringView& val);
    void add(const String& key, const String& val);

    void remove(const StringView& key, s32 cnt = 1);

    StringView get(const StringView& key, usz pos = 0)const;

    void clear() {
        mData.clear();
    }

    TVector<HeadLine>& getData() {
        return mData;
    }

    HeadLine& operator[](const usz idx) {
        DASSERT(idx < mData.size());
        return mData[idx];
    }

    const HeadLine& operator[](const usz idx)const {
        DASSERT(idx < mData.size());
        return mData[idx];
    }

    usz size()const {
        return mData.size();
    }

private:
    TVector<HeadLine> mData;
};

}//namespace net
}//namespace app

#endif //APP_HTTPHEAD_H
