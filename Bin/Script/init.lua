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
