require "Public/ShowInfo"


local function main()
    -- ShowInfo.showTable("Web.WebDev", WebDev)
    local ctx = VContext.mCTX
    local fpath = "Web/fs/"
    local body  = [[<UL class=shop> PATH = ]] .. fpath .. [[<br><hr><br>
        <DIV align=left><TABLE border=0 cellPadding=8 cellSpacing=4 width="80%">
    ]];
    local bodylen = -1  -- use chunk mode if -1,  else = string.len(body)  -- utf8.len(body)
    ctx:writeLine(200, "OK", bodylen)
    ctx:writeHead("Connection", "close")
    ctx:writeHead("Content-Type", "text/html; charset=utf-8")
    ctx:writeBody(Web.WebDev.mPublicHead)
    local dirList = Eng.getPathNodes(fpath, 4); -- 4 = strlen("Web/")
    if(nil ~= dirList) then
        Log("total count = " .. dirList.mNodes .. ", paths count = " .. dirList.mPaths)
        for i=1,dirList.mNodes,1 do
            if(i>dirList.mPaths) then
                body = body .. "<tr><td>*</td><td><a href=\"../" .. dirList[i] .. "\">" .. dirList[i] .. "</a></td></tr>"
            else
                body = body .. "<tr><td>+</td><td><a href=\"../" .. dirList[i] .. "\">" .. dirList[i] .. "</a></td></tr>"
            end
        end
        ctx:writeBody(body)
    else
        Log("ListPath.lua> empty path: " .. fpath)
    end

    local ret = ctx:sendResp(HTTP_BODY_PART)
    if(0 ~= ret) then
        Log("ListPath.lua> head sendResp fail = " .. ret)
        return;
    end
    ctx:writeBody("</TABLE></DIV></UL>"  ..  Web.WebDev.mPublicTail)
    ret = ctx:sendResp(HTTP_BODY_END)
    if(0 ~= ret) then
        Log("ListPath.lua> body sendResp fail = " .. ret)
    end
end

main()
