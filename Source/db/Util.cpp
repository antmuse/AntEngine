#include "db/Util.h"

namespace app {
namespace db {

std::stringstream& getThreadLocalStream() {
    thread_local std::stringstream strm;

    strm.clear();
    strm.str("");

    return strm;
}

} //namespace db
} //namespace app
