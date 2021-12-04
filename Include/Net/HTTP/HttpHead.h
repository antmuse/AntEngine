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

class HttpHead {
public:
    class HeadLine {
    public:
        String mKey;
        String mVal;
        HeadLine(){ }
        HeadLine(const String& kk, const String& vv) :
            mKey(kk), mVal(vv) {
        }
        HeadLine(const HeadLine& it) :
            mKey(it.mKey), mVal(it.mVal) {
        }
    };

    HttpHead();

    ~HttpHead();

    //Transfer-Encoding : chunked
    bool isChunked()const;

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
