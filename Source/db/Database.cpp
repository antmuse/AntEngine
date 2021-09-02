#include "db/Database.h"

namespace app {
namespace db {

static void AppThreadInit() {
    mysql_thread_init();
}

static void AppThreadUninit() {
    mysql_thread_end();
}

Database::Database(ConnectConfig dbinfo)
    : mPool(std::move(dbinfo)) {
}

Database::~Database() {
    stop();
}


void Database::start(u32 pmax) {
    mThreads.setMaxAllocated(1000);
    mThreads.setThreadCalls(AppThreadInit, AppThreadUninit);
    mThreads.start(pmax);
}


bool Database::postTask(Task* task) {
    if (!task || !task->mFuncFinish) {
        return false;
    }
    return mThreads.postTask(&Database::executor, this, task);
}


//thread callback
void Database::executor(Task* task) {
    Connector* con = mPool.pop();
    con->execute(task);
    task->mFuncFinish(task, con); //callback user's func
    mPool.push(con);
}


} //namespace db
} //namespace app
