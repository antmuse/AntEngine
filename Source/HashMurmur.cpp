//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

#include "Config.h"


namespace app {


static DFINLINE u32 AppRotate32(u32 x, s32 r) {
    return (x << r) | (x >> (32 - r));
}

static DFINLINE u64 AppRotate64(u64 x, s32 r) {
    return (x << r) | (x >> (64 - r));
}

// Finalization mix - force all bits of a hash block to avalanche
static DFINLINE u32 AppAvalanche32(u32 h) {
    h ^= h >> 16;
    h *= 0x85ebca6bU;
    h ^= h >> 13;
    h *= 0xc2b2ae35U;
    h ^= h >> 16;
    return h;
}


// Finalization mix - force all bits of a hash block to avalanche
static DFINLINE u64 AppAvalanche64(u64 k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdLLU;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53LLU;
    k ^= k >> 33;
    return k;
}


u32 MurmurHash3_x86_32(const void* key, const u64 len, u32 seed) {
    const u8* data = (const u8*)key;
    const u64 nblocks = len / 4;
    s64 i;

    u32 h1 = seed;

    u32 c1 = 0xcc9e2d51;
    u32 c2 = 0x1b873593;

    //----------
    // body

    const u32* blocks = (const u32*)(data + nblocks * 4);

    for(i = -((s64)nblocks); i; i++) {
        u32 k1 = blocks[i];

        k1 *= c1;
        k1 = AppRotate32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = AppRotate32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail

    const u8* tail = (const u8*)(data + nblocks * 4);

    u32 k1 = 0;

    switch(len & 3) {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
        k1 *= c1; k1 = AppRotate32(k1, 15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization
    h1 ^= len;
    h1 = AppAvalanche32(h1);
    return h1;
}


void MurmurHash3_x86_128(const void* key, const u64 len, u32 seed, void* out) {
    const u8* data = (const u8*)key;
    const u64 nblocks = len / 16;
    s64 i;

    u32 h1 = seed;
    u32 h2 = seed;
    u32 h3 = seed;
    u32 h4 = seed;

    u32 c1 = 0x239b961b;
    u32 c2 = 0xab0e9789;
    u32 c3 = 0x38b34ae5;
    u32 c4 = 0xa1e38b93;

    //----------
    // body

    const u32* blocks = (const u32*)(data + nblocks * 16);

    for(i = -((s64)nblocks); i; i++) {
        u32 k1 = blocks[i * 4 + 0];
        u32 k2 = blocks[i * 4 + 1];
        u32 k3 = blocks[i * 4 + 2];
        u32 k4 = blocks[i * 4 + 3];

        k1 *= c1; k1 = AppRotate32(k1, 15); k1 *= c2; h1 ^= k1;

        h1 = AppRotate32(h1, 19); h1 += h2; h1 = h1 * 5 + 0x561ccd1b;

        k2 *= c2; k2 = AppRotate32(k2, 16); k2 *= c3; h2 ^= k2;

        h2 = AppRotate32(h2, 17); h2 += h3; h2 = h2 * 5 + 0x0bcaa747;

        k3 *= c3; k3 = AppRotate32(k3, 17); k3 *= c4; h3 ^= k3;

        h3 = AppRotate32(h3, 15); h3 += h4; h3 = h3 * 5 + 0x96cd1c35;

        k4 *= c4; k4 = AppRotate32(k4, 18); k4 *= c1; h4 ^= k4;

        h4 = AppRotate32(h4, 13); h4 += h1; h4 = h4 * 5 + 0x32ac3b17;
    }

    //----------
    // tail

    const u8* tail = (const u8*)(data + nblocks * 16);

    u32 k1 = 0;
    u32 k2 = 0;
    u32 k3 = 0;
    u32 k4 = 0;

    switch(len & 15) {
    case 15: k4 ^= tail[14] << 16;
    case 14: k4 ^= tail[13] << 8;
    case 13: k4 ^= tail[12] << 0;
        k4 *= c4; k4 = AppRotate32(k4, 18); k4 *= c1; h4 ^= k4;

    case 12: k3 ^= tail[11] << 24;
    case 11: k3 ^= tail[10] << 16;
    case 10: k3 ^= tail[9] << 8;
    case  9: k3 ^= tail[8] << 0;
        k3 *= c3; k3 = AppRotate32(k3, 17); k3 *= c4; h3 ^= k3;

    case  8: k2 ^= tail[7] << 24;
    case  7: k2 ^= tail[6] << 16;
    case  6: k2 ^= tail[5] << 8;
    case  5: k2 ^= tail[4] << 0;
        k2 *= c2; k2 = AppRotate32(k2, 16); k2 *= c3; h2 ^= k2;

    case  4: k1 ^= tail[3] << 24;
    case  3: k1 ^= tail[2] << 16;
    case  2: k1 ^= tail[1] << 8;
    case  1: k1 ^= tail[0] << 0;
        k1 *= c1; k1 = AppRotate32(k1, 15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;

    h1 = AppAvalanche32(h1);
    h2 = AppAvalanche32(h2);
    h3 = AppAvalanche32(h3);
    h4 = AppAvalanche32(h4);

    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;

    ((u32*)out)[0] = h1;
    ((u32*)out)[1] = h2;
    ((u32*)out)[2] = h3;
    ((u32*)out)[3] = h4;
}


void MurmurHash3_x64_128(const void* key, const u64 len, const u32 seed, void* out) {
    const u8* data = (const u8*)key;
    const u64 nblocks = len / 16;
    u64 i;

    u64 h1 = seed;
    u64 h2 = seed;

    u64 c1 = 0x87c37b91114253d5LLU;
    u64 c2 = 0x4cf5ad432745937fLLU;

    //----------
    // body

    const u64* blocks = (const u64*)(data);

    for(i = 0; i < nblocks; i++) {
        u64 k1 = blocks[i * 2 + 0];
        u64 k2 = blocks[i * 2 + 1];

        k1 *= c1; k1 = AppRotate64(k1, 31); k1 *= c2; h1 ^= k1;

        h1 = AppRotate64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;

        k2 *= c2; k2 = AppRotate64(k2, 33); k2 *= c1; h2 ^= k2;

        h2 = AppRotate64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
    }

    //----------
    // tail

    const u8* tail = (const u8*)(data + nblocks * 16);

    u64 k1 = 0;
    u64 k2 = 0;

    switch(len & 15) {
    case 15: k2 ^= (u64)(tail[14]) << 48;
    case 14: k2 ^= (u64)(tail[13]) << 40;
    case 13: k2 ^= (u64)(tail[12]) << 32;
    case 12: k2 ^= (u64)(tail[11]) << 24;
    case 11: k2 ^= (u64)(tail[10]) << 16;
    case 10: k2 ^= (u64)(tail[9]) << 8;
    case  9: k2 ^= (u64)(tail[8]) << 0;
        k2 *= c2; k2 = AppRotate64(k2, 33); k2 *= c1; h2 ^= k2;

    case  8: k1 ^= (u64)(tail[7]) << 56;
    case  7: k1 ^= (u64)(tail[6]) << 48;
    case  6: k1 ^= (u64)(tail[5]) << 40;
    case  5: k1 ^= (u64)(tail[4]) << 32;
    case  4: k1 ^= (u64)(tail[3]) << 24;
    case  3: k1 ^= (u64)(tail[2]) << 16;
    case  2: k1 ^= (u64)(tail[1]) << 8;
    case  1: k1 ^= (u64)(tail[0]) << 0;
        k1 *= c1; k1 = AppRotate64(k1, 31); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len; h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = AppAvalanche64(h1);
    h2 = AppAvalanche64(h2);

    h1 += h2;
    h2 += h1;

    ((u64*)out)[0] = h1;
    ((u64*)out)[1] = h2;
}

//针对64位处理器的Murmur哈希
u64 MurmurHash_x64(const void* key, u64 len, u32 seed) {
    const u64 m = 0xc6a4a7935bd1e995;
    const int r = 47;

    u64 h = seed ^ (len * m);

    const u64* data = (const u64*)key;
    const u64* end = data + (len / 8);

    while(data != end) {
        u64 k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const u8* data2 = (const u8*)data;

    switch(len & 7) {
    case 7: h ^= u64(data2[6]) << 48;
    case 6: h ^= u64(data2[5]) << 40;
    case 5: h ^= u64(data2[4]) << 32;
    case 4: h ^= u64(data2[3]) << 24;
    case 3: h ^= u64(data2[2]) << 16;
    case 2: h ^= u64(data2[1]) << 8;
    case 1: h ^= u64(data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

//针对32位处理器的Murmur哈希
u64 MurmurHash_x86(const void* key, u64 len, u32 seed) {
    const u32 m = 0x5bd1e995U;
    const int r = 24;

    u32 h1 = (u32)(seed ^ len);
    u32 h2 = 0;

    const u32* data = (const u32*)key;

    while(len >= 8) {
        u32 k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;

        u32 k2 = *data++;
        k2 *= m; k2 ^= k2 >> r; k2 *= m;
        h2 *= m; h2 ^= k2;
        len -= 4;
    }

    if(len >= 4) {
        u32 k1 = *data++;
        k1 *= m; k1 ^= k1 >> r; k1 *= m;
        h1 *= m; h1 ^= k1;
        len -= 4;
    }

    switch(len) {
    case 3: h2 ^= ((u8*)data)[2] << 16;
    case 2: h2 ^= ((u8*)data)[1] << 8;
    case 1: h2 ^= ((u8*)data)[0];
        h2 *= m;
    };

    h1 ^= h2 >> 18; h1 *= m;
    h2 ^= h1 >> 22; h2 *= m;
    h1 ^= h2 >> 17; h1 *= m;
    h2 ^= h1 >> 19; h2 *= m;

    u64 h = h1;

    h = (h << 32) | h2;

    return h;
}


//hash seed: Murmur, 种子最好用一个质数
static u32 G_HASH_SEED_MMUR = 0xEE6B27EBU;  //一个40亿内的质数

void AppSetHashSeedMurmur(const u32 seed) {
    G_HASH_SEED_MMUR = seed;
}

u32& AppGetHashSeedMurmur() {
    return G_HASH_SEED_MMUR;
}

u32 AppHashMurmur32(const void* buf, u64 len) {
    return MurmurHash3_x86_32(buf, len, G_HASH_SEED_MMUR);
}

u64 AppHashMurmur64(const void* buf, u64 len) {
#ifdef DOS_64BIT
    return MurmurHash_x64(buf, len, G_HASH_SEED_MMUR);
#else
    return MurmurHash_x86(buf, len, G_HASH_SEED_MMUR);
#endif
}

void AppHashMurmur128(const void* buf, u64 len, void* out) {
#ifdef DOS_64BIT
    MurmurHash3_x64_128(buf, len, G_HASH_SEED_MMUR, out);
#else
    MurmurHash3_x86_128(buf, len, G_HASH_SEED_MMUR, out);
#endif
}

}//namespace app
