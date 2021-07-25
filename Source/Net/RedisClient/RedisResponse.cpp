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


#include "Net/RedisClient/RedisResponse.h"
#include "Converter.h"
#include <string.h>


namespace app {
namespace net {

//const static u32 valsize = sizeof(RedisResponse);

RedisResponse::RedisResponse(MemoryHub& it) :
    mUsed(0),
    mAllocated(0),
    mHub(it),
    mStatus(EDS_INIT),
    mType(ERRT_NIL) {
    mHub.grab();
    mValue.mNodes = nullptr;
}

RedisResponse::~RedisResponse() {
    clear();
    mHub.drop();
}

void RedisResponse::clear() {
    if (EDS_INIT == mStatus) {
        return;
    }
    switch (mType) {
    case ERRT_ERROR:
    case ERRT_STRING:
    case ERRT_BULK_STR:
        mHub.release(mValue.mValStr);
        break;

    case ERRT_ARRAY:
        if (mValue.mNodes) {
            for (u32 i = 0; i < mAllocated; ++i) {
                if (mValue.mNodes[i]) {
                    RedisResponse* nd = reinterpret_cast<RedisResponse*>(mValue.mNodes[i]);
                    nd->clear();
                    mHub.release(nd);
                    mHub.drop();
                }
            }
            mHub.release(mValue.mNodes);
            //mValue.mNodes = nullptr;
        }
        break;

    case ERRT_INT:
    case ERRT_NIL:
    default:
        break;
    }//switch
    mStatus = EDS_INIT;
    mUsed = 0;
    mAllocated = 0;
    mType = ERRT_NIL;
    mValue.mValStr = nullptr;
}

bool RedisResponse::decodeSize(const s8 ch) {
    if ('\r' == ch) {
        return false;
    }
    if ('\n' != ch) {
        mValue.mVal64 *= 10;
        mValue.mVal64 += ch - '0';
    }
    return ('\n' == ch);
}

const s8* RedisResponse::import(const s8* str, u64 len) {
    const s8* const end = str + len;
    for (; str < end; ++str) {
        switch (mStatus) {
        case EDS_INT:
            if (decodeSize(str[0])) {
                mStatus = EDS_DONE;
            }
            break;

        case EDS_INIT:
            mType = static_cast<ERedisResultType>(str[0]);
            switch (mType) {
            case ERRT_ARRAY:
                mStatus = EDS_ARRAY_SIZE;
                break;
            case ERRT_ERROR:
            case ERRT_STRING:
                mAllocated = 120;
                mValue.mValStr = reinterpret_cast<s8*>(mHub.allocateAndClear(mAllocated));
                mStatus = EDS_STR;
                break;
            case ERRT_BULK_STR:
                mStatus = EDS_BULK_SIZE;
                break;
            case ERRT_INT:
                mStatus = EDS_INT;
                break;
            default:
                DASSERT(0);
                break;
            }
            continue;//skip \r

        case EDS_BULK_STR:
            if (mUsed + 1 < mAllocated) {
                mValue.mValStr[mUsed++] = str[0];
            } else {
                if ('\n' == str[0]) {
                    mValue.mValStr[mUsed] = 0;
                    mStatus = EDS_DONE;
                }
            }
            break;

        case EDS_STR:
            if ('\n' == str[0]) {
                mValue.mValStr[mUsed] = 0;
                mStatus = EDS_DONE;
            } else if ('\r' != str[0]) {
                if (mUsed + 1 >= mAllocated) {
                    mAllocated += 32;
                    s8* nstr = reinterpret_cast<s8*>(mHub.allocateAndClear(mAllocated));
                    memcpy(nstr, mValue.mValStr, mUsed);
                    mHub.release(mValue.mValStr);
                    mValue.mValStr = nstr;
                    mValue.mValStr[mUsed++] = str[0];
                }
                mValue.mValStr[mUsed++] = str[0];
            }
            break;

            //case EDS_ARRAY:
        case EDS_ARRAY_SIZE:
            if (decodeSize(str[0])) {
                if(0 == mValue.mVal64) {
                    mStatus = EDS_DONE;
                    break;
                }
                mStatus = EDS_ARRAY;
                mUsed = 0;
                mAllocated = mValue.mVal64;
                mValue.mNodes = reinterpret_cast<RedisResponse**>
                    (mHub.allocateAndClear(sizeof(RedisResponse*)*mAllocated));
                for (u32 i = 0; i < mAllocated; ++i) {
                    mValue.mNodes[i] = new
                    (reinterpret_cast<RedisResponse*>(mHub.allocateAndClear(sizeof(RedisResponse)))) RedisResponse(mHub);
                }
            }
            break;

        case EDS_ARRAY:
        {
            RedisResponse* nd = reinterpret_cast<RedisResponse*>(mValue.mNodes[mUsed]);
            str = nd->import(str, end - str);
            if (nd->isFinished()) {
                if (++mUsed == mAllocated) {
                    mStatus = EDS_DONE;
                }
                --str;
            }
            break;
        }

        case EDS_BULK_SIZE:
            if (decodeSize(str[0])) {
                mStatus = EDS_BULK_STR;
                mUsed = 0;
                if (mValue.mVal64 > 0) {
                    mAllocated = mValue.mVal64 + 1;
                    mValue.mValStr = reinterpret_cast<s8*>(mHub.allocateAndClear(mAllocated));
                } else if (0 == mValue.mVal64) {
                    mStatus = EDS_DONE; //empty str
                } else {
                    mType = ERRT_NIL;
                    mStatus = EDS_DONE;
                }
            }
            break;

        case EDS_DONE:
            return str;

        default:
            DASSERT(0);
            break;
        }//switch status
    }//for

    return str;
}

void RedisResponse::makeError(const s8* str) {
    str = (nullptr == str ? "unknown" : str);
    clear();
    mStatus = EDS_DONE;
    mType = ERRT_ERROR;
    mUsed = static_cast<u32>(strlen(str));
    mAllocated = mUsed + 1;
    mValue.mValStr = mHub.allocate(mAllocated);
    memcpy(mValue.mValStr, str, mAllocated);
}

void RedisResponse::show(u32 level, u32 index)const {
    if (EDS_DONE != mStatus) {
        printf("status!=EDS_DONE\n");
        return;
    }
    for (u32 i = 1; i < level; ++i) {
        printf("--");
    }
    switch (mType) {
    case ERRT_ARRAY:
        printf("[%u.%u][array]=%u\n", level, index, mUsed);
        for (u32 i = 0; i < mUsed; ++i) {
            RedisResponse* nd = reinterpret_cast<RedisResponse*>(mValue.mNodes[i]);
            if (nd) {
                nd->show(level + 1, i + 1);
            }
        }
        break;
    case ERRT_ERROR:
    case ERRT_STRING:
    case ERRT_BULK_STR:
        printf("[%u.%u][string]=%s\n", level, index, mValue.mValStr);
        break;
    case ERRT_INT:
        printf("[%u.%u][int]=%lld\n", level, index, mValue.mVal64);
        break;
    case ERRT_NIL:
        printf("[%u.%u][nil]=NULL\n", level, index);
        break;
    }
}

} //namespace net {
} // namespace app