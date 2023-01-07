#include "SenderUDP.h"
#include "Engine.h"
#include "AppTicker.h"
#include "Timer.h"

namespace app {

const u32 GMAX_UDP_SIZE = 1500;
u32 SenderUDP::mSN = 0;

SenderUDP::SenderUDP() :
    mMaxMsg(1),
    mLoop(&Engine::getInstance().getLoop()),
    mPack(GMAX_UDP_SIZE) {
}


SenderUDP::~SenderUDP() {
}


void SenderUDP::setMaxMsg(s32 cnt) {
    mMaxMsg = cnt > 0 ? cnt : 1;
}


s32 SenderUDP::open(const String& addr) {
    mUDP.setClose(EHT_UDP, SenderUDP::funcOnClose, this);
    mUDP.setTime(SenderUDP::funcOnTime, 5 * 1000, 2 * 1000, -1);

    net::RequestTCP* it = net::RequestTCP::newRequest(GMAX_UDP_SIZE);
    it->mUser = this;
    it->mCall = funcOnRead;
    s32 ret = mUDP.open(it, addr.c_str(), nullptr, 2);
    if (EE_OK != ret) {
        net::RequestTCP::delRequest(it);
    }
    sendMsgs(mMaxMsg);
    return ret;
}


s32 SenderUDP::onTimeout(HandleTime& it) {
    DASSERT(mUDP.getGrabCount() > 0);
    return sendMsgs(1);
}


void SenderUDP::onClose(Handle* it) {
    DASSERT(&mUDP == it && "SenderUDP::onClose handle");
    //delete this;
}


void SenderUDP::onWrite(net::RequestTCP* it) {
    if (EE_OK == it->mError) {
        //++AppTicker::gTotalActive;
        ++AppTicker::gTotalPacketOut;
        AppTicker::gTotalSizeOut += it->mUsed;
    } else {
        Logger::log(ELL_ERROR, "SenderUDP::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    net::RequestTCP::delRequest(it);
}


void SenderUDP::onRead(net::RequestTCP* it) {
    if (EE_OK == it->mError) {
        StringView buf = it->getReadBuf();
        Logger::log(ELL_INFO, "SenderUDP::onRead>>read=%.*s", buf.mLen, buf.mData);
    }
    it->mUsed = 0;
    if (EE_OK != mUDP.read(it)) {
        net::RequestTCP::delRequest(it);
        Logger::log(ELL_ERROR, "SenderUDP::onRead>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
}


s32 SenderUDP::sendMsgs(s32 max) {
    for (s32 i = 0; i < max; ++i) {
        net::RequestTCP* out = net::RequestTCP::newRequest(512);
        if (EE_OK != sendMsgs(out)) {
            Logger::log(ELL_ERROR, "SenderUDP::sendMsgs>>ecode=%d", out->mError);
            net::RequestTCP::delRequest(out);
        }
    }
    return EE_OK;
}


s32 SenderUDP::sendMsgs(net::RequestTCP* out) {
    DASSERT(out);
    StringView buf = out->getWriteBuf();
    out->mUsed = static_cast<u32>(Timer::getTimeStr(buf.mData, buf.mLen));
    out->mUsed += 1 + snprintf(buf.mData + out->mUsed, buf.mLen - out->mUsed,
        ", msgID=%d, ip=[%s->%s]", ++mSN, mUDP.getLocal().getStr(), mUDP.getRemote().getStr());
    out->mCall = SenderUDP::funcOnWrite;
    return mUDP.write(out);
}

} //namespace app