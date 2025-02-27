#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include "Engine.h"
#include "System.h"
#include "FileWriter.h"
#include "AppTicker.h"
#include "Net/HandleTCP.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/HttpEvtFileSave.h"


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

class GetFile : public net::HttpMsgReceiver {
public:
    GetFile() {
    }
    virtual ~GetFile() {
    }
    virtual s32 stepMsg(net::HttpMsg* msg) {
        s32 ret = EE_OK;
        switch (msg->getStationID()) {
        case net::ES_INIT:
            break;
        case net::ES_PATH:
            break;
        case net::ES_HEAD:
            break;
        case net::ES_BODY:
            break;
        case net::ES_BODY_DONE:
            break;
        case net::ES_RESP_HEAD:
            break;
        case net::ES_RESP_BODY:
            break;
        case net::ES_RESP_BODY_DONE:
            break;
        case net::ES_ERROR:
            break;
        case net::ES_CLOSE:
            break;
        default:
            break;
        }
        return ret;
    }

private:
    FileWriter mFile;
};

AppTicker::AppTicker() : mLoop(Engine::getInstance().getLoop()) {
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
    // 32=blank,27=esc
    if (!_kbhit() || (ch = _getch()) != 27)
#endif
    {
        if (32 == ch) {
            ch = 0;
            Logger::log(
                ELL_INFO, "AppTicker::onTimeout>>%p, timeout=%lld, gap=%lld", it, it->getTimeout(), it->getTimeGap());
            Engine::getInstance().postCommand(ECT_ACTIVE);

            s8 url[256];
            std::cin.getline(url, sizeof(url), '\n');
            if (0 == url[0]) {
                snprintf(url, sizeof(url), "%s", "http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
            }
            GetFile* evt = new GetFile();
            net::HttpLayer* nd = new net::HttpLayer(evt, net::EHTTP_RESPONSE);
            evt->drop();
            net::HttpMsg* msg = new net::HttpMsg(nd);
            msg->getHeadOut().setKeepAlive(true);
            msg->getHeadOut().add("Accept", "*/*");
            msg->setMethod(net::HTTP_GET);
            msg->setURL(url);
            s32 fly = nd->post(msg);
            if (EE_OK != fly) {
                printf("url = %s, ip=%s\n", url, nd->getHandle().getRemote().getStr());
            }
            nd->drop();
            msg->drop();
        } else {
            printf("Handle=%d, Fly=%d, In=%llu/%llu, Out=%llu/%llu, Active=%llu/%llu\n", mLoop.getHandleCount(),
                mLoop.getFlyRequest(), gTotalPacketIn, gTotalSizeIn, gTotalPacketOut, gTotalSizeOut, gTotalActive,
                gTotalActiveResp);
        }
        if (++G_LOG_FLUSH_CNT >= 20) {
            G_LOG_FLUSH_CNT = 0;
            Logger::flush();
        }
        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT); // mLoop.stop();
    Logger::log(ELL_INFO, "AppTicker::onTimeout>>exiting...");
    return EE_ERROR;
}

} // namespace app
