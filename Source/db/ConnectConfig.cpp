#include "db/ConnectConfig.h"
#include <mysql.h>




namespace app {
namespace db {

ConnectConfig::ConnectConfig(std::string host, u16 port,
    std::string user, std::string password,
    std::string database, u64 flags) :
    mHost(std::move(host))
    , mPort(port)
    , mUnixSock("0")
    , mUser(std::move(user))
    , mPassword(std::move(password))
    , mDatabase(std::move(database))
    , mClientFlags(flags | CLIENT_MULTI_STATEMENTS) {

}

ConnectConfig::ConnectConfig(std::string uxsock, std::string user,
    std::string password, std::string database, u64 flags)
    : mHost("localhost")
    , mPort(0)
    , mUnixSock(std::move(uxsock))
    , mUser(std::move(user))
    , mPassword(std::move(password))
    , mDatabase(std::move(database))
    , mClientFlags(flags | CLIENT_MULTI_STATEMENTS) {
}

} //namespace db
} //namespace app
