#include "daScript/misc/platform.h"
#include "daScript/misc/sysos.h"

#include "daScript/ast/ast.h"

namespace das {

DynamicModuleInfo::~DynamicModuleInfo() {
    for (auto handler : dll_handlers) {
        closeLibrary(handler);
    }
}

daScriptEnvironment *daScriptEnvironment::getBound() { return *(daScriptEnvironment::bound); }
daScriptEnvironment *daScriptEnvironment::getOwned() { return *(daScriptEnvironment::owned); }
void daScriptEnvironment::setBound(daScriptEnvironment *bnd) { *daScriptEnvironment::bound = bnd; }
void daScriptEnvironment::setOwned(daScriptEnvironment *own) { *daScriptEnvironment::owned = own; }
daScriptEnvironment *daScriptEnvironment::exchangeBound(daScriptEnvironment *bnd) {
    auto prev = *daScriptEnvironment::bound;
    *daScriptEnvironment::bound = bnd;
    return prev;
}
daScriptEnvironment *daScriptEnvironment::exchangeOwned(daScriptEnvironment *own) {
    auto prev = *daScriptEnvironment::owned;
    *daScriptEnvironment::owned = own;
    return prev;
}

daScriptEnvironmentGuard::daScriptEnvironmentGuard(das::daScriptEnvironment *bound, das::daScriptEnvironment *owned) {
    initialBound = das::daScriptEnvironment::exchangeBound(bound);
    initialOwned = das::daScriptEnvironment::exchangeOwned(owned);
}

daScriptEnvironmentGuard::~daScriptEnvironmentGuard() {
    das::daScriptEnvironment::setBound(initialBound);
    das::daScriptEnvironment::setOwned(initialOwned);
}

void daScriptEnvironment::ensure() {
    if ( !*daScriptEnvironment::bound ) {
        if ( !*daScriptEnvironment::owned ) {
            *daScriptEnvironment::owned = new daScriptEnvironment();
        }
        *daScriptEnvironment::bound = *daScriptEnvironment::owned;
    }
}

uint64_t getCancelLimit() {
    if ( !daScriptEnvironment::getBound() ) return 0;
    return daScriptEnvironment::getBound()->dataWalkerStringLimit;
}

}