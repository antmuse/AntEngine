#include "LinkerTCP.h"
#include "Net/Acceptor.h"
#include "AppTicker.h"

namespace app {
namespace net {

LinkerTCP::LinkerTCP() :
    mTLS(false),
    mServer(nullptr),
    mPack(1024) {
}


LinkerTCP::~LinkerTCP() {
}

s32 LinkerTCP::onTimeout(HandleTime& it) {
    printf("LinkerTCP::onTimeout>>closed handle's flag = 0x%X\n", it.getFlag());
    return EE_ERROR;
}


void LinkerTCP::onClose(Handle* it) {
    printf("LinkerTCP::onClose>>closed handle = %p\n", it);
    if (mTLS) {
        DASSERT(&mTCP == it && "LinkerTCP::onClose tls handle?");
    } else {
        DASSERT(&mTCP.getHandleTCP() == it && "LinkerTCP::onClose tcp handle?");
    }
    if (mServer) {
        NetServerTCP* sev = mServer;
        mServer = nullptr;
        sev->unbind(this);
    } else {
        drop();
    }
}


bool LinkerTCP::onLink(NetServerTCP* sev, RequestFD* it) {
    DASSERT(sev);
    mServer = sev;
    mTLS = sev->isTLS();

    if (mTLS) {
        mTCP.setClose(EHT_TCP_LINK, LinkerTCP::funcOnClose, this);
        mTCP.setTime(LinkerTCP::funcOnTime, 15 * 1000, 30 * 1000, -1);
    } else {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, LinkerTCP::funcOnClose, this);
        mTCP.getHandleTCP().setTime(LinkerTCP::funcOnTime, 15 * 1000, 30 * 1000, -1);
    }

    RequestFD* read = RequestFD::newRequest(4 * 1024);
    read->mUser = this;
    read->mCall = LinkerTCP::funcOnRead;
    s32 ret = mTLS ? mTCP.open(*(RequestAccept*)it, read)
        : mTCP.getHandleTCP().open(*(RequestAccept*)it, read);
    if (0 == ret) {
        mServer->bind(this);
        Logger::log(ELL_INFO, "LinkerTCP::onLink>> [%s->%s]", mTCP.getRemote().getStr(), mTCP.getLocal().getStr());
    } else {
        mServer = nullptr;
        RequestFD::delRequest(read);
        Logger::log(ELL_INFO, "LinkerTCP::onLink>> [%s->%s], ecode=%d", mTCP.getRemote().getStr(), mTCP.getLocal().getStr(), ret);
    }
    return 0 == ret;
}


void LinkerTCP::onWrite(RequestFD* it) {
    if (0 == it->mError) {
        StringView dat = it->getReadBuf();
        MsgHeader* head = (MsgHeader*)dat.mData;
        switch (head->mType) {
        case net::EPT_ACTIVE_RESP:
            ++AppTicker::gTotalActiveResp;
            break;
        case net::EPT_SUBMIT:
            AppTicker::gTotalSizeOut += it->mUsed;
            ++AppTicker::gTotalPacketOut;
            break;
        }
    } else {
        Logger::log(ELL_ERROR, "LinkerTCP::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    RequestFD::delRequest(it);
}


void LinkerTCP::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        StringView dat = it->getReadBuf();
        usz got = mPack.write(dat.mData, dat.mLen);
        it->mUsed = 0;
        if (EE_OK != readIF(it)) {
            RequestFD::delRequest(it);
        }

        MsgHeader* head = (MsgHeader*)mPack.getPointer();
        while (got >= sizeof(MsgHeader::mSize) && head->mSize <= got) {
            switch (head->mType) {
            case net::EPT_ACTIVE:
            {
                ++AppTicker::gTotalActive;
                RequestFD* out = RequestFD::newRequest(head->mSize);
                out->mUser = this;
                out->mCall = LinkerTCP::funcOnWrite;
                dat = out->getWriteBuf();
                head->mType = net::EPT_ACTIVE_RESP;
                memcpy(dat.mData, head, head->mSize);
                out->mUsed = head->mSize;
                if (0 != writeIF(out)) {
                    RequestFD::delRequest(out);
                }
                break;
            }
            case net::EPT_SUBMIT:
            {
                ++AppTicker::gTotalPacketIn;
                AppTicker::gTotalSizeIn += head->mSize;
                RequestFD* out = RequestFD::newRequest(head->mSize);
                out->mUser = this;
                out->mCall = LinkerTCP::funcOnWrite;
                dat = out->getWriteBuf();
                memcpy(dat.mData, head, head->mSize);
                out->mUsed = head->mSize;
                if (0 != writeIF(out)) {
                    RequestFD::delRequest(out);
                }
                break;
            }
            case net::EPT_ACTIVE_RESP:
            case net::EPT_SUBMIT_RESP:
            default:
                break;
            }//switch
            got -= head->mSize;
            head = (MsgHeader*)((s8*)head + head->mSize);
        }//while
        mPack.clear(mPack.size() - got);
        //printf("LinkerTCP::onRead>>%.*s\n", (s32)dat.mLen, dat.mData);
        return;
    }

    //stop read
    printf("LinkerTCP::onRead>>read 0 bytes, ecode=%d\n", it->mError);
    RequestFD::delRequest(it);
}

} //namespace net
} //namespace app
