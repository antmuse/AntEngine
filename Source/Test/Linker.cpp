#include "Linker.h"
#include "Net/Acceptor.h"
#include "AppTicker.h"

namespace app {
namespace net {

Linker::Linker() :
    mTLS(true),
    mPack(1024) {
}


Linker::~Linker() {
}

s32 Linker::onTimeout(HandleTime& it) {
    return EE_ERROR;
}


void Linker::onClose(Handle* it) {
    printf("Linker::onClose>>closed handle = %p\n", it);
    if (mTLS) {
        DASSERT(&mTCP == it && "Linker::onClose tls handle?");
    } else {
        DASSERT(&mTCP.getHandleTCP() == it && "Linker::onClose tcp handle?");
    }
    delete this;
}


void Linker::onLink(net::RequestTCP* it) {
    net::Acceptor* accp = (net::Acceptor*)(it->mUser);
    mTLS = nullptr != accp->getUser();
    if (mTLS) {
        mTCP.setClose(EHT_TCP_LINK, Linker::funcOnClose, this);
        mTCP.setTime(Linker::funcOnTime, 15 * 1000, 30 * 1000, -1);
    } else {
        mTCP.getHandleTCP().setClose(EHT_TCP_LINK, Linker::funcOnClose, this);
        mTCP.getHandleTCP().setTime(Linker::funcOnTime, 15 * 1000, 30 * 1000, -1);
    }

    net::RequestTCP* read = net::RequestTCP::newRequest(4 * 1024);
    read->mUser = this;
    read->mCall = Linker::funcOnRead;
    s32 ret = mTLS ? mTCP.open(*(net::RequestAccept*)it, read)
        : mTCP.getHandleTCP().open(*(net::RequestAccept*)it, read);
    if (0 == ret) {
        Logger::log(ELL_INFO, "Linker::onLink>> [%s->%s]", mTCP.getRemote().getStr(), mTCP.getLocal().getStr());
    } else {
        net::RequestTCP::delRequest(read);
        Logger::log(ELL_INFO, "Linker::onLink>> [%s->%s], ecode=%d", mTCP.getRemote().getStr(), mTCP.getLocal().getStr(), ret);
        delete this;
    }
}


void Linker::onWrite(net::RequestTCP* it) {
    if (0 == it->mError) {
        StringView dat = it->getReadBuf();
        MsgHeader* head = (MsgHeader*)dat.mData;
        switch (head->mType) {
        case net::EPT_ACTIVE_RESP:
            break;
        case net::EPT_SUBMIT:
            AppTicker::gTotalSizeOut += it->mUsed;
            ++AppTicker::gTotalPacketOut;
            break;
        }
    } else {
        Logger::log(ELL_ERROR, "Linker::onWrite>>size=%u, ecode=%d", it->mUsed, it->mError);
    }
    net::RequestTCP::delRequest(it);
}


void Linker::onRead(net::RequestTCP* it) {
    if (it->mUsed > 0) {
        StringView dat = it->getReadBuf();
        usz got = mPack.write(dat.mData, dat.mLen);
        it->mUsed = 0;
        if (EE_OK != readIF(it)) {
            net::RequestTCP::delRequest(it);
        }

        MsgHeader* head = (MsgHeader*)mPack.getPointer();
        while (got >= sizeof(MsgHeader::mSize) && head->mSize <= got) {
            switch (head->mType) {
            case net::EPT_ACTIVE:
            {
                ++AppTicker::gTotalActive;
                net::RequestTCP* out = net::RequestTCP::newRequest(head->mSize);
                out->mUser = this;
                out->mCall = Linker::funcOnWrite;
                dat = out->getWriteBuf();
                head->mType = net::EPT_ACTIVE_RESP;
                memcpy(dat.mData, head, head->mSize);
                out->mUsed = head->mSize;
                if (0 != writeIF(out)) {
                    net::RequestTCP::delRequest(out);
                }
                break;
            }
            case net::EPT_SUBMIT:
            {
                ++AppTicker::gTotalPacketIn;
                AppTicker::gTotalSizeIn += head->mSize;
                net::RequestTCP* out = net::RequestTCP::newRequest(head->mSize);
                out->mUser = this;
                out->mCall = Linker::funcOnWrite;
                dat = out->getWriteBuf();
                memcpy(dat.mData, head, head->mSize);
                out->mUsed = head->mSize;
                if (0 != writeIF(out)) {
                    net::RequestTCP::delRequest(out);
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
        //printf("Linker::onRead>>%.*s\n", (s32)dat.mLen, dat.mData);
        return;
    }

    //stop read
    printf("Linker::onRead>>read 0 bytes, ecode=%d\n", it->mError);
    net::RequestTCP::delRequest(it);
}

} //namespace net
} //namespace app
