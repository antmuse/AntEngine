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


#include "Net/RedisClient/RedisClientPool.h"
#include "Logger.h"
#include "MemoryHub.h"
#include "Net/RedisClient/RedisClient.h"
#include "Net/RedisClient/RedisRequest.h"

namespace app {
namespace net {

RedisClientPool::RedisClientPool(Loop* loop, RedisClientCluster* cls) :
    mLoop(loop),
    mCluster(cls),
    mDatabaseID(0),
    mFlyCount(0),
    mFailCount(0),
    mMaxRetry(2),
    mRunning(false),
    mPassword(32),
    mMaxTCP(3) {
    mHub = new MemoryHub();
}

RedisClientPool::~RedisClientPool() {
    /*std::chrono::milliseconds gap(200);
    while (0 != mFlyCount) {
        std::this_thread::sleep_for(gap);
    }*/
    DASSERT(0 == mFlyCount);
    RedisClient* nd;
    while (!mIdleQueue.empty()) {
        nd = reinterpret_cast<RedisClient*>(mIdleQueue.getNext());
        nd->delink();
        delete nd;
    }

    mHub->drop();
    mHub = nullptr;
}

void RedisClientPool::open(const net::NetAddress& serverIP, s32 maxTCP, const s8* passowrd, s32 dbID) {
    if (mRunning) {
        return;
    }
    mRunning = true;
    mRemoterAddr = serverIP;
    mDatabaseID = dbID;
    mMaxTCP = AppClamp(maxTCP, 1, 1000);
    setPassword(passowrd);
    for (s32 i = 0; i < mMaxTCP; ++i) {
        RedisClient* it = new RedisClient(this, mHub);
        if (EE_OK == it->open(serverIP)) {
            ++mFlyCount;
        } else {
            pushIdle(it);
            //delete it;
            Logger::logError("RedisClientPool::open>>fail to open connect,server=%s", serverIP.getStr());
        }
    }
}

void RedisClientPool::onTimeout(RedisClient* it) {

}

void RedisClientPool::onClose(RedisClient* it, bool fly) {
    if (fly) {
        if (mRunning && EE_OK == it->open(mRemoterAddr)) {
            return;
        }
        --mFlyCount;
        pushIdle(it);
    } else {
        it->delink();
        if (mRunning && EE_OK == it->open(mRemoterAddr)) {
            return;
        }
        pushIdle(it);
    }
    //Logger::logError("RedisClientPool::onClose>>stop reconnect server=%s", mRemoterAddr.getStr());
}

RedisClient* RedisClientPool::popIdle() {
    if (mIdleQueue.empty()) {
        return nullptr;
    }
    RedisClient* ret = reinterpret_cast<RedisClient*>(mIdleQueue.getPrevious());
    ret->delink();
    ret->setUserPointer(nullptr);
    return ret;
}

void RedisClientPool::pushIdle(RedisClient* it) {
    DASSERT(it);
    //it->delink();
    mIdleQueue.pushBack(*it);
    if (++mFailCount == mMaxTCP) {
        if (mCluster) {
            RedisResponse* resp = new
            (reinterpret_cast<RedisResponse*>(mHub->allocate(sizeof(RedisResponse)))) RedisResponse(*mHub);
            resp->makeError("RedisClientPool::exit");
            for (RedisCommand* tsk = popTask(); tsk; tsk = popTask()) {
                if (tsk->getCallback()) {
                    tsk->getCallback()((RedisRequest*)tsk, resp);
                }
            }
            resp->~RedisResponse();
            mHub->release(resp);
            mCluster->onClose(this);
        }
    }
}

RedisClient* RedisClientPool::pop() {
    if (mAliveQueue.empty()) {
        return nullptr;
    }
    RedisClient* ret = reinterpret_cast<RedisClient*>(mAliveQueue.getPrevious());
    ret->delink();
    ++mFlyCount;
    return ret;
}

void RedisClientPool::push(RedisClient* it) {
    DASSERT(it);
    RedisCommand* req = popTask();
    if (req) {
        if (it->postTask(req)) {
            req->drop();
            return;
        }
        //req->grab();
        pushTask(req);
    }

    if (!mRunning) {
        it->close();
    }
    mAliveQueue.pushBack(*it);
    --mFlyCount;
}

RedisCommand* RedisClientPool::popTask() {
    if (mTask.empty()) {
        return nullptr;
    }
    RedisRequest* ret = reinterpret_cast<RedisRequest*>(mTask.getPrevious());
    ret->delink();
    return ret;
}

void RedisClientPool::pushTask(RedisCommand* req) {
    DASSERT(req);
    mTask.pushBack(*req);
}

bool RedisClientPool::postTask(RedisCommand* req) {
    if (!mRunning) {
        return false;
    }
    RedisClient* nd = pop();
    if (nd) {
        if (nd->postTask(req)) {
            return true;
        }
        push(nd);
    }
    req->grab();
    pushTask(req);
    return true;
}

void RedisClientPool::setPassword(const s8* pass) {
    if (pass && *pass) {
        mPassword = pass;
    }
}

void RedisClientPool::close() {
    if (!mRunning) {
        return;
    }
    mRunning = false;

    RedisClient* nd = reinterpret_cast<RedisClient*>(mAliveQueue.getNext());
    for (; nd != &mAliveQueue; nd = reinterpret_cast<RedisClient*>(nd->getNext())) {
        nd->close();
    }
}

} //namespace net
} // namespace app

