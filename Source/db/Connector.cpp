#include "db/Connector.h"
#include "db/ConnectorPool.h"
#include "db/Util.h"

#include <mysql.h>


namespace app {
namespace db {

Connector::Connector(ConnectorPool& dbpool, const ConnectConfig& connection)
    : mPool(dbpool)
    , mNext(nullptr)
    , mConInfo(connection)
    , mConnected(false)
    , mHaveError(false)
    , mStatus(EE_NO_OPEN)
    , mParsedResult(false) {

    mysql_init(&mMySQL);

    setTimeout(10);

    u32 connect_timeout = 1;
    mysql_options(&mMySQL, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);

    u64 auto_reconnect = 1;
    mysql_options(&mMySQL, MYSQL_OPT_RECONNECT, &auto_reconnect);

#ifdef D_MYSQL_OPT_SSL_ENFORCE_DISABLED
    bool ssl = false;
    mysql_options(&mMySQL, MYSQL_OPT_SSL_ENFORCE, &ssl);
#endif

    //  Some mysql libraries do not support SSL_MODE_DISABLED
#ifdef D_PERCONA_SSL_DISABLED
    u32 ssl_mode = SSL_MODE_DISABLED;
    mysql_options(&mMySQL, MYSQL_OPT_SSL_MODE, &ssl_mode);
#endif
}


Connector::~Connector() {
    reset();
    mysql_close(&mMySQL);
}

EErrorCode Connector::getStatus() const {
    return mStatus;
}

const std::string& Connector::getFinalRequest() const {
    return mFinalRequest;
}

std::string Connector::getError() const {
    return mHaveError ? mysql_error(&mMySQL) : "";
}

usz Connector::getFieldCount() const {
    return mFieldCount;
}

usz Connector::getRowCount() const {
    return mRowCount;
}

const Row& Connector::getRow(usz idx) const {
    return mRows.at(idx);
}

void Connector::reset() {
    freeResult();
    mStatus = EE_NO_OPEN;
    mHaveError = false;
}

void Connector::setTimeout(u32 seconds) {
    mysql_options(&mMySQL, MYSQL_OPT_READ_TIMEOUT, &seconds);
}

EErrorCode Connector::execute(Task* task) {
    DASSERT(task);

    setTimeout(task->mTimeout);

    if (!mConnected) {
        if (!connect()) {
            mStatus = EE_NO_OPEN;
            mHaveError = true;
            return mStatus;
        }
    }

    freeResult();

    try {
        mFinalRequest = task->prepareCMD(
            [this](const std::string& str_value) {
            if (str_value.empty()) {
                throw std::invalid_argument("Empty task part passed");
            }
            // https://dev.mysql.com/doc/refman/5.7/en/mysql-real-escape-string.html
            std::string buffer;
            buffer.resize(str_value.length() * 2 + 1);

            usz length = mysql_real_escape_string(&mMySQL, (s8*)buffer.data(), &str_value.front(), str_value.length());
            buffer.resize(length);

            return buffer;
        });
    } catch (const std::invalid_argument& einv) {
        mStatus = EE_INVALID_PARAM;
        mHaveError = true;
        return mStatus;
    }

    if (0 == mysql_real_query(&mMySQL, mFinalRequest.c_str(), mFinalRequest.length())) {
        mResult = mysql_store_result(&mMySQL); //mysql_use_result()
        if (mResult != nullptr) {
            mStatus = EE_OK;
            mFieldCount = mysql_num_fields(mResult);
            mRowCount = mysql_num_rows(mResult);
            parseRows();
        } else {
            // Use this function to determine if the query should have returned values
            if (mysql_field_count(&mMySQL) == 0) {
                mStatus = EE_OK;
                mFieldCount = 0;
                mRowCount = mysql_affected_rows(&mMySQL);
                mParsedResult = true;
            } else {
                mStatus = EE_ERROR;
                mHaveError = true;
            }
        }
    } else {
        mStatus = EE_ERROR;
        mHaveError = true;
    }
    return mStatus;
}

bool Connector::connect() {
    if (!mConnected) {
        MYSQL* success = mysql_real_connect(
            &mMySQL,
            mConInfo.getHost().c_str(),
            mConInfo.getUser().c_str(),
            mConInfo.getPassword().c_str(),
            mConInfo.getDatabase().c_str(),
            mConInfo.getPort(),
            mConInfo.getUnixSocket().c_str(),
            mConInfo.getClientFlags());

        mConnected = nullptr != success;
    }
    return mConnected;
}

void Connector::parseRows() {
    if (!mParsedResult && getRowCount() > 0) {
        mParsedResult = true;
        mRows.reserve(getRowCount());
        MYSQL_ROW mysql_row = nullptr;
        while ((mysql_row = mysql_fetch_row(mResult))) {
            auto lengths = mysql_fetch_lengths(mResult);
            Row row(mysql_row, mFieldCount, lengths);
            mRows.emplace_back(std::move(row));
        }
    }
}

void Connector::freeResult() {
    if (mResult) {
        mysql_free_result(mResult);
        mResult = nullptr;
    }
    mParsedResult = false;
    mFieldCount = 0;
    mRowCount = 0;
    mRows.clear();
}


} //namespace db
} //namespace app
