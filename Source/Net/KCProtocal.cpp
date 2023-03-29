/********************************************************************************
KCP has only one kind of segment: both the data and control messages are
encoded into the same structure and share the same header.

The KCP packet (aka. segment) structure is as following:

0               4   5   6       8 (BYTE)
+---------------+---+---+-------+
|     conv      |cmd|frg|  wnd  |
+---------------+---+---+-------+   8
|     ts        |     sn        |
+---------------+---------------+  16
|     una       |     len       |
+---------------+---------------+  24
|                               |
|        DATA (optional)        |
|                               |
+-------------------------------+

1. conv: conversation id (32 bits integer)

   The conversation id is used to identify each connection, which will not change
   during the connection life-time.

   It is represented by a 32 bits integer which is given at the moment the KCP
   control block (aka. struct ikcpcb, or kcp object) has been created. Each
   packet sent out will carry the conversation id in the first 4 bytes and a
   packet from remote endpoint will not be accepted if it has a different
   conversation id.

   The value can be any random number, but in practice, both side between a
   connection will have many KCP objects (or control block) storing in the
   containers like a map or an array. A index is used as the key to look up one
   KCP object from the container.

   So, the higher 16 bits of conversation id can be used as caller's index while
   the lower 16 bits can be used as callee's index. KCP will not handle
   handshake, and the index in both side can be decided and exchanged after
   connection establish.

   When you receive and accept a remote packet, the local index can be extracted
   from the conversation id and the kcp object which is in charge of this
   connection can be find out from your map or array.

2. cmd: command
3. frg: fragment count
4. wnd: window size
5. ts: timestamp
6. sn: serial number
7. una: un-acknowledged serial number
********************************************************************************/


#include "Net/KCProtocal.h"
#include "Logger.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


namespace app {
namespace net {

#define IKCP_LOG_OUTPUT 1
#define IKCP_LOG_INPUT 2
#define IKCP_LOG_SEND 4
#define IKCP_LOG_RECV 8
#define IKCP_LOG_IN_DATA 16
#define IKCP_LOG_IN_ACK 32
#define IKCP_LOG_IN_PROBE 64
#define IKCP_LOG_IN_WINS 128
#define IKCP_LOG_OUT_DATA 256
#define IKCP_LOG_OUT_ACK 512
#define IKCP_LOG_OUT_PROBE 1024
#define IKCP_LOG_OUT_WINS 2048


const u32 IKCP_RTO_NDL = 30;  // no delay min rto
const u32 IKCP_RTO_MIN = 100; // normal min rto
const u32 IKCP_RTO_DEF = 200;
const u32 IKCP_RTO_MAX = 60000;
const u32 IKCP_CMD_PUSH = 81; // cmd: push data
const u32 IKCP_CMD_ACK = 82;  // cmd: ack
const u32 IKCP_CMD_WASK = 83; // cmd: window probe (ask)
const u32 IKCP_CMD_WINS = 84; // cmd: window size (tell)
const u32 IKCP_ASK_SEND = 1;  // need to send IKCP_CMD_WASK
const u32 IKCP_ASK_TELL = 2;  // need to send IKCP_CMD_WINS
const u32 IKCP_WND_SND = 32;
const u32 IKCP_WND_RCV = 128; // must >= max fragment size
const u32 IKCP_MTU_DEF = 1400;
const u32 IKCP_ACK_FAST = 3;
const u32 IKCP_INTERVAL = 100; // millisec
const u32 IKCP_OVERHEAD = 24;
const u32 IKCP_DEADLINK = 20;
const u32 IKCP_THRESH_INIT = 2;
const u32 IKCP_THRESH_MIN = 2;
const u32 IKCP_PROBE_INIT = 7000;    // 7 secs to probe window size
const u32 IKCP_PROBE_LIMIT = 120000; // up to 120 secs to probe window
const u32 IKCP_FASTACK_LIMIT = 5;    // max times to trigger fastack


static DFINLINE s8* encodeU8(s8* p, u8 c) {
    *(u8*)p++ = c;
    return p;
}

static DFINLINE const s8* decodeU8(const s8* p, u8* c) {
    *c = *(u8*)p++;
    return p;
}

static DFINLINE s8* encodeU16(s8* p, u16 w) {
#if DENDIAN_BIG
    *(u8*)(p + 0) = (w & 255);
    *(u8*)(p + 1) = (w >> 8);
#else
    *(u16*)p = w;  // memcpy(p, &w, 2);
#endif
    p += 2;
    return p;
}

static DFINLINE const s8* decodeU16(const s8* p, u16* w) {
#if DENDIAN_BIG
    *w = *(const u8*)(p + 1);
    *w = *(const u8*)(p + 0) + (*w << 8);
#else
    *w = *(u16*)p; // memcpy(w, p, 2);
#endif
    p += 2;
    return p;
}

static DFINLINE s8* encodeU32(s8* p, u32 l) {
#if DENDIAN_BIG
    *(u8*)(p + 0) = (u8)((l >> 0) & 0xff);
    *(u8*)(p + 1) = (u8)((l >> 8) & 0xff);
    *(u8*)(p + 2) = (u8)((l >> 16) & 0xff);
    *(u8*)(p + 3) = (u8)((l >> 24) & 0xff);
#else
    *(u32*)p = l;  // memcpy(p, &l, 4);
#endif
    p += 4;
    return p;
}

static DFINLINE const s8* decodeU32(const s8* p, u32* l) {
#if DENDIAN_BIG
    *l = *(const u8*)(p + 3);
    *l = *(const u8*)(p + 2) + (*l << 8);
    *l = *(const u8*)(p + 1) + (*l << 8);
    *l = *(const u8*)(p + 0) + (*l << 8);
#else
    *l = *(u32*)p; // memcpy(l, p, 4);
#endif
    p += 4;
    return p;
}

static DFINLINE s32 AppTimeDiff(u32 later, u32 earlier) {
    return ((s32)(later - earlier));
}

static void AppPrintQueue(const s8* name, const Node2* head) {
#if 0
	const Node2 *p;
	printf("<%s>: [", name);
	for (p = head->mNext; p != head; p = p->mNext) {
		const SegmentKCP *seg = DGET_HOLDER(p, const SegmentKCP, mLinkNode);
		printf("(%lu %d)", (unsigned long)seg->sn, (s32)(seg->ts % 10000));
		if (p->mNext != head) printf(",");
	}
	printf("]\n");
#endif
}


KCProtocal::KCProtocal() {
    memset(this, 0, sizeof(*this));
}


KCProtocal::~KCProtocal() {
    clear();
}


u32 KCProtocal::getMaxSupportedMsgSize() const {
    return IKCP_WND_RCV * mMSS;
}


bool KCProtocal::init(KcpSendRawCall cb_out, MemPool* it, u32 conv, void* user) {
    DASSERT(it);
    setRawCall(cb_out);
    mMemPool = it;
    mConvID = conv;
    mUser = user;

    mSendUNA = 0;
    mSendNextSN = 0;
    mReceiveNextSN = 0;
    mRecentTime = 0;
    mLaskAckTime = 0;
    mNextProbeTime = 0;
    mProbeWait = 0;
    mSendWindowSize = IKCP_WND_SND;
    mReceiveWindowSize = IKCP_WND_RCV;
    mRemoteRecvWindowSize = IKCP_WND_RCV;
    mCongestionWindowSize = 0;
    mIncr = 0;
    mProbe = 0;
    mMTU = IKCP_MTU_DEF;
    mMSS = mMTU - IKCP_OVERHEAD;
    mStream = 0;

    mBuffer = (s8*)mMemPool->allocate((mMTU + IKCP_OVERHEAD) * 3);
    if (mBuffer == nullptr) {
        return false;
    }

    mSendQueue.clear();
    mReceiveQueue.clear();
    mSendBuffer.clear();
    mReceiveBuffer.clear();
    mRecvBufCount = 0;
    mSendBufCount = 0;
    mRecvQueueCount = 0;
    mSendQueueCount = 0;
    mState = 0;
    mListACK = nullptr;
    mAckBlock = 0;
    mAckCount = 0;
    mRX_srtt = 0;
    mRX_rttval = 0;
    mRX_RTO = IKCP_RTO_DEF;
    mRX_MinRTO = IKCP_RTO_MIN;
    mCurrent = 0;
    mInterval = IKCP_INTERVAL;
    mNextFlushTime = IKCP_INTERVAL;
    mNoDelay = 0;
    mUpdated = 0;
    mLogMask = 0;
    mSsthresh = IKCP_THRESH_INIT;
    mFastResend = 0;
    mFastAckLimit = IKCP_FASTACK_LIMIT;
    mNoCongestionWindow = 0;
    mXmit = 0;
    mDeadLink = IKCP_DEADLINK;
    // mCallSendRaw = nullptr;
    return true;
}


void KCProtocal::writeLog(s32 mask, const s8* fmt, ...) {
    if (!canLog(mask)) {
        return;
    }
    s8 buffer[1024];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    va_end(argptr);
    Logger::logInfo(buffer);
}


bool KCProtocal::canLog(s32 mask) const {
    return ((mask & mLogMask) != 0);
}


s32 KCProtocal::sendSegment(const void* data, s32 size) {
    DASSERT(mCallSendRaw);
    if (canLog(IKCP_LOG_OUTPUT)) {
        writeLog(IKCP_LOG_OUTPUT, "[RO] %ld bytes", (long)size);
    }
    if (size == 0) {
        return 0;
    }
    return mCallSendRaw(data, size, mUser);
}


void KCProtocal::clear() {
    if (mBuffer == nullptr) {
        return;
    }
    SegmentKCP* seg;
    while (!mSendBuffer.empty()) {
        seg = DGET_HOLDER(mSendBuffer.mNext, SegmentKCP, mLinkNode);
        seg->mLinkNode.delink();
        releaseSeg(seg);
    }
    while (!mReceiveBuffer.empty()) {
        seg = DGET_HOLDER(mReceiveBuffer.mNext, SegmentKCP, mLinkNode);
        seg->mLinkNode.delink();
        releaseSeg(seg);
    }
    while (!mSendQueue.empty()) {
        seg = DGET_HOLDER(mSendQueue.mNext, SegmentKCP, mLinkNode);
        seg->mLinkNode.delink();
        releaseSeg(seg);
    }
    while (!mReceiveQueue.empty()) {
        seg = DGET_HOLDER(mReceiveQueue.mNext, SegmentKCP, mLinkNode);
        seg->mLinkNode.delink();
        releaseSeg(seg);
    }
    if (mBuffer) {
        mMemPool->release(mBuffer);
    }
    if (mListACK) {
        mMemPool->release(mListACK);
    }

    mRecvBufCount = 0;
    mSendBufCount = 0;
    mRecvQueueCount = 0;
    mSendQueueCount = 0;
    mAckCount = 0;
    mBuffer = nullptr;
    mListACK = nullptr;
}


KCProtocal::SegmentKCP* KCProtocal::createSeg(s32 size) {
    return (SegmentKCP*)mMemPool->allocate(sizeof(SegmentKCP) + size);
}


void KCProtocal::releaseSeg(SegmentKCP* seg) {
    mMemPool->release(seg);
}


void KCProtocal::setRawCall(KcpSendRawCall cb_out) {
    mCallSendRaw = cb_out;
}


s32 KCProtocal::receiveKCP(s8* buffer, s32 len) {
    s32 ispeek = (len < 0) ? 1 : 0;
    s32 peeksize;
    s32 recover = 0;
    SegmentKCP* seg;

    if (mReceiveQueue.empty()) {
        return EE_RETRY;
    }
    if (len < 0) {
        len = -len;
    }
    peeksize = peekSize();

    if (peeksize < 0) {
        return EE_RETRY;
    }
    if (peeksize > len) {
        return EE_INVALID_PARAM; // user should provide bigger buf and retry
    }
    if (mRecvQueueCount >= mReceiveWindowSize) {
        recover = 1;
    }
    // merge fragment
    Node2* p;
    for (len = 0, p = mReceiveQueue.mNext; p != &mReceiveQueue;) {
        s32 fragment;
        seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        p = p->mNext;

        if (buffer) {
            memcpy(buffer, seg->mData, seg->mDataSize);
            buffer += seg->mDataSize;
        }

        len += seg->mDataSize;
        fragment = seg->mFragment;

        if (canLog(IKCP_LOG_RECV)) {
            writeLog(IKCP_LOG_RECV, "recv sn=%lu", (unsigned long)seg->mSN);
        }

        if (ispeek == 0) {
            seg->mLinkNode.delink();
            releaseSeg(seg);
            mRecvQueueCount--;
        }

        if (fragment == 0) {
            break;
        }
    }

    DASSERT(len == peeksize);

    // move available data from rcv_buf -> rcv_queue
    while (!mReceiveBuffer.empty()) {
        seg = DGET_HOLDER(mReceiveBuffer.mNext, SegmentKCP, mLinkNode);
        if (seg->mSN == mReceiveNextSN && mRecvQueueCount < mReceiveWindowSize) {
            seg->mLinkNode.delink();
            mRecvBufCount--;
            mReceiveQueue.pushFront(seg->mLinkNode);
            mRecvQueueCount++;
            mReceiveNextSN++;
        } else {
            break;
        }
    }

    // fast recover
    if (mRecvQueueCount < mReceiveWindowSize && recover) {
        // ready to send back IKCP_CMD_WINS in KCProtocal::flush()
        // tell remote my window size
        mProbe |= IKCP_ASK_TELL;
    }

    return len;
}


s32 KCProtocal::peekSize() const {
    if (mReceiveQueue.empty()) {
        return -1;
    }
    SegmentKCP* seg = DGET_HOLDER(mReceiveQueue.mNext, SegmentKCP, mLinkNode);
    if (seg->mFragment == 0) {
        return seg->mDataSize;
    }
    if (mRecvQueueCount < seg->mFragment + 1) {
        return -1;
    }
    s32 length = 0;
    for (Node2* p = mReceiveQueue.mNext; p != &mReceiveQueue; p = p->mNext) {
        seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        length += seg->mDataSize;
        if (seg->mFragment == 0) {
            break;
        }
    }
    return length;
}


s32 KCProtocal::sendKCP(const s8* buffer, s32 len) {
    DASSERT(mMSS > 0);
    if (len < 0) {
        return EE_INVALID_PARAM;
    }
    SegmentKCP* seg;
    s32 count, i;
    s32 sent = 0;

    // append to previous segment in streaming mode (if possible)
    if (mStream != 0) {
        if (!mSendQueue.empty()) {
            SegmentKCP* old = DGET_HOLDER(mSendQueue.mPrevious, SegmentKCP, mLinkNode);
            if (old->mDataSize < mMSS) {
                s32 capacity = mMSS - old->mDataSize;
                s32 extend = (len < capacity) ? len : capacity;
                seg = createSeg(old->mDataSize + extend);
                DASSERT(seg);
                if (seg == nullptr) {
                    return -2;
                }
                mSendQueue.pushFront(seg->mLinkNode);
                memcpy(seg->mData, old->mData, old->mDataSize);
                if (buffer) {
                    memcpy(seg->mData + old->mDataSize, buffer, extend);
                    buffer += extend;
                }
                seg->mDataSize = old->mDataSize + extend;
                seg->mFragment = 0;
                len -= extend;
                old->mLinkNode.delink();
                releaseSeg(old);
                sent = extend;
            }
        }
        if (len <= 0) {
            return sent;
        }
    }

    count = (len <= (s32)mMSS) ? 1 : ((len + mMSS - 1) / mMSS);
    if (count >= (s32)IKCP_WND_RCV) {
        if (mStream != 0 && sent > 0) {
            return sent;
        }
        return EE_RETRY;
    }

    if (count == 0) {
        count = 1;
    }

    // fragment
    for (i = 0; i < count; i++) {
        s32 size = len > (s32)mMSS ? (s32)mMSS : len;
        seg = createSeg(size);
        DASSERT(seg);
        if (seg == nullptr) {
            return -2;
        }
        if (buffer && len > 0) {
            memcpy(seg->mData, buffer, size);
        }
        seg->mDataSize = size;
        seg->mFragment = (mStream == 0) ? (count - i - 1) : 0;
        seg->mLinkNode.clear();
        mSendQueue.pushFront(seg->mLinkNode);
        mSendQueueCount++;
        if (buffer) {
            buffer += size;
        }
        len -= size;
        sent += size;
    }

    return sent;
}


void KCProtocal::updateACK(s32 rtt) {
    s32 rto = 0;
    if (mRX_srtt == 0) {
        mRX_srtt = rtt;
        mRX_rttval = rtt / 2;
    } else {
        long delta = rtt - mRX_srtt;
        if (delta < 0)
            delta = -delta;
        mRX_rttval = (3 * mRX_rttval + delta) / 4;
        mRX_srtt = (7 * mRX_srtt + rtt) / 8;
        if (mRX_srtt < 1)
            mRX_srtt = 1;
    }
    rto = mRX_srtt + AppMax<u32>(mInterval, 4 * mRX_rttval);
    mRX_RTO = AppClamp<s32>(rto, mRX_MinRTO, IKCP_RTO_MAX);
}


void KCProtocal::shrinkBuf() {
    Node2* p = mSendBuffer.mNext;
    if (p != &mSendBuffer) {
        SegmentKCP* seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        mSendUNA = seg->mSN;
    } else {
        mSendUNA = mSendNextSN;
    }
}


void KCProtocal::parseACK(u32 sn) {
    if (AppTimeDiff(sn, mSendUNA) < 0 || AppTimeDiff(sn, mSendNextSN) >= 0) {
        return;
    }
    Node2* next;
    for (Node2* p = mSendBuffer.mNext; p != &mSendBuffer; p = next) {
        SegmentKCP* seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        next = p->mNext;
        if (sn == seg->mSN) {
            p->delink();
            releaseSeg(seg);
            mSendBufCount--;
            break;
        }
        if (AppTimeDiff(sn, seg->mSN) < 0) {
            break;
        }
    }
}


void KCProtocal::parseUNA(u32 una) {
    Node2* next;
    for (Node2* p = mSendBuffer.mNext; p != &mSendBuffer; p = next) {
        SegmentKCP* seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        next = p->mNext;
        if (AppTimeDiff(una, seg->mSN) > 0) {
            p->delink();
            releaseSeg(seg);
            mSendBufCount--;
        } else {
            break;
        }
    }
}


void KCProtocal::parseFaskACK(u32 sn, u32 ts) {
    if (AppTimeDiff(sn, mSendUNA) < 0 || AppTimeDiff(sn, mSendNextSN) >= 0) {
        return;
    }
    Node2* next;
    for (Node2* p = mSendBuffer.mNext; p != &mSendBuffer; p = next) {
        SegmentKCP* seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        next = p->mNext;
        if (AppTimeDiff(sn, seg->mSN) < 0) {
            break;
        } else if (sn != seg->mSN) {
#ifndef IKCP_FASTACK_CONSERVE
            seg->mFastACK++;
#else
            if (AppTimeDiff(ts, seg->ts) >= 0)
                seg->mFastACK++;
#endif
        }
    }
}


void KCProtocal::pushACK(u32 sn, u32 ts) {
    u32 newsize = mAckCount + 1;
    u32* ptr;

    if (newsize > mAckBlock) {
        u32* acklist;
        u32 newblock;

        for (newblock = 8; newblock < newsize; newblock <<= 1) {
        }
        acklist = (u32*)mMemPool->allocate(newblock * sizeof(u32) * 2);

        if (acklist == nullptr) {
            DASSERT(acklist != nullptr);
            abort();
        }

        if (mListACK != nullptr) {
            u32 x;
            for (x = 0; x < mAckCount; x++) {
                acklist[x * 2 + 0] = mListACK[x * 2 + 0];
                acklist[x * 2 + 1] = mListACK[x * 2 + 1];
            }
            mMemPool->release(mListACK);
        }

        mListACK = acklist;
        mAckBlock = newblock;
    }

    ptr = &mListACK[mAckCount * 2];
    ptr[0] = sn;
    ptr[1] = ts;
    mAckCount++;
}


void KCProtocal::getACK(s32 p, u32* sn, u32* ts) const {
    if (sn) {
        sn[0] = mListACK[p * 2 + 0];
    }
    if (ts) {
        ts[0] = mListACK[p * 2 + 1];
    }
}


void KCProtocal::parseSeg(SegmentKCP* newseg) {
    Node2 *p, *prev;
    u32 sn = newseg->mSN;
    s32 repeat = 0;

    if (AppTimeDiff(sn, mReceiveNextSN + mReceiveWindowSize) >= 0 || AppTimeDiff(sn, mReceiveNextSN) < 0) {
        releaseSeg(newseg);
        return;
    }

    for (p = mReceiveBuffer.mPrevious; p != &mReceiveBuffer; p = prev) {
        SegmentKCP* seg = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        prev = p->mPrevious;
        if (seg->mSN == sn) {
            repeat = 1;
            break;
        }
        if (AppTimeDiff(sn, seg->mSN) > 0) {
            break;
        }
    }

    if (repeat == 0) {
        newseg->mLinkNode.clear();
        p->pushBack(newseg->mLinkNode); // IQUEUE_ADD(&newseg->mLinkNode, p);
        mRecvBufCount++;
    } else {
        releaseSeg(newseg);
    }

#if 0
	AppPrintQueue("rcvbuf", &mReceiveBuffer);
	printf("rcv_nxt=%lu\n", mReceiveNextSN);
#endif

    // move available data from rcv_buf -> rcv_queue
    while (!mReceiveBuffer.empty()) {
        SegmentKCP* seg = DGET_HOLDER(mReceiveBuffer.mNext, SegmentKCP, mLinkNode);
        if (seg->mSN == mReceiveNextSN && mRecvQueueCount < mReceiveWindowSize) {
            seg->mLinkNode.delink();
            mRecvBufCount--;
            mReceiveQueue.pushFront(seg->mLinkNode);
            mRecvQueueCount++;
            mReceiveNextSN++;
        } else {
            break;
        }
    }

#if 0
	AppPrintQueue("queue", &mReceiveQueue);
	printf("rcv_nxt=%lu\n", mReceiveNextSN);
#endif

#if 1
//	printf("snd(buf=%d, queue=%d)\n", mSendBufCount, mSendQueueCount);
//	printf("rcv(buf=%d, queue=%d)\n", mRecvBufCount, mRecvQueueCount);
#endif
}


s32 KCProtocal::importRaw(const s8* data, long size) {
    u32 prev_una = mSendUNA;
    u32 maxack = 0, latest_ts = 0;
    s32 flag = 0;
    if (canLog(IKCP_LOG_INPUT)) {
        writeLog(IKCP_LOG_INPUT, "[RI] %d bytes", (s32)size);
    }
    if (data == nullptr || (s32)size < (s32)IKCP_OVERHEAD) {
        return -1;
    }
    while (1) {
        u32 ts, sn, len, una, conv;
        u16 wnd;
        u8 cmd, frg;
        SegmentKCP* seg;
        if (size < (s32)IKCP_OVERHEAD) {
            break;
        }
        data = decodeU32(data, &conv);
        if (conv != mConvID) {
            return -1;
        }
        data = decodeU8(data, &cmd);
        data = decodeU8(data, &frg);
        data = decodeU16(data, &wnd);
        data = decodeU32(data, &ts);
        data = decodeU32(data, &sn);
        data = decodeU32(data, &una);
        data = decodeU32(data, &len);

        size -= IKCP_OVERHEAD;

        if ((long)size < (long)len || (s32)len < 0) {
            return -2;
        }
        if (cmd != IKCP_CMD_PUSH && cmd != IKCP_CMD_ACK && cmd != IKCP_CMD_WASK && cmd != IKCP_CMD_WINS) {
            return -3;
        }
        mRemoteRecvWindowSize = wnd;
        parseUNA(una);
        shrinkBuf();

        if (cmd == IKCP_CMD_ACK) {
            if (AppTimeDiff(mCurrent, ts) >= 0) {
                updateACK(AppTimeDiff(mCurrent, ts));
            }
            parseACK(sn);
            shrinkBuf();
            if (flag == 0) {
                flag = 1;
                maxack = sn;
                latest_ts = ts;
            } else {
                if (AppTimeDiff(sn, maxack) > 0) {
#ifndef IKCP_FASTACK_CONSERVE
                    maxack = sn;
                    latest_ts = ts;
#else
                    if (AppTimeDiff(ts, latest_ts) > 0) {
                        maxack = sn;
                        latest_ts = ts;
                    }
#endif
                }
            }
            if (canLog(IKCP_LOG_IN_ACK)) {
                writeLog(IKCP_LOG_IN_ACK, "input ack: sn=%lu rtt=%ld rto=%ld", (unsigned long)sn,
                    (long)AppTimeDiff(mCurrent, ts), (long)mRX_RTO);
            }
        } else if (cmd == IKCP_CMD_PUSH) {
            if (canLog(IKCP_LOG_IN_DATA)) {
                writeLog(IKCP_LOG_IN_DATA, "input psh: sn=%lu ts=%lu", (unsigned long)sn, (unsigned long)ts);
            }
            if (AppTimeDiff(sn, mReceiveNextSN + mReceiveWindowSize) < 0) {
                pushACK(sn, ts);
                if (AppTimeDiff(sn, mReceiveNextSN) >= 0) {
                    seg = createSeg(len);
                    seg->mConvID = conv;
                    seg->mCMD = cmd;
                    seg->mFragment = frg;
                    seg->mWindow = wnd;
                    seg->mTimestamp = ts;
                    seg->mSN = sn;
                    seg->mUNA = una;
                    seg->mDataSize = len;

                    if (len > 0) {
                        memcpy(seg->mData, data, len);
                    }

                    parseSeg(seg);
                }
            }
        } else if (cmd == IKCP_CMD_WASK) {
            // ready to send back IKCP_CMD_WINS in KCProtocal::flush()
            // tell remote my window size
            mProbe |= IKCP_ASK_TELL;
            if (canLog(IKCP_LOG_IN_PROBE)) {
                writeLog(IKCP_LOG_IN_PROBE, "input probe");
            }
        } else if (cmd == IKCP_CMD_WINS) {
            // do nothing
            if (canLog(IKCP_LOG_IN_WINS)) {
                writeLog(IKCP_LOG_IN_WINS, "input wins: %lu", (unsigned long)(wnd));
            }
        } else {
            return -3;
        }

        data += len;
        size -= len;
    }

    if (flag != 0) {
        parseFaskACK(maxack, latest_ts);
    }

    if (AppTimeDiff(mSendUNA, prev_una) > 0) {
        if (mCongestionWindowSize < mRemoteRecvWindowSize) {
            u32 mss = mMSS;
            if (mCongestionWindowSize < mSsthresh) {
                mCongestionWindowSize++;
                mIncr += mss;
            } else {
                if (mIncr < mss)
                    mIncr = mss;
                mIncr += (mss * mss) / mIncr + (mss / 16);
                if ((mCongestionWindowSize + 1) * mss <= mIncr) {
#if 1
                    mCongestionWindowSize = (mIncr + mss - 1) / ((mss > 0) ? mss : 1);
#else
                    mCongestionWindowSize++;
#endif
                }
            }
            if (mCongestionWindowSize > mRemoteRecvWindowSize) {
                mCongestionWindowSize = mRemoteRecvWindowSize;
                mIncr = mRemoteRecvWindowSize * mss;
            }
        }
    }

    return 0;
}


s8* KCProtocal::SegmentKCP::encode2Buf(s8* ptr) const {
    ptr = encodeU32(ptr, mConvID);
    ptr = encodeU8(ptr, (u8)mCMD);
    ptr = encodeU8(ptr, (u8)mFragment);
    ptr = encodeU16(ptr, (u16)mWindow);
    ptr = encodeU32(ptr, mTimestamp);
    ptr = encodeU32(ptr, mSN);
    ptr = encodeU32(ptr, mUNA);
    ptr = encodeU32(ptr, mDataSize);
    return ptr;
}


s32 KCProtocal::getWindowUnused() const {
    if (mRecvQueueCount < mReceiveWindowSize) {
        return mReceiveWindowSize - mRecvQueueCount;
    }
    return 0;
}


void KCProtocal::flush() {
    u32 current = mCurrent;
    s8* buffer = mBuffer;
    s8* ptr = buffer;
    s32 count, size, i;
    u32 resent, cwnd;
    u32 rtomin;
    Node2* p;
    s32 change = 0;
    s32 lost = 0;
    SegmentKCP seg;

    // KCProtocal::update() haven't been called.
    if (mUpdated == 0) {
        return;
    }
    seg.mConvID = mConvID;
    seg.mCMD = IKCP_CMD_ACK;
    seg.mFragment = 0;
    seg.mWindow = getWindowUnused();
    seg.mUNA = mReceiveNextSN;
    seg.mDataSize = 0;
    seg.mSN = 0;
    seg.mTimestamp = 0;

    // flush acknowledges
    count = mAckCount;
    for (i = 0; i < count; i++) {
        size = (s32)(ptr - buffer);
        if (size + (s32)IKCP_OVERHEAD > (s32)mMTU) {
            sendSegment(buffer, size);
            ptr = buffer;
        }
        getACK(i, &seg.mSN, &seg.mTimestamp);
        ptr = seg.encode2Buf(ptr);
    }

    mAckCount = 0;

    // probe window size (if remote window size equals zero)
    if (mRemoteRecvWindowSize == 0) {
        if (mProbeWait == 0) {
            mProbeWait = IKCP_PROBE_INIT;
            mNextProbeTime = mCurrent + mProbeWait;
        } else {
            if (AppTimeDiff(mCurrent, mNextProbeTime) >= 0) {
                if (mProbeWait < IKCP_PROBE_INIT)
                    mProbeWait = IKCP_PROBE_INIT;
                mProbeWait += mProbeWait / 2;
                if (mProbeWait > IKCP_PROBE_LIMIT)
                    mProbeWait = IKCP_PROBE_LIMIT;
                mNextProbeTime = mCurrent + mProbeWait;
                mProbe |= IKCP_ASK_SEND;
            }
        }
    } else {
        mNextProbeTime = 0;
        mProbeWait = 0;
    }

    // flush window probing commands
    if (mProbe & IKCP_ASK_SEND) {
        seg.mCMD = IKCP_CMD_WASK;
        size = (s32)(ptr - buffer);
        if (size + (s32)IKCP_OVERHEAD > (s32)mMTU) {
            sendSegment(buffer, size);
            ptr = buffer;
        }
        ptr = seg.encode2Buf(ptr);
    }

    // flush window probing commands
    if (mProbe & IKCP_ASK_TELL) {
        seg.mCMD = IKCP_CMD_WINS;
        size = (s32)(ptr - buffer);
        if (size + (s32)IKCP_OVERHEAD > (s32)mMTU) {
            sendSegment(buffer, size);
            ptr = buffer;
        }
        ptr = seg.encode2Buf(ptr);
    }

    mProbe = 0;

    // calculate window size
    cwnd = AppMin<u32>(mSendWindowSize, mRemoteRecvWindowSize);
    if (mNoCongestionWindow == 0) {
        cwnd = AppMin<u32>(mCongestionWindowSize, cwnd);
    }

    // move data from snd_queue to snd_buf
    while (AppTimeDiff(mSendNextSN, mSendUNA + cwnd) < 0) {
        if (mSendQueue.empty()) {
            break;
        }

        SegmentKCP* newseg = DGET_HOLDER(mSendQueue.mNext, SegmentKCP, mLinkNode);
        newseg->mLinkNode.delink();
        mSendBuffer.pushFront(newseg->mLinkNode);
        mSendQueueCount--;
        mSendBufCount++;

        newseg->mConvID = mConvID;
        newseg->mCMD = IKCP_CMD_PUSH;
        newseg->mWindow = seg.mWindow;
        newseg->mTimestamp = current;
        newseg->mSN = mSendNextSN++;
        newseg->mUNA = mReceiveNextSN;
        newseg->mResendTime = current;
        newseg->mRTO = mRX_RTO;
        newseg->mFastACK = 0;
        newseg->mSegXmit = 0;
    }

    // calculate resent
    resent = (mFastResend > 0) ? (u32)mFastResend : 0xffffffff;
    rtomin = (mNoDelay == 0) ? (mRX_RTO >> 3) : 0;

    // flush data segments
    for (p = mSendBuffer.mNext; p != &mSendBuffer; p = p->mNext) {
        SegmentKCP* segment = DGET_HOLDER(p, SegmentKCP, mLinkNode);
        s32 needsend = 0;
        if (segment->mSegXmit == 0) {
            needsend = 1;
            segment->mSegXmit++;
            segment->mRTO = mRX_RTO;
            segment->mResendTime = current + segment->mRTO + rtomin;
        } else if (AppTimeDiff(current, segment->mResendTime) >= 0) {
            needsend = 1;
            segment->mSegXmit++;
            mXmit++;
            if (mNoDelay == 0) {
                segment->mRTO += AppMax<u32>(segment->mRTO, (u32)mRX_RTO);
            } else {
                s32 step = (mNoDelay < 2) ? ((s32)(segment->mRTO)) : mRX_RTO;
                segment->mRTO += step / 2;
            }
            segment->mResendTime = current + segment->mRTO;
            lost = 1;
        } else if (segment->mFastACK >= resent) {
            if ((s32)segment->mSegXmit <= mFastAckLimit || mFastAckLimit <= 0) {
                needsend = 1;
                segment->mSegXmit++;
                segment->mFastACK = 0;
                segment->mResendTime = current + segment->mRTO;
                change++;
            }
        }

        if (needsend) {
            s32 need;
            segment->mTimestamp = current;
            segment->mWindow = seg.mWindow;
            segment->mUNA = mReceiveNextSN;

            size = (s32)(ptr - buffer);
            need = IKCP_OVERHEAD + segment->mDataSize;

            if (size + need > (s32)mMTU) {
                sendSegment(buffer, size);
                ptr = buffer;
            }

            ptr = segment->encode2Buf(ptr);

            if (segment->mDataSize > 0) {
                memcpy(ptr, segment->mData, segment->mDataSize);
                ptr += segment->mDataSize;
            }

            if (segment->mSegXmit >= mDeadLink) {
                mState = (u32)-1;
            }
        }
    }

    // flash remain segments
    size = (s32)(ptr - buffer);
    if (size > 0) {
        sendSegment(buffer, size);
    }

    // update ssthresh
    if (change) {
        u32 inflight = mSendNextSN - mSendUNA;
        mSsthresh = inflight / 2;
        if (mSsthresh < IKCP_THRESH_MIN)
            mSsthresh = IKCP_THRESH_MIN;
        mCongestionWindowSize = mSsthresh + resent;
        mIncr = mCongestionWindowSize * mMSS;
    }

    if (lost) {
        mSsthresh = cwnd / 2;
        if (mSsthresh < IKCP_THRESH_MIN) {
            mSsthresh = IKCP_THRESH_MIN;
        }
        mCongestionWindowSize = 1;
        mIncr = mMSS;
    }

    if (mCongestionWindowSize < 1) {
        mCongestionWindowSize = 1;
        mIncr = mMSS;
    }
}


void KCProtocal::update(u32 current) {
    mCurrent = current;

    if (mUpdated == 0) {
        mUpdated = 1;
        mNextFlushTime = mCurrent;
    }

    s32 slap = AppTimeDiff(mCurrent, mNextFlushTime);

    if (slap >= 10000 || slap < -10000) {
        mNextFlushTime = mCurrent;
        slap = 0;
    }

    if (slap >= 0) {
        mNextFlushTime += mInterval;
        if (AppTimeDiff(mCurrent, mNextFlushTime) >= 0) {
            mNextFlushTime = mCurrent + mInterval;
        }
        flush();
    }
}


u32 KCProtocal::check(u32 current) {
    u32 ts_flush = mNextFlushTime;
    s32 tm_flush = 0x7fffffff;
    s32 tm_packet = 0x7fffffff;
    u32 minimal = 0;

    if (mUpdated == 0) {
        return current;
    }

    if (AppTimeDiff(current, ts_flush) >= 10000 || AppTimeDiff(current, ts_flush) < -10000) {
        ts_flush = current;
    }

    if (AppTimeDiff(current, ts_flush) >= 0) {
        return current;
    }

    tm_flush = AppTimeDiff(ts_flush, current);

    for (Node2* p = mSendBuffer.mNext; p != &mSendBuffer; p = p->mNext) {
        const SegmentKCP* seg = DGET_HOLDER(p, const SegmentKCP, mLinkNode);
        s32 diff = AppTimeDiff(seg->mResendTime, current);
        if (diff <= 0) {
            return current;
        }
        if (diff < tm_packet) {
            tm_packet = diff;
        }
    }

    minimal = (u32)(tm_packet < tm_flush ? tm_packet : tm_flush);
    if (minimal >= mInterval) {
        minimal = mInterval;
    }
    return current + minimal;
}


s32 KCProtocal::setMTU(s32 mtu) {
    if (mtu < 50 || mtu < (s32)IKCP_OVERHEAD) {
        return -1;
    }
    s8* buffer = (s8*)mMemPool->allocate((mtu + IKCP_OVERHEAD) * 3);
    if (buffer == nullptr) {
        return -2;
    }
    mMTU = mtu;
    mMSS = mMTU - IKCP_OVERHEAD;
    mMemPool->release(mBuffer);
    mBuffer = buffer;
    return 0;
}


void KCProtocal::setInterval(s32 interval) {
    mInterval = AppClamp(interval, 10, 5000);
}


s32 KCProtocal::setNodelay(s32 nodelay, s32 interval, s32 resend, s32 nc) {
    if (nodelay >= 0) {
        mNoDelay = nodelay;
        if (nodelay) {
            mRX_MinRTO = IKCP_RTO_NDL;
        } else {
            mRX_MinRTO = IKCP_RTO_MIN;
        }
    }
    if (interval >= 0) {
        mInterval = AppClamp(interval, 10, 5000);
    }
    if (resend >= 0) {
        mFastResend = resend;
    }
    if (nc >= 0) {
        mNoCongestionWindow = nc;
    }
    return 0;
}


void KCProtocal::setWindowSize(s32 sndwnd, s32 rcvwnd) {
    if (sndwnd > 0) {
        mSendWindowSize = sndwnd;
    }
    if (rcvwnd > 0) { // must >= max fragment size
        mReceiveWindowSize = AppMax<u32>(rcvwnd, IKCP_WND_RCV);
    }
}


s32 KCProtocal::getWaitSend() const {
    return mSendBufCount + mSendQueueCount;
}


u32 KCProtocal::getConv(const void* ptr) {
    u32 conv;
    decodeU32((const s8*)ptr, &conv);
    return conv;
}


} // namespace net
} // namespace app
