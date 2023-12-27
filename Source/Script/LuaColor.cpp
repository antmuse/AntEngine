#include "Color.h"
#include "Script/LuaFunc.h"

namespace app {
namespace script {

static const s8* const GMETA_NAME_COLOR = "Color";
#if defined(DUSE_SCRIPT_NAMESPACE)
static const s8* const GMETA_NAMESPACE_COLOR = "core";
static const s8* const GMETA_FULL_NAME_COLOR = "core_Color"; // idx of reg_table
#else
static const s8* const GMETA_NAMESPACE_COLOR = nullptr;
static const s8* const GMETA_FULL_NAME_COLOR = GMETA_NAME_COLOR; // idx of reg_table
#endif

/**
 * @brief  构造函数: 颜色
 * @param none
 * --or--
 * @param 1 RGBA
 * --or--
 * @param 1 红
 * @param 2 绿
 * @param 3 蓝
 * @param 4 透明度
 * ------
 * @return 颜色
 */
int LuaColorNew(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    Color** cc = reinterpret_cast<Color**>(lua_newuserdata(vm, sizeof(void*)));
    *cc = new Color();
    if (1 == cnt) {
        u32 col = lua_tointeger(vm, 1);
        (*cc)->setColor(col);
    } else if (4 == cnt) {
        u32 col = 0xFFu & lua_tointeger(vm, 1);
        col |= (0xFFu & lua_tointeger(vm, 2)) << 8;
        col |= (0xFFu & lua_tointeger(vm, 3)) << 16;
        col |= (0xFFu & lua_tointeger(vm, 4)) << 24;
        (*cc)->setColor(col);
    }
    luaL_setmetatable(vm, GMETA_FULL_NAME_COLOR);
    printf("LuaColorNew: col=%p, col=%u\n", *cc, (*cc)->getColor());
    return 1;
}

int LuaColorDel(lua_State* vm) {
    // s32 cnt = lua_gettop(vm);
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        printf("LuaColorDel: col=%p, col=%u\n", *col, (*col)->getColor());
        delete *col;
        *col = nullptr;
    }
    return 0;
}

int LuaColor2Str(lua_State* vm) {
    // s32 cnt = lua_gettop(vm);
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        String str = (*col)->toStr();
        lua_pushlstring(vm, str.c_str(), str.getLen());
    } else {
        lua_pushnil(vm);
    }
    return 1;
}

int LuaColorGetR(lua_State* vm) {
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        lua_pushinteger(vm, (*col)->getRed());
    } else {
        lua_pushnil(vm);
    }
    return 1;
}

int LuaColorGetG(lua_State* vm) {
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        lua_pushinteger(vm, (*col)->getGreen());
    } else {
        lua_pushnil(vm);
    }
    return 1;
}

int LuaColorGetB(lua_State* vm) {
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        lua_pushinteger(vm, (*col)->getBlue());
    } else {
        lua_pushnil(vm);
    }
    return 1;
}

int LuaColorGetA(lua_State* vm) {
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        lua_pushinteger(vm, (*col)->getAlpha());
    } else {
        lua_pushnil(vm);
    }
    return 1;
}

int LuaColorSetR(lua_State* vm) {
    u32 val = static_cast<u32>(luaL_checkinteger(vm, -1));
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        (*col)->setRed(val);
    }
    return 0;
}

int LuaColorSetG(lua_State* vm) {
    u32 val = static_cast<u32>(luaL_checkinteger(vm, -1));
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        (*col)->setGreen(val);
    }
    return 0;
}

int LuaColorSetB(lua_State* vm) {
    u32 val = static_cast<u32>(luaL_checkinteger(vm, -1));
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        (*col)->setBlue(val);
    }
    return 0;
}

int LuaColorSetA(lua_State* vm) {
    u32 val = static_cast<u32>(luaL_checkinteger(vm, -1));
    Color** col = reinterpret_cast<Color**>(lua_touserdata(vm, 1));
    if (col && *col) {
        (*col)->setAlpha(val);
    }
    return 0;
}

luaL_Reg LuaClassColor[] = {
    {"new", LuaColorNew}, {"getRed", LuaColorGetR}, {"getGreen", LuaColorGetG}, {"getBlue", LuaColorGetB},
    {"getAlpha", LuaColorGetA}, {"setRed", LuaColorSetR}, {"setGreen", LuaColorSetG}, {"setBlue", LuaColorSetB},
    {"setAlpha", LuaColorSetA}, {"toStr", LuaColor2Str}, {"__gc", LuaColorDel}, {nullptr, nullptr} // sentinel
};


s32 LuaRegColor(lua_State* vm) {
    return LuaRegistClass(vm, LuaClassColor, DSIZEOF(LuaClassColor), GMETA_NAME_COLOR, GMETA_NAMESPACE_COLOR);
}

} // namespace script
} // namespace app
