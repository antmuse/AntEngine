#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationRespBodyDone : public MsgStation {
public:
    StationRespBodyDone() { };
    virtual ~StationRespBodyDone() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


} // namespace net
} // namespace app
