#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationReqBodyDone : public MsgStation {
public:
    StationReqBodyDone(){};
    virtual ~StationReqBodyDone(){};
    virtual s32 onMsg(HttpMsg* msg) override;
};


} // namespace net
} // namespace app
