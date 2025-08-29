#ifndef APP_HTTPEVTLUA_H
#define APP_HTTPEVTLUA_H

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"
#include "Script/ScriptManager.h"

namespace app {

class HttpEvtLua : public net::HttpEventer {
public:
    HttpEvtLua(const StringView& file);
    virtual ~HttpEvtLua();

    virtual s32 onLayerClose(net::HttpMsg* msg) override;

    // req parse err
    virtual s32 onReadError(net::HttpMsg* msg) override;

    virtual s32 onRespWrite(net::HttpMsg* msg) override;
    virtual s32 onRespWriteError(net::HttpMsg* msg) override;

    virtual s32 onReqHeadDone(net::HttpMsg* msg) override;
    virtual s32 onReqBody(net::HttpMsg* msg) override;
    virtual s32 onReqBodyDone(net::HttpMsg* msg) override;

    virtual s32 onReqChunkHeadDone(net::HttpMsg* msg) override;
    virtual s32 onReqChunkBodyDone(net::HttpMsg* msg) override;

    //virtual s32 onBackSent(RequestFD* it);
    //virtual s32 onBackFinish(RequestFD* it);
    //virtual s32 onBackBodyPart(RequestFD* it);
    //virtual s32 onBackOpen(RequestFD* it);
    //virtual s32 onBackClose();

private:
    // lua
    script::LuaThread mLuaThread;
    script::Script mScript;

    String mFileName;
    SRingBufPos mChunkPos;
    RingBuffer* mBody;
    RequestFD mReqs;
    net::HttpMsg* mMsg;
    usz mReaded;
    u16 mEvtFlags;

    void creatCurrContext();
    void onRead(RequestFD* it);
    void onClose(Handle* it);
    s32 launchRead();

    static void funcOnRead(RequestFD* it) {
        HttpEvtLua& nd = *(HttpEvtLua*)it->mUser;
        nd.onRead(it);
    }
    static void funcOnClose(Handle* it) {
        HttpEvtLua& nd = *(HttpEvtLua*)it->getUser();
        nd.onClose(it);
    }
};

} // namespace app
#endif // APP_HTTPEVTLUA_H