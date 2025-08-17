#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include "ClientNAT.h"
#include "Engine.h"
#include "System.h"
#include "Net/HandleTCP.h"


#ifdef DOS_WINDOWS
#include <conio.h>
#endif

namespace app {
static s32 G_LOG_FLUSH_CNT = 0;

ClientNAT::ClientNAT() : mLoop(Engine::getInstance().getLoop()) {
}

ClientNAT::~ClientNAT() {
}

s32 ClientNAT::start(const s8* ipt) {
    if (!ipt || 0 == *ipt) {
        return EE_ERROR;
    }
    RequestUDP* req = RequestUDP::newRequest(1500);
    req->mUser = this;
    req->mCall = funcOnRead;
    mUDP.setClose(EHT_UDP, ClientNAT::funcOnClose, this);
    mUDP.setTime(ClientNAT::funcOnTime, 100, 1000, -1);
    s32 ret = mUDP.open(req, ipt, nullptr, 2);
    if (ret) {
        DLOG(ELL_ERROR, "ClientNAT start fail, remote=%s", ipt);
        RequestUDP::delRequest(req);
    }
    DLOG(ELL_INFO, "ClientNAT start, remote=%s, loc=%s", ipt, mUDP.getLocal().getStr());
    mStatus = net::ESN_INIT;
    return ret;
}


void ClientNAT::onClose(Handle* it) {
    s32 grab = it->getGrabCount();
    DASSERT(0 == grab);
    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "ClientNAT::onClose>>=%s, grab=%d, pid=%d", mUDP.getLocal().getStr(), grab,
        Engine::getInstance().getPID());
}


void ClientNAT::onRead(RequestUDP* it) {
    if (it->mUsed > 0) {
        DLOG(ELL_INFO, "on read: my-addres: %.*s", it->mUsed, it->getBuf());
    }
    it->mUsed = 0;
    s32 ret = mUDP.read(it);
    if (ret) {
        RequestUDP::delRequest(it);
        DLOG(ELL_ERROR, "on read fail, loc: %s", mUDP.getRemote().getStr());
    }
}

void ClientNAT::onWrite(RequestUDP* it) {
    // if (it->mUsed == 0) {
    //     DLOG(ELL_ERROR, "on write fail, loc: %s", mUDP.getRemote().getStr());
    // }
    DLOG(ELL_INFO, "status=%d, writed=%u, remote=%s", mStatus, it->mUsed, mUDP.getRemote().getStr());
    RequestUDP::delRequest(it);
}


s32 ClientNAT::onTimeout(HandleTime* it) {
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

        switch (mStatus) {
        case net::ESN_INIT:
        {
            RequestUDP* req = RequestUDP::newRequest(1500);
            req->mUser = this;
            req->mCall = funcOnWrite;
            req->mUsed = snprintf(req->getBuf(), req->getWriteSize(), "conneting-port:%s", mUDP.getLocal().getStr());
            s32 ret = mUDP.write(req); // connect as before
            if (EE_OK != ret) {
                DLOG(ELL_ERROR, "fail req-connect, remote=%s", mUDP.getRemote().getStr());
                RequestUDP::delRequest(req);
            }
            break;
        }
        default:
            break;
        };

        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "ClientNAT::onTimeout>>exiting...");
    return EE_ERROR;
}

} // namespace app
