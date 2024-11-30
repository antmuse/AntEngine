#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationReqPath : public MsgStation {
public:
    StationReqPath() { };
    virtual ~StationReqPath() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};

} // namespace net
} // namespace app
