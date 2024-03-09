#ifndef APP_DB_VALUE_H
#define	APP_DB_VALUE_H

#include "Converter.h"
#include "Strings.h"

namespace app {
namespace db {

class Row;

class Value {
    friend Row;

public:
    Value() :mValid(false) { }
    ~Value() = default;
    Value(const Value&) = default;
    Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;


    bool isValid() const;

    /**
     * @brief 转换失败将返回默认值.
     *
     * It is also safe to always call asStrView() since the
     * MySQL server will always return the data as char[].
     *
     * @return 转换值.
     * @{
     */
    StringView asStrView() const;
    u64 asUInt64() const;
    s64 asInt64() const;
    u32 asUInt32() const;
    s32 asInt32() const;
    u16 asUInt16() const;
    s16 asInt16() const;
    u8 asUInt8() const;
    s8 asInt8() const;
    bool asBool() const;
    f32 asFloat() const;
    f64 asDouble() const;
    /** @} */

private:
    explicit Value(StringView data, bool valid);
    bool mValid;
    StringView mValue;
};

} //namespace db
} //namespace app

#endif //APP_DB_VALUE_H
