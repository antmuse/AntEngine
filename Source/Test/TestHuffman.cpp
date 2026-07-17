#include "Logger.h"
#include "Huffman.h"

namespace app {


s32 AppTestHuffman(s32 argc, s8** argv) {
    s8 srcbuf[128];
    s8 enbuf[128];
    s8 rebuf[128];
    const s8* rawss = "HTTP2/AppTestHuffman";
    if (argc > 2) {
        rawss = argv[2];
    }
    usz len = snprintf(srcbuf, sizeof(srcbuf), "%s", rawss);
    usz elen = AppHuffEncode((u8*)srcbuf, len, (u8*)enbuf, true);
    DLOG(ELL_INFO, "src[%lu]= %s,  huff-len= %lu", len, srcbuf, elen);
    u8 stat = 0;
    u8* dst = (u8*)rebuf;
    bool ret = AppHuffDecode((u8*)enbuf, elen, &dst, &stat, true);
    usz len2 = dst - (u8*)rebuf;
    DLOG(ELL_INFO, "ret = %s, stat = x%x, resrc[%lu]= %.*s", ret ? "success" : "fail", stat, len2, len2, rebuf);
    return 0;
}


} // namespace app
