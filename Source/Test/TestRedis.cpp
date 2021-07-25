#include "Config.h"
#include <string.h>
#include <stdio.h>
#include <memory>
#include "Engine.h"
#include "Loop.h"
#include "Converter.h"
#include "StrConverter.h"
#include "Net/RedisClient/RedisRequest.h"
#include "Net/RedisClient/RedisResponse.h"
#include "Net/RedisClient/RedisClientCluster.h"

namespace app {
static const s32 G_TOTAL = 1;
static s32 LAND_TOTAL = 0;
static s32 LAND_OK = 0;

void AppRedisCallback(net::RedisRequest* it, net::RedisResponse* res) {
    net::RedisClientCluster* cls = it->getCluster();
    //RedisClient* nd = it->getClient();
    res->show();
    if (res->isOK()) {
        ++LAND_OK;
    }
    if (++LAND_TOTAL == G_TOTAL) {
        cls->close();
        //Engine::getInstance().getLoop().stop();
    }
}

s32 AppTestRedis(s32 argc, s8** argv) {
    printf("AppTestRedis>>start\n");
    net::RedisClientCluster* cls = new net::RedisClientCluster();
    cls->setMaxTcp(10);
    cls->setPassword("123456");
    cls->open("10.1.63.128:5000");

    Loop& loop = Engine::getInstance().getLoop();
    s8 kkk[128];
    s8 vvv[128];
    s32 kid = 0;

    while (loop.run()) {
        if (kid < G_TOTAL) {
            net::RedisRequest* req = new net::RedisRequest();
            snprintf(kkk, sizeof(kkk), "kkk%d", kid);
            snprintf(vvv, sizeof(vvv), "vvv%d", kid);
            req->setCallback(AppRedisCallback);
            req->setCluster(cls);
            if (2 == argc) {
                if (req->set(kkk, vvv)) {
                    ++kid;
                }
            } else {
                if (req->del(kkk)) {
                    ++kid;
                }
            }
            req->drop();
        }
    }

    loop.stop();
    delete cls;

    Logger::log(ELL_INFO, "AppTestRedis>>Land=%d/%d,Total=%d", LAND_OK, LAND_TOTAL, G_TOTAL);
    return 0;
}

} //namespace app
