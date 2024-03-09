#include "Config.h"
#include <string.h>
#include <stdio.h>
#include <memory>
#include "StrConverter.h"
#include "System.h"
#include "Engine.h"

namespace app {

void AppStrNormalizeGBK(s8* text, u32 options);
void AppStrNormalizeUTF8(s8* text, u32 options);

s32 AppTestStrConvGBKU8(s32 argc, s8** argv) {
    const s8* utf8 = u8"我是utf-8字符！";
    const s8* gbk = "我是GBK字符！";
    usz utf8_len = strlen(utf8);
    usz gbk_len = strlen(utf8);
    usz utf8buffer_len = utf8_len * 3 + 1;
    usz gbkbuffer_len = gbk_len * 2 + 1;
    s8* utf8buffer = (s8*)malloc(utf8buffer_len);
    s8* gbkbuffer = (s8*)malloc(gbkbuffer_len);
    memset(utf8buffer, 0, utf8buffer_len);
    memset(gbkbuffer, 0, gbkbuffer_len);

    gbkbuffer_len = AppUTF8ToGBK(utf8, gbkbuffer, gbkbuffer_len);
    utf8buffer_len = AppGbkToUTF8(gbk, utf8buffer, utf8buffer_len);
    printf("utf8: %s,%llu \t gbkbuffer: %s,%llu\n", utf8, utf8_len, gbkbuffer, gbkbuffer_len);
    printf("gbk: %s,%llu \t utf8buffer: %s,%llu\n", gbk, gbk_len, utf8buffer, utf8buffer_len);

    memcpy(gbkbuffer, "國圓", sizeof("國圓"));
    printf("gbk: %s\n", gbkbuffer);
    AppStrNormalizeGBK(gbkbuffer, 8);
    printf("gbk: %s\n", gbkbuffer);
    AppStrNormalizeGBK(gbkbuffer, 0);
    printf("gbk: %s\n", gbkbuffer);

    free(utf8buffer);
    free(gbkbuffer);
    return 0;
}


s32 AppTestSystem(s32 argc, s8** argv) {
    TVector<FileInfo> nds;
    s32 eee = System::isExist(Engine::getInstance().getAppPath());
    eee = System::isExist(Engine::getInstance().getAppPath() + "Config/config.json");
    System::getPathNodes(Engine::getInstance().getAppPath(), 0, nds);
    return 0;
}


} //namespace app
