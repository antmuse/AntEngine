#pragma once
#include <string>
#include "Config.h"

namespace app {
#ifdef DENDIAN_BIG
#define AppCodec16(buf) (*(u16*)(buf))
#define AppCodec32(buf) (*(u32*)(buf))
#define AppCodecVal16(v) (v)
#define AppCodecVal32(v) (v)
#else
#define AppCodec16(buf) AppSwap16(*(u16*)(buf))
#define AppCodec32(buf) AppSwap32(*(u32*)(buf))
#define AppCodecVal16(v) AppSwap16(v)
#define AppCodecVal32(v) AppSwap32(v)
#endif

/**
 * 一个HTTP/2流可以处于idle、reserverd (local)、reserved (remote)、open、half closed (remote)、half cloased
 * (local)以及close状态。
 */
enum Http2StreamStatus {
    H2SS_IDLE = 0,
    H2SS_OPEN_BOTH = 1,
    H2SS_RESERVER_LOCAL = 2,
    H2SS_RESERVER_REMOTE = 3,
    H2SS_CLOSE_LOCAL = 4,
    H2SS_CLOSE_REMOTE = 5,
    H2SS_CLOSE_BOTH = 6
};

enum Http2StaticTableIndex {
    TAB_HTTP2_AUTHORITY_INDEX = 1,
    TAB_HTTP2_METHOD_INDEX = 2,
    TAB_HTTP2_METHOD_GET_INDEX = 2,
    TAB_HTTP2_METHOD_POST_INDEX = 3,
    TAB_HTTP2_PATH_INDEX = 4,
    TAB_HTTP2_PATH_ROOT_INDEX = 4,
    TAB_HTTP2_PATH_DEF_INDEX = 5,
    TAB_HTTP2_SCHEME_HTTP_INDEX = 6,
    TAB_HTTP2_SCHEME_HTTPS_INDEX = 7,
    TAB_HTTP2_STATUS_INDEX = 8,
    TAB_HTTP2_STATUS_200_INDEX = 8,
    TAB_HTTP2_STATUS_204_INDEX = 9,
    TAB_HTTP2_STATUS_206_INDEX = 10,
    TAB_HTTP2_STATUS_304_INDEX = 11,
    TAB_HTTP2_STATUS_400_INDEX = 12,
    TAB_HTTP2_STATUS_404_INDEX = 13,
    TAB_HTTP2_STATUS_500_INDEX = 14,
    TAB_HTTP2_CONTENT_LENGTH_INDEX = 28,
    TAB_HTTP2_CONTENT_TYPE_INDEX = 31,
    TAB_HTTP2_DATE_INDEX = 33,
    TAB_HTTP2_LAST_MODIFIED_INDEX = 44,
    TAB_HTTP2_LOCATION_INDEX = 46,
    TAB_HTTP2_SERVER_INDEX = 54,
    TAB_HTTP2_VARY_INDEX = 59
};

constexpr s32 GHTTP2_MAX_WINDOW = ((1U << 31) - 1);
constexpr s32 GHTTP2_DEFAULT_WINDOW = 65535;
constexpr s32 GHTTP2_DEFAULT_WEIGHT = 16;
constexpr s32 GHTTP2_FRAME_HEADER_SIZE = 9;
constexpr s32 GHTTP2_INT_OCTETS = 4;
constexpr s32 GHTTP2_MAX_DTABLE_SIZE = 4096; // dynamic table max size
constexpr u32 GHTTP2_MAX_WINDOW_SIZE = ((1U << 31) - 1);
constexpr s32 GHTTP2_MAX_FRAME_SIZE = ((1 << 24) - 1);
constexpr s32 GHTTP2_DEF_FRAME_SIZE = (1 << 14); // default frame size

constexpr u32 GHTTP2_MASK_HUFFMAN = 0x80; // huffman string
constexpr u32 GHTTP2_MASK_RAW = 0x00;     // raw string

constexpr u32 GHTTP2_MASK_TABLE_STATIC = 0x80;   // both kv in static table or dynamic table
constexpr u32 GHTTP2_MASK_TABLE_INC = 0x40;      // need add in dynamic table
constexpr u32 GHTTP2_MASK_TABLE_SET_SIZE = 0x20; // need update dynamic table size
constexpr u32 GHTTP2_MASK_TABLE_NO_INDEX = 0x10; // never cached kv in table

constexpr s32 GHTTP2_MAX_FIELD = (127 + (1 << (GHTTP2_INT_OCTETS - 1) * 7) - 1);
#define DHTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define DLEN_MASK_PREFIX(bits) ((1 << (bits)) - 1)

// @see Http2StaticTableIndex
#define DHTTP2_TABLE_INDEX(i) (GHTTP2_MASK_TABLE_STATIC | (i))
#define DHTTP2_TABLE_INC_INDEX(i) (GHTTP2_MASK_TABLE_INC | (i))

enum Http2Error {
    H2E_OK = 0x0,
    H2E_PROTOCOL_ERROR = 0x1,
    H2E_INTERNAL_ERROR = 0x2,
    H2E_FLOW_CTRL_ERROR = 0x3,
    H2E_SETTINGS_TIMEOUT = 0x4,
    H2E_STREAM_CLOSED = 0x5,
    H2E_SIZE_ERROR = 0x6,
    H2E_REFUSED_STREAM = 0x7,
    H2E_CANCEL = 0x8,
    H2E_COMP_ERROR = 0x9,
    H2E_CONNECT_ERROR = 0xa,
    H2E_ENHANCE_YOUR_CALM = 0xb,
    H2E_INADEQUATE_SECURITY = 0xc,
    H2E_HTTP_1_1_REQUIRED = 0xd
};

enum Http2FrameType {
    H2FT_DATA_FRAME = 0x0,
    H2FT_HEADERS_FRAME = 0x1,
    H2FT_PRIORITY_FRAME = 0x2,
    H2FT_RST_STREAM_FRAME = 0x3,
    H2FT_SETTINGS_FRAME = 0x4,
    H2FT_PUSH_PROMISE_FRAME = 0x5,
    H2FT_PING_FRAME = 0x6,
    H2FT_GOAWAY_FRAME = 0x7,
    H2FT_WINDOW_UPDATE_FRAME = 0x8,
    H2FT_CONTINUATION_FRAME = 0x9
};


enum Http2FrameSize {
    H2FS_SETTINGS_ACK_SIZE = 0,
    H2FS_RST_STREAM_SIZE = 4,
    H2FS_PRIORITY_SIZE = 5,
    H2FS_PING_SIZE = 8,
    H2FS_GOAWAY_SIZE = 8,
    H2FS_WINDOW_UPDATE_SIZE = 4,
    H2FS_SETTINGS_PARAM_SIZE = 6 // setting frame's size = 6*x (是6的倍数)
};


enum Http2FrameFlags {
    H2FF_NO_FLAG = 0x00,
    H2FF_ACK_FLAG = 0x01,        // TODO
    H2FF_END_STREAM_FLAG = 0x01, // TODO
    H2FF_END_HEADERS_FLAG = 0x04,
    H2FF_PADDED_FLAG = 0x08,
    H2FF_PRIORITY_FLAG = 0x20
};


/**
 * @brief head for all frames.
    +-----------------------------------------------+
    |                 Length (24)                   |
    +---------------+---------------+---------------+
    |   Type (8)    |   Flags (8)   |
    +-+-------------+---------------+-------------------------------+
    |R|                 Stream Identifier (31)                      |
    +=+=============================================================+
    |                   Frame Payload (0...)                      ...
    +---------------------------------------------------------------+
 */
class Http2FrameHead {
public:
    mutable u32 mLen = 0;
    u32 mStreamID = 0; // 0 for tcp, else for http2 stream.
    u8 mType = 0;
    u8 mFlags = 0;
    void show() const;
    s32 parseRaw(const u8** buf, usz len);
    s32 writeOut(std::string& out) const;
    s32 writeOut(u8* out) const;

    /** @return len of writed in bytes. */
    static u32 writeHpackU32(u32 len, u32 prefix, u8* out);
    static void writeHpackU32(u32 len, u32 prefix, std::string& out);
    static void writeHpackStr(const std::string& in, std::string& out);
};


/**
 * @brief Http2Setting帧始终适用于连接，而不是作用于单个流。 is the 1st frame of a new TCP connection.
 * @note stream id必须为0, 否则PROTOCOL_ERROR。  Http2Setting带ACK帧的有效负载必须为空, 否则PROTOCOL_ERROR。
    +-------------------------------+
    |       Identifier (16)         |
    +-------------------------------+-------------------------------+
    |                        Value (32)                             |
    +---------------------------------------------------------------+
    | ......
    +---------------------------------------------------------------+
*/
class Http2Setting {
public:
    // settings fields
    enum ESET_FIELDS {
        ESET_HEADER_TABLE_SIZE_SETTING = 0x1,
        ESET_ENABLE_PUSH_SETTING = 0x2,
        ESET_MAX_STREAMS_SETTING = 0x3,
        ESET_INIT_WINDOW_SIZE_SETTING = 0x4,
        ESET_MAX_FRAME_SIZE_SETTING = 0x5,
        ESET_MAX_HEADER_LIST_SIZE = 0x6,

        ESET_SETTINGS_COUNT
    };

    // 以下6个u32必须按ESET_FIELDS中顺序定义： @see Http2Setting::writeOut()
    u32 mTableSize = GHTTP2_MAX_DTABLE_SIZE; // 发送方允许的最大动态表容量
    u32 mPushVal = 0;                        // 发送方是否允许发送PUSH_PROMISE帧
    u32 mMaxStreams = 100;                   // 发送方允许的最大并发流数
    u32 mWindowSize = 32 * 1024;             // 发送方的初始窗口大小(以八位字节为单位), 用于 stream 流级别流量控制
    u32 mFrameSize = GHTTP2_DEF_FRAME_SIZE;  // 发送方愿意接收的最大帧负载, (以八位字节为单位)
    u32 mHeadListSize = 4 * 1024;            // 发送方准备接受的头列表的最大大小  (基于头字段的未压缩大小)

    void show() const;
    s32 parseBody(const u8** buf, usz len, Http2FrameHead& head);
    s32 writeOut(u8 flag, std::string& out) const;
    s32 writeOutACK(std::string& out) const;
};



/**
 * @brief Http2PushPromise帧始用于发送方打算发起的流之前提前通知对端
 * @note stream id必须为0, 否则PROTOCOL_ERROR。  Http2Setting带ACK帧的有效负载必须为空, 否则PROTOCOL_ERROR。
    +---------------+
    |Pad Length? (8)|
    +-+-------------+-----------------------------------------------+
    |R|                  Promised Stream ID (31)                    |
    +-+-----------------------------+-------------------------------+
    |                   Header Block Fragment (*)                 ...
    +---------------------------------------------------------------+
    |                           Padding (*)                       ...
    +---------------------------------------------------------------+
*/
class Http2PushPromise {
public:
    u8 mPadLen;
    u32 mPromiseStreamID;
    // TODO
};


/**
 * @brief 流量控制在两个级别上运行：在每个单独的流上和整个连接上。代理服务器不需要向上游转发 WINDOW_UPDATE 帧
     +-+-------------------------------------------------------------+
    |R|              Window Size Increment (31)                     |
    +-+-------------------------------------------------------------+
 */
class Http2UpdateWindow {
public:
    u32 mWinSizeIncrement = 32 * 1024; // 必须非0, 否则PROTOCOL_ERROR
    s32 parseBody(const u8** buf, usz len, Http2FrameHead& head);
};


/**
 * @brief 此帧可以在任何流的状态下发送，包括空闲或关闭的流。
 * @note stream id必须非0, 否则PROTOCOL_ERROR
    +-+-------------------------------------------------------------+
    |E|                  Stream Dependency (31)                     |
    +-+-------------+-----------------------------------------------+
    |   Weight (8)  |
    +-+-------------+
 */
class Http2Priority {
public:
    u8 mWeight = 0;
    u32 mDependency = 0;
    s32 parseBody(const u8** buf, usz len, Http2FrameHead& head);
};


/**
 * @brief 关闭流。关闭后仍然可以发送Http2Priority
 * @note stream id必须非0, 否则PROTOCOL_ERROR
    +---------------------------------------------------------------+
    |                        Error Code (32)                        |
    +---------------------------------------------------------------+
 */
class Http2ResetStream {
public:
    u32 mErrCode = 0; // @see Http2Error
    s32 parseBody(const u8** buf, usz len, Http2FrameHead& head);
};



/** @brief Http2Header for open a new stream */
class Http2Header {
public:
    u8 mPadLen = 0;      // only valid if H2FF_PADDED_FLAG on
    u8 mWeight = 0;      // only valid if H2FF_PRIORITY_FLAG on
    u8 mExclusive = 0;   // only valid if H2FF_PRIORITY_FLAG on
    u32 mDependency = 0; // only valid if H2FF_PRIORITY_FLAG on
    // TODO
    s32 writeOut(u8 flag, u32 sid, std::string& out) const;
};

/**
 * @brief CONTINUATION
 * @note stream id必须非0, 否则PROTOCOL_ERROR
    +---------------------------------------------------------------+
    |                   Header Block Fragment (*)                 ...
    +---------------------------------------------------------------+
 */
class Http2HeaderContinue {
public:
    // TODO
};



class Http2Data {
public:
    u8 mPadLen = 0;      // only valid if H2FF_PADDED_FLAG on
    u8 mWeight = 0;      // only valid if H2FF_PRIORITY_FLAG on
    u8 mExclusive = 0;   // only valid if H2FF_PRIORITY_FLAG on
    u32 mDependency = 0; // only valid if H2FF_PRIORITY_FLAG on
    s32 parseBody(const u8* buf);
    s32 writeOut(u8 flag, u32 sid, u32 len, std::string& out) const;
};


/**
 * @brief Http2Ping的响应应该设置ACK, 且优先于任何其他帧发送。
 * @note stream id必须是0, 否则PROTOCOL_ERROR
    +---------------------------------------------------------------+
    |                      Opaque Data (64)                         |
    +---------------------------------------------------------------+
 */
class Http2Ping {
public:
    u8 mOpaqueData[8] = {'h', '2', '-', 'p', 'i', 'n', 'g', 0};
    s32 writeOut(u8 flag, std::string& out) const;
};


/**
 * @brief Http2Goaway适用于连接，而不是特定的流。 用于连接关闭或发出严重错误信号, 帧的接收者不再使用该连接.
 * @note stream id必须是0, 否则PROTOCOL_ERROR
    +-+-------------------------------------------------------------+
    |R|                  Last-Stream-ID (31)                        |
    +-+-------------------------------------------------------------+
    |                      Error Code (32)                          |
    +---------------------------------------------------------------+
    |                  Additional Debug Data (*)                    |
    +---------------------------------------------------------------+
 */
class Http2Goaway {
public:
    u32 mLastStreamID = 0;
    u32 mErrCode = 0; // @see Http2Error
    // TODO
};



/** @brief header line */
struct Http2HeadField {
    std::string mName;
    std::string mValue;
};


} // namespace app
