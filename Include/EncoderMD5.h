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


#ifndef APP_ENCODERMD5_H
#define APP_ENCODERMD5_H

#include "IntID.h"
#include "TString.h"

namespace app {

class EncoderMD5 {
public:
    constexpr static u8 GMD5_LENGTH = 16;

    EncoderMD5() {
        clear();
    }

    ~EncoderMD5() {
    }

    void clear();

    void add(const void* data, usz size);

    //@param buf at least 16 bytes buf;
    void finish(u8 buf[16]);

    //@return the result in hex str, 32 bytes;
    String finish();

    void finish(ID128& result) {
        finish((u8*)& result);
    }

private:
    const void* transform(const u8* data, usz size);

    u32 mCountLow, mCountHi;
    u32 mA, mB, mC, mD;
    u8 mBuffer[64];
    u32 mBlock[16];
};


}// end namespace app
#endif //APP_ENCODERMD5_H
