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


#ifndef APP_BASE64CODER_H
#define APP_BASE64CODER_H

#include "Config.h"

namespace app {


#define APP_BASE64_ENCODE_OUT_SIZE(s)	(((s) + 2) / 3 * 4)
#define APP_BASE64_DECODE_OUT_SIZE(s)	(((s)) / 4 * 3)


class CodecBase64 {
public:

    static u32 getDecodeCacheSize(u32 inputSize) {
        return (((inputSize)) / 4 * 3);
    }

    static u32 getEncodeCacheSize(u32 inputSize) {
        return (((inputSize)+2) / 3 * 4);
    }

    /**
    *@brief encode base64 string.
    *@param inputData Input buffer.
    *@param inLength Input buffer size.
    *@note output cache size should be no less than result of getEncodeCacheSize().
    *@return How many bytes were written.
    */
    static u32 encode(const u8* inputData, u32 inLength, s8* outputData);

    /**
    *@brief decode base64 string.
    *@param inputData Input buffer.
    *@param inLength Input buffer size.
    *@note output cache size should be no less than result of getDecodeCacheSize().
    *@return How many bytes were written.
    */
    static u32 decode(const s8* inputData, u32 inLength, u8* outputData);


protected:
    CodecBase64() { }
    ~CodecBase64() { }
};

} //namespace app

#endif //APP_BASE64CODER_H