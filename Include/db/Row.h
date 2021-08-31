#ifndef APP_DB_ROW_H
#define	APP_DB_ROW_H

#include "db/Value.h"
#include <vector>
#include <mysql.h>

namespace app {
namespace db {

class Connector;

class Row {
    friend Connector;

public:
    Row():mColumnCount(0){ }

    ~Row() { }

    Row(const Row&)noexcept = delete;

    Row(Row&& it) noexcept {
        mValues = std::move(it.mValues);
        mColumnCount = it.mColumnCount;
    }

    Row& operator=(const Row&) noexcept = delete;

    Row& operator=(Row&& it) noexcept {
        mValues = std::move(it.mValues);
        mColumnCount = it.mColumnCount;
        return *this;
    }

    /**
     * @return The number of values/columns in this row.
     */
    usz getColumnCount() const {
        return mColumnCount;
    }

    /**
     * @param idx The value to fetch at this index.
     * @return The specified value at idx.
     */
    const Value& getColumn(usz idx) const {
        return mValues.at(idx);
    }

    /**
     * @return The column values for this row.
     */
    const std::vector<Value>& getColumns() const {
        return mValues;
    }

private:
    Row(MYSQL_ROW mysql_row, usz field_count, unsigned long* lengths);

    std::vector<Value> mValues;
    usz mColumnCount;
};

} //namespace db
} //namespace app

#endif //APP_DB_ROW_H
