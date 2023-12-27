#include "Net/HTTP/HttpEvtLua.h"
#include "RingBuffer.h"
#include "Net/HTTP/Website.h"
#include "Net/HTTP/MsgStation.h"
#include "Script/LuaFunc.h"

// TODO

namespace app {

static const s32 G_BLOCK_HEAD_SIZE = 6;

HttpEvtLua::HttpEvtLua(const StringView& file) :
    mReaded(0), mFileName(file), mEvtFlags(0), mMsg(nullptr), mBody(nullptr) {

    mReqs.mCall = HttpEvtLua::funcOnRead;
    // mReqs.mUser = this;
    // mLuaUserData.setClose(EHT_FILE, HttpEvtLua::funcOnClose, this);
}

HttpEvtLua::~HttpEvtLua() {
}

s32 HttpEvtLua::onSent(net::HttpMsg& msg) {
    script::ScriptManager::resumeThread(mLuaThread);
    return EE_OK;
}

s32 HttpEvtLua::onOpen(net::HttpMsg& msg) {
    mBody = &msg.getCacheOut();
    net::Website* site = msg.getHttpLayer()->getWebsite();
    if (!site || mLuaThread.mSubVM) {
        return EE_ERROR;
    }
    mEvtFlags = EHF_OPEN;
    mReaded = 0;
    script::ScriptManager& eng = script::ScriptManager::getInstance();

    mLuaThread.mSubVM = eng.createThread();
    if (!mLuaThread.mSubVM || !mScript.load(mLuaThread.mSubVM, site->getConfig().mRootPath, mFileName, false, false)) {
        eng.deleteThread(mLuaThread.mSubVM);
        mEvtFlags = EHF_CLOSE;
        return EE_ERROR;
    }
    creatCurrContext();
    eng.resumeThread(mLuaThread);
    if (EE_OK == mLuaThread.mStatus) {
        eng.deleteThread(mLuaThread.mSubVM);
        mLuaThread.mStatus = EE_CLOSING;
        mEvtFlags = EHF_CLOSING;
        return EE_OK;
    }
    if (EE_RETRY == mLuaThread.mStatus) {
        msg.grab();
        grab();
        mMsg = &msg;
        return EE_OK;
    }
    mEvtFlags = EHF_CLOSE;
    return EE_ERROR;
}

s32 HttpEvtLua::onClose() {
    mEvtFlags |= EHF_CLOSE;
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
            script::ScriptManager::getInstance().deleteThread(mLuaThread.mSubVM);
        }
        drop();
    }
}

void HttpEvtLua::onRead(RequestFD* it) {
    if (it->mError) {
        mEvtFlags = true;
        mMsg->setStationID(net::ES_RESP_BODY_DONE);
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
    if (!0) {
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
    // set context for coroutine
    script::ScriptManager::setENV(mLuaThread.mSubVM, false); // don't pop context_table
    lua_pushlightuserdata(mLuaThread.mSubVM, mLuaThread.mSubVM);
    lua_setfield(mLuaThread.mSubVM, -2, "vVM");
    lua_pushstring(mLuaThread.mSubVM, "This is the global ctx table");
    lua_setfield(mLuaThread.mSubVM, -2, "vBrief");
    script::Script::pushTable(mLuaThread.mSubVM, "vName", "HttpEvtLua");
    lua_pop(mLuaThread.mSubVM, 1); // pop context_table
    // script::LuaDumpStack(mLuaThread.mSubVM);
}


} // namespace app
