#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationReqBodyDone : public MsgStation {
public:
    StationReqBodyDone(){};
    virtual ~StationReqBodyDone(){};
    virtual s32 onMsg(HttpMsg* msg) override;

private:
    HttpEventer* createEvt(HttpMsg* msg);
};


} // namespace net
} // namespace app
