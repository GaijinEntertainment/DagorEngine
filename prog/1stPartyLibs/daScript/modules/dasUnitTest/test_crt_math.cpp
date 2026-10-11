#include "daScript/misc/platform.h"

#include "daScript/ast/ast_interop.h"

#include "module_unitTest.h"
#include "unitTest.h"

void Module_UnitTest::addCrtMath(ModuleLibrary & lib) {
    addExternInline<DAS_BIND_FUN(crt_expf)>(*this, lib, "crt_expf", SideEffects::none, "crt_expf")->arg("x");
    addExternInline<DAS_BIND_FUN(crt_exp2f)>(*this, lib, "crt_exp2f", SideEffects::none, "crt_exp2f")->arg("x");
    addExternInline<DAS_BIND_FUN(crt_logf)>(*this, lib, "crt_logf", SideEffects::none, "crt_logf")->arg("x");
    addExternInline<DAS_BIND_FUN(crt_log2f)>(*this, lib, "crt_log2f", SideEffects::none, "crt_log2f")->arg("x");
    addExternInline<DAS_BIND_FUN(crt_powf)>(*this, lib, "crt_powf", SideEffects::none, "crt_powf")->args({"x","y"});
    addExternInline<DAS_BIND_FUN(crt_sinf)>(*this, lib, "crt_sinf", SideEffects::none, "crt_sinf")->arg("x");
    addExternInline<DAS_BIND_FUN(crt_cosf)>(*this, lib, "crt_cosf", SideEffects::none, "crt_cosf")->arg("x");
    addExternInline<DAS_BIND_FUN(crt_tanf)>(*this, lib, "crt_tanf", SideEffects::none, "crt_tanf")->arg("x");
}
