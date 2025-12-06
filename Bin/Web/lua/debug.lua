--for debug

VGlobalVal = "my glob val"
local vLocalVal = "my local val"

local function showTable(name, tab)
    Log("------------------------"..name)
    local idx = 0;
    for n in pairs(tab) do
        idx=idx+1
        print(name.."["..idx.."], "..n)
    end
    Log("------------------------"..name)
end


local function test(yieldcnt)
    Eng.showInfo()
    showTable("G", _G)
    Log("VGlobalVal="..VGlobalVal)
    Log("vLocalVal="..vLocalVal)
    Log("ENV.VGlobalVal=".._ENV.VGlobalVal)
    Log("G.VGlobalVal=".._G.VGlobalVal)
    for i=1,yieldcnt,1 do
        Log("yield, idx = "..i)
        coroutine.yield()
    end
    local col = Color(11,22,33,44)
    local col2 = Color()
    --col = core.Color(11,22,33,44)  --with namespace
    --col2 = core.Color()
    col:setRed(55);
    bval = col:getBlue();
    Log("col = "..col:toStr()..", blue = "..bval)
    Log("col2 = "..col2:toStr()..", red = "..col2:getRed())
    Log("VContext.vBrief = "..VContext.vBrief)
    Log("Test finish, yield cnt = "..yieldcnt)
end



local function main()
    -- showTable("ENV", _ENV)
    -- showTable("VContext", VContext)
    local body  = "<htm><head><title>debug.lua</title></head><body style=\"font-size: 24px;\">This is a test file for lua debug</body></html>";
    local bodylen = string.len(body)  -- utf8.len(body)
    Log("debug.lua> body len = "..bodylen..", type(bodylen) = "..type(bodylen))
    --bodylen = -1  -- use chunk mode
    VContext.mCTX:writeLine(200, "OK", bodylen)
    -- VContext.mCTX:writeHead("Content-Length", tostring(bodylen))
    VContext.mCTX:writeHead("Set-Cookie", "loc=debug.lua")
    VContext.mCTX:writeHead("Connection", "close")
    VContext.mCTX:writeHead("Content-Type", "text/html; charset=utf-8")
    local ret = VContext.mCTX:sendResp(1)
    if(ret ~= nil and 0 ~= ret) then
        Log("debug.lua> head sendResp fail = "..ret)
        return;
    end
    VContext.mCTX:writeBody(body)
    ret = VContext.mCTX:sendResp(7)
    if(ret ~= nil and 0 ~= ret) then
        Log("debug.lua> body sendResp fail = "..ret)
    end
end

main()
