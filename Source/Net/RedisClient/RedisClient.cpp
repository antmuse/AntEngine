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


#include "Net/RedisClient/RedisClient.h"
#include "Logger.h"

namespace app {
namespace net {

RedisClient::RedisClient(RedisClientPool* pool, MemoryHub* hub) :
    mStatus(0),
    mPool(pool),
    mHub(hub),
    mUserPointer(nullptr),
    mResult(nullptr),
    mCMD(nullptr),
    mLoop(pool->getLoop()) {

    mTCP.setClose(EHT_TCP_CONNECT, RedisClient::funcOnClose, this);
    mTCP.setTime(RedisClient::funcOnTime, 15 * 1000, 20 * 1000, -1);
}


RedisClient::~RedisClient() {
    if (mResult) {
        mResult->~RedisResponse();
        mHub->release(mResult);
        mResult = nullptr;
    }
}


void RedisClient::onClose(Handle* it) {
    if (mCMD) {
        if (!mResult) {
            mResult = new
            (reinterpret_cast<RedisResponse*>(mHub->allocate(sizeof(RedisResponse)))) RedisResponse(*mHub);
        }
        if (!mResult->isFinished()) {
            mResult->makeError("RedisClient::onClose");
        }
        callback();
    }
    mPool->onClose(this, mStatus & 1);
}


void RedisClient::onWrite(RequestFD* it) {
    RequestFD::delRequest(it);
}


void RedisClient::onRead(RequestFD* it) {
    if (it->mUsed > 0) {
        if (nullptr == mResult) {
            mResult = new
            (reinterpret_cast<RedisResponse*>(mHub->allocate(sizeof(RedisResponse)))) RedisResponse(*mHub);
        }
        StringView dat = it->getReadBuf();
        mResult->import(dat.mData, dat.mLen);

        if (mResult->isFinished()) {
            if (3 == mStatus) {
                onSelectDB();
            } else if (1 == mStatus) {
                onPassword();
            } else {
                callback();
            }
        }
        it->mUsed = 0;
        if (EE_OK == mTCP.read(it)) {
            return;
        }
    }
    Logger::log(ELL_INFO, "RedisClient::onRead>>addr=%s, ecode=%d",
        mPool->getRemoterAddr().getStr(), it->mError);
    RequestFD::delRequest(it);
}


void RedisClient::callback() {
    RedisRequest* cmd = mCMD;
    RedisResponse* rst = mResult;
    mResult = nullptr;
    mCMD = nullptr;
    net::RedisClientPool* pool = cmd->getPool();
    DASSERT(pool);
    if (rst->isError()) {
        s32 replay = (rst->isAsk() ? 2 : (rst->isMoved() ? 4 : 1));
        if (replay > 1) {
            u32 slot = cmd->getSlot();
            const s8* ipt = rst->getStr()
                + (4 == replay ? sizeof("MOVED") : (2 == replay ? sizeof("ASK") : 0));
            slot = App10StrToU32(ipt, &ipt);
            ++ipt;
            if (cmd->relaunch(slot, ipt, replay)) {
                mStatus &= ~1;
                pool->push(this);
                rst->clear();
                cmd->drop();
                cmd = nullptr;
            }
        }
    }
    if (cmd) {
        mStatus &= ~1;
        pool->push(this);
        AppRedisCaller fun = cmd->getCallback();
        if (fun) {
            fun(cmd, rst);
        }
        cmd->drop();
        rst->clear();
    }
    rst->~RedisResponse();
    mHub->release(rst);
}


s32 RedisClient::open() {
    if (EE_OK != mLoop->openHandle(&mTCP)) {
        Logger::logError("RedisClient::open>>fail to open redis = %s", mTCP.getRemote().getStr());
        return EE_ERROR;
    }
    mStatus = 1;

    RequestFD* it = RequestFD::newRequest(4 * 1024);
    it->mUser = this;
    it->mCall = funcOnConnect;
    s32 ret = mTCP.connect(it);
    if (EE_OK != ret) {
        RequestFD::delRequest(it);
        Logger::logError("RedisClient::open>>fail to connect redis = %s", mTCP.getRemote().getStr());
    }
    return EE_OK;
}


s32 RedisClient::close() {
    return mLoop->closeHandle(&mTCP);
}


s32 RedisClient::onTimeout(HandleTime& it) {
    DASSERT(mTCP.getGrabCount() > 0);

    //RequestFD* tick = RequestFD::newRequest(sizeof(net::PackActive));
    //tick->mUser = this;
    //tick->mCall = RedisClient::funcOnWrite;
    //tick->mUsed = 1212;
    //if (0 != mTCP.write(tick)) {
    //    RequestFD::delRequest(tick);
    //    return EE_ERROR;
    //}
    if (mStatus & 1) {
        return EE_ERROR;
    }
    return EE_OK;
}


bool RedisClient::postTask(RedisCommand* it) {
    if (mCMD) {
        DASSERT(0);
        return false;
    }
    mCMD = static_cast<RedisRequest*>(it);
    mCMD->grab();

    RequestFD* out = RequestFD::newRequest(0);
    out->mData = (s8*)it->getRequestBuf();
    out->mAllocated = it->getRequestSize();
    out->mUsed = out->mAllocated;
    out->mCall = RedisClient::funcOnWrite;
    if (0 == mTCP.write(out)) {
        mStatus |= 1;
        return true;
    }

    RequestFD::delRequest(out);
    mCMD->drop();
    mCMD = nullptr;
    return false;
}


void RedisClient::onPassword() {
    if (mResult->isOK()) {
        mResult->clear();
        if (mPool->getDatabaseID() <= 0) {
            setUserPointer(nullptr);
            mStatus |= 2 | 4;
            mStatus &= ~1;
            mPool->push(this);
        } else {
            mStatus |= 2;
            RequestFD* out = RequestFD::newRequest(128);
            out->mUser = this;
            out->mCall = RedisClient::funcOnWrite;
            out->mUsed = snprintf(out->mData, out->mAllocated, "%d", mPool->getDatabaseID());
            out->mUsed = snprintf(out->mData, out->mAllocated,
                "*2\r\n$6\r\nSELECT$%u\r\n%d", out->mUsed, mPool->getDatabaseID());
            if (0 != mTCP.write(out)) {
                RequestFD::delRequest(out);
            }
        }
        return;
    } else {
        Logger::logError("RedisClient::onPassword>>addr=%s, err password=%s",
            mPool->getRemoterAddr().getStr(), mPool->getPassword().c_str());
        mPool->close();
    }
    mResult->clear();
    close();
}


void RedisClient::onSelectDB() {
    if (mResult->isOK()) {
        mResult->clear();
        mStatus |= 4;
        mStatus &= ~1;
        mPool->push(this);
        return;
    }
    mResult->clear();
    close();
}


void RedisClient::onConnect(RequestFD* it) {
    if (nullptr == mResult) {
        mResult = new
        (reinterpret_cast<RedisResponse*>(mHub->allocate(sizeof(RedisResponse)))) RedisResponse(*mHub);
    } else {
        mResult->clear();
    }
    if (0 == it->mError) {
        it->mCall = RedisClient::funcOnRead;
        if (0 == mTCP.read(it)) {
            const String& pwd = mPool->getPassword();
            const s32 dbid = mPool->getDatabaseID();
            if (dbid <= 0 && 0 == pwd.getLen()) {
                mStatus |= 2|4;
                mStatus &= ~1;
                mPool->push(this);
            } else {
                RequestFD* out = RequestFD::newRequest(128);
                out->mUser = this;
                out->mCall = RedisClient::funcOnWrite;
                if (pwd.getLen() > 0) {
                    //"CLUSTER SLOTS"="*2\r\n$7\r\nCLUSTER\r\n$5\r\nSLOTS\r\n"
                    //"AUTH 123456"="*2\r\n$4\r\nAUTH\r\n$6\r\n123456\r\n"
                    out->mUsed = snprintf(out->mData, out->mAllocated,
                        "*2\r\n$4\r\nAUTH\r\n$%llu\r\n%s\r\n", pwd.getLen(), pwd.c_str());
                } else {
                    mStatus |= 2;
                    //"SELECT 5"="*2\r\n$6\r\nSELECT\r\n$1\r\n5\r\n"
                    out->mUsed = snprintf(out->mData, out->mAllocated,
                        "*2\r\n$6\r\nSELECT\r\n$%llu\r\n%s\r\n", pwd.getLen(), pwd.c_str());
                }
                if (0 != mTCP.write(out)) {
                    RequestFD::delRequest(out);
                }
            }
            return;
        }
    }
    RequestFD::delRequest(it);
    mPool->onClose(this, mStatus & 1);
}


} //namespace net
} //namespace app

