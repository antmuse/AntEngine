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


#include "Net/RedisClient/RedisRequest.h"

namespace app {
namespace net {

bool RedisRequest::zadd(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLens || 0 == count) {
        return false;
    }
    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"ZADD";
    lens[0] = sizeof("ZADD") - 1;

    argv[1] = const_cast<s8*>(key);
    lens[1] = keyLen;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(val[i]);
        lens[i + start] = valLens[i];
    }

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(key, keyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::zrem(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLens || 0 == count) {
        return false;
    }
    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"ZREM";
    lens[0] = sizeof("ZREM") - 1;

    argv[1] = const_cast<s8*>(key);
    lens[1] = keyLen;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(val[i]);
        lens[i + start] = valLens[i];
    }

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(key, keyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::zscore(const s8* key, u32 keyLen, const s8* val, const u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZSCORE";
    lens[0] = sizeof("ZSCORE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zincrby(const s8* key, u32 keyLen, const s8* val, const u32 valLen, s32 value) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZINCRBY";
    lens[0] = sizeof("ZINCRBY") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    argv[3] = val;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrank(const s8* key, u32 keyLen, const s8* val, const u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZRANK";
    lens[0] = sizeof("ZRANK") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrevrank(const s8* key, u32 keyLen, const s8* val, const u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZREVRANK";
    lens[0] = sizeof("ZREVRANK") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zcard(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZCARD";
    lens[0] = sizeof("ZCARD") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zcount(const s8* key, u32 keyLen, s32 min, s32 max) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZCOUNT";
    lens[0] = sizeof("ZCOUNT") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", min);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", max);
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrange(const s8* key, u32 keyLen, s32 start, s32 stop, bool withScore) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 5;
    const s8* argv[max];
    u32 lens[max];
    argv[0] = (s8*)"ZRANGE";
    lens[0] = sizeof("ZRANGE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", start);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", stop);
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    u32 cnt = 4;
    if (withScore) {
        argv[4] = key;
        lens[4] = keyLen;
        ++cnt;
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zremrangebyrank(const s8* key, u32 keyLen, s32 start, s32 stop) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZREMRANGEBYRANK";
    lens[0] = sizeof("ZREMRANGEBYRANK") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", start);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", stop);
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);


    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrevrange(const s8* key, u32 keyLen, s32 start, s32 stop, bool withScore) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 5;
    const s8* argv[max];
    u32 lens[max];
    argv[0] = (s8*)"ZREVRANGE";
    lens[0] = sizeof("ZREVRANGE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", start);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", stop);
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    u32 cnt = 4;
    if (withScore) {
        argv[4] = key;
        lens[4] = keyLen;
        ++cnt;
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrangebyscore(const s8* key, u32 keyLen, f32 iMin, f32 iMax, u32 flag, bool withScore) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 5;
    const s8* argv[max];
    u32 lens[max];
    argv[0] = (s8*)"ZRANGEBYSCORE";
    lens[0] = sizeof("ZRANGEBYSCORE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    if (0 != (flag & 2U)) {
        snprintf(buf, sizeof(buf), "-inf");
    } else if (0 != (flag & 1U)) {
        snprintf(buf, sizeof(buf), "(%f", iMin);
    } else {
        snprintf(buf, sizeof(buf), "%f", iMin);
    }
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    if (0 != (flag & 8U)) {
        snprintf(buf2, sizeof(buf2), "+inf");
    } else if (0 != (flag & 4U)) {
        snprintf(buf2, sizeof(buf2), "(%f", iMax);
    } else {
        snprintf(buf2, sizeof(buf2), "%f", iMax);
    }
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    u32 cnt = 4;
    if (withScore) {
        argv[4] = (s8*)"WITHSCORES";
        lens[4] = sizeof("WITHSCORES") - 1;
        ++cnt;
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrevrangebyscore(const s8* key, u32 keyLen, f32 iMin, f32 iMax, u32 flag, bool withScore) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 5;
    const s8* argv[max];
    u32 lens[max];
    argv[0] = (s8*)"ZREVRANGEBYSCORE";
    lens[0] = sizeof("ZREVRANGEBYSCORE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    if (0 != (flag & 2U)) {
        snprintf(buf, sizeof(buf), "-inf");
    } else if (0 != (flag & 1U)) {
        snprintf(buf, sizeof(buf), "(%f", iMax);
    } else {
        snprintf(buf, sizeof(buf), "%f", iMax);
    }
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    if (0 != (flag & 8U)) {
        snprintf(buf2, sizeof(buf2), "+inf");
    } else if (0 != (flag & 4U)) {
        snprintf(buf2, sizeof(buf2), "(%f", iMin);
    } else {
        snprintf(buf2, sizeof(buf2), "%f", iMin);
    }
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    u32 cnt = 4;
    if (withScore) {
        argv[4] = (s8*)"WITHSCORES";
        lens[4] = sizeof("WITHSCORES") - 1;
        ++cnt;
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zremrangebyscore(const s8* key, u32 keyLen, f32 iMin, f32 iMax, u32 flag) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"ZRANGEBYSCORE";
    lens[0] = sizeof("ZRANGEBYSCORE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    if (0 != (flag & 2U)) {
        snprintf(buf, sizeof(buf), "-inf");
    } else if (0 != (flag & 1U)) {
        snprintf(buf, sizeof(buf), "(%f", iMin);
    } else {
        snprintf(buf, sizeof(buf), "%f", iMin);
    }
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    if (0 != (flag & 8U)) {
        snprintf(buf2, sizeof(buf2), "+inf");
    } else if (0 != (flag & 4U)) {
        snprintf(buf2, sizeof(buf2), "(%f", iMax);
    } else {
        snprintf(buf2, sizeof(buf2), "%f", iMax);
    }
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zrangebylex(const s8* key, u32 keyLen,
    const s8* min, u32 minLen, const s8* max, u32 maxLen) {
    if (nullptr == key || 0 == keyLen
        || nullptr == min || 0 == minLen
        || nullptr == max || 0 == maxLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = (s8*)"ZRANGEBYLEX";
    lens[0] = sizeof("ZRANGEBYLEX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = min;
    lens[2] = minLen;

    argv[3] = max;
    lens[3] = maxLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zlexcount(const s8* key, u32 keyLen,
    const s8* min, u32 minLen, const s8* max, u32 maxLen) {
    if (nullptr == key || 0 == keyLen
        || nullptr == min || 0 == minLen
        || nullptr == max || 0 == maxLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = (s8*)"ZLEXCOUNT";
    lens[0] = sizeof("ZLEXCOUNT") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = min;
    lens[2] = minLen;

    argv[3] = max;
    lens[3] = maxLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zremrangebylex(const s8* key, u32 keyLen,
    const s8* min, u32 minLen, const s8* max, u32 maxLen) {
    if (nullptr == key || 0 == keyLen
        || nullptr == min || 0 == minLen
        || nullptr == max || 0 == maxLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = (s8*)"ZREMRANGEBYLEX";
    lens[0] = sizeof("ZREMRANGEBYLEX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = min;
    lens[2] = minLen;

    argv[3] = max;
    lens[3] = maxLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::zscan(const s8* key, u32 keyLen, u32 offset, const s8* pattern, u32 patLen, u32 count) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 7;
    const s8* argv[max];
    u32 lens[max];

    argv[0] = (s8*)"ZSCAN";
    lens[0] = sizeof("ZSCAN") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", offset);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    u32 cnt = 3;

    if (pattern && patLen > 0) {
        argv[2] = (s8*)"MATCH";
        lens[2] = sizeof("MATCH") - 1;

        argv[3] = pattern;
        lens[3] = patLen;

        cnt += 2;
    }

    if (count > 0) {
        argv[cnt] = (s8*)"COUNT";
        lens[cnt++] = sizeof("COUNT") - 1;

        s8 buf2[16];
        snprintf(buf2, sizeof(buf2), "%u", count);
        argv[cnt] = buf2;
        lens[cnt++] = (u32)strlen(buf2);
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}


bool RedisRequest::zunionstore(const s8* destkey, u32 destkeyLen,
    const s8** keys, const u32* keyLens,
    const s8** weight, const u32* weightLens,
    u32 count, s32 aggregate) {
    if (nullptr == destkey || 0 == destkeyLen || 0 == count
        || nullptr == keys || nullptr == keyLens
        || nullptr == weight || nullptr == weightLens) {
        return false;
    }

    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache];
    u32 tmp_size[maxcache];
    const u32 cnt = start + 2 * count + 1;
    s8** argv = cnt > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = cnt > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"ZUNIONSTORE";
    lens[0] = sizeof("ZUNIONSTORE") - 1;

    s8 num[16];
    argv[1] = num;
    lens[1] = snprintf(num, sizeof(num), "%u", count);

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }
    for (u32 i = 0; i < count; ++i) {
        argv[i + start + count] = const_cast<s8*>(weight[i]);
        lens[i + start + count] = weightLens[i];
    }
    argv[start + 2 * count] = (s8*)(0 == aggregate ? "SUM" : (aggregate > 0 ? "MAX" : "MIN"));
    lens[start + 2 * count] = 3;

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(destkey, destkeyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    if (cnt > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}


bool RedisRequest::zinterstore(const s8* destkey, u32 destkeyLen,
    const s8** keys, const u32* keyLens,
    const s8** weight, const u32* weightLens,
    u32 count, s32 aggregate) {
    if (nullptr == destkey || 0 == destkeyLen || 0 == count
        || nullptr == keys || nullptr == keyLens
        || nullptr == weight || nullptr == weightLens) {
        return false;
    }

    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache];
    u32 tmp_size[maxcache];
    const u32 cnt = start + 2 * count + 1;
    s8** argv = cnt > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = cnt > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"ZINTERSTORE";
    lens[0] = sizeof("ZINTERSTORE") - 1;

    s8 num[16];
    argv[1] = num;
    lens[1] = snprintf(num, sizeof(num), "%u", count);

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }
    for (u32 i = 0; i < count; ++i) {
        argv[i + start + count] = const_cast<s8*>(weight[i]);
        lens[i + start + count] = weightLens[i];
    }
    argv[start + 2 * count] = (s8*)(0 == aggregate ? "SUM" : (aggregate > 0 ? "MAX" : "MIN"));
    lens[start + 2 * count] = 3;

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(destkey, destkeyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    if (cnt > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

} //namespace net {
} // namespace app

