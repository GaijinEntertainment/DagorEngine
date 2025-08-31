#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"

namespace das {

void daScriptEnvironment::ensure() {
    if ( !*daScriptEnvironment::bound ) {
        if ( !*daScriptEnvironment::owned ) {
            *daScriptEnvironment::owned = new daScriptEnvironment();
        }
        *daScriptEnvironment::bound = *daScriptEnvironment::owned;
    }
}

uint64_t getCancelLimit() {
    if ( !*daScriptEnvironment::bound ) return 0;
    return (*daScriptEnvironment::bound)->dataWalkerStringLimit;
}

}