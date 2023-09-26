#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "System.h"
#include "Net/HandleTCP.h"
#include "AppTicker.h"

#ifdef DOS_WINDOWS
#include <conio.h>
#endif

namespace app {

static s32 G_LOG_FLUSH_CNT = 0;

usz AppTicker::gTotalPacketIn = 0;
usz AppTicker::gTotalSizeIn = 0;
usz AppTicker::gTotalPacketOut = 0;
usz AppTicker::gTotalSizeOut = 0;
usz AppTicker::gTotalActive = 0;
usz AppTicker::gTotalActiveResp = 0;


AppTicker::AppTicker(Loop& loop) :
    mLoop(loop) {
    mTime.setClose(EHT_TIME, AppTicker::funcOnClose, this);
    mTime.setTime(AppTicker::funcOnTime, 2000, 1000, -1);
}

AppTicker::~AppTicker() {
}

s32 AppTicker::start() {
    return mLoop.openHandle(&mTime);
}

void AppTicker::onClose(Handle* it) {
    s32 grab = it->getGrabCount();
    DASSERT(0 == grab);
    Logger::log(ELL_INFO, "AppTicker::onClose>>=%p, grab=%d", it, grab);
}


s32 AppTicker::onTimeout(HandleTime* it) {
    s32 ch = 0;
#ifdef DOS_WINDOWS
    //32=blank,27=esc
    if (!_kbhit() || (ch = _getch()) != 27)
#endif
    {
        if (32 == ch) {
            ch = 0;
            Logger::log(ELL_INFO, "AppTicker::onTimeout>>%p, timeout=%lld, gap=%lld",
                it, it->getTimeout(), it->getTimeGap());
            Engine::getInstance().postCommand(ECT_ACTIVE);
        } else {
            printf("Handle=%d, Fly=%d, In=%llu/%llu, Out=%llu/%llu, Active=%llu/%llu\n",
                mLoop.getHandleCount(), mLoop.getFlyRequest(),
                gTotalPacketIn, gTotalSizeIn, gTotalPacketOut, gTotalSizeOut, gTotalActive, gTotalActiveResp);
        }
        if (++G_LOG_FLUSH_CNT >= 20) {
            G_LOG_FLUSH_CNT = 0;
            Logger::flush();
        }
        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT);    //mLoop.stop();
    Logger::log(ELL_INFO, "AppTicker::onTimeout>>exiting...");
    return EE_ERROR;
}

}//namespace app
