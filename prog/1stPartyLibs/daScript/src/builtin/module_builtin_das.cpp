#include "daScript/misc/platform.h"

#include "module_builtin.h"

namespace das {
    #include "builtin.das.inc"
    bool Module_BuiltIn::appendCompiledFunctions() {
        return compileBuiltinModule("builtin.das",builtin_das, sizeof(builtin_das));
    }
}
