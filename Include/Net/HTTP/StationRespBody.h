#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationRespBody : public MsgStation {
public:
    StationRespBody() { };
    virtual ~StationRespBody() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


} // namespace net
} // namespace app
