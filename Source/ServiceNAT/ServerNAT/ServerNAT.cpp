#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include "ServerNAT.h"
#include "Engine.h"
#include "System.h"
#include "Net/HandleTCP.h"


#ifdef DOS_WINDOWS
#include <conio.h>
#endif

namespace app {
static s32 G_LOG_FLUSH_CNT = 0;

ServerNAT::ServerNAT() : mLoop(Engine::getInstance().getLoop()) {
}

ServerNAT::~ServerNAT() {
}

s32 ServerNAT::start(const s8* ipt) {
    if (!ipt || 0 == *ipt) {
        return EE_ERROR;
    }
    RequestUDP* req = RequestUDP::newRequest(1500);
    req->mUser = this;
    req->mCall = funcOnRead;
    mUDP.setClose(EHT_UDP, ServerNAT::funcOnClose, this);
    mUDP.setTime(ServerNAT::funcOnTime, 2000, 1000, -1);
    s32 ret = mUDP.open(req, nullptr, ipt, 1);
    if (ret) {
        DLOG(ELL_INFO, "ServerNAT start fail, addr=%s", ipt);
        RequestUDP::delRequest(req);
    }
    return ret;
}


void ServerNAT::onClose(Handle* it) {
    s32 grab = it->getGrabCount();
    DASSERT(0 == grab);
    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "ServerNAT::onClose>>=%p, grab=%d, pid=%d", it, grab, Engine::getInstance().getPID());
}


void ServerNAT::onRead(RequestUDP* it) {
    RequestUDP* req = RequestUDP::newRequest(1500);
    req->mUser = this;
    req->mCall = funcOnRead;
    req->mRemote.setAddrSize(it->mRemote.getAddrSize());
    //it->mUsed = 0;
    s32 ret = mUDP.readFrom(req);
    if (ret) {
        RequestUDP::delRequest(req);
        RequestUDP::delRequest(it);
        DLOG(ELL_ERROR, "on read fail, loc: %s", mUDP.getLocal().getStr());
        return;
    }

    it->mCall = funcOnWrite;
    it->mRemote.reverse();
    it->mUsed = snprintf(it->getBuf(), it->mAllocated, "%s", it->mRemote.getStr());
    ret = mUDP.writeTo(it);
    if (ret) {
        RequestUDP::delRequest(it);
        DLOG(ELL_ERROR, "writeTo fail, remote: %s", mUDP.getRemote().getStr());
    }
}

void ServerNAT::onWrite(RequestUDP* it) {
    if (it->mUsed == 0) {
        DLOG(ELL_ERROR, "on write fail, loc: %s", mUDP.getRemote().getStr());
    }
    RequestUDP::delRequest(it);
}


s32 ServerNAT::onTimeout(HandleTime* it) {
    EngineStats& estat = Engine::getInstance().getEngineStats();

    if (!Engine::getInstance().isMainProcess()) {
    }

    s8 ch = 0;
#ifdef DOS_WINDOWS
    // 32=blank,27=esc
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
        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "ServerNAT::onTimeout>>exiting...");
    return EE_ERROR;
}

} // namespace app
