//=====================================================================
// KCP - A Better ARQ Protocol Implementation for C++
// codes from https://github.com/skywind3000/kcp
//
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//=====================================================================
#ifndef APP_KCPROTOCAL_H
#define APP_KCPROTOCAL_H

#include "Nocopy.h"
#include "Node.h"
#include "MemoryPool.h"

namespace app {
namespace net {

using KcpSendRawCall = s32 (*)(const void* buf, s32 len, void* user);


class KCProtocal : public Nocopy {
public:
    KCProtocal();
    ~KCProtocal();

    // create a new kcp control object, 'conv' must equal in two endpoint
    // from the same connection. 'user' will be passed to the output callback
    // output callback can be setup like this: 'kcp->output = my_udp_output'
    bool init(KcpSendRawCall cb_out, MemPool* it, u32 conv, void* user);

    // release kcp control object
    void clear();

    void flush();

    /** user's level sender
     * @return >=0: send size if stream mode, else bsize.
                <0: errors,
                    if EE_RETRY, should retry later. */
    s32 sendKCP(const s8* buffer, s32 bsize);

    /** user's level receiver
     * @param buffer buffer provided
     * @param bsize >0: read, buf size, <0: peek, buf size
     * @return >0: read or peek size(complete data package)
               <0: errors,
                   if EE_RETRY, should retry later.
                   if EE_INVALID_PARAM, provide bigger buf and retry. */
    s32 receiveKCP(s8* buffer, s32 bsize);

    // when you received a low level packet (eg. UDP packet), call it
    s32 importRaw(const s8* data, long size);

    /**
     * update state (call it repeatedly, every 10ms-100ms), or you can ask
     * KCProtocal::check when to call it again (without importRaw/_send calling).
     * @param current current timestamp in millisec(don't use relative time). */
    void update(u32 current);

    // Determine when should you invoke KCProtocal::update:
    // returns when you should invoke KCProtocal::update in millisec, if there
    // is no importRaw/_send calling. you can call KCProtocal::update in that
    // time, instead of call update repeatly.
    // Important to reduce unnacessary KCProtocal::update invoking. use it to
    // schedule KCProtocal::update (eg. implementing an epoll-like mechanism,
    // or optimize KCProtocal::update when handling massive kcp connections)
    u32 check(u32 current);

    // change MTU size, default is 1400
    s32 setMTU(s32 mtu);

    // check the size of next message in the recv queue
    s32 peekSize() const;


    // set maximum window size: sndwnd=32, rcvwnd=32 by default
    void setWindowSize(s32 sndwnd, s32 rcvwnd);

    // get how many packet is waiting to be sent
    s32 getWaitSend() const;

    /**
     * fastest: setNodelay(1, 20, 2, 1)
     * nodelay: 0:disable(default), 1:enable
     * interval: internal update timer interval in millisec, default is 100ms
     * resend: 0:disable fast resend(default), 1:enable fast resend
     * nc: 0:normal congestion control(default), 1:disable congestion control */
    s32 setNodelay(s32 nodelay, s32 interval, s32 resend, s32 nc);

    void setInterval(s32 interval);

    u32 getCurrTime() const {
        return mCurrent;
    }

    u32 getNextFlushTime() const {
        return mNextFlushTime;
    }

    u32 getID() const {
        return mConvID;
    }

    u32 getMaxSupportedMsgSize() const;

    // read conv
    static u32 getConv(const void* ptr);

private:
    struct SegmentKCP {
        Node2 mLinkNode;
        u32 mConvID;     // conversation, 会话序号: 接收到的数据包与发送的一致才接收此数据包
        u32 mCMD;        // command, 指令类型: 代表这个Segment的类型
        u32 mFragment;   // 分段序号
        u32 mWindow;     // 窗口大小
        u32 mTimestamp;  // 发送的时间戳
        u32 mSN;         // sequence number, segment序号
        u32 mUNA;        // unacknowledged, 当前未收到的序号: 即代表这个序号之前的包均收到
        u32 mDataSize;   // 数据长度
        u32 mResendTime; // 重发的时间戳
        u32 mRTO;        // 超时重传的时间间隔
        u32 mFastACK;    // ack跳过的次数，用于快速重传
        u32 mSegXmit;    // 发送的次数(即重传的次数)
        s8 mData[1];

        s8* encode2Buf(s8* out) const;
    };
    s32 getWindowUnused() const;
    SegmentKCP* createSeg(s32 size);
    void releaseSeg(SegmentKCP* seg);
    void parseSeg(SegmentKCP* newseg);
    void getACK(s32 p, u32* sn, u32* ts) const;
    void pushACK(u32 sn, u32 ts);
    void updateACK(s32 rtt);
    void parseACK(u32 sn);
    void parseFaskACK(u32 sn, u32 ts);
    void parseUNA(u32 una);
    void shrinkBuf();
    void writeLog(s32 mask, const s8* fmt, ...);
    s32 sendSegment(const void* data, s32 size);
    bool canLog(s32 mask) const;

    // @param cb_out output callback, which will be invoked by kcp
    void setRawCall(KcpSendRawCall cb_out);

    ///////////////////////////////////////////////////////////////////////////

    u32 mConvID; // 会话ID
    u32 mMTU;    // 最大传输单元
    u32 mMSS;    // 最大分片大小
    u32 mState;  // 连接状态（0xFFFFFFFF表示断开连接）

    u32 mSendUNA;       // 第一个未确认的包
    u32 mSendNextSN;    // 待发送包的序号
    u32 mReceiveNextSN; // 待接收消息的序号

    u32 mRecentTime;  // 暂无用
    u32 mLaskAckTime; // 暂无用
    u32 mSsthresh;    // 拥塞窗口的阈值,以包为单位（TCP以字节为单位）

    s32 mRX_rttval; // ack接收rtt浮动值
    s32 mRX_srtt;   // ack接收rtt平滑值(smoothed)
    s32 mRX_RTO;    // 由ack接收延迟计算出来的复原时间
    s32 mRX_MinRTO; // 最小复原时间

    u32 mSendWindowSize;
    u32 mReceiveWindowSize;

    u32 mRemoteRecvWindowSize; // 远端接收窗口大小
    u32 mCongestionWindowSize; // 拥塞窗口大小Congestion window size
    // 探查变量，IKCP_ASK_TELL表示告知远端窗口大小。IKCP_ASK_SEND表示请求远端告知窗口大小
    u32 mProbe;

    u32 mCurrent;       // 当前的时间戳
    u32 mInterval;      // 内部flush刷新间隔
    u32 mNextFlushTime; // 下次flush刷新时间戳
    u32 mXmit;

    u32 mRecvBufCount;   // 接收缓存中消息数量
    u32 mSendBufCount;   // 发送缓存中消息数量
    u32 mRecvQueueCount; // 接收队列中消息数量
    u32 mSendQueueCount; // 发送队列中消息数量
    u32 mNoDelay;        // 是否启动无延迟模式

    // 是否调用过update函数的标识(需要上层不断的调用KCProtocal::update和KCProtocal::check来驱动kcp的收发过程)
    u32 mUpdated;

    u32 mNextProbeTime;   // 下次探查窗口的时间戳
    u32 mProbeWait;       // 探查窗口需要等待的时间
    u32 mDeadLink;        // 最大重传次数
    u32 mIncr;            // 可发送的最大数据量
    Node2 mSendQueue;     // 发送消息的队列
    Node2 mReceiveQueue;  // 接收消息的队列
    Node2 mSendBuffer;    // 发送消息的缓存
    Node2 mReceiveBuffer; // 接收消息的缓存
    u32* mListACK;        // 待发送的ack列表
    u32 mAckCount;        // mListACK中ack的数量，每个ack在mListACK中存储ts，sn两个量
    u32 mAckBlock;        // 2的倍数，标识mListACK最大可容纳的ack数量
    void* mUser;          // KCProtocal nevel use this param,just callback
    s8* mBuffer;
    s32 mFastResend; // 触发快速重传的重复ack个数
    s32 mFastAckLimit;
    s32 mNoCongestionWindow; // 取消拥塞控制
    s32 mStream;             // 是否采用流传输模式
    s32 mLogMask;
    KcpSendRawCall mCallSendRaw;
    MemPool* mMemPool;
};

} // namespace net
} // namespace app

#endif // APP_KCPROTOCAL_H