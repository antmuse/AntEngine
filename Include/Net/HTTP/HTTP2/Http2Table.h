#pragma once
#include <deque>
#include "Net/HTTP/HTTP2/Http2Frame.h"

namespace app {

/**
 * @brief dynamic table for http2
 */
class Http2Table {
public:
    Http2Table();
    ~Http2Table();

    /**
     * @param index  index of table(rang[0-60] for static table else for dynamic table)
     * @param flag:  bit 1 = name, bit 2= value
     * @return 0 if success else fail code.
     */
    s32 getLineBy(usz index, s32 flag, Http2HeadField& h2line);

    void addLine(const Http2HeadField& md);

    // @return index of headline if >=0, else fail
    s32 getIndexBy(const Http2HeadField& h2line);

    // called when recv SETTINGS frame,  SETTINGS_HEADER_TABLE_SIZE
    void setMaxLimitSize(u32 limit);

    // called when recv HEADER frame (RFC 7540),  6.3.  Dynamic Table Size Update
    s32 updateMaxSize(u32 size);

    usz getSize();
    u32 getMaxLimitSize();
    u32 getMaxSize();

private:
    void adjustTableBySize();

    u32 mTableMaxLimitSize = GHTTP2_MAX_DTABLE_SIZE; // set by SETTINGS_HEADER_TABLE_SIZE
    u32 mTableMaxSize = GHTTP2_MAX_DTABLE_SIZE;      // set by HEADER FRAME
    u32 mCurrentTableSize = 0;
    std::deque<Http2HeadField> mDynamicTable;
};

} // namespace app
