-- �ļ���Ϊ ScriptInfo.lua
-- ����һ����Ϊ ScriptInfo ��ģ��
ScriptInfo = {}
 
-- ����һ������
ScriptInfo.constant = "����һ������"
 
-- ����һ������
function ScriptInfo.init()
    io.write("����һ�����к�����\n")
end
 
local function func2()
    print("����һ��˽�к�����")
end
 
function ScriptInfo.func()
    func2()
end
 
return ScriptInfo
