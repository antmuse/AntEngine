#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "Connector.h"
#include "Linker.h"
#include "AppTicker.h"
#include "Timer.h"
#include "AsyncFile.h"

namespace app {
namespace net {


}//namespace net


s32 AppTestNetServer(s32 argc, s8** argv) {
    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);

    bool tls = App4Char2S32(argv[3]) == App4Char2S32("tls");

    net::NetServer* sev = new net::NetServer(tls);
    net::Acceptor* conn = new net::Acceptor(loop, net::NetServer::funcOnLink, sev);
    sev->drop();
    succ = conn->open(argv[2]);
    if (EE_OK != succ) {
        //delete sev & conn;
        conn->drop();
    }

    //std::chrono::milliseconds waitms(10);
    while (loop.run()) {
        //std::this_thread::sleep_for(waitms);
    }

    Logger::log(ELL_INFO, "AppTestNetServer>>stop");
    return 0;
}



s32 AppTestDefault(s32 argc, s8** argv) {
    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);

    //std::chrono::milliseconds waitms(10);
    while (loop.run()) {
        //std::this_thread::sleep_for(waitms);
    }

    Logger::log(ELL_INFO, "AppTestDefault>>stop");
    return 0;
}



s32 AppTestFile(s32 argc, s8** argv) {
    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);

    if (4 != argc) {
        Logger::log(ELL_INFO, "AppTestFile>>usage: exe filename flag");
        return 0;
    }
    AsyncFile* file = new AsyncFile();
    if (0 != file->open(argv[2], argv[3][0] - '0')) {
        Logger::log(ELL_INFO, "AppTestFile>>fail to open file=%s", argv[1]);
        delete file;
    }

    //std::chrono::milliseconds waitms(10);
    while (loop.run()) {
        //std::this_thread::sleep_for(waitms);
    }

    Logger::log(ELL_INFO, "AppTestDefault>>stop");
    return 0;
}

} //namespace app
