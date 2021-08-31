#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include "Engine.h"
#include "AppTicker.h"
#include "Timer.h"
#include "db/Database.h"


namespace app {
static std::atomic<s32> gCOUNT(0);

void DBCallBack(db::Connector* it) {
    s32 cnt = ++gCOUNT;

    EErrorCode stat = it->getStatus();
    if (EE_OK == stat) {
        if (it->getFieldCount() > 0) { //select
            for (usz i = 0; i < it->getRowCount(); ++i) {
                const db::Row& row = it->getRow(i);
                printf("id=%d, mob=%lld, name=%s\n",
                    row.getColumn(0).asInt32(),
                    row.getColumn(1).asInt64(),
                    row.getColumn(2).asStrView().mData);
            }
        } else { //insert/del
            printf("idx=%d, affected rows=%llu, last idx=%llu, %.*s\n",
                cnt, it->getRowCount(), it->getLastInsertID(),
                (s32)(it->getFinalRequest().length() > 16 ? 16 : it->getFinalRequest().length()),
                it->getFinalRequest().c_str());
        }
    } else {
        printf("idx=%d, task=[%s], error=%s\n", cnt,
            it->getFinalRequest().c_str(), it->getError().c_str());
    }
}


void AppMySQLtest() {
    s32 posted = 0;
    static const std::string TEST_DB_ADDR = "127.0.0.1";
    static const std::string TEST_DB_USER = "root";
    static const std::string TEST_DB_PWD = "witcore.cn";
    static const u16 TEST_DB_PORT = 5000;
    static const std::string TEST_DB = "temp_db";

    std::vector<std::string> table_names{
        //"User", "Task"
    };

    db::ConnectConfig connection{TEST_DB_ADDR, TEST_DB_PORT, TEST_DB_USER, TEST_DB_PWD};
    db::Database executor(std::move(connection));
    executor.start(6);

    db::Task create_db_stm;
    create_db_stm << "CREATE DATABASE IF NOT EXISTS " << TEST_DB;

    if (executor.postTask(std::move(create_db_stm), 10, DBCallBack)) {
        ++posted;
    }

    for (const auto& name : table_names) {
        db::Task drop_table_stm;
        drop_table_stm << "DROP TABLE IF EXISTS " << TEST_DB << "." << name;
        if (executor.postTask(std::move(drop_table_stm), 10, DBCallBack)) {
            ++posted;
        }
    }

    db::Task crt_user;

    crt_user
        << "CREATE TABLE IF NOT EXISTS " << TEST_DB << ".User (\n"
        << "id INT UNSIGNED NOT NULL AUTO_INCREMENT, \n"
        << "mobile BIGINT, \n"
        << "name VARCHAR(128), \n"
        << "PRIMARY KEY (id)\n"
        << ")";

    if (executor.postTask(std::move(crt_user), 10, DBCallBack)) {
        ++posted;
    }


    db::Task crt_task;
    s8 tmp[128];
    for (s32 i = 0; i < 1000; ++i) {
        //snprintf(tmp, sizeof(tmp), "(%llu,\"myname%d\")", 18023030000 + i, i);
        snprintf(tmp, sizeof(tmp), "{\"json\":%d}", i);
        db::Task::Arg arg(tmp); //转义时需用Arg
        crt_task
            << "INSERT INTO " << TEST_DB
            << ".User (mobile,name) values ("
            << 18023030000LL + i
            << ",\""
            << arg
            << "\")";
        if (executor.postTask(std::move(crt_task), 10, DBCallBack)) {
            ++posted;
        }
    }

    crt_task << "SELECT * FROM " << TEST_DB << ".User order by id desc LIMIT 5";
    if (executor.postTask(std::move(crt_task), 10, DBCallBack)) {
        ++posted;
    }

    crt_task << "DELETE FROM " << TEST_DB << ".User where id>12";
    if (executor.postTask(std::move(crt_task), 10, DBCallBack)) {
        ++posted;
    }

    printf("before stop>>posted=%d, land=%d\n", posted, gCOUNT.load());
    executor.stop();
    printf("stoped>>posted=%d, land=%d\n", posted, gCOUNT.load());
}


s32 AppTestDBClient(s32 argc, s8** argv) {
    const char* serveraddr = argc > 2 ? argv[2] : "127.0.0.1:9981";

    Logger::log(ELL_INFO, "AppTestDBClient>>connect server=%s", serveraddr);

    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);


    s32 post = 0;
    while (loop.run()) {
        if (post < 1) {
            AppMySQLtest();
            post = 1;
        }
    }

    Logger::log(ELL_INFO, "AppTestDBClient>>stop");
    return 0;
}

} //namespace app