--for debug
require "Public/ShowInfo"


local function main()
    ShowInfo.showTable("Web.WebDev", Web.WebDev)
    ShowInfo.showTable("VContext", VContext)
    local body  = "<htm><head><title>debug.lua</title></head><body style=\"font-size: 24px;\">This is a test file for lua debug</body></html>";
    local bodylen = string.len(body)  -- utf8.len(body)
    Log("debug.lua> body len = " .. bodylen .. ", type(bodylen) = " .. type(bodylen))
    --bodylen = -1  -- use chunk mode
    VContext.mCTX:writeLine(200, "OK", bodylen)  -- VContext.mCTX:writeHead("Content-Length", tostring(bodylen))
    VContext.mCTX:writeHead("Set-Cookie", "loc=debug.lua")
    VContext.mCTX:writeHead("Connection", "close")
    VContext.mCTX:writeHead("Content-Type", "text/html; charset=utf-8")
    local ret = VContext.mCTX:sendResp(HTTP_HEAD_END)
    if(0 ~= ret) then
        Log("debug.lua> head sendResp fail = " .. ret)
        return;
    end
    VContext.mCTX:writeBody(body)
    ret = VContext.mCTX:sendResp(HTTP_BODY_END)
    if(0 ~= ret) then
        Log("debug.lua> body sendResp fail = " .. ret)
    end


    Log("debug.lua> web.pub.head = " .. Web.WebDev.mPublicTail)
end

main()
