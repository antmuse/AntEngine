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


#ifndef APP_REDISCLIENTCLUSTER_H
#define	APP_REDISCLIENTCLUSTER_H

#include "TMap.h"
#include "Strings.h"
#include "Net/NetAddress.h"
#include "Net/RedisClient/RedisClientPool.h"

namespace app {
namespace net {
class RedisResponse;

/**
 * @brief redis cluster connection pool.
 */
class RedisClientCluster {
public:
    RedisClientCluster();

    ~RedisClientCluster();

    /**
     * @brief reponse of redis5.0(cluster slots) is like:
     * 10.1.63.126:5000> CLUSTER SLOTS
     * 1) 1) (integer) 10923
     *    2) (integer) 16383
     *    3) 1) "10.1.63.126"
     *       2) (integer) 5000
     *       3) "767748b5eafab62bb0fb022e2768b2af811c6233"
     *    4) 1) "10.1.63.128"
     *       2) (integer) 5100
     *       3) "9f25c264ba29a8622da1f88c6ca79d605f88073c"
     * 2) 1) (integer) 0
     *    2) (integer) 5460
     *    3) 1) "10.1.63.127"
     *       2) (integer) 5100
     *       3) "74303a35dc0aead2820dd10bb46290525d945994"
     *    4) 1) "10.1.63.128"
     *       2) (integer) 5000
     *       3) "5c34ac955c6df13bf4ef7ef721d6b0f701d9813f"
     * 3) 1) (integer) 5461
     *    2) (integer) 10922
     *    3) 1) "10.1.63.126"
     *       2) (integer) 5100
     *       3) "dfe76fde02fc8b90e969eebf549c409f66386ef0"
     *    4) 1) "10.1.63.127"
     *       2) (integer) 5000
     *       3) "31727a89e6e6b7a614129c7b8832390f84eb8f38"
     */
    void updateSlots(RedisResponse* res);

    bool updateSlots();


    /**
    * @brief open a pool
    * @param passowrd 为空则使用默认密码
    */
    RedisClientPool* open(const s8* ipport, u32 maxTCP = 0, const s8* passowrd = nullptr);

    RedisClientPool* open(const net::NetAddress& addr, u32 maxTCP = 0, const s8* passowrd = nullptr);

    const TMap<net::NetAddress::ID, String>& getAllPassword()const {
        return mPassword;
    }

    /**
    * @brief set password against to IP:Port
    */
    void setPassword(const s8* ipport, const s8* passowrd);

    /**
    * @param passowrd 设置默认密码
    */
    void setPassword(const s8* passowrd) {
        if (passowrd) {
            mDefaultPassword = passowrd;
        }
    }

    const String& getPassword()const {
        return mDefaultPassword;
    }

    void setPassword(const net::NetAddress& addr, const s8* passowrd);

    bool getPassword(const net::NetAddress::ID& id, String& passowrd);

    void setMaxTcp(u32 it) {
        mMaxTCP = AppClamp(it, 1U, 1000U);
    }

    u32 getMaxTcp() const {
        return mMaxTCP;
    }

    void setMaxRedirect(u32 it) {
        mMaxRedirect = it;
    }

    u32 getMaxRedirect() const {
        return mMaxRedirect;
    }

    u32 getMaxSlot() const {
        return mMaxSlots;
    }

    RedisClientPool* getBySlot(u32 it);

    RedisClientPool* getByAddress(const s8* iport);

    void onClose(RedisClientPool* it);

    void setSlot(u32 pos, RedisClientPool* it);

    void close();

private:
    //0=init,1=starting,2=got slots error,3=started
    volatile u32 mStatus;
    u32 mMaxRedirect; //limit of moved or ask
    u32 mMaxSlots;
    RedisClientPool** mSlots;
    Loop* mLoop;
    u32 mMaxTCP;
    TMap<net::NetAddress::ID, String> mPassword;
    String mDefaultPassword;
    TMap<net::NetAddress::ID, RedisClientPool*> mAllPool;

    void clearSlot(u32 it);
    void clearSlots(RedisClientPool* it);
};

} //namespace net
} // namespace app

#endif //APP_REDISCLIENTCLUSTER_H