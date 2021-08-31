#include "db/Row.h"

namespace app {
namespace db {

Row::Row(MYSQL_ROW dbrow, usz field_count, unsigned long* lengths)
    : mColumnCount(field_count) {
    mValues.reserve(field_count);
    StringView data;
    for (usz i = 0; i < field_count; ++i) {
        auto* dbval = dbrow[i];
        if (dbval) {
            auto length = (lengths != nullptr) ? lengths[i] : strlen(dbval);
            data.set(dbval, length);
        } else {
            data.set("", 0);
        }
        Value value(std::move(data), nullptr!= dbval);
        mValues.emplace_back(value);
    }
}

} //namespace db
} //namespace app
