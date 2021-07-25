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

bool RedisRequest::set(const s8* key, const s8* value) {
    if (nullptr == key || nullptr == value) {
        return false;
    }
    return set(key, static_cast<u32>(::strlen(key)),
        value, static_cast<u32>(::strlen(value)));
}

bool RedisRequest::set(const s8* key, u32 keyLen,
    const s8* value, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const s8* argv[3];
    u32 lens[3];
    argv[0] = "SET";
    lens[0] = 3;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = value;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), 3, argv, lens);
    }
    return launch(3, argv, lens);
}

bool RedisRequest::getset(const s8* key, u32 keyLen,
    const s8* value, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "GETSET";
    lens[0] = sizeof("GETSET") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = value;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::setnx(const s8* key, u32 keyLen,
    const s8* value, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const s8* argv[3];
    u32 lens[3];
    argv[0] = "SETNX";
    lens[0] = sizeof("SETNX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = value;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), 3, argv, lens);
    }
    return launch(3, argv, lens);
}

bool RedisRequest::get(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return get(key, static_cast<u32>(::strlen(key)));
}

bool RedisRequest::get(const s8* key, u32 len) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "GET";
    lens[0] = sizeof("GET") - 1;

    argv[1] = key;
    lens[1] = len;

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::incr(const s8* key, u32 len) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "INCR";
    lens[0] = sizeof("INCR") - 1;

    argv[1] = key;
    lens[1] = len;

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::incrby(const s8* key, u32 len, s32 addon) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "INCRBY";
    lens[0] = sizeof("INCRBY") - 1;

    argv[1] = key;
    lens[1] = len;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", addon);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::decr(const s8* key, u32 len) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "DECR";
    lens[0] = sizeof("DECR") - 1;

    argv[1] = key;
    lens[1] = len;

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::decrby(const s8* key, u32 len, s32 addon) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "DECRBY";
    lens[0] = sizeof("DECRBY") - 1;

    argv[1] = key;
    lens[1] = len;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", addon);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::incrbyfloat(const s8* key, u32 len, f64 addon) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "INCRBYFLOAT";
    lens[0] = sizeof("INCRBYFLOAT") - 1;

    argv[1] = key;
    lens[1] = len;

    s8 buf[64];
    snprintf(buf, sizeof(buf), "%lf", addon);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::stringlen(const s8* key, u32 len) {
    if (nullptr == key || 0 == len) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "STRLEN";
    lens[0] = sizeof("STRLEN") - 1;

    argv[1] = key;
    lens[1] = len;

    if (isClusterMode()) {
        return launch(hashSlot(key), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::setex(const s8* key, u32 keyLen, const s8* value, u32 valLen, u32 timeout) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "SETEX";
    lens[0] = sizeof("SETEX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", timeout);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    argv[3] = value;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::psetex(const s8* key, u32 keyLen, const s8* value, u32 valLen, u32 timeout) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "PSETEX";
    lens[0] = sizeof("PSETEX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", timeout);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    argv[3] = value;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::append(const s8* key, u32 keyLen,
    const s8* value, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "APPEND";
    lens[0] = sizeof("APPEND") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = value;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}


bool RedisRequest::setrange(const s8* key, u32 keyLen, const s8* value, u32 valLen, u32 offset) {
    if (nullptr == key || 0 == keyLen || nullptr == value || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "SETRANGE";
    lens[0] = sizeof("SETRANGE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", offset);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    argv[3] = value;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}


bool RedisRequest::getrange(const s8* key, u32 keyLen, s32 start, s32 end) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "GETRANGE";
    lens[0] = sizeof("GETRANGE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", start);
    argv[2] = buf;
    lens[2] = (u32)::strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", end);
    argv[3] = buf2;
    lens[3] = (u32)::strlen(buf2);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::mset(const s8** key, const u32* keyLen, u32 count) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 start = 1;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"MSET";
    lens[0] = sizeof("MSET") - 1;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(key[i]);
        lens[i + start] = keyLen[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::mget(const s8** key, const u32* keyLen, u32 count) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }

    const u32 start = 1;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"MGET";
    lens[0] = sizeof("MGET") - 1;

    for (u32 i = 0; i < count; ++i) {
        argv[i + start] = const_cast<s8*>(key[i]);
        lens[i + start] = keyLen[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    if (count > maxcache) {
        delete[] argv;
        delete[] lens;
    }
    return ret;
}

bool RedisRequest::msetnx(const s8** key, const u32* keyLen, u32 count) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 1 + count;
    s8** argv = new s8*[cnt];
    u32* lens = new u32[cnt];

    argv[0] = (s8*)"MSETNX";
    lens[0] = sizeof("MSETNX") - 1;

    for (u32 i = 1; i <= count; ++i) {
        argv[i] = const_cast<s8*>(key[i]);
        lens[i] = keyLen[i];
    }

    bool ret = launch(cnt, const_cast<const s8**>(argv), lens);
    delete[] argv;
    delete[] lens;
    return ret;
}

} //namespace net {
} // namespace app

