#include "Connector.h"
#include "Engine.h"
#include "AppTicker.h"

namespace app {

u32 Connector::mSN = 0;

Connector::Connector() :
    mMaxMsg(1),
    mTLS(true),
    mLoop(&Engine::getInstance().getLoop()),
    mPack(1024) {
}


Connector::~Connector() {
}


void Connector::setMaxMsg(s32 cnt) {
    mMaxMsg = cnt > 0 ? cnt : 1;
}


s32 Connector::open(const String& addr, bool tls) {
    mTLS = tls;
    if (mTLS) {
        mTCP.setClose(EHT_TCP_CONNECT, Connector::funcOnClose, this);
        mTCP.setTime(Connector::funcOnTime, 15 * 1000, 20 * 1000, -1);
    } else {
        mTCP.getHandleTCP().setClose(EHT_TCP_CONNECT, Connector::funcOnClose, this);
        mTCP.getHandleTCP().setTime(Connector::funcOnTime, 15 * 1000, 20 * 1000, -1);
    }

    net::RequestTCP* it = net::RequestTCP::newRequest(4 * 1024);
    it->mUser = this;
    it->mCall = funcOnConnect;
    s32 ret = mTLS ? mTCP.open(addr, it) : mTCP.getHandleTCP().open(addr, it);
    if (EE_OK != ret) {
        net::RequestTCP::delRequest(it);
    }
    return ret;
}


s32 Connector::onTimeout(HandleTime& it) {
    DASSERT(mTCP.getGrabCount() > 0);

    net::RequestTCP* tick = net::RequestTCP::newRequest(sizeof(net::PackActive));
    tick->mUser = this;
    tick->mCall = Connector::funcOnWrite;
    net::PackActive* head = (net::PackActive*)(tick->getWriteBuf().mData);
    head->pack(++mSN);
    tick->mUsed = head->mSize;
    if (0 != writeIF(tick)) {
        net::RequestTCP::delRequest(tick);
        //@note: 此时,不管是否调用close handle，皆必须返回EE_ERROR
        return EE_ERROR;
    }

    return EE_OK;
}


void Connector::onClose(Handle* it) {
    if (mTLS) {
        DASSERT(&mTCP == it && "Connector::onClose tls handle?");
    } else {
        DASSERT(&mTCP.getHandleTCP() == it && "Connector::onClose tcp handle?");
    }
    //delete this;
}


void Connector::onWrite(net::RequestTCP* it) {
    if (0 == it->mError) {
        StringView dat = it->getReadBuf();
        MsgHeader* head = (MsgHeader*)dat.mData;
        switch (head->mType) {
        case net::EPT_ACTIVE:
            ++AppTicker::gTotalActive;
            break;
        case net::EPT_ACTIVE_RESP:
            break;
        case net::EPT_SUBMIT:
            AppTicker::gTotalSizeOut += it->mUsed;
            ++AppTicker::gTotalPacketOut;
            break;
        }
        //printf("Connector::onWrite>>success = %u\n", it->mUsed);
    } else {
        Logger::log(ELL_ERROR, "Connector::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    net::RequestTCP::delRequest(it);
    //mTCP.getSock().closeSend();
    //mTCP.getSock().closeReceive();
}


void Connector::onRead(net::RequestTCP* it) {
    if (it->mUsed > 0) {
        StringView dat = it->getReadBuf();
        usz got = mPack.write(dat.mData, dat.mLen);
        AppTicker::gTotalSizeIn += dat.mLen;
        it->mUsed = 0;
        if (EE_OK != readIF(it)) {
            net::RequestTCP::delRequest(it);
            return;
        }

        MsgHeader* head = (MsgHeader*)mPack.getPointer();
        while (got >= sizeof(MsgHeader::mSize) && head->mSize <= got) {
            switch (head->mType) {
            case net::EPT_ACTIVE:
                break;
            case net::EPT_ACTIVE_RESP:
                ++AppTicker::gTotalActiveResp;
                break;
            case net::EPT_SUBMIT:
                ++AppTicker::gTotalPacketIn;
                if (mMaxMsg > 0) {
                    --mMaxMsg;
                    net::RequestTCP* out = net::RequestTCP::newRequest(head->mSize);
                    out->mUser = this;
                    out->mCall = Connector::funcOnWrite;
                    dat = out->getWriteBuf();
                    memcpy(dat.mData, head, head->mSize);
                    ((MsgHeader*)dat.mData)->mSN = ++mSN;
                    u32 sz;
                    s8* val = head->readItem(net::PackSubmit::EI_VALUE, sz);
                    out->mUsed = head->mSize;
                    if (0 != writeIF(out)) {
                        net::RequestTCP::delRequest(out);
                        return;
                    }
                }
                break;
            case net::EPT_SUBMIT_RESP:
                break;
            }//switch
            got -= head->mSize;
            head = (MsgHeader*)((s8*)head + head->mSize);
        }//while
        mPack.clear(mPack.size() - got);
        //printf("Connector::onRead>>%.*s\n", (s32)dat.mLen, dat.mData);
        return;
    } else {
        //stop read
        Logger::log(ELL_INFO, "Connector::onRead>>read 0, ecode=%d", it->mError);
        net::RequestTCP::delRequest(it);
    }
}


void Connector::onConnect(net::RequestTCP* it) {
    if (0 == it->mError) {
        //printf("Connector::onConnect>>success\n");
        it->mCall = Connector::funcOnRead;
        if (0 != readIF(it)) {
            net::RequestTCP::delRequest(it);
            return;
        }

        /*for (s32 i = 0; i < 2; ++i) {
            net::RequestTCP* read = net::RequestTCP::newRequest(4*1024);
            read->mUser = this;
            read->mCall = Connector::funcOnRead;
            if (0 != readIF(read)) {
                net::RequestTCP::delRequest(read);
                return;
            }
        }*/

        for (s32 i = 0; mMaxMsg > 0 && i < 2; ++i) {
            net::RequestTCP* out = net::RequestTCP::newRequest(1024);
            StringView buf = out->getWriteBuf();
            net::PackSubmit& head = *(net::PackSubmit*)buf.mData;
            head.clear();
            net::PackSubmit::Item& val = head.getNode(head.EI_VALUE);
            val.mOffset = head.mSize;
            val.mSize = 1 + snprintf(buf.mData + head.mSize, buf.mLen - head.mSize,
                "%s", "GET / HTTP/1.1\r\nHost: 10.1.63.128\r\nConnection : keep-alive\r\n\r\n");
            head.mSize += val.mSize;
            head.mSN = ++mSN;
            head.mSize = 1024; //just for test

            out->mUsed = head.mSize;
            out->mCall = Connector::funcOnWrite;
            if (0 == writeIF(out)) {
                --mMaxMsg;
            } else {
                net::RequestTCP::delRequest(out);
                break;
            }
        }
    } else {
        Logger::log(ELL_ERROR, "Connector::onConnect>>ecode=%d", it->mError);
        net::RequestTCP::delRequest(it);
    }
}


} //namespace app
