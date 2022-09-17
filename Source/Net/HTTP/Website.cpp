#include "Net/HTTP/Website.h"

namespace app {
namespace net {

Website::Website(EngineConfig::WebsiteCfg& cfg)
    : mConfig(cfg) {
    init();
}

Website::~Website() {
    clear();
}

s32 Website::stepMsg(HttpMsg* msg) {
    if (!msg) {
        return EE_ERROR;
    }
    MsgStation* stn = getStation(msg->getStationID());
    if (!stn) {
        stn = getStation(ES_ERROR);
        DASSERT(stn);
    }
    return stn->onMsg(msg);
}


void Website::clear() {
    for (s32 i = 0; i < ES_COUNT; ++i) {
        if (mStations[i]) {
            mStations[i]->drop();
            mStations[i] = nullptr;
        }
    }
}

void Website::init() {
    mStations.resize(ES_COUNT);
    memset(mStations.getPointer(), 0, sizeof(mStations[0]) * mStations.size());

    MsgStation* nd = new StationInit();
    setStation(ES_INIT, nd);
    nd->drop();

    nd = new StationPath();
    setStation(ES_PATH, nd);
    nd->drop();

    nd = new StationReqHead();
    setStation(ES_HEAD, nd);
    nd->drop();

    nd = new StationBody();
    setStation(ES_BODY, nd);
    nd->drop();

    nd = new StationBodyDone();
    setStation(ES_BODY_DONE, nd);
    nd->drop();

    nd = new StationRespHead();
    setStation(ES_RESP_HEAD, nd);
    nd->drop();

    nd = new StationRespBody();
    setStation(ES_RESP_BODY, nd);
    nd->drop();

    nd = new StationRespBodyDone();
    setStation(ES_RESP_BODY_DONE, nd);
    nd->drop();

    nd = new StationError();
    setStation(ES_ERROR, nd);
    nd->drop();

    nd = new StationClose();
    setStation(ES_CLOSE, nd);
    nd->drop();
}

bool Website::setStation(EStationID id, MsgStation* it) {
    if (id >= ES_COUNT || !it) {
        return false;
    }
    if (mStations[id]) {
        mStations[id]->drop();
    }
    if (it) {
        it->grab();
    }
    mStations[id] = it;
    return true;
}


}  // namespace net
}  // namespace app