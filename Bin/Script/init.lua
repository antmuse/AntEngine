-- This file called once when Engine init.
require "Eng"
local MyMod = require("Public/MyModule")

local function showInfo(val)
    Eng.showInfo()
    Log("init.lua, param= "..val);
    Log("init.lua, GLibPath = "..GPath);
    MyMod.show()
end

function main()
    showInfo("show")
end

Log("init.lua, you can call this main() after exec this script")
