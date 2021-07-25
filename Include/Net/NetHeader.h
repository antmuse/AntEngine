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
    u8  mVerAndSize;        // ��λIP�汾�ţ���λ�ײ�����
    u8  mType;              // ��������
    u16 mTotalSize;         // �ܵ����ݰ�����
    u16 mIdent;             // �����ʶ��
    u16 mFlag;              // 3λ��־λ,13λƬλ��
    u8  mTTL;               // ����ʱ��
    u8  mProtocol;          // Э�飨TCP��UDP�ȣ�
    u16 mChecksum;          // IPУ���=ipͷ��
    u32 mLocalIP;           // Դ��ַ
    u32 mRemoteIP;          // Ŀ���ַ

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
    u8  mType;                 //ICMP����ͷ
    u8  mCode;                 //ICMP����
    u16 mChecksum;             //�����=ICMPͷ+����
    u16 mID;                   //��ʶ��
    u16 mSN;                   //���к�
    u32 mTimestamp;            //ʱ���
};

struct SHeadOptionIP {
    u8 mType;           // ѡ������
    u8 mSize;           //ѡ��ͷ�ĳ���
    u8 mOffset;         //��ַƫ���� 
    u32 mAddress[9];    // IP��ַ�б�
};

struct SFakeHeadTCP {
    u32 mLocalIP;        //Դ��ַ
    u32 mRemoteIP;       //Ŀ�ĵ�ַ
    u8  mPadding;        //���λ��ȡ0
    u8  mProtocol;       //Э������
    u16 mSizeofTCP;      //����TCP����=SHeadTCP+SHeadOptionTCP+data(�����), ������SFakeHeadTCP��

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
        EFLAG_URG = 0x20,   //������־���ͽ���ָ�����ʹ�ã�����Ϊ1ʱ��ʾ���˱���Ҫ���촫�͡�
        EFLAG_ACK = 0x10,   //ȷ�ϱ�־����ȷ�Ϻ��ֶ����ʹ�ã���ACKλ��1ʱ��ȷ�Ϻ��ֶ���Ч��
        EFLAG_PSH = 0x08,   //Ϊ���ͱ�־����1ʱ�����ͷ����������ͻ������е����ݡ�
        EFLAG_RST = 0x04,   //��λ��־����1ʱ�����������ز�������ͷ����ӡ�
        EFLAG_SYN = 0x02,   //ͬ����־����1ʱ����ʾ���������ӡ�
        EFLAG_FIN = 0x01    //��ֹ��־����1ʱ�����������Ѿ������꣬�����ͷ����ӡ�
    };
    u16 mLocalPort;          //16λԴ�˿�
    u16 mRemotePort;         //16λĿ�Ķ˿�
    u32 mSN;                 //32λ���к�
    u32 mACK;                //32λȷ�Ϻ�
    u16 mSizeReserveFlag;    //4λTCPͷ����,6λ����λ,6λ��־λ
    u16 mWindow;             //16λ���ڴ�С
    u16 mChecksum;           //16λУ���=tcpαͷ��+tcpͷ��+data(�����)
    u16 mOffset;             //16λ��������ƫ����

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
    u8 mType;           //ѡ������
    u8 mSize;           //ѡ��ͷ�ĳ���
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
    u8  mRemoteMAC[6];          //Ŀ��MAC��ַ
    u8  mLocalMAC[6];           //ԴMAC��ַ
    u16 mType;                  //��һ��Э�����ͣ���0x0800������һ����IPЭ�飬0x0806Ϊarp
};

//28bytes
struct SHeadARP {
    u16 mHardwareType;      //Ӳ������,2�ֽڣ���������ARP����������ͣ���̫��������1
    u16 mProtocol;          //Э������,2�ֽڣ������ϲ�Э�����ͣ�����IPV4Э�飬���ֶ�ֵΪ0800
    u8 mHardwareAddressLen; //Ӳ����ַ����,8λ�ֶΣ������Ӧ�����ַ���ȣ���̫�������ֵΪ6
    u8 mProtocolAddressLen; //Э���ַ����,8λ�ֶΣ��������ֽ�Ϊ��λ���߼���ַ���ȣ���IPV4Э�����ֵΪ4
    u16 mOperation;            //�����ֶ�,���ݰ�����,ARP����ֵΪ1��������ARPӦ��ֵΪ2��
    u8 mLocalMAC[6];           //Դ�����Ͷˣ�mac��ַ,�ɱ䳤���ֶΣ�����̫������ֶ���6�ֽڳ�
    u8 mLocalIP[4];            //Դ�����Ͷ̣�ip��ַ,���Ͷ�Э���ַ���ɱ䳤���ֶΣ���IPЭ�飬����ֶ���4�ֽڳ�
    u8 mRemoteMAC[6];          //Ŀ�ģ����նˣ�mac��ַ
    u8 mRemoteIP[4];           //Ŀ�ģ����նˣ�ip��ַ,ע�ⲻ��Ϊu_int�ͣ��ṹ�����
};


struct SNetPackARP {
    SHeadEthernet mEthernet;
    SHeadARP mARP;
};

#pragma pack()

}//namespace net
}//namespace app

#endif //APP_NETHEADER_H
