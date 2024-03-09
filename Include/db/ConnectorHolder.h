#ifndef APP_DB_CONNECTORHOLDER_H
#define	APP_DB_CONNECTORHOLDER_H

#include "db/Connector.h"

#include <memory>

namespace app {
namespace db {

/**
 * @brief 仅仅是一个Connector代理类，确保析构时将Connector返回给DB连接池
 */
class ConnectorHolder {
    friend class ConnectorPool;
    friend class Database;

public:
    ConnectorHolder() :mConnector(nullptr) { }
    ~ConnectorHolder();

    ConnectorHolder(const ConnectorHolder&) = delete;
    ConnectorHolder& operator=(const ConnectorHolder&) noexcept = delete;

    ConnectorHolder(ConnectorHolder&& from)noexcept;
    ConnectorHolder& operator=(ConnectorHolder&&) noexcept;


    Connector& operator*();
    const Connector& operator*() const;
    Connector* operator-> ();
    const Connector* operator-> () const;

private:
    explicit ConnectorHolder(Connector* query);
    Connector* mConnector;
};

} //namespace db
} //namespace app


#endif //APP_DB_CONNECTORHOLDER_H