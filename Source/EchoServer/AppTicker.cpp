#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include "Engine.h"
#include "System.h"
#include "AppTicker.h"
#include "Net/HandleTCP.h"


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


AppTicker::AppTicker()
    :mLoop(Engine::getInstance().getLoop()) {
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
    Logger::log(ELL_INFO, "AppTicker::onClose>>=%p, grab=%d, pid=%d", it, grab, Engine::getInstance().getPID());
}

static void tmpfunc(void* it) {
    printf("tmpfunc = %lld\n", (ssz)it);
}

static void poolfunc(void* it) {
    if (EE_OK != Engine::getInstance().getLoop().postTask(&tmpfunc, it)) {
        printf("poolfunc idx=%lld", (ssz)it);
    }
}

s32 AppTicker::onTimeout(HandleTime* it) {
    static ssz icntLast = 0;   //in count
    static ssz ocntLast = 0;   //out count
    static ssz ipackLast = 0;   //in packets count
    static ssz opackLast = 0;   //out packets count
    static ssz tickLast = 0;    //heatbeat count
    static ssz tickRespLast = 0;   //heatbeat count

    ssz icnt = gTotalSizeIn - icntLast;
    icntLast = gTotalSizeIn;
    ssz ocnt = gTotalSizeOut - ocntLast;
    ocntLast = gTotalSizeOut;
    ssz ipack = gTotalPacketIn - ipackLast;
    ipackLast = gTotalPacketIn;
    ssz opack = gTotalPacketOut - opackLast;
    opackLast = gTotalPacketOut;
    ssz tick = gTotalActive - tickLast;
    tickLast = gTotalActive;
    ssz tickResp = gTotalActiveResp - tickRespLast;
    tickRespLast = gTotalActiveResp;

    EngineStats& estat = Engine::getInstance().getEngineStats();
    estat.mInBytes.fetch_add(icnt);
    estat.mOutBytes.fetch_add(ocnt);
    estat.mInPackets.fetch_add(ipack);
    estat.mOutPackets.fetch_add(opack);
    estat.mHeartbeat.fetch_add(tick);
    estat.mHeartbeatResp.fetch_add(tickResp);

    if (!Engine::getInstance().isMainProcess()) {
        return EE_OK;
    }

#ifdef DOS_WINDOWS
    s32 ch = 0; //32=blank,27=esc
    if (!_kbhit() || (ch = _getch()) != 27)
#endif
    {
        printf("Handle=%lld, Fly=%lld, In=%lld/%lld, Out=%lld/%lld, Active=%lld/%lld\n", estat.mTotalHandles.load(),
            estat.mFlyRequests.load(), estat.mInPackets.load(), estat.mInBytes.load(), estat.mOutPackets.load(),
            estat.mOutBytes.load(), estat.mHeartbeat.load(), estat.mHeartbeatResp.load());
        if (++G_LOG_FLUSH_CNT >= 20) {
            G_LOG_FLUSH_CNT = 0;
            Logger::flush();
        }

        //for (ssz i = 0; i < 100; ++i) {
        //    //mLoop.postTask(tmpfunc, (void*)i);
        //    Engine::getInstance().getThreadPool().postTask(poolfunc, (void*)i);
        //}
        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "AppTicker::onTimeout>>exiting...");
    return EE_ERROR;
}

}//namespace app
