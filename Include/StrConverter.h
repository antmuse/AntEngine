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


#ifndef APP_STRCONVERTER_H
#define	APP_STRCONVERTER_H

#include "Config.h"

namespace app {

/**
 * @brief usage:
 *   wchar_t dest[8];
 *   size_t strlen = AppUTF8ToWchar("src",dest,sizeof(dest));
 *
 * @param src input str
 * @param dest output cache
 * @param osize sizeof output cache
 * @return length of dest wstring.
 */
usz AppUTF8ToWchar(const s8* src, wchar_t* dest, const usz osize);

/**
 * @brief usage:
 *   wchar_t dest[8];
 *   size_t strlen = AppUTF8ToWchar("src",dest,sizeof(dest));
 *
 * @param src input str
 * @param dest output cache
 * @param osize sizeof output cache
 * @return length of dest string.
 */
usz AppWcharToUTF8(const wchar_t* src, s8* dest, const usz osize);

/**
* @see AppUTF8ToWchar()
*/
usz AppUTF8ToUCS4(const s8* src, u32* dest, usz osize);

/**
* @see AppUTF8ToWchar()
*/
usz AppUTF8ToUCS2(const s8* src, u16* dest, usz osize);

/**
* @see AppWcharToUTF8()
*/
usz AppUCS4ToUTF8(const u32* src, s8* dest, usz osize);

/**
* @see AppWcharToUTF8()
*/
usz AppUCS2ToUTF8(const u16* src, s8* dest, usz osize);

/**
 * @return len of the hex str, exclude the tail '\0'. */
usz AppBufToHex(const void* src, usz insize, s8* dest, usz osize);
usz AppHexToBuf(const s8* src, usz insize, void* dest, usz osize);

void AppStrConverterInit();
usz AppUTF8ToGBK(const s8* from, s8* out, usz outsz);
usz AppGbkToUTF8(const s8* from, s8* out, usz outsz);


} // end namespace app

#endif //APP_STRCONVERTER_H