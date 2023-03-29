#include "Net/HTTP/HttpEvtLua.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Net/HTTP/MsgStation.h"
#include "Script/LuaFunc.h"

// TODO

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtLua::HttpEvtLua(const StringView& file) :
    mReaded(0), mRefVM(LUA_NOREF), mFileName(file), mEvtFlags(0), mLuaUserData(nullptr), mSubVM(nullptr), mMsg(nullptr),
    mBody(nullptr) {

    mReqs.mCall = HttpEvtLua::funcOnRead;
    // mReqs.mUser = this;
    // mLuaUserData.setClose(EHT_FILE, HttpEvtLua::funcOnClose, this);
}

HttpEvtLua::~HttpEvtLua() {
}

s32 HttpEvtLua::onSent(net::HttpMsg& msg) {
    // go on to send body here
    script::ScriptManager& eng = script::ScriptManager::getInstance();
    if (!eng.getThread(mRefVM)) {
        return EE_ERROR;
    }

    const s32 subcnt = lua_gettop(mSubVM);
    s32 ret = lua_getglobal(mSubVM, "OnSent");
    s32 nret;
    ret = mScript.resumeThread(mSubVM, 0, nret);
    return EE_OK;
}

s32 HttpEvtLua::onOpen(net::HttpMsg& msg) {
    mBody = &msg.getCacheOut();
    net::Website* site = msg.getHttpLayer()->getWebsite();
    if (!site || mSubVM) {
        return EE_ERROR;
    }
    mEvtFlags = EHF_OPEN;
    mReaded = 0;
    script::ScriptManager& eng = script::ScriptManager::getInstance();
    s32 subcnt = lua_gettop(eng.getRootVM());
    mSubVM = eng.createThread(mRefVM);
    subcnt = lua_gettop(eng.getRootVM());
    if (!mSubVM || !mScript.load(mSubVM, eng.getScriptPath(), mFileName, false, true)) {
        eng.deleteThread(mSubVM, mRefVM);
        return EE_ERROR;
    }
    subcnt = lua_gettop(eng.getRootVM());
    if (!mScript.exec(mSubVM)) {
        eng.deleteThread(mSubVM, mRefVM);
        return EE_ERROR;
    }
    if (!mScript.getLoaded(mSubVM, mFileName)) {
        lua_pop(mSubVM, 1);
        return EE_ERROR;
    }
    subcnt = lua_gettop(eng.getRootVM());
    subcnt = lua_gettop(mSubVM);
    creatCurrContext();
    subcnt = lua_gettop(mSubVM);

    s32 ret = lua_getglobal(mSubVM, "OnOpen");
    s32 nret;
    ret = mScript.resumeThread(mSubVM, 0, nret);
    if (EE_OK == ret) {
        lua_settop(mSubVM, -1);
        eng.deleteThread(mSubVM, mRefVM);
        return EE_OK;
    }
    if (EE_POSTED == ret) {
        msg.grab();
        grab();
        mMsg = &msg;
        return EE_OK;
    }
    return EE_ERROR;
}

s32 HttpEvtLua::onClose() {
    mEvtFlags |= EHF_CLOSE;
    if (!mLuaUserData) {
    }
    return EE_OK;
}

s32 HttpEvtLua::onFinish(net::HttpMsg& msg) {
    return onBodyPart(msg);
}

void HttpEvtLua::onClose(Handle* it) {
    if (mMsg) {
        mMsg->drop();
        mMsg = nullptr;
    }
    if (mEvtFlags) {
        if (1 == getRefCount()) {
            script::ScriptManager::getInstance().deleteThread(mSubVM, mRefVM);
            mLuaUserData = nullptr;
        }
        drop();
    }
}

void HttpEvtLua::onRead(RequestFD* it) {
    if (it->mError) {
        mEvtFlags = true;
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
        Logger::log(ELL_ERROR, "HttpEvtLua::onRead>>err file=%p", mLuaUserData);
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
            mMsg->setStationID(net::ES_RESP_BODY_DONE);
        }
    } else {
        mEvtFlags = true;
        mBody->write("0\r\n\r\n", 5);
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
    }

    net::Website* site = mMsg->getHttpLayer()->getWebsite();
    if (site) {
        if (EE_OK != site->stepMsg(mMsg)) {
            mEvtFlags = true;
        }
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

s32 HttpEvtLua::onBodyPart(net::HttpMsg& msg) {
    if (!mLuaUserData) {
        return EE_ERROR;
    }
    return launchRead();
}


s32 HttpEvtLua::launchRead() {
    if (mReqs.mUser) {
        return EE_RETRY;
    }
    if (0 == mReqs.mUsed) {
        mChunkPos = mBody->getTail();
        mReqs.mAllocated = mBody->peekTailNode(G_BLOCK_HEAD_SIZE, &mReqs.mData, 4 * 1024);
        mReqs.mUser = this;
        // if (EE_OK != mLuaUserData.read(&mReqs, mReaded)) {
        //     mReqs.mUser = nullptr;
        //     return mLuaUserData.launchClose();
        // }
        return EE_OK;
    }
    return EE_ERROR;
}

void HttpEvtLua::creatCurrContext() {
    script::Script::createGlobalTable(mSubVM, 0, 0);
    script::Script::pushTable(mSubVM, "curr_ctx", sizeof("curr_ctx"), this);
    script::Script::pushTable(mSubVM, "curr_name", sizeof("curr_name"), "curr_vname", sizeof("curr_vname"));
    script::Script::popParam(mSubVM, 1); //pop table
}

void HttpEvtLua::pushCurrParam() {
    lua_pushlstring(mSubVM, "curr_front_flag", sizeof("curr_front_flag") - 1);
    lua_pushinteger(mSubVM, mEvtFlags);
    lua_rawset(mSubVM, -2);
}

} // namespace app
