#ifndef APP_APPTICKER_H
#define	APP_APPTICKER_H

#include "Loop.h"
#include "MsgHeader.h"


namespace app {
namespace net {

enum EPackType {
    EPT_RESP_BIT = 0x8000,

    EPT_ACTIVE = 0x0001,
    EPT_ACTIVE_RESP = EPT_RESP_BIT| EPT_ACTIVE,
    EPT_SUBMIT = 0x0002,
    EPT_SUBMIT_RESP = EPT_RESP_BIT | EPT_SUBMIT,

    EPT_VERSION = 0xF001
};

//心跳包
struct PackActive : MsgHeader {
    void pack(u32 sn) {
        finish(EPT_ACTIVE, sn, EPT_VERSION);
    }
    //心跳包响应
    void packResp(u32 sn) {
        finish(EPT_ACTIVE_RESP, sn, EPT_VERSION);
    }
};


struct PackSubmit : MsgHeader {
    s32 mPacketID;
    s8 mBuffer[1];
    enum EPackItem {
        EI_NAME = 0,
        EI_VALUE,
        EI_COUNT
    };

    PackSubmit() {
        clear();
    }
    void clear() {
        init(EPT_SUBMIT, DOFFSET(PackSubmit, mBuffer), EI_COUNT, 1);
    }
    void writeHeader(u32 sn) {
        mSN = sn;
    }
};
struct PackSubmitResp : MsgHeader {
    void writeHeader(u32 sn) {
        finish(EPT_SUBMIT_RESP, sn, 1);
    }
};
} //net



class AppTicker {
public:
    AppTicker(Loop& loop);

    ~AppTicker();

    s32 start();

    bool isActive() {
        return mTime.getGrabCount() > 0;
    }

    void onClose(Handle* it);

    s32 onTimeout(HandleTime* it);

    static void funcOnClose(Handle* it) {
        AppTicker& nd = *(AppTicker*)it->getUser();
        nd.onClose(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        AppTicker& nd = *(AppTicker*)it->getUser();
        return nd.onTimeout(it);
    }


    static usz gTotalPacketIn;
    static usz gTotalSizeIn;
    static usz gTotalPacketOut;
    static usz gTotalSizeOut;
    static usz gTotalActive;
    static usz gTotalActiveResp;


private:
    HandleTime mTime;
    Loop& mLoop;
};

}//namespace app


#endif //APP_APPTICKER_H