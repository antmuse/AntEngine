#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "AppTicker.h"
#include "Timer.h"
#include "Converter.h"


#ifdef DOS_WINDOWS
#ifdef DDEBUG
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
// #pragma comment(linker, "/subsystem:console /entry:mainWCRTStartup")
#else
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
// #pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
// #pragma comment(linker, "/subsystem:windows /entry:mainWCRTStartup")
#endif // release version

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif



namespace app {
s32 AppTestDBClient(s32 argc, s8** argv);
void AppTestNetAddress();
void AppTestTree2heap();
void AppTestStrConv();
void AppTestDict();
void AppTestBase64();
void AppTestMD5(s32 argc, s8** argv);
void AppTestStr(s32 argc, s8** argv);
void AppTestVector();
void AppTestRBTreeMap();
s32 AppTestThreadPool(s32 argc, s8** argv);
s32 AppTestMemPool(s32 argc, s8** argv);
s32 AppTestStrConvGBKU8(s32 argc, s8** argv);
s32 AppTestSystem(s32 argc, s8** argv);
s32 AppTestRedis(s32 argc, s8** argv);
s32 AppTestDefault(s32 argc, s8** argv);
s32 AppTestHttpsClient(s32 argc, s8** argv);
s32 AppTestFile(s32 argc, s8** argv);
#if defined(DUSE_ZLIB)
s32 AppTestZlib(s32 argc, s8** argv);
s32 AppTestRingBlocks(s32 argc, s8** argv);
#endif
} // namespace app



using namespace app;



int main(int argc, char** argv) {
    printf("main>>start AntTest,argc=%d,argv[0]=%s\n", argc, argv[0]);
    if (argc < 2) {
        printf("main>>cmds should >=2, input=%d\n", argc);
        return 0;
    }
    printf("main>>cmd=%s\n", argv[1]);
    const s32 cmd = App10StrToS32(argv[1]);

    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0])) {
        printf("main>> engine init fail\n");
        return 0;
    }
    s32 ret = 0;
    switch (cmd) {
    case 0:
        // exe 0 0.0.0.0:9981 tls/tcp
        break;
    case 1:
        // exe 1 0.0.0.0:9981 tls/tcp 10 5000
        break;
    case 2:
        // exe 2
        ret = 2 == argc ? AppTestDefault(argc, argv) : argc;
        break;
    case 3:
        // exe 3
        ret = 2 == argc ? AppTestThreadPool(argc, argv) : argc;
        break;
    case 4:
        // exe 4
        ret = 2 == argc ? AppTestRedis(argc, argv) : argc;
        break;
    case 5:
        // exe 5
        ret = 2 == argc ? AppTestHttpsClient(argc, argv) : argc;
        break;
    case 6:
        // exe 6 af.html 1
        ret = 4 == argc ? AppTestFile(argc, argv) : argc;
        break;
    case 7:
        // exe 7 zip/unzip af.html out.gz
        ret = 5 == argc ? AppTestZlib(argc, argv) : argc;
        break;
    case 8:
        // exe 8 127.0.0.1:5000 user passowrd
        ret = 5 == argc ? AppTestDBClient(argc, argv) : argc;
        break;
    default:
        if (true) {
            AppTestMD5(argc, argv);
        } else {
            AppTestRingBlocks(argc, argv);
            AppTestMemPool(argc, argv);
            AppTestStr(argc, argv);
            AppTestStrConvGBKU8(argc, argv);
            AppTestThreadPool(argc, argv);
            AppTestNetAddress();
            AppTestVector();
            AppTestRBTreeMap();
            AppTestBase64();
            AppTestDict();
            AppTestTree2heap();
            AppTestStrConv();
            AppTestSystem(argc, argv);
            AppTestMemPool(argc, argv);
        }
        break;
    }

    Engine::getInstance().postCommand(ECT_EXIT);
    eng.run();
    DLOG(ELL_INFO, "Test=%s", "val");
    DLOG(ELL_INFO, "Test exit");
    Logger::log(ELL_INFO, "main>>exit...");
    Logger::flush();
    eng.uninit();
    printf("main>>stop\n");
    return 0;
}
