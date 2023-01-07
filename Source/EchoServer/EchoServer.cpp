#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Timer.h"
#include "Engine.h"
#include "AppTicker.h"
#include "Converter.h"
#include "LinkerTCP.h"
#include "LinkerUDP.h"


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
#pragma comment(lib, "Advapi32.lib")
#endif



using namespace app;

/**
 * usage:
 *    ./exe tcp 127.0.0.1:5000
 *    ./exe udp 127.0.0.1:5000
 */
int main(int argc, char** argv) {
    printf("main>>argc=%d,argv[0]=%s\n", argc, argv[0]);
    if (argc < 3) {
        return 0;
    }
    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0], false)) {
        printf("EchoServer>> engine init fail\n");
        return 1;
    }

    bool tcp = 0 == AppStrNocaseCMP("tcp", argv[1], sizeof("udp") - 1);
    bool tls = tcp && 'T' == argv[1][0];
    const char* serv_addr = argc > 2 ? argv[2] : "127.0.0.1:9981";

    Logger::log(ELL_INFO, "EchoServer>>remote server=%s,type=%s",
        serv_addr, tcp ? (tls ? "tcp_tls" : "tcp") : "udp");

    AppTicker tick;
    s32 succ = tick.start();

    if (tcp) {
        net::NetServerTCP* sev = new net::NetServerTCP(tls);
        net::Acceptor* conn = new net::Acceptor(eng.getLoop(), net::NetServerTCP::funcOnLink, sev);
        sev->drop();
        if (EE_OK != conn->open(serv_addr)) {
            Logger::log(ELL_INFO, "EchoServer>>tcp open fail, listen = %s", serv_addr);
            conn->drop();
        }
        eng.run();
    } else {
        net::NetServerUDP* sev = new net::NetServerUDP(tls);
        if (EE_OK != sev->open(serv_addr)) {
            Logger::log(ELL_INFO, "EchoServer>>udp open fail, listen = %s", serv_addr);
            sev->drop();
        }
        eng.run();
    }

    Logger::log(ELL_INFO, "EchoServer>>exit...");
    eng.uninit();
    printf("EchoServer>>stop ok\n");
    return 0;
}
