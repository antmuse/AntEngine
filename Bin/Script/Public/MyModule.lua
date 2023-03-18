-- file:MyModule.lua
-- define: MyModule
MyModule = {}

-- 定义一个常量
MyModule.CONST_VAL= "a const val"

function MyModule.show()
    io.write("this is a public function\n")
end

local function pshow()
    print("this is a private function\n")
end

function MyModule.showp()
    pshow()
end

return MyModule
