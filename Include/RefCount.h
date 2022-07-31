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


#ifndef APP_REFCOUNT_H
#define	APP_REFCOUNT_H

#include <atomic>
#include "Config.h"

namespace app {

/**
 * @brief 引用计数
 *        不推荐多重继承, 子类形成菱形继承时(多继承)注意需虚继承自此类, eg:
 *        class SonA : virtual public TRefCount {};
 *        class SonB : virtual public TRefCount {};
 *        class Grandson : public SonA, public SonB {};
 */
template <class T>
class TRefCount {
public:
    TRefCount() : mRefCount(1) {
    }

    virtual ~TRefCount() {
        DASSERT(0 == mRefCount);
    }

    s32 getRefCount() const {
        return mRefCount;
    }

    virtual void grab() const {
        DASSERT(mRefCount > 0);
        ++mRefCount;
    }

    virtual s32 drop() const {
        DASSERT(mRefCount > 0);
        s32 ret = --mRefCount;
        if (0 == ret) {
            delete this;
        }
        return ret;
    }

private:
    mutable T mRefCount;
};


using RefCount = TRefCount<s32>;
using RefCountAtomic = TRefCount<std::atomic<s32>>;

} //namespace app

#endif //APP_REFCOUNT_H