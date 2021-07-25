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


#ifndef APP_NETHEADER_H
#define APP_NETHEADER_H

#include "Config.h"


namespace app {
namespace net {

#pragma pack(1)

struct SHeadIP {
    enum EFlag {
        EFLAG_MORE_FRAG = 0x1,  //more fragment
        EFLAG_NO_FRAG = 0x2,    //don't fragment
        EFLAG_R = 0x4,          //abandon
    };
    //@see SHeadIP::mProtocol
    enum EProtocalType {
        EPT_ICMP = 1,
        EPT_TCP = 6,
        EPT_UDP = 17
    };
    u8  mVerAndSize;        // 四位IP版本号，四位首部长度
    u8  mType;              // 服务类型
    u16 mTotalSize;         // 总的数据包长度
    u16 mIdent;             // 特殊标识符
    u16 mFlag;              // 3位标志位,13位片位移
    u8  mTTL;               // 存在时间
    u8  mProtocol;          // 协议（TCP，UDP等）
    u16 mChecksum;          // IP校验和=ip头部
    u32 mLocalIP;           // 源地址
    u32 mRemoteIP;          // 目标地址

    //3bit flags
    DINLINE void setFlag(u16 it) {
#if defined(DENDIAN_BIG)
        mFlag = (mFlag & 0x1FFF) | (it << 13);
#else
        mFlag = (mFlag & 0xFF1F) | ((it & 0x0007) << 5);//bug clean
#endif
    }

    DINLINE u16 getFlag()const {
#if defined(DENDIAN_BIG)
        return (mFlag & 0xE000) >> 13;
#else
        return (mFlag & 0x000E) >> 5;
#endif
    }

    //Get ident, in OS endian.
    DINLINE u16 getIdent()const {
#if defined(DENDIAN_BIG)
        return mIdent;
#else
        return AppSwap16(mIdent);
#endif
    }


    DINLINE void setIdent(u16 it) {
#if defined(DENDIAN_BIG)
        mIdent = it;
#else
        mIdent = AppSwap16(it);
#endif
    }


    DINLINE void setTotalSize(u16 it) {
#if defined(DENDIAN_BIG)
        mTotalSize = it;
#else
        mTotalSize = AppSwap16(it);
#endif
    }

    //Get total size, in OS endian.
    DINLINE u16 getTotalSize()const {
#if defined(DENDIAN_BIG)
        return mTotalSize;
#else
        return AppSwap16(mTotalSize);
#endif
    }

    DINLINE void setVersion(u8 it) {
#if defined(DENDIAN_BIG)
        mVerAndSize = (mVerAndSize & 0xF0) | (it & 0x0F);
#else
        mVerAndSize = (mVerAndSize & 0x0F) | (it << 4);
#endif
    }

    DINLINE u8 getVersion()const {
#if defined(DENDIAN_BIG)
        return (mVerAndSize & 0x0F);
#else
        return (mVerAndSize & 0xF0) >> 4;
#endif
    }

    DINLINE void setSize(u8 it) {
        it >>= 2; // it/=sizeof(u32);  it/=4;
#if defined(DENDIAN_BIG)
        mVerAndSize = (mVerAndSize & 0x0F) | (it << 4);
#else
        mVerAndSize = (mVerAndSize & 0xF0) | (it & 0x0F);
#endif
    }

    DINLINE u8 getSize()const {
#if defined(DENDIAN_BIG)
        //return (mVerAndSize & 0xF0) >> 4;
        return (mVerAndSize & 0xF0) >> 2; //it*=4;
#else
        return (mVerAndSize & 0x0F) << 2; //it*=4;
#endif
    }
};

struct SHeadICMP {
    u8  mType;                 //ICMP报文头
    u8  mCode;                 //ICMP代码
    u16 mChecksum;             //检验和=ICMP头+数据
    u16 mID;                   //标识符
    u16 mSN;                   //序列号
    u32 mTimestamp;            //时间戳
};

struct SHeadOptionIP {
    u8 mType;           // 选项类型
    u8 mSize;           //选项头的长度
    u8 mOffset;         //地址偏移量 
    u32 mAddress[9];    // IP地址列表
};

struct SFakeHeadTCP {
    u32 mLocalIP;        //源地址
    u32 mRemoteIP;       //目的地址
    u8  mPadding;        //填充位，取0
    u8  mProtocol;       //协议类型
    u16 mSizeofTCP;      //整个TCP长度=SHeadTCP+SHeadOptionTCP+data(如果有), 不包括SFakeHeadTCP。

    DINLINE void setSize(u16 it) {
#if defined(DENDIAN_BIG)
        mSizeofTCP = it;
#else
        mSizeofTCP = AppSwap16(it);
#endif
    }
};

struct SHeadTCP {
    enum EFlag {
        EFLAG_URG = 0x20,   //紧急标志，和紧急指针配合使用，当其为1时表示，此报文要尽快传送。
        EFLAG_ACK = 0x10,   //确认标志，和确认号字段配合使用，当ACK位置1时，确认号字段有效。
        EFLAG_PSH = 0x08,   //为推送标志，置1时，发送方将立即发送缓冲区中的数据。
        EFLAG_RST = 0x04,   //复位标志，置1时，表明有严重差错，必须释放连接。
        EFLAG_SYN = 0x02,   //同步标志，置1时，表示请求建立连接。
        EFLAG_FIN = 0x01    //终止标志，置1时，表明数据已经发送完，请求释放连接。
    };
    u16 mLocalPort;          //16位源端口
    u16 mRemotePort;         //16位目的端口
    u32 mSN;                 //32位序列号
    u32 mACK;                //32位确认号
    u16 mSizeReserveFlag;    //4位TCP头长度,6位保留位,6位标志位
    u16 mWindow;             //16位窗口大小
    u16 mChecksum;           //16位校验和=tcp伪头部+tcp头部+data(如果有)
    u16 mOffset;             //16位紧急数据偏移量

    //Get SN, in OS endian.
    DINLINE u32 getSN() const {
#if defined(DENDIAN_BIG)
        return mSN;
#else
        return AppSwap32(mSN);
#endif
    }

    DINLINE void setSN(u32 it) {
#if defined(DENDIAN_BIG)
        mSN = it;
#else
        mSN = AppSwap32(it);
#endif
    }

    DINLINE void setACK(u32 it) {
#if defined(DENDIAN_BIG)
        mACK = it;
#else
        mACK = AppSwap32(it);
#endif
    }

    //Get ACK, in OS endian.
    DINLINE u32 getACK()const {
#if defined(DENDIAN_BIG)
        return mACK;
#else
        return AppSwap32(mACK);
#endif
    }

    //Get local port, in OS endian.
    DINLINE u16 getLocalPort()const {
#if defined(DENDIAN_BIG)
        return mLocalPort;
#else
        return AppSwap16(mLocalPort);
#endif
    }

    //Get remote port, in OS endian.
    DINLINE u16 getRemotePort()const {
#if defined(DENDIAN_BIG)
        return mRemotePort;
#else
        return AppSwap16(mRemotePort);
#endif
    }

    DINLINE void setWindowSize(u16 it) {
#if defined(DENDIAN_BIG)
        mWindow = it;
#else
        mWindow = AppSwap16(it);
#endif
    }

    //Get window size, in OS endian.
    DINLINE u16 getWindowSize() const {
#if defined(DENDIAN_BIG)
        return mWindow;
#else
        return AppSwap16(mWindow);
#endif
    }

    DINLINE void setSize(u16 it) {
        it >>= 2; // it/=sizeof(u32);  it/=4;
#if defined(DENDIAN_BIG)
        mSizeReserveFlag = (mSizeReserveFlag & 0x0FFF) | (it << 12);
#else
        mSizeReserveFlag = (mSizeReserveFlag & 0xFF0F) | ((it & 0x000F) << 4);//bug clean
#endif
    }

    DINLINE u8 getSize()const {
#if defined(DENDIAN_BIG)
        return ((mSizeReserveFlag & 0xF000) >> 10);   //it*=4;
#else
        return (mSizeReserveFlag & 0x00F0) >> 2; //it*=4;
#endif
    }

    DINLINE void setFlag(u16 it) {
#if defined(DENDIAN_BIG)
        mSizeReserveFlag = (mSizeReserveFlag & 0xFFC0) | (it & 0x003F);
#else
        mSizeReserveFlag = (mSizeReserveFlag & 0xC0FF) | ((it & 0x003F) << 8); //bug clean
#endif
    }

    DINLINE u8 getFlag()const {
#if defined(DENDIAN_BIG)
        return (mSizeReserveFlag & 0x003F);
#else
        return (mSizeReserveFlag & 0x3F00) >> 8;//bug clean
#endif
    }
};

struct SHeadOptionTCP {
    enum EType {
        ETYPE_OPTION_END = 0x0,     //option end
        ETYPE_NO_OPTION = 0x1,      //No option
        ETYPE_MSS = 0x2,            //MSS
        ETYPE_WIN_SCALE = 0x3,      //Window enlarge rate
        ETYPE_SACK_PERMITTED = 0x4,   //SACK permitted
        ETYPE_TIMESTAMP = 0x8,      //Timestamp
    };
    u8 mType;           //选项类型
    u8 mSize;           //选项头的长度
    u16 mMSS;           //MSS
    u8 mOther[8];       //Other options

    DINLINE void setMSS(u16 it) {
#if defined(DENDIAN_BIG)
        mMSS = it;
#else
        mMSS = AppSwap16(it);
#endif
    }

};

struct SHeadUDP {
    u16 mLocalPort;       // Source port
    u16 mRemotePort;      // Destination port
    u16 mSize;            // Datagram length
    u16 mChecksum;        // Checksum
};

//14bytes
struct SHeadEthernet {
    u8  mRemoteMAC[6];          //目的MAC地址
    u8  mLocalMAC[6];           //源MAC地址
    u16 mType;                  //上一层协议类型，如0x0800代表上一层是IP协议，0x0806为arp
};

//28bytes
struct SHeadARP {
    u16 mHardwareType;      //硬件类型,2字节，定义运行ARP的网络的类型，以太网是类型1
    u16 mProtocol;          //协议类型,2字节，定义上层协议类型，对于IPV4协议，该字段值为0800
    u8 mHardwareAddressLen; //硬件地址长度,8位字段，定义对应物理地址长度，以太网中这个值为6
    u8 mProtocolAddressLen; //协议地址长度,8位字段，定义以字节为单位的逻辑地址长度，对IPV4协议这个值为4
    u16 mOperation;            //操作字段,数据包类型,ARP请求（值为1），或者ARP应答（值为2）
    u8 mLocalMAC[6];           //源（发送端）mac地址,可变长度字段，对以太网这个字段是6字节长
    u8 mLocalIP[4];            //源（发送短）ip地址,发送端协议地址，可变长度字段，对IP协议，这个字段是4字节长
    u8 mRemoteMAC[6];          //目的（接收端）mac地址
    u8 mRemoteIP[4];           //目的（接收端）ip地址,注意不能为u_int型，结构体对其
};


struct SNetPackARP {
    SHeadEthernet mEthernet;
    SHeadARP mARP;
};

#pragma pack()

}//namespace net
}//namespace app

#endif //APP_NETHEADER_H
