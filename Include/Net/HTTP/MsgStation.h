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
    MsgStation() : mNext(nullptr) { };
    virtual ~MsgStation() { };

    /**
     * @return 0 if success else error
    */
    virtual s32 onMsg(HttpMsg* msg) = 0;

protected:
    MsgStation* mNext;
};


class StationInit : public MsgStation {
public:
    StationInit() { };
    virtual ~StationInit() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


class StationPath : public MsgStation {
public:
    StationPath() { };
    virtual ~StationPath() { };
    virtual s32 onMsg(HttpMsg* msg) override;
private:
    s32 check(HttpMsg* msg);
};


/***************************************************************************************************
* @brief station for in coming headers
***************************************************************************************************/
class StationReqHead : public MsgStation {
public:
    StationReqHead() { };
    virtual ~StationReqHead() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


/***************************************************************************************************
* @brief station for public headers
***************************************************************************************************/
class StationBody : public MsgStation {
public:
    StationBody() { };
    virtual ~StationBody() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


class StationBodyDone : public MsgStation {
public:
    StationBodyDone() { };
    virtual ~StationBodyDone() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


class StationRespHead : public MsgStation {
public:
    StationRespHead() { };
    virtual ~StationRespHead() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


class StationRespBody : public MsgStation {
public:
    StationRespBody() { };
    virtual ~StationRespBody() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


class StationRespBodyDone : public MsgStation {
public:
    StationRespBodyDone() { };
    virtual ~StationRespBodyDone() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};

class StationError : public MsgStation {
public:
    StationError() { };
    virtual ~StationError() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


class StationClose : public MsgStation {
public:
    StationClose() { };
    virtual ~StationClose() { };
    virtual s32 onMsg(HttpMsg* msg) override;
};


} //namespace net
} //namespace app
#endif //APP_MSGSTATION_H
