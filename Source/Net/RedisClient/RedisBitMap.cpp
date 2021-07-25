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

bool RedisRequest::setbit(const s8* key, u32 keyLen, u32 offset, bool val) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "SETBIT";
    lens[0] = sizeof("SETBIT") - 1;
    argv[1] = key;
    lens[1] = keyLen;
    s8 offsetstr[16];
    argv[2] = offsetstr;
    lens[2] = snprintf(offsetstr, sizeof(offsetstr), "%u", offset);
    argv[3] = val ? "1" : "0";
    lens[3] = 1;

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(key, keyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    return ret;
}

bool RedisRequest::getbit(const s8* key, u32 keyLen, u32 offset) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 3;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "GETBIT";
    lens[0] = sizeof("GETBIT") - 1;
    argv[1] = key;
    lens[1] = keyLen;
    s8 offsetstr[16];
    argv[2] = offsetstr;
    lens[2] = snprintf(offsetstr, sizeof(offsetstr), "%u", offset);

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(key, keyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    return ret;
}


bool RedisRequest::bitcount(const s8* key, u32 keyLen, u32 min, u32 max) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 4;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "BITCOUNT";
    lens[0] = sizeof("BITCOUNT") - 1;
    argv[1] = key;
    lens[1] = keyLen;
    s8 minstr[16];
    s8 maxstr[16];
    argv[2] = minstr;
    lens[2] = snprintf(minstr, sizeof(minstr), "%u", min);
    argv[3] = maxstr;
    lens[3] = snprintf(maxstr, sizeof(maxstr), "%u", max);

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(key, keyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    return ret;
}


bool RedisRequest::bitpos(const s8* key, u32 keyLen, bool val, u32 min, u32 max) {
    if (nullptr == key || 0 == keyLen) {
        return false;
    }
    const u32 cnt = 5;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "BITPOS";
    lens[0] = sizeof("BITPOS") - 1;
    argv[1] = key;
    lens[1] = keyLen;
    argv[2] = val ? "1" : "0";
    lens[2] = 1;
    s8 minstr[16];
    s8 maxstr[16];
    argv[3] = minstr;
    lens[3] = snprintf(minstr, sizeof(minstr), "%u", min);
    argv[4] = maxstr;
    lens[4] = snprintf(maxstr, sizeof(maxstr), "%u", max);

    bool ret;
    if (isClusterMode()) {
        ret = launch(hashSlot(key, keyLen), cnt, const_cast<const s8**>(argv), lens);
    } else {
        ret = launch(cnt, const_cast<const s8**>(argv), lens);
    }
    return ret;
}


bool RedisRequest::bitop(const s8* key, u32 keyLen, u32 flag,
    const s8** val, const u32* valLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLens
        || 0 == count) {
        return false;
    }
    const u32 start = 3;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"BITOP";
    lens[0] = sizeof("BITOP") - 1;
    switch (flag) {
    default:
    case 0:
        argv[1] = (s8*)"AND";
        lens[1] = sizeof("AND") - 1;
        break;
    case 1:
        argv[1] = (s8*)"OR";
        lens[1] = sizeof("OR") - 1;
        break;
    case 2:
        argv[1] = (s8*)"NOT";
        lens[1] = sizeof("NOT") - 1;
        break;
    case 3:
        argv[1] = (s8*)"XOR";
        lens[1] = sizeof("XOR") - 1;
        break;
    }
    argv[2] = const_cast<s8*>(key);
    lens[2] = keyLen;
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


bool RedisRequest::bitfiled(const s8* key, u32 keyLen,
    const s8** val, const u32* valLens, u32 count) {
    if (nullptr == key || 0 == keyLen || nullptr == val || 0 == valLens
        || 0 == count) {
        return false;
    }
    const u32 start = 2;
    const u32 maxcache = 128;
    s8* tmp_argv[maxcache + start];
    u32 tmp_size[maxcache + start];
    const u32 cnt = start + count;
    s8** argv = count > maxcache ? new s8*[cnt] : tmp_argv;
    u32* lens = count > maxcache ? new u32[cnt] : tmp_size;

    argv[0] = (s8*)"BITFIELD";
    lens[0] = sizeof("BITFIELD") - 1;
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

} //namespace net {
} // namespace app
