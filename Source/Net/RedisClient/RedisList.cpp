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

bool RedisRequest::llen(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "LLEN";
    lens[0] = sizeof("LLEN") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lset(const s8* key, u32 keyLen, u32 pos, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LSET";
    lens[0] = sizeof("LSET") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", pos);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    argv[3] = val;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::linsert(const s8* key, u32 keyLen, const s8* pivot, u32 pivotLen,
    const s8* val, u32 valLen, bool after) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen || nullptr == pivot || 0 == pivotLen) {
        return false;
    }
    const u32 cnt = 5;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LINSERT";
    lens[0] = sizeof("LINSERT") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (after) {
        argv[2] = "AFTER";
        lens[2] = sizeof("AFTER") - 1;
    } else {
        argv[2] = "BEFORE";
        lens[2] = sizeof("BEFORE") - 1;
    }

    argv[3] = pivot;
    lens[3] = pivotLen;

    argv[4] = val;
    lens[4] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lindex(const s8* key, u32 keyLen, u32 pos) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LINDEX";
    lens[0] = sizeof("LINDEX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", pos);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lrem(const s8* key, u32 keyLen, s32 pos, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LREM";
    lens[0] = sizeof("LREM") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", pos);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    argv[3] = val;
    lens[3] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lpush(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LPUSH";
    lens[0] = sizeof("LPUSH") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::rpush(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "RPUSH";
    lens[0] = sizeof("RPUSH") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lpushx(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LPUSHX";
    lens[0] = sizeof("LPUSHX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::rpushx(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "RPUSHX";
    lens[0] = sizeof("RPUSHX") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::rpoplpush(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "RPOPLPUSH";
    lens[0] = sizeof("RPOPLPUSH") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::brpoplpush(const s8* key, u32 keyLen, const s8* val, u32 valLen) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "BRPOPLPUSH";
    lens[0] = sizeof("BRPOPLPUSH") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    argv[2] = val;
    lens[2] = valLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lrange(const s8* key, u32 keyLen, s32 start, s32 end) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LRANGE";
    lens[0] = sizeof("LRANGE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", start);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", end);
    argv[3] = buf2;
    lens[3] = (u32)strlen(buf2);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::ltrim(const s8* key, u32 keyLen, s32 start, s32 end) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LTRIM";
    lens[0] = sizeof("LTRIM") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", start);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%d", end);
    argv[3] = buf2;
    lens[3] = (u32)strlen(buf2);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::rpop(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "RPOP";
    lens[0] = sizeof("RPOP") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::lpop(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "LPOP";
    lens[0] = sizeof("LPOP") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::brpop(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "BRPOP";
    lens[0] = sizeof("BRPOP") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::blpop(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];

    argv[0] = "BLPOP";
    lens[0] = sizeof("BLPOP") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

} //namespace net {
} // namespace app

