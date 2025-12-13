-- define: ShowInfo
ShowInfo = {}

ShowInfo.mVersion = 100000

function ShowInfo.showTable(name, tab)
    Log("------------------------"..name)
    local idx = 0;
    for n in pairs(tab) do
        idx=idx+1
        Log(name.."["..idx.."], "..n)
    end
    Log("------------------------"..name)
end


-- local function pshow()
--     print("this is a private function\n")
-- end

function ShowInfo.showENV()
    ShowInfo.showTable("ENV", _ENV)
end

return ShowInfo
