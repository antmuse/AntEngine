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


#ifndef APP_CONVERTER_H
#define APP_CONVERTER_H

#include "Config.h"
#include <math.h>

namespace app {

#define APP_DECIMAL_POINTS '.'

#define APP_ATOF_TABLE_SIZE 17
const f32 G_FAST_ATOF_TABLE[17] = {
    0.f,
    0.1f,
    0.01f,
    0.001f,
    0.0001f,
    0.00001f,
    0.000001f,
    0.0000001f,
    0.00000001f,
    0.000000001f,
    0.0000000001f,
    0.00000000001f,
    0.000000000001f,
    0.0000000000001f,
    0.00000000000001f,
    0.000000000000001f,
    0.0000000000000001f
};

//! Convert a simple string of base 10 digits into an unsigned 32 bit integer.
/** \param[in] in: The string of digits to convert. No leading chars are
allowed, only digits 0 to 9. Parsing stops at the first non-digit.
\param[out] out: (optional) If provided, it will be set to point at the
first character not used in the calculation.
\return The unsigned integer value of the digits. If the string specifies
too many digits to encode in an u32 then INT_MAX will be returned.
*/
template <typename T>
inline u32 App10StrToU32(const T* in, const T** out = nullptr) {
    if (!in) {
        if (out)
            *out = in;
        return 0;
    }

    bool overflow = false;
    u32 unsignedValue = 0;
    while ((*in >= '0') && (*in <= '9')) {
        const u32 tmp = (unsignedValue * 10) + (*in - '0');
        if (tmp < unsignedValue) {
            unsignedValue = (u32)0xffffffff;
            overflow = true;
        }
        if (!overflow)
            unsignedValue = tmp;
        ++in;
    }

    if (out) {
        *out = in;
    }
    return unsignedValue;
}

//! Convert a simple string of base 10 digits into a signed 32 bit integer.
/** \param[in] in: The string of digits to convert. Only a leading - or +
followed by digits 0 to 9 will be considered. Parsing stops at the first
non-digit.
\param[out] out: (optional) If provided, it will be set to point at the
first character not used in the calculation.
\return The signed integer value of the digits. If the string specifies
too many digits to encode in an s32 then +INT_MAX or -INT_MAX will be
returned.
*/
template <typename T>
inline s32 App10StrToS32(const T* in, const T** out = nullptr) {
    if (!in) {
        if (out) {
            *out = in;
        }
        return 0;
    }

    const bool negative = ('-' == *in);
    if (negative || ('+' == *in)) {
        ++in;
    }
    const u32 unsignedValue = App10StrToU32(in, out);
    if (unsignedValue > 0x7FFFFFFFU) {
        return (negative ? (-2147483647 - 1) : 0x7FFFFFFF); // (-2147483647 - 1)=INT_MIN;
    } else {
        return negative ? -((s32)unsignedValue) : (s32)unsignedValue;
    }
}

//! Convert a hex-encoded character to an unsigned integer.
/** \param[in] in The digit to convert. Only digits 0 to 9 and chars A-F,a-f
will be considered.
\return The unsigned integer value of the digit. 0 if the input is
not hex
*/
template <typename T>
inline u32 App16CharToU32(T in) {
    if (in >= '0' && in <= '9')
        return in - '0';
    else if (in >= 'a' && in <= 'f')
        return 10u + in - 'a';
    else if (in >= 'A' && in <= 'F')
        return 10u + in - 'A';
    else
        return 0;
}

//! Convert a simple string of base 16 digits into an unsigned 32 bit integer.
/** \param[in] in: The string of digits to convert. No leading chars are
allowed, only digits 0 to 9 and chars A-F,a-f are allowed. Parsing stops
at the first illegal char.
\param[out] out: (optional) If provided, it will be set to point at the
first character not used in the calculation.
\return The unsigned integer value of the digits. If the string specifies
too many digits to encode in an u32 then INT_MAX will be returned.
*/
template <typename T>
inline u32 App16StrToU32(const T* in, const T** out = nullptr) {
    if (!in) {
        if (out)
            *out = in;
        return 0;
    }

    bool overflow = false;
    u32 unsignedValue = 0;
    while (true) {
        u32 tmp = 0;
        if ((*in >= '0') && (*in <= '9'))
            tmp = (unsignedValue << 4u) + (*in - '0');
        else if ((*in >= 'A') && (*in <= 'F'))
            tmp = (unsignedValue << 4u) + (*in - 'A') + 10;
        else if ((*in >= 'a') && (*in <= 'f'))
            tmp = (unsignedValue << 4u) + (*in - 'a') + 10;
        else
            break;
        if (tmp < unsignedValue) {
            unsignedValue = 0x7FFFFFFFU;
            overflow = true;
        }
        if (!overflow)
            unsignedValue = tmp;
        ++in;
    }

    if (out)
        *out = in;

    return unsignedValue;
}

//! Convert a simple string of base 8 digits into an unsigned 32 bit integer.
/** \param[in] in The string of digits to convert. No leading chars are
allowed, only digits 0 to 7 are allowed. Parsing stops at the first illegal
char.
\param[out] out (optional) If provided, it will be set to point at the
first character not used in the calculation.
\return The unsigned integer value of the digits. If the string specifies
too many digits to encode in an u32 then INT_MAX will be returned.
*/
template <typename T>
inline u32 App8StrToU32(const T* in, const T** out = nullptr) {
    if (!in) {
        if (out)
            *out = in;
        return 0;
    }

    bool overflow = false;
    u32 unsignedValue = 0;
    while (true) {
        u32 tmp = 0;
        if ((*in >= '0') && (*in <= '7'))
            tmp = (unsignedValue << 3u) + (*in - '0');
        else
            break;
        if (tmp < unsignedValue) {
            unsignedValue = 0x7FFFFFFFU;
            overflow = true;
        }
        if (!overflow)
            unsignedValue = tmp;
        ++in;
    }

    if (out)
        *out = in;

    return unsignedValue;
}

//! Convert a C-style prefixed string (hex, oct, integer) into an unsigned 32 bit integer.
/** \param[in] in The string of digits to convert. If string starts with 0x the
hex parser is used, if only leading 0 is used, oct parser is used. In all
other cases, the usual unsigned parser is used.
\param[out] out (optional) If provided, it will be set to point at the
first character not used in the calculation.
\return The unsigned integer value of the digits. If the string specifies
too many digits to encode in an u32 then INT_MAX will be returned.
*/
template <typename T>
inline u32 AppPrefixStrToU32(const T* in, const T** out = nullptr) {
    if (!in) {
        if (out) {
            *out = in;
        }
        return 0;
    }
    if ('0' == in[0]) {
        return (('x' == in[1] || 'X' == in[1]) ? App16StrToU32(in + 2, out) : App8StrToU32(in + 1, out));
    }
    return App10StrToU32(in, out);
}

//! Converts a sequence of digits into a whole positive floating point value.
/** Only digits 0 to 9 are parsed.  Parsing stops at any other character,
including sign characters or a decimal point.
\param in: the sequence of digits to convert.
\param out: (optional) will be set to point at the first non-converted
character.
\return The whole positive floating point representation of the digit
sequence.
*/
template <typename T>
inline f32 AppStrToF32P(const T* in, const T** out = nullptr) {
    if (!in) {
        if (out) {
            *out = in;
        }
        return 0.f;
    }

    const u32 MAX_SAFE_U32_VALUE = 0xFFFFFFFFU / 10 - 10;
    u32 intValue = 0;

    // Use integer arithmetic for as long as possible, for speed
    // and precision.
    while ((*in >= '0') && (*in <= '9')) {
        // If it looks like we're going to overflow, bail out
        // now and start using floating point.
        if (intValue >= MAX_SAFE_U32_VALUE) {
            break;
        }
        intValue = (intValue * 10) + (*in - '0');
        ++in;
    }

    f32 floatValue = (f32)intValue;

    // If there are any digits left to parse, then we need to use
    // floating point arithmetic from here.
    while ((*in >= '0') && (*in <= '9')) {
        floatValue = (floatValue * 10.f) + (f32)(*in - '0');
        ++in;
        if (floatValue > 3.402823466e+38F) { //>FLT_MAX, Just give up.
            break;
        }
    }

    if (out) {
        *out = in;
    }
    return floatValue;
}

//! Provides a fast function for converting a string into a f32.
/** This is not guaranteed to be as accurate as atof(), but is
approximately 6 to 8 times as fast.
\param[in] in The string to convert.
\param[out] out Optional pointer to the first character in the string that
wasn't used to create the f32 value.
\return Float value parsed from the input string
*/
template <typename T>
inline const f32 AppStrToF32(const T* in, const T** out = nullptr) {
    // Please run the regression test when making any modifications to this function.
    if (!in) {
        if (out)
            *out = in;
        return 0.f;
    }

    const bool negative = ('-' == *in);
    if (negative || ('+' == *in))
        ++in;

    f32 value = AppStrToF32P(in, &in);

    if (APP_DECIMAL_POINTS == (*in)) {
        const char* afterDecimal = ++in;
        f32 decimal = AppStrToF32P(in, &afterDecimal);
        size_t numDecimals = afterDecimal - in;
        if (numDecimals < APP_ATOF_TABLE_SIZE) {
            value += decimal * G_FAST_ATOF_TABLE[numDecimals];
        } else {
            value += decimal * (f32)pow(10.f, -(f32)numDecimals);
        }
        in = afterDecimal;
    }

    if ('e' == *in || 'E' == *in) {
        ++in;
        // Assume that the exponent is a whole number.
        // App10StrToS32() will deal with both + and - signs,
        // but calculate as f32 to prevent overflow at FLT_MAX
        // Using pow with f32 cast instead of powf as otherwise accuracy decreases.
        value *= (f32)pow(10.f, (f32)App10StrToS32(in, &in));
    }

    if (out) {
        *out = in;
    }

    return negative ? -value : value;
}



} // end namespace app

#endif //APP_CONVERTER_H

