#ifndef APP_WEBSITE_H
#define	APP_WEBSITE_H

#include "TVector.h"
#include "Net/Acceptor.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/HTTP/MsgStation.h"
#include "Net/TlsContext.h"

namespace app {
namespace net {



/**
 * @brief Website bind with Acceptor & HttpLayer.
 *        HttpLayer bind with HandleTCP & HttpMsg;
 *        HttpMsg bind with logic worker.
 */
class Website :public RefCount {
public:
    Website(EngineConfig::WebsiteCfg& cfg);
    virtual ~Website();

    static void funcOnLink(RequestFD* it) {
        DASSERT(it && it->mUser);
        net::Acceptor* accp = reinterpret_cast<net::Acceptor*>(it->mUser);
        Website* web = reinterpret_cast<Website*>(accp->getUser());
        web->onLink(it);
    }

    s32 stepMsg(HttpMsg* msg);

    bool setStation(EStationID id, MsgStation* it);

    MsgStation* getStation(u32 idx)const {
        if (idx < ES_COUNT) {
            return mStations[idx];
        }
        return nullptr;
    }

    const EngineConfig::WebsiteCfg& getConfig()const {
        return mConfig;
    }

    const TlsContext& getTlsContext() const {
        return mTlsContext;
    }

    void bind(HttpLayer* it) {
        if (it) {
            it->grab();
            grab();
        }
    }

    void unbind(HttpLayer* it) {
        if (it) {
            drop();
            it->drop();
        }
    }


private:
    //friend class HttpLayer;

    void init();
    void loadTLS();
    void clear();

    void onLink(RequestFD* it) {
        HttpLayer* con = new HttpLayer();
        con->onLink(it);
        con->drop();
    }

    TlsContext mTlsContext;
    TVector<MsgStation*> mStations;
    TVector<MsgStation*> mLineLua;
    TVector<MsgStation*> mLineStatic;
    TVector<MsgStation*> mLinePre;
    TVector<MsgStation*> mLinePost;
    EngineConfig::WebsiteCfg& mConfig;
};

}//namespace net
}//namespace app

#endif //APP_WEBSITE_H