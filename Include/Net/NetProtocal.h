/**
*@file NetProtocal.h
*@brief A Better ARQ Protocol Implementation
*
*  Features:
*  + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
*  + Maximum RTT reduce three times vs tcp.
*  + Lightweight, distributed as a single source file.
*
* Test result:
* 1. default mode result (20917ms):
* avgrtt=740 maxrtt=1507
*
* 2. normal mode result (20131ms):
* avgrtt=156 maxrtt=571
*
* 3. fast mode result (20207ms):
* avgrtt=138 maxrtt=392
*/

#ifndef APP_NETPROTOCAL_H
#define APP_NETPROTOCAL_H

#include "Node.h"

namespace app {
namespace net {
enum ENetProtocal {
    ENET_LOG_OUTPUT = 1,
    ENET_LOG_INPUT = 2,
    ENET_LOG_SEND = 4,
    ENET_LOG_RECEIVE = 8,
    ENET_LOG_IN_DATA = 16,
    ENET_LOG_IN_ACK = 32,
    ENET_LOG_IN_PROBE = 64,
    ENET_LOG_IN_WINDOW = 128,
    ENET_LOG_OUT_DATA = 256,
    ENET_LOG_OUT_ACK = 512,
    ENET_LOG_OUT_PROBE = 1024,
    ENET_LOG_OUT_WINDOW = 2048
};


class INetDataSender {
public:
    INetDataSender() {
    };
    virtual ~INetDataSender() {
    }
    virtual s32 sendBuffer(void* iUserPointer, const s8* iData, s32 iLength) = 0;
};



class NetProtocal {
public:
    struct SKCPSegment {
        u32 mConnectionID;											///<connection id, must equal in two endpoint from the same connection.
        u32 mCMD;													///<Commands
        u32 mFrag;
        u32 mWindow;
        u32 mTime;
        u32 mSN;													///<SN
        u32 mUNA;
        u32 mLength;												///<length of data buffer: mData
        u32 mResendTime;											///<Resend time
        u32 mRTO;
        u32 mFastACK;
        u32 mXMIT;
        s8 mData[1];
    };

    Node2* createSegment(s32 size);

    void log(s32 mask, const s8* fmt, ...);
    void updateACK(s32 rtt);
    void shrinkBuffer();
    void parseSegment(Node2* newnode);
    void getACK(s32 p, u32* sn, u32* ts);
    void appendACK(u32 sn, u32 ts);
    void parseACK(u32 sn);
    void parseUNA(u32 una);
    void parseFastACK(u32 sn);
    s8* encodeSegment(s8* ptr, const SKCPSegment* seg);
    s32 getUnusedWindowCount();

    // create a new kcp control object
    NetProtocal();

    // release kcp control object
    ~NetProtocal();

    ///set connection ID
    void setID(u32 id) {
        mConnectionID = id;
    }

    ///get connection ID
    u32 getID() const {
        return mConnectionID;
    }

    ///get connection ID
    u32 getID(const s8* iBuffer) const;

    void setUserPointer(void* it) {
        mUserPointer = it;
    }


    DFINLINE void* getUserPointer() const {
        return mUserPointer;
    }


    void setMinRTO(s32 rto) {
        mMinRTO = rto;
    }

    void setFastResend(s32 it) {
        mFastResend = it;
    }

    /**
    *@brief set callback which wrapped the real socket.
    */
    void setSender(INetDataSender* iSender);

    /**
    *@brief User or upper level receiver.
    *@param iBuffer The cache used to receive data.
    *@param iLength The cache size.
    *@return received size or below zero if error.
    */
    s32 receiveData(s8* iBuffer, s32 iLength);

    /**
   *@brief User or upper level sender.
   *@param iBuffer The cache used to receive data.
   *@param iLength The cache size.
   *@return 0 if success, or below zero if error.
   */
    s32 sendData(const s8* iBuffer, s32 iLength);

    /**
    *@brief update state (call it repeatedly, every 10ms-100ms), or you can ask
    *check() when to call it again (without import/_send calling).
    *@param current current timestamp in millisec.
    */
    void update(u64 current);

    /**
    *@brief Determine when should you invoke next update() instead of call update repeatly.
    * It's important to reduce unnacessary update() invoking.
    * use it to schedule update (eg. implementing an epoll-like mechanism,
    * or optimize update() when handling massive connections)
    *@return When should you invoke next update() in millisecond, if there
    * is no import()/sendData() calling.
    */
    u64 check(u64 current);

    /**
    *@brief Write raw data(eg. UDP packet) into NetProtocal.
    *@param iBuffer The low level data.
    *@param iLength The data size.
    *@return 0 if success, else below zero.
    */
    s32 import(const s8* iBuffer, long size);

    /// flush pending data
    void flush();

    ///check the size of next message in the recv queue
    s32 peekNextSize();

    ///set Maximum Transfer Unit, default is 1400
    s32 setMTU(s32 iMTU);

    /**
    *@brief set maximum window size.
    *@param iSendWindow Max send window size, default is 32.
    *@param iReceiveWindow Max receive window size, default is 32.
    */
    void setMaxWindowSize(s32 iSendWindow, s32 iReceiveWindow);

    /// get how many packet is waiting to be sent
    s32 getWaitSendCount();

    // fastest: setNodelay(1, 20, 2, 1)
    // nodelay: 0:disable(default), 1:enable
    // interval: internal update timer interval in millisecond, default is 100ms 
    // resend: 0:disable fast resend(default), 1:enable fast resend
    // nc: 0:normal congestion control(default), 1:disable congestion control
    s32 setNodelay(s32 nodelay, s32 interval, s32 resend, s32 nc);


    /**
    *@brief Set internal update time interval in millisecond, default is 100ms.
    *@param interval:  time interval in millisecond, range(10-5000).
    */
    void setInterval(s32 interval);

    /**
    *@brief Set stream mode, default is false.
    *@param it true: enable stream mode, else false.
    */
    void setStreamMode(bool it) {
        mStreamMode = it;
    }

private:
    s32 sendOut(const s8* data, s32 size);
    DINLINE void releaseMemory(void *ptr);
    DINLINE void* allocateMemory(s32 size);
    //s32 receiveBufferCount();
    //s32 sendBufferCount();
    //void setAllocator(AppMallocFunction iMalloc, AppFreeFunction iFree);


    u32 mMTU;																		///<Maximum Transfer Unit
    u32 mConnectionID;													///<connection id, must equal in two endpoint from the same connection.
    u32 mMSS;																		///<Max Segment Size
    u32 mState;
    u32 mSendUNA;
    u32 mSendNext;
    u32 mReceiveNext;
    u32 mSSThreshold;														///<Slow start threshold
    s32 mRTT;
    s32 mSRTT;
    s32 mRTO;
    s32 mMinRTO;
    u32 mMaxSendWindowSize;                                    ///<Max send window size
    u32 mMaxReceiveWindowSize;								 ///<Max receive window size
    u32 mWindowRemote;												///<Remote window
    u32 mWindowCongestion;										///<Congestion window
    u32 mProbeFlag;															///<Probe flags
    u64 mTimeCurrent;                                                      ///<Current time
    u64 mTimeFlush;                                                           ///<flush time
    u32 mXMIT;
    u32 mInterval;																///<internal update time interval in millisecond, default is 100ms 
    u32 mNodelay;																///<0:disable(default), 1:enable
    u32 mUpdated;
    u32 mTimeProbe;
    u32 mTimeProbeWait;
    u32 mDeadLink;
    u32 mIncrease;
    u32 mReceiveBufferCount;
    u32 mSendBufferCount;
    u32 mReceiveQueueCount;
    u32 mSendQueueCount;
    Node2 mQueueSend;
    Node2 mQueueReceive;
    Node2 mQueueSendBuffer;
    Node2 mQueueReceiveBuffer;
    s32 mFastResend;															///<0:disable fast resend(default), 1:enable fast resend
    s32 mNoCongestionControl;										///<0:normal congestion control(default), 1:disable congestion control
    u32 mCountACK;															///<ACK count
    u32 mBlockACK;
    u32* mListACK;																///<ACK list
    void* mUserPointer;													///<will be passed to the output callback
    s8* mBuffer;
    INetDataSender* mSender;                                      ///<The real socket wrapper.
    bool mStreamMode;                                                   ///<stream mode, default is false.
    s32 mLogMask;
    //AppMallocFunction mMallocHook;
    //AppFreeFunction mFreeHook;
    void(*mLogWriter)(const s8 *log, NetProtocal* kcp, void *user);
};

} //namespace net 
} //namespace app 


#endif //APP_NETPROTOCAL_H


