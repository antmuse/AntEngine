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


using namespace app;
const s8* G_CFG_CONTENT = R"({
    "Daemon": false,
    "Print": 1, //0=disable print, 1=main process print, 2=all process print
    "LogPath": "Log/",
    "PidFile": "Log/pid.txt",
    "ShareMem": "GMEM/MainMem.map",
    "ShareMemSize": 1,
    "AcceptPost": 10,
    "ThreadPool": 3,
    "Process": 0,
    "TLS": {
        "Ciphers": "HIGH:!aNULL:!MD5", //for TLSv1.2
        "Ciphersuites": "", //for TLSv1.3
        "CA": "Config/eng_ca.crt",
        "Cert": "Config/eng.crt",
        "Key": "Config/eng.key",
        "VersionOff": "v1.0, v1.1",
        "VerifyDepth": 5,
        "HttpALPN": 1,
        "PreferServerCiphers": false,
        "Debug": true,
        "Verify": 0
    }
})";

int main(int argc, char** argv) {
    const char* url = "https://www.baidu.com";
    if (argc > 1) {
        url = argv[1];
    }
    printf("main>>argc=%d, url=%s\n", argc, url);
    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0], false, G_CFG_CONTENT)) {
        printf("main>> engine init fail\n");
        return 1;
    }
    eng.getLoop().postTask(AppTicker::http_task, (void*)url);
    eng.run();
    Logger::log(ELL_INFO, "main>>exit...");
    eng.uninit();
    printf("main>>stop\n");
    return 0;
}
