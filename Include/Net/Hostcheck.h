#ifndef APP_HOSTCHECK_H
#define APP_HOSTCHECK_H

namespace app {

/**
* @return true if host match, else false.
*/
bool AppCheckHost(const char *match_pattern, const char *hostname);

} //namespace app

#endif //APP_HOSTCHECK_H
