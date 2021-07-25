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


#ifndef APP_CHECKCRC_H
#define APP_CHECKCRC_H

#include "Config.h"

namespace app {

/*
* @brief CRC16 implementation according to CCITT standards.
*/
class CheckCRC16 {
public:

    CheckCRC16() : mResult(0) {
    }

    ~CheckCRC16() {
    }

    u16 add(const void* buffer, u32 size);

    void clear() {
        mResult = 0;
    }

    u16 getResult()const {
        return mResult;
    }

private:
    u16 mResult;
};


class CheckCRC32 {
public:
    CheckCRC32() : mResult(0xFFFFFFFF) {
    }
    ~CheckCRC32() {
    }

    void clear() {
        mResult = 0xFFFFFFFF;
    }

    u32 getResult()const {
        return mResult ^ 0xFFFFFFFF;
    }

    u32 add(const void* buffer, u64 iSize);

private:
    u32 mResult;
};


class CheckCRC64 {
public:
    CheckCRC64() : mResult(0) {
    }
    ~CheckCRC64() {
    }

    void clear() {
        mResult = 0;
    }

    u64 getResult()const {
        return mResult;
    }

    u64 add(const void* buffer, u64 iSize);

private:
    u64 mResult;
};

}//namespace app
#endif //APP_CHECKCRC_H
