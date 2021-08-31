#include "db/Task.h"
#include "db/Util.h"

namespace app {
namespace db {

Task& Task::operator<<(Arg parameter) {
    mTaskPart.emplace_back(std::move(parameter.mValStr), parameter.mRequiresEscaping);
    return *this;
}

Task::TaskPart::TaskPart(std::string value, bool iEscape)
    : mValStr(std::move(value))
    , mRequiresEscaping(iEscape) {
}

} //namespace db
} //namespace app
