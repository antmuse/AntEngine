#include "db/ConnectorHolder.h"
#include "db/ConnectorPool.h"

namespace app {
namespace db {

ConnectorHolder::ConnectorHolder(Connector* query)
    : mConnector(query) {
}

ConnectorHolder::~ConnectorHolder() {
    if (mConnector) {
        mConnector->mPool.push(mConnector);
        mConnector = nullptr;
    }
}

ConnectorHolder::ConnectorHolder(ConnectorHolder&& from)noexcept
    : mConnector(std::move(from.mConnector)) {
}

ConnectorHolder& ConnectorHolder::operator=(ConnectorHolder&& from) noexcept {
    mConnector = std::move(from.mConnector);
    return *this;
}

Connector& ConnectorHolder::operator*() {
    return *mConnector;
}

const Connector& ConnectorHolder::operator*() const {
    return *mConnector;
}

Connector* ConnectorHolder::operator-> () {
    return mConnector;
}

const Connector* ConnectorHolder::operator-> () const {
    return mConnector;
}

} //namespace db
} //namespace app
