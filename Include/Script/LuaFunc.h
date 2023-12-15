/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#ifndef APP_SCRIPTFUNC_H
#define	APP_SCRIPTFUNC_H

#include "Config.h"
#include "Script/HLua.h"

namespace app {
namespace script {

/**
 * @brief log a msg
 * @param 1 opt: level
 * @param 2:     msg
 * @return 0
 */
extern int LuaLog(lua_State* iState);

//callback function for err
extern int LuaErrorFunc(lua_State* vm);

extern int LuaPanic(lua_State* vm);

extern void LuaDumpStack(lua_State* vm);

/**
 * @brief  构造函数: 颜色
 * @param 1 红
 * @param 2 绿
 * @param 3 蓝
 * @param 4 透明度
 * @return 颜色
 */
extern int LuaColor(lua_State* iState);


extern int LuaRandom(lua_State* iState);

extern int LuaEngInfo(lua_State* iState);
extern int LuaEngExit(lua_State* iState);

extern int LuaOpenEngLib(lua_State* iState);

extern int LuaInclude(lua_State* iState);

extern void LuaRegistClass(lua_State* vm, const luaL_Reg* func, const usz funcsz,
    const s8* className, const s8* namespac = nullptr);

}//namespace script
}//namespace app
#endif	// APP_SCRIPTFUNC_H
