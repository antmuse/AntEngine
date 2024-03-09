#ifndef APP_DB_CONNECTCONFIG_H
#define APP_DB_CONNECTCONFIG_H

#include "Config.h"
#include <string>

namespace app {
namespace db {

class ConnectConfig {
public:
    /**
     * Creates connection information for which database server to connect to.
     * @param host The database server hostname.
     * @param port The database server port.
     * @param user The user to authenticate with.
     * @param password The password to authenticate with.
     * @param database The database to use upon connecting (optional).
     * @param flags database client flags (optional).
     */
    ConnectConfig(
        std::string host, u16 port, std::string user, std::string password, std::string database = "", u64 flags = 0);

    /**
     * Creates connection information for which database server to connect to.
     * @param uxsock The unix-socket to communicate over.
     * @param user The user to authenticate with.
     * @param password The password to authenticate with.
     * @param database The database to use upon connecting (optional).
     * @param flags database client flags (optional).
     */
    ConnectConfig(std::string uxsock, std::string user, std::string password, std::string database = "", u64 flags = 0);

    const std::string& getHost() const {
        return mHost;
    }

    u16 getPort() const {
        return mPort;
    }

    /**
     * @return The database unix-uxsock.
     */
    const std::string& getUnixSocket() const {
        return mUnixSock;
    }


    const std::string& getUser() const {
        return mUser;
    }


    const std::string& getPassword() const {
        return mPassword;
    }


    const std::string& getDatabase() const {
        return mDatabase;
    }


    /**
     * @return The database client flags.
     * mysql_real_connect函数的最后一个参数为client_flag,默认通常用0,
     * 但使用存储过程时,是在同一字符串中执行多条语句,
     * 所以该参数需要带上mysql::CLIENT_MULTI_STATEMENTS
     */
    u64 getClientFlags() const {
        return mClientFlags;
    }

private:
    std::string mHost;
    u16 mPort;
    std::string mUnixSock;
    std::string mUser;
    std::string mPassword;
    std::string mDatabase;
    u64 mClientFlags;
};

} // namespace db
} // namespace app

#endif // APP_DB_CONNECTCONFIG_H
