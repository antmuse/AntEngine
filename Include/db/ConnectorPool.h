#ifndef APP_DB_CONNECTORPOOL_H
#define	APP_DB_CONNECTORPOOL_H


#include "db/ConnectConfig.h"
#include "db/Connector.h"
#include "db/ConnectorHolder.h"
#include <atomic>
#include <mutex>

namespace app {
namespace db {

class Database;


/**
* @brief 数据库连接池
*/
class ConnectorPool {
    friend Database;
    friend ConnectorHolder;

public:
    ConnectorPool() = delete;
    ~ConnectorPool();

    ConnectorPool(const ConnectorPool&) = delete;
    ConnectorPool(ConnectorPool&&) = delete;
    ConnectorPool& operator=(const ConnectorPool&) noexcept = delete;
    ConnectorPool& operator=(ConnectorPool&&) noexcept = delete;

    const ConnectConfig& getConnectionInfo() const {
        return mConInfo;
    }


private:
    std::mutex mMutex;
    Connector* mConQueue;
    ConnectConfig mConInfo;

    explicit ConnectorPool(ConnectConfig connection);


    /**
     * @brief pop a Connector from the pool with the provided task, timeout and on complete callback.
     * @return A Connector handle.
     */
    Connector* pop();


    /**
    * @brief 将连接还回数据库连接池
    */
    void push(Connector* con);

    /**
    * @return 返回关闭的连接数量
    */
    usz close();
};

} //namespace db
} //namespace app

#endif //APP_DB_CONNECTORPOOL_H