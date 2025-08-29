#ifndef APP_APPTICKER_H
#define	APP_APPTICKER_H

#include "MsgHeader.h"
#include "RefCount.h"
#include "Loop.h"


namespace app {


class AppTicker  : public RefCount {
public:
    AppTicker();

    ~AppTicker();

    s32 start();

    bool isActive() {
        return mTime.getGrabCount() > 0;
    }

    void onClose(Handle* it);

    s32 onTimeout(HandleTime* it);

    static void http_task(void* url);

    static usz gTotalPacketIn;
    static usz gTotalSizeIn;
    static usz gTotalPacketOut;
    static usz gTotalSizeOut;
    static usz gTotalActive;
    static usz gTotalActiveResp;

private:
    static void funcOnClose(Handle* it) {
        AppTicker& nd = *(AppTicker*)it->getUser();
        nd.onClose(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        AppTicker& nd = *(AppTicker*)it->getUser();
        return nd.onTimeout(it);
    }

    HandleTime mTime;
    Loop& mLoop;
};

}//namespace app


#endif //APP_APPTICKER_H