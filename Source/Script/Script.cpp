#include "Script/Script.h"
#include <atomic>
#include "Script/HLua.h"
#include "Logger.h"
#include "Engine.h"
#include "FileReader.h"
#include "Script/ScriptManager.h"

namespace app {
namespace script {

Script::Script(const String& iName, const String& iPath) :
    mBuffer(nullptr),
    mBufferSize(0),
    mName(iName),
    mPath(iPath) {
    static std::atomic<u32> mIDCount(0);
    mID = mIDCount++;
}

Script::~Script() {
    unload();
}

bool Script::load(bool reload) {
    if (!reload && mBuffer) {
        return false;
    }
    unload();
    FileReader file;
    if (!file.openFile(mPath)) {
        Logger::logError("Script::load, open fail, script = %s", mName.c_str());
        return false;
    }
    mBufferSize = file.getFileSize();
    mBuffer = new s8[mBufferSize + 1];
    if (mBufferSize != file.read(mBuffer, mBufferSize)) {
        Logger::logError("Script::load, read fail, script = %s", mName.c_str());
        unload();
        // file.close();
        return false;
    }
    Logger::logInfo("Script::load, script[%llu] = %s", mBufferSize, mName.c_str());
    mBuffer[mBufferSize] = '\0';
    return true;
}

void Script::unload() {
    if (mBuffer) {
        delete[] mBuffer;
        mBufferSize = 0;
        mBuffer = nullptr;
    }
}

}//namespace script
}//namespace app
