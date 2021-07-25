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


#ifndef APP_HASHFUNCTIONS_H
#define APP_HASHFUNCTIONS_H


#include "Config.h"

namespace app {
typedef u32(*AppHashCallback32)(const void*, u64);
typedef u64(*AppHashCallback64)(const void*, u64);

//////////////////////////////////sip hash//////////////////////////////////
u64 AppHashSIP(const void* key, u64 len);

u64 AppCaseHashSIP(const void* buf, u64 len);

//@param seed 16 bytes array
void AppSetHashSeedSIP(const void* seed);

//@return seed 16 bytes array, can't delete/free
void* AppGetHashSeedSIP();


//////////////////////////////////murmur hash//////////////////////////////////
void AppSetHashSeedMurmur(const u32 seed);

u32& AppGetHashSeedMurmur();

//@param out 128 bits cache
void AppHashMurmur128(const void* buf, u64 len, void* out);
u64 AppHashMurmur64(const void* buf, u64 len);
u32 AppHashMurmur32(const void* buf, u64 len);

} //namespace app

#endif //APP_HASHFUNCTIONS_H
