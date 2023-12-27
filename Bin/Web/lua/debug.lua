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


local function main(yieldcnt)
    showTable("ENV", _ENV)
    showTable("G", _G)
    --showTable("core", core)      --namespace: core
    showTable("VContext", VContext)
    Log("VGlobalVal="..VGlobalVal)
    Log("vLocalVal="..vLocalVal)
    Log("ENV.VGlobalVal=".._ENV.VGlobalVal)
    Log("G.VGlobalVal=".._G.VGlobalVal)
    for i=1,yieldcnt,1 do
        Log("yield, idx = "..i)
        coroutine.yield()
    end
    Eng.showInfo()
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

--yield count = 3
main(0)
