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


bool RedisRequest::auth(const s8* passowrd) {
    if (nullptr == passowrd) {
        return false;
    }
    const s8* argv[2];
    argv[0] = "AUTH";
    argv[1] = passowrd;
    u32 lens[2];
    lens[0] = 4;
    lens[1] = static_cast<u32>(strlen(passowrd));
    return launch(2, argv, lens);
}

bool RedisRequest::echo(const s8* msg) {
    if (nullptr == msg) {
        return false;
    }
    const s8* argv[2];
    argv[0] = "ECHO";
    argv[1] = msg;
    u32 lens[2];
    lens[0] = 4;
    lens[1] = static_cast<u32>(strlen(msg));
    return launch(2, argv, lens);
}

bool RedisRequest::select(s32 dbID) {
    s8 dbstr[32];
    snprintf(dbstr, sizeof(dbstr), "%d", dbID);
    const s8* argv[2];
    argv[0] = "SELECT";
    argv[1] = dbstr;
    u32 lens[2];
    lens[0] = 6;
    lens[1] = static_cast<u32>(strlen(dbstr));
    return launch(2, argv, lens);
}

bool RedisRequest::ping() {
    const s8* argv[1];
    argv[0] = "PING";
    u32 lens[1];
    lens[0] = 4;
    return launch(1, argv, lens);
}

bool RedisRequest::quit() {
    const s8* argv[1];
    argv[0] = "QUIT";
    u32 lens[1];
    lens[0] = 4;
    return launch(1, argv, lens);
}


bool RedisRequest::shutdown() {
    const s8* argv[1];
    argv[0] = "SHUTDOWN";
    u32 lens[1];
    lens[0] = 8;
    return launch(1, argv, lens);
}

bool RedisRequest::save() {
    const u32 cnt = 1;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "SAVE";
    lens[0] = sizeof("SAVE") - 1;
    return launch(cnt, argv, lens);
}


bool RedisRequest::bgsave() {
    const u32 cnt = 1;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "BGSAVE";
    lens[0] = sizeof("BGSAVE") - 1;
    return launch(cnt, argv, lens);
}


bool RedisRequest::bgrewriteaof() {
    const u32 cnt = 1;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "BGREWRITEAOF";
    lens[0] = sizeof("BGREWRITEAOF") - 1;
    return launch(cnt, argv, lens);
}


bool RedisRequest::lastsave() {
    const u32 cnt = 1;
    const s8* argv[cnt];
    u32 lens[cnt];
    argv[0] = "LASTSAVE";
    lens[0] = sizeof("LASTSAVE") - 1;
    return launch(cnt, argv, lens);
}


} //namespace net {
} // namespace app

