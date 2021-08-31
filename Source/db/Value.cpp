#include "db/Value.h"

#include <string>

namespace app {
namespace db {

Value::Value(StringView data, bool valid)
    : mValue(data), mValid(valid) {
}

bool Value::isValid() const {
    return mValid;
}

StringView Value::asStrView() const {
    return mValue;
}

u64 Value::asUInt64() const {
    try {
        return std::stoull(mValue.mData);
    } catch (...) {
    }
    return 0;
}

s64 Value::asInt64() const {
    try {
        return std::stoll(mValue.mData);
    } catch (...) {
    }
    return 0;
}

u32 Value::asUInt32() const {
    try {
        return std::stoul(mValue.mData);
    } catch (...) {
    }
    return 0;
}

s32 Value::asInt32() const {
    try {
        return std::stoi(mValue.mData);
    } catch (...) {
    }
    return 0;
}

u16 Value::asUInt16() const {
    try {
        return static_cast<u16>(std::stoul(mValue.mData));
    } catch (...) {
    }

    return 0;
}

s16 Value::asInt16() const {
    try {
        return static_cast<s16>(std::stoi(mValue.mData));
    } catch (...) {
    }

    return 0;
}

u8 Value::asUInt8() const {
    try {
        return static_cast<u8>(std::stoul(mValue.mData));
    } catch (...) {
    }

    return 0;
}

s8 Value::asInt8() const {
    try {
        return static_cast<s8>(std::stoi(mValue.mData));
    } catch (...) {
    }

    return 0;
}

bool Value::asBool() const {
    try {
        return (std::stol(mValue.mData) != 0);
    } catch (...) {
    }

    return false;
}

f32 Value::asFloat() const {
    try {
        return std::stof(mValue.mData);
    } catch (...) {
    }

    return 0;
}

f64 Value::asDouble() const {
    try {
        return std::stod(mValue.mData);
    } catch (...) {
    }

    return 0;
}

} //namespace db
} //namespace app
