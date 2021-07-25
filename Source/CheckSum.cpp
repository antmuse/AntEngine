#include "CheckSum.h"
namespace app {


inline u32 AppCheckOverflow(u32 it) {
    it = (it >> 16) + (it & 0xFFFF);
    /*if(it > 0xFFFF) {
        it -= 0xFFFF;
    }
    return it;
    */
    return (it >> 16) + (it & 0xFFFF);
}


void CheckSum::add(const void* iData, s32 iSize) {
    if (iSize <= 0) {
        return;
    }
    register const u16* buffer = (const u16*)iData;
    s32 byte_swapped = 0;
    register s32 mlen = 0;

    //leftover byte of last buffer
    if (mHaveTail) {
        *(((u8*)&mLeftover) + 1) = *(const u8*)buffer;
        mSum += mLeftover;
        mHaveTail = false;
        buffer = (const u16*)(((const u8*)buffer) + 1);
        mlen = iSize - 1;
    } else {
        mlen = iSize;
    }
    /*
    指针非内存对齐
    先将未对其的1个 byte 暂存，这样可迫使指针对齐，
    但又为了让同奇位、同偶位内存相加，所以使 mSum<<8
    如果前面mSum是已经左移过的，则再次 mSum<<8，让mSum回归最初的奇偶次序
    */
    if ((1 & ((size_t)buffer)) && (mlen > 0)) {
        mSum = AppCheckOverflow(mSum);
        mSum <<= 8;
        mLeftover = 0;
        *((u8*)(&mLeftover)) = *(const u8*)buffer;
        buffer = (const u16*)(((const u8*)buffer) + 1);
        mlen--;
        byte_swapped = 1;
        mHaveTail = true;
    }
    /*
    * Unroll the loop to make overhead from
    * branches &c small.
    */
    while ((mlen -= 32) >= 0) {
        mSum += buffer[0];
        mSum += buffer[1];
        mSum += buffer[2];
        mSum += buffer[3];
        mSum += buffer[4];
        mSum += buffer[5];
        mSum += buffer[6];
        mSum += buffer[7];
        mSum += buffer[8];
        mSum += buffer[9];
        mSum += buffer[10];
        mSum += buffer[11];
        mSum += buffer[12];
        mSum += buffer[13];
        mSum += buffer[14];
        mSum += buffer[15];
        buffer += 16;
    }
    mlen += 32;
    while ((mlen -= 8) >= 0) {
        mSum += buffer[0];
        mSum += buffer[1];
        mSum += buffer[2];
        mSum += buffer[3];
        buffer += 4;
    }
    mlen += 8;
    if (mlen == 0 && byte_swapped == 0) {
        return;
    }
    mSum = AppCheckOverflow(mSum);
    while ((mlen -= 2) >= 0) {
        mSum += *buffer++;
    }
    if (byte_swapped) {
        mSum = AppCheckOverflow(mSum);
        mSum <<= 8;
        byte_swapped = 0;
        if (mlen == -1) {
            *(((u8*)&mLeftover) + 1) = *(const u8*)buffer;
            mSum += mLeftover;
            mlen = 0;
            mHaveTail = false;
        } else {
            mlen = -1;
        }
    } else if (mlen == -1) {//当前内存块还余一个byte
        mLeftover = 0;
        *((u8*)(&mLeftover)) = *(const u8*)buffer;
        mHaveTail = true;
    }

    mSum = AppCheckOverflow(mSum);
}


u16 CheckSum::finish() {
    //对尾部最后一个 byte 的处理
    if (mHaveTail) {
        *(((u8*)&mLeftover) + 1) = 0;
        mSum += mLeftover;
        mHaveTail = false;
        mSum = AppCheckOverflow(mSum);
    }
    return (~mSum & 0xFFFF);
}


u16 CheckSum::get()const {
    u32 ret = mSum;
    if (mHaveTail) {
        *(((u8*)&mLeftover) + 1) = 0;
        ret += mLeftover;
        ret = AppCheckOverflow(ret);
    }
    return (~ret & 0xFFFF);
}

} //namespace app
