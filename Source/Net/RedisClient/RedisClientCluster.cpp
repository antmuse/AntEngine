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


#include "Net/RedisClient/RedisClientCluster.h"
#include "Net/RedisClient/RedisClient.h"
#include "Logger.h"
#include "MemoryHub.h"
#include "Net/RedisClient/RedisRequest.h"
#include "Engine.h"


namespace app {
namespace net {

void AppClusterCallback(RedisRequest* it, RedisResponse* res) {
    RedisClientCluster* cls = it->getCluster();
    cls->updateSlots(res);
}


RedisClientCluster::RedisClientCluster() :
    mMaxSlots(16384),
    mLoop(&Engine::getInstance().getLoop()),
    mStatus(0),
    mMaxRedirect(3),
    mMaxTCP(3) {
    mSlots = new RedisClientPool * [mMaxSlots];
    memset(mSlots, 0, mMaxSlots * sizeof(RedisClientPool*));
}


RedisClientCluster::~RedisClientCluster() {
    TMap<net::NetAddress::ID, RedisClientPool*>::Iterator nd = mAllPool.getIterator();
    while (!nd.atEnd()) {
        RedisClientPool* pool = nd->getValue();
        delete pool;
        nd++;
    }

    delete[] mSlots;
    mSlots = nullptr;
}


void RedisClientCluster::updateSlots(RedisResponse* res) {
    if (res->isError()) {
        mStatus = 2;
        Logger::logError("RedisClientCluster::updateSlots>>failed");
        return;
    }
    res->show();
    net::NetAddress addr;
    s8* ids;
    for (u32 i = 0; i < res->mUsed; ++i) {
        RedisResponse* ndj = res->getResult(i);
        u64 slot1 = ndj->getResult(0)->getS64();
        u64 slot2 = ndj->getResult(1)->getS64();
        RedisResponse* ndk = ndj->getResult(2);
        addr.setIP(ndk->getResult(0)->getStr());
        addr.setPort((u16)ndk->getResult(1)->getS64());
        ids = ndk->getResult(2)->getStr();
        RedisClientPool* pool = open(addr, 0, nullptr);
        if (pool && slot2 < mMaxSlots) {
            for (; slot1 <= slot2; ++slot1) {
                mSlots[slot1] = pool;
            }
        } else {
            DASSERT(0);
        }
    }
    mStatus = 3;
}


void RedisClientCluster::setPassword(const s8* ipport, const s8* passowrd) {
    if (!ipport || !passowrd) {
        return;
    }
    net::NetAddress addr;
    addr.setIPort(ipport);
    setPassword(addr, passowrd);
}


void RedisClientCluster::setPassword(const net::NetAddress& addr, const s8* passowrd) {
    if (nullptr == passowrd) {
        return;
    }
    String pass(passowrd);
    mPassword.insert(addr.toID(), pass);
}


bool RedisClientCluster::getPassword(const net::NetAddress::ID& ipport, String& passowrd) {
    TMap<net::NetAddress::ID, String>::Node* nd = mPassword.find(ipport);
    if (nullptr == nd) {
        return false;
    }
    passowrd = nd->getValue();
    return true;
}


RedisClientPool* RedisClientCluster::open(const s8* ipport, u32 maxTCP, const s8* passowrd) {
    if (nullptr == ipport) {
        return nullptr;
    }
    net::NetAddress addr;
    addr.setIPort(ipport);
    return open(addr, maxTCP, passowrd);
}


RedisClientPool* RedisClientCluster::open(const net::NetAddress& addr, u32 maxTCP, const s8* passowrd) {
    maxTCP = 0 == maxTCP ? mMaxTCP : AppClamp(maxTCP, 1U, mMaxTCP);
    TMap<net::NetAddress::ID, RedisClientPool*>::Node* nd = mAllPool.find(addr.toID());
    if (nd) {
        return nd->getValue();
    }

    RedisClientPool* pool = new RedisClientPool(mLoop, this);
    if (!mAllPool.insert(addr.toID(), pool)) {
        delete pool;
        return nullptr;
    }
    String pass(64);
    if (nullptr == passowrd) {
        if (!getPassword(addr.toID(), pass)) {
            pass = mDefaultPassword;
        }
    } else {
        pass = passowrd;
        setPassword(addr, passowrd);
    }
    pool->open(addr, maxTCP, pass.c_str(), 0);
    if (1 == mAllPool.size()) {
        for (u32 i = 0; i < mMaxSlots; ++i) {
            mSlots[i] = pool;
        }
        updateSlots();
    }
    return pool;
}


void RedisClientCluster::onClose(RedisClientPool* it) {
    if (it) {
        clearSlots(it);
        mAllPool.remove(it->getRemoterAddr().toID());
        delete it;
    }
}


bool RedisClientCluster::updateSlots() {
    if (0 != mStatus) {
        return true;
    }
    if (0 == mAllPool.size()) {
        return false;
    }

    if (0 == mStatus) {
        RedisClientPool* pool = mAllPool.getIterator().getNode()->getValue();
        RedisRequest* cmd = new RedisRequest();
        cmd->setCallback(AppClusterCallback);
        cmd->setPool(pool);
        cmd->setCluster(this);
        mStatus = cmd->clusterSlots() ? 1 : 0;
        cmd->drop();
    }

    return 0 != mStatus;
}


void RedisClientCluster::close() {
    if (!mStatus) {
        return;
    }
    mStatus = 0;
    TMap<net::NetAddress::ID, RedisClientPool*>::Iterator nd = mAllPool.getIterator();
    while (!nd.atEnd()) {
        RedisClientPool* pool = nd->getValue();
        pool->close();
        nd++;
    }
}


void RedisClientCluster::clearSlot(u32 it) {
    if (it < mMaxSlots) {
        mSlots[it] = nullptr;
    }
}


void RedisClientCluster::clearSlots(RedisClientPool* it) {
    for (u32 i = 0; i < mMaxSlots; ++i) {
        if (mSlots[i] == it) {
            mSlots[i] = nullptr;
        }
    }
}


void RedisClientCluster::setSlot(u32 pos, RedisClientPool* it) {
    if (it && pos < mMaxSlots) {
        mSlots[pos] = it;
    }
}


RedisClientPool* RedisClientCluster::getBySlot(u32 it) {
    it = it % mMaxSlots;
    if (nullptr == mSlots[it]) {
        return nullptr;
    }
    RedisClientPool* ret = mSlots[it];
    return ret;
}


RedisClientPool* RedisClientCluster::getByAddress(const s8* iport) {
    if (nullptr == iport) {
        return nullptr;
    }
    net::NetAddress addr;
    addr.setIPort(iport);
    return open(addr, 0, nullptr);
}

} //namespace net
} //namespace app

