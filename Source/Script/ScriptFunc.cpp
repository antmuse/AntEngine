#include "Script/ScriptFunc.h"
#include "Logger.h"

namespace app {
namespace script {

/**
 * @brief log a msg
 * @param 1 opt: level
 * @param 2:     msg
 * @return 0
 */
int LuaLog(lua_State* iState) {
    u32 cnt = lua_gettop(iState);
    if (1 == cnt) {
        Logger::logInfo("Script> %s", lua_tostring(iState, 1));
    } else if (2 == cnt) {
        cnt = static_cast<u32>(lua_tointeger(iState, 1));
        Logger::log((ELogLevel)(cnt < ELL_COUNT ? cnt : ELL_INFO),
            "Script> %s", lua_tostring(iState, 2));
    } else {
        Logger::logError("Script> invalid log param, cnt=%d", cnt);
    }
    return 0;
}


/**
 * @brief  构造函数: 颜色
 * @param 1 红
 * @param 2 绿
 * @param 3 蓝
 * @param 4 透明度
 * @return 颜色
 */
int LuaColor(lua_State* iState) {
    u32 col = 0xFF & lua_tointeger(iState, 1);
    col |= (0xFF & lua_tointeger(iState, 2)) << 8;
    col |= (0xFF & lua_tointeger(iState, 2)) << 16;
    col |= (0xFF & lua_tointeger(iState, 2)) << 24;

    void* ret = lua_newuserdata(iState, sizeof(col));
    // memcpy(ret, &col, sizeof(col));
    *reinterpret_cast<u32*>(ret) = col;
    return 1;
}

}//namespace script
}//namespace app
