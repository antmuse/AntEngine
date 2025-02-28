#include "EncoderSHA1.h"
#include <string.h>

namespace app {
#define SHA_BYTE_ORDER 1234
#define SHA_VERSION 1


/* UNRAVEL should be fastest & biggest */
/* UNROLL_LOOPS should be just as big, but slightly slower */
/* both undefined should be smallest and slowest */

#define UNRAVEL
/* #define UNROLL_LOOPS */

/* SHA f()-functions */

#define f1(x, y, z) ((x & y) | (~x & z))
#define f2(x, y, z) (x ^ y ^ z)
#define f3(x, y, z) ((x & y) | (x & z) | (y & z))
#define f4(x, y, z) (x ^ y ^ z)

/* SHA constants */

#define CONST1 0x5a827999L
#define CONST2 0x6ed9eba1L
#define CONST3 0x8f1bbcdcL
#define CONST4 0xca62c1d6L

/* truncate to 32 bits -- should be a null op on 32-bit machines */

#define T32(x) ((x)&0xffffffffL)

/* 32-bit rotate */

#define R32(x, n) T32(((x << n) | (x >> (32 - n))))

/* the generic case, for when the overall rotation is not unraveled */

#define FG(n)                                                                                                          \
    T = T32(R32(A, 5) + f##n(B, C, D) + E + *WP++ + CONST##n);                                                         \
    E = D;                                                                                                             \
    D = C;                                                                                                             \
    C = R32(B, 30);                                                                                                    \
    B = A;                                                                                                             \
    A = T

/* specific cases, for when the overall rotation is unraveled */

#define FA(n)                                                                                                          \
    T = T32(R32(A, 5) + f##n(B, C, D) + E + *WP++ + CONST##n);                                                         \
    B = R32(B, 30)

#define FB(n)                                                                                                          \
    E = T32(R32(T, 5) + f##n(A, B, C) + D + *WP++ + CONST##n);                                                         \
    A = R32(A, 30)

#define FC(n)                                                                                                          \
    D = T32(R32(E, 5) + f##n(T, A, B) + C + *WP++ + CONST##n);                                                         \
    T = R32(T, 30)

#define FD(n)                                                                                                          \
    C = T32(R32(D, 5) + f##n(E, T, A) + B + *WP++ + CONST##n);                                                         \
    E = R32(E, 30)

#define FE(n)                                                                                                          \
    B = T32(R32(C, 5) + f##n(D, E, T) + A + *WP++ + CONST##n);                                                         \
    D = R32(D, 30)

#define FT(n)                                                                                                          \
    A = T32(R32(B, 5) + f##n(C, D, E) + T + *WP++ + CONST##n);                                                         \
    C = R32(C, 30)



/* do SHA transformation */
void EncoderSHA1::transform(const u8* dp) {
    s32 i;
    u32 T, A, B, C, D, E, W[80], *WP;

/*
the following makes sure that at least one code block below is
traversed or an error is reported, without the necessity for nested
preprocessor if/else/endif blocks, which are a great pain in the
nether regions of the anatomy...
*/
#undef SWAP_DONE

#if (SHA_BYTE_ORDER == 1234)
#define SWAP_DONE
    for (i = 0; i < 16; ++i) {
        T = *((u32*)dp);
        dp += 4;
        W[i] = ((T << 24) & 0xff000000) | ((T << 8) & 0x00ff0000) | ((T >> 8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
    }
#endif /* SHA_BYTE_ORDER == 1234 */

#if (SHA_BYTE_ORDER == 4321)
#define SWAP_DONE
    for (i = 0; i < 16; ++i) {
        T = *((u32*)dp);
        dp += 4;
        W[i] = T32(T);
    }
#endif /* SHA_BYTE_ORDER == 4321 */

#if (SHA_BYTE_ORDER == 12345678)
#define SWAP_DONE
    for (i = 0; i < 16; i += 2) {
        T = *((u32*)dp);
        dp += 8;
        W[i] = ((T << 24) & 0xff000000) | ((T << 8) & 0x00ff0000) | ((T >> 8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
        T >>= 32;
        W[i + 1]
            = ((T << 24) & 0xff000000) | ((T << 8) & 0x00ff0000) | ((T >> 8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
    }
#endif /* SHA_BYTE_ORDER == 12345678 */

#if (SHA_BYTE_ORDER == 87654321)
#define SWAP_DONE
    for (i = 0; i < 16; i += 2) {
        T = *((u32*)dp);
        dp += 8;
        W[i] = T32(T >> 32);
        W[i + 1] = T32(T);
    }
#endif /* SHA_BYTE_ORDER == 87654321 */

#ifndef SWAP_DONE
#error Unknown byte order -- you need to add code here
#endif

    for (i = 16; i < 80; ++i) {
        W[i] = W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16];
#if (SHA_VERSION == 1)
        W[i] = R32(W[i], 1); // SHA-1
// #else SHA, do nothing;
#endif
    }
    A = mDigest[0];
    B = mDigest[1];
    C = mDigest[2];
    D = mDigest[3];
    E = mDigest[4];
    WP = W;
#ifdef UNRAVEL
    FA(1);
    FB(1);
    FC(1);
    FD(1);
    FE(1);
    FT(1);
    FA(1);
    FB(1);
    FC(1);
    FD(1);
    FE(1);
    FT(1);
    FA(1);
    FB(1);
    FC(1);
    FD(1);
    FE(1);
    FT(1);
    FA(1);
    FB(1);
    FC(2);
    FD(2);
    FE(2);
    FT(2);
    FA(2);
    FB(2);
    FC(2);
    FD(2);
    FE(2);
    FT(2);
    FA(2);
    FB(2);
    FC(2);
    FD(2);
    FE(2);
    FT(2);
    FA(2);
    FB(2);
    FC(2);
    FD(2);
    FE(3);
    FT(3);
    FA(3);
    FB(3);
    FC(3);
    FD(3);
    FE(3);
    FT(3);
    FA(3);
    FB(3);
    FC(3);
    FD(3);
    FE(3);
    FT(3);
    FA(3);
    FB(3);
    FC(3);
    FD(3);
    FE(3);
    FT(3);
    FA(4);
    FB(4);
    FC(4);
    FD(4);
    FE(4);
    FT(4);
    FA(4);
    FB(4);
    FC(4);
    FD(4);
    FE(4);
    FT(4);
    FA(4);
    FB(4);
    FC(4);
    FD(4);
    FE(4);
    FT(4);
    FA(4);
    FB(4);
    mDigest[0] = T32(mDigest[0] + E);
    mDigest[1] = T32(mDigest[1] + T);
    mDigest[2] = T32(mDigest[2] + A);
    mDigest[3] = T32(mDigest[3] + B);
    mDigest[4] = T32(mDigest[4] + C);
#else /* !UNRAVEL */
#ifdef UNROLL_LOOPS
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(1);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(2);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(3);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
    FG(4);
#else  /* !UNROLL_LOOPS */
    for (i = 0; i < 20; ++i) {
        FG(1);
    }
    for (i = 20; i < 40; ++i) {
        FG(2);
    }
    for (i = 40; i < 60; ++i) {
        FG(3);
    }
    for (i = 60; i < 80; ++i) {
        FG(4);
    }
#endif /* !UNROLL_LOOPS */
    mDigest[0] = T32(mDigest[0] + A);
    mDigest[1] = T32(mDigest[1] + B);
    mDigest[2] = T32(mDigest[2] + C);
    mDigest[3] = T32(mDigest[3] + D);
    mDigest[4] = T32(mDigest[4] + E);
#endif /* !UNRAVEL */
}



EncoderSHA1::EncoderSHA1() {
    clear();
}

EncoderSHA1::~EncoderSHA1() {
}

String EncoderSHA1::finish() {
    u8 key[SHA_DIGESTSIZE];
    finish(key);
    s8 out[sizeof(key) * 2 + 1];
    AppBufToHex((u8*)key, sizeof(key), out, sizeof(out));
    return out;
}


void EncoderSHA1::clear() {
    mDigest[0] = 0x67452301L;
    mDigest[1] = 0xefcdab89L;
    mDigest[2] = 0x98badcfeL;
    mDigest[3] = 0x10325476L;
    mDigest[4] = 0xc3d2e1f0L;
    mCountLow = 0;
    mCountHi = 0;
    mLocal = 0;
}

/* update the SHA digest */
void EncoderSHA1::add(const void* data, usz count) {
    const u8* buffer = static_cast<const u8*>(data);
    u32 clo = T32(mCountLow + ((u32)count << 3));
    if (clo < mCountLow) {
        ++mCountHi;
    }
    mCountLow = clo;
    mCountHi += (u32)(count >> 29);
    if (mLocal) {
        u32 i = SHA_BLOCKSIZE - mLocal;
        if (((usz)i) > count) {
            i = (u32)count;
        }
        memcpy(mData + mLocal, buffer, i);
        mLocal += i;
        if (mLocal == SHA_BLOCKSIZE) {
            count -= i;
            buffer += i;
            transform(mData);
        } else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        transform(buffer);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
    }
    memcpy(mData, buffer, count);
    mLocal = (u32)count; // leftover byte < SHA_BLOCKSIZE
}

void EncoderSHA1::finish(u8 digest[20]) {
    u32 lo_bit_count = mCountLow;
    u32 hi_bit_count = mCountHi;
    s32 count = (s32)((lo_bit_count >> 3) & 0x3f);
    mData[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
        memset(mData + count, 0, SHA_BLOCKSIZE - count);
        transform(mData);
        memset(mData, 0, SHA_BLOCKSIZE - 8);
    } else {
        memset(mData + count, 0, SHA_BLOCKSIZE - 8 - count);
    }
    mData[56] = (u8)((hi_bit_count >> 24) & 0xff);
    mData[57] = (u8)((hi_bit_count >> 16) & 0xff);
    mData[58] = (u8)((hi_bit_count >> 8) & 0xff);
    mData[59] = (u8)((hi_bit_count >> 0) & 0xff);
    mData[60] = (u8)((lo_bit_count >> 24) & 0xff);
    mData[61] = (u8)((lo_bit_count >> 16) & 0xff);
    mData[62] = (u8)((lo_bit_count >> 8) & 0xff);
    mData[63] = (u8)((lo_bit_count >> 0) & 0xff);
    transform(mData);
    digest[0] = (u8)((mDigest[0] >> 24) & 0xff);
    digest[1] = (u8)((mDigest[0] >> 16) & 0xff);
    digest[2] = (u8)((mDigest[0] >> 8) & 0xff);
    digest[3] = (u8)((mDigest[0]) & 0xff);
    digest[4] = (u8)((mDigest[1] >> 24) & 0xff);
    digest[5] = (u8)((mDigest[1] >> 16) & 0xff);
    digest[6] = (u8)((mDigest[1] >> 8) & 0xff);
    digest[7] = (u8)((mDigest[1]) & 0xff);
    digest[8] = (u8)((mDigest[2] >> 24) & 0xff);
    digest[9] = (u8)((mDigest[2] >> 16) & 0xff);
    digest[10] = (u8)((mDigest[2] >> 8) & 0xff);
    digest[11] = (u8)((mDigest[2]) & 0xff);
    digest[12] = (u8)((mDigest[3] >> 24) & 0xff);
    digest[13] = (u8)((mDigest[3] >> 16) & 0xff);
    digest[14] = (u8)((mDigest[3] >> 8) & 0xff);
    digest[15] = (u8)((mDigest[3]) & 0xff);
    digest[16] = (u8)((mDigest[4] >> 24) & 0xff);
    digest[17] = (u8)((mDigest[4] >> 16) & 0xff);
    digest[18] = (u8)((mDigest[4] >> 8) & 0xff);
    digest[19] = (u8)((mDigest[4]) & 0xff);
}


} // namespace app
