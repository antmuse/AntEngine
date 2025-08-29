#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include "ClientNAT.h"
#include "Engine.h"
#include "System.h"
#include "Converter.h"
#include "Net/HandleTCP.h"


#ifdef DOS_WINDOWS
#include <conio.h>
#endif

namespace app {
static s32 G_LOG_FLUSH_CNT = 0;

ClientNAT::ClientNAT() :
    mLoop(Engine::getInstance().getLoop()), mStatus(net::ESN_REGIST), mSN(0), mUserID(1), mFindCount(0), mMaxFind(20) {
}

ClientNAT::~ClientNAT() {
}

s32 ClientNAT::start(const s8* ipt, const s8* userid, const s8* password, const s8* peerid) {
    if (!ipt || 0 == *ipt) {
        return EE_ERROR;
    }
    mPassword = password ? password : "";
    mUserID = userid ? App10StrToU32(userid) : 0;
    mPeerID = peerid ? App10StrToU32(peerid) : 0;
    mServerAddr.setIPort(ipt);
    RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
    req->mUser = this;
    req->mCall = funcOnRead;
    mUDP.setClose(EHT_UDP, ClientNAT::funcOnClose, this);
    mUDP.setTime(ClientNAT::funcOnTime, 100, 1000, -1);
    s32 ret = mUDP.open(req, ipt, "0.0.0.0:20008", 0);
    if (ret) {
        DLOG(ELL_ERROR, "ClientNAT start fail, remote=%s", ipt);
        RequestUDP::delRequest(req);
    }
    DLOG(ELL_INFO, "ClientNAT start, remote=%s, loc=%s", ipt, mUDP.getLocal().getStr());
    mStatus = net::ESN_REGIST;
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
        processMsg(mStatus, it->getBuf());
        // DLOG(ELL_INFO, "on read: my-addres: %.*s", it->mUsed, it->getBuf());
    }
    it->mUsed = 0;
    s32 ret = mUDP.read(it);
    if (ret) {
        // mStatus = net::ESN_REGIST;
        RequestUDP::delRequest(it);
        DLOG(ELL_ERROR, "on read fail, loc: %s", mUDP.getRemote().getStr());
    }
}

void ClientNAT::onWrite(RequestUDP* it) {
    // DLOG(ELL_INFO, "status=%d, writed=%u, remote=%s", mStatus, it->mUsed, mUDP.getRemote().getStr());
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
        processMsg(mStatus, nullptr);
        return EE_OK;
    }

    Engine::getInstance().postCommand(ECT_EXIT);
    Logger::log(ELL_INFO, "ClientNAT::onTimeout>>exiting...");
    return EE_ERROR;
}



void ClientNAT::processMsg(s32 status, void* resp) {
    MsgHeader& hed = *(MsgHeader*)(resp);
    auto funcsend = [&](RequestUDP* it) {
        it->mUser = this;
        it->mCall = funcOnWrite;
        s32 ret = mUDP.writeTo(it);
        if (EE_OK != ret) {
            DLOG(ELL_ERROR, "fail req, remote=%s", it->mRemote.getStr());
            RequestUDP::delRequest(it);
        }
    };
    switch (status) {
    case net::ESN_REGIST:
    {
        if (resp && net::EPT_REGIST_RESP == hed.mType) {
            net::PackRegistResp& msg = (net::PackRegistResp&)(hed);
            u32 len;
            s8* addr = msg.readItem(msg.EI_ADDR, len);
            if (addr && msg.mUserID > 0) {
                mStatus = net::ESN_FIND;
                DLOG(ELL_INFO, "reg ok, act my addr = %s", addr);
                /*for (s32 i = 0; i < 3; ++i) {
                    RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
                    net::PackActive& msg = *(net::PackActive*)(req->getBuf());
                    msg.pack(++mSN);
                    req->mUsed = msg.mSize;
                    req->mRemote.setIPort(addr);
                    DASSERT(msg.mSize < net::EPT_MAX_SIZE);
                    funcsend(req);
                }*/
            } else {
                DLOG(ELL_INFO, "reg fail");
            }
        } else {
            RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
            net::PackRegist& msg = *(net::PackRegist*)(req->getBuf());
            msg.clear();
            msg.mUserID = mUserID; // accid
            msg.writeItem(msg.EI_LOC_ADDR, mUDP.getLocal().getStr(), strlen(mUDP.getLocal().getStr()) + 1);
            req->mUsed = msg.mSize;
            req->mRemote = mServerAddr;
            DASSERT(msg.mSize < net::EPT_MAX_SIZE);
            funcsend(req);
            DLOG(ELL_INFO, "reg... uid=%llu", mUserID);
        }
        break;
    }
    case net::ESN_FIND:
    {
        if (resp && net::EPT_FIND_RESP == hed.mType) {
            net::PackFindResp& msg = (net::PackFindResp&)(hed);
            u32 len;
            s8* addr = msg.readItem(msg.EI_PEER_ADDR, len);
            if (addr && msg.mUserID == mPeerID) {
                mStatus = net::ESN_CONNECT;
                mPeerAddr.setIPort(addr);
                DLOG(ELL_INFO, "find ok, peer = %d, peer_addr=%s", msg.mUserID, addr);
            } else {
                DLOG(ELL_INFO, "find fail");
            }
            mFindCount = 0;
        } else {
            if (++mFindCount > mMaxFind) {
                DLOG(ELL_INFO, "finding timeout");
                mFindCount = 0;
                mStatus = net::ESN_REGIST;
            } else {
                RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
                net::PackFind& msg = *(net::PackFind*)(req->getBuf());
                msg.clear();
                msg.mUserID = mPeerID; // peer accid
                DASSERT(msg.mSize < net::EPT_MAX_SIZE);
                req->mRemote = mServerAddr;
                req->mUsed = msg.mSize;
                funcsend(req);
                DLOG(ELL_INFO, "finding peerID=%llu ...", mPeerID);
            }
        }
        break;
    }
    case net::ESN_CONNECT:
    {
        if (resp && net::EPT_ACTIVE == hed.mType) {
            mFindCount = 0;
            DLOG(ELL_INFO, "active ok, peer");
        } else {
            if (++mFindCount > mMaxFind) {
                DLOG(ELL_INFO, "active timeout");
                mFindCount = 0;
                mStatus = net::ESN_REGIST;
            } else {
                RequestUDP* req = RequestUDP::newRequest(net::EPT_MAX_SIZE);
                net::PackActive& msg = *(net::PackActive*)(req->getBuf());
                msg.pack(++mSN);
                req->mRemote = mPeerAddr;
                req->mUsed = msg.mSize;
                DLOG(ELL_INFO, "active peer = %s", mPeerAddr.getStr());
                funcsend(req);
            }
        }
        break;
    }
    default:
        break;
    };
}


} // namespace app
