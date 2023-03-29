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
#define	APP_STRINGS_H

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
template<class T>
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

template<class T>
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
template<typename T, typename P>
static ssz AppSundayFind(const T* txt, ssz txtLen, const P* pattern, ssz patLen) {
    DASSERT(txt && pattern);
    if (patLen > 0 && txtLen >= patLen) {
        txtLen -= patLen;
        ssz i, j, pos;
        T ch;
        for (ssz start = 0; start <= txtLen; ) {
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
    return -1;  //not find
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

    const TStrView& operator=(const TStrView&& it)noexcept {
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

    T* mData;
    usz mLen;
};



template <typename T, typename TAlloc = TAllocator<T> >
class TString {
public:
    /**
     * @brief default constructor which have not alloc mem
     */
    TString() : mBuffer(reinterpret_cast<T*>(&mAllocated)), mAllocated(0), mLen(0) {
    }

    TString(usz reserve) : mAllocated(1 + reserve), mLen(0) {
        mBuffer = mAllocator.allocate(mAllocated);
        mBuffer[0] = 0;
    }

    TString(const TString<T, TAlloc>& other) : mBuffer(reinterpret_cast<T*>(&mAllocated)),
        mAllocated(0), mLen(0) {
        *this = other;
    }

    TString(TString<T, TAlloc>&& it) noexcept : mBuffer(it.mBuffer), mAllocated(it.mAllocated), mLen(it.mLen) {
        if (it.isAllocated()) {
            it.mBuffer = reinterpret_cast<T*>(&it.mAllocated);
            it.mAllocated = 0;
            it.mLen = 0;
            AppSwap(mAllocator, it.mAllocator);
        } else {
            mBuffer = reinterpret_cast<T*>(&mAllocated);
            mAllocated = 0;
            mLen = 0;
        }
    }

    //! Constructor from other String types
    template <class B, class A>
    TString(const TString<B, A>& other) : mBuffer(reinterpret_cast<T*>(&mAllocated)),
        mAllocated(0), mLen(0) {
        *this = other;
    }

    TString<T, TAlloc>(const TStrView<T>& other) : mBuffer(reinterpret_cast<T*>(&mAllocated)),
        mAllocated(0), mLen(0) {
        reallocate(other.mLen);
        append(other.mData, other.mLen);
    }

    template <class B>
    TString(const B* const str, usz length) : mBuffer(reinterpret_cast<T*>(&mAllocated)),
        mAllocated(0), mLen(0) {
        reallocate(length);
        if (str) {
            for (usz i = 0; i < length; ++i) {
                mBuffer[i] = (T)str[i];
            }
            mLen = length;
            mBuffer[length] = 0;
        } else {
            mLen = 0;
            mBuffer[0] = 0;
        }
    }


    //! Constructor for unicode and ascii strings
    template <class B>
    TString(const B* const c) : mBuffer(reinterpret_cast<T*>(&mAllocated)), mAllocated(0), mLen(0) {
        *this = c;
    }

    TString(const T* const c) : mBuffer(reinterpret_cast<T*>(&mAllocated)), mAllocated(0), mLen(0) {
        *this = c;
    }

    ~TString() {
        setBuffer(&mAllocated);
        mAllocated = 0;
        mLen = 0;
    }


    TString<T, TAlloc>& operator=(const TString<T, TAlloc>& other) {
        if (this != &other) {
            mLen = other.mLen;
            if (mLen >= mAllocated) {
                reallocate(mLen);
            }
            for (usz i = 0; i < mLen; ++i) {
                mBuffer[i] = other.mBuffer[i];
            }
            mBuffer[mLen] = 0;
        }
        return *this;
    }

    TString<T, TAlloc>& operator=(TString<T, TAlloc>&& it) noexcept {
        if (&it != this) {
            if (it.isAllocated()) {
                setBuffer(it.mBuffer);
                it.mBuffer = reinterpret_cast<T*>(&it.mAllocated);
                it.mAllocated = 0;
                it.mLen = 0;
                AppSwap(mAllocator, it.mAllocator);
            } else {
                setBuffer(&mAllocated);
                mAllocated = 0;
                mLen = 0;
            }
        }
        return *this;
    }

    template <class B, class A>
    TString<T, TAlloc>& operator=(const TString<B, A>& other) {
        *this = other.c_str();
        return *this;
    }

    TString<T, TAlloc>& operator=(const TStrView<T>& other) {
        mLen = 0;
        return append(other.mData, other.mLen);
    }

    template <class B>
    TString<T, TAlloc>& operator=(const B* const c) {
        if (!c) {
            setLen(0);
            return *this;
        }

        if ((void*)c == (void*)mBuffer) {
            return *this;
        }

        usz len = 0;
        const B* p = c;
        while (*p++) {
            ++len;
        }

        // we'll keep the old String for a while, because the new
        // String could be a part of the current String.
        T* buf = mBuffer;

        if (len >= mAllocated) {
            mAllocated = nextAlloc(len);
            buf = mAllocator.allocate(mAllocated);
        }

        for (usz l = 0; l < len; ++l) {
            buf[l] = (T)c[l];
        }
        mLen = len;
        buf[len] = 0;
        if (buf != mBuffer) {
            setBuffer(buf);
        }
        return *this;
    }


    TString<T, TAlloc> operator+(const TString<T, TAlloc>& other) const {
        TString<T, TAlloc> str(*this);
        return str.append(other, other.getLen());
    }


    TString<T, TAlloc> operator+(const TStrView<T>& other) const {
        TString<T, TAlloc> str(*this);
        return str.append(other.mData, other.mLen);
    }

    template <class B>
    TString<T, TAlloc> operator+(const B* const c) const {
        TString<T, TAlloc> str(*this);
        return str.append(c);
    }


    T& operator[](const usz index) {
        DASSERT(index < mLen);
        return mBuffer[index];
    }


    const T& operator[](const usz index) const {
        DASSERT(index < mLen);
        return mBuffer[index];
    }


    bool operator==(const T* const str) const {
        if (!str) {
            return false;
        }
        usz i;
        for (i = 0; i < mLen && str[i]; ++i) {
            if (mBuffer[i] != str[i]) {
                return false;
            }
        }
        return mBuffer[i] == str[i];
    }


    bool operator==(const TString<T, TAlloc>& other) const {
        if (mLen == other.mLen) {
            for (usz i = 0; i < mLen; ++i) {
                if (mBuffer[i] != other.mBuffer[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }


    bool operator<(const TString<T, TAlloc>& other) const {
        usz cnt = AppMin(mLen, other.mLen);
        for (usz i = 0; i < cnt; ++i) {
            s32 diff = (s32)mBuffer[i] - (s32)other.mBuffer[i];
            if (diff) {
                return (diff < 0);
            }
        }
        return (mLen < other.mLen);
    }

    bool operator>(const TString<T, TAlloc>& other) const {
        usz cnt = AppMin(mLen, other.mLen);
        for (usz i = 0; i < cnt; ++i) {
            s32 diff = (s32)mBuffer[i] - (s32)other.mBuffer[i];
            if (diff) {
                return (diff > 0);
            }
        }
        return (mLen > other.mLen);
    }

    bool operator!=(const T* const str) const {
        if (!str) {
            return true;
        }
        usz i;
        for (i = 0; i < mLen && str[i]; ++i) {
            if (mBuffer[i] != str[i]) {
                return true;
            }
        }
        return mBuffer[i] != str[i];
    }


    bool operator!=(const TString<T, TAlloc>& other) const {
        if (mLen == other.mLen) {
            for (usz i = 0; i < mLen; ++i) {
                if (mBuffer[i] != other.mBuffer[i]) {
                    return true;
                }
            }
            return false;
        }
        return true;
    }

    usz getAllocated() const {
        return mAllocated;
    }

    /**
    * @return 长度，不含tail.
    */
    usz getLen() const {
        return mLen;
    }

    usz size() const {
        return mLen;
    }

    bool empty() const {
        return 0 == mLen;
    }

    /**
    * @return A valid pointer to C-style NULL terminated String.
    */
    const T* c_str() const {
        return mBuffer;
    }


    TString<T, TAlloc>& toLower() {
        AppStr2Lower(mBuffer, mLen);
        return *this;
    }

    TString<T, TAlloc>& toUpper() {
        AppStr2Upper(mBuffer, mLen);
        return *this;
    }


    /**
    * @brief Compares the strings ignoring case.
    * @param other: Other TString to compare.
    * @return True if the strings are equal ignoring case. */
    bool equalsNocase(const TString<T, TAlloc>& other) const {
        if (mLen == other.mLen) {
            return 0 == AppStrNocaseCMP(mBuffer, other.mBuffer, mLen);
        }
        return false;
    }

    /**
    * @brief Compares the strings ignoring case.
    * @param other: Other String to compare.
    * @param sourcePos: where to start to compare in the String
    * @return True if the strings are equal ignoring case. */
    bool equalsSubNocase(const TString<T, TAlloc>& other, const usz sourcePos = 0) const {
        if (sourcePos >= mLen) {
            return false;
        }
        usz i = mLen - sourcePos;
        if (i == other.mLen) {
            return 0 == AppStrNocaseCMP(mBuffer + sourcePos, other.mBuffer, i);
        }
        return false;
    }


    /**
    * @brief Compares the strings ignoring case.
    * @param other: Other TString to compare.
    * @return True if this TString is smaller ignoring case. */
    bool lowerNocase(const TString<T, TAlloc>& other) const {
        return AppStrNocaseCMP(mBuffer, other.mBuffer, AppMax(mLen, other.mLen)) < 0;
    }


    /**
    * @brief compares the first n characters of the strings
    * @param other Other String to compare.
    * @param n Number of characters to compare
    * @return True if the n first characters of both strings are equal. */
    bool equalsn(const TString<T, TAlloc>& other, usz n) const {
        if (n <= mLen && n <= other.mLen) {
            for (usz i = 0; i < n; ++i) {
                if (mBuffer[i] != other.mBuffer[i]) {
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
        if (n <= mLen) {
            for (usz i = 0; i < n && str[i]; ++i) {
                if (mBuffer[i] != str[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }


    void setLen(usz val) {
        if (val >= mAllocated) {
            reallocate(val);
        }
        mLen = val;
        mBuffer[val] = 0;
    }


    /**
    * @brief Appends a String to this String
    * @param other: Char String to append.
    * @param length: The length of the String to append. */
    TString<T, TAlloc>& append(const T* const other, usz len = 0xFFFFFFFFFFFFFFFFULL) {
        if (!other || 0 == len /*|| other == mBuffer*/) {
            return *this;
        }
        if (0xFFFFFFFFFFFFFFFFULL == len) {
            const T* p = other;
            while (*p++) { }
            len = (usz)(p - other) - 1;
        }
        // we'll keep the old String for a while, because the new
        // String could be a part of the current String.
        T* buf = mBuffer;
        if (mLen + len + 1 > mAllocated) {
            mAllocated = nextAlloc(mLen + len);
            buf = mAllocator.allocate(mAllocated);
            for (usz i = 0; i < mLen; ++i) {
                buf[i] = mBuffer[i];
            }
        }
        for (usz i = 0; i < len; ++i) {
            buf[mLen + i] = other[i];
        }
        mLen += len;
        buf[mLen] = 0;
        if (buf != mBuffer) {
            setBuffer(buf);
        }
        return *this;
    }


    TString<T, TAlloc>& append(const TString<T, TAlloc>& other, usz length) {
        return append(other.mBuffer, length > other.getLen() ? other.getLen() : length);
    }


    /**
    * @brief Reserves some memory.
    * @param count: Amount of characters to reserve. */
    void reserve(usz count) {
        if (count >= mAllocated) {
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
        for (usz i = 0; i < mLen; ++i) {
            for (usz j = 0; j < count; ++j) {
                if (mBuffer[i] == c[j]) {
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
        for (usz i = 0; i < mLen; ++i) {
            usz j;
            for (j = 0; j < count; ++j) {
                if (mBuffer[i] == c[j]) {
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
        for (ssz i = (ssz)(mLen)-1; i >= 0; --i) {
            usz j;
            for (j = 0; j < count; ++j) {
                if (mBuffer[i] == c[j]) {
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
    ssz findFirst(T c) const {
        for (usz i = 0; i < mLen; ++i) {
            if (mBuffer[i] == c) {
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
        for (usz i = startPos; i < mLen; ++i) {
            if (mBuffer[i] == c) {
                return i;
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
        start = AppClamp(start < 0 ? (ssz)mLen - 1 : start, -1LL, (ssz)mLen - 1);
        for (ssz i = start; i >= 0; --i) {
            if (mBuffer[i] == ch) {
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
        usz j;
        for (ssz i = (ssz)mLen - 1; i >= 0; --i) {
            for (j = 0; j < count; ++j) {
                if (mBuffer[i] == cs[j]) {
                    return i;
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
            if (mLen < start + len) {
                return -1;
            }
            ssz ret = AppSundayFind(mBuffer + start, mLen - start, str, len);
            return ret < 0 ? ret : ret + start;
#if (0)
            //暴力查找，性能比AppSundayFind()慢一倍以上
            if (len > mLen) {
                return -1;
            }
            const usz mx = mLen - len;
            usz j;
            for (usz i = start; i < mx; ++i) {
                j = 0;
                while (str[j] && mBuffer[i + j] == str[j]) {
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
    * @param iLower copy only lower case
    * @return a substring */
    TString<T> subString(usz begin, usz length, bool iLower = false) const {
        if (0 == length || begin >= mLen) {
            return TString<T>();
        }
        if (length > mLen - begin) {
            length = mLen - begin;
        }
        TString<T> ret(length);

        usz i;
        if (iLower) {
            for (i = 0; i < length; ++i) {
                ret.mBuffer[i] = App2Lower(mBuffer[i + begin]);
            }
        } else {
            for (i = 0; i < length; ++i) {
                ret.mBuffer[i] = mBuffer[i + begin];
            }
        }

        ret.mBuffer[length] = 0;
        ret.mLen = length;
        return std::move(ret);
    }


    TString<T, TAlloc>& operator+=(T ch) {
        if (mLen + 2 > mAllocated) {
            reallocate(mLen + 2);
        }
        mBuffer[mLen++] = ch;
        mBuffer[mLen] = 0;
        return *this;
    }


    TString<T, TAlloc>& operator+=(const T* const str) {
        return append(str);
    }


    TString<T, TAlloc>& operator+=(const TString<T, TAlloc>& str) {
        return append(str.mBuffer, str.mLen);
    }

    TString<T, TAlloc> operator+=(const TStrView<T>& other) {
        return append(other.mData, other.mLen);
    }

    TString<T, TAlloc>& operator=(const s32 val) {
        s8 tmpbuf[16];
        mLen = 0;
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%d", val));
    }

    TString<T, TAlloc>& operator=(const u32 val) {
        s8 tmpbuf[16];
        mLen = 0;
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%u", val));
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
        mLen = 0;
        s8 tmpbuf[317]; //6位小数的DBL_MAX长度为316
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%0.6lf", val));
    }

    TString<T, TAlloc>& operator+=(const f64 val) {
        s8 tmpbuf[317];//6位小数的DBL_MAX长度为316
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%0.6lf", val));
    }

    TString<T, TAlloc>& operator=(const f32 val) {
        mLen = 0;
        s8 tmpbuf[48];//6位小数的FLT_MAX长度为46
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%0.6f", val));
    }

    TString<T, TAlloc>& operator+=(const f32 val) {
        s8 tmpbuf[48];//6位小数的FLT_MAX长度为46
        return append(tmpbuf, snprintf(tmpbuf, sizeof(tmpbuf), "%0.6f", val));
    }


    TString<T, TAlloc>& replace(T toReplace, T replaceWith) {
        for (usz i = 0; i < mLen; ++i) {
            if (mBuffer[i] == toReplace) {
                mBuffer[i] = replaceWith;
            }
        }
        return *this;
    }


    /**
    * @brief Replaces all instances of a String with another one.
    * @param toReplace The String to replace.
    * @param replaceWith The String replacing the old one. */
    TString<T, TAlloc>& replace(const TString<T, TAlloc>& toReplace, const TString<T, TAlloc>& replaceWith) {
        if (toReplace.mLen == 0) {
            return *this;
        }
        const T* other = toReplace.mBuffer;
        const T* replace = replaceWith.mBuffer;
        const usz other_size = toReplace.mLen;
        const usz replace_size = replaceWith.mLen;

        // Determine the delta.  The algorithm will change depending on the delta.
        ssz delta = replace_size - other_size;

        //The String will not shrink or grow.
        if (delta == 0) {
            ssz pos = 0;
            while ((pos = find(other, pos, toReplace.mLen)) != -1) {
                for (usz i = 0; i < replace_size; ++i) {
                    mBuffer[pos + i] = replace[i];
                }
                ++pos;
            }
            return *this;
        }

        //The String will shrink.
        if (delta < 0) {
            usz i = 0;
            for (usz pos = 0; pos < mLen; ++i, ++pos) {
                // Is this potentially a match?
                if (mBuffer[pos] == *other) {
                    // Check to see if we have a match.
                    usz j;
                    for (j = 0; j < other_size; ++j) {
                        if (mBuffer[pos + j] != other[j]) {
                            break;
                        }
                    }

                    // If we have a match, replace characters.
                    if (j == other_size) {
                        for (j = 0; j < replace_size; ++j)
                            mBuffer[i + j] = replace[j];
                        i += replace_size - 1;
                        pos += other_size - 1;
                        continue;
                    }
                }

                // No match found, just copy characters.
                mBuffer[i] = mBuffer[pos];
            }
            mLen = i;
            mBuffer[i] = 0;
            return *this;
        }

        //The String will grow.
        // Count the number of times toReplace exists in the TString so we can allocate the new size.
        usz find_count = 0;
        ssz pos = 0;
        while ((pos = find(other, pos, toReplace.mLen)) != -1) {
            ++find_count;
            pos += toReplace.mLen;
        }

        // Re-allocate the TString now, if needed.
        usz len = delta * find_count;
        if (mLen + len >= mAllocated) {
            reallocate(mLen + len);
        }

        // Start replacing.
        pos = 0;
        while ((pos = find(other, pos, toReplace.mLen)) != -1) {
            T* start = mBuffer + pos + other_size - 1;
            T* ptr = mBuffer + mLen - 1;
            T* end = mBuffer + delta + mLen - 1;

            // Shift characters to make room for the TString.
            while (ptr != start) {
                *end = *ptr;
                --ptr;
                --end;
            }

            // Add the new TString now.
            for (usz i = 0; i < replace_size; ++i) {
                mBuffer[pos + i] = replace[i];
            }
            pos += replace_size;
            mLen += delta;
        }
        mBuffer[mLen] = 0;
        return *this;
    }


    /**
    * @brief Removes characters from a TString.
    * @param ch: Character to remove. */
    TString<T, TAlloc>& remove(T ch) {
        usz pos = 0;
        usz found = 0;
        for (usz i = 0; i < mLen; ++i) {
            if (mBuffer[i] == ch) {
                ++found;
                continue;
            }
            mBuffer[pos++] = mBuffer[i];
        }
        mLen -= found;
        mBuffer[mLen] = 0;
        return *this;
    }


    TString<T, TAlloc>& remove(const TString<T, TAlloc>& toRemove) {
        usz size = toRemove.getLen();
        if (size == 0) {
            return *this;
        }
        usz pos = 0;
        usz found = 0;
        for (usz i = 0; i < mLen; ++i) {
            usz j = 0;
            while (j < size) {
                if (mBuffer[i + j] != toRemove[j]) {
                    break;
                }
                ++j;
            }
            if (j == size) {
                found += size;
                i += size - 1;
                continue;
            }
            mBuffer[pos++] = mBuffer[i];
        }
        mLen -= found;
        mBuffer[mLen] = 0;
        return *this;
    }


    /**
     * @brief Removes characters from a TString.
     * @param characters: Characters to remove. */
    TString<T, TAlloc>& removeChars(const TString<T, TAlloc>& characters) {
        if (characters.getLen() == 0) {
            return *this;
        }
        usz pos = 0;
        usz found = 0;
        for (usz i = 0; i < mLen; ++i) {
            // Don't use characters.findFirst as it finds the \0,
            // causing mLen to become incorrect.
            bool docontinue = false;
            for (usz j = 0; j < characters.getLen(); ++j) {
                if (characters[j] == mBuffer[i]) {
                    ++found;
                    docontinue = true;
                    break;
                }
            }
            if (docontinue) {
                continue;
            }
            mBuffer[pos++] = mBuffer[i];
        }
        mLen -= found;
        mBuffer[mLen] = 0;

        return *this;
    }


    /**
     * @brief Removes the specified characters from the begining and the end of the String.
     * @param characters: Characters to remove. */
    TString<T, TAlloc>& trim(const TString<T, TAlloc>& whitespace = " \t\n\r") {
        // find start and end of the substring without the specified characters
        const ssz begin = findFirstCharNotInList(whitespace.c_str(), whitespace.mLen);
        if (-1 == begin) {
            mLen = 0;
            mBuffer[0] = 0;
            return (*this);
        }
        const ssz end = findLastCharNotInList(whitespace.c_str(), whitespace.mLen);
        mLen = 0;
        return append(mBuffer + begin, (end + 1) - begin);
    }


    /**
     * @brief Erases a character from the String, maybe slow.
     * @param index Index of element to be erased. */
    TString<T, TAlloc>& erase(usz index) {
        if (index < mLen) {
            for (usz i = index + 1; i < mLen; ++i) {
                mBuffer[i - 1] = mBuffer[i];
            }
            --mLen;
            mBuffer[mLen] = 0;
        }
        return *this;
    }


    TString<T, TAlloc>& validate() {
        for (usz i = 0; i <= mLen; ++i) {
            if (mBuffer[i] == 0) {
                mLen = i;
                return *this;
            }
        }
        mBuffer[mLen] = 0;
        return *this;
    }

    /**
    * @return last char or null */
    T lastChar() const {
        return mLen > 0 ? mBuffer[mLen - 1] : 0;
    }

    TString<T>& deletePathFromFilename() {
        T* s = mBuffer;
        const T* p = s + mLen;
        // search for path separator or beginning
        while (*p != '/' && *p != '\\' && p != s) {
            --p;
        }
        if (p != s) {
            ++p;
            //*this = p;
            for (const T* epos = s + mLen + 1; p < epos; ++p) {
                s[0] = p[0];
                ++s;
            }
        }
        return *this;
    }

    TString<T>& deleteFilename() {
        T* s = mBuffer;
        const T* p = s + mLen;
        // search for path separator or beginning
        while (*p != '/' && *p != '\\' && p != s) {
            --p;
        }
        mLen = p - s + 1;
        mBuffer[mLen] = 0;
        return *this;
    }

    TString<T>& deleteFilenameExtension() {
        s64 pos = findLast('.');
        mLen = pos < 0 ? mLen : pos;
        mBuffer[mLen] = 0;
        return *this;
    }

    bool isFileExtension(const TString<T>& ext)const {
        s64 pos = findLast('.');
        if (pos < 0) {
            return false;
        }
        return equalsSubNocase(ext, pos + 1);
    }

    bool isFileExtension(const T* ext)const {
        s64 pos = ext ? findLast('.') : -1;
        if (pos < 0) {
            return false;
        }
        return 0 == AppStrNocaseCMP(mBuffer + pos + 1, ext, mLen - pos);
    }

    // trim path
    TString<T>& trimPath(s32 pathCount) {
        s64 i = getLen();
        while (i >= 0) {
            if (mBuffer[i] == '/' || mBuffer[i] == '\\') {
                if (--pathCount <= 0) {
                    break;
                }
            }
            --i;
        }
        mLen = (i > 0) ? i + 1 : 0;
        mBuffer[mLen] = 0;
        return *this;
    }


protected:
    DFINLINE bool isAllocated() const {
        return reinterpret_cast<const T*>(&mAllocated) != mBuffer;
    }

    DFINLINE void setBuffer(void* val) {
        if (isAllocated()) {
            mAllocator.deallocate(mBuffer);
        }
        mBuffer = reinterpret_cast<T*>(val);
    }

    DFINLINE usz nextAlloc(usz newlen) const {
        return newlen + (mLen < 512 ? (8 | mLen) : (mLen >> 2));
    }

    void reallocate(usz newlen) {
        newlen = nextAlloc(newlen);
        T* buf = mAllocator.allocate(newlen);
        mAllocated = newlen;

        usz amount = mLen < newlen ? mLen : newlen - 1;
        for (usz i = 0; i < amount; ++i) {
            buf[i] = mBuffer[i];
        }
        buf[amount] = 0;
        mLen = amount;
        setBuffer(buf);
    }


    T* mBuffer;
    usz mAllocated;
    usz mLen;
    TAlloc mAllocator;
};

using StringView = TStrView<s8>;
using String = TString<s8>;
using WString = TString<wchar_t>;

}//namespace app


#endif //APP_STRINGS_H