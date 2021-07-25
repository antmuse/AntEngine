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
#include <string>

namespace app {
namespace net {

bool RedisRequest::del(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return del(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::del(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "DEL";
    lens[0] = sizeof("DEL") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::unlink(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return unlink(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::unlink(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "UNLINK";
    lens[0] = sizeof("UNLINK") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::pttl(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return pttl(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::pttl(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "PTTL";
    lens[0] = sizeof("PTTL") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::ttl(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return ttl(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::ttl(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "TTL";
    lens[0] = sizeof("TTL") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::type(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return type(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::type(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "TYPE";
    lens[0] = sizeof("TYPE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::dump(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return dump(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::dump(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "DUMP";
    lens[0] = sizeof("DUMP") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}


bool RedisRequest::keys(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return keys(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::keys(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "KEYS";
    lens[0] = sizeof("KEYS") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::pexpire(const s8* key, u32 timeout) {
    if (nullptr == key) {
        return false;
    }
    return pexpire(key, static_cast<u32>(strlen(key)), timeout);
}

bool RedisRequest::pexpire(const s8* key, u32 keyLen, u32 timeout) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "PEXPIRE";
    lens[0] = sizeof("PEXPIRE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", timeout);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::move(const s8* key, u32 destDB) {
    if (nullptr == key) {
        return false;
    }
    return move(key, static_cast<u32>(strlen(key)), destDB);
}

bool RedisRequest::move(const s8* key, u32 keyLen, u32 destDB) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "MOVE";
    lens[0] = sizeof("MOVE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", destDB);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::persist(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return persist(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::persist(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "PERSIST";
    lens[0] = sizeof("PERSIST") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::objectRefcount(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return objectRefcount(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::objectRefcount(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "OBJECT";
    lens[0] = sizeof("OBJECT") - 1;

    argv[1] = "REFCOUNT";
    lens[1] = sizeof("REFCOUNT") - 1;

    argv[2] = key;
    lens[2] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::objectEncoding(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return objectEncoding(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::objectEncoding(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "OBJECT";
    lens[0] = sizeof("OBJECT") - 1;

    argv[1] = "ENCODING";
    lens[1] = sizeof("ENCODING") - 1;

    argv[2] = key;
    lens[2] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::objectIdletime(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return objectIdletime(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::objectIdletime(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "OBJECT";
    lens[0] = sizeof("OBJECT") - 1;

    argv[1] = "IDLETIME";
    lens[1] = sizeof("IDLETIME") - 1;

    argv[2] = key;
    lens[2] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::exists(const s8* key) {
    if (nullptr == key) {
        return false;
    }
    return exists(key, static_cast<u32>(strlen(key)));
}

bool RedisRequest::exists(const s8* key, u32 keyLen) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 2;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "EXISTS";
    lens[0] = sizeof("EXISTS") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::expire(const s8* key, u32 timeout) {
    if (nullptr == key) {
        return false;
    }
    return expire(key, static_cast<u32>(strlen(key)), timeout);
}

bool RedisRequest::expire(const s8* key, u32 keyLen, u32 timeout) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "EXPIRE";
    lens[0] = sizeof("EXPIRE") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", timeout);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::expireat(const s8* key, s64 timeout) {
    if (nullptr == key) {
        return false;
    }
    return expireat(key, static_cast<u32>(strlen(key)), timeout);
}

bool RedisRequest::expireat(const s8* key, u32 keyLen, s64 timeout) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "EXPIREAT";
    lens[0] = sizeof("EXPIREAT") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[24];
    snprintf(buf, sizeof(buf), "%lld", timeout);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::pexpireat(const s8* key, s64 timeout) {
    if (nullptr == key) {
        return false;
    }
    return pexpireat(key, static_cast<u32>(strlen(key)), timeout);
}

bool RedisRequest::pexpireat(const s8* key, u32 keyLen, s64 timeout) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "PEXPIREAT";
    lens[0] = sizeof("PEXPIREAT") - 1;

    argv[1] = key;
    lens[1] = keyLen;

    s8 buf[24];
    snprintf(buf, sizeof(buf), "%lld", timeout);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::randomkey() {
    const u32 cnt = 1;
    const s8* argv[cnt];

    u32 lens[cnt];
    argv[0] = "RANDOMKEY";
    lens[0] = sizeof("RANDOMKEY") - 1;

    return launch(cnt, argv, lens);
}

bool RedisRequest::rename(const s8* key1, const s8* key2) {
    if (nullptr == key1 || nullptr == key2) {
        return false;
    }
    return rename(key1, static_cast<u32>(strlen(key1)), key2, static_cast<u32>(strlen(key2)));
}

bool RedisRequest::rename(const s8* key1, u32 keyLen1, const s8* key2, u32 keyLen2) {
    if (nullptr == key1 || 0 == keyLen1 || nullptr == key2 || 0 == keyLen2) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];

    u32 lens[cnt];
    argv[0] = "RENAME";
    lens[0] = sizeof("RENAME") - 1;

    argv[1] = key1;
    lens[1] = keyLen1;

    argv[2] = key2;
    lens[2] = keyLen2;

    if (isClusterMode()) {
        return launch(hashSlot(key1, keyLen1), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::renamenx(const s8* key1, const s8* key2) {
    if (nullptr == key1 || nullptr == key2) {
        return false;
    }
    return renamenx(key1, static_cast<u32>(strlen(key1)), key2, static_cast<u32>(strlen(key2)));
}

bool RedisRequest::renamenx(const s8* key1, u32 keyLen1, const s8* key2, u32 keyLen2) {
    if (nullptr == key1 || 0 == keyLen1 || nullptr == key2 || 0 == keyLen2) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];

    u32 lens[cnt];
    argv[0] = "RENAMENX";
    lens[0] = sizeof("RENAMENX") - 1;

    argv[1] = key1;
    lens[1] = keyLen1;

    argv[2] = key2;
    lens[2] = keyLen2;

    if (isClusterMode()) {
        return launch(hashSlot(key1, keyLen1), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

bool RedisRequest::restore(const s8* key1, u32 keyLen1, const s8* key2, u32 keyLen2, s32 ttl, bool replace) {
    if (nullptr == key1 || 0 == keyLen1 || nullptr == key2 || 0 == keyLen2) {
        return false;
    }
    const u32 cnt = 5;
    const s8* argv[cnt];

    u32 lens[cnt];
    argv[0] = "RESTORE";
    lens[0] = sizeof("RESTORE") - 1;

    argv[1] = key1;
    lens[1] = keyLen1;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%d", ttl);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    argv[3] = key2;
    lens[3] = keyLen2;

    u32 argc = cnt - 1;
    if (replace) {
        ++argc;
        argv[4] = "REPLACE";
        lens[4] = sizeof("REPLACE") - 1;
    }

    if (isClusterMode()) {
        return launch(hashSlot(key1, keyLen1), argc, argv, lens);
    }
    return launch(argc, argv, lens);
}

bool RedisRequest::scan(u32 offset, const s8* pattern, u32 patLen, u32 count) {
    const u32 max = 6;
    const s8* argv[max];
    u32 lens[max];

    argv[0] = "SCAN";
    lens[0] = sizeof("SCAN") - 1;

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", offset);
    argv[1] = buf;
    lens[1] = (u32)strlen(buf);

    u32 cnt = 2;

    if (pattern && patLen > 0) {
        argv[2] = "MATCH";
        lens[2] = sizeof("MATCH") - 1;

        argv[3] = pattern;
        lens[3] = patLen;

        cnt += 2;
    }

    s8 buf2[16];
    if (count > 0) {
        argv[cnt] = "COUNT";
        lens[cnt++] = sizeof("COUNT") - 1;

        snprintf(buf2, sizeof(buf2), "%u", count);
        argv[cnt] = buf2;
        lens[cnt++] = (u32)strlen(buf2);
    }

    return launch(cnt, argv, lens);
}

bool RedisRequest::migrate(const s8* ip, u16 port, const s8* key, u32 keyLen,
    u32 destDB, u32 timeout, s8 flag) {
    if (nullptr == ip || nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 max = 7;
    const s8* argv[max];
    u32 lens[max];

    argv[0] = "MIGRATE";
    lens[0] = sizeof("MIGRATE") - 1;

    argv[1] = ip;
    lens[1] = (u32)strlen(ip);

    s8 buf[16];
    snprintf(buf, sizeof(buf), "%u", port);
    argv[2] = buf;
    lens[2] = (u32)strlen(buf);

    argv[3] = key;
    lens[3] = keyLen;

    s8 buf2[16];
    snprintf(buf2, sizeof(buf2), "%u", destDB);
    argv[4] = buf2;
    lens[4] = (u32)strlen(buf2);

    s8 buf3[16];
    snprintf(buf3, sizeof(buf3), "%u", timeout);
    argv[5] = buf3;
    lens[5] = (u32)strlen(buf3);

    u32 cnt = 6;
    if ('c' == flag) {
        argv[6] = "COPY";
        lens[6] = sizeof("COPY") - 1;
        ++cnt;
    } else if ('r' == flag) {
        argv[6] = "REPLACE";
        lens[6] = sizeof("REPLACE") - 1;
        ++cnt;
    }

    if (isClusterMode()) {
        return launch(hashSlot(key, keyLen), cnt, argv, lens);
    }
    return launch(cnt, argv, lens);
}

} //namespace net {
} // namespace app

