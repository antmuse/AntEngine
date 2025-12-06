#ifndef APP_HTTPEVTLUA_H
#define APP_HTTPEVTLUA_H

#include "HandleFile.h"
#include "Net/HTTP/HttpLayer.h"
#include "Script/ScriptManager.h"

namespace app {

class HttpEvtLua : public net::HttpEventer {
public:
    enum ERespStep {
        RSTEP_INIT = 0,
        RSTEP_MASK = 0x7,

        RSTEP_HEAD_END = 1,
        RSTEP_BODY_PART = 2,
        RSTEP_BODY_END = 4,
        RSTEP_STEP_CHUNK = 8,
    };
    HttpEvtLua();
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

    // virtual s32 onBackSent(RequestFD* it);
    // virtual s32 onBackFinish(RequestFD* it);
    // virtual s32 onBackBodyPart(RequestFD* it);
    // virtual s32 onBackOpen(RequestFD* it);
    // virtual s32 onBackClose();
    /**
     * @param bodyLen:  chunk if <0, else set length in head.
     */
    s32 writeRespLine(s32 num, const s8* brief, s64 bodyLen = -1);
    s32 writeRespHeader(const s8* name, const s8* val);
    s32 writeRespBody(const s8* buf, usz len);
    /**
     * @param step: resp step, @see ERespStep
     */
    s32 sendResp(u32 step);


private:
    // lua
    script::LuaThread mLuaThread;
    script::Script mScript;
    s32 mRespStep = 0;

    String mFileName;
    SRingBufPos mChunkPos;
    RingBuffer* mBody;
    RequestFD mReqs;
    net::HttpMsg* mMsg;
    usz mReaded;
    u16 mEvtFlags;

    /** @brief set context for coroutine */
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