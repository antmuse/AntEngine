#pragma once

#include <sstream>

namespace app {
namespace db {

/**

* @brief 获取线程本地流
*
* 注意：使用此功能的地方不得相互调用
* 如果这个流曾经在另一个方法中使用过，它一定不能递归地
* 调用自身或使用此流的另一个方法，否则它们会相互践踏。
*
*@return 当前线程唯一的stringstream的引用
*/
std::stringstream& getThreadLocalStream();

} //namespace db
} //namespace app
