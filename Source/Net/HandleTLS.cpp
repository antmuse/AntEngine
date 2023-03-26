/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#include "Net/HandleTLS.h"
#include "Net/Acceptor.h"
#include "Net/TlsSession.h"
#include "Engine.h"


namespace app {
namespace net {

HandleTLS::HandleTLS() :
    mTlsSession(nullptr),
    mFlyWrites(nullptr),
    mFlyReads(nullptr),
    mLandWrites(nullptr),
    mLandReads(nullptr) {
    mLoop = &Engine::getInstance().getLoop();
    mHostName[0] = 0;
    mHostName[sizeof(mHostName) - 1] = 0;
}


HandleTLS::~HandleTLS() {
    uninit();
}


void HandleTLS::uninit() {
    if (mTlsSession) {
        TlsSession* session = (TlsSession*)(mTlsSession);
        delete session;
        mTlsSession = nullptr;
        mInBuffers.uninit();
        mOutBuffers.uninit();
    }
}


void HandleTLS::init(const TlsContext& tlsCTX) {
    if (!mTlsSession) {
        mInBuffers.init();
        mOutBuffers.init();
        SSL_CTX* ssl_ctx = reinterpret_cast<SSL_CTX*>(tlsCTX.getTlsContext());
        DASSERT(ssl_ctx);
        mTlsSession = new TlsSession(ssl_ctx, &mInBuffers, &mOutBuffers);
    }
    mRead.mUser = nullptr; //null表示未在使用中，否则为占用中
    mWrite.mUser = nullptr; //null表示未在使用中，否则为占用中
    mHostName[0] = 0;
    mCommitPos = mOutBuffers.getHead();
}


s32 HandleTLS::handshake() {
    TlsSession* session = (TlsSession*)mTlsSession;
#ifdef DDEBUG
    assert(!session->isInitFinished() && "Handshake shouldn't be finished");
#endif
    s32 rc = session->handshake();
    if (rc <= 0) {
        s32 err = session->getError(rc);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_NONE) {
            TlsSession::showError();
            //it->mError = EE_ERROR;
            return EE_ERROR;
        }
    }

    mWrite.mCall = HandleTLS::funcOnWriteHello;
    return postWrite();
}


s32 HandleTLS::postRead() {
    if (mRead.mUser) {
        return EE_OK;
    }
    mRead.mAllocated = mInBuffers.peekTailNode(&mRead.mData, D_RBUF_BLOCK_SIZE);
    mRead.mUsed = 0;
    if (mRead.mAllocated > 0) {
        mRead.mUser = this;
        if (EE_OK == mTCP.read(&mRead)) {
            return EE_OK;
        }
    } else {
        Logger::log(ELL_ERROR, "HandleTLS::postRead>>allocated=0");
    }
    mRead.mError = EE_ERROR;
    return EE_ERROR;
}


s32 HandleTLS::postWrite() {
    if (mWrite.mUser) {
        return EE_OK;
    }

    if (mOutBuffers.getSize() > 0) {
        s32 bufcnt = 1;
        StringView buf;
        mCommitPos = mOutBuffers.peekHeadNode(mOutBuffers.getHead(), &buf, &bufcnt);
        //mWrite.mCall = HandleTLS::funcOnWrite;
        mWrite.mData = buf.mData;
        mWrite.mAllocated = (u32)buf.mLen;
        mWrite.mUsed = mWrite.mAllocated;
        DASSERT(mWrite.mUsed > 0);
        mWrite.mUser = this;
        mWrite.mError = mTCP.write(&mWrite);
        return mWrite.mError;
    }

    return EE_OK;
}


s32 HandleTLS::verify(s32 vfg) {
    TlsSession* session = (TlsSession*)mTlsSession;
    if (!session) {
        return EE_ERROR;
    }
    //return session->verify(Engine::getInstance().getTlsContext().getVerifyFlags(), host);
    return session->verify(vfg, mHostName);
}


void HandleTLS::landQueue(RequestFD*& que) {
    for (RequestFD* nd = AppPopRingQueueHead_1(que);
        nd; nd = AppPopRingQueueHead_1(que)) {
        DASSERT(nd->mCall);
        nd->mCall(nd);
    }
}


void HandleTLS::doRead() {
    TlsSession* session = (TlsSession*)mTlsSession;
    bool gogo = true;
    
    do {
        for (RequestFD* it = AppPopRingQueueHead_1(mFlyReads);
            it && gogo; it = AppPopRingQueueHead_1(mFlyReads)) {
            s32 nread = session->read(it->mData, (s32)it->mAllocated);
            if (nread > 0) {
                it->mUsed = nread;
                AppPushRingQueueTail_1(mLandReads, it);
                //it->mCall(it);
            } else {
                gogo = false;
                s32 error = session->getError(nread);
                if (error == SSL_ERROR_WANT_READ) {
                    //Wait for next read
                    AppPushRingQueueHead_1(mFlyReads, it);
                } else if (SSL_ERROR_ZERO_RETURN == error) {
                    //read 0
                    close();
                    it->mUsed = 0;
                    AppPushRingQueueTail_1(mLandReads, it);
                    //it->mCall(it);
                } else {
                    close();
                    it->mError = error;
                    it->mUsed = 0;
                    AppPushRingQueueTail_1(mLandReads, it);
                    //it->mCall(it);
                }
                break;
            }
        }//for
        landReads();
    } while (gogo && mFlyReads);

    DASSERT(nullptr == mLandReads);
}


s32 HandleTLS::setHost(const s8* host, usz length) {
    if (!host || length >= sizeof(mHostName)) {
        return EE_ERROR;
    }
    memcpy(mHostName, host, length);
    mHostName[length] = '\0';

    ((TlsSession*)mTlsSession)->setHost(mHostName);
    return 0;
}


s32 HandleTLS::open(const NetAddress& addr, RequestFD* oit, const net::TlsContext* tlsctx) {
    init(tlsctx ? *tlsctx : Engine::getInstance().getTlsContext());

    mType = EHT_TCP_CONNECT;
    mTCP.setClose(EHT_TCP_CONNECT, HandleTLS::funcOnClose, this);

    DASSERT(oit && oit->mCall);
    oit->mType = ERT_CONNECT;
    oit->mError = 0;
    oit->mHandle = this;

    mWrite.mUser = this;
    mWrite.mCall = HandleTLS::funcOnConnect;
    s32 ret = mTCP.open(addr, &mWrite);
    mFlag = mTCP.getFlag();
    if (EE_OK == ret) {
        grab();
        AppPushRingQueueTail_1(mFlyWrites, oit);
    }
    return ret;
}


s32 HandleTLS::open(const String& addr, RequestFD* oit, const net::TlsContext* tlsctx) {
    init(tlsctx ? *tlsctx : Engine::getInstance().getTlsContext());

    mType = EHT_TCP_CONNECT;
    mTCP.setClose(EHT_TCP_CONNECT, HandleTLS::funcOnClose, this);

    DASSERT(oit && oit->mCall);
    oit->mType = ERT_CONNECT;
    oit->mError = 0;
    oit->mHandle = this;

    mWrite.mUser = this;
    mWrite.mCall = HandleTLS::funcOnConnect;
    s32 ret = mTCP.open(addr, &mWrite);
    mFlag = mTCP.getFlag();
    if (EE_OK == ret) {
        grab();
        AppPushRingQueueTail_1(mFlyWrites, oit);
    }
    return ret;
}


s32 HandleTLS::open(const RequestAccept& accp, RequestFD* oit, const net::TlsContext* tlsctx) {
    init(tlsctx ? *tlsctx : Engine::getInstance().getTlsContext());
    ((TlsSession*)mTlsSession)->setAcceptState();

    mType = EHT_TCP_LINK;
    mTCP.setClose(EHT_TCP_LINK, HandleTLS::funcOnClose, this);

    if (oit) {
        DASSERT(oit->mCall);
        oit->mType = ERT_READ;
        oit->mError = 0;
        oit->mHandle = this;
        mRead.mUser = this;
        mRead.mAllocated = mInBuffers.peekTailNode(&mRead.mData, D_RBUF_BLOCK_SIZE);
        mRead.mUsed = 0;
        DASSERT(mRead.mAllocated > 0);
    }
    mRead.mCall = HandleTLS::funcOnReadHello;

    s32 ret = mTCP.open(accp, oit ? &mRead : nullptr);
    mFlag = mTCP.getFlag();
    if (EE_OK == ret) {
        grab();
        if (oit) {
            AppPushRingQueueTail_1(mFlyReads, oit);
        }
    }
    return ret;
}


s32 HandleTLS::write(RequestFD* req) {
    if (0 == (EHF_WRITEABLE & mFlag)) {
        return EE_NO_WRITEABLE;
    }
    DASSERT(req && req->mCall);
    req->mError = 0;
    req->mType = ERT_WRITE;
    req->mHandle = this;

    if (mFlyWrites) {
        AppPushRingQueueTail_1(mFlyWrites, req);
        postWrite();
        return EE_OK;
    }

    TlsSession* session = (TlsSession*)mTlsSession;
    s32 wsz = session->write(req->mData, (s32)req->mUsed);
    if (wsz > 0) {
        if ((u32)wsz >= req->mUsed) {
            //done
            AppPushRingQueueTail_1(mLandWrites, req);
            postWrite();
            return EE_OK;
        } else {
            req->setStepSize(wsz);
        }
    }
    AppPushRingQueueHead_1(mFlyWrites, req);
    postWrite();
    return EE_OK;
}


s32 HandleTLS::read(RequestFD* req) {
    if (0 == (EHF_READABLE & mFlag)) {
        return EE_NO_READABLE;
    }
    DASSERT(req && req->mCall);
    req->mError = 0;
    req->mType = ERT_READ;
    req->mHandle = this;
    if (mFlyReads) {
        AppPushRingQueueTail_1(mFlyReads, req);
        postRead();
        return EE_OK;
    }
    StringView buf = req->getWriteBuf();
    TlsSession* session = (TlsSession*)mTlsSession;
    s32 wsz = session->read(buf.mData, (s32)buf.mLen);
    if (wsz > 0) {
        req->mUsed = wsz;
        AppPushRingQueueTail_1(mLandReads, req);
        //done
    } else {
        AppPushRingQueueTail_1(mFlyReads, req);
    }
    postRead();
    return EE_OK;
}


s32 HandleTLS::onTimeout(HandleTime& it) {
    DASSERT(mTCP.getGrabCount() > 0);
    return mCallTime(this);
}


void HandleTLS::onClose(Handle* it) {
    DASSERT(&mTCP == it && "HandleTLS::onClose what handle?");
    mFlag = mTCP.getFlag();
    landReads();
    landWrites();
    landQueue(mFlyWrites);
    landQueue(mFlyReads);
    uninit();
    drop();
    mCallClose(this);
}


void HandleTLS::onWriteHello(RequestFD* it) {
    if (0 == it->mError) {
        mWrite.mUser = nullptr;
        mOutBuffers.commitHeadPos(mCommitPos);
        handshake();
        return;
    }

    mFlag = mTCP.getFlag();
    Logger::log(ELL_ERROR, "HandleTLS::onWriteHello>>size=%u, ecode=%d", it->mUsed, it->mError);
}


void HandleTLS::onWrite(RequestFD* it) {
    landWrites();
    if (0 == it->mError) {
        mWrite.mUser = nullptr;
        mOutBuffers.commitHeadPos(mCommitPos);
        postWrite();
        return;
    }
    mFlag = mTCP.getFlag();
    Logger::log(ELL_ERROR, "HandleTLS::onWrite>>flag=0x%X, size=%u/%u, ecode=%d",
        mFlag, it->mUsed, it->mAllocated, it->mError);
}


void HandleTLS::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        mInBuffers.commitTailPos((s32)it->mUsed);
        doRead();
        mRead.mUser = nullptr;
        postRead();
        return;
    }
    mInBuffers.setRet(0);
    mFlag = mTCP.getFlag();
    doRead();
    Logger::log(ELL_ERROR, "HandleTLS::onRead>>read 0/%u, flag=0x%X, ecode=%d",
        it->mAllocated, mFlag, it->mError);
}


void HandleTLS::onReadHello(RequestFD* it) {
    if (it->mUsed > 0) {
        mRead.mUser = nullptr;
        mInBuffers.commitTailPos((s32)it->mUsed);
        s32 ret = handshake();
        if (0 == ret) {
            TlsSession* session = (TlsSession*)mTlsSession;
            if (session->isInitFinished()) {
                it->mCall = HandleTLS::funcOnRead;
                mWrite.mCall = HandleTLS::funcOnWrite;

                if (EHT_TCP_CONNECT == mType) {
                    RequestFD* oit = AppPopRingQueueHead_1(mFlyWrites);
                    DASSERT(oit);
                    if (oit) {
                        //回调中自行决定是否要验证域名证书， HandleTLS::verify(1, "www.baidu.com");
                        oit->mError = 0;
                        oit->mCall(oit);
                    }
                } else {
                    //link type
                    doRead();
                }
            }
        } else {
            close();
        }

        if (EE_OK == postRead()) {
            if (HandleTLS::funcOnReadHello == it->mCall && mOutBuffers.getSize() > 0) {
                handshake();
            }
            return;
        }
    }

    mInBuffers.setRet(0);
    mFlag = mTCP.getFlag();
    Logger::log(ELL_INFO, "HandleTLS::onReadHello>>read 0, ecode=%d", it->mError);
}


void HandleTLS::onConnect(RequestFD* it) {
    DASSERT(it == &mWrite);
    mWrite.mUser = nullptr;
    mFlag = mTCP.getFlag();

    if (0 == it->mError) {
        ((TlsSession*)mTlsSession)->setConnectState();
        mWrite.mCall = HandleTLS::funcOnWriteHello;
        mRead.mCall = HandleTLS::funcOnReadHello;
        if (EE_OK == postRead()) {
            handshake();
            return;
        }
    }

    Logger::log(ELL_ERROR, "HandleTLS::onConnect>>ecode=%d", it->mError);
    //callback on close
}


}//namespace net
}//namespace app
