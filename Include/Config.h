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


#ifndef APP_CONFIG_H
#define	APP_CONFIG_H

////////////////////////////////////////////////////////////////////////////////

//Configure: Version
#define DVERSION_MAJOR 1
#define DVERSION_MINOR 0
#define DVERSION_MICRO 0
#define DVERSION_MINI  0
#define DVERSION_NAME  "1.0.0.0"
#define DVERSION_ID    10000

#define DUSE_OPENSSL
#define DUSE_ZLIB
#define DUSE_MYSQL


#if !defined(DDEBUG)
#if defined(_DEBUG) || defined(DEBUG)
#define DDEBUG
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

//Configure: Platform
#if !defined(DOS_WINDOWS)
#if defined(_WIN64)  || defined(WIN64) || defined(_WIN32) || defined(WIN32)
#define DOS_WINDOWS
#define DINLINE inline
#define DFINLINE __forceinline
#define DALIGN(N) __declspec(align(N))

#define LIKELY(X) (X)
#define UNLIKELY(X) (X)

#if defined(_UNICODE) || defined(UNICODE)
#define DWCHAR_SYS
#endif

#endif
#endif


#if !defined( DOS_LINUX )
#if (defined(_UNIX) || defined(__linux)) && !defined(ANDROIDNDK) && !defined(__ANDROID_)
#define DOS_LINUX
#define DINLINE inline
#define DFINLINE __attribute__((always_inline)) inline
#define DALIGN(N) __attribute__((__aligned__((N))))
#ifdef __GNUC__
#define LIKELY(X) __builtin_expect(!!(X), 1)
#define UNLIKELY(X) __builtin_expect(!!(X), 0)
#else
#define LIKELY(X) (X)
#define UNLIKELY(X) (X)
#endif
#include <stdlib.h>
#endif

#define DUSE_IO_URING

#endif


#if !defined( DOS_ANDROID )
#if defined(ANDROIDNDK) || defined(__ANDROID_)
#define DOS_ANDROID
#define DINLINE inline
#define DFINLINE __attribute__((always_inline)) inline
#define DALIGN(N) __attribute__((__aligned__((N))))
#define LIKELY(X) (X)
#define UNLIKELY(X) (X)
#endif
#endif


#if defined(_WIN64)  || defined(WIN64) || defined(__LP64__)  || defined(__amd64) || defined(__x86_64__) || defined(__aarch64__)
#define DOS_64BIT
#else
#if defined(_WIN32) || defined(WIN32) || defined(__unix__)  || defined(__i386__)
#define DOS_32BIT
#endif
#endif


//Define byte order
#ifndef DENDIAN_BIG
#ifdef __BIG_ENDIAN__
#define DENDIAN_BIG
#endif
#endif


#ifndef DUSE_GCC
#if defined(__GNUC__) || defined(__GCCXML__)
#define DUSE_GCC
#endif
#endif

#ifndef DUSE_MSVC
#if defined(_MSC_VER)
#if _MSC_VER >= 1600
#define DUSE_MSVC
#else
#error This version of the MSVC compilier is not supported
#endif
#endif
#endif


#ifdef DDEBUG
#include <assert.h>
#define DASSERT(T)  assert(T)
#else
#define DASSERT(T)
#endif

#if (_MSC_VER >= 1310) //vs 2003 or higher
#define DDEPRECATED __declspec(deprecated)
#elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
//all versions above 3.0 should support this feature
#define DDEPRECATED  __attribute__ ((deprecated))
#else
#define DDEPRECATED
#endif


////////////////////////////////////////////////////////////////////////////////
//Configure: API
#define DEXTC extern "C"

#ifdef DNO_STATIC_LIB
#undef DSTATIC_LIB
#endif


#ifdef DSTATIC_LIB
#define DAPI
#else
#ifdef DEXPORTS
// We are building this library
#define DAPI __declspec(dllexport)
#else
// We are using this library
#define DAPI __declspec(dllimport)
#endif
#endif


#if defined(DCALL_STD)
#define DCALL __stdcall
#else
#define DCALL __cdecl
#endif


#ifndef DSIZEOF
# define DSIZEOF(a) (sizeof(a) / sizeof((a)[0]))
#endif



#define DOFFSET(T, NAME) ((size_t)(&(((T*)0)->NAME)))
#define DOFFSETP(PT, NAME) (((size_t)(&(PT->NAME))) - ((size_t)(PT)))
#define DGET_HOLDER(PT, T, NAME) ((T*)(((char*)((T*)PT)) - ((size_t) &((T*)0)->NAME)))
////////////////////////////////////////////////////////////////////////////////



namespace app {

using s8 = char;
using u8 = unsigned char;
using s16 = short;
using u16 = unsigned short;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;

#if defined(DOS_64BIT)
using usz = u64;
using ssz = s64;
#else
using usz = u32;
using ssz = s32;
#endif


#if defined(DWCHAR_SYS)
using tchar = wchar_t;
#define DSTR(N) L##N
#define DSLEN(N) wcslen(N)

#else

using tchar = char;
#define DSTR(N) N
#define DSLEN(N) strlen(N)

#endif


DFINLINE u16 AppSwap16(u16 it) {
    return (it << 8) | (it >> 8);
}


DFINLINE u32 AppSwap32(u32 it) {
    return (((it & 0x000000FFU) << 24) | ((it & 0xFF000000U) >> 24)
        | ((it & 0x0000FF00U) << 8) | ((it & 0x00FF0000U) >> 8));
}

DFINLINE u64 AppSwap64(u64 it) {
    return (((it & 0x00000000000000FFULL) << 56) | ((it & 0xFF00000000000000ULL) >> 56)
        | ((it & 0x000000000000FF00ULL) << 40) | ((it & 0x00FF000000000000ULL) >> 40)
        | ((it & 0x0000000000FF0000ULL) << 24) | ((it & 0x0000FF0000000000ULL) >> 24)
        | ((it & 0x00000000FF000000ULL) << 8) | ((it & 0x000000FF00000000ULL) >> 8));
}


/**
* @param bytes 待对字节数
* @param num 对齐单位(必须是2的幂次方)
* @return 对齐字节数
*/
DFINLINE usz AppAlignSize(usz bytes, usz num) {
    return (bytes + (num - 1)) & (~(num - 1));
}


/**
* @param pot 待对齐指针
* @param num 对齐单位(必须是2的幂次方)
* @return 对齐指针
*/
template<class T>
DFINLINE T* AppAlignPoint(T* pot, usz num) {
    return (T*)(((size_t)(pot)+(num - 1)) & (~(num - 1)));
}



template <class T>
DFINLINE void AppSwap(T& a, T& b) {
    T c = a;
    a = b;
    b = c;
}


#define DMIN(a,b) ((a) < (b) ? (a) : (b))
#define DMAX(a,b) ((a) > (b) ? (a) : (b))
#define DCLAMP(V,L,H) (DMIN(DMAX(V, L), H))

template<class T>
DFINLINE const T& AppMin(const T& a, const T& b) {
    return a < b ? a : b;
}


template<class T>
DFINLINE const T& AppMax(const T& a, const T& b) {
    return a < b ? b : a;
}


template <class T>
DFINLINE const T& AppClamp(const T& value, const T& low, const T& high) {
    return AppMin(AppMax(value, low), high);
}


DFINLINE bool AppIsBitON(const void* a, usz i) {
    return !!(((unsigned char*)a)[i >> 3] & (1 << (i & 7)));
}

DFINLINE void AppSetBitON(void* a, usz i) {
    ((unsigned char*)a)[i >> 3] |= (1 << (i & 7));
}

DFINLINE void AppSetBitOFF(void* a, usz i) {
    ((unsigned char*)a)[i >> 3] &= ~(1 << (i & 7));
}


}//namespace app

#endif	//APP_CONFIG_H

