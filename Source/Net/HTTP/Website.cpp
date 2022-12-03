#include "Net/HTTP/Website.h"
#include "Logger.h"
#include "FileReader.h"
#include "Packet.h"

namespace app {
namespace net {

static const s8* G_CA_FILE = "ca.crt";
static const s8* G_CERT_FILE = "server.crt";
static const s8* G_PRIVATE_FILE = "server.unsecure.key";

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


void Website::loadTLS() {
    if (1 != mConfig.mType) { //ssl
        return;
    }
    Packet buf(1024);
    FileReader keyfile;
    if (keyfile.openFile(mConfig.mPathTLS + G_CA_FILE)) {
        buf.resize(keyfile.getFileSize());
        if (buf.size() == keyfile.read(buf.getPointer(), buf.size())) {
            if (EE_OK != mTlsContext.addTrustedCerts(buf.getPointer(), buf.size())) {
                Logger::logError("Website::init, host=%s, ca err=%s", mConfig.mLocal.getStr(), G_CERT_FILE);
            }
        }
    }
    if (keyfile.openFile(mConfig.mPathTLS + G_CERT_FILE)) {
        buf.resize(keyfile.getFileSize());
        if (buf.size() == keyfile.read(buf.getPointer(), buf.size())) {
            if (EE_OK != mTlsContext.setCert(buf.getPointer(), buf.size())) {
                Logger::logError("Website::init, host=%s, server ca err=%s", mConfig.mLocal.getStr(), G_CERT_FILE);
            }
        }
    }
    if (keyfile.openFile(mConfig.mPathTLS + G_PRIVATE_FILE)) {
        buf.resize(keyfile.getFileSize());
        if (buf.size() == keyfile.read(buf.getPointer(), buf.size())) {
            if (EE_OK != mTlsContext.setPrivateKey(buf.getPointer(), buf.size())) {
                Logger::logError("Website::init, host=%s, private err=%s", mConfig.mLocal.getStr(), G_PRIVATE_FILE);
            }
        }
    }
}


void Website::init() {
    mTlsContext.init(ETLS_VERIFY_CERT);
    loadTLS();

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