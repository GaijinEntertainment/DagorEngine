#include "daScript/ast/ast.h"
#include "daScript/daScriptModule.h"

static void register_builtin_modules_impl() {
    using das::Module;
    if (!Module::require("$")) {
        NEED_MODULE(Module_BuiltIn);
    }
    if (!Module::require("math")) {
        NEED_MODULE(Module_Math);
    }
    if (!Module::require("strings")) {
        NEED_MODULE(Module_Strings);
    }
    if (!Module::require("rtti_core")) {
        NEED_MODULE(Module_Rtti);
    }
    if (!Module::require("ast_core")) {
        NEED_MODULE(Module_Ast);
    }
    if (!Module::require("jit")) {
        NEED_MODULE(Module_Jit);
    }
    if (!Module::require("debugapi")) {
        NEED_MODULE(Module_Debugger);
    }
    if (!Module::require("network_core")) {
        NEED_MODULE(Module_Network);
    }
    if (!Module::require("uriparser")) {
        NEED_MODULE(Module_UriParser);
    }
    if (!Module::require("jobque")) {
        NEED_MODULE(Module_JobQue);
    }
    if (!Module::require("fio_core")) {
        NEED_MODULE(Module_FIO);
    }
    if (!Module::require("dasbind")) {
        NEED_MODULE(Module_DASBIND);
    }
}

namespace das {
    void register_builtin_modules() {
        // Register from namespace is not yet supported.
        register_builtin_modules_impl();
    }
}