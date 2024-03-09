#include "StrConverter.h"

/*
 * From rfc3629, the UTF-8 spec:
 *	http://www.ietf.org/rfc/rfc3629.txt
 *
 *	 Char. number range  |		  UTF-8 octet sequence
 *		(hexadecimal)	 |				(binary)
 *	 --------------------+-------------------------------------
 *	 0000 0000-0000 007F | 0xxxxxxx
 *	 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
 *	 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
 *	 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */



namespace app {


/*
 * This may not be the best value, but it's one that isn't represented
 *	in Unicode (0x10FFFF is the largest codepoint value). We return this
 *	value from AppUTF8Codepoint() if there's bogus bits in the
 *	stream. AppUTF8Codepoint() will turn this value into something
 *	reasonable (like a question mark), for text that wants to try to recover,
 *	whereas utf8valid() will use the value to determine if a string has bad
 *	bits.
 */
#define UNICODE_BOGUS_CHAR_VALUE 0xFFFFFFFF

/*
 * This is the codepoint we currently return when there was bogus bits in a UTF-8 string.
 */
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'

static u32 AppUTF8Codepoint(const s8** _str) {
    const s8* str = *_str;
    u32 retval = 0;
    u32 octet = (u32)((u8)*str);
    u32 octet2, octet3, octet4;

    if (octet == 0) { /* null terminator, end of string. */
        return 0;
    } else if (octet < 128) { /* one octet s8: 0 to 127 */
        (*_str)++;            /* skip to next possible start of codepoint. */
        return (octet);
    } else if ((octet > 127) && (octet < 192)) { // bad (starts with 10xxxxxx)
        /*
         * Apparently each of these is supposed to be flagged as a bogus
         *	s8, instead of just resyncing to the next valid codepoint.
         */
        (*_str)++; /* skip to next possible start of codepoint. */
        return UNICODE_BOGUS_CHAR_VALUE;
    } else if (octet < 224) { // two octets
        octet -= (128 + 64);
        octet2 = (u32)((u8) * (++str));
        if ((octet2 & (128 + 64)) != 128) { /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;
        }
        *_str += 2; /* skip to next possible start of codepoint. */
        retval = ((octet << 6) | (octet2 - 128));
        if ((retval >= 0x80) && (retval <= 0x7FF))
            return retval;
    } else if (octet < 240) { // three octets
        octet -= (128 + 64 + 32);
        octet2 = (u32)((u8) * (++str));
        if ((octet2 & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet3 = (u32)((u8) * (++str));
        if ((octet3 & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 3; /* skip to next possible start of codepoint. */
        retval = (((octet << 12)) | ((octet2 - 128) << 6) | ((octet3 - 128)));

        /* There are seven "UTF-16 surrogates" that are illegal in UTF-8. */
        switch (retval) {
        case 0xD800:
        case 0xDB7F:
        case 0xDB80:
        case 0xDBFF:
        case 0xDC00:
        case 0xDF80:
        case 0xDFFF:
            return UNICODE_BOGUS_CHAR_VALUE;
        } // switch

        /* 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge. */
        if ((retval >= 0x800) && (retval <= 0xFFFD))
            return retval;
    } else if (octet < 248) { // four octets
        octet -= (128 + 64 + 32 + 16);
        octet2 = (u32)((u8) * (++str));
        if ((octet2 & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet3 = (u32)((u8) * (++str));
        if ((octet3 & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet4 = (u32)((u8) * (++str));
        if ((octet4 & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 4; /* skip to next possible start of codepoint. */
        retval = (((octet << 18)) | ((octet2 - 128) << 12) | ((octet3 - 128) << 6) | ((octet4 - 128)));
        if ((retval >= 0x10000) && (retval <= 0x10FFFF))
            return retval;
    } else if (octet < 252) { // five octets
        /*
         * Five and six octet sequences became illegal in rfc3629.
         *	We throw the codepoint away, but parse them to make sure we move
         *	ahead the right number of bytes and don't overflow the buffer.
         */
        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 5; /* skip to next possible start of codepoint. */
        return UNICODE_BOGUS_CHAR_VALUE;
    } else { // six octets
        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (u32)((u8) * (++str));
        if ((octet & (128 + 64)) != 128) /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 6; /* skip to next possible start of codepoint. */
        return UNICODE_BOGUS_CHAR_VALUE;
    }

    return UNICODE_BOGUS_CHAR_VALUE;
}


usz AppUTF8ToUCS4(const s8* src, u32* dst, usz len) {
    if (len < sizeof(u32)) {
        return 0;
    }
    const u32* start = dst;
    len -= sizeof(u32); // for str tail
    while (len >= sizeof(u32)) {
        u32 cp = AppUTF8Codepoint(&src);
        if (cp == 0) {
            break;
        } else if (cp == UNICODE_BOGUS_CHAR_VALUE) {
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;
        }
        *(dst++) = cp;
        len -= sizeof(u32);
    }
    *dst = 0;
    return dst - start;
}


usz AppUTF8ToUCS2(const s8* src, u16* dst, usz len) {
    if (len < sizeof(u16)) {
        return 0;
    }
    const u16* start = dst;
    len -= sizeof(u16); // for str tail
    while (len >= sizeof(u16)) {
        u32 cp = AppUTF8Codepoint(&src);
        if (cp == 0) {
            break;
        } else if (cp == UNICODE_BOGUS_CHAR_VALUE) {
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;
        }
        // UTF-16 surrogates
        if (cp > 0xFFFF) {
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;
        }
        *(dst++) = cp;
        len -= sizeof(u16);
    }
    *dst = 0;
    return dst - start;
}


static usz AppUTF8FromCodepoint(u32 cp, s8** _dst, usz* _len) {
    s8* dst = *_dst;
    usz len = *_len;
    if (len == 0) {
        return 0;
    }
    if (cp > 0x10FFFF) {
        cp = UNICODE_BOGUS_CHAR_CODEPOINT;
    } else if ((cp == 0xFFFE) || (cp == 0xFFFF)) { /* illegal values. */
        cp = UNICODE_BOGUS_CHAR_CODEPOINT;
    } else {
        /* There are seven "UTF-16 surrogates" that are illegal in UTF-8. */
        switch (cp) {
        case 0xD800:
        case 0xDB7F:
        case 0xDB80:
        case 0xDBFF:
        case 0xDC00:
        case 0xDF80:
        case 0xDFFF:
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;
        }
    }

    /* Do the encoding... */
    if (cp < 0x80) {
        *(dst++) = (s8)cp;
        len--;
    } else if (cp < 0x800) {
        if (len < 2) {
            *_len = 0;
            return 0; // len = 0;
        } else {
            *(dst++) = (s8)((cp >> 6) | 128 | 64);
            *(dst++) = (s8)(cp & 0x3F) | 128;
            len -= 2;
        }
    } else if (cp < 0x10000) {
        if (len < 3) {
            *_len = 0;
            return 0; // len = 0;
        } else {
            *(dst++) = (s8)((cp >> 12) | 128 | 64 | 32);
            *(dst++) = (s8)((cp >> 6) & 0x3F) | 128;
            *(dst++) = (s8)(cp & 0x3F) | 128;
            len -= 3;
        }
    } else {
        if (len < 4) {
            *_len = 0;
            return 0; // len = 0;
        } else {
            *(dst++) = (s8)((cp >> 18) | 128 | 64 | 32 | 16);
            *(dst++) = (s8)((cp >> 12) & 0x3F) | 128;
            *(dst++) = (s8)((cp >> 6) & 0x3F) | 128;
            *(dst++) = (s8)(cp & 0x3F) | 128;
            len -= 4;
        }
    }

    usz ret = *_len - len;
    *_dst = dst;
    *_len = len;
    return ret;
}

template <class T>
DFINLINE static usz AppTypeToUTF8(const T* src, s8* dst, usz len) {
    if (len == 0) {
        return 0;
    }
    --len;
    usz ret = 0;
    while (len) {
        const u32 cp = (u32)((T)(*(src++)));
        if (cp == 0) {
            break;
        }
        ret += AppUTF8FromCodepoint(cp, &dst, &len);
    }
    *dst = '\0';
    return ret;
}


usz AppUCS4ToUTF8(const u32* src, s8* dst, usz len) {
    return AppTypeToUTF8<u32>(src, dst, len);
}

usz AppUCS2ToUTF8(const u16* src, s8* dst, usz len) {
    return AppTypeToUTF8<u16>(src, dst, len);
}


usz AppUTF8ToWchar(const s8* in, wchar_t* out, const usz len) {
#if defined(DOS_WINDOWS)
    return AppUTF8ToUCS2(in, (u16*)out, len);
#else
    return AppUTF8ToUCS4(in, (u32*)out, len);
#endif
}


usz AppWcharToUTF8(const wchar_t* in, s8* out, const usz len) {
#if defined(DOS_WINDOWS)
    return AppUCS2ToUTF8((const u16*)in, out, len);
#else
    return AppUCS4ToUTF8((const u32*)in, out, len);
#endif
}

usz AppBufToHex(const void* src, usz insize, s8* dest, usz osize) {
    DASSERT(src && dest && osize);
    const static u8 hextab[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    usz ret = 0;
    if (src && dest && osize > 0) {
        const u8* buf = static_cast<const u8*>(src);
        while (ret + 2 < osize && insize > 0) {
            *dest++ = hextab[(*buf) >> 4];
            *dest++ = hextab[(*buf++) & 0xF];
            ret += 2;
            --insize;
        }
        *dest = 0;
    }
    return ret;
}
usz AppHexToBuf(const s8* src, usz insize, void* dest, usz osize) {
    DASSERT(src && dest && osize);
    const static u8 GMAP_UN_HEX[256] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0, 1, 2,
        3, 4, 5, 6, 7, 8, 9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 10, 11, 12, 13, 14, 15, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 10, 11, 12, 13, 14, 15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    usz ret = 0;
    if (src && dest && osize > 0) {
        u8* buf = static_cast<u8*>(dest);
        while (ret < osize && insize > 1) {
            *buf = GMAP_UN_HEX[static_cast<u8>(*src++)] << 4;
            *buf++ |= GMAP_UN_HEX[static_cast<u8>(*src++)];
            insize -= 2;
            ++ret;
        }
        if (ret < osize) {
            *buf = 0;
        }
    }
    return ret;
}


} // end namespace app
