#include "LinkerUDP.h"
#include "Engine.h"
#include "AppTicker.h"
#include "Timer.h"

namespace app {
namespace net {

const u32 GMAX_UDP_SIZE = 1500;
u32 LinkerUDP::mSN = 0;

LinkerUDP::LinkerUDP() :
    mLoop(&Engine::getInstance().getLoop()),
    mPack(GMAX_UDP_SIZE) {
}


LinkerUDP::~LinkerUDP() {
}

s32 LinkerUDP::onLink(const NetServerUDP& sev, net::RequestUDP* sit) {
    mUDP.setClose(EHT_UDP, LinkerUDP::funcOnClose, this);
    mUDP.setTime(LinkerUDP::funcOnTime, 5 * 1000, 2 * 1000, -1);
    net::NetAddress& from = sit->mRemote;
    from.reverse();
    net::RequestUDP* it = net::RequestUDP::newRequest(GMAX_UDP_SIZE);
    it->mUser = this;
    it->mCall = LinkerUDP::funcOnRead;
    s32 ret = mUDP.open(it, &from,
        &sev.getHandleUDP().getLocal(), 3);
    if (EE_OK != ret) {
        net::RequestUDP::delRequest(it);
        return ret;
    }
    //sev.bind(this);
    return ret;
}


s32 LinkerUDP::onTimeout(HandleTime& it) {
    DASSERT(mUDP.getGrabCount() > 0);
    return sendMsgs(1);
}


void LinkerUDP::onClose(Handle* it) {
    DASSERT(&mUDP == it && "LinkerUDP::onClose handle");
    //sev.unbind(this);
    drop();
}


void LinkerUDP::onWrite(net::RequestUDP* it) {
    if (EE_OK == it->mError) {
        //++AppTicker::gTotalActive;
        ++AppTicker::gTotalPacketOut;
        AppTicker::gTotalSizeOut += it->mUsed;
    } else {
        Logger::log(ELL_ERROR, "LinkerUDP::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    net::RequestUDP::delRequest(it);
}


void LinkerUDP::onRead(net::RequestUDP* it) {
    if (EE_OK == it->mError) {
        StringView buf = it->getReadBuf();
        Logger::log(ELL_INFO, "LinkerUDP::onRead>>read=%.*s", buf.mLen, buf.mData);
    }
    it->mUsed = 0;
    if (EE_OK != mUDP.read(it)) {
        net::RequestUDP::delRequest(it);
        Logger::log(ELL_ERROR, "LinkerUDP::onRead>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
}


s32 LinkerUDP::sendMsgs(s32 max) {
    for (s32 i = 0; i < max; ++i) {
        net::RequestUDP* out = net::RequestUDP::newRequest(512);
        if (EE_OK != sendMsgs(out)) {
            Logger::log(ELL_ERROR, "LinkerUDP::sendMsgs>>ecode=%d", out->mError);
            net::RequestUDP::delRequest(out);
        }
    }
    return EE_OK;
}


s32 LinkerUDP::sendMsgs(net::RequestUDP* out) {
    DASSERT(out);
    StringView buf = out->getWriteBuf();
    out->mUsed = static_cast<u32>(Timer::getTimeStr(buf.mData, buf.mLen));
    out->mUsed += 1 + snprintf(buf.mData + out->mUsed, buf.mLen - out->mUsed,
        ", LinkerUDP msgID=%d, ip=[%s->%s]", ++mSN, mUDP.getLocal().getStr(), mUDP.getRemote().getStr());
    out->mCall = LinkerUDP::funcOnWrite;
    return mUDP.write(out);
}



//----------------------------------------------------------------------------------------------------
//NetServerUDP
//----------------------------------------------------------------------------------------------------

s32 NetServerUDP::open(const String& addr) {
    mUDP.setClose(EHT_UDP, NetServerUDP::funcOnClose, this);
    mUDP.setTime(NetServerUDP::funcOnTime, 25 * 1000, 25 * 1000, -1);

    net::RequestUDP* it = net::RequestUDP::newRequest(GMAX_UDP_SIZE + sizeof(net::NetAddress));
    net::NetAddress* from = reinterpret_cast<net::NetAddress*>(it->mData);
    new (from) net::NetAddress(addr);  // init it, we need to know ipv6 or ipv4
    it->mUsed = sizeof(net::NetAddress);
    it->mUser = this;
    it->mCall = NetServerUDP::funcOnRead;
    mUDP.setLocal(addr);
    s32 ret = mUDP.open(it, from, &mUDP.getLocal(), 1);
    if (EE_OK != ret) {
        net::RequestUDP::delRequest(it);
    }
    return ret;
}

void NetServerUDP::onRead(net::RequestUDP* it) {
    net::LinkerUDP* con = new net::LinkerUDP();
    if (EE_OK != con->onLink(*this, it)) {
        con->drop();
    }
    it->mUsed = 0;
    if (EE_OK != mUDP.readFrom(it)) {
        Logger::log(ELL_ERROR, "NetServerUDP::onRead>>size=%u, ecode=%d", it->mUsed, it->mError);
        net::RequestUDP::delRequest(it);
    }
}

s32 NetServerUDP::onTimeout(HandleTime& it) {
    DASSERT(mUDP.getGrabCount() > 0);
    return EE_OK;
}

void NetServerUDP::onClose(Handle* it) {
    DASSERT(&mUDP == it && "NetServerUDP::onClose handle");
    drop();
}

} //namespace net
} //namespace app
