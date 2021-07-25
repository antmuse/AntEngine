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

bool RedisRequest::sdiff(const s8** keys, const u32* keyLens, u32 count) {
    if (nullptr == keys || 0 == keyLens || count < 2) {
        return false;
    }
    const u32 start = 1;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"SDIFF";
    lens[0] = sizeof("SDIFF") - 1;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::sdiffstore(const s8* key, u32 keyLen, const s8** keys, const u32* keyLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == keys || 0 == keyLens || count < 2) {
        return false;
    }
    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"SDIFFSTORE";
    lens[0] = sizeof("SDIFFSTORE") - 1;

    argv[1] = const_cast<s8*>(key);
    lens[1] = keyLen;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::sinterstore(const s8* key, u32 keyLen, const s8** keys, const u32* keyLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == keys || 0 == keyLens || count < 2) {
        return false;
    }
    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"SINTERSTORE";
    lens[0] = sizeof("SINTERSTORE") - 1;

    argv[1] = const_cast<s8*>(key);
    lens[1] = keyLen;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::sinter(const s8** keys, const u32* keyLens, u32 count) {
    if (nullptr == keys || 0 == keyLens || count < 2) {
        return false;
    }
    const u32 start = 1;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"SINTER";
    lens[0] = sizeof("SINTER") - 1;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::sunion(const s8** keys, const u32* keyLens, u32 count) {
    if (nullptr == keys || 0 == keyLens || count < 2) {
        return false;
    }
    const u32 start = 1;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"SUNION";
    lens[0] = sizeof("SUNION") - 1;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::sunionstore(const s8* key, u32 keyLen, const s8** keys, const u32* keyLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == keys || 0 == keyLens || count < 2) {
        return false;
    }
    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"SUNIONSTORE";
    lens[0] = sizeof("SUNIONSTORE") - 1;

    argv[1] = const_cast<s8*>(key);
    lens[1] = keyLen;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(keys[i]);
        lens[i + start] = keyLens[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::sadd(const s8* key, u32 keyLen, const s8* val, const u32 valLens) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLens) {
        return false;
    }
    return sadd(key, keyLen, &val, &valLens, 1);
}

bool RedisRequest::sadd(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count) {
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

    argv[0] = (s8*)"SADD";
    lens[0] = sizeof("SADD") - 1;

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

bool RedisRequest::srem(const s8* key, u32 keyLen, const s8* val, const u32 valLens) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLens) {
        return false;
    }
    return srem(key, keyLen, &val, &valLens, 1);
}

bool RedisRequest::srem(const s8* key, u32 keyLen, const s8** val, const u32* valLens, u32 count) {
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

    argv[0] = (s8*)"SREM";
    lens[0] = sizeof("SREM") - 1;

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

bool RedisRequest::sismember(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"SISMEMBER";
    lens[0] = sizeof("SISMEMBER") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::spop(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"SPOP";
    lens[0] = sizeof("SPOP") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::scard(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"SCARD";
    lens[0] = sizeof("SCARD") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::smembers(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"SMEMBERS";
    lens[0] = sizeof("SMEMBERS") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::srandmember(const s8* key, u32 keyLen, u32 count) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 3;
    const s8* argv[max];
    u32 lens[max];
    argv[0] = (s8*)"SRANDMEMBER";
    lens[0] = sizeof("SRANDMEMBER") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    u32 cnt = 2;
    s8 buf[16];
    if (count > 0) {
        snprintf(buf, sizeof(buf), "%u", count);
        argv[2] = buf;
        lens[2] = (u32)::strlen(buf);
        ++cnt;
    }
    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::smove(const s8* key1, u32 keyLen1,
    const s8* key2, u32 keyLen2, const s8* val, const u32 valLen) {
    if (!key1 || 0 == keyLen1 || !key2 || 0 == keyLen2 || !val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = (s8*)"SMOVE";
    lens[0] = sizeof("SMOVE") - 1;

    argv[1] = key1;
    lens[1] = keyLen1;

    argv[2] = key2;
    lens[2] = keyLen2;

    argv[3] = val;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key1, keyLen1), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::sscan(const s8* key, u32 keyLen, u32 offset, const s8* pattern, u32 patLen, u32 count) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 7;
    const s8* argv[max];
    u32 lens[max];

    argv[0] = (s8*)"SSCAN";
    lens[0] = sizeof("SSCAN") - 1;

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

    s8 buf2[16];
    if (count > 0) {
        argv[cnt] = (s8*)"COUNT";
        lens[cnt++] = sizeof("COUNT") - 1;

        snprintf(buf2, sizeof(buf2), "%u", count);
        argv[cnt] = buf2;
        lens[cnt++] = (u32)strlen(buf2);
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

} //namespace net {
} // namespace app

