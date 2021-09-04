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

void DBCallBack(db::Task* task, db::Connector* it) {
    delete task;

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
                (s32)(it->getFinalRequest().length() > 32 ? 32 : it->getFinalRequest().length()),
                it->getFinalRequest().c_str());
        }
    } else {
        printf("idx=%d, task=[%s], error=%s\n", cnt,
            it->getFinalRequest().c_str(), it->getError().c_str());
    }
}


void AppMySQLtest(s32 argc, s8** argv) {
    s32 posted = 0;
    static const std::string TEST_DB = "temp_db";
    s8* curr = argv[2];
    while (*curr != ':' && *curr != '\0') {
        ++curr;
    }
    *curr = 0;
    const u16 port = (u16)App10StrToU32(++curr);

    std::vector<std::string> table_names{
        //"User", "Task"
    };

    db::ConnectConfig connection{argv[2], port, argv[3], argv[4]};
    db::Database executor(std::move(connection));
    executor.start(6);

    db::Task* task = new db::Task(DBCallBack, 10);
    *task << "CREATE DATABASE IF NOT EXISTS " << TEST_DB;

    if (executor.postTask(task)) {
        ++posted;
    } else {
        delete task;
    }

    for (const auto& name : table_names) {
        task = new db::Task(DBCallBack, 10);
        *task << "DROP TABLE IF EXISTS " << TEST_DB << "." << name;
        if (executor.postTask(task)) {
            ++posted;
        } else {
            delete task;
        }
    }

    task = new db::Task(DBCallBack, 10);

    *task << "CREATE TABLE IF NOT EXISTS " << TEST_DB << ".User (\n"
        << "id INT UNSIGNED NOT NULL AUTO_INCREMENT, \n"
        << "mobile BIGINT, \n"
        << "name VARCHAR(128), \n"
        << "PRIMARY KEY (id)\n"
        << ")";

    if (executor.postTask(task)) {
        ++posted;
    } else {
        delete task;
    }


    s8 tmp[128];
    for (s32 i = 0; i < 1000; ++i) {
        task = new db::Task(DBCallBack, 10);
        //snprintf(tmp, sizeof(tmp), "(%llu,\"myname%d\")", 18023030000 + i, i);
        snprintf(tmp, sizeof(tmp), "{\"json\":%d}", i);
        db::Task::Arg arg(tmp); //转义时需用Arg
        *task
            << "INSERT INTO " << TEST_DB
            << ".User (mobile,name) values ("
            << 18023030000LL + i
            << ",\""
            << arg
            << "\")";
        if (executor.postTask(task)) {
            ++posted;
        } else {
            delete task;
        }
    }

    task = new db::Task(DBCallBack, 10);
    *task << "SELECT * FROM " << TEST_DB << ".User order by id desc LIMIT 5";
    if (executor.postTask(task)) {
        ++posted;
    } else {
        delete task;
    }

    task = new db::Task(DBCallBack, 10);
    *task << "DELETE FROM " << TEST_DB << ".User where id>12";
    if (executor.postTask(task)) {
        ++posted;
    } else {
        delete task;
    }

    printf("before stop>>posted=%d, land=%d\n", posted, gCOUNT.load());
    executor.stop();
    printf("stoped>>posted=%d, land=%d\n", posted, gCOUNT.load());
}


s32 AppTestDBClient(s32 argc, s8** argv) {
    Engine& eng = Engine::getInstance();
    Loop& loop = eng.getLoop();
    AppTicker tick(loop);
    s32 succ = tick.start();
    DASSERT(0 == succ);


    s32 post = 0;
    while (loop.run()) {
        if (post < 1) {
            AppMySQLtest(argc, argv);
            post = 1;
        }
    }

    Logger::log(ELL_INFO, "AppTestDBClient>>stop");
    return 0;
}

} //namespace app