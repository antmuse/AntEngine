#pragma once

#include "Net/HTTP/MsgStation.h"

namespace app {
namespace net {

class StationReqHead : public MsgStation {
public:
    StationReqHead() { };
    virtual ~StationReqHead() { };
    virtual s32 onMsg(HttpMsg* msg) override;

private:
    HttpEventer* createEvt(HttpMsg* msg);
};


} // namespace net
} // namespace app
