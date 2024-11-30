#ifndef APP_MSGSTATION_H
#define APP_MSGSTATION_H
#include "Net/HTTP/HttpMsg.h"

namespace app {
namespace net {

enum EStationID {
    ES_INIT = 0,
    ES_PATH,
    ES_HEAD,
    ES_BODY,
    ES_BODY_DONE,
    ES_RESP_HEAD,
    ES_RESP_BODY,
    ES_RESP_BODY_DONE,
    ES_ERROR,
    ES_CLOSE,
    ES_COUNT
};

class MsgStation : public RefCount {
public:
    MsgStation() : mNext(nullptr){};
    virtual ~MsgStation(){};

    /**
     * @return 0 if success else error
     */
    virtual s32 onMsg(HttpMsg* msg) = 0;

protected:
    MsgStation* mNext;
};


class StationInit : public MsgStation {
public:
    StationInit(){};
    virtual ~StationInit(){};
    virtual s32 onMsg(HttpMsg* msg) override;
};


} // namespace net
} // namespace app
#endif // APP_MSGSTATION_H
