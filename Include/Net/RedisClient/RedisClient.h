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


#ifndef APP_REDISCLIENT_H
#define	APP_REDISCLIENT_H

#include "Converter.h"
#include "Loop.h"
#include "Packet.h"
#include "Net/NetAddress.h"
#include "Net/RedisClient/RedisResponse.h"
#include "Net/RedisClient/RedisRequest.h"


namespace app {
namespace net {
class RedisClientPool;

class RedisClient : public Node2 {
public:
    RedisClient(RedisClientPool* pool, MemoryHub* hub);

    ~RedisClient();

    /**
    * @brief inner use only
    */
    void setUserPointer(void* it) {
        mUserPointer = it;
    }

    /**
    * @brief inner use only
    */
    void* getUserPointer()const {
        return mUserPointer;
    }

    RedisClientPool* getRedisPool() const {
        return mPool;
    }

    s32 open(const net::NetAddress& remote) {
        mTCP.setRemote(remote);
        return open();
    }

    s32 close();

    bool postTask(RedisCommand* it);

    s32 onTimeout(HandleTime& it);

    void onClose(Handle* it);

    void onConnect(RequestFD* it);

    void onWrite(RequestFD* it);

    void onRead(RequestFD* it);

    net::HandleTCP& getHandleTCP() {
        return mTCP;
    }

private:

    static s32 funcOnTime(HandleTime* it) {
        RedisClient& nd = *(RedisClient*)it->getUser();
        return nd.onTimeout(*it);
    }

    static void funcOnWrite(RequestFD* it) {
        RedisClient& nd = *(RedisClient*)it->mUser;
        nd.onWrite(it);
    }

    static void funcOnRead(RequestFD* it) {
        RedisClient& nd = *(RedisClient*)it->mUser;
        nd.onRead(it);
    }

    static void funcOnConnect(RequestFD* it) {
        RedisClient& nd = *(RedisClient*)it->mUser;
        nd.onConnect(it);
    }

    static void funcOnClose(Handle* it) {
        RedisClient& nd = *(RedisClient*)it->getUser();
        nd.onClose(it);
    }

    /* SELECT is not allowed in cluster mode.
       0=init, 1=fly, 2=password, 4=select db(ready) */
    s32 mStatus;
    void* mUserPointer;
    RedisRequest* mCMD;
    RedisClientPool* mPool;
    net::HandleTCP mTCP;
    Loop* mLoop;
    MemoryHub* mHub;
    RedisResponse* mResult;

    void callback();

    void onPassword();

    void onSelectDB();

    s32 open();
};

} //namespace net
} //namespace app

#endif //APP_REDISCLIENT_H