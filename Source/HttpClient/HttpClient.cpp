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

int main(int argc, char** argv) {
    const char* url = "https://www.baidu.com";
    if (argc > 1) {
        url = argv[1];
    }
    printf("main>>argc=%d, url=%s\n", argc, url);
    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0], false, "{}")) {
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
