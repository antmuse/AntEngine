#include "Net/NetProtocal.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//#define APP_DEBUG_PRINT_ON

namespace app {
namespace net {

const u32 APP_NET_NODELAY_MIN_RTO = 30;				    ///< no delay min RTO
const u32 APP_NET_NORMAL_MIN_RTO = 100;					///< normal min RTO
const u32 APP_NET_MAX_RTO = 60000;						///<Max RTO
const u32 APP_NET_DEFAULT_RTO = 200;					///<Default RTO

const u32 APP_NET_CMD_PUSH_DATA = 81;					///< cmd: push data
const u32 APP_NET_CMD_ACK = 82;							///< cmd: ack
const u32 APP_NET_CMD_ASK_WINDOW = 83;					///< cmd: window probe (ask)
const u32 APP_NET_CMD_TELL_WINDOW = 84;					///< cmd: window size (tell)

const u32 APP_NET_ENDIANASK_SEND = 1;					// need to send APP_NET_CMD_ASK_WINDOW
const u32 APP_NET_ENDIANASK_TELL = 2;					// need to send APP_NET_CMD_TELL_WINDOW
const u32 APP_NET_DEFAULT_SWINDOW = 32;					///<default send window size
const u32 APP_NET_DEFAULT_RWINDOW = 32;					///<default receive window size
const u32 APP_NET_DEFAULT_MTU = 1400;					///<default MTU
const u32 APP_NET_DEFAULT_INTERVAL_TIME = 100;			///<Default interval time of update steps.
const u32 APP_NET_ENDIANOVERHEAD = 24;
const u32 APP_NET_DEFAULT_DEAD_LINK = 20;				///<Default dead link
const u32 APP_NET_DEFAULT_SSTHRESHOLD = 2;				///<Default Slow start threshold
const u32 APP_NET_MIN_SSTHRESHOLD = 2;					///<Min Slow start threshold
const u32 APP_NET_PROBE_TIME = 7000;					///<7 secs to probe window size
const u32 APP_NET_PROBE_TIME_LIMIT = 120000;			///<Probe time limit. up to 120 secs to probe window

static inline u64 AppValueDiff(u64 later, u64 earlier) {
    return (later - earlier);
}

// output queue
#if defined(APP_DEBUG_PRINT_ON)
void AppPrintQueue(const s8* name, const Node2* head) {
    const Node2* p;
    printf("<%s>: [", name);
    for (; !head->empty(); ) {
        p = head->getNext();
        //const SKCPSegment *seg = APP_GET_VALUE_POINTER(p, const SKCPSegment, mNode);
        const NetProtocal::SKCPSegment* seg = (const NetProtocal::SKCPSegment*) (p + 1);
        printf("(%lu %d),", (unsigned long)seg->mSN, (s32)(seg->mTime % 10000));
    }
    printf("]\n");
}
#endif

DINLINE void* NetProtocal::allocateMemory(s32 size) {
    return malloc(size);
}

DINLINE void NetProtocal::releaseMemory(void* ptr) {
    free(ptr);
}

// allocate a new kcp segment
Node2* NetProtocal::createSegment(s32 size) {
    return (Node2*)allocateMemory(size + sizeof(SKCPSegment));
}


// write log
void NetProtocal::log(s32 mask, const s8* fmt, ...) {
    if ((mask & mLogMask) == 0 || mLogWriter == 0) {
        return;
    }
    s8 iBuffer[1024];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(iBuffer, 1024, fmt, argptr);
    va_end(argptr);
    mLogWriter(iBuffer, this, mUserPointer);
}

// output segment
s32 NetProtocal::sendOut(const s8* data, s32 size) {
    DASSERT(mSender);
    if (size <= 0) {
        return 0;
    }
    log(ENET_LOG_OUTPUT, "[RO] %ld bytes", (long)size);
    return mSender->sendBuffer(mUserPointer, data, size);
}


NetProtocal::NetProtocal() :
    mConnectionID(0),
    mUserPointer(0),
    mSendUNA(0),
    mSendNext(0),
    mReceiveNext(0),
    mTimeProbe(0),
    mTimeProbeWait(0),
    mMaxSendWindowSize(APP_NET_DEFAULT_SWINDOW),
    mMaxReceiveWindowSize(APP_NET_DEFAULT_RWINDOW),
    mWindowRemote(APP_NET_DEFAULT_RWINDOW),
    mWindowCongestion(0),
    mIncrease(0),
    mProbeFlag(0),
    mMTU(APP_NET_DEFAULT_MTU),
    mStreamMode(false),
    mReceiveBufferCount(0),
    mSendBufferCount(0),
    mReceiveQueueCount(0),
    mSendQueueCount(0),
    mState(0),
    mListACK(0),
    mBlockACK(0),
    mCountACK(0),
    mSRTT(0),
    mRTT(0),
    mRTO(APP_NET_DEFAULT_RTO),
    mMinRTO(APP_NET_NORMAL_MIN_RTO),
    mTimeCurrent(0),
    mInterval(APP_NET_DEFAULT_INTERVAL_TIME),
    mTimeFlush(APP_NET_DEFAULT_INTERVAL_TIME),
    mNodelay(0),
    mUpdated(0),
    mLogMask(0),
    mSSThreshold(APP_NET_DEFAULT_SSTHRESHOLD),
    mFastResend(0),
    mNoCongestionControl(0),
    mXMIT(0),
    mDeadLink(APP_NET_DEFAULT_DEAD_LINK),
    mSender(0),
    mLogWriter(0) {

    mMSS = mMTU - APP_NET_ENDIANOVERHEAD;
    mBuffer = (s8*)allocateMemory((mMTU + APP_NET_ENDIANOVERHEAD) * 3);
    if (!mBuffer) {
        return;
    }
    mQueueSend.clear();
    mQueueReceive.clear();
    mQueueSendBuffer.clear();
    mQueueReceiveBuffer.clear();
}


NetProtocal::~NetProtocal() {
    mQueueSendBuffer.clear(mFreeHook);
    mQueueReceiveBuffer.clear(mFreeHook);
    mQueueSend.clear(mFreeHook);
    mQueueReceive.clear(mFreeHook);
    if (mBuffer) {
        releaseMemory(mBuffer);
    }
    if (mListACK) {
        releaseMemory(mListACK);
    }
    mReceiveBufferCount = 0;
    mSendBufferCount = 0;
    mReceiveQueueCount = 0;
    mSendQueueCount = 0;
    mCountACK = 0;
    mBuffer = 0;
    mListACK = 0;
}


u32 NetProtocal::getID(const s8* iBuffer) const {
    u32 conv;
    core::AppDecodeU32(iBuffer, &conv);
    return conv;
}


//---------------------------------------------------------------------
// set output callback, which will be invoked by kcp
//---------------------------------------------------------------------
void NetProtocal::setSender(INetDataSender* iSender) {
    mSender = iSender;
}


//---------------------------------------------------------------------
// user/upper level recv: returns size, returns below zero for EAGAIN
//---------------------------------------------------------------------
s32 NetProtocal::receiveData(s8* iBuffer, s32 len) {
    if (mQueueReceive.empty()) {
        return -1;
    }
    s32 peeksize = peekNextSize();
    if (peeksize < 0) return -2;

    if (peeksize > len) return -3;

    s32 ispeek = (len < 0) ? 1 : 0;
    s32 recover = 0;
    SKCPSegment* seg;

    if (len < 0) len = -len;

    if (mReceiveQueueCount >= mMaxReceiveWindowSize)  recover = 1;

    // merge fragment
    Node2* next;
    s32 fragment;
    len = 0;
    for (Node2* p = mQueueReceive.getNext(); p != &mQueueReceive; p = next) {
        seg = (SKCPSegment*)(p + 1);
        next = p->getNext();

        if (iBuffer) {
            memcpy(iBuffer, seg->mData, seg->mLength);
            iBuffer += seg->mLength;
        }

        len += seg->mLength;
        fragment = seg->mFrag;

        log(ENET_LOG_RECEIVE, "recv sn=%lu", seg->mSN);

        if (ispeek == 0) {
            p->delink();
            Node2::deleteNode(p, mFreeHook);
            mReceiveQueueCount--;
        }

        if (fragment == 0)
            break;
    }//for

    DASSERT(len == peeksize);

    // move available data from mQueueReceiveBuffer -> mQueueReceive
    Node2* node;
    while (!mQueueReceiveBuffer.empty()) {
        node = mQueueReceiveBuffer.getNext();
        SKCPSegment* seg = (SKCPSegment*)(node + 1);
        if (seg->mSN == mReceiveNext && mReceiveQueueCount < mMaxReceiveWindowSize) {
            node->delink();
            mReceiveBufferCount--;
            mQueueReceive.pushBack(*node);
            mReceiveQueueCount++;
            mReceiveNext++;
        } else {
            break;
        }
    }

    // fast recover
    if (mReceiveQueueCount < mMaxReceiveWindowSize && recover) {
        // ready to send back APP_NET_CMD_TELL_WINDOW in flush, tell remote my window size
        mProbeFlag |= APP_NET_ENDIANASK_TELL;
    }

    return len;
}


//---------------------------------------------------------------------
// peek data size
//---------------------------------------------------------------------
s32 NetProtocal::peekNextSize() {
    if (mQueueReceive.empty()) {
        return -1;
    }

    SKCPSegment* seg = (SKCPSegment*)(mQueueReceive.getNext() + 1);
    if (seg->mFrag == 0) {
        return seg->mLength;
    }

    if (mReceiveQueueCount < seg->mFrag + 1) {
        return -1;
    }

    s32 length = 0;

    for (Node2* p = mQueueReceive.getNext(); p != &mQueueReceive; p = p->getNext()) {
        seg = (SKCPSegment*)(p + 1);
        length += seg->mLength;
        if (seg->mFrag == 0) {
            break;
        }
    }

    return length;
}


//---------------------------------------------------------------------
// user/upper level send, returns below zero for error
//---------------------------------------------------------------------
s32 NetProtocal::sendData(const s8* iBuffer, s32 len) {
    DASSERT(mMSS > 0);
    if (len < 0) {
        return -1;
    }
    Node2* node;
    Node2* newnode;
    SKCPSegment* seg;
    s32 count;

    // append to previous segment in streaming mode (if possible)
    if (mStreamMode) {
        if (!mQueueSend.empty()) {
            node = mQueueSend.getPrevious();
            SKCPSegment* old = (SKCPSegment*)(node + 1);
            if (old->mLength < mMSS) {
                s32 capacity = mMSS - old->mLength;
                s32 extend = (len < capacity) ? len : capacity;
                newnode = createSegment(old->mLength + extend);
                seg = (SKCPSegment*)(newnode + 1);
                DASSERT(seg);
                if (seg == NULL) {
                    return -2;
                }
                mQueueSend.pushBack(*newnode);
                memcpy(seg->mData, old->mData, old->mLength);
                if (iBuffer) {
                    memcpy(seg->mData + old->mLength, iBuffer, extend);
                    iBuffer += extend;
                }
                seg->mLength = old->mLength + extend;
                seg->mFrag = 0;
                len -= extend;
                node->delink();
                Node2::deleteNode(node, mFreeHook);
            }
        }//if
        if (len <= 0) {
            return 0;
        }
    }//if

    if (len <= (s32)mMSS) {
        count = 1;
    } else {
        count = (len + mMSS - 1) / mMSS;
    }

    if (count > 255) return -2;

    if (count == 0) count = 1;

    // fragment
    for (s32 i = 0; i < count; i++) {
        s32 size = len > (s32)mMSS ? (s32)mMSS : len;
        newnode = createSegment(size);
        seg = (SKCPSegment*)(newnode + 1);
        DASSERT(seg);
        if (0 == seg) {
            return -2;
        }
        if (iBuffer && len > 0) {
            memcpy(seg->mData, iBuffer, size);
        }
        seg->mLength = size;
        seg->mFrag = (mStreamMode ? 0 : (count - i - 1));
        mQueueSend.pushBack(*newnode);
        mSendQueueCount++;
        if (iBuffer) {
            iBuffer += size;
        }
        len -= size;
    }

    return 0;
}


//---------------------------------------------------------------------
// parse ack
//---------------------------------------------------------------------
void NetProtocal::updateACK(s32 rtt) {
    s32 rto = 0;
    if (mSRTT == 0) {
        mSRTT = rtt;
        mRTT = rtt / 2;
    } else {
        long delta = rtt - mSRTT;
        if (delta < 0) delta = -delta;
        mRTT = (3 * mRTT + delta) / 4;
        mSRTT = (7 * mSRTT + rtt) / 8;
        if (mSRTT < 1) mSRTT = 1;
    }
    rto = mSRTT + AppMin<u32>(mInterval, 4 * mRTT);
    mRTO = AppClamp<u32>(rto, mMinRTO, APP_NET_MAX_RTO);
}


void NetProtocal::shrinkBuffer() {
    Node2* p = mQueueSendBuffer.getNext();
    if (p != &mQueueSendBuffer) {
        SKCPSegment* seg = (SKCPSegment*)(p + 1);
        mSendUNA = seg->mSN;
    } else {
        mSendUNA = mSendNext;
    }
}


void NetProtocal::parseACK(u32 sn) {
    if (AppValueDiff(sn, mSendUNA) < 0 || AppValueDiff(sn, mSendNext) >= 0) {
        return;
    }
    SKCPSegment* seg;
    Node2* next;
    for (Node2* p = mQueueSendBuffer.getNext(); p != &mQueueSendBuffer; p = next) {
        next = p->getNext();
        seg = (SKCPSegment*)(p + 1);
        if (sn == seg->mSN) {
            p->delink();
            Node2::deleteNode(p, mFreeHook);
            mSendBufferCount--;
            break;
        }
        if (AppValueDiff(sn, seg->mSN) < 0) {
            break;
        }
    }//for
}


void NetProtocal::parseUNA(u32 una) {
    SKCPSegment* seg;
    Node2* next;
    for (Node2* p = mQueueSendBuffer.getNext(); p != &mQueueSendBuffer; p = next) {
        next = p->getNext();
        seg = (SKCPSegment*)(p + 1);
        if (AppValueDiff(una, seg->mSN) > 0) {
            p->delink();
            Node2::deleteNode(p, mFreeHook);
            mSendBufferCount--;
        } else {
            break;
        }
    }
}


void NetProtocal::parseFastACK(u32 sn) {
    if (AppValueDiff(sn, mSendUNA) < 0 || AppValueDiff(sn, mSendNext) >= 0) {
        return;
    }
    SKCPSegment* seg;
    Node2* next;
    for (Node2* p = mQueueSendBuffer.getNext(); p != &mQueueSendBuffer; p = next) {
        next = p->getNext();
        seg = (SKCPSegment*)(p + 1);
        if (AppValueDiff(sn, seg->mSN) < 0) {
            break;
        } else if (sn != seg->mSN) {
            seg->mFastACK++;
        }
    }
}


void NetProtocal::appendACK(u32 sn, u32 ts) {
    size_t newsize = mCountACK + 1;
    u32* ptr;

    if (newsize > mBlockACK) {
        u32* acklist;
        size_t newblock;

        for (newblock = 8; newblock < newsize; newblock <<= 1) {
        }

        acklist = (u32*)allocateMemory(newblock * sizeof(u32) * 2);

        if (acklist == NULL) {
            DASSERT(acklist != NULL);
            abort();
        }

        if (mListACK != NULL) {
            for (u32 x = 0; x < mCountACK; x++) {
                acklist[x * 2 + 0] = mListACK[x * 2 + 0];
                acklist[x * 2 + 1] = mListACK[x * 2 + 1];
            }
            releaseMemory(mListACK);
        }

        mListACK = acklist;
        mBlockACK = newblock;
    } //if

    ptr = &mListACK[mCountACK * 2];
    ptr[0] = sn;
    ptr[1] = ts;
    mCountACK++;
}


void NetProtocal::getACK(s32 p, u32* sn, u32* ts) {
    if (sn) {
        sn[0] = mListACK[p * 2 + 0];
    }
    if (ts) {
        ts[0] = mListACK[p * 2 + 1];
    }
}


//---------------------------------------------------------------------
// parse data
//---------------------------------------------------------------------
void NetProtocal::parseSegment(Node2* node) {
    SKCPSegment* newseg = (SKCPSegment*)(node + 1);
    u32 sn = newseg->mSN;

    if (AppValueDiff(sn, mReceiveNext + mMaxReceiveWindowSize) >= 0 ||
        AppValueDiff(sn, mReceiveNext) < 0) {
        node->delink();
        Node2::deleteNode(node, mFreeHook);
        return;
    }

    s32 repeat = 0;
    Node2* p;
    Node2* prev;
    for (p = mQueueReceiveBuffer.getPrevious(); p != &mQueueReceiveBuffer; p = prev) {
        prev = p->getPrevious();
        SKCPSegment* seg = (SKCPSegment*)(p + 1);
        if (seg->mSN == sn) {
            repeat = 1;
            break;
        }
        if (AppValueDiff(sn, seg->mSN) > 0) {
            break;
        }
    }//for

    if (repeat == 0) {
        p->pushFront(*node);
        mReceiveBufferCount++;
    } else {
        node->delink();
        Node2::deleteNode(node, mFreeHook);
    }

#if defined(APP_DEBUG_PRINT_ON)
    AppPrintQueue("rcvbuf", &mQueueReceiveBuffer);
    printf("rcv_nxt=%lu\n", mReceiveNext);
#endif

    // move available data from mQueueReceiveBuffer -> mQueueReceive
    while (!mQueueReceiveBuffer.empty()) {
        node = mQueueReceiveBuffer.getNext();
        SKCPSegment* seg = (SKCPSegment*)(node->getValue());
        if (seg->mSN == mReceiveNext && mReceiveQueueCount < mMaxReceiveWindowSize) {
            node->delink();
            mReceiveBufferCount--;
            mQueueReceive.pushBack(*node);
            mReceiveQueueCount++;
            mReceiveNext++;
        } else {
            break;
        }
    }//while

#if defined(APP_DEBUG_PRINT_ON)
    AppPrintQueue("queue", &mQueueReceive);
    printf("rcv_nxt=%lu\n", mReceiveNext);
#endif

#if defined(APP_DEBUG_PRINT_ON)
    //	printf("snd(buf=%d, queue=%d)\n", mSendBufferCount, mSendQueueCount);
    //	printf("rcv(buf=%d, queue=%d)\n", mReceiveBufferCount, mReceiveQueueCount);
#endif
}


//---------------------------------------------------------------------
// import raw data
//---------------------------------------------------------------------
s32 NetProtocal::import(const s8* data, long size) {
    u32 una = mSendUNA;
    u32 maxack = 0;
    s32 flag = 0;

    log(ENET_LOG_INPUT, "[RI] %d bytes", size);

    if (data == NULL || size < APP_NET_ENDIANOVERHEAD) return -1;

    Node2* node;

    for (;;) {
        u32 ts;
        u32 sn, len, una, conv;
        u16 wnd;
        u8 cmd, frg;
        SKCPSegment* seg;

        if (size < (s32)APP_NET_ENDIANOVERHEAD) break;

        data = core::AppDecodeU32(data, &conv);
        if (conv != mConnectionID) {
            return -1;
        }
        data = core::AppDecodeU8(data, &cmd);
        data = core::AppDecodeU8(data, &frg);
        data = core::AppDecodeU16(data, &wnd);
        data = core::AppDecodeU32(data, &ts);
        data = core::AppDecodeU32(data, &sn);
        data = core::AppDecodeU32(data, &una);
        data = core::AppDecodeU32(data, &len);

        size -= APP_NET_ENDIANOVERHEAD;

        if ((long)size < (long)len) return -2;

        if (cmd != APP_NET_CMD_PUSH_DATA &&
            cmd != APP_NET_CMD_ACK &&
            cmd != APP_NET_CMD_ASK_WINDOW &&
            cmd != APP_NET_CMD_TELL_WINDOW) {
            return -3;
        }

        mWindowRemote = wnd;
        parseUNA(una);
        shrinkBuffer();

        if (cmd == APP_NET_CMD_ACK) {
            if ((mTimeCurrent - ts) >= 0) {
                updateACK((mTimeCurrent - ts));
            }
            parseACK(sn);
            shrinkBuffer();
            if (flag == 0) {
                flag = 1;
                maxack = sn;
            } else {
                if ((sn - maxack) > 0) {
                    maxack = sn;
                }
            }
            log(ENET_LOG_IN_DATA, "input ack: sn=%lu rtt=%ld rto=%ld", sn,
                (long)(mTimeCurrent - ts), (long)mRTO);
        } else if (cmd == APP_NET_CMD_PUSH_DATA) {
            log(ENET_LOG_IN_DATA, "input psh: sn=%lu ts=%lu", sn, ts);
            if (AppValueDiff(sn, mReceiveNext + mMaxReceiveWindowSize) < 0) {
                appendACK(sn, ts);
                if ((sn - mReceiveNext) >= 0) {
                    //seg = createSegment(len);
                    node = createSegment(len);
                    seg = (SKCPSegment*)(node+1);
                    seg->mConnectionID = conv;
                    seg->mCMD = cmd;
                    seg->mFrag = frg;
                    seg->mWindow = wnd;
                    seg->mTime = ts;
                    seg->mSN = sn;
                    seg->mUNA = una;
                    seg->mLength = len;

                    if (len > 0) {
                        memcpy(seg->mData, data, len);
                    }

                    parseSegment(node);
                }
            }
        } else if (cmd == APP_NET_CMD_ASK_WINDOW) {
            // ready to send back APP_NET_CMD_TELL_WINDOW in flush
            // tell remote my window size
            mProbeFlag |= APP_NET_ENDIANASK_TELL;
            log(ENET_LOG_IN_PROBE, "input probe");
        } else if (cmd == APP_NET_CMD_TELL_WINDOW) {
            // do nothing
            log(ENET_LOG_IN_WINDOW, "input wins: %lu", (u32)(wnd));
        } else {
            return -3;
        }

        data += len;
        size -= len;
    } //for

    if (flag != 0) {
        parseFastACK(maxack);
    }

    if (AppValueDiff(mSendUNA, una) > 0) {
        if (mWindowCongestion < mWindowRemote) {
            u32 mss = mMSS;
            if (mWindowCongestion < mSSThreshold) {
                mWindowCongestion++;
                mIncrease += mss;
            } else {
                if (mIncrease < mss) mIncrease = mss;
                mIncrease += (mss * mss) / mIncrease + (mss / 16);
                if ((mWindowCongestion + 1) * mss <= mIncrease) {
                    mWindowCongestion++;
                }
            }
            if (mWindowCongestion > mWindowRemote) {
                mWindowCongestion = mWindowRemote;
                mIncrease = mWindowRemote * mss;
            }
        }
    }

    return 0;
}



s8* NetProtocal::encodeSegment(s8* ptr, const SKCPSegment* seg) {
    ptr = core::AppEncodeU32(seg->mConnectionID, ptr);
    ptr = core::AppEncodeU8((u8)seg->mCMD, ptr);
    ptr = core::AppEncodeU8((u8)seg->mFrag, ptr);
    ptr = core::AppEncodeU16((u16)seg->mWindow, ptr);
    ptr = core::AppEncodeU32(seg->mTime, ptr);
    ptr = core::AppEncodeU32(seg->mSN, ptr);
    ptr = core::AppEncodeU32(seg->mUNA, ptr);
    ptr = core::AppEncodeU32(seg->mLength, ptr);
    return ptr;
}


s32 NetProtocal::getUnusedWindowCount() {
    if (mReceiveQueueCount < mMaxReceiveWindowSize) {
        return mMaxReceiveWindowSize - mReceiveQueueCount;
    }
    return 0;
}


void NetProtocal::flush() {
    u32 current = mTimeCurrent;
    s8* iBuffer = mBuffer;
    s8* ptr = iBuffer;
    s32 count, size;
    u32 resent, cwnd;
    u32 rtomin;
    s32 change = 0;
    s32 lost = 0;
    SKCPSegment seg;

    // 'update' haven't been called. 
    if (mUpdated == 0) {
        return;
    }
    seg.mConnectionID = mConnectionID;
    seg.mCMD = APP_NET_CMD_ACK;
    seg.mFrag = 0;
    seg.mWindow = getUnusedWindowCount();
    seg.mUNA = mReceiveNext;
    seg.mLength = 0;
    seg.mSN = 0;
    seg.mTime = 0;

    // flush acknowledges
    count = mCountACK;
    for (s32 i = 0; i < count; i++) {
        size = (s32)(ptr - iBuffer);
        if (size + (s32)APP_NET_ENDIANOVERHEAD > (s32)mMTU) {
            sendOut(iBuffer, size);
            ptr = iBuffer;
        }
        getACK(i, &seg.mSN, &seg.mTime);
        ptr = encodeSegment(ptr, &seg);
    }//for

    mCountACK = 0;

    // probe window size (if remote window size equals zero)
    if (mWindowRemote == 0) {
        if (mTimeProbeWait == 0) {
            mTimeProbeWait = APP_NET_PROBE_TIME;
            mTimeProbe = mTimeCurrent + mTimeProbeWait;
        } else {
            if (AppValueDiff(mTimeCurrent, mTimeProbe) >= 0) {
                if (mTimeProbeWait < APP_NET_PROBE_TIME)
                    mTimeProbeWait = APP_NET_PROBE_TIME;
                mTimeProbeWait += mTimeProbeWait / 2;
                if (mTimeProbeWait > APP_NET_PROBE_TIME_LIMIT)
                    mTimeProbeWait = APP_NET_PROBE_TIME_LIMIT;
                mTimeProbe = mTimeCurrent + mTimeProbeWait;
                mProbeFlag |= APP_NET_ENDIANASK_SEND;
            }
        }
    } else {
        mTimeProbe = 0;
        mTimeProbeWait = 0;
    }

    // flush window probing commands
    if (mProbeFlag & APP_NET_ENDIANASK_SEND) {
        seg.mCMD = APP_NET_CMD_ASK_WINDOW;
        size = (s32)(ptr - iBuffer);
        if (size + (s32)APP_NET_ENDIANOVERHEAD > (s32)mMTU) {
            sendOut(iBuffer, size);
            ptr = iBuffer;
        }
        ptr = encodeSegment(ptr, &seg);
    }//if

    // flush window probing commands
    if (mProbeFlag & APP_NET_ENDIANASK_TELL) {
        seg.mCMD = APP_NET_CMD_TELL_WINDOW;
        size = (s32)(ptr - iBuffer);
        if (size + (s32)APP_NET_ENDIANOVERHEAD > (s32)mMTU) {
            sendOut(iBuffer, size);
            ptr = iBuffer;
        }
        ptr = encodeSegment(ptr, &seg);
    }//if

    mProbeFlag = 0;

    // calculate window size
    cwnd = AppMin<u32>(mMaxSendWindowSize, mWindowRemote);

    if (mNoCongestionControl == 0) {
        cwnd = AppMin<u32>(mWindowCongestion, cwnd);
    }

    // move data from mQueueSend to mQueueSendBuffer
    Node2* node;
    SKCPSegment* newseg;
    while (AppValueDiff(mSendNext, mSendUNA + cwnd) < 0) {
        if (mQueueSend.empty()) {
            break;
        }
        node = mQueueSend.getNext();
        newseg = (SKCPSegment*)(node+1);
        node->delink();
        mQueueSendBuffer.pushBack(*node);
        mSendQueueCount--;
        mSendBufferCount++;

        newseg->mConnectionID = mConnectionID;
        newseg->mCMD = APP_NET_CMD_PUSH_DATA;
        newseg->mWindow = seg.mWindow;
        newseg->mTime = current;
        newseg->mSN = mSendNext++;
        newseg->mUNA = mReceiveNext;
        newseg->mResendTime = current;
        newseg->mRTO = mRTO;
        newseg->mFastACK = 0;
        newseg->mXMIT = 0;
    }//while

    // calculate resent
    resent = (mFastResend > 0) ? (u32)mFastResend : 0xffffffff;
    rtomin = (mNodelay == 0) ? (mRTO >> 3) : 0;

    // flush data segments
    SKCPSegment* segment;
    for (Node2* p = mQueueSendBuffer.getNext(); p != &mQueueSendBuffer; p = p->getNext()) {
        segment = (SKCPSegment*)(p+1);
        s32 needsend = 0;
        if (segment->mXMIT == 0) {
            needsend = 1;
            segment->mXMIT++;
            segment->mRTO = mRTO;
            segment->mResendTime = current + segment->mRTO + rtomin;
        } else if (AppValueDiff(current, segment->mResendTime) >= 0) {
            needsend = 1;
            segment->mXMIT++;
            mXMIT++;
            if (mNodelay == 0) {
                segment->mRTO += mRTO;
            } else {
                segment->mRTO += mRTO / 2;
            }
            segment->mResendTime = current + segment->mRTO;
            lost = 1;
        } else if (segment->mFastACK >= resent) {
            needsend = 1;
            segment->mXMIT++;
            segment->mFastACK = 0;
            segment->mResendTime = current + segment->mRTO;
            change++;
        }

        if (needsend) {
            s32 size, need;
            segment->mTime = current;
            segment->mWindow = seg.mWindow;
            segment->mUNA = mReceiveNext;

            size = (s32)(ptr - iBuffer);
            need = APP_NET_ENDIANOVERHEAD + segment->mLength;

            if (size + need > (s32)mMTU) {
                sendOut(iBuffer, size);
                ptr = iBuffer;
            }

            ptr = encodeSegment(ptr, segment);

            if (segment->mLength > 0) {
                memcpy(ptr, segment->mData, segment->mLength);
                ptr += segment->mLength;
            }

            if (segment->mXMIT >= mDeadLink) {
                mState = -1;
            }
        }
    }//for

    // flash remain segments
    size = (s32)(ptr - iBuffer);
    if (size > 0) {
        sendOut(iBuffer, size);
    }

    // update ssthresh
    if (change) {
        u32 inflight = mSendNext - mSendUNA;
        mSSThreshold = inflight / 2;
        if (mSSThreshold < APP_NET_MIN_SSTHRESHOLD)
            mSSThreshold = APP_NET_MIN_SSTHRESHOLD;
        mWindowCongestion = mSSThreshold + resent;
        mIncrease = mWindowCongestion * mMSS;
    }

    if (lost) {
        mSSThreshold = cwnd / 2;
        if (mSSThreshold < APP_NET_MIN_SSTHRESHOLD)
            mSSThreshold = APP_NET_MIN_SSTHRESHOLD;
        mWindowCongestion = 1;
        mIncrease = mMSS;
    }

    if (mWindowCongestion < 1) {
        mWindowCongestion = 1;
        mIncrease = mMSS;
    }
}


void NetProtocal::update(u64 current) {
    mTimeCurrent = current;
    if (mUpdated == 0) {
        mUpdated = 1;
    }
    if (mTimeCurrent <= mTimeFlush) {
        return;
    }
    if ((mTimeCurrent - mTimeFlush) >= 10000) {
        mTimeFlush = mTimeCurrent;
    } else {
        mTimeFlush = mTimeCurrent + mInterval;
    }

    flush();
}


u64 NetProtocal::check(u64 current) {
    if (mUpdated == 0) {
        return current;
    }
    u64 ts_flush = mTimeFlush;
    s32 tm_flush = 0x7fffffff;
    s32 tm_packet = 0x7fffffff;
    u32 minimal = 0;

    if (AppValueDiff(current, ts_flush) >= 10000 ||
        AppValueDiff(current, ts_flush) < -10000) {
        ts_flush = current;
    }

    if (AppValueDiff(current, ts_flush) >= 0) {
        return current;
    }

    tm_flush = AppValueDiff(ts_flush, current);

    for (Node2* p = mQueueSendBuffer.getNext(); p != &mQueueSendBuffer; p = p->getNext()) {
        const SKCPSegment* seg = (SKCPSegment*)(p + 1);
        s32 diff = AppValueDiff(seg->mResendTime, current);
        if (diff <= 0) {
            return current;
        }
        if (diff < tm_packet) tm_packet = diff;
    }//for

    minimal = (u32)(tm_packet < tm_flush ? tm_packet : tm_flush);

    if (minimal >= mInterval) minimal = mInterval;

    return current + minimal;
}


s32 NetProtocal::setMTU(s32 iMTU) {
    if (iMTU < sizeof(SKCPSegment) || iMTU < (s32)APP_NET_ENDIANOVERHEAD) {
        return -1;
    }

    s8* iBuffer = (s8*)allocateMemory((iMTU + APP_NET_ENDIANOVERHEAD) * 3);

    if (!iBuffer) {
        return -2;
    }

    mMTU = iMTU;
    mMSS = mMTU - APP_NET_ENDIANOVERHEAD;
    releaseMemory(mBuffer);
    mBuffer = iBuffer;
    return 0;
}


void NetProtocal::setInterval(s32 interval) {
    if (interval > 5000) {
        interval = 5000;
    } else if (interval < 10) {
        interval = 10;
    }
    mInterval = interval;
}


s32 NetProtocal::setNodelay(s32 nodelay, s32 interval, s32 resend, s32 nc) {
    setInterval(interval);

    if (nodelay >= 0) {
        mNodelay = nodelay;
        if (nodelay) {
            mMinRTO = APP_NET_NODELAY_MIN_RTO;
        } else {
            mMinRTO = APP_NET_NORMAL_MIN_RTO;
        }
    }


    if (resend >= 0) {
        mFastResend = resend;
    }

    if (nc >= 0) {
        mNoCongestionControl = nc;
    }

    return 0;
}


void NetProtocal::setMaxWindowSize(s32 iSendWindow, s32 iReceiveWindow) {
    if (iSendWindow > 0) {
        mMaxSendWindowSize = iSendWindow;
    }
    if (iReceiveWindow > 0) {
        mMaxReceiveWindowSize = iReceiveWindow;
    }
}


s32 NetProtocal::getWaitSendCount() {
    return mSendBufferCount + mSendQueueCount;
}


} //namespace net 
} //namespace app 
