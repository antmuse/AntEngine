#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "ClientNAT.h"
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

const char* const G_CFG = R"(
{
    "Daemon": false,
    "Print": 1, //0=disable print, 1=main process print, 2=all process print
    "LogPath": "Log/",
    "PidFile": "Log/cnat_pid.txt",
    "ShareMem": "GMEM/ClientNAT.map", //共享内存名
    "ShareMemSize": 1, //[1-10 * 1024], 1MB
    "AcceptPost": 10, //[1-255]监听端口上侯命的请求数
    "ThreadPool": 3, //[1-255]
    "Process": 0,     //进程数
}
)";

using namespace app;

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: ./ServerNat.bin 0.0.0.0:55555\n");
        return 0;
    }
    printf("main>>start ClientNAT, argc=%d, argv[0]=%s\n", argc, argv[0]);
    // App10StrToS32(argv[1]);
    const char* cmd = argv[1];
    printf("main>>iport = %s\n", cmd);
    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0], false, G_CFG)) {
        printf("main>> engine init fail\n");
        return 1;
    }
    /*std::chrono::milliseconds waitms(1000);
    while (1) {
        std::this_thread::sleep_for(waitms);
    }*/
    ClientNAT tick;
    if (tick.start(cmd)) {
        DLOG(ELL_INFO, "main>>start fail on: %s", cmd);
    } else {
        DLOG(ELL_INFO, "main>>start success on: %s", cmd);
        eng.run();
    }
    DLOG(ELL_INFO, "main>>pid=%d, main=%s, exit...", eng.getPID(), eng.isMainProcess() ? "Yes" : "No");
    eng.uninit();
    printf("main>>stop\n");
    return 0;
}
