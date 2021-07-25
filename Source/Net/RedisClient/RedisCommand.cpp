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


#include "Net/RedisClient/RedisCommand.h"
#include "Net/RedisClient/RedisClient.h"
#include "CheckCRC.h"
#include "Logger.h"

namespace app {
namespace net {


RedisCommand::RedisCommand() :
    mSlot(0),
    mReferenceCount(1),
    mCluster(nullptr),
    mLink(nullptr),
    mPool(nullptr),
    mHub(nullptr),
    mRequestBuf(nullptr),
    mRequestSize(0),
    mRequestCount(0),
    mUserPointer(nullptr),
    mCallback(nullptr) {
}

RedisCommand::~RedisCommand() {
    if (mRequestBuf) {
        mHub->release(mRequestBuf);
        mRequestBuf = nullptr;
        mRequestSize = 0;
    }
    mHub->drop();
    mHub = nullptr;
}


void RedisCommand::grab() {
    DASSERT(mReferenceCount > 0);
    ++mReferenceCount;
}

void RedisCommand::drop() {
    s32 ret = --mReferenceCount;
    DASSERT(ret >= 0);
    if (0 == ret) {
        delete this;
    }
}

bool RedisCommand::launch(u32 slot, u32 argc, const s8** argv, const u32* lens) {
    if (nullptr == mCluster) {
        DASSERT(0);
        return false;
    }
    RedisClientPool* pool = mCluster->getBySlot(slot);
    if (nullptr == pool) {
        Logger::logError("RedisCommand::launch>>can't get pool, slot=%u", slot);
        DASSERT(0);
        return false;
    }
    setPool(pool);
    return launch(argc, argv, lens);
}

bool RedisCommand::launch(u32 argc, const s8** argv, const u32* lens) {
    if (!mPool) {
        return false;
    }
    if (mRequestBuf) { //reuse?
        mHub->release(mRequestBuf);
        mRequestBuf = nullptr;
        mRequestSize = 0;
    }
    u64 allsz = 13ULL + 13ULL * argc + 1; //0xFFFFFFFF 最大10位
    for (u32 i = 0; i < argc; ++i) {
        allsz += lens[i];
    }
    s8* const buf = mHub->allocate(allsz);
    s8* curr = buf + snprintf(buf, allsz, "*%u\r\n", argc);
    for (u32 i = 0; i < argc; ++i) {
        curr += snprintf(curr, allsz - (curr - buf), "$%u\r\n", lens[i]);
        memcpy(curr, argv[i], lens[i]);
        curr += lens[i];
        *curr++ = '\r';
        *curr++ = '\n';
    }
    *curr = '\0';   //for debug: show str
    mRequestBuf = buf;
    mRequestSize = static_cast<u32>(curr - buf);
    if (mPool->postTask(this)) {
        return true;
    }
    mHub->release(buf);
    mRequestBuf = nullptr;
    mRequestSize = 0;
    mRequestCount = 1;
    return false;
}

bool RedisCommand::relaunch(u32 slot, const s8* iport, s32 itype) {
    ++mRequestCount;
    if (mCluster) {
        if (mRequestCount > mCluster->getMaxRedirect()) {
            Logger::logInfo("RedisCommand::relaunch>>Redirect threshold=%u, launched=%u",
                mCluster->getMaxRedirect(), mRequestCount);
            return false;
        }
        //重定向或原槽位上重试
        mPool = itype > 1 ? mCluster->getByAddress(iport) : mCluster->getBySlot(slot);
        if (!mPool) {
            Logger::logError("RedisCommand::relaunch>>can't get pool, slot=%u, IP=%s", slot, iport ? iport : "");
            return false;
        }
        if (4 == itype) {
            mCluster->setSlot(mSlot, mPool);
            mCluster->setSlot(slot, mPool);
        }
    } else {
        //非集群模式下重试
        if (!mPool) {
            Logger::logError("RedisCommand::relaunch>>can't retry, slot=%u, IP=%s", slot, iport ? iport : "");
            return false;
        }
        if (mRequestCount > mPool->getMaxRetry()) {
            Logger::logInfo("RedisCommand::relaunch>>more retry=%u, max=%u", mRequestCount, mPool->getMaxRetry());
            return false;
        }
    }

    if (mPool->postTask(this)) {
        return true;
    }
    mHub->release(mRequestBuf);
    mRequestBuf = nullptr;
    mRequestSize = 0;
    return false;
}

u16 RedisCommand::hashSlot(const s8* key, u64 len) {
    if (key) {
        CheckCRC16 crc;
        mSlot = crc.add(key, static_cast<u32>(len));
        return mSlot;
    }
    mSlot = 0;
    return 0;
}

} //namespace net
} //namespace app

