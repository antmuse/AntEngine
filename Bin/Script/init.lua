-- This file called once when Engine init.

-- require "Eng"           --loaded by default
require "Public/ShowInfo"  --local ShowInfo = require("Public/ShowInfo")

function main()
    Log(">>>>>>>>>>>>>>>>>>>>>> init script manager start")
    Eng.showInfo()
    ShowInfo.showENV()
    Log("<<<<<<<<<<<<<<<<<<<<<< init script manager finish")
end
