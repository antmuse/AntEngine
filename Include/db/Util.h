#pragma once

#include <sstream>

namespace app {
namespace db {

/**

* @brief ��ȡ�̱߳�����
*
* ע�⣺ʹ�ô˹��ܵĵط������໥����
* ����������������һ��������ʹ�ù�����һ�����ܵݹ��
* ���������ʹ�ô�������һ���������������ǻ��໥��̤��
*
*@return ��ǰ�߳�Ψһ��stringstream������
*/
std::stringstream& getThreadLocalStream();

} //namespace db
} //namespace app
