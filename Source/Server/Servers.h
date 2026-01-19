#pragma once

#include "RefCount.h"
#include "Engine.h"
#include "ServerConfig.h"


namespace app {

class Servers : public RefCount {
public:
    Servers();

    ~Servers();

    s32 start();

    bool isActive() {
        return mTime.getGrabCount() > 0;
    }

    void onClose(Handle* it);

    s32 onTimeout(HandleTime* it);

    static void funcOnClose(Handle* it) {
        Servers& nd = *(Servers*)it->getUser();
        nd.onClose(it);
    }

    static s32 funcOnTime(HandleTime* it) {
        Servers& nd = *(Servers*)it->getUser();
        return nd.onTimeout(it);
    }


private:
    HandleTime mTime;
    ServerConfig mConfig;
    s32 mLogFlushCount = 0;
    void processTask(void* it);
};

} // namespace app
