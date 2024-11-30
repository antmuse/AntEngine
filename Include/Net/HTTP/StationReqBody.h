#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationReqBody : public MsgStation {
public:
    StationReqBody() { };
    virtual ~StationReqBody() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};

} // namespace net
} // namespace app
