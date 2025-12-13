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


#ifndef APP_STRINGS_H
#define APP_STRINGS_H

#include <string.h>
#include <string>
#include "StrConverter.h"
#include "TAllocator.h"


namespace app {

DFINLINE s16 App2Char2S16(const s8* it) {
    return *reinterpret_cast<const s16*>(it);
}

DFINLINE s32 App4Char2S32(const s8* it) {
    return *reinterpret_cast<const s32*>(it);
}

template <typename T>
DFINLINE static T App2Lower(T x) {
    return x >= 'A' && x <= 'Z' ? x | 32 : x;
}

template <typename T>
DFINLINE static T App2Upper(T x) {
    return x >= 'a' && x <= 'z' ? x & (~32) : x;
}

template <typename T>
static void AppStr2Upper(T* str, usz len) {
    for (; len > 0; --len) {
        if (*(str) >= 'a' && *(str) <= 'z') {
            *(str) &= (~32);
        }
        ++str;
    }
}

template <typename T>
static void AppStr2Lower(T* str, usz len) {
    for (; len > 0; --len) {
        if (*(str) >= 'A' && *(str) <= 'Z') {
            *(str) |= 32;
        }
        ++str;
    }
}


/**
 * @brief 字符串限长比较, char or wchar_t
 * @param s1 字符串1,不可传空指针
 * @param s2 字符串2,不可传空指针
 * @param n 限长（比较前n个字母）
 * @return [0]=(s1==s2)，[>0]=(s1>s2), [<0]=(s1<s2)
 */
template <class T>
static s32 AppStrNocaseCMP(const T* s1, const T* s2, usz n) {
    if (s1 == s2 || n == 0) {
        return 0;
    }
    T ca, cb;
    do {
        ca = (*s1++) | 32;
        cb = (*s2++) | 32;
    } while (--n > 0 && ca == cb && ca != '\0');
    return ca - cb;
}

template <class T>
static s32 AppStrCMP(const T* s1, const T* s2, usz n) {
    if (s1 == s2 || n == 0) {
        return 0;
    }
    T ca, cb;
    do {
        ca = *s1++;
        cb = *s2++;
    } while (--n > 0 && ca == cb && ca != '\0');
    return ca - cb;
}



/**
 * @brief Sunday算法，查找子串, 常用于字符串的模式匹配
 * @param txt 查找空间, 不可为null
 * @param txtLen 查找空间长度，单位=sizeof(T)
 * @param pattern 待查找内容, 不可为null
 * @param patLen 待查找内容长度，单位=sizeof(P)
 * @return 为\p txt中首次出现\p pattern的位置(>=0)，<0则未找到
 */
template <typename T, typename P>
static ssz AppSundayFind(const T* txt, ssz txtLen, const P* pattern, ssz patLen) {
    DASSERT(txt && pattern);
    if (patLen > 0 && txtLen >= patLen) {
        txtLen -= patLen;
        ssz i, j, pos;
        T ch;
        for (ssz start = 0; start <= txtLen;) {
            for (i = start, j = 0; j < patLen && txt[i] == pattern[j]; ++i, ++j) {
            }
            if (j == patLen) {
                return start;
            }
            ch = txt[start + patLen];
            pos = patLen - 1;
            for (; pos >= 0 && ch != pattern[pos]; --pos) {
            }
            start += patLen - pos;
        }
    }
    return -1; // not find
}


DFINLINE bool AppIsPathDelimiter(s32 v) {
    return '/' == v || '\\' == v;
}

/**
 * @return length after simplified.
 */
template <typename T>
static usz AppSimplifyPath(T* str, const T* const end) {
    DASSERT(str);
    T* dest = str;
    T* curr = str;
    while (curr < end) {
        if (AppIsPathDelimiter(*curr)) {
            *curr = '/';
            if (curr + 1 == end) {
                *dest++ = '/';
                break;
            }
            if (AppIsPathDelimiter(curr[1])) {
                ++curr;
                //*curr = '/';
                continue;
            }
            if ('.' == curr[1]) {
                if (curr + 2 == end) {
                    *dest++ = '/';
                    break;
                }
                if (AppIsPathDelimiter(curr[2])) { // case:  /./
                    curr += 2;
                    //*curr = '/';
                    continue;
                }
                if ('.' == curr[2] && (curr + 3 == end || AppIsPathDelimiter(curr[3]))) { // case: /../
                    if (dest > str) {
                        --dest;
                        if (dest[0] == '/') {
                            --dest;
                        }
                        while (dest > str && dest[0] != '/') {
                            --dest;
                        }
                    }
                    if (curr + 3 == end) {
                        *dest++ = '/';
                        break;
                    }
                    curr += 3;
                    //*curr = '/';
                    continue;
                }
            }
        }
        *dest++ = *curr++;
    } // while
    return dest - str;
}


/**
 * @brief
 * 此类不管理任何内存
 */
template <typename T>
class TStrView {
public:
    TStrView() : mData(nullptr), mLen(0) {
    }

    TStrView(const T* str, usz len) : mData((T*)str), mLen(len) {
    }

    TStrView(const TStrView& it) : mData(it.mData), mLen(it.mLen) {
    }

    TStrView(TStrView&& it) noexcept : mData(it.mData), mLen(it.mLen) {
    }

    ~TStrView() {
        mData = nullptr;
        mLen = 0;
    }

    const TStrView& operator=(const TStrView& it) {
        mData = it.mData;
        mLen = it.mLen;
        return *this;
    }

    const TStrView& operator=(const TStrView&& it) noexcept {
        mData = it.mData;
        mLen = it.mLen;
        return *this;
    }

    bool operator==(const TStrView<T>& other) const {
        if (mLen == other.mLen) {
            for (usz i = 0; i < mLen; ++i) {
                if (mData[i] != other.mData[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    bool operator==(const T* str) const {
        if (str && mData) {
            return 0 == AppStrCMP(mData, str, mLen);
        }
        return false;
    }

    bool equalsn(const T* str, usz len) const {
        if (str && mData) {
            return 0 == AppStrCMP(mData, str, len);
        }
        return false;
    }

    void set(const T* str, usz len) {
        mData = (T*)str;
        mLen = len;
    }

    void toLower() {
        AppStr2Lower(mData, mLen);
    }

    void toUpper() {
        AppStr2Upper(mData, mLen);
    }

    void simplifyPath() {
        mLen = AppSimplifyPath(mData, mData + mLen);
    }

    T* mData;
    usz mLen;
};


template <typename T, typename TAlloc = TAllocator<T>>
class TString {
public:
    /**
     * @brief default constructor which have not alloc mem
     */
    TString() noexcept {
        initSmall();
    }

    TString(usz reserve) noexcept {
        if (reserve > G_SMALL_CAPACITY) {
            mBigStr.mCapacity = (reserve + 1) & G_ALLOC_MASK;
            mBigStr.mLength = 0;
            mBigStr.mBuffer = mAllocator.allocate(mBigStr.mCapacity | 1);
            mBigStr.mBuffer[0] = 0;
        } else {
            initSmall();
        }
    }

    template <class B>
    TString(const B* const str, usz len) noexcept {
        initStr<B>(str, len);
    }


    TString(const TString<T, TAlloc>& other) noexcept {
        initStr<T>(other.c_str(), other.size());
    }


    TString<T, TAlloc>(const TStrView<T>& other) noexcept {
        initStr<T>(other.mData, other.mLen);
    }


    /** @brief Constructor from other String types */
    template <class B, class A>
    TString(const TString<B, A>& other) noexcept {
        initStr<B>(other.c_str(), other.size());
    }

    template <class B>
    TString<T, TAlloc>(const TStrView<B>& other) noexcept {
        initStr<B>(other.mData, other.mLen);
    }

    TString(TString<T, TAlloc>&& it) noexcept {
        mBigStr = it.mBigStr;
        AppSwap(mAllocator, it.mAllocator);
        it.initSmall();
    }

    //! Constructor for unicode and ascii strings
    template <class B>
    TString(const B* const c) : TString() {
        *this = c;
    }

    TString(const T* const c) : TString() {
        *this = c;
    }

    ~TString() {
        if (isAllocated()) {
            mAllocator.deallocate(mBigStr.mBuffer);
#ifdef DDEBUG
            mBigStr.mBuffer = nullptr;
            initSmall();
#endif
        }
    }


    TString<T, TAlloc>& operator=(const TString<T, TAlloc>& it) {
        if (&it != this) {
            usz length = it.size();
            resize(length);
            memcpy(data(), it.data(), length * sizeof(T));
        }
        return *this;
    }

    TString<T, TAlloc>& operator=(TString<T, TAlloc>&& it) noexcept {
        if (&it != this) {
            if (isAllocated()) {
                mAllocator.deallocate(mBigStr.mBuffer);
            }
            mBigStr = it.mBigStr;
            AppSwap(mAllocator, it.mAllocator);
            it.initSmall();
        }
        return *this;
    }

    template <class B, class A>
    TString<T, TAlloc>& operator=(const TString<B, A>& it) {
        usz len = it.size();
        resize(len);
        copyBufType(it.data(), len, data());
        return *this;
    }

    TString<T, TAlloc>& operator=(const TStrView<T>& it) {
        usz length = it.mLen;
        if (inCurrentMem(it.mData)) {
            T* buf = data();
            length = AppMin(capacity() - (it.mData - buf), length);
            if (it.mData > buf && length > 0) {
                memmove(buf, it.mData, length * sizeof(T));
            }
            cutLen(length);
        } else {
            resize(length);
            memcpy(data(), it.mData, length * sizeof(T));
        }
        return *this;
    }

    template <class B>
    TString<T, TAlloc>& operator=(const B* ss) {
        if (!ss) {
            cutLen(0);
            return *this;
        }
        T* buf = data();
        usz slen = size();
        usz len = 0;
        if (inCurrentMem((T*)ss)) {
            len = ((T*)ss - buf);
            len = len >= slen ? 0 : slen - len;
            if ((T*)ss > buf && len > 0) {
                memmove(buf, ss, len * sizeof(T));
            }
            cutLen(len);
            return *this;
        }
        const B* p = ss;
        while (*p++) {
            ++len;
        }

        resize(len);
        copyBufType(ss, len, data());
        cutLen(len);
        return *this;
    }


    TString<T, TAlloc> operator+(const TString<T, TAlloc>& it) const {
        TString<T, TAlloc> str(size() + it.size());
        memcpy(str.data(), data(), sizeof(T) * size());
        memcpy(str.data() + size(), it.data(), sizeof(T) * it.size());
        str.cutLen(size() + it.size());
        return str;
    }


    TString<T, TAlloc> operator+(const TStrView<T>& it) const {
        usz mlen = size();
        TString<T, TAlloc> str(mlen + it.mLen);
        memcpy(str.data(), data(), sizeof(T) * mlen);
        memcpy(str.data() + mlen, it.mData, sizeof(T) * it.mLen);
        str.cutLen(mlen + it.mLen);
        return str;
    }

    template <class B>
    TString<T, TAlloc> operator+(const B* const ss) {
        const T* p = ss;
        while (*p++) {
        }
        usz len = (usz)(p - ss) - 1;
        usz mlen = size();
        TString<T, TAlloc> str(mlen + len);
        memcpy(str.data(), data(), sizeof(T) * mlen);
        copyBufType(ss, len, str.data() + mlen);
        str.cutLen(mlen + len);
        return str;
    }


    T& operator[](const usz index) {
        DASSERT(index < size());
        return *(data() + index);
    }


    const T& operator[](const usz index) const {
        DASSERT(index < size());
        return *(c_str() + index);
    }


    bool operator==(const T* const str) const {
        if (!str) {
            return false;
        }
        const T* buf = c_str();
        usz len = size();
        usz i;
        for (i = 0; i < len && str[i]; ++i) {
            if (buf[i] != str[i]) {
                return false;
            }
        }
        return buf[i] == str[i];
    }


    bool operator==(const TString<T, TAlloc>& other) const {
        usz len = size();
        if (len == other.size()) {
            if (0 == memcmp(c_str(), other.c_str(), len * sizeof(T))) {
                return true;
            }
        }
        return false;
    }


    bool operator<(const TString<T, TAlloc>& other) const {
        usz len = size();
        usz cnt = AppMin(len, other.size());
        const T* buf = c_str();
        const T* buf2 = other.c_str();
        for (usz i = 0; i < cnt; ++i) {
            s32 diff = (s32)buf[i] - (s32)buf2[i];
            if (diff) {
                return (diff < 0);
            }
        }
        return (len < other.size());
    }

    bool operator>(const TString<T, TAlloc>& other) const {
        usz len = size();
        usz cnt = AppMin(len, other.size());
        const T* buf = c_str();
        const T* buf2 = other.c_str();
        for (usz i = 0; i < cnt; ++i) {
            s32 diff = (s32)buf[i] - (s32)buf2[i];
            if (diff) {
                return (diff > 0);
            }
        }
        return (len > other.size());
    }

    bool operator!=(const T* const str) const {
        if (!str) {
            return true;
        }
        const T* buf = c_str();
        usz len = size();
        usz i;
        for (i = 0; i < len && str[i]; ++i) {
            if (buf[i] != str[i]) {
                return true;
            }
        }
        return buf[i] != str[i];
    }


    bool operator!=(const TString<T, TAlloc>& other) const {
        usz len = size();
        if (len == other.size()) {
            if (0 == memcmp(c_str(), other.c_str(), len * sizeof(T))) {
                return false;
            }
        }
        return true;
    }

    TString<T, TAlloc>& toLower() {
        AppStr2Lower(data(), size());
        return *this;
    }

    TString<T, TAlloc>& toUpper() {
        AppStr2Upper(data(), size());
        return *this;
    }


    /**
     * @brief Compares the strings ignoring case.
     * @param other: Other TString to compare.
     * @return true if the strings are equal ignoring case. */
    bool equalsNocase(const TString<T, TAlloc>& other) const {
        return 0 == AppStrNocaseCMP(c_str(), other.c_str(), size());
    }

    /**
     * @brief Compares the strings ignoring case.
     * @param other: Other String to compare.
     * @param sourcePos: where to start to compare in the String
     * @return true if the strings are equal ignoring case. */
    bool equalsSubNocase(const TString<T, TAlloc>& other, const usz sourcePos = 0) const {
        usz len = size();
        if (sourcePos >= len) {
            return false;
        }
        len -= sourcePos;
        return 0 == AppStrNocaseCMP(c_str(), other.c_str(), len);
    }


    /**
     * @brief compares the first n characters of the strings
     * @param other Other String to compare.
     * @param n Number of characters to compare
     * @return True if the n first characters of both strings are equal. */
    bool equalsn(const TString<T, TAlloc>& other, usz n) const {
        if (n <= size() && n <= other.size()) {
            const T* buf = c_str();
            const T* bufb = other.c_str();
            for (usz i = 0; i < n; ++i) {
                if (buf[i] != bufb[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }


    /**
     * @brief compares the first n characters of the strings
     * @param str Other String to compare.
     * @param n Number of characters to compare
     * @return True if the n first characters of both strings are equal. */
    bool equalsn(const T* const str, usz n) const {
        if (!str) {
            return false;
        }
        if (n <= size()) {
            const T* buf = c_str();
            for (usz i = 0; i < n; ++i) {
                if (buf[i] != str[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    void simplifyPath() {
        T* buf = data();
        cutLen(AppSimplifyPath(buf, buf + size()));
    }

    void resize(usz val) {
        if (val > capacity()) {
            reallocateAuto(val, val);
        }
        cutLen(val);
    }

    template <class B>
    TString<T, TAlloc>& assign(const B* const it) {
        const B* tmp = it;
        while (*tmp++) {
        }
        return assign(it, tmp - it - 1);
    }

    template <class B>
    TString<T, TAlloc>& assign(const B* const it, usz len) {
        if (inCurrentMem(it)) { // it from myself
            usz slen = capacity();
            T* buf = data();
            slen -= it - buf;
            len = AppMin(len, slen);
            memmove(buf, it, len * sizeof(T));
            cutLen(len);
            return *this;
        }
        resize(len);
        copyBufType(it, len, data());
        return *this;
    }

    /**
     * @brief Appends a String to this String
     * @param other: Char String to append.
     * @param length: The length of the String to append. */
    template <class B>
    TString<T, TAlloc>& append(const B* const other, usz len = GMAX_USIZE) {
        if (!other || 0 == len) {
            return *this;
        }
        if (GMAX_USIZE == len) {
            const B* p = other;
            while (*p++) {
            }
            len = (usz)(p - other) - 1;
            if (0 == len) {
                return *this;
            }
        }
        // we'll keep the old String for a while, because the new
        // String could be a part of the current String.
        usz slen = size();
        T* buf = data();
        T* del = nullptr;
        T* newbuf = nullptr;
        usz newcap;
        if (slen + len > capacity()) {
            if (isAllocated()) {
                del = buf;
            }
            newcap = nextAlloc(slen + len, slen);
            newbuf = mAllocator.allocate(newcap | 1);
            memcpy(newbuf, buf, sizeof(T) * slen);
            buf = newbuf;
        }
        copyBufType(other, len, buf + slen);
        if (newbuf) {
            mBigStr.mCapacity = newcap;
            mBigStr.mBuffer = newbuf;
            if (del) {
                mAllocator.deallocate(del);
            }
            cutLenBig(slen + len);
            return *this;
        }
        cutLen(slen + len);
        return *this;
    }


    /**
     * @brief Reserves some memory.
     * @param count: Amount of characters to reserve. */
    void reserve(usz count) {
        if (count > capacity()) {
            reallocate(count);
        }
    }


    /**
    * @brief finds first occurrence of a character of a list in String
    * @param c: List of characters to find. For example if the method
            should find the first occurrence of 'a' or 'b', this parameter should be "ab".
    * @param count: Amount of characters in the list. Usually this should be strlen(c)
    * @return Position where one of the characters has been found, or -1 if not found. */
    ssz findFirstChar(const T* const c, usz count = 1) const {
        if (!c || 0 == count) {
            return -1;
        }
        const T* buf = c_str();
        usz len = size();
        for (usz i = 0; i < len; ++i) {
            for (usz j = 0; j < count; ++j) {
                if (buf[i] == c[j]) {
                    return i;
                }
            }
        }
        return -1;
    }


    /**
    * @brief Finds first position of a character not in a given list.
    * @param c: List of characters not to find. For example if the method
        should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
    * @param count: Amount of characters in the list. Usually this should be strlen(c)
    * @return Position where the character has been found, or -1 if not found. */
    template <class B>
    ssz findFirstCharNotInList(const B* const c, usz count = 1) const {
        if (!c || 0 == count) {
            return -1;
        }
        const T* buf = c_str();
        usz len = size();
        for (usz i = 0; i < len; ++i) {
            usz j;
            for (j = 0; j < count; ++j) {
                if (buf[i] == c[j]) {
                    break;
                }
            }
            if (j == count) {
                return i;
            }
        }
        return -1;
    }

    /**
    * @brief Finds last position of a character not in a given list.
    * @param c: List of characters not to find. For example if the method
        should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
    * @param count: Amount of characters in the list. Usually this should be strlen(c)
    * @return Position where the character has been found, or -1 if not found. */
    template <class B>
    ssz findLastCharNotInList(const B* const c, usz count = 1) const {
        if (!c || 0 == count) {
            return -1;
        }
        const T* buf = c_str();
        for (ssz i = (ssz)(size()) - 1; i >= 0; --i) {
            usz j;
            for (j = 0; j < count; ++j) {
                if (buf[i] == c[j]) {
                    break;
                }
            }
            if (j == count) {
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief finds first occurrence of character in String
     * @param c: Character to search for.
     * @return Position where the character has been found, or -1 if not found. */
    ssz findFirst(T ch) const {
        const T* buf = c_str();
        usz len = size();
        for (usz i = 0; i < len; ++i) {
            if (buf[i] == ch) {
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief finds next occurrence of character in String
     * @param c: Character to search for.
     * @param startPos: Position in TString to start searching.
     * @return Position where the character has been found, or -1 if not found. */
    ssz findNext(T c, usz startPos) const {
        const T* buf = c_str();
        usz len = size();
        for (; startPos < len; ++startPos) {
            if (buf[startPos] == c) {
                return startPos;
            }
        }
        return -1;
    }


    /**
     * @brief 查找末尾字符位置
     * @param ch: 要找的字符
     * @param start: 起始位，从此处开始反向查找，-1则从末尾开始
     * @return 符合字符的位置(以0始), 失败则返回-1. */
    ssz findLast(T ch, ssz start = -1) const {
        ssz len = static_cast<ssz>(size()) - 1;
        start = AppClamp(start < 0 ? len : start, -1LL, len);
        const T* buf = c_str();
        for (ssz i = start; i >= 0; --i) {
            if (buf[i] == ch) {
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief 查找末尾字符位置
     * @param cs: 要找的字符，找到任一则结束查找. 例如查找 'a' 或 'b', 参数应为 "ab".
     * @param count: strlen(cs)
     * @return 符合任一字符的位置(以0始), 失败则返回-1. */
    ssz findLastChar(const T* const cs, usz count = 1) const {
        if (!cs || 0 == count) {
            return -1;
        }
        const T* buf = c_str();
        usz j;
        for (ssz pos = static_cast<ssz>(size()) - 1; pos >= 0; --pos) {
            for (j = 0; j < count; ++j) {
                if (buf[pos] == cs[j]) {
                    return pos;
                }
            }
        }
        return -1;
    }

    /**
     * @brief finds another TString in this TString
     * @param str: Another TString
     * @param start: Start position of the search
     * @param len: len of str
     * @return Positions where the TString has been found, or -1 if not found. */
    template <class B>
    ssz find(const B* const str, const usz start = 0, usz len = 0) const {
        if (str && *str) {
            if (0 == len) {
                while (str[len]) {
                    ++len;
                }
            }
            const T* buf = c_str();
            usz slen = size();
            if (slen < start + len) {
                return -1;
            }
            ssz ret = AppSundayFind(c_str() + start, slen - start, str, len);
            return ret < 0 ? ret : ret + start;
#if (0)
            // 暴力查找，性能比AppSundayFind()慢一倍以上
            if (len > slen) {
                return -1;
            }
            const usz mx = slen - len;
            usz j;
            for (usz i = start; i < mx; ++i) {
                j = 0;
                while (str[j] && buf[i + j] == str[j]) {
                    ++j;
                }
                if (!str[j]) {
                    return i;
                }
            }
#endif
        }

        return -1;
    }


    /**
     * @brief substring
     * @param begin Start of substring.
     * @param length Length of substring.
     * @return a substring */
    TString<T> subString(usz begin, usz length = GMAX_USIZE) const {
        usz len = size();
        if (0 == length || begin >= len) {
            return TString<T>();
        }
        len -= begin;
        if (length > len) {
            length = len;
        }
        TString<T> ret(length);
        T* dest = ret.data();
        const T* src = data() + begin;
        memcpy(dest, src, length * sizeof(T));
        ret.cutLen(length);
        return std::move(ret);
    }


    TString<T, TAlloc>& operator+=(T ch) {
        usz len = size();
        if (len + 1 > capacity()) {
            reallocateAuto(len + 1, len);
        }
        *(data() + len) = ch;
        cutLen(len + 1);
        return *this;
    }


    TString<T, TAlloc>& operator+=(const T* const str) {
        return append(str);
    }


    TString<T, TAlloc>& operator+=(const TString<T, TAlloc>& str) {
        return append(str.c_str(), str.size());
    }

    TString<T, TAlloc> operator+=(const TStrView<T>& other) {
        return append(other.mData, other.mLen);
    }

    TString<T, TAlloc>& operator=(const s32 val) {
        s8 tmpbuf[16];
        usz len = snprintf(tmpbuf, sizeof(tmpbuf), "%d", val);
        resize(len);
        copyBufType(tmpbuf, len, data());
        return *this;
    }

    TString<T, TAlloc>& operator=(const u32 val) {
        s8 tmpbuf[16];
        usz len = snprintf(tmpbuf, sizeof(tmpbuf), "%u", val);
        resize(len);
        copyBufType(tmpbuf, len, data());
        return *this;
    }

    TString<T, TAlloc>& operator+=(const s32 val) {
        s8 tmpbuf[16];
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%d", val));
    }

    TString<T, TAlloc>& operator+=(const u32 val) {
        s8 tmpbuf[16];
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%u", val));
    }

    TString<T, TAlloc>& operator=(const f64 val) {
        s8 tmpbuf[317]; // 6位小数的DBL_MAX长度为316
        usz len = snprintf(tmpbuf, sizeof(tmpbuf), "%0.6lf", val);
        resize(len);
        copyBufType(tmpbuf, len, data());
        return *this;
    }

    TString<T, TAlloc>& operator+=(const f64 val) {
        s8 tmpbuf[317]; // 6位小数的DBL_MAX长度为316
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%0.6lf", val));
    }

    TString<T, TAlloc>& operator=(const f32 val) {
        s8 tmpbuf[48]; // 6位小数的FLT_MAX长度为46
        usz len = snprintf(tmpbuf, sizeof(tmpbuf), "%0.6f", val);
        resize(len);
        copyBufType(tmpbuf, len, data());
        return *this;
    }

    TString<T, TAlloc>& operator+=(const f32 val) {
        s8 tmpbuf[48]; // 6位小数的FLT_MAX长度为46
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%0.6f", val));
    }


    TString<T, TAlloc>& replace(T toReplace, T replaceWith) {
        T* buf = data();
        usz len = size();
        for (usz i = 0; i < len; ++i) {
            if (buf[i] == toReplace) {
                buf[i] = replaceWith;
            }
        }
        return *this;
    }


    /**
     * @brief Replaces all instances of a String with another one.
     * @param toReplace The String to replace.
     * @param replaceWith The String replacing the old one. */
    TString<T, TAlloc>& replace(const TString<T, TAlloc>& toReplace, const TString<T, TAlloc>& replaceWith) {
        if (0 == toReplace.size()) {
            return *this;
        }
        const T* other = toReplace.c_str();
        const T* replace = replaceWith.c_str();
        const usz other_size = toReplace.size();
        const usz replace_size = replaceWith.size();

        // Determine the delta.  The algorithm will change depending on the delta.
        ssz delta = replace_size - other_size;
        T* buf = data();
        // The String will not shrink or grow.
        if (0 == delta) {
            ssz pos = 0;
            while ((pos = find(other, pos, other_size)) != -1) {
                for (usz i = 0; i < replace_size; ++i) {
                    buf[pos + i] = replace[i];
                }
                ++pos;
            }
            return *this;
        }

        // The String will shrink.
        usz len = size();
        if (delta < 0) {
            usz i = 0;
            for (usz pos = 0; pos < len; ++i, ++pos) {
                // Is this potentially a match?
                if (buf[pos] == *other) {
                    // Check to see if we have a match.
                    usz j;
                    for (j = 0; j < other_size; ++j) {
                        if (buf[pos + j] != other[j]) {
                            break;
                        }
                    }

                    // If we have a match, replace characters.
                    if (j == other_size) {
                        for (j = 0; j < replace_size; ++j)
                            buf[i + j] = replace[j];
                        i += replace_size - 1;
                        pos += other_size - 1;
                        continue;
                    }
                }

                // No match found, just copy characters.
                buf[i] = buf[pos];
            }
            cutLen(i);
            return *this;
        }

        // The String will grow.
        //  Count the number of times toReplace exists in the TString so we can allocate the new size.
        usz find_count = 0;
        ssz pos = 0;
        while ((pos = find(other, pos, other_size)) != -1) {
            ++find_count;
            pos += other_size;
        }

        // Re-allocate the TString now, if needed.
        usz addon = delta * find_count;
        if (len + addon > capacity()) {
            reallocateAuto(len + addon, addon);
            buf = data();
        }
        // Start replacing.
        pos = 0;
        while ((pos = find(other, pos, other_size)) != -1) {
            T* start = buf + pos + other_size - 1;
            T* ptr = buf + len - 1;
            T* end = buf + delta + len - 1;

            // Shift characters to make room for the TString.
            while (ptr != start) {
                *end = *ptr;
                --ptr;
                --end;
            }

            // Add the new TString now.
            for (usz i = 0; i < replace_size; ++i) {
                buf[pos + i] = replace[i];
            }
            pos += replace_size;
            len += delta;
        }
        cutLen(len);
        return *this;
    }


    /**
     * @brief Removes characters from a TString.
     * @param ch: Character to remove. */
    TString<T, TAlloc>& remove(T ch) {
        usz pos = 0;
        usz found = 0;
        usz len = size();
        T* buf = data();
        for (usz i = 0; i < len; ++i) {
            if (buf[i] == ch) {
                ++found;
                continue;
            }
            buf[pos++] = buf[i];
        }
        cutLen(len - found);
        return *this;
    }


    TString<T, TAlloc>& remove(const TString<T, TAlloc>& toRemove) {
        usz tlen = toRemove.size();
        if (tlen == 0) {
            return *this;
        }
        usz pos = 0;
        usz found = 0;
        usz len = size();
        T* buf = data();
        for (usz i = 0; i < len; ++i) {
            usz j = 0;
            while (j < tlen) {
                if (buf[i + j] != toRemove[j]) {
                    break;
                }
                ++j;
            }
            if (j == tlen) {
                found += tlen;
                i += tlen - 1;
                continue;
            }
            buf[pos++] = buf[i];
        }
        cutLen(len - found);
        return *this;
    }


    /**
     * @brief Removes characters from a TString.
     * @param characters: Characters to remove. */
    TString<T, TAlloc>& removeChars(const TString<T, TAlloc>& characters) {
        usz tlen = characters.size();
        if (0 == tlen) {
            return *this;
        }
        usz pos = 0;
        usz found = 0;
        usz len = size();
        T* buf = data();
        for (usz i = 0; i < len; ++i) {
            bool docontinue = false;
            for (usz j = 0; j < tlen; ++j) {
                if (characters[j] == buf[i]) {
                    ++found;
                    docontinue = true;
                    break;
                }
            }
            if (docontinue) {
                continue;
            }
            buf[pos++] = buf[i];
        }
        cutLen(len - found);
        return *this;
    }


    /**
     * @brief Removes the specified characters from the begining and the end of the String.
     * @param characters: Characters to remove. */
    TString<T, TAlloc>& trim(const TString<T, TAlloc>& whitespace = " \t\n\r") {
        // find start and end of the substring without the specified characters
        usz tlen = whitespace.size();
        const ssz begin = findFirstCharNotInList(whitespace.c_str(), tlen);
        if (-1 == begin) {
            cutLen(0);
            return (*this);
        }
        const ssz end = findLastCharNotInList(whitespace.c_str(), tlen);
        usz len = (end + 1) - begin;
        T* buf = data();
        memmove(buf, buf + begin, len * sizeof(T));
        cutLen(len);
        return *this;
    }


    /**
     * @brief Erases a character from the String, maybe slow.
     * @param index Index of element to be erased. */
    TString<T, TAlloc>& erase(usz index) {
        usz len = size();
        T* buf = data();
        if (index < len) {
            if (index + 1 < len) {
                memmove(buf + index, buf + index + 1, (len - index - 1) * sizeof(T));
            }
            cutLen(len - 1);
        }
        return *this;
    }


    TString<T, TAlloc>& validate() {
        usz len = size();
        T* buf = data();
        for (usz i = 0; i <= len; ++i) {
            if (buf[i] == 0) {
                cutLen(i);
                return *this;
            }
        }
        return *this;
    }

    /**
     * @return last char or 0 */
    T lastChar() const {
        usz len = size();
        T* buf = data();
        return len > 0 ? buf[len - 1] : 0;
    }

    TString<T>& deletePathFromFilename() {
        T* buf = data();
        usz len = size();
        const T* p = buf + len;
        // search for path separator or beginning
        while (*p != '/' && *p != '\\' && p != buf) {
            --p;
        }
        if (p != buf) {
            ++p;
            len -= p - buf;
            if (len > 0) {
                memmove(buf, p, len * sizeof(T));
            }
            cutLen(len);
        }
        return *this;
    }

    TString<T>& deleteFilename() {
        T* buf = data();
        const T* p = buf + size();
        // search for path separator or beginning
        while (*p != '/' && *p != '\\' && p != buf) {
            --p;
        }
        cutLen(p - buf + 1);
        return *this;
    }

    TString<T>& deleteFilenameExtension() {
        ssz pos = findLast('.');
        if (pos >= 0) {
            cutLen(pos);
        }
        return *this;
    }

    bool isFileExtension(const TString<T>& ext) const {
        s64 pos = findLast('.');
        if (pos < 0) {
            return false;
        }
        return equalsSubNocase(ext, pos + 1);
    }

    bool isFileExtension(const T* ext) const {
        s64 pos = ext ? findLast('.') : -1;
        if (pos < 0) {
            return false;
        }
        return 0 == AppStrNocaseCMP(c_str() + pos + 1, ext, size() - pos);
    }

    // trim path
    TString<T>& trimPath(s32 pathCount) {
        s64 i = size();
        T* buf = data();
        while (i >= 0) {
            if (buf[i] == '/' || buf[i] == '\\') {
                if (--pathCount <= 0) {
                    break;
                }
            }
            --i;
        }
        cutLen((i > 0) ? i + 1 : 0);
        return *this;
    }

    bool empty() const {
        return 0 == size();
    }

    /**
     * @return A valid pointer to C-style NULL terminated String.
     */
    const T* c_str() const {
        return data();
    }

    usz size() const {
        return isAllocated() ? mBigStr.mLength : (mSmallStr.mLen.mLength >> 1);
    }

    T* data() const {
        return const_cast<T*>(isAllocated() ? mBigStr.mBuffer : mSmallStr.mDat.mBuffer);
    }

    usz capacity() const {
        return isAllocated() ? mBigStr.mCapacity : G_SMALL_CAPACITY;
    }

protected:
    template <class B>
    void copyBufType(const B* src, usz len, T* dst) {
        if (sizeof(T) == sizeof(B)) {
            memcpy(dst, src, len * sizeof(T));
        } else {
            for (usz i = 0; i < len; ++i) {
                *dst++ = static_cast<T>(src[i]);
            }
        }
    }

    DINLINE bool inCurrentMem(const T* const buf) const {
        if (isAllocated()) {
            return buf >= mBigStr.mBuffer && buf <= mBigStr.mBuffer + mBigStr.mCapacity;
        }
        return buf >= mSmallStr.mDat.mBuffer && buf <= mSmallStr.mDat.mBuffer + G_SMALL_CAPACITY;
    }
    DINLINE void initSmall() {
        static_assert(sizeof(mSmallStr) == sizeof(mBigStr));
        static_assert(sizeof(mSmallStr.mLen) == sizeof(mBigStr));
        static_assert(sizeof(mSmallStr.mDat) == sizeof(mBigStr));
        static_assert(sizeof(mBigStr) == sizeof(usz) * 3);
        static_assert(sizeof(*this) == sizeof(usz) * 4);
        static_assert(1 == sizeof(T) || 0 == (sizeof(T) & 1));
        static_assert(sizeof(TAlloc) <= sizeof(usz));
        static_assert(sizeof(T) <= sizeof(usz));
        mSmallStr.mLen.mLength = G_SMALL_FLAG;
        mSmallStr.mDat.mBuffer[0] = 0;
    }

    // @note this is construct function without init before
    template <class B>
    void initStr(const B* const str, usz len) noexcept {
        if (str) {
            if (len > G_SMALL_CAPACITY) {
                mBigStr.mCapacity = (len + 1) & G_ALLOC_MASK;
                mBigStr.mLength = len;
                mBigStr.mBuffer = mAllocator.allocate(mBigStr.mCapacity | 1);
                copyBufType(str, len, mBigStr.mBuffer);
                mBigStr.mBuffer[len] = 0;
            } else {
                // initSmall();
                mSmallStr.mLen.mLength = static_cast<u8>((len << 1) | G_SMALL_FLAG);
                copyBufType(str, len, mSmallStr.mDat.mBuffer);
                mSmallStr.mDat.mBuffer[len] = 0;
            }
        } else {
            initSmall();
        }
    }
    DINLINE void cutLenBig(usz len) {
        mBigStr.mLength = len;
        mBigStr.mBuffer[len] = 0;
    }
    DINLINE void cutLenSmall(usz len) {
        mSmallStr.mLen.mLength = static_cast<u8>((len << 1) | G_SMALL_FLAG);
        mSmallStr.mDat.mBuffer[len] = 0;
    }
    DINLINE void cutLen(usz len) {
        if (isAllocated()) {
            mBigStr.mLength = len;
            mBigStr.mBuffer[len] = 0;
        } else {
            mSmallStr.mLen.mLength = static_cast<u8>((len << 1) | G_SMALL_FLAG);
            mSmallStr.mDat.mBuffer[len] = 0;
        }
    }

    DFINLINE bool isAllocated() const {
        return 0 == (G_SMALL_FLAG & mSmallStr.mLen.mLength);
    }

    DFINLINE usz nextAlloc(usz newlen, usz currLen) const {
        return (newlen + (currLen < 512 ? (8 | currLen) : (currLen >> 2))) & G_ALLOC_MASK;
    }

    void reallocateAuto(usz newlen, usz len) {
        reallocate(nextAlloc(newlen, len));
    }

    void reallocate(usz newlen) {
        newlen = ((newlen + 1) & G_ALLOC_MASK);
        T* buf = mAllocator.allocate(newlen | 1);
        T* oldbuf = data();
        usz len = size();
        len = len < newlen ? len : newlen;
        memcpy(buf, oldbuf, len * sizeof(T));
        buf[len] = 0;
        if (isAllocated()) {
            mAllocator.deallocate(oldbuf);
        }
        mBigStr.mCapacity = newlen;
        mBigStr.mLength = len;
        mBigStr.mBuffer = buf;
    }

    union {
#if defined(DENDIAN_BIG)
        struct {
            usz mLength;
            T* mBuffer;
            usz mCapacity;
        } mBigStr;
        union {
            struct {
                u8 mPacked[sizeof(usz) * 3 - 1]; // do not use
                u8 mLength;
            } mLen;
            struct {
                T mBuffer[sizeof(usz) * 3 / sizeof(T) - 1];
                T mPacked; // do not use
            } mDat;
        } mSmallStr;
#else
        struct {
            usz mCapacity;
            usz mLength;
            T* mBuffer;
        } mBigStr;
        union {
            struct {
                u8 mLength;
                u8 mPacked[sizeof(usz) * 3 - 1]; // do not use
            } mLen;
            struct {
                T mPacked; // do not use
                T mBuffer[sizeof(usz) * 3 / sizeof(T) - 1];
            } mDat;
        } mSmallStr;
#endif
    };
    TAlloc mAllocator;

    const static usz G_SMALL_CAPACITY = sizeof(mSmallStr.mDat.mBuffer) / sizeof(T) - 1;
    const static usz G_SMALL_FLAG = 0x1;
    const static usz G_ALLOC_MASK = ~1ULL;
};

using StringView = TStrView<s8>;
using String = TString<s8>;
using WString = TString<wchar_t>;
using U16String = TString<s16>;
using U32String = TString<s32>;

} // namespace app


#endif // APP_STRINGS_H