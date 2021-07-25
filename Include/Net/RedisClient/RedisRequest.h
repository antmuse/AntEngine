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

#ifndef APP_REDISREQUEST_H
#define	APP_REDISREQUEST_H


#include "Net/RedisClient/RedisCommand.h"

namespace app {
namespace net {

class RedisRequest : public RedisCommand {
public:
    RedisRequest();
    ~RedisRequest();

    //---------------------------------cluster---------------------------------
    bool clusterInfo();
    bool clusterNodes();
    bool clusterSlots();

    //---------------------------------connect---------------------------------
    bool auth(const s8* passowrd);
    bool echo(const s8* msg);
    bool select(s32 dbID);
    bool ping();
    bool quit();
    bool shutdown();
    bool save();
    bool bgsave();
    bool bgrewriteaof();
    bool lastsave();

    //---------------------------------key---------------------------------
    bool del(const s8* key);
    bool del(const s8* key, u32 kenLen);

    bool unlink(const s8* key);
    bool unlink(const s8* key, u32 kenLen);

    bool ttl(const s8* key);
    bool ttl(const s8* key, u32 kenLen);

    bool pttl(const s8* key);
    bool pttl(const s8* key, u32 kenLen);

    bool objectRefcount(const s8* key);
    bool objectRefcount(const s8* key, u32 keyLen);
    bool objectEncoding(const s8* key);
    bool objectEncoding(const s8* key, u32 keyLen);
    bool objectIdletime(const s8* key);
    bool objectIdletime(const s8* key, u32 keyLen);

    bool type(const s8* key);
    bool type(const s8* key, u32 kenLen);

    bool dump(const s8* key);
    bool dump(const s8* key, u32 kenLen);

    bool randomkey();

    /**
    * @brief rename key1 to key2.
    */
    bool rename(const s8* key1, const s8* key2);
    bool rename(const s8* key1, u32 kenLen1, const s8* key2, u32 kenLen2);

    bool renamenx(const s8* key1, const s8* key2);
    bool renamenx(const s8* key1, u32 kenLen1, const s8* key2, u32 kenLen2);

    /**
    * @param timeout in seconds.
    */
    bool expire(const s8* key, u32 timeout);
    bool expire(const s8* key, u32 kenLen, u32 timeout);

    bool move(const s8* key, u32 destDB);
    bool move(const s8* key, u32 kenLen, u32 destDB);

    /**
    * @param timeout in milliseconds.
    */
    bool pexpire(const s8* key, u32 timeout);
    bool pexpire(const s8* key, u32 kenLen, u32 timeout);

    bool pexpireat(const s8* key, s64 timestamp);
    bool pexpireat(const s8* key, u32 kenLen, s64 timestamp);

    /**
    * @brief 用 UNIX 时间截设置 KEY 的生存周期
    * set the expiration for a key as a UNIX timestamp
    * @param timestamp UNIX 时间截，即自 1970 年以来的秒数
    */
    bool expireat(const s8* key, s64 timestamp);
    bool expireat(const s8* key, u32 kenLen, s64 timestamp);

    bool exists(const s8* key);
    bool exists(const s8* key, u32 kenLen);

    bool keys(const s8* key);
    bool keys(const s8* key, u32 kenLen);

    bool persist(const s8* key);
    bool persist(const s8* key, u32 kenLen);

    bool restore(const s8* key, u32 keyLen, const s8* key2, u32 keyLen2, s32 ttl, bool replace);

    bool scan(u32 offset, const s8* pattern, u32 patLen, u32 count = 0);

    /**
    * @brief MIGRATE host port key destination-db timeout [COPY] [REPLACE]
    * @param flag c=[COPY] r=[REPLACE] 0=default
    * eg: MIGRATE 127.0.0.1 8899 greeting 0 1000
    */
    bool migrate(const s8* ip, u16 port, const s8* key, u32 keyLen, u32 destDB, u32 timeout, s8 flag = 0);





    //---------------------------------list---------------------------------
    bool lset(const s8* key, u32 keyLen, u32 pos, const s8* val, u32 valLen);
    bool lindex(const s8* key, u32 keyLen, u32 pos);

    //LINSERT key BEFORE|AFTER pivot value
    bool linsert(const s8* key, u32 keyLen, const s8* pivot, u32 pivotLen,
        const s8* val, u32 valLen, bool after);
     
    bool lrem(const s8* key, u32 keyLen, s32 pos, const s8* val, u32 valLen);
    bool ltrim(const s8* key, u32 keyLen, s32 start, s32 end);

    bool lpush(const s8* key, u32 keyLen, const s8* val, u32 valLen);
    bool rpush(const s8* key, u32 keyLen, const s8* val, u32 valLen);

    bool lpushx(const s8* key, u32 keyLen, const s8* val, u32 valLen);
    bool rpushx(const s8* key, u32 keyLen, const s8* val, u32 valLen);

    bool rpoplpush(const s8* key, u32 keyLen, const s8* val, u32 valLen);
    bool brpoplpush(const s8* key, u32 keyLen, const s8* val, u32 valLen);

    bool lpop(const s8* key, u32 keyLen);
    bool rpop(const s8* key, u32 keyLen);

    bool blpop(const s8* key, u32 keyLen);
    bool brpop(const s8* key, u32 keyLen);

    bool llen(const s8* key, u32 keyLen);
    bool lrange(const s8* key, u32 keyLen, s32 start, s32 end);


    //---------------------------------string---------------------------------
    bool set(const s8* key, const s8* value);
    bool set(const s8* key, u32 kenLen, const s8* value, u32 valLen);
    bool mset(const s8** key, const u32* keyLen, u32 count);
    bool msetnx(const s8** key, const u32* keyLen, u32 count);
    bool mget(const s8** key, const u32* keyLen, u32 count);
    bool getset(const s8* key, u32 kenLen, const s8* value, u32 valLen);
    bool setnx(const s8* key, u32 kenLen, const s8* value, u32 valLen);
    bool setex(const s8* key, u32 kenLen, const s8* value, u32 valLen, u32 timeout);
    bool psetex(const s8* key, u32 kenLen, const s8* value, u32 valLen, u32 timeout);
    bool get(const s8* key);
    bool get(const s8* key, u32 keyLen);
    bool incr(const s8* key, u32 keyLen);
    bool decr(const s8* key, u32 keyLen);
    bool incrby(const s8* key, u32 keyLen, s32 addon);
    bool incrbyfloat(const s8* key, u32 keyLen, f64 addon);
    bool decrby(const s8* key, u32 keyLen, s32 addon);
    bool stringlen(const s8* key, u32 keyLen);
    bool append(const s8* key, u32 keyLen, const s8* value, u32 valLen);
    bool setrange(const s8* key, u32 kenLen, const s8* value, u32 valLen, u32 offset);
    bool getrange(const s8* key, u32 keyLen, s32 start, s32 end);


    //---------------------------------set---------------------------------
    bool sdiff(const s8** keys, const u32* keyLens, u32 count);
    bool sdiffstore(const s8* destKey, u32 destKeyLen, const s8** keys, const u32* keyLens, u32 count);
    bool sunion(const s8** keys, const u32* keyLens, u32 count);
    bool sunionstore(const s8* destKey, u32 destKeyLen, const s8** keys, const u32* keyLens, u32 count);
    bool sinter(const s8** keys, const u32* keyLens, u32 count);
    //SINTERSTORE
    bool sinterstore(const s8* destKey, u32 destKeyLen, const s8** keys, const u32* keyLens, u32 count);
    bool sadd(const s8* key, u32 keyLen, const s8* val, const u32 valLens);
    bool sadd(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    //SISMEMBER key member
    bool sismember(const s8* key, u32 keyLen, const s8* val, const u32 valLens);
    bool spop(const s8* key, u32 keyLen);
    bool scard(const s8* key, u32 keyLen);
    bool smembers(const s8* key, u32 keyLen);
    //SRANDMEMBER key [count]
    bool srandmember(const s8* key, u32 keyLen, u32 count = 0);
    bool srem(const s8* key, u32 keyLen, const s8* val, const u32 valLens);
    bool srem(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    bool smove(const s8* key1, u32 keyLen1, const s8* key2, u32 keyLen2, const s8* val, const u32 valLen);
    bool sscan(const s8* key, u32 keyLen, u32 offset, const s8* pattern, u32 patLen, u32 count = 0);


    //---------------------------------zset---------------------------------
    /*
    * @brief ZADD key score member [[score member] [score member] …]
    */
    bool zadd(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    bool zscore(const s8* key, u32 keyLen, const s8* val, const u32 valLens);
    //ZINCRBY key increment member
    bool zincrby(const s8* key, u32 keyLen, const s8* val, const u32 valLens, s32 value);
    bool zcard(const s8* key, u32 keyLen);
    //ZCOUNT key min max
    bool zcount(const s8* key, u32 keyLen, s32 min, s32 max);
    //ZRANGE key start stop [WITHSCORES]
    bool zrange(const s8* key, u32 keyLen, s32 start, s32 stop, bool withScore = false);
    bool zrevrange(const s8* key, u32 keyLen, s32 start, s32 stop, bool withScore = false);
    /**
    * @brief ZRANGEBYSCORE key min max [WITHSCORES] [LIMIT offset count]
    * @param flag 位开关 1: >min, 2: -inf, 3: <max, 4: +inf
    */
    bool zrangebyscore(const s8* key, u32 keyLen, f32 min, f32 max, u32 flag = 0, bool withScore = false);
    //ZREVRANGEBYSCORE key max min [WITHSCORES] [LIMIT offset count]
    bool zrevrangebyscore(const s8* key, u32 keyLen, f32 min, f32 max, u32 flag = 0, bool withScore = false);
    bool zrank(const s8* key, u32 keyLen, const s8* val, const u32 valLens);
    bool zrevrank(const s8* key, u32 keyLen, const s8* val, const u32 valLens);
    bool zrem(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    //ZREMRANGEBYRANK key start stop
    bool zremrangebyrank(const s8* key, u32 keyLen, s32 start, s32 stop);
    //ZREMRANGEBYSCORE key min max
    bool zremrangebyscore(const s8* key, u32 keyLen, f32 min, f32 max, u32 flag = 0);
    //ZRANGEBYLEX key min max [LIMIT offset count]
    bool zrangebylex(const s8* key, u32 keyLen, const s8* min, u32 minLen, const s8* max, u32 maxLen);
    //ZLEXCOUNT key min max
    bool zlexcount(const s8* key, u32 keyLen, const s8* min, u32 minLen, const s8* max, u32 maxLen);
    //ZREMRANGEBYLEX key min max
    bool zremrangebylex(const s8* key, u32 keyLen, const s8* min, u32 minLen, const s8* max, u32 maxLen);
    //ZSCAN key cursor [MATCH pattern] [COUNT count]
    bool zscan(const s8* key, u32 keyLen, u32 offset, const s8* pattern, u32 patLen, u32 count = 0);
    /**
    * @brief ZUNIONSTORE destination numkeys key [key …] [WEIGHTS weight [weight …]] [AGGREGATE SUM|MIN|MAX]
    * @note destkey used for slot calculate, cmd successed when all the keys should in same slots.
    * @param aggregate 0=SUM,1=MAX,-1=MIN
    */
    bool zunionstore(const s8* destkey, u32 destkeyLen,
        const s8** val, const u32* valLens,
        const s8** weight, const u32* weightLens,
        u32 count, s32 aggregate = 0);
    /**
    * @brief ZINTERSTORE destination numkeys key [key …] [WEIGHTS weight [weight …]] [AGGREGATE SUM|MIN|MAX]
    * @note destkey used for slot calculate, cmd successed when all the keys should in same slots.
    * @see zunionstore
    * @param aggregate 0=SUM,1=MAX,-1=MIN
    */
    bool zinterstore(const s8* destkey, u32 destkeyLen,
        const s8** val, const u32* valLens,
        const s8** weight, const u32* weightLens,
        u32 count, s32 aggregate = 0);

    //---------------------------------hash---------------------------------
    bool hset(const s8* key, u32 keyLen, const s8* name, u32 nameLen, const s8* value, u32 valLen);
    bool hsetnx(const s8* key, u32 keyLen, const s8* name, u32 nameLen, const s8* value, u32 valLen);
    bool hget(const s8* key, u32 keyLen, const s8* name, u32 nameLen);
    bool hgetall(const s8* key, u32 keyLen);
    bool hexists(const s8* key, u32 keyLen, const s8* name, u32 nameLen);
    bool hdel(const s8* key, u32 keyLen, const s8* name, u32 nameLen);
    bool hlen(const s8* key, u32 keyLen);
    bool hstrlen(const s8* key, u32 keyLen, const s8* name, u32 nameLen);
    bool hincrby(const s8* key, u32 keyLen, const s8* name, u32 nameLen, s32 val);
    bool hincrbyfloat(const s8* key, u32 keyLen, const s8* name, u32 nameLen, f32 val);
    bool hkeys(const s8* key, u32 keyLen);
    bool hvals(const s8* key, u32 keyLen);
    bool hscan(const s8* key, u32 keyLen, u32 offset, const s8* pattern, u32 patLen, u32 count = 0);
    bool hmset(const s8* key, u32 keyLen, const s8** pairs, const u32* pairLens, u32 count);
    bool hmget(const s8* key, u32 keyLen, const s8** name, const u32* nameLen, u32 count);

    //---------------------------------HyperLogLog---------------------------------
    //PFADD key element [element …]
    bool pfadd(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    //PFCOUNT key [key …]
    bool pfcount(const s8** val, const u32* valLens, u32 count);
    /**
    * @brief PFMERGE destkey sourcekey [sourcekey …]
    * @param count src keys count
    */
    bool pafmerge(const s8* destkey, u32 destkeyLen, const s8** srckeys, const u32* srcLens, u32 count);


    //---------------------------------GEO---------------------------------
    //GEOADD key longitude latitude member [longitude latitude member …]
    bool geoadd(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    //GEOPOS key member [member …]
    bool geopos(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);
    /**
    * @brief GEODIST key member1 member2 [unit]
    * unit=
    * 0=m 表示单位为米。
    * 1=km 表示单位为千米。
    * 2=mi 表示单位为英里。
    * 3=ft 表示单位为英尺。
    */
    bool geodist(const s8* key, u32 keyLen,
        const s8* key1, u32 keyLen1,
        const s8* key2, u32 keyLen2, u32 unit = 0);

    /**
    * @brief GEORADIUS key longitude latitude radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [ASC|DESC] [COUNT count]
    * @param unit @see geodist()
    * @param flag 1=WITHCOORD,2=WITHDIST,4=WITHHASH,8=ASC|DESC
    */
    bool georadius(const s8* key, u32 keyLen,
        f32 posX, f32 posY, f32 redius, u32 max = 0, u32 unit = 0, u32 flag = 0);

    /**
    * @brief GEORADIUSBYMEMBER key member radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [ASC|DESC] [COUNT count]
    * @param unit @see geodist()
    * @param flag 1=WITHCOORD,2=WITHDIST,4=WITHHASH,8=ASC|DESC
    */
    bool georadiusbymember(const s8* key, u32 keyLen,
        const s8* member, u32 memberLen, f32 redius, u32 max = 0, u32 unit = 0, u32 flag = 0);
    /**
    * @brief GEOHASH key member [member …]
    */
    bool geohash(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);


    //---------------------------------BITMAP---------------------------------
    /**
    * @brief SETBIT key offset value
    */
    bool setbit(const s8* key, u32 keyLen, u32 offset, bool val);
    /**
    * @brief GETBIT key offset
    */
    bool getbit(const s8* key, u32 keyLen, u32 offset);

    /**
    * @brief BITCOUNT key[start][end]
    */
    bool bitcount(const s8* key, u32 keyLen, u32 min = 0, u32 max = 0xFFFFFFFF);

    /**
    * @brief BITPOS key bit [start] [end]
    */
    bool bitpos(const s8* key, u32 keyLen, bool val, u32 min = 0, u32 max = 0xFFFFFFFF);

    /**
    * @brief BITOP operation destkey key [key …]
    * @param flag 0=AND,1=OR,2=NOT,3=XOR
    */
    bool bitop(const s8* destkey, u32 destkeyLen, u32 flag,
        const s8** val, const u32* valLens, u32 count);
    /**
    * @brief BITFIELD key [GET type offset] [SET type offset value] [INCRBY type offset increment] [OVERFLOW WRAP|SAT|FAIL]
    * @param 
    */
    bool bitfiled(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count);


    //---------------------------------Publish---------------------------------
    //PUBLISH channel message
    bool publish(const s8* key, u32 keyLen, const s8* val, const u32 valLen);
    //SUBSCRIBE channel [channel …]
    bool subscribe(const s8** val, const u32* valLens, u32 count);
    //UNSUBSCRIBE [channel [channel …]]
    bool unsubscribe(const s8** val = nullptr, const u32* valLens = nullptr, u32 count = 0);
    //PSUBSCRIBE pattern [pattern …]
    bool psubscribe(const s8** val, const u32* valLens, u32 count);
    //PUNSUBSCRIBE [pattern [pattern …]]
    bool punsubscribe(const s8** val = nullptr, const u32* valLens = nullptr, u32 count = 0);
    //PUBSUB <subcommand> [argument [argument …]]
    //TODO>>PUBSUB

    //---------------------------------OOO---------------------------------

private:
};


} //namespace net {
} // namespace app

#endif //APP_REDISREQUEST_H