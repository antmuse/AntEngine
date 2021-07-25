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


#ifndef APP_REDISCOMMAND_H
#define	APP_REDISCOMMAND_H


#include "Node.h"
#include "MemoryHub.h"
#include "Net/RedisClient/RedisResponse.h"
#include "Net/RedisClient/RedisClientPool.h"
#include "Net/RedisClient/RedisClientCluster.h"

namespace app {
namespace net {

class RedisCommand : public Node2 {
public:
    RedisCommand();
    ~RedisCommand();

    void grab();

    void drop();

    void setPool(RedisClientPool* it) {
        if (it) {
            mPool = it;
            setMemHub(mPool->getMemHub());
        }
    }

    void setCluster(RedisClientCluster* it) {
        if (it) {
            mCluster = it;
        }
    }

    RedisClientPool* getPool()const {
        return mPool;
    }

    RedisClientCluster* getCluster()const {
        return mCluster;
    }

    RedisClient* getClient()const {
        return mLink;
    }

    void setClient(RedisClient* it) {
        mLink = it;
    }

    void setCallback(AppRedisCaller it) {
        mCallback = it;
    }

    AppRedisCaller getCallback()const {
        return mCallback;
    }

    void setMemHub(MemoryHub* it) {
        if (mHub) {
            return;
        }
        if (it) {
            it->grab();
            mHub = it;
        }
    }

    MemoryHub* getMemHub()const {
        return mHub;
    }


    void setUserPointer(void* it) {
        mUserPointer = it;
    }

    void* getUserPointer()const {
        return mUserPointer;
    }

    /**
    * @brief redirect: MOVED, ASK, fix slot if MOVED.
    *        MOVED 866 172.21.16.4:6379
    *        ASK 866 172.21.16.4:6379
    *
    * @param itype 1=Error, 2=ASK, 4=MOVED
    */
    bool relaunch(u32 slot, const s8* ip, s32 itype);

    u16 hashSlot(const s8* key, u64 len);

    u16 hashSlot(const s8* key) {
        return hashSlot(key, strlen(key));
    }

    u16 getSlot()const {
        return mSlot;
    }

    const s8* getRequestBuf()const {
        return mRequestBuf;
    }

    u32 getRequestSize()const {
        return mRequestSize;
    }

protected:
    MemoryHub* mHub;
    RedisClientPool* mPool;
    AppRedisCaller mCallback;
    RedisClientCluster* mCluster;
    s8* mRequestBuf;
    u32 mRequestSize;
    u32 mRequestCount; //relaunch times
    u16 mSlot;

    bool launch(u32 argc, const s8** argv, const u32* lens);

    bool launch(u32 slot, u32 argc, const s8** argv, const u32* lens);

    bool isClusterMode()const {
        return nullptr != mCluster;
    }

private:
    s32 mReferenceCount;
    void* mUserPointer;
    RedisClient* mLink;
    RedisCommand(const RedisCommand&) = delete;
    RedisCommand(RedisCommand&&) = delete;
    const RedisCommand& operator=(const RedisCommand&) = delete;
    const RedisCommand& operator=(RedisCommand&&) = delete;
};


} //namespace net {
} // namespace app

#endif //APP_REDISCOMMAND_H
