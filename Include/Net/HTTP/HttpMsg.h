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


#ifndef APP_HTTPMSG_H
#define	APP_HTTPMSG_H

#include "Packet.h"
#include "Net/HTTP/HttpURL.h"
#include "Net/HTTP/HttpHead.h"

namespace app {
namespace net {

class HttpMsg {
public:
    HttpMsg();

    ~HttpMsg();

    static StringView getMethodStr(http_method it);

    /**
    * @brief 支持<=18的后缀
    * @param filename, 文件名, 可以不以\0结尾，不用在意大小写。
    * @return mime if success, else "application/octet-stream" as default.
    */
    static const StringView getMimeType(const s8* filename, usz iLen);

    /**
    * @param flag, 组合值, 0=只写入, 1=写入并加索引, 2=结束
    */
    void writeLength(usz sz, s32 flag = 0) {
        s8 tmp[128];
        snprintf(tmp, sizeof(tmp), "%llu", sz);
        writeHead("Content-Length", tmp, flag);
    }


    void writeContentRange(usz total, usz start, usz stop, s32 flag = 0) {
        s8 tmp[128];
        snprintf(tmp, sizeof(tmp), "bytes %llu-%llu/%llu", start, stop, total);
        writeHead("Content-Range", tmp, flag);
    }



    /**
    * @param flag, 组合值, 0=只写入, 1=写入并加索引, 2=结束
    */
    void writeHead(const s8* name, const s8* value, s32 flag = 0);

    /**
    * @param flag, 组合值, 0=写入并结束, 1=写入并加Chunk
    */
    void writeBody(const void* buf, usz bsz, s32 flag = 0);


    Packet& getCache() {
        return mCache;
    }

    HttpHead& getHead() {
        return mHead;
    }

    StringView getBody() const {
        DASSERT(mCache.size() >= mHeadSize);
        StringView ret(mCache.getPointer() + mHeadSize, mCache.size() - mHeadSize);
        return ret;
    }

    void onHeadName(const void* buf, usz sz) {
        mTemp.set((s8*)mCache.size(), sz);
        mCache.write(buf, sz);
        mCache.writeU8(':');
    }

    //Range: bytes=200-1000, 2000-6576, 19000-
    void onHeadValue(const void* buf, usz sz) {
        StringView vvv((s8*)mCache.size(), sz);
        mCache.write(buf, sz);
        mCache.writeU16(App2Char2S16("\r\n"));
        mHead.add(mTemp, vvv);
    }

    void clear() {
        mFlag = 0;
        mHeadSize = 0;
        mHead.clear();
        mCache.shrink(512);
    }

    /*
     @return 0=success, 1=skip body, 2=upgrade, else error
    */
    s32 onHeadFinish(s32 flag, ssz bodySize);

    void setKeepAlive(bool it) {
        if(it) {
            mFlag &= ~F_CONNECTION_CLOSE;
            mFlag |= F_CONNECTION_KEEP_ALIVE;
            writeHead("Connection", "keep-alive");
        } else {
            mFlag &= ~F_CONNECTION_KEEP_ALIVE;
            mFlag |= F_CONNECTION_CLOSE;
            writeHead("Connection", "close");
        }
    }

    void setChunked(s32 flag = 0) {
        mFlag |= F_CHUNKED;
        writeHead("Transfer-Encoding", "chunked", flag);
    }

    bool isKeepAlive()const {
        return (F_CONNECTION_KEEP_ALIVE & mFlag) > 0;
    }

    //test page: http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx
    //http://www.xingkong.com/
    bool isChunked()const {
        return (F_CHUNKED & mFlag) > 0;
    }

    void clearBody() {
        mCache.resize(mHeadSize);
    }

    usz getHeadSize()const {
        return mHeadSize;
    }

    usz getBodySize()const {
        return mCache.size() - mHeadSize;
    }

    void onChunkHead(usz chunksz) {
        mCache.reallocate(mHeadSize + chunksz);
    }


protected:
    s32 mFlag;
    u32 mHeadSize;
    StringView mTemp;
    Packet mCache;
    HttpHead mHead;
};


}//namespace net
}//namespace app


#endif //APP_HTTPMSG_H