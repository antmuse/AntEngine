#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationClose : public MsgStation {
public:
    StationClose() { };
    virtual ~StationClose() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};

} // namespace net
} // namespace app
