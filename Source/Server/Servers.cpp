#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include "Servers.h"
#include "Engine.h"
#include "System.h"
#include "Net/HandleTCP.h"
#include "Net/Acceptor.h"
#include "Net/TcpProxy.h"
#include "ServerWeb.h"


#ifdef DOS_WINDOWS
#include <conio.h>
#endif

namespace app {

const s8* G_SERV_CFG = "Config/server.json";

Servers::Servers() {
    mTime.setClose(EHT_TIME, Servers::funcOnClose, this);
    mTime.setTime(Servers::funcOnTime, 2000, 1000, -1);
}

Servers::~Servers() {
}


s32 Servers::start() {
    Engine& eng = Engine::getInstance();
    if (!mConfig.load(eng.getAppPath(), G_SERV_CFG, eng.isMainProcess())) {
        Logger::log(ELL_INFO, "Servers::start>> can't load cfg: =%s", G_SERV_CFG);
        return EE_ERROR;
    }
    if (!eng.isMainProcess() || (0 == eng.getChildCount() && eng.isMainProcess())) {
        s32 err = eng.getLoop().postTask(&Servers::processTask, this, static_cast<void*>(nullptr));
        Logger::log(ELL_INFO, "Servers::start>> postTask %s, err = %d", EE_OK == err ? "success" : "fail", err);
    }
    s32 ret = EE_OK;
    if (eng.isMainProcess()) {
        ret = eng.getLoop().openHandle(&mTime);
        if (EE_OK == ret) {
            grab();
        }
    }
    return ret;
}


void Servers::processTask(void* it) {
    for (usz i = 0; i < mConfig.mProxy.size(); ++i) {
        net::TcpProxyHub* pxhub = new net::TcpProxyHub(mConfig.mProxy[i]);
        net::Acceptor* nd = new net::Acceptor(Engine::getInstance().getLoop(), net::TcpProxyHub::funcOnLink, pxhub);
        pxhub->drop();
        nd->setTimeout(mConfig.mProxy[i].mTimeout);
        nd->setBackend(mConfig.mProxy[i].mRemote);
        if (0 == nd->open(mConfig.mProxy[i].mLocal)) {
            Logger::log(ELL_INFO, "Engine::init>>start TcpProxy=[%s->%s]", mConfig.mProxy[i].mLocal.getStr(),
                mConfig.mProxy[i].mRemote.getStr());
        } else {
            Logger::log(ELL_ERROR, "Engine::init>>fail TcpProxy=[%s->%s]", mConfig.mProxy[i].mLocal.getStr(),
                mConfig.mProxy[i].mRemote.getStr());
            nd->drop();
        }
    }


    for (usz i = 0; i < mConfig.mWebsite.size(); ++i) {
        switch (mConfig.mWebsite[i].mType) {
        case 0: // http
        case 1: // https
        {
            net::ServerWeb* website = new net::ServerWeb(mConfig.mWebsite[i]);
            net::Acceptor* nd = new net::Acceptor(Engine::getInstance().getLoop(), net::Website::funcOnLink, website);
            website->drop();
            nd->getHandleTCP().setTimeGap(mConfig.mWebsite[i].mTimeout);
            nd->getHandleTCP().setTimeout(mConfig.mWebsite[i].mTimeout);
            if (0 == nd->open(mConfig.mWebsite[i].mLocal)) {
                Logger::log(ELL_INFO, "Engine::init>>start website=%s,path=%s", mConfig.mWebsite[i].mLocal.getStr(),
                    mConfig.mWebsite[i].mRootPath.c_str());
            } else {
                Logger::log(ELL_ERROR, "Engine::init>>fail website=%s,path=%s", mConfig.mWebsite[i].mLocal.getStr(),
                    mConfig.mWebsite[i].mRootPath.c_str());
                // delete website & nd;
                nd->drop();
            }
            break;
        }
        default:
            Logger::log(ELL_ERROR, "Engine::init>>invalid Proxy=%lld, type=%d", i, mConfig.mWebsite[i].mType);
            break;
        }
    }

    // mConfig.mWebsite.clear();
}


void Servers::onClose(Handle* it) {
    s32 grab = it->getGrabCount();
    DASSERT(0 == grab);
    Logger::log(ELL_INFO, "Servers::onClose>>=%p, grab=%d, pid=%d", it, grab, Engine::getInstance().getPID());
    drop();
}


s32 Servers::onTimeout(HandleTime* it) {
    EngineStats& estat = Engine::getInstance().getEngineStats();
    s8 ch = 0;
#ifdef DOS_WINDOWS
    // 32=blank,27=esc
    if (!_kbhit() || (ch = _getch()) != 27)
#endif
    {
        printf("Handle=%lld, Fly=%lld, In=%lld/%lld, Out=%lld/%lld, Active=%lld/%lld\n", estat.mTotalHandles.load(),
            estat.mFlyRequests.load(), estat.mInPackets.load(), estat.mInBytes.load(), estat.mOutPackets.load(),
            estat.mOutBytes.load(), estat.mHeartbeat.load(), estat.mHeartbeatResp.load());
        if (++mLogFlushCount >= 30) {
            mLogFlushCount = 0;
            Logger::flush();
        }
        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "Servers::onTimeout>>exiting...");
    return EE_ERROR;
}

} // namespace app
