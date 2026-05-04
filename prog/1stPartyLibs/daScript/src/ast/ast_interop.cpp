#include "daScript/ast/ast_interop.h"

namespace das {
    // add extern func

    void addExternFunc(Module& mod, const FunctionPtr & fnX, bool isCmres, SideEffects seFlags) {
        if (!isCmres) {
            if (fnX->result->isRefType() && !fnX->result->ref) {
                DAS_FATAL_ERROR(
                    "addExtern(%s)::failed in module %s\n"
                    "  this function should be bound with addExtern<DAS_BIND_FUNC(%s), SimNode_ExtFuncCallAndCopyOrMove>\n"
                    "  likely cast<> is implemented for the return type, and it should not\n",
                    fnX->name.c_str(), mod.name.c_str(), fnX->name.c_str());
            }
        }
        fnX->setSideEffects(seFlags);
        if (!mod.addFunction(fnX)) {
            DAS_FATAL_ERROR("addExtern(%s) failed in module %s\n", fnX->name.c_str(), mod.name.c_str());
        }
    }
}
