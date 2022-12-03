-- module: ScriptInfo
ScriptInfo = {}
 
-- a const value
ScriptInfo.mConstVal = "a const val"
 
function ScriptInfo.init()
    print("this is a public function, const="..ScriptInfo.mConstVal)
end
 
local function func2()
    print("this is a private function")
end
 
function ScriptInfo.func()
    func2()
end
 
return ScriptInfo
