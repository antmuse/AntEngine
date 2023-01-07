-- This file called once when Engine init.
require "Public/public"
require "Public/ScriptInfo"

G_VLAUE = 333;

local function test(val)
    Log("lua: func test val="..val);
    print("test G_VLAUE = "..G_VLAUE)
    print("test val = "..val)
end

function main()
    Log("lua: func main, path = "..GPath);
    print("main val = "..G_VLAUE)
    print("main GPath = "..GPath)
    test("ok100")
    test(110)
    show()
    ScriptInfo.init()
end



local filend = HandleFile.new()
filend:open("Script/init.lua_null", 1)
 
print(filend)
print("file= " .. filend:getFileName())
--class::__tostring()
 
--let it been gc
filend = nil
 
--force gc
--collectgarbage("collect")
