#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"

#include "daScript/simulate/aot_builtin.h"

namespace das {

    void Module_BuiltIn::addArrayTypes(ModuleLibrary & lib) {
        // array functions
        // the public 'clear' is a builtin.das generic (finalize banner); this is its raw half
        addExternInline<DAS_BIND_FUN(builtin_array_clear)>(*this, lib, "__builtin_array_clear",
            SideEffects::modifyArgument, "builtin_array_clear")
                ->args({"array","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_size)>(*this, lib, "length",
            SideEffects::none, "builtin_array_size")
                ->arg("array");
        addExternInline<DAS_BIND_FUN(builtin_array_empty)>(*this, lib, "empty",
            SideEffects::none, "builtin_array_empty")
                ->arg("array");
        addExternInline<DAS_BIND_FUN(builtin_array_capacity)>(*this, lib, "capacity",
            SideEffects::none, "builtin_array_capacity")
                ->arg("array");
        addExternInline<DAS_BIND_FUN(builtin_array_long_size)>(*this, lib, "long_length",
            SideEffects::none, "builtin_array_long_size")
                ->arg("array");
        addExternInline<DAS_BIND_FUN(builtin_array_long_capacity)>(*this, lib, "long_capacity",
            SideEffects::none, "builtin_array_long_capacity")
                ->arg("array");
        addExternInline<DAS_BIND_FUN(builtin_array_lock_count)>(*this, lib, "lock_count",
            SideEffects::none, "builtin_array_lock_count")
                ->arg("array");
        // array built-in functions
        addExternInline<DAS_BIND_FUN(builtin_array_resize)>(*this, lib, "__builtin_array_resize",
            SideEffects::modifyArgument, "builtin_array_resize")
                ->args({"array","newSize","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_resize_no_init)>(*this, lib, "__builtin_array_resize_no_init",
            SideEffects::modifyArgument, "builtin_array_resize_no_init")
                ->args({"array","newSize","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_reserve)>(*this, lib, "__builtin_array_reserve",
            SideEffects::modifyArgument, "builtin_array_reserve")
                ->args({"array","newSize","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_resize_i64)>(*this, lib, "__builtin_array_resize_i64",
            SideEffects::modifyArgument, "builtin_array_resize_i64")
                ->args({"array","newSize","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_resize_no_init_i64)>(*this, lib, "__builtin_array_resize_no_init_i64",
            SideEffects::modifyArgument, "builtin_array_resize_no_init_i64")
                ->args({"array","newSize","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_reserve_i64)>(*this, lib, "__builtin_array_reserve_i64",
            SideEffects::modifyArgument, "builtin_array_reserve_i64")
                ->args({"array","newSize","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_push)>(*this, lib, "__builtin_array_push",
            SideEffects::modifyArgument, "builtin_array_push")
                ->args({"array","index","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_push_zero)>(*this, lib, "__builtin_array_push_zero",
            SideEffects::modifyArgument, "builtin_array_push_zero")
                ->args({"array","index","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_push_back)>(*this, lib, "__builtin_array_push_back",
            SideEffects::modifyArgument, "builtin_array_push_back")
                ->args({"array","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_push_back_zero)>(*this, lib, "__builtin_array_push_back_zero",
            SideEffects::modifyArgument, "builtin_array_push_back_zero")
                ->args({"array","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_erase)>(*this, lib, "__builtin_array_erase",
            SideEffects::modifyArgument, "builtin_array_erase")
                ->args({"array","index","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_erase_range)>(*this, lib, "__builtin_array_erase_range",
            SideEffects::modifyArgument, "builtin_array_erase_range")
                ->args({"array","index","count","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_erase_i64)>(*this, lib, "__builtin_array_erase_i64",
            SideEffects::modifyArgument, "builtin_array_erase_i64")
                ->args({"array","index","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_erase_range_i64)>(*this, lib, "__builtin_array_erase_range_i64",
            SideEffects::modifyArgument, "builtin_array_erase_range_i64")
                ->args({"array","index","count","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_lock)>(*this, lib, "__builtin_array_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_array_lock")
                ->args({"array","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_lock_mutable)>(*this, lib, "__builtin_array_lock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_array_lock_mutable")
                ->args({"array","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_unlock)>(*this, lib, "__builtin_array_unlock",
            SideEffects::modifyArgumentAndExternal, "builtin_array_unlock")
                ->args({"array","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_array_unlock_mutable)>(*this, lib, "__builtin_array_unlock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_array_unlock_mutable")
                ->args({"array","context","at"});

        addExternInline<DAS_BIND_FUN(builtin_array_clear_lock)>(*this, lib, "__builtin_array_clear_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_array_clear_lock")
                ->args({"array","context"});
        addExternInline<DAS_BIND_FUN(builtin_array_tag)>(*this, lib, "tag_array",
            SideEffects::modifyExternal, "builtin_array_tag")
                ->args({"array","name","context"});
        // scratch: the owner promises no interior alias escapes a grow, so growth may free
        // the old buffer eagerly even in a verySafeContext (set once on internal containers)
        addExternInline<DAS_BIND_FUN(builtin_array_set_scratch)>(*this, lib, "set_scratch",
            SideEffects::modifyArgument, "builtin_array_set_scratch")
                ->args({"array","value","context"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(builtin_array_is_scratch)>(*this, lib, "is_scratch",
            SideEffects::none, "builtin_array_is_scratch")
                ->arg("array");
        addExternInline<DAS_BIND_FUN(builtin_array_scratch_reserve)>(*this, lib, "__builtin_array_scratch_reserve",
            SideEffects::modifyArgument, "builtin_array_scratch_reserve")
                ->args({"array","newSize","stride","context","at"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(builtin_array_scratch_reserve_i64)>(*this, lib, "__builtin_array_scratch_reserve_i64",
            SideEffects::modifyArgument, "builtin_array_scratch_reserve_i64")
                ->args({"array","newSize","stride","context","at"})->unsafeOperation = true;
    }
}
