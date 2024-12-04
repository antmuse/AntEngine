#ifndef APP_MSGSTATION_H
#define APP_MSGSTATION_H
#include "Net/HTTP/HttpMsg.h"

namespace app {
namespace net {

class MsgStation : public RefCount {
public:
    MsgStation() : mNext(nullptr){};
    virtual ~MsgStation(){};

    /**
     * @return 0 if success else error
     */
    virtual s32 onMsg(HttpMsg* msg) = 0;

protected:
    MsgStation* mNext;
};


class StationInit : public MsgStation {
public:
    StationInit(){};
    virtual ~StationInit(){};
    virtual s32 onMsg(HttpMsg* msg) override;
};


} // namespace net
} // namespace app
#endif // APP_MSGSTATION_H
