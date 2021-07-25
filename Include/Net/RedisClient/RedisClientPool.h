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


#ifndef APP_REDISCLIENTPOOL_H
#define	APP_REDISCLIENTPOOL_H

#include "Loop.h"
#include "Net/NetAddress.h"
#include "Net/HandleTCP.h"

namespace app {
class MemoryHub;

namespace net {
class RedisCommand;
class RedisRequest;
class RedisResponse;
class RedisClient;
class RedisClientCluster;


/**
 * @brief redis 连接池类
 */
class RedisClientPool {
public:
    RedisClientPool(Loop* loop, RedisClientCluster* cls = nullptr);

    ~RedisClientPool();

    const String& getPassword()const {
        return mPassword;
    }

    /**
     * 获得本连接池所对应的db
     * @return id of db
     */
    s32 getDatabaseID() const {
        return mDatabaseID;
    }

    void push(RedisClient* it);

    const net::NetAddress& getRemoterAddr()const {
        return mRemoterAddr;
    }

    void onTimeout(RedisClient* it);

    void onClose(RedisClient* it, bool fly);

    /**
     *
     * @param dbnum id of db, range[0-15], 在非集群模式下设置连接建立后所选择的db
     */
    void open(const net::NetAddress& serverIP, s32 maxTCP, const s8* passowrd, s32 dbnum = 0);

    void close();

    MemoryHub* getMemHub()const {
        return mHub;
    }

    void setMaxRetry(u32 it) {
        mMaxRetry = it;
    }

    u32 getMaxRetry() const {
        return mMaxRetry;
    }

    Loop* getLoop()const {
        return mLoop;
    }

    bool postTask(RedisCommand* it);

protected:
    void setPassword(const s8* pass);

    RedisCommand* popTask();
    void pushTask(RedisCommand* it);

    RedisClient* popIdle();
    void pushIdle(RedisClient* it);

    RedisClient* pop();


private:
    bool mRunning;
    String mPassword;
    s32 mDatabaseID;
    u32 mMaxRetry;
    net::NetAddress mRemoterAddr;
    Loop* mLoop;
    RedisClientCluster* mCluster;
    s32 mMaxTCP;
    s32 mFlyCount;
    s32 mFailCount;
    Node2 mAliveQueue;
    Node2 mIdleQueue;  //disconnected tcp
    Node2 mTask;
    MemoryHub* mHub;
};

} //namespace net
} //namespace app

#endif //APP_REDISCLIENTPOOL_H