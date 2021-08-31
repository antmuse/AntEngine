#include "db/ConnectorPool.h"
#include "Node.h"
#include <thread>

namespace app {
namespace db {

ConnectorPool::ConnectorPool(ConnectConfig connection)
    : mConInfo(std::move(connection))
    , mBusyCount(0)
    , mIdleCount(0)
    , mConQueue(nullptr)
    , mRunning(true) {
}


ConnectorPool::~ConnectorPool() {
    close();
}


Connector* ConnectorPool::pop(Task task, u32 timeout, FuncDBTask on_complete) {
    std::lock_guard<std::mutex> ak(mMutex);

    if (!mRunning) {
        return nullptr;
    }

    ++mBusyCount;
    Connector* con = AppPopRingQueueHead_1(mConQueue);
    if (con) {
        --mIdleCount;
        con->mTask = std::move(task);
        con->mTimeout = timeout;
        con->mFuncFinish = on_complete;
    } else {
        if (mBusyCount > 20) {
            std::chrono::milliseconds gap(100);
            std::this_thread::sleep_for(gap);
        }
        con = new Connector(*this, mConInfo, on_complete, timeout, std::move(task));
    }
    return con;
}


void ConnectorPool::push(Connector* con) {
    if (!con) {
        return;
    }

    if (con->isError()) {
        delete con;
        --mBusyCount;
        return;
    }

    ++mIdleCount;
    con->reset();
    {
        std::lock_guard<std::mutex> ak(mMutex);
        AppPushRingQueueTail_1(mConQueue, con);
    }
    --mBusyCount;
}


usz ConnectorPool::close() {
    {
        std::lock_guard<std::mutex> ak1(mMutex);
        if (!mRunning) {
            return 0;
        }
        mRunning = false;
    }

    //wait...
    std::chrono::milliseconds gap(10);
    while (mBusyCount.load() > 0) {
        std::this_thread::sleep_for(gap);
    }

    usz ret = 0;
    {
        std::lock_guard<std::mutex> ak2(mMutex);
        for (Connector* i = AppPopRingQueueHead_1(mConQueue);
            i; i = AppPopRingQueueHead_1(mConQueue)) {
            delete i;
            ++ret;
        }
    }
    DASSERT(ret == mIdleCount.load());
    mIdleCount = 0;
    return ret;
}


} //namespace db
} //namespace app
