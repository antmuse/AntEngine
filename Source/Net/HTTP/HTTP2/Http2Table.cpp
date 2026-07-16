#include "Http2Table.h"
#include "Logger.h"

namespace app {


static Http2HeadField HttpV2StaticTable[] = {
    {std::string(":authority"), std::string("")}, // host
    {std::string(":method"), std::string("GET")},
    {std::string(":method"), std::string("POST")},
    {std::string(":path"), std::string("/")},
    {std::string(":path"), std::string("/index.html")},
    {std::string(":scheme"), std::string("http")},
    {std::string(":scheme"), std::string("https")},
    {std::string(":status"), std::string("200")},
    {std::string(":status"), std::string("204")},
    {std::string(":status"), std::string("206")},
    {std::string(":status"), std::string("304")},
    {std::string(":status"), std::string("400")},
    {std::string(":status"), std::string("404")},
    {std::string(":status"), std::string("500")},
    {std::string("accept-charset"), std::string("")},
    {std::string("accept-encoding"), std::string("gzip, deflate")},
    {std::string("accept-language"), std::string("")},
    {std::string("accept-ranges"), std::string("")},
    {std::string("accept"), std::string("")},
    {std::string("access-control-allow-origin"), std::string("")},
    {std::string("age"), std::string("")},
    {std::string("allow"), std::string("")},
    {std::string("authorization"), std::string("")},
    {std::string("cache-control"), std::string("")},
    {std::string("content-disposition"), std::string("")},
    {std::string("content-encoding"), std::string("")},
    {std::string("content-language"), std::string("")},
    {std::string("content-length"), std::string("")},
    {std::string("content-location"), std::string("")},
    {std::string("content-range"), std::string("")},
    {std::string("content-type"), std::string("")},
    {std::string("cookie"), std::string("")},
    {std::string("date"), std::string("")},
    {std::string("etag"), std::string("")},
    {std::string("expect"), std::string("")},
    {std::string("expires"), std::string("")},
    {std::string("from"), std::string("")},
    {std::string("host"), std::string("")},
    {std::string("if-match"), std::string("")},
    {std::string("if-modified-since"), std::string("")},
    {std::string("if-none-match"), std::string("")},
    {std::string("if-range"), std::string("")},
    {std::string("if-unmodified-since"), std::string("")},
    {std::string("last-modified"), std::string("")},
    {std::string("link"), std::string("")},
    {std::string("location"), std::string("")},
    {std::string("max-forwards"), std::string("")},
    {std::string("proxy-authenticate"), std::string("")},
    {std::string("proxy-authorization"), std::string("")},
    {std::string("range"), std::string("")},
    {std::string("referer"), std::string("")},
    {std::string("refresh"), std::string("")},
    {std::string("retry-after"), std::string("")},
    {std::string("server"), std::string("")},
    {std::string("set-cookie"), std::string("")},
    {std::string("strict-transport-security"), std::string("")},
    {std::string("transfer-encoding"), std::string("")},
    {std::string("user-agent"), std::string("")},
    {std::string("vary"), std::string("")},
    {std::string("via"), std::string("")},
    {std::string("www-authenticate"), std::string("")},
};



Http2Table::Http2Table() {
}

Http2Table::~Http2Table() {
}

s32 Http2Table::getLineBy(usz index, s32 flag, Http2HeadField& h2line) {
    constexpr u32 static_max = DSIZEOF(HttpV2StaticTable);
    const Http2HeadField* line;
    if (index < static_max) {
        line = &HttpV2StaticTable[index];
    } else {
        flag |= 4;
        index -= static_max;
        if (index >= mDynamicTable.size()) {
            DLOG(ELL_ERROR, "head line not in dynamic table[%lu]", index);
            return EE_ERROR;
        }
        line = &mDynamicTable[index];
    }
    if (flag & 1) {
        h2line.mName = line->mName;
    }
    if (flag & 2) {
        h2line.mValue = line->mValue;
    }
    DLOG(ELL_INFO, "read[%d] in table[%lu], %s = %s", flag, index, h2line.mName.data(), h2line.mValue.data());
    return EE_OK;
}


void Http2Table::addLine(const Http2HeadField& md) {
    constexpr u32 static_max = DSIZEOF(HttpV2StaticTable);
    mDynamicTable.push_front(md);
    mCurrentTableSize += md.mName.size() + md.mValue.size();
    mCurrentTableSize += 32;
    adjustTableBySize();
    DLOG(ELL_INFO, "addLine,  [%s = %s]", md.mName.data(), md.mValue.data());
    // #ifdef DDEBUG
    DLOG(ELL_INFO, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (usz i = 0; i < mDynamicTable.size(); i++) {
        u32 idx = i + static_max;
        std::string str_key = mDynamicTable[i].mName;
        std::string str_value = mDynamicTable[i].mValue;
        DLOG(ELL_INFO, "dynamic table[%u] [%s = %s]", idx, str_key.data(), str_value.data());
    }
    DLOG(ELL_INFO, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    // #endif
}

// when recv SETTINGS FRAME
// SETTINGS_HEADER_TABLE_SIZE
void Http2Table::setMaxLimitSize(u32 limit) {
    mTableMaxLimitSize = limit;
}

// when recv header frame (RFC 7540)
// 6.3.  Dynamic Table Size Update
s32 Http2Table::updateMaxSize(u32 size) {
    if(size>mTableMaxLimitSize){
        return EE_INVALID_PARAM;
    }
    mTableMaxSize = size;
    adjustTableBySize();
    return EE_OK;
}

usz Http2Table::getSize() {
    return mDynamicTable.size();
}

u32 Http2Table::getMaxLimitSize() {
    return mTableMaxLimitSize;
}

u32 Http2Table::getMaxSize() {
    return mTableMaxSize;
}

s32 Http2Table::getIndexBy(const Http2HeadField& h2line) {
    usz count = mDynamicTable.size();
    for (usz i = 0; i < count; i++) {
        if (h2line.mName == mDynamicTable[i].mName && h2line.mValue == mDynamicTable[i].mValue) {
            return static_cast<s32>(i);
        }
    }
    return -1;
}

void Http2Table::adjustTableBySize() {
    while (mCurrentTableSize > mTableMaxSize && !mDynamicTable.empty()) {
        auto entry = mDynamicTable.back();
        auto element_size = entry.mName.size() + entry.mValue.size();
        mCurrentTableSize -= element_size + 32;
        mDynamicTable.pop_back();
    }
}


} // namespace app
