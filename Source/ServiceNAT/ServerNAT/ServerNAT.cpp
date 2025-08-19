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

ServerNAT::ServerNAT() : mLoop(Engine::getInstance().getLoop()), mSN(0), mUserSN(0) {
}

ServerNAT::~ServerNAT() {
}

s32 ServerNAT::start(const s8* ipt) {
    if (!ipt || 0 == *ipt) {
        return EE_ERROR;
    }
    RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
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
    RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
    req->mUser = this;
    req->mCall = funcOnRead;
    req->mRemote.setAddrSize(it->mRemote.getAddrSize());
    // it->mUsed = 0;
    s32 ret = mUDP.readFrom(req);
    if (ret) {
        RequestUDP::delRequest(req);
        RequestUDP::delRequest(it);
        DLOG(ELL_ERROR, "on read fail, loc: %s", mUDP.getLocal().getStr());
        return;
    }

    if (it->mUsed < sizeof(MsgHeader)) {
        DLOG(ELL_ERROR, "on read mini msg, %u", it->mUsed);
        RequestUDP::delRequest(it);
        return;
    }
    MsgHeader& hed = *(MsgHeader*)(it->getBuf());
    switch (hed.mType) {
    case net::EPT_ACTIVE:
    {
        net::PackActive& msg = (net::PackActive&)(hed);
        msg.packResp(++mSN);
        break;
    }
    case net::EPT_REGIST:
    {
        net::PackRegist& msg0 = (net::PackRegist&)(hed);
        u64 uid = msg0.mUserID;
        TMap<u64, String>::Node* nd = mBinds.find(uid);
        net::PackRegistResp& msg = (net::PackRegistResp&)(hed);
        msg.clear();
        it->mRemote.reverse();
        if (!nd) {
            mBinds[uid] = it->mRemote.getStr();
            DLOG(ELL_INFO, "regist, uid=%llu, remote: %s", uid, it->mRemote.getStr());
        } else {
            nd->getValue() = it->mRemote.getStr();
            DLOG(ELL_INFO, "regist_update, uid=%llu, remote: %s", uid, it->mRemote.getStr());
        }
        msg.mUserID = uid;
        msg.writeItem(msg.EI_ADDR, it->mRemote.getStr(), strlen(it->mRemote.getStr()) + 1);
        msg.writeHeader(++mSN);
        break;
    }
    case net::EPT_FIND:
    {
        net::PackFind& msg0 = (net::PackFind&)(hed);
        net::PackFindResp& msg = (net::PackFindResp&)(hed);
        TMap<u64, String>::Node* nd = mBinds.find(msg0.mUserID);
        msg.clear();
        if (nd) {
            msg.mUserID = nd->getKey();
            msg.writeItem(msg.EI_PEER_ADDR, nd->getValue().data(), nd->getValue().size() + 1);
        } else {
            msg.mUserID = 0;
        }
        msg.writeHeader(++mSN);
        break;
    }
    default:
    {
        RequestUDP::delRequest(it);
        DLOG(ELL_ERROR, "invalid cmd, remote: %s", mUDP.getRemote().getStr());
        return;
    }
    } // switch

    it->mUser = this;
    it->mCall = funcOnWrite;
    it->mUsed = hed.mSize;
    DASSERT(hed.mSize <= net::EPT_MAX_SIZE);
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
