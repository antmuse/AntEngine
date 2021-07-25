/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#ifndef APP_MSGHEADER_H
#define APP_MSGHEADER_H

#include "Strings.h"

namespace app {

#pragma pack (1)

/**
* @brief 通用的协议包头
*/
struct MsgHeader {
    static u32 gSharedSN;
    struct Item {
        u32 mOffset;
        u32 mSize;
    };

    u32 mSize;			    //Packet total bytes
    u32 mSN;                //唯一包序
    u16 mType;			    //msg Type
    u16 mVersion;			//msg version
    u16 mItem;			    //索引区Item count
    u16 mOffset;            //索引区偏移字节量

    /**
    * @brief 无载荷的消息类型直接封包，例如: 心跳包
    */
    void finish(u32 iType, u32 iSN, u16 version = 1) {
        mSize = sizeof(*this);
        mType = (u16)iType;
        mSN = iSN;
        mVersion = version;
        mItem = 0;
        mOffset = mSize;
    }

    /**
    * @brief 有载荷的消息类型初始化
    * @param offset >=MsgHeader大小
    */
    void init(u16 iType, u16 offset, u16 item = 0, u16 version = 1) {
        DASSERT(offset >= sizeof(*this));
        mType = iType;
        mVersion = version;
        mItem = item;
        mOffset = offset;
        mSize = getIndexSize() + mOffset;
        memset(((s8*)this) + sizeof(MsgHeader), 0, mSize - sizeof(MsgHeader));
    }

    s8* getPayload()const {
        return ((s8*)this) + sizeof(*this);
    }

    u32 getPayloadSize()const {
        return mSize - sizeof(*this);
    }

    Item& getNode(u16 idx)const {
        DASSERT(mOffset > 0 && idx < mItem);
        Item* base = reinterpret_cast<Item*>(((s8*)this) + mOffset);
        return *(base + idx);
    }

    //@return 索引区大小
    u32 getIndexSize()const {
        return sizeof(Item) * mItem;
    }

    //@return 索引区末尾
    s8* getIndexEnd()const {
        DASSERT(mItem > 0);
        Item* base = reinterpret_cast<Item*>(((s8*)this) + mOffset);
        return (s8*)(base + mItem);
    }

    void removeItem(u16 idx) {
        DASSERT(idx < mItem);
        Item& nd = getNode(idx);
        if (nd.mSize > 0) {
            u32 offs = nd.mOffset;
            u32 ndsz = nd.mSize;
            nd.mOffset = 0;
            nd.mSize = 0;
            s8* pos = ((s8*)this) + offs;
            usz mvsz = mSize - (pos - (s8*)this) - ndsz;
            if (mvsz > 0) {
                memmove(pos, pos + ndsz, mvsz);
                for (idx = 0; idx < mItem; ++idx) {
                    Item& curr = getNode(idx);
                    if (curr.mOffset > offs) {
                        curr.mOffset -= ndsz;
                    }
                }
            }
        }
    }

    void appendItem(u16 idx, const void* buf, u32 size) {
        DASSERT(idx < mItem);

        Item& nd = getNode(idx);
        if (0 == nd.mSize) {
            writeItem(idx, buf, size);
            return;
        }
        const u32 offs = nd.mOffset;
        const u32 ndsz = nd.mSize;
        if (offs + ndsz >= mSize) {//just append at tail
            memcpy(((s8*)this) + mSize, buf, size);
            mSize += size;
            nd.mSize += size;
            return;
        }

        s8* pos = ((s8*)this) + offs;
        s8 tmp[256];
        s8* old = tmp;
        if (ndsz > sizeof(tmp)) {
            old = new s8[ndsz];
        }
        memcpy(old, pos, ndsz);
        removeItem(idx);
        nd.mOffset = mSize;
        nd.mSize = ndsz + size;
        memcpy(((s8*)this) + mSize, old, ndsz);
        mSize += ndsz;
        memcpy(((s8*)this) + mSize, buf, size);
        mSize += size;
        if (ndsz > sizeof(tmp)) {
            delete[] old;
        }
    }

    u32 copyItem(u16 idx, void* out, u32 iMax) {
        DASSERT(iMax > 0 && out);
        u32 size = 0;
        s8* buf = readItem(idx, size);
        if (buf) {
            size = (size < iMax ? size : iMax);
            memcpy(out, buf, size);
        }
        return size;
    }

    u32 copyStr(u16 idx, s8* out, u32 iMax) {
        DASSERT(iMax > 0 && out);
        u32 ret = copyItem(idx, out, --iMax);
        out[ret] = '\0';
        return ret;
    }

    u32 copyWStr(u16 idx, wchar_t* out, u32 iMax) {
        DASSERT(iMax > 0 && out);
        --iMax;
        u32 ret = copyItem(idx, out, iMax * sizeof(*out));
        ret /= sizeof(*out);
        out[ret] = L'\0';
        return ret;
    }

    void writeStr(u16 idx, const s8* str) {
        writeItem(idx, str, (u32)(sizeof(*str) + strlen(str)));
    }

    void writeWStr(u16 idx, const wchar_t* str) {
        writeItem(idx, str, (u32)(sizeof(*str) * (wcslen(str) + 1)));
    }

    void writeItem(u16 idx, const void* buf, u32 size) {
        DASSERT(idx < mItem&& buf);
        Item& nd = getNode(idx);
        nd.mOffset = mSize;
        nd.mSize = size;
        memcpy(((s8*)this) + mSize, buf, size);
        mSize += size;
    }

    s8* readItem(u16 idx, u32& size)const {
        DASSERT(idx < mItem);
        Item& nd = getNode(idx);
        size = nd.mSize;
        return nd.mSize > 0 ? ((s8*)this) + nd.mOffset : nullptr;
    }

    StringView readItem(u16 idx)const {
        DASSERT(idx < mItem);
        StringView ret;
        Item& nd = getNode(idx);
        ret.mLen = nd.mSize;
        ret.mData = nd.mSize > 0 ? ((s8*)this) + nd.mOffset : nullptr;
        return ret;
    }
};

#pragma pack ()

} //namespace app

#endif //APP_MSGHEADER_H

