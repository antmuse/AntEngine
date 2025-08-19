#pragma once

#include "MsgHeader.h"

namespace app {
namespace net {

enum EStatusNAT{
    ESN_REGIST = 0,
    ESN_FIND = 1,
    ESN_CONNECT = 2,
    ESN_ACTIVE  = 3
};

enum EPackType {
    EPT_RESP_BIT = 0x8000,

    EPT_ACTIVE = 0x0001,
    EPT_ACTIVE_RESP = EPT_RESP_BIT | EPT_ACTIVE,
    EPT_REGIST = 0x0002,
    EPT_REGIST_RESP = EPT_RESP_BIT | EPT_REGIST,
    EPT_FIND = 0x0003,
    EPT_FIND_RESP = EPT_RESP_BIT | EPT_FIND,
    EPT_SUBMIT = 0x0004,
    EPT_SUBMIT_RESP = EPT_RESP_BIT | EPT_SUBMIT,

    EPT_VERSION = 0x1,
    EPT_MAX_SIZE = 1500   // max udp packet
};

// 心跳包
struct PackActive : MsgHeader {
    void pack(u32 sn) {
        finish(EPT_ACTIVE, sn, EPT_VERSION);
    }
    // 心跳包响应
    void packResp(u32 sn) {
        finish(EPT_ACTIVE_RESP, sn, EPT_VERSION);
    }
};


struct PackRegist : MsgHeader {
    u64 mUserID;
    s8 mBuffer[128];
    enum EPackItem {
        EI_NAME = 0,
        EI_LOC_ADDR,
        EI_COUNT
    };

    PackRegist() {
        clear();
    }
    void clear() {
        init(EPT_REGIST, DOFFSET(PackRegist, mBuffer), EI_COUNT, EPT_VERSION);
    }
    void writeHeader(u32 sn) {
        mSN = sn;
    }
};
struct PackRegistResp : MsgHeader {
    u64 mUserID;
    s8 mBuffer[128];
    enum EPackItem {
        EI_NAME = 0,
        EI_ADDR,
        EI_COUNT
    };

    PackRegistResp() {
        clear();
    }
    void clear() {
        init(EPT_REGIST_RESP, DOFFSET(PackRegistResp, mBuffer), EI_COUNT, EPT_VERSION);
    }
    void writeHeader(u32 sn) {
        mSN = sn;
    }
};


struct PackFind : MsgHeader {
    u64 mUserID;
    enum EPackItem {
        EI_COUNT
    };

    PackFind() {
        clear();
    }
    void clear() {
        init(EPT_FIND, sizeof(PackFind), EI_COUNT, EPT_VERSION);
    }
    void writeHeader(u32 sn) {
        mSN = sn;
    }
};

struct PackFindResp : MsgHeader {
    u64 mUserID;
    s8 mBuffer[128];
    enum EPackItem {
        EI_PEER_ADDR,
        EI_COUNT
    };

    PackFindResp() {
        clear();
    }
    void clear() {
        init(EPT_FIND_RESP, DOFFSET(PackFindResp, mBuffer), EI_COUNT, EPT_VERSION);
    }
    void writeHeader(u32 sn) {
        mSN = sn;
    }
};


} // namespace net
} // namespace app
