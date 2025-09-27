#ifndef APP_WEBSITE_H
#define APP_WEBSITE_H

#include "TVector.h"
#include "Net/Acceptor.h"
#include "Net/HTTP/HttpLayer.h"
#include "Net/TlsContext.h"

namespace app {
namespace net {

/**
 * @brief Website bind with Acceptor & HttpLayer.
 *        HttpLayer bind with HandleTCP & HttpMsg;
 *        HttpMsg bind with logic worker.
 */
class Website : public RefCount {
public:
    Website(EngineConfig::WebsiteCfg& cfg);
    virtual ~Website();

    /**
     * @brief called when http_req_head is readed.
     */
    s32 createMsgEvent(HttpMsg* msg);

    static void funcOnLink(RequestFD* it) {
        DASSERT(it && it->mUser);
        net::Acceptor* accp = reinterpret_cast<net::Acceptor*>(it->mUser);
        Website* web = reinterpret_cast<Website*>(accp->getUser());
        web->onLink(it);
    }

    const EngineConfig::WebsiteCfg& getConfig() const {
        return mConfig;
    }

    const TlsContext& getTlsContext() const {
        return mTlsContext;
    }

private:
    void init();
    void clear();

    void onLink(RequestFD* it) {
        HttpLayer* con = new HttpLayer(EHTTP_REQUEST, 1 == getConfig().mType, &mTlsContext);
        con->onLink(it);
        con->drop();
    }

    TlsContext mTlsContext;
    EngineConfig::WebsiteCfg& mConfig;
};

} // namespace net
} // namespace app

#endif // APP_WEBSITE_H