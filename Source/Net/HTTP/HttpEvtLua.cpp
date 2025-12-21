#include "Net/HTTP/HttpEvtLua.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Script/LuaFunc.h"


namespace app {
namespace script {
extern s32 LuaHttpEventNew(lua_State* vm, HttpEvtLua* evt);
}


static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtLua::HttpEvtLua() {
    mReqs.mCall = HttpEvtLua::funcOnRead;
    // mReqs.mUser = this;
    // mLuaUserData.setClose(EHT_FILE, HttpEvtLua::funcOnClose, this);
}

HttpEvtLua::~HttpEvtLua() {
}

s32 HttpEvtLua::onRespWrite(net::HttpMsg* msg) {
    if (mLuaThread.mSubVM && EE_RETRY == mLuaThread.mStatus) {
        script::ScriptManager::resumeThread(mLuaThread);
    }
    return EE_OK;
}
s32 HttpEvtLua::onReadError(net::HttpMsg* msg) {
    return EE_OK;
}
s32 HttpEvtLua::onRespWriteError(net::HttpMsg* msg) {
    return EE_OK;
}


s32 HttpEvtLua::onReqHeadDone(net::HttpMsg* msg) {
    net::Website* site = msg->getHttpLayer()->getWebsite();
    if (!site || mLuaThread.mSubVM || msg->getRealPath().size() <= site->getConfig().mRootPath.size()) {
        return EE_ERROR;
    }
    mEvtFlags = EHF_OPEN;
    mReaded = 0;
    script::ScriptManager& eng = script::ScriptManager::getInstance();
    mLuaThread.mSubVM = eng.createThread();
    mWebRootPath = site->getConfig().mRootPath;
    String fname = msg->getRealPath().subString(site->getConfig().mRootPath.size());
    // TODO: don't reload script file
    if (!mLuaThread.mSubVM || !mScript.load(mLuaThread.mSubVM, site->getConfig().mRootPath, fname, true, false)) {
        eng.deleteThread(mLuaThread.mSubVM);
        mEvtFlags = EHF_CLOSE;
        return EE_ERROR;
    }
    mMsgResp = new net::HttpMsg(msg->getHttpLayer());
    creatCurrContext();
    msg->grab();
    grab();
    mMsg = msg;
    return EE_OK;
}

s32 HttpEvtLua::onLayerClose(net::HttpMsg* msg) {
    mEvtFlags |= EHF_CLOSE;
    return EE_OK;
}

s32 HttpEvtLua::onReqBodyDone(net::HttpMsg* msg) {
    script::ScriptManager& eng = script::ScriptManager::getInstance();
    eng.resumeThread(mLuaThread);
    if (EE_OK == mLuaThread.mStatus) {
        eng.deleteThread(mLuaThread.mSubVM);
        mLuaThread.mStatus = EE_CLOSING;
        mEvtFlags = EHF_CLOSING;
        return EE_OK;
    }
    if (EE_RETRY == mLuaThread.mStatus) {
        return EE_OK;
    }
    mEvtFlags = EHF_CLOSE;
    mMsg->drop();
    mMsg = nullptr;
    return EE_ERROR;


    return onReqBody(msg);
}

void HttpEvtLua::onClose(Handle* it) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mEvtFlags) {
        if (1 == getRefCount()) {
            script::ScriptManager::getInstance().deleteThread(mLuaThread.mSubVM);
        }
        drop();
    }
}

void HttpEvtLua::onRead(RequestFD* it) {
    // TODO
    if (it->mError) {
        Logger::log(ELL_ERROR, "HttpEvtLua::onRead>>err file=%p", 0);
        return;
    }
}

s32 HttpEvtLua::onReqBody(net::HttpMsg* msg) {
    return EE_OK;
    return launchRead();
}


s32 HttpEvtLua::launchRead() {
    if (mReqs.mUser) {
        return EE_RETRY;
    }
    return EE_ERROR;
}


void HttpEvtLua::creatCurrContext() {
    script::ScriptManager::setENV(mLuaThread.mSubVM, false); // @note don't pop context_table

    script::LuaHttpEventNew(mLuaThread.mSubVM, this);
    lua_setfield(mLuaThread.mSubVM, -2, "mCTX");

    script::Script::pushTable(mLuaThread.mSubVM, "mWebPath", getWebRootPath().data());

    script::AppTableDisableModif(mLuaThread.mSubVM, -1);

    lua_pop(mLuaThread.mSubVM, 1); // @note pop context_table here
    // script::LuaDumpStack(mLuaThread.mSubVM);
}



s32 HttpEvtLua::onReqChunkHeadDone(net::HttpMsg* msg) {
    return EE_OK;
}

s32 HttpEvtLua::onReqChunkBodyDone(net::HttpMsg* msg) {
    return EE_OK;
}



//-----------------------------------------------------
s32 HttpEvtLua::writeRespLine(s32 num, const s8* brief, s64 bodyLen) {
    if (!mMsgResp || !brief) {
        return EE_NO_OPEN;
    }
    mMsgResp->setStatus(static_cast<u16>(num), brief);
    mRespStep = RSTEP_INIT;
    if (bodyLen < 0) {
        mRespStep = RSTEP_STEP_CHUNK;
        mMsgResp->getHead().setChunked();
    } else {
        mMsgResp->getHead().setLength(bodyLen);
    }
    return EE_OK;
}

s32 HttpEvtLua::writeRespHeader(const s8* name, const s8* val) {
    if (!mMsgResp || !name || !val) {
        return EE_NO_OPEN;
    }
    StringView wa(name, strlen(name)), wb(val, strlen(val));
    mMsgResp->getHead().add(wa, wb);
    return EE_OK;
}

s32 HttpEvtLua::writeRespBody(const s8* buf, usz len) {
    if (!mMsgResp || !buf) {
        return EE_NO_OPEN;
    }
    if (0 == (RSTEP_HEAD_END & mRespStep)) {
        mRespStep |= RSTEP_HEAD_END;
    }
    if (RSTEP_STEP_CHUNK & mRespStep) {
        mMsgResp->writeChunkLen(len);
    }
    mMsgResp->writeBody(buf, len);
    if (RSTEP_STEP_CHUNK & mRespStep) {
        mMsg->writeBody("\r\n", 2);
    }
    return EE_OK;
}

s32 HttpEvtLua::sendResp(u32 step) {
    if (!mMsgResp || RSTEP_INIT == (RSTEP_MASK & step)) {
        return EE_NO_OPEN;
    }
    if (0 == (RSTEP_HEAD_END & mRespStep) && (RSTEP_HEAD_END & step)) {
        mRespStep |= step;
    }
    if ((RSTEP_BODY_END & step) && (RSTEP_STEP_CHUNK & mRespStep)) {
        mMsgResp->writeLastChunk();
    }
    // DLOG(ELL_INFO, "sendResp: step= %d, post send = %d", step, ret);
    return mMsgResp->getHttpLayer()->sendOut(mMsgResp);
}


} // namespace app
