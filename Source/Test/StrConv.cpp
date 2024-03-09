#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <vector>
#include "Timer.h"
#include "TVector.h"
#include "TList.h"
#include "Converter.h"
#include "Strings.h"
#include "Spinlock.h"
#include "CodecBase64.h"
#include "EncoderMD5.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "Packet.h"
#include "Net/HTTP/HttpMsg.h"
#include "gzip/EncoderGzip.h"
#include "gzip/DecoderGzip.h"

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

    bool trlk = spi.tryLock();
    spi.unlock();
    trlk = spi.tryLock();
    trlk = spi.tryLock();
}

void AppTestBase64() {
    // base64 = "YmFzZTY0dGVzdA=="
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

void AppTestMD5(s32 argc, s8** argv) {
    EncoderMD5 md5;
    // md5 = "9567E4269E9974E8B28C2F903037F501"
    const s8* src = "base64test";
    const s32 strsize = (s32)strlen(src);
    md5.add(src, strsize);
    ID128 resID;
    md5.finish(resID);
    u8* res = (u8*)&resID;
    printf("str[%d] = %s\n", strsize, src);
    printf("buf = %lld,%lld\nmd5 = 0x", resID.mLow, resID.mHigh);
    for (s32 i = 0; i < EncoderMD5::GMD5_LENGTH; ++i) {
        printf("%.2X", res[i]);
    }
    printf("\n");

    s8 hex[EncoderMD5::GMD5_LENGTH * 2 + 1];
    AppBufToHex(&resID, EncoderMD5::GMD5_LENGTH, hex, sizeof(hex));
    printf("AppBufToHex = 0x%s\n", hex);
    resID.clear();
    AppHexToBuf(hex, sizeof(hex) - 1, &resID, sizeof(resID));
    printf("re_buf = %lld,%lld\n", resID.mLow, resID.mHigh);
}

// String::find()性能测试
void AppTestStrFind(s32 cnt) {
    const s32 mx = 2;
    ssz pos[mx];
    String stdsrc[mx] = {
        "固实压缩：把要压缩的视为同一个文件以加大压缩比，代价是取用包中任何文件需解压整个压缩包。",
        "恢复记录：加入冗余数据用于修复，在压缩包本身损坏但恢复记录够多时可对损坏压缩包进行恢复。",
    };
    const s8* pattern[] = {
        "任何文件",
        "恢复记录够多",
    };

    s64 tm0 = Timer::getRelativeTime();
    for (s32 c = 0; c < cnt; ++c) {
        for (s32 i = 0; i < mx; ++i) {
            pos[i] = stdsrc[i].find(pattern[i], 0);
        }
    }
    s64 tm1 = Timer::getRelativeTime();
    for (s32 i = 0; i < mx; ++i) {
        printf("String::find>>src len=%llu, pos[%d] = %lld, %s\n", stdsrc[i].getLen(), i, pos[i],
            pos[i] < 0 ? "-" : stdsrc[i].c_str() + pos[i]);
    }
    f32 spd = (f32)(tm1 - tm0);
    printf("String::find>>%llums/%d, speed=%.2fms\n\n\n\n", tm1 - tm0, cnt, spd / mx);
}


void AppTestStr(s32 argc, s8** argv) {
    AppTestStrFind(argc > 2 ? App10StrToS32(argv[2]) : 1);

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
    str1.toUpper();


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

    str1 = "fss/filss.doc";
    StringView mime = net::HttpMsg::getMimeType(str1.c_str(), str1.getLen());
    printf("%s, mime[%llu]=%s\n", str1.c_str(), mime.mLen, mime.mData);
    str1 = "fss/filss.ddnss";
    mime = net::HttpMsg::getMimeType(str1.c_str(), str1.getLen());
    printf("%s, mime[%llu]=%s\n", str1.c_str(), mime.mLen, mime.mData);
    str1 = "fss/filss.HtMl";
    mime = net::HttpMsg::getMimeType(str1.c_str(), str1.getLen());
    printf("%s, mime[%llu]=%s\n", str1.c_str(), mime.mLen, mime.mData);
    str1 = "fss/filss.CPpSSV";
    mime = net::HttpMsg::getMimeType(str1.c_str(), str1.getLen() - 3);
    printf("%s, mime[%llu]=%s\n", str1.c_str(), mime.mLen, mime.mData);
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
    arr.resize(80);

    printf("src arr\n\t");
    for (s32 i = 0; i < arr.size(); ++i) {
        arr[i] = rand();
        printf("%d)%d,", i, arr[i]);
    }
    printf("\n");

    // AppSelectSort(arr.getPointer(), arr.size());
    // printf("select sorted arr\n\t");
    // AppPrintIntVector(arr.getPointer(), arr.size());
    // printf("\n");

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
        }
        ~VNode() {
            if (mStr) {
                --G_BUILD_CNT;
                delete[] mStr;
                mStr = nullptr;
            }
        }
        VNode(const VNode& it) {
            ++G_BUILD_CNT;
            mStr = new s8[16];
            snprintf(mStr, 16, "%s", it.mStr);
        }
        VNode(VNode&& it) noexcept {
            mStr = it.mStr;
            it.mStr = nullptr;
        }
        VNode& operator=(const VNode& it) {
            if (mStr != it.mStr) {
                snprintf(mStr, 16, "%s", it.mStr);
            }
            return *this;
        }
        VNode& operator=(VNode&& it) noexcept {
            if (mStr != it.mStr) {
                mStr = it.mStr;
                it.mStr = nullptr;
            }
            return *this;
        }
        const s8* getDat() const {
            return mStr;
        }

    private:
        s8* mStr;
    };
    TVector<VNode>& vnds = *(new TVector<VNode>());
    vnds.resize(8);
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    vnds.resize(20);
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    vnds.quickErase(2);
    vnds.erase(8, 4);
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    {
        VNode my[20];
        vnds.pushBack(my[0]);
        vnds.insert(my[4], 3);
        vnds.pushFront(my[7]);
        for (usz i = 0; i < sizeof(my) / sizeof(my[0]); ++i) {
            if (vnds.size() > 0) {
                for (usz i = 0; i < vnds.size(); ++i) {
                    printf("size=%llu,%s\n", i, vnds[i].getDat());
                }
            }
            if ((3 & i) == 3) {
                vnds.emplaceBack(my[i]);
            } else if ((2 & i) == 2) {
                vnds.emplaceFront(my[i]);
            } else {
                vnds.emplace(my[i], i);
            }
        }
    }
    printf("size=%llu,G_BUILD_CNT=%d\n", vnds.size(), G_BUILD_CNT);
    for (usz i = 0; i < vnds.size(); ++i) {
        printf("size=%llu,%s\n", i, vnds[i].getDat());
    }
    {
        TVector<VNode>& vvv2 = *(new TVector<VNode>());
        vvv2.resize(3);
        vvv2 = vnds;
        delete &vvv2;
    }
    delete &vnds;
    printf("mem check, G_BUILD_CNT=%d\n", G_BUILD_CNT);


    TList<VNode> llst;
    VNode ndd[10];
    llst.pushBack(ndd[0]);
    llst.pushBack(ndd[1]);
    printf("mem check, G_BUILD_CNT=%d\n", G_BUILD_CNT);
    for (usz i = 0; i < sizeof(ndd) / sizeof(ndd[0]); ++i) {
        if ((1 & i) == 1) {
            llst.emplaceFront(ndd[i]);
        } else {
            llst.emplaceBack(ndd[i]);
        }
    }
    printf("mem check, G_BUILD_CNT=%d\n", G_BUILD_CNT);
}

#if defined(DUSE_ZLIB)
// 7 zip Log/ff.ico Log/ff.gz
// 7 unzip Log/ff.gz Log/ff.ico
s32 AppTestZlib(s32 argc, s8** argv) {
    FileReader rfile;
    if (!rfile.openFile(argv[3])) {
        printf("could not open in file\n");
        return 0;
    }
    printf("in file size = %llu\n", rfile.getFileSize());
    FileWriter wfile;
    if (!wfile.openFile(argv[4], false)) {
        printf("could not open out file\n");
        return 0;
    }
    std::vector<s8> output;
    s8 tmp[1024];

    if ('z' == argv[2][0]) { // zip
        s32 level = true ? Z_BEST_COMPRESSION : Z_NO_COMPRESSION;
        EncoderGzip comp(level);
        for (usz rdsz = rfile.read(tmp, sizeof(tmp)); rdsz > 0; rdsz = rfile.read(tmp, sizeof(tmp))) {
            comp.compress(output, tmp, rdsz);
            if (output.size()) {
                wfile.write(output.data(), output.size());
                output.clear();
            }
        }
        comp.finish(output);
        if (output.size()) {
            wfile.write(output.data(), output.size());
            output.clear();
        }
    } else { // unzip
        size_t limit = 2LL * 1024 * 1024 * 1024;
        DecoderGzip decomp(limit);

        for (usz rdsz = rfile.read(tmp, sizeof(tmp)); rdsz > 0; rdsz = rfile.read(tmp, sizeof(tmp))) {
            decomp.decompress(output, tmp, rdsz);
            if (output.size()) {
                wfile.write(output.data(), output.size());
                output.clear();
            }
        }
        decomp.finish(output);
        if (output.size()) {
            wfile.write(output.data(), output.size());
            output.clear();
        }
        limit = output.size();
        printf("file should be about 500 mb uncompressed, output size = %llu\n", limit / 1024 / 1024);
        rfile.close();
    }

    printf("out file size = %llu\n", wfile.getFileSize());
    return 0;
}
#endif // DUSE_ZLIB


} // namespace app
