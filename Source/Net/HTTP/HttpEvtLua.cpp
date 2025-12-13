#include "Net/HTTP/HttpEvtLua.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Script/LuaFunc.h"


namespace app {
namespace script {
extern s32 LuaHttpEventNew(lua_State* vm, HttpEvtLua* evt);
}


static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtLua::HttpEvtLua() : mReaded(0), mEvtFlags(0), mMsg(nullptr), mBody(nullptr) {
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
    mBody = &msg->getCacheOut();
    mLuaThread.mSubVM = eng.createThread();
    mWebRootPath = site->getConfig().mRootPath;
    String fname = msg->getRealPath().subString(site->getConfig().mRootPath.size());
    // TODO: don't reload script file
    if (!mLuaThread.mSubVM || !mScript.load(mLuaThread.mSubVM, site->getConfig().mRootPath, fname, true, false)) {
        eng.deleteThread(mLuaThread.mSubVM);
        mEvtFlags = EHF_CLOSE;
        return EE_ERROR;
    }
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
    if (it->mError) {
        mEvtFlags = true;
        Logger::log(ELL_ERROR, "HttpEvtLua::onRead>>err file=%p", 0);
        return;
    }
    if (it->mUsed > 0) {
        s8 chunked[8];
        snprintf(chunked, sizeof(chunked), "%04x\r\n", it->mUsed);
        mBody->rewrite(mChunkPos, chunked, G_BLOCK_HEAD_SIZE);
        mBody->commitTailPos(it->mUsed);
        mBody->write("\r\n", 2);
        if (it->mUsed < it->mAllocated) {
            mEvtFlags = true;
            mBody->write("0\r\n\r\n", 5);
        }
    } else {
        mEvtFlags = true;
        mBody->write("0\r\n\r\n", 5);
    }

    net::Website* site = mMsg->getHttpLayer()->getWebsite();
    if (site) {
        // if (EE_OK != site->stepMsg(mMsg)) {
        //     mEvtFlags = true;
        // }
    } else {
        mEvtFlags = true;
    }

    mReaded += it->mUsed;
    it->mUsed = 0;
    it->mUser = nullptr;
    if (mEvtFlags) {
        // mLuaUserData.launchClose();
    } else {
        launchRead();
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

    // lua_pushlightuserdata(mLuaThread.mSubVM, mLuaThread.mSubVM);
    // lua_setfield(mLuaThread.mSubVM, -2, "mVM");
    // lua_pushstring(mLuaThread.mSubVM, "This is the ctx table");
    // lua_setfield(mLuaThread.mSubVM, -2, "mBrief");
    // script::Script::pushTable(mLuaThread.mSubVM, "mCTXID", this);

    script::LuaHttpEventNew(mLuaThread.mSubVM, this);
    lua_setfield(mLuaThread.mSubVM, -2, "mCTX");

    script::Script::pushTable(mLuaThread.mSubVM, "mWebPath", getWebRootPath().data());

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
    if (!mMsg || !brief) {
        return EE_NO_OPEN;
    }
    mMsg->setStatus(static_cast<u16>(num), brief);
    mRespStep = RSTEP_INIT;
    if (bodyLen < 0) {
        mRespStep = RSTEP_STEP_CHUNK;
        mMsg->getHeadOut().setChunked();
    } else {
        mMsg->getHeadOut().setLength(bodyLen);
    }
    return EE_OK;
}

s32 HttpEvtLua::writeRespHeader(const s8* name, const s8* val) {
    if (!mMsg || !name || !val) {
        return EE_NO_OPEN;
    }
    StringView wa(name, strlen(name)), wb(val, strlen(val));
    mMsg->getHeadOut().add(wa, wb);
    return EE_OK;
}

s32 HttpEvtLua::writeRespBody(const s8* buf, usz len) {
    if (!mMsg || !buf) {
        return EE_NO_OPEN;
    }
    if (0 == (RSTEP_HEAD_END & mRespStep)) {
        mRespStep |= RSTEP_HEAD_END;
        mMsg->buildResp();
    }
    if (RSTEP_STEP_CHUNK & mRespStep) {
        mMsg->writeOutChunkLen(len);
    }
    mMsg->writeOutBody(buf, len);
    if (RSTEP_STEP_CHUNK & mRespStep) {
        mMsg->writeOutBody("\r\n", 2);
    }
    return EE_OK;
}

s32 HttpEvtLua::sendResp(u32 step) {
    if (!mMsg || RSTEP_INIT == (RSTEP_MASK & step)) {
        return EE_NO_OPEN;
    }
    if (0 == (RSTEP_HEAD_END & mRespStep) && (RSTEP_HEAD_END & step)) {
        mRespStep |= step;
        mMsg->buildResp();
    }
    if ((RSTEP_BODY_END & step) && (RSTEP_STEP_CHUNK & mRespStep)) {
        mMsg->writeOutLastChunk();
    }
    // DLOG(ELL_INFO, "sendResp: step= %d, post send = %d", step, ret);
    return mMsg->getHttpLayer()->sendResp(mMsg);
}


} // namespace app
