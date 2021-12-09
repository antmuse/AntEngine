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


#include "Net/HTTP/HttpHead.h"

namespace app {
namespace net {

HttpHead::HttpHead() {
}


HttpHead::~HttpHead() {
}


bool HttpHead::isChunked()const {
    StringView nm("Transfer-Encoding", sizeof("Transfer-Encoding") - 1);
    const StringView& par = get(nm);
    return 7 == par.mLen && 0 == AppStrNocaseCMP("chunked", par.mData, sizeof("chunked") - 1);
}

void HttpHead::add(const StringView& key, const StringView& val) {
    HeadLine nd = {key, val};
    mData.emplaceBack(nd);
}

void HttpHead::add(const String& key, const String& val) {
    HeadLine nd = {key, val};
    mData.emplaceBack(nd);
}

void HttpHead::remove(const StringView& key, s32 cnt) {
    usz mx = mData.size();
    for (usz i = 0; i < mx; ++i) {
        if (key.mLen == mData[i].mKey.getLen() && 0 == AppStrNocaseCMP(mData[i].mKey.c_str(), key.mData, key.mLen)) {
            mData[i] = mData[mx - 1];
            mData.resize(mx - 1);
            if (--cnt < 1) {
                break;
            }
        }
    }
}


StringView HttpHead::get(const StringView& key, usz pos)const {
    StringView ret;
    size_t mx = mData.size();
    for (; pos < mx; ++pos) {
        if (key.mLen == mData[pos].mKey.getLen() && 0 == AppStrNocaseCMP(mData[pos].mKey.c_str(), key.mData, key.mLen)) {
            ret.set(mData[pos].mVal.c_str(), mData[pos].mVal.getLen());
            break;
        }
    }
    return ret;
}


}//namespace net
}//namespace app {
