#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "Net/HandleTCP.h"
#include "AppTicker.h"
#include "Timer.h"
#include "HttpsClient.h"

namespace app {

s32 AppTestHttpsClient(s32 argc, s8** argv) {
    const s8* serveraddr = argc > 2 ? argv[2] : "202.108.22.5:443"; //https://www.baidu.com
    s32 maxth = argc > 3 ? atoi(argv[3]) : 1;
    if (maxth > 100) {
        maxth = 100;
    }
    Logger::log(ELL_INFO, "AppTestHttpsClient>>connect server=%s,tcp=%d", serveraddr, maxth);

    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);

    HttpsClient* conn = new HttpsClient[maxth];
    s32 post = 0;
    while (loop.run()) {
        if (post < maxth) {
            succ = conn[post].open(serveraddr);
            DASSERT(0 == succ);
            ++post;
        }
    }

    delete[] conn;

    Logger::log(ELL_INFO, "AppTestHttpsClient>>stop");
    return 0;
}

} //namespace app