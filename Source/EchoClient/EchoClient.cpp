#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Timer.h"
#include "Engine.h"
#include "AppTicker.h"
#include "Converter.h"
#include "SenderUDP.h"
#include "SenderUDP2.h"
#include "SenderTCP.h"


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
#pragma comment(lib, "Advapi32.lib")
#endif



using namespace app;

/**
 * usage:
 *    ./exe tcp 127.0.0.1:5000 1 30
 *    ./exe TCP 127.0.0.1:5000 1 30
 *    ./exe udp 127.0.0.1:5000 1 30
 *    ./exe UDP 127.0.0.1:5000 1 30
 */
int main(int argc, char** argv) {
    printf("main>>argc=%d,argv[0]=%s\n", argc, argv[0]);
    if (argc < 5) {
        printf("* usage:\n");
        printf("*   ./exe tcp 127.0.0.1:5000 1 30 \t//1 raw tcp client, it send 30 packets\n");
        printf("*   ./exe TCP 127.0.0.1:5000 10 30 \t//10 TLS-tcp client, each send 30 packets\n");
        printf("*   ./exe udp 127.0.0.1:5000 100 30 \t//100 udp client, each send 30 packets\n");
        printf("*   ./exe UDP 127.0.0.1:5000 100 30 \t//100 udp client, each send 30 packets\n");
        return 0;
    }
    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0], false)) {
        printf("EchoClient>> engine init fail\n");
        return 1;
    }

    bool tcp = 0 == AppStrNocaseCMP("tcp", argv[1], sizeof("udp") - 1);
    bool tls = tcp && 'T' == argv[1][0];
    bool udpPro = !tcp && 'U' == argv[1][0];
    const char* serv_addr = argc > 2 ? argv[2] : "127.0.0.1:9981";

    s32 max_sock = atoi(argv[3]);
    if (max_sock > 10000) {
        max_sock = 10000;
    }
    const s32 maxmsg = atoi(argv[4]);
    Logger::log(ELL_INFO, "EchoClient>>remote server=%s,type=%s,sock=%d,msg=%d", serv_addr,
        tcp ? (tls ? "tcp_tls" : "tcp") : (udpPro ? "udp_pro" : "udp"), max_sock, maxmsg);

    AppTicker tick;
    s32 succ = tick.start();

    if (tcp) {
        SenderTCP* conn = new SenderTCP[max_sock];
        s32 post = 0;
        while (eng.step()) {
            if (post < max_sock) {
                conn[post].setMaxMsg(maxmsg);
                succ = conn[post].open(serv_addr, tls);
                DASSERT(0 == succ);
                ++post;
            }
        }
        delete[] conn;
    } else {
        if (udpPro) {
            SenderUDP2* conn = new SenderUDP2[max_sock];
            s32 post = 0;
            while (eng.step()) {
                if (post < max_sock) {
                    conn[post].setMaxMsg(maxmsg);
                    succ = conn[post].open(serv_addr);
                    DASSERT(0 == succ);
                    ++post;
                }
            }
            delete[] conn;
        } else {
            SenderUDP* conn = new SenderUDP[max_sock];
            s32 post = 0;
            while (eng.step()) {
                if (post < max_sock) {
                    conn[post].setMaxMsg(maxmsg);
                    succ = conn[post].open(serv_addr);
                    DASSERT(0 == succ);
                    ++post;
                }
            }
            delete[] conn;
        }
    }

    Logger::log(ELL_INFO, "EchoClient>>exit...");
    eng.uninit();
    printf("EchoClient>>stop ok\n");
    return 0;
}
