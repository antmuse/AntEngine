#include "Http2Msg.h"
#include <string.h>
#include "Logger.h"
#include "Huffman.h"

namespace app {

s32 Http2FrameHead::parseRaw(const u8** p0, usz len) {
    if (len < GHTTP2_FRAME_HEADER_SIZE) {
        return EE_RETRY;
    }
    const u8* p1 = *p0;
    mLen = AppCodec32(p1);
    mLen >>= 8;
    mType = p1[3];
    mFlags = p1[4];
    mStreamID = 0x7FFFFFFF & AppCodec32(p1 + 5);
    *p0 += GHTTP2_FRAME_HEADER_SIZE;
    return EE_OK;
}

s32 Http2FrameHead::writeOut(std::string& out) const {
    if (mLen > 0xFFFFFF) {
        DLOG(ELL_ERROR, "frame too big, len=%u, type=%u", mLen, mType);
        return EE_ERROR;
    }
    usz len = out.size();
    out.resize(len + GHTTP2_FRAME_HEADER_SIZE);
    return writeOut((u8*)out.data() + len);
}

s32 Http2FrameHead::writeOut(u8* p0) const {
    if (mLen > 0xFFFFFF) {
        DLOG(ELL_ERROR, "frame too big, len=%u, type=%u", mLen, mType);
        return EE_ERROR;
    }
    *(u32*)(p0) = AppCodecVal32((mLen << 8) | mType);
    p0 += sizeof(u32);
    *p0++ = mFlags;
    *(u32*)(p0) = AppCodecVal32(mStreamID);
    // p0 += sizeof(u32);
    return EE_OK;
}

void Http2FrameHead::writeHpackU32(u32 value, u32 prefix, std::string& out) {
    usz olen = out.size();
    out.resize(olen + sizeof(u32) + 1);
    olen += writeHpackU32(value, prefix, (u8*)out.data() + olen);
    out.resize(olen);
}
u32 Http2FrameHead::writeHpackU32(u32 value, u32 prefix, u8* pos) {
    if (value < prefix) {
        *pos |= value;
        return 1;
    }
    *pos++ |= prefix;
    value -= prefix;
    u32 add = 1;
    while (value >= 128) {
        ++add;
        *pos++ = value % 128 + 128;
        value /= 128;
    }
    *pos = (u8)value;
    ++add;
    return add;
}


void Http2FrameHead::writeHpackStr(const std::string& in, std::string& out) {
    const u32 ilen = in.size();
    const usz olen = out.size();
    out.resize(olen + (ilen * 8ULL / 5) + sizeof(u32) + 1);
    u8* p0 = (u8*)out.data() + olen;

    u32 add;
    u32 wlen = AppHuffEncode((u8*)in.data(), ilen, p0 + 5, true);
    if (wlen > 0) {
        *p0 = GHTTP2_MASK_HUFFMAN;
        add = writeHpackU32(wlen, DLEN_MASK_PREFIX(7), p0);
        memmove(p0 + add, p0 + 5, wlen);
        out.resize(olen + add + wlen);
        return;
    }
    *p0 = GHTTP2_MASK_RAW;
    add = writeHpackU32(ilen, DLEN_MASK_PREFIX(7), p0);
    memcpy(p0 + add, in.data(), ilen);
    out.resize(olen + add + ilen);
}


void Http2FrameHead::show() const {
    DLOG(ELL_INFO, "len=%u,sid=%u,type=x%X,flags=x%X", mLen, mStreamID, mType, mFlags);
}

s32 Http2UpdateWindow::parseBody(const u8** buf, usz len, Http2FrameHead& head) {
    if (sizeof(u32) != head.mLen) {
        return EE_ERROR;
    }
    if (len < sizeof(u32)) {
        return EE_RETRY;
    }
    mWinSizeIncrement = 0x7FFFFFFF & AppCodec32(*buf);
    *buf += sizeof(u32);
    head.mLen -= sizeof(u32);
    return 0 == mWinSizeIncrement ? EE_ERROR : EE_OK;
}


s32 Http2Data::parseBody(const u8* buf) {
    return EE_OK;
}

s32 Http2Data::writeOut(u8 flag, u32 sid, u32 len, std::string& out) const {
    Http2FrameHead head;
    head.mLen = len;
    head.mStreamID = sid;
    head.mType = H2FT_DATA_FRAME;
    head.mFlags = flag;
    head.writeOut(out);
    return EE_OK;
}



void Http2Setting::show() const {
    DLOG(ELL_INFO, "Http2Setting = [%u,%u,%u,%u,%u,%u]", mTableSize, mPushVal, mMaxStreams, mWindowSize, mFrameSize,
        mHeadListSize);
}

s32 Http2Setting::parseBody(const u8** buf, usz len, Http2FrameHead& head) {
    if ((head.mFlags & H2FF_ACK_FLAG) && head.mLen > 0) {
        DLOG(ELL_ERROR, "Http2Setting's body whith ACK, sid = %u", head.mStreamID);
        return EE_ERROR;
    }
    if (0 != (head.mLen % H2FS_SETTINGS_PARAM_SIZE)) {
        DLOG(ELL_ERROR, "Http2Setting with invalid len = %u, sid = %u", head.mLen, head.mStreamID);
        return EE_ERROR;
    }
    s32 idx;
    u32 val;
    len = len > head.mLen ? head.mLen : len;
    while (len >= H2FS_SETTINGS_PARAM_SIZE) {
        idx = AppCodec16(*buf);
        val = AppCodec32(*buf + sizeof(u16));
        switch (idx) {
        case ESET_HEADER_TABLE_SIZE_SETTING:
            mTableSize = val;
            break;
        case ESET_ENABLE_PUSH_SETTING:
            if (val > 1) {
                DLOG(ELL_ERROR, "Http2Setting with invalid push = %u, sid = %u", val, head.mStreamID);
                return EE_ERROR;
            }
            mPushVal = val;
            break;
        case ESET_MAX_STREAMS_SETTING:
            mMaxStreams = val;
            break;
        case ESET_INIT_WINDOW_SIZE_SETTING:
            if (val > GHTTP2_MAX_WINDOW_SIZE) {
                DLOG(ELL_ERROR, "Http2Setting with invalid winsize = %u, sid = %u", val, head.mStreamID);
                return EE_ERROR;
            }
            mWindowSize = val;
            break;
        case ESET_MAX_FRAME_SIZE_SETTING:
            if (val > GHTTP2_MAX_FRAME_SIZE || val < GHTTP2_DEF_FRAME_SIZE) {
                DLOG(ELL_ERROR, "Http2Setting with invalid framesize = %u, sid = %u", val, head.mStreamID);
                return EE_ERROR;
            }
            mFrameSize = val;
            break;
        case ESET_MAX_HEADER_LIST_SIZE:
            mHeadListSize = val;
            break;
        default:
            return EE_ERROR;
        }
        head.mLen -= H2FS_SETTINGS_PARAM_SIZE;
        len -= H2FS_SETTINGS_PARAM_SIZE;
        *buf += H2FS_SETTINGS_PARAM_SIZE;
    }
    return 0 == head.mLen ? EE_OK : EE_RETRY;
}

s32 Http2Setting::writeOutACK(std::string& out) const {
    Http2FrameHead head;
    head.mLen = 0;
    head.mType = H2FT_SETTINGS_FRAME;
    head.mFlags = H2FF_ACK_FLAG;
    head.writeOut(out);
    return EE_OK;
};
s32 Http2Setting::writeOut(u8 flag, std::string& out) const {
    Http2FrameHead head;
    head.mLen = H2FS_SETTINGS_PARAM_SIZE * (ESET_SETTINGS_COUNT - 1);
    head.mType = H2FT_SETTINGS_FRAME;
    head.mFlags = flag;
    usz len = out.size();
    out.reserve(len + GHTTP2_FRAME_HEADER_SIZE + head.mLen);
    head.writeOut(out);
    len = out.size();
    out.resize(len + head.mLen);
    u8* p0 = (u8*)out.data() + len;
    const u32* pv = &mTableSize;
    for (u16 idx = ESET_HEADER_TABLE_SIZE_SETTING; idx < ESET_SETTINGS_COUNT;) {
        // DLOG(ELL_INFO, "Http2Setting out: idx[%d] = %u", idx, *pv);
        *(u16*)(p0) = AppCodecVal16(idx++);
        p0 += sizeof(u16);
        *(u32*)(p0) = AppCodecVal32(*pv++);
        p0 += sizeof(u32);
    }
    return EE_OK;
}

s32 Http2Header::writeOut(u8 flag, u32 sid, std::string& out) const {
    // if (0) {
    //     Http2FrameHead head;
    //     head.mLen = 1;
    //     head.mStreamID = sid;
    //     head.mType = H2FT_HEADERS_FRAME;
    //     head.mFlags = flag;
    //     const usz len = out.size();
    //     out.resize(GHTTP2_FRAME_HEADER_SIZE + 1);
    //     u8* p0 = (u8*)out.data() + len;
    //     head.writeOut(p0);
    //     p0[GHTTP2_FRAME_HEADER_SIZE] = DHTTP2_TABLE_INDEX(TAB_HTTP2_STATUS_200_INDEX); // :status 200
    //     return EE_OK;
    // }

    const usz len = out.size();
    out.reserve(len + GHTTP2_FRAME_HEADER_SIZE + 1024);
    out.resize(len + GHTTP2_FRAME_HEADER_SIZE + 2);
    u8* p0 = (u8*)out.data() + out.size() - 2;
    *p0++ = DHTTP2_TABLE_INDEX(TAB_HTTP2_STATUS_200_INDEX); // :status 200

    *p0 = GHTTP2_MASK_TABLE_NO_INDEX;
    Http2FrameHead::writeHpackStr("server", out);
    Http2FrameHead::writeHpackStr("dev", out);

    Http2FrameHead head;
    head.mLen = out.size() - len - GHTTP2_FRAME_HEADER_SIZE;
    head.mStreamID = sid;
    head.mType = H2FT_HEADERS_FRAME;
    head.mFlags = flag;
    head.writeOut((u8*)out.data() + len);
    return EE_OK;
}

s32 Http2Ping::writeOut(u8 flag, std::string& out) const {
    Http2FrameHead head;
    head.mLen = sizeof(mOpaqueData);
    head.mStreamID = 0;
    head.mType = H2FT_PING_FRAME;
    head.mFlags = flag;
    usz len = out.size();
    out.reserve(len + GHTTP2_FRAME_HEADER_SIZE + sizeof(mOpaqueData));
    head.writeOut(out);
    len = out.size();
    out.resize(len + head.mLen);
    memcpy(out.data() + len, mOpaqueData, sizeof(mOpaqueData));
    return EE_OK;
}

} // namespace app
