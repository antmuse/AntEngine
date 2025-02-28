#include "EncoderMD5.h"
#include <cstring>

namespace app {
#define MD5_BLOCKSIZE 64
/*
 * The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z) (((x) ^ (y)) ^ (z))
#define H2(x, y, z) ((x) ^ ((y) ^ (z)))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

/*
 * The MD5 transformation for all four rounds.
 */
#define STEP(f, a, b, c, d, x, t, s)                                                                                   \
    (a) += f((b), (c), (d)) + (x) + (t);                                                                               \
    (a) = (((a) << (s)) | (((a)&0xffffffff) >> (32 - (s))));                                                           \
    (a) += (b);

/*
 * SET reads 4 input bytes in little-endian byte order and stores them
 * in a properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned
 * memory accesses is just an optimization.  Nothing will break if it
 * doesn't work.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
#define SET(n) (*(u32*)&ptr[(n)*4])
#define GET(n) SET(n)
#else
#define SET(n)                                                                                                         \
    (mBlock[(n)]                                                                                                       \
        = (u32)ptr[(n)*4] | ((u32)ptr[(n)*4 + 1] << 8) | ((u32)ptr[(n)*4 + 2] << 16) | ((u32)ptr[(n)*4 + 3] << 24))
#define GET(n) (mBlock[(n)])
#endif


/*
 * This processes one or more 64-byte data blocks, but does NOT update
 * the bit counters.  There are no alignment requirements.
 */
const void* EncoderMD5::transform(const u8* ptr, usz size) {
    u32 a, b, c, d;
    u32 saved_a, saved_b, saved_c, saved_d;

    a = mA;
    b = mB;
    c = mC;
    d = mD;

    do {
        saved_a = a;
        saved_b = b;
        saved_c = c;
        saved_d = d;

        /* Round 1 */
        STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
        STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
        STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
        STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
        STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
        STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
        STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
        STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
        STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
        STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
        STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
        STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
        STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
        STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
        STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
        STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

        /* Round 2 */
        STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
        STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
        STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
        STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
        STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
        STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
        STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
        STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
        STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
        STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
        STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
        STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
        STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
        STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
        STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
        STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

        /* Round 3 */
        STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
        STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11)
        STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
        STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23)
        STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
        STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11)
        STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
        STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23)
        STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
        STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11)
        STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
        STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23)
        STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
        STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11)
        STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
        STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23)

        /* Round 4 */
        STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
        STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
        STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
        STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
        STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
        STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
        STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
        STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
        STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
        STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
        STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
        STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
        STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
        STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
        STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
        STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

        a += saved_a;
        b += saved_b;
        c += saved_c;
        d += saved_d;

        ptr += MD5_BLOCKSIZE;
    } while (size -= MD5_BLOCKSIZE);

    mA = a;
    mB = b;
    mC = c;
    mD = d;

    return ptr;
}


void EncoderMD5::clear() {
    mA = 0x67452301;
    mB = 0xefcdab89;
    mC = 0x98badcfe;
    mD = 0x10325476;
    mCountLow = 0;
    mCountHi = 0;
}


void EncoderMD5::add(const void* data, usz size) {
    u32 saved_lo = mCountLow;
    if ((mCountLow = (saved_lo + size) & 0x1fffffff) < saved_lo) {
        mCountHi++;
    }
    mCountHi += (u32)(size >> 29);
    u32 used = saved_lo & 0x3f;
    usz available;
    if (used) {
        available = MD5_BLOCKSIZE - used;
        if (size < available) {
            memcpy(&mBuffer[used], data, size);
            return;
        }
        memcpy(&mBuffer[used], data, available);
        data = (const u8*)data + available;
        size -= available;
        transform(mBuffer, MD5_BLOCKSIZE);
    }

    if (size >= MD5_BLOCKSIZE) {
        data = transform((const u8*)data, size & ~(usz)0x3f);
        size &= 0x3f;
    }

    memcpy(mBuffer, data, size);
}


String EncoderMD5::finish() {
    u8 key[GMD5_LENGTH];
    finish(key);
    s8 out[sizeof(key) * 2 + 1];
    AppBufToHex((u8*)key, sizeof(key), out, sizeof(out));
    return out;
}


void EncoderMD5::finish(u8 result[16]) {
    u32 used = mCountLow & 0x3f;
    mBuffer[used++] = 0x80;
    u32 available = MD5_BLOCKSIZE - used;

    if (available < 8) {
        memset(&mBuffer[used], 0, available);
        transform(mBuffer, MD5_BLOCKSIZE);
        used = 0;
        available = MD5_BLOCKSIZE;
    }

    memset(&mBuffer[used], 0, available - 8);

    mCountLow <<= 3;
    mBuffer[56] = mCountLow;
    mBuffer[57] = mCountLow >> 8;
    mBuffer[58] = mCountLow >> 16;
    mBuffer[59] = mCountLow >> 24;
    mBuffer[60] = mCountHi;
    mBuffer[61] = mCountHi >> 8;
    mBuffer[62] = mCountHi >> 16;
    mBuffer[63] = mCountHi >> 24;

    transform(mBuffer, MD5_BLOCKSIZE);

    result[0] = mA;
    result[1] = mA >> 8;
    result[2] = mA >> 16;
    result[3] = mA >> 24;
    result[4] = mB;
    result[5] = mB >> 8;
    result[6] = mB >> 16;
    result[7] = mB >> 24;
    result[8] = mC;
    result[9] = mC >> 8;
    result[10] = mC >> 16;
    result[11] = mC >> 24;
    result[12] = mD;
    result[13] = mD >> 8;
    result[14] = mD >> 16;
    result[15] = mD >> 24;

    // memset(this, 0, sizeof(*this));
}


} // end namespace app
