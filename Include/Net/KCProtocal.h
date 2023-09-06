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
        u32 mConvID;     // conversation, �Ự���: ���յ������ݰ��뷢�͵�һ�²Ž��մ����ݰ�
        u32 mCMD;        // command, ָ������: �������Segment������
        u32 mFragment;   // �ֶ����
        u32 mWindow;     // ���ڴ�С
        u32 mTimestamp;  // ���͵�ʱ���
        u32 mSN;         // sequence number, segment���
        u32 mUNA;        // unacknowledged, ��ǰδ�յ������: ������������֮ǰ�İ����յ�
        u32 mDataSize;   // ���ݳ���
        u32 mResendTime; // �ط���ʱ���
        u32 mRTO;        // ��ʱ�ش���ʱ����
        u32 mFastACK;    // ack�����Ĵ��������ڿ����ش�
        u32 mSegXmit;    // ���͵Ĵ���(���ش��Ĵ���)
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

    u32 mConvID; // �ỰID
    u32 mMTU;    // ����䵥Ԫ
    u32 mMSS;    // ����Ƭ��С
    u32 mState;  // ����״̬��0xFFFFFFFF��ʾ�Ͽ����ӣ�

    u32 mSendUNA;       // ��һ��δȷ�ϵİ�
    u32 mSendNextSN;    // �����Ͱ������
    u32 mReceiveNextSN; // ��������Ϣ�����

    u32 mRecentTime;  // ������
    u32 mLaskAckTime; // ������
    u32 mSsthresh;    // ӵ�����ڵ���ֵ,�԰�Ϊ��λ��TCP���ֽ�Ϊ��λ��

    s32 mRX_rttval; // ack����rtt����ֵ
    s32 mRX_srtt;   // ack����rttƽ��ֵ(smoothed)
    s32 mRX_RTO;    // ��ack�����ӳټ�������ĸ�ԭʱ��
    s32 mRX_MinRTO; // ��С��ԭʱ��

    u32 mSendWindowSize;
    u32 mReceiveWindowSize;

    u32 mRemoteRecvWindowSize; // Զ�˽��մ��ڴ�С
    u32 mCongestionWindowSize; // ӵ�����ڴ�СCongestion window size
    // ̽�������IKCP_ASK_TELL��ʾ��֪Զ�˴��ڴ�С��IKCP_ASK_SEND��ʾ����Զ�˸�֪���ڴ�С
    u32 mProbe;

    u32 mCurrent;       // ��ǰ��ʱ���
    u32 mInterval;      // �ڲ�flushˢ�¼��
    u32 mNextFlushTime; // �´�flushˢ��ʱ���
    u32 mXmit;

    u32 mRecvBufCount;   // ���ջ�������Ϣ����
    u32 mSendBufCount;   // ���ͻ�������Ϣ����
    u32 mRecvQueueCount; // ���ն�������Ϣ����
    u32 mSendQueueCount; // ���Ͷ�������Ϣ����
    u32 mNoDelay;        // �Ƿ��������ӳ�ģʽ

    // �Ƿ���ù�update�����ı�ʶ(��Ҫ�ϲ㲻�ϵĵ���KCProtocal::update��KCProtocal::check������kcp���շ�����)
    u32 mUpdated;

    u32 mNextProbeTime;   // �´�̽�鴰�ڵ�ʱ���
    u32 mProbeWait;       // ̽�鴰����Ҫ�ȴ���ʱ��
    u32 mDeadLink;        // ����ش�����
    u32 mIncr;            // �ɷ��͵����������
    Node2 mSendQueue;     // ������Ϣ�Ķ���
    Node2 mReceiveQueue;  // ������Ϣ�Ķ���
    Node2 mSendBuffer;    // ������Ϣ�Ļ���
    Node2 mReceiveBuffer; // ������Ϣ�Ļ���
    u32* mListACK;        // �����͵�ack�б�
    u32 mAckCount;        // mListACK��ack��������ÿ��ack��mListACK�д洢ts��sn������
    u32 mAckBlock;        // 2�ı�������ʶmListACK�������ɵ�ack����
    void* mUser;          // KCProtocal nevel use this param,just callback
    s8* mBuffer;
    s32 mFastResend; // ���������ش����ظ�ack����
    s32 mFastAckLimit;
    s32 mNoCongestionWindow; // ȡ��ӵ������
    s32 mStream;             // �Ƿ����������ģʽ
    s32 mLogMask;
    KcpSendRawCall mCallSendRaw;
    MemPool* mMemPool;
};

} // namespace net
} // namespace app

#endif // APP_KCPROTOCAL_H