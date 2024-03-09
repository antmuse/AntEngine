#ifndef APP_DB_CONNECTOR_H
#define APP_DB_CONNECTOR_H

#include "db/ConnectConfig.h"
#include "Logger.h"
#include "db/Row.h"
#include "db/Task.h"

#include <string>
#include <mysql.h>

namespace app {
namespace db {

class GlobalDBInitializer {
public:
    GlobalDBInitializer() {
        mysql_library_init(0, nullptr, nullptr);
    }
    ~GlobalDBInitializer() {
        mysql_library_end();
    }
};


class ConnectorPool;
class ConnectorHolder;
class Database;
class Connector;

class Connector {
    friend ConnectorPool;
    friend ConnectorHolder;
    friend Database;

public:
    Connector* mNext; // inner use only

    Connector() = delete;
    ~Connector();

    Connector(const Connector&) = delete;
    Connector(Connector&&) = delete;
    Connector& operator=(const Connector&) noexcept = delete;
    Connector& operator=(Connector&&) noexcept = delete;

    /**
     * @return Gets the task status after executed.
     */
    EErrorCode getStatus() const;

    /**
     * @return the original query that was executed
     */
    const std::string& getFinalRequest() const;

    /**
     * @return If no error then empty str, otherwise the MySQL client error message.
     */
    std::string getError() const;

    /// Has this MySQL client had an error?
    bool isError() const {
        return mHaveError;
    }

    usz getFieldCount() const;

    usz getRowCount() const;

    /**
     * @param idx The row to fetch.
     * @return The row.
     */
    const Row& getRow(usz idx) const;

    /**
     * @return The query result's rows.
     */
    const std::vector<Row>& getRows() const {
        return mRows;
    }

    /**
     * @return The last insert ID from this query.
     */
    u64 getLastInsertID() {
        return mysql_insert_id(&mMySQL);
    }

private:
    /**
     * @brief
     * @param timeout in seconds
     */
    Connector(ConnectorPool& query_pool, const ConnectConfig& connection);

    /**
     * Resets Connector, free result and clear task.
     */
    void reset();

    void setTimeout(u32 seconds);

    /**
     * Executes the task synchronously.
     * @return 0 if success, else error or timeout.
     */
    EErrorCode execute(Task* task);

    /**
     * @brief 如果句柄尚未连接，将阻塞并连接。
     * @return true if success, else false.
     */
    bool connect();

    void parseRows();

    void freeResult();

    ConnectorPool& mPool;
    const ConnectConfig& mConInfo;

    mutable MYSQL mMySQL;

    MYSQL_RES* mResult{nullptr};

    /// True if the result has already been parsed (to avoid doing it multiple times).
    bool mParsedResult{false};

    /// The number of fields returned from the query.
    usz mFieldCount{0};

    /// The number of rows returned from the query.
    usz mRowCount{0};

    /// User facing rows view.
    std::vector<Row> mRows;

    bool mConnected;
    bool mHaveError;

    /// The status of the last query.
    EErrorCode mStatus;

    /// The SQL task that was last executed
    std::string mFinalRequest;
};

} // namespace db
} // namespace app

#endif // APP_DB_CONNECTOR_H