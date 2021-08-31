#include "db/Database.h"

namespace app {
namespace db {

static void AppThreadInit() {
    mysql_thread_init();
}

static void AppThreadUninit() {
    mysql_thread_end();
}

Database::Database(ConnectConfig dbinfo) : mPool(std::move(dbinfo)) {
}

Database::~Database() {
    stop();
}

void Database::start(u32 pmax) {
    mThreads.setThreadCalls(AppThreadInit, AppThreadUninit);
    mThreads.start(pmax);
}


bool Database::postTask(Task task, u32 timeout, FuncDBTask funcFinish) {
    Connector* con = mPool.pop(std::move(task), timeout, funcFinish);
    if (con) {
        if (mThreads.postTask(&Database::executor, this, con)) {
            return true;
        }
        mPool.push(con);
    }
    return false;
}


//thread callback
void Database::executor(Connector* con) {
    con->execute();
    auto funcFinish = con->mFuncFinish;
    funcFinish(con); //callback user's func
    mPool.push(con);
}


} //namespace db
} //namespace app
