#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationError : public MsgStation {
public:
    StationError(){};
    virtual ~StationError(){};
    virtual s32 onMsg(HttpMsg* msg) override;

private:
};


} // namespace net
} // namespace app
