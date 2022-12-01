-- 文件名为 ScriptInfo.lua
-- 定义一个名为 ScriptInfo 的模块
ScriptInfo = {}
 
-- 定义一个常量
ScriptInfo.constant = "这是一个常量"
 
-- 定义一个函数
function ScriptInfo.init()
    io.write("这是一个公有函数！\n")
end
 
local function func2()
    print("这是一个私有函数！")
end
 
function ScriptInfo.func()
    func2()
end
 
return ScriptInfo
