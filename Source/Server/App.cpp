#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "AppTicker.h"
#include "Timer.h"
#include "Converter.h"


#ifdef   DOS_WINDOWS
#ifdef   DDEBUG
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:mainWCRTStartup")
#else
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainWCRTStartup")
#endif  //release version

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif


using namespace app;

int main(int argc, char** argv) {
    printf("main>>start Server,argc=%d,argv[0]=%s\n", argc, argv[0]);
    const char* cmd = argc < 2 ? "" : argv[1];
    if (*cmd) {
        //App10StrToS32(argv[1]);
        printf("main>>cmd=%s\n", cmd);
    }

    Engine& eng = Engine::getInstance();

    switch (*cmd) {
    case '-':
    {
        /*std::chrono::milliseconds waitms(1000);
        while (1) {
            std::this_thread::sleep_for(waitms);
        }*/
        if (!eng.init(argv[0], true)) {
            printf("main>> engine fork fail\n");
            return 1;
        }
        break;
    }
    default:
        if (!eng.init(argv[0], false)) {
            printf("main>> engine init fail\n");
            return 1;
        }
        break;
    }
    AppTicker tick;
    tick.start();
    eng.run();
    DLOG(ELL_INFO, "main>>pid=%d, main=%s, exit...", eng.getPID(), eng.isMainProcess() ? "Yes" : "No");
    eng.uninit();
    printf("main>>stop\n");

    return 0;
}
