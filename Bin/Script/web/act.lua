-- This file called once when Engine init.
require "Eng"

local function showInfo(val)
    Log("web/act.lua, param= "..val)
    print("web/act.lua, global.start------------------------")
    --打印出_G表中的所有变量
    for n in pairs(_G["_G"]) do
        print(n)
    end
    print("web/act.lua, global.end------------------------")
    print("web/act.lua _G.curr_name=" .. _G["curr_name"])
end

function OnOpen()
    showInfo("show")
    -------------
    local buf = RequestFD(2000)
    local ftxt = HandleFile()
    local allc =  buf:getAllocated(buf)
    print("buf:allocated = " .. allc)
    local ret =  ftxt:open("Script/readme.txt", 1)
    print("file open = " , ret)
    print("file= " ..  ftxt:getFileName(ftxt))
    ret = ftxt:read(buf)
    coroutine.yield(0)
    if ret then
        while (0 == buf:getErr(buf)) do
            if (buf:getSize(buf)>0) then
                break
            end
            coroutine.yield(0)
        end
    end
    buf:close()
    ftxt:close()
end


function OnClose()
    print("web/act.lua loaded")
end

function OnSent()
    print("web/act.lua loaded" .. curr_name)
end

function OnRead()
    print("web/act.lua loaded")
end

function OnFinish()
    print("web/act.lua loaded")
end

function OnTimeout()
    print("web/act.lua loaded")
end


function OnMsgPart()
    print("web/act.lua loaded")
end

print("web/act.lua loaded")
