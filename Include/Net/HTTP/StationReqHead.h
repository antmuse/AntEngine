#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationReqHead : public MsgStation {
public:
    StationReqHead() { };
    virtual ~StationReqHead() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


} // namespace net
} // namespace app
