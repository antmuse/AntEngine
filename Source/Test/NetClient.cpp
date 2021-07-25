#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "Net/HandleTCP.h"
#include "Connector.h"
#include "AppTicker.h"
#include "Timer.h"

namespace app {

s32 AppTestNetClient(s32 argc, s8** argv) {
    const char* serveraddr = argc > 2 ? argv[2] : "127.0.0.1:9981";
    bool tls = App4Char2S32(argv[3]) == App4Char2S32("tls");

    s32 maxth = atoi(argv[4]);
    if (maxth > 10000) {
        maxth = 10000;
    }
    const s32 maxmsg = atoi(argv[5]);
    Logger::log(ELL_INFO, "AppTestNetClient>>connect server=%s,tcp=%d,msg=%d", serveraddr, maxth, maxmsg);

    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);

    //std::chrono::milliseconds waitms(100);
    Connector* conn = new Connector[maxth];
    s32 post = 0;
    while (loop.run()) {
        //std::this_thread::sleep_for(waitms);
        if (post < maxth) {
            conn[post].setMaxMsg(maxmsg);
            succ = conn[post].open(serveraddr, tls);
            DASSERT(0 == succ);
            ++post;
        }
    }

    delete[] conn;

    Logger::log(ELL_INFO, "AppTestNetClient>>stop");
    return 0;
}

} //namespace app