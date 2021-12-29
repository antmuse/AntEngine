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
    printf("main>>argc=%d,argv[0]=%s\n", argc, argv[0]);

    Engine& eng = Engine::getInstance();
    
    if (!eng.init(argv[0], false)) {
        printf("main>> engine init fail\n");
        return 1;
    }

    AppTicker tick;
    s32 succ = tick.start();

    eng.run();

    Logger::log(ELL_INFO, "main>>exit...");
    eng.uninit();
    printf("main>>stop\n");
    return 0;
}
