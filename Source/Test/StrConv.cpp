#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <vector>
#include "Timer.h"
#include "TVector.h"
#include "Strings.h"
#include "Spinlock.h"
#include "CodecBase64.h"
#include "EncoderMD5.h"

namespace app {

void AppTestStrConv() {
    s8 tmp[32] = "aBcv";
    StringView str(tmp, 3);
    printf("str = %.*s\n", (s32)str.mLen, str.mData);
    str.toLower();
    printf("str = %.*s\n", (s32)str.mLen, str.mData);
    str.toUpper();
    printf("str = %.*s\n", (s32)str.mLen, str.mData);

    const wchar_t* src = L"use果汁";
    const s8* src8 = u8"use果汁";
    s8 str8[128];
    wchar_t wstr[128];
    usz len = AppWcharToUTF8(src, str8, sizeof(str8));
    s32 cmp = memcmp(src8, str8, len);
    printf("cmp = %d, len=%llu\n", cmp, len);
    len = AppUTF8ToWchar(src8, wstr, sizeof(wstr));



    Spinlock spi;
    spi.lock();

    bool trlk = spi.trylock();
    spi.unlock();
    trlk = spi.trylock();
    trlk = spi.trylock();
}

void AppTestBase64() {
    //base64 = "YmFzZTY0dGVzdA=="
    const s8* src = "base64test";
    const s32 strsize = (s32)strlen(src);
    s32 esize = CodecBase64::getEncodeCacheSize(strsize);
    s8* dest = new s8[esize];
    esize = CodecBase64::encode((app::u8*)src, strsize, dest);
    printf("src=%s/%d, encode=%s/%d\n", src, strsize, dest, esize);

    s32 dsize = CodecBase64::getDecodeCacheSize(esize);
    s8* decode = new s8[dsize];
    dsize = CodecBase64::decode(dest, esize, (app::u8*)decode);
    printf("decode=%.*s/%d, encode=%s/%d\n", dsize, decode, dsize, dest, esize);
    delete[] dest;
    delete[] decode;
}

void AppTestMD5() {
    EncoderMD5 md5;
    //md5 = "9567E4269E9974E8B28C2F903037F501"
    const s8* src = "base64test";
    const s32 strsize = (s32)strlen(src);
    md5.add(src, strsize);
    ID128 resID;
    md5.finish(resID);
    u8* res = (u8*)&resID;
    printf("str[%d] = %s, md5=", strsize, src);
    for (s32 i = 0; i < EncoderMD5::GMD5_LENGTH; ++i) {
        printf("%.2X", res[i]);
    }
    printf("\n");
}


void AppTestStr() {
    std::string stds1, stds2;
    const s8* ss1 = stds1.c_str();
    const s8* ss2 = stds2.c_str();
    String null;
    String str1, str2;
    WString wstr1, wstr2;
    DASSERT(str1 != nullptr);
    DASSERT(str1 == "");

    DASSERT(str1 == str2);
    str1 = 234;
    DASSERT(str1 != str2);
    str2 = 234;
    DASSERT(str1 == str2);
    str2 = 2356;
    DASSERT(str1 < str2);
    str2 = 1356;
    DASSERT(str1 > str2);
    printf("str1=%s\n", str1.c_str());

    str1 = "st\nvip\ns23vipEND";
    str1.replace('\n', '\r');
    ssz pos = str1.findFirst('\r');
    str1.remove('\r');
    pos = str1.find("vip");
    pos = str1.find("vip", pos + 1);
    pos = str1.findLast('P');
    pos = str1.findLast('p');
    DASSERT(str1.equalsNocase("stVIps23viPend"));
    DASSERT(str1.equalsn("stvipRS3viPend", 5));
    str2 = str1.subString(2, 3);
    printf("str1=%s\n", str1.c_str());
    wstr2 = str1;
    str1.replace("vip", "[---VIP---]");
    printf("str1=%s\n", str1.c_str());
    str1 = wstr2;
    str1.replace("vip", "");
    printf("str1=%s\n", str1.c_str());
    str1 = wstr2;
    str1.replace("st", "#@");
    printf("str1=%s\n", str1.c_str());
    str1.replace("#@", "");
    printf("str1=%s\n", str1.c_str());
    str2.toUpper();
    str1.remove(str2);
    str1 = "\r\nstlewls\t\n\t";
    str1.removeChars("\t\r\n");
    str1 = "\r\nstlew\tls\tv\n\r\r\n";
    str1.trim("\r\n");
    printf("str1=%s\n", str1.c_str());
    printf("str2=%s\n", str2.c_str());

    str1 += str1;
    str1 += '+';
    str1 += str1;
    printf("str1=%s\n", str1.c_str());


    str1 = 0xFFFFFFFF;
    printf("str1=%s\n", str1.c_str());

    str1 = FLT_MIN;
    str1 = FLT_MAX;
    printf("str1=%s\n", str1.c_str());

    str1 = DBL_MIN;
    str1 = DBL_MAX;
    printf("str1=%s\n", str1.c_str());

    str1.setLen(20);
    *(s8*)(str1.c_str() + 20) = 'E';
    str1.validate();
    str1.erase(2);
    str1[4] = 0;
    str1.validate();
    str1 = "UPStr";
    str1.toLower();
    printf("str1=%s\n", str1.c_str());
    //str1.split()

    str1 = "d:\\path\\fname.dat";
    bool ext = str1.isFileExtension("Dat");
    str1.deleteFilenameExtension();
    printf("str1=%s\n", str1.c_str());
    str1.deleteFilename();
    printf("str1=%s\n", str1.c_str());
    str1 = "d:\\path\\tws\\nkls\\fname.dat";
    str1.trimPath(2);
    str1.deletePathFromFilename();
    printf("str1=%s\n", str1.c_str());
}

void AppPrintIntVector(s32* in, usz max) {
    for (usz i = 0; i < max; ++i) {
        printf("%llu)%d,", i, in[i]);
    }
}

void AppTestVector() {
    std::vector<s32> svec;
    svec.reserve(12);
    svec.resize(8);
    usz cap = svec.capacity();
    TVector<s32> arr(16);
    srand((u32)Timer::getTimestamp());
    arr.setUsed(80);

    printf("src arr\n\t");
    for (s32 i = 0; i < arr.size(); ++i) {
        arr[i] = rand();
        printf("%d)%d,", i, arr[i]);
    }
    printf("\n");

    //AppSelectSort(arr.getPointer(), arr.size());
    //printf("select sorted arr\n\t");
    //AppPrintIntVector(arr.getPointer(), arr.size());
    //printf("\n");

    s32 nd = arr[arr.size() >> 1];
    s64 pos = arr.linearSearch(nd);
    arr.setSorted(false);
    arr.heapSort(true);
    pos = arr.linearReverseSearch(nd);
    s64 pos2 = arr.linearSearch(nd);
    pos2 = arr.binarySearch(nd);

    printf("heap sorted arr\n\t");
    AppPrintIntVector(arr.getPointer(), arr.size());
    printf("\n");


    arr.setSorted(false);
    arr.heapSort(false);
    printf("heap sorted arr\n\t");
    AppPrintIntVector(arr.getPointer(), arr.size());
    printf("\n");


    arr.setSorted(false);
    arr.quickSort(true);
    printf("quick sorted arr\n\t");
    AppPrintIntVector(arr.getPointer(), arr.size());
    printf("\n");

    arr.setSorted(false);
    arr.quickSort(false);
    printf("quick sorted arr\n\t");
    AppPrintIntVector(arr.getPointer(), arr.size());
    printf("\n");


    static s32 G_BUILD_CNT = 0;
    class VNode {
    public:
        VNode() {
            ++G_BUILD_CNT;
            mStr = new s8[16];
            snprintf(mStr, 16, "%d", G_BUILD_CNT);
            mStr[0] = 0;
        }
        ~VNode() {
            --G_BUILD_CNT;
            delete[] mStr;
            mStr = nullptr;
        }
    private:
        s8* mStr;
    };

    TVector<VNode>& vnds = *(new TVector<VNode>());
    vnds.resize(8);
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    vnds.resize(3);
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    vnds.resize(5);
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    vnds.setUsed(2); //mem leak
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    delete &vnds;
    printf("mem leak=5-2, G_BUILD_CNT=%d\n", G_BUILD_CNT);
}


} //namespace app
