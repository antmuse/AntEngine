#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationRespHead : public MsgStation {
public:
    StationRespHead() { };
    virtual ~StationRespHead() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};

} // namespace net
} // namespace app
