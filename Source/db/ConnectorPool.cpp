#include "db/ConnectorPool.h"
#include "Node.h"
#include <thread>

namespace app {
namespace db {

ConnectorPool::ConnectorPool(ConnectConfig connection)
    : mConInfo(std::move(connection))
    , mConQueue(nullptr) {
}


ConnectorPool::~ConnectorPool() {
    close();
}


Connector* ConnectorPool::pop() {
    std::lock_guard<std::mutex> ak(mMutex);

    Connector* con = AppPopRingQueueHead_1(mConQueue);
    if (!con) {
        con = new Connector(*this, mConInfo);
    }
    return con;
}


void ConnectorPool::push(Connector* con) {
    if (!con) {
        return;
    }

    if (con->isError()) {
        delete con;
        return;
    }

    con->reset();
    {
        std::lock_guard<std::mutex> ak(mMutex);
        AppPushRingQueueTail_1(mConQueue, con);
    }
}


usz ConnectorPool::close() {
    usz ret = 0;
    std::lock_guard<std::mutex> ak2(mMutex);
    for (Connector* i = AppPopRingQueueHead_1(mConQueue);
        i; i = AppPopRingQueueHead_1(mConQueue)) {
        delete i;
        ++ret;
    }
    return ret;
}


} //namespace db
} //namespace app
