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


#ifndef APP_REDISRESPONSE_H
#define	APP_REDISRESPONSE_H

#include "MemoryHub.h"

namespace app {
namespace net {

class RedisRequest;
class RedisResponse;
class RedisClient;

///A callable function pointer for RedisResult.
typedef void(*AppRedisCaller)(RedisRequest* it, RedisResponse*);

enum ERedisResultType {
    ERRT_NIL,   //NULL, not exist
    ERRT_ERROR = '-',
    ERRT_STRING = '+',
    ERRT_BULK_STR = '$',
    ERRT_INT = ':',
    ERRT_ARRAY = '*'
};

class RedisResponse {
public:
    union SRedisValue {
        s64 mVal64;
        s8* mValStr;
        RedisResponse** mNodes; //only for ERRT_ARRAY
    };
    SRedisValue mValue;
    u32 mUsed;
    u32 mAllocated;
    MemoryHub& mHub;
    u8 mType;
    u8 mStatus; //@see EDecodeStatus

    RedisResponse(MemoryHub& it);
    ~RedisResponse();

    void clear();

    s8* getStr()const {
        return mValue.mValStr;
    }

    s64 getS64()const {
        return mValue.mVal64;
    }

    bool isFinished()const {
        return EDS_DONE == mStatus;
    }

    bool isError()const {
        return (ERRT_ERROR == mType);
    }

    bool isOK()const {
        return (ERRT_STRING == mType && mValue.mValStr
            && (*(u16*)mValue.mValStr) == *(u16*)("OK"));
    }

    //eg: MOVED 153 127.0.0.1:3292
    bool isMoved() const {
        return (ERRT_ERROR == mType && mValue.mValStr
            && (*(u32*)mValue.mValStr) == *(u32*)("MOVE")
            && (*(u16*)(mValue.mValStr + sizeof(u32))) == *(u16*)("D "));
    }

    //eg: ASK 153 127.0.0.1:3292
    bool isAsk() const {
        return (ERRT_ERROR == mType && mValue.mValStr
            && (*(u32*)mValue.mValStr) == *(u32*)("ASK "));
    }

    /**
    * @brief decode a item in stream.
    * eg: stream is "+OK\r\n" or "$3\r\nabc\r\n"
    *     \p str point to the start of stream.
    *     \p len the position of stream's last '\n'(position start at 0).
    * @return the end position;
    */
    const s8* import(const s8* str, u64 len);

    void show(u32 level = 1, u32 index = 1)const;

    void makeError(const s8* str);

    RedisResponse* getResult(u32 idx)const {
        if (ERRT_ARRAY != mType || EDS_DONE != mStatus || nullptr == mValue.mNodes) {
            return nullptr;
        }
        if (idx < mAllocated) {
            return mValue.mNodes[idx];
        }
        return nullptr;
    }

private:
    bool decodeSize(const s8 ch);

    enum EDecodeStatus {
        EDS_INIT = 0,
        EDS_INT,
        EDS_ARRAY,
        EDS_ARRAY_SIZE,
        EDS_STR,
        EDS_BULK_STR,
        EDS_BULK_SIZE,
        //EDS_TAIL,   // "\r\n"
        EDS_DONE = 0xFF
    };

    RedisResponse(const RedisResponse&) = delete;
    const RedisResponse& operator=(const RedisResponse&) = delete;
};


} //namespace net
} //namespace app

#endif //APP_REDISRESPONSE_H