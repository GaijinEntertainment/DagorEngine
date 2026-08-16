options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = false
options relaxed_pointer_const
options unsafe_table_lookup = false
options no_global_variables = false
options stack = 4_194_304 // On huge files we need huge stack for jit

module llvm_exe shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_intrin
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_dll_utils
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/safe_addr
require daslib/strings_boost
require daslib/defer
require daslib/lpipe
require daslib/rtti
require daslib/clargs
require llvm/daslib/llvm_tune   // tune_scopes_status for the release-deps JSON
require daslib/json_boost
require jit
require daslib/fio

module llvm_exe shared private

bitfield TabOperation {
    at,
    find,
    erase
}


// Get-or-add das_aot_get_func_by_mnh and build a call resolving `mnh` against `ctx_arg` to the host's
// SimFunction* (void*). Shared by the aot-object register path and the globinit ctor.
def public build_aot_get_func_by_mnh(b : LLVMBuilderRef; types : PrimitiveTypes?; mnh : uint64; ctx_arg : LLVMOpaqueValue?) : LLVMOpaqueValue? {
    let gfType = LLVMFunctionType(types.LLVMVoidPtrType(),
        fixed_array<LLVMTypeRef>(types.t_int64, types.LLVMVoidPtrType()))
    var gfFn = LLVMGetNamedFunction(g_mod, "das_aot_get_func_by_mnh")
    if (gfFn == null) {
        gfFn = LLVMAddFunctionWithType(g_mod, "das_aot_get_func_by_mnh", gfType)
    }
    var gf_args = array<LLVMValueRef>(types.ConstI64(mnh), ctx_arg)
    return LLVMBuildCall2(b, gfType, gfFn, gf_args, "")
}


class public CollectExternVisitor : AstVisitor {
    uid : UidNodes?

    // Let's keep order of traversal during initialization.
    // In this way we need `seen` table to not initialize twice.
    table_calls : array<tuple<t_type : Type; dll_name : DllName; tab_operation : TabOperation>>
    address_expr : array<tuple<dll_name : DllName; fn : string>>
    tab_at_accessor : LLVMOpaqueValue?
    tab_find_accessor : LLVMOpaqueValue?
    tab_erase_accessor : LLVMOpaqueValue?
    tab_at_wh_accessor : LLVMOpaqueValue?
    tab_at_apm_accessor : LLVMOpaqueValue?
    reg_extern_tab_type : LLVMOpaqueType?
    reg_extern_tab_wh_type : LLVMOpaqueType?
    reg_standalone_fn_type : LLVMOpaqueType?
    reg_standalone_fn : LLVMOpaqueValue?
    builder : LLVMBuilderRef
    ctx : LLVMContextRef
    mod : LLVMOpaqueModule?
    types : PrimitiveTypes -const? -const
    seen : table<string; bool>
    needs_whole_lib : bool
    extern_modules : table<string; bool>
    registered_modules : table<string; bool>
    dynamic_modules : table<string; bool>
    register_mod_type : LLVMOpaqueType?
    init_builder : LLVMBuilderRef

    reg_extern_fn_type : LLVMOpaqueType?
    reg_extern_fn : LLVMOpaqueValue?

    get_extern_ann_type : LLVMOpaqueType?
    get_extern_ann : LLVMOpaqueValue?

    c_get_jit_new_type : LLVMOpaqueType?
    c_get_jit_new : LLVMOpaqueValue?
    c_get_jit_delete_type : LLVMOpaqueType?
    c_get_jit_delete : LLVMOpaqueValue?
    c_get_jit_clone_type : LLVMOpaqueType?
    c_get_jit_clone : LLVMOpaqueValue?

    standalone_ctx : LLVMOpaqueValue?

    // The globinit ctor's Context* parameter (set by emit_aot_object_globinit),
    // used to resolve Func-value globs to the host's SimFunction* at load.
    aot_ctx_arg : LLVMOpaqueValue?
    jit_mod : LlvmJitMode

    // Create the initialize_modules() function, position a builder in it, and emit das_ensure_environment
    // plus the builtin-module registrations (all native modules if register_all, else $ + strings). Returns the builder.
    def private setup_init_builder(register_all : bool) : LLVMBuilderRef {
        let init_fn = LLVMGetNamedFunction(g_mod, "initialize_modules")
        let init_entry = LLVMAppendBasicBlockInContext(ctx, init_fn, "entry")
        var ib = LLVMCreateBuilder()
        LLVMPositionBuilderAtEnd(ib, init_entry)
        let env_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
        var env_fn = LLVMGetNamedFunction(g_mod, "das_ensure_environment")
        if (env_fn == null) {
            env_fn = LLVMAddFunctionWithType(g_mod, "das_ensure_environment", env_type)
        }
        LLVMBuildCall2(ib, env_type, env_fn, array<LLVMValueRef>(), "")
        if (register_all) {
            var reg_all = LLVMGetNamedFunction(g_mod, "jit_register_all_builtin_modules")
            if (reg_all == null) {
                reg_all = LLVMAddFunctionWithType(g_mod, "jit_register_all_builtin_modules", env_type)
            }
            LLVMBuildCall2(ib, env_type, reg_all, array<LLVMValueRef>(), "")
            for (m in fixed_array("$", "math", "strings", "rtti_core", "ast_core", "jit",
                                  "debugapi", "network_core", "uriparser", "jobque",
                                  "fio_core", "dasbind")) {
                registered_modules[m] = true
            }
        } else {
            // Register $ and strings unless inject_main's static sweep already emitted the (non-idempotent) call.
            if (!(g_exe_emitted_reg |> key_exists("jit_register_Module_BuiltIn"))) {
                g_exe_emitted_reg["jit_register_Module_BuiltIn"] = true
                var reg_builtin = LLVMAddFunctionWithType(g_mod, "jit_register_Module_BuiltIn", register_mod_type)
                LLVMBuildCall2(ib, register_mod_type, reg_builtin, array<LLVMValueRef>(), "")
            }
            registered_modules["$"] = true
            if (!(g_exe_emitted_reg |> key_exists("jit_register_Module_Strings"))) {
                g_exe_emitted_reg["jit_register_Module_Strings"] = true
                var reg_strings = LLVMAddFunctionWithType(g_mod, "jit_register_Module_Strings", register_mod_type)
                LLVMBuildCall2(ib, register_mod_type, reg_strings, array<LLVMValueRef>(), "")
            }
            registered_modules["strings"] = true
        }
        return ib
    }

    def CollectExternVisitor(standalone_context : LLVMOpaqueValue?; _ctx : LLVMContextRef; // nolint:STYLE038 — LLVM type-table setup: one cached type/fn-type per line
                             _builder : LLVMBuilderRef,
                             _mod : LLVMOpaqueModule?; var _types : PrimitiveTypes?,
                             uids : UidNodes?; register_all : bool; _jit_mod : LlvmJitMode) {
        needs_whole_lib = false
        uid := uids
        ctx = _ctx
        standalone_ctx = standalone_context
        mod = _mod
        jit_mod = _jit_mod
        types = _types
        builder = _builder
        register_mod_type = LLVMFunctionType(types.LLVMVoidPtrType(), array<LLVMTypeRef>())

        // Only exe/dll init modules at startup; AOT-object re-resolves globals into the caller's ctor (builder), no init_builder.
        if (!g_emit_aot_object_globals) {
            init_builder = setup_init_builder(register_all)
        }

        // void * get_jit_table_{at,find,erase} ( int32_t baseType, Context * context, LineInfoArg * at )
        reg_extern_tab_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.t_int32,            // baseType
                types.LLVMVoidPtrType(),  // context
                types.LLVMVoidPtrType(),  // at
            )
        )
        tab_at_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_table_at", reg_extern_tab_type)
        tab_find_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_table_find", reg_extern_tab_type)
        tab_erase_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_table_erase", reg_extern_tab_type)

        // String `at` routes through this precomputed-hash global (grow fallback), baked into the exe (find is fully inline).
        reg_extern_tab_wh_type = LLVMFunctionType(types.LLVMVoidPtrType(), [])
        tab_at_wh_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_string_table_at_with_hash", reg_extern_tab_wh_type)

        // String insert-after-packed-miss global (inline packed find calls it on a miss); baked into the exe to survive ASLR.
        tab_at_apm_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_string_table_at_after_packed_miss", reg_extern_tab_wh_type)

        reg_extern_fn_type = g_fn_types[FN_JIT_INIT_EXTERN]
        reg_extern_fn = LLVMGetNamedFunction(g_mod, FN_JIT_INIT_EXTERN)

        get_extern_ann_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // const char* module_name
                types.LLVMVoidPtrType(),  // const char* ann_name
            )
        )
        get_extern_ann = LLVMAddFunctionWithType(g_mod, "jit_get_annotation", get_extern_ann_type)

        c_get_jit_new_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // TypeAnnotation*
            )
        )
        c_get_jit_new = LLVMAddFunctionWithType(g_mod, "das_get_jit_new", c_get_jit_new_type)

        c_get_jit_delete_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // TypeAnnotation*
            )
        )
        c_get_jit_delete = LLVMAddFunctionWithType(g_mod, "das_get_jit_delete", c_get_jit_delete_type)

        c_get_jit_clone_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // TypeAnnotation*
            )
        )
        c_get_jit_clone = LLVMAddFunctionWithType(g_mod, "das_get_jit_clone", c_get_jit_clone_type)

    // For each JIT function, register it in the context and point any ExprAddr global to it.
        reg_standalone_fn_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // Context *
                types.t_int64,            // uint64_t index
                types.LLVMVoidPtrType(),  // const char * name
                types.LLVMVoidPtrType(),  // const char * mangledName
                types.t_int64,            // uint64_t mnh
                types.t_int32,            // uint32_t stackSize
                types.LLVMVoidPtrType(),  // void * fnPtr
                types.t_int1,             // bool cmres
                types.t_int1,             // bool fastcall
                types.t_int1,             // bool pinvoke
                types.t_int32,            // uint32_t nArguments
            )
        )
        reg_standalone_fn = LLVMAddFunctionWithType(g_mod, "jit_register_standalone_function", reg_standalone_fn_type)
    }

    def register_function(fn : Function?) : LLVMOpaqueValue? {
        // AOT-into-host: a Func VALUE is the host's SimFunction*, looked up by mnh against the ctor's Context* (works for builtins too).
        if (jit_mod == LlvmJitMode.LLVM_AOT) {
            return build_aot_get_func_by_mnh(builder, types, fn.getMangledNameHash, aot_ctx_arg)
        }
        let fnmna = uid.get_dll_fn_name_ptr(fn)
        let fn_publ = LLVMGetNamedFunction(g_mod, fnmna.publ())
        if (fn_publ == null) return null
        let fn_ptr_cast = LLVMBuildPointerCast(builder, fn_publ, types.LLVMVoidPtrType(), "")
        var reg_args = fixed_array<LLVMOpaqueValue?>(
            standalone_ctx,
            types.ConstI64(fn.index |> uint64),
            get_string_constant_ptr(builder, string(fn.name)),
            get_string_constant_ptr(builder, fn |> get_mangled_name()),
            types.ConstI64(fn.getMangledNameHash),
            types.ConstI32(fn.totalStackSize |> uint64),
            fn_ptr_cast,
            types.ConstI1(isCMRES(fn)),
            types.ConstI1(fn.flags.fastCall),
            types.ConstI1(fn.moreFlags.pinvoke),
            types.ConstI32(fn.arguments |> length |> uint64),
        )
        return LLVMBuildCall2(builder, reg_standalone_fn_type, reg_standalone_fn, reg_args, "")
    }

    def override preVisitFunction(var fun : FunctionPtr) : void {
        uid.reset(fun)
    }

    def override visitFunction(fun : FunctionPtr) : FunctionPtr {
        uid.reset(null)
        return fun
    }

    // No need to set global pointers more than one time.
    // Although it won't break anything.
    def initialize(name : DllName) {
        if (!seen |> key_exists(name.glob())) {
            seen |> insert(name.glob(), true)
            return true
        } else {
            return false
        }
    }

    def ensure_module(m : Module?) {
        if (g_emit_aot_object_globals || m == null) return  // host already has every module registered
        let mod_name = string(m.name)
        // Dynamic modules register via the runtime load path (skipped here) — except wasm cross-compile, where they're statically linked and must register like any module.
        if (key_exists(registered_modules, mod_name) || (key_exists(dynamic_modules, mod_name) && !g_target_is_wasm)) return
        registered_modules[mod_name] = true
        // First ensure all dependencies of this module are registered
        module_for_each_dependency(m) $(var dep : Module?; var _pub : bool) {
            ensure_module(dep)
        }
        let cpp_name = string(m.cppClassName)
        if (empty(cpp_name)) return
        let reg_fn_name = "jit_register_{cpp_name}"
        to_log(LOG_INFO, "LLVM EXE: NEED_MODULE({cpp_name}) for `{mod_name}`\n")
        // ast_core / network_core live in the compiler lib, so their jit_register thunk forces a whole-compiler-lib link.
        if (mod_name == "ast_core" || mod_name == "network_core") {
            needs_whole_lib = true
        }
        // One call per thunk process-wide (not idempotent); get-or-add avoids a silent rename to an undefined symbol.
        return if (g_exe_emitted_reg |> key_exists(reg_fn_name))
        g_exe_emitted_reg[reg_fn_name] = true
        var reg_fn = LLVMGetNamedFunction(g_mod, reg_fn_name)
        if (reg_fn == null) {
            reg_fn = LLVMAddFunctionWithType(g_mod, reg_fn_name, register_mod_type)
        }
        LLVMBuildCall2(init_builder, register_mod_type, reg_fn, array<LLVMValueRef>(), "")
    }

    def make_call(expr : ExprCallFunc?) {
        if (expr.func == null || has_intrinsic(expr)) return
        if (expr.func.flags.builtIn) {
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (FUNC_PTR == null) return
            let name = uid.get_dll_fn_name_ptr(expr.func)
            if (initialize(name)) {
                // Emit a registration call that assigns the global function-pointer at startup.
                let global_var = LLVMGetNamedGlobal(g_mod, name.glob())
                // JIT may skip emitting a global under optimization (seen only for `count`); absent = never accessed, so skip.
                if (global_var == null) return
                let mod_name = string(expr.func._module.name)
                extern_modules[mod_name] = true
                ensure_module(expr.func._module)
                // Sort functions with cblock use Context/Block infrastructure from the compiler lib
                let fn_name = string(expr.func.name)
                if (find(fn_name, "sort") != -1 && find(fn_name, "cblock") != -1) {
                    needs_whole_lib = true
                }
                var call_args = array<LLVMValueRef>(
                    get_string_constant_ptr(builder, mod_name),
                    get_string_constant_ptr(builder, get_mangled_name(expr.func)),
                    LLVMBuildPointerCast(builder, global_var, types.LLVMVoidPtrType(), ""),
                )
                LLVMBuildCall2(builder, reg_extern_fn_type, reg_extern_fn, call_args, "")
            }
        }
    }

    def get_annotation(ann : TypeAnnotation?) {
        let mod_name = string(ann._module.name)
        extern_modules[mod_name] = true
        ensure_module(ann._module)
        var call_args = array<LLVMValueRef>(
            get_string_constant_ptr(builder, mod_name),
            get_string_constant_ptr(builder, string(ann.name)),
        )
        return LLVMBuildCall2(builder, get_extern_ann_type, get_extern_ann, call_args, "")
    }

    def register_new_from_annotation(name : DllName; ann : TypeAnnotation?) {
        let ann_ptr = get_annotation(ann)
        var call_args = array<LLVMValueRef>(ann_ptr)
        let fn_ptr = LLVMBuildCall2(builder, c_get_jit_new_type, c_get_jit_new, call_args, "")
        let global_var = LLVMGetNamedGlobal(g_mod, name.glob())
        LLVMBuildStore(builder, fn_ptr, global_var)
    }

    def register_delete_from_annotation(name : DllName; ann : TypeAnnotation?) {
        let ann_ptr = get_annotation(ann)
        var call_args = array<LLVMValueRef>(ann_ptr)
        let fn_ptr = LLVMBuildCall2(builder, c_get_jit_delete_type, c_get_jit_delete, call_args, "")
        let global_var = LLVMGetNamedGlobal(g_mod, name.glob())
        LLVMBuildStore(builder, fn_ptr, global_var)
    }

    def register_clone_from_annotation(name : DllName; ann : TypeAnnotation?) {
        let ann_ptr = get_annotation(ann)
        var call_args = array<LLVMValueRef>(ann_ptr)
        let fn_ptr = LLVMBuildCall2(builder, c_get_jit_clone_type, c_get_jit_clone, call_args, "")
        let global_var = LLVMGetNamedGlobal(g_mod, name.glob())
        LLVMBuildStore(builder, fn_ptr, global_var)
    }

    def override preVisitExprCall(expr : ExprCall?) {
        make_call(expr)
    }

    def override preVisitExprOp2(var expr : ExprOp2?) {
        if (isExprOp2_Func(expr)) {
            make_call(expr)
        }
    }

    def override preVisitExprOp1(var expr : ExprOp1?) {
        if (isExprOp1_Func(expr)) {
            make_call(expr)
        }
    }

    def override preVisitExprNew(expr : ExprNew?) {
        if (expr.typeexpr.isHandle) {
            let name = uid.get_new(expr.typeexpr)
            if (initialize(name)) {
                register_new_from_annotation(name, expr.typeexpr.annotation)
            }
        } else {
            if (expr.typeexpr.baseType == Type.tFixedArray) {
                // handle gate is on the chain element (head is tFixedArray); borrow-walk (outside ast_gc_guard, clones would leak)
                var elemT = expr.typeexpr
                while (elemT.baseType == Type.tFixedArray && elemT.firstType != null) {
                    elemT = elemT.firstType
                }
                if (elemT.baseType == Type.tHandle) {
                    let name = uid.get_new(expr.typeexpr)
                    if (initialize(name)) {
                        register_new_from_annotation(name, elemT.annotation)
                    }
                } else {
                    if (expr.initializer) {
                        make_call(expr)
                    }
                }
            } else {
                if (expr.initializer) {
                    make_call(expr)
                }
            }
        }
    }

    def override preVisitExprDelete(var expr : ExprDelete?) {
        assume subexpr = expr.subexpr
        assume subT = expr.subexpr._type
        if (subT.isPointer) {
            assume firstT = expr.subexpr._type.firstType
            if (firstT.isHandle) {
                let name = uid.get_delete_type(firstT)
                if (initialize(name)) {
                    register_delete_from_annotation(name, firstT.annotation)
                }
            }
        }
    }

    def override preVisitExprClone(expr : ExprClone?) : void {
        assume leftT = expr.left._type
        if (leftT.isHandle) {
            let name = uid.get_clone_type(leftT)
            if (initialize(name)) {
                register_clone_from_annotation(name, leftT.annotation)
            }
        }
    }

    def override preVisitExprAddr(expr : ExprAddr?) {
        let fn_ptr = register_function(expr.func)
        if (fn_ptr == null) return
        let name = uid.get_addr(expr)
        let global_var = LLVMGetNamedGlobal(mod, name.glob())
        // Can be unvisited, e.g. default function argument.
        if (global_var == null) return
        LLVMBuildStore(builder, fn_ptr, global_var)
    }

    def override preVisitExprMakeStructIndex(expr : ExprMakeStruct?; index : int; last : bool) : void {
        if (expr.constructor != null && !expr.constructor.moreFlags.requestJit) {
            register_function(expr.constructor)
        }
    }

    def private get_table_t(subexprT : TypeDeclPtr) {
        assume firstT = subexprT.firstType
        if (firstT.isWorkhorseType) {
            return firstT.baseType
        } else {
            var uvt = get_underlying_value_type(firstT)
            return uvt.baseType
        }
    }

    // get_jit_table_{at,find,erase} dispatch on baseType only; context+at unused.
    // Pass null for both to match the C++ signature without a real Context.
    def add_table_at_call(index_t : TypeDeclPtr) {
        let typ = get_table_t(index_t)
        if (typ == Type.tString) {
            // string keys route through the precomputed-hash at (see build_table_at); bake it + its after-packed-miss insert helper
            let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_wh_type, tab_at_wh_accessor, [], "")
            let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_STRING_TABLE_AT_WITH_HASH()).glob())
            if (tab_method != null) {
                LLVMBuildStore(builder, tab_ptr, tab_method)
            }
            let apm_ptr = LLVMBuildCall2(builder, reg_extern_tab_wh_type, tab_at_apm_accessor, [], "")
            let apm_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_STRING_TABLE_AT_AFTER_PACKED_MISS()).glob())
            if (apm_method != null) {
                LLVMBuildStore(builder, apm_ptr, apm_method)
            }
            return
        }
        let null_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
        var call_args = array<LLVMValueRef>(types.ConstI32(uint64(typ)), null_ptr, null_ptr)
        let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_type, tab_at_accessor, call_args, "")

        let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_TABLE_AT(typ)).glob())
        LLVMBuildStore(builder, tab_ptr, tab_method)
    }

    def add_table_find_call(index_t : TypeDeclPtr) {
        let typ = get_table_t(index_t)
        // String + workhorse find are fully inline; the per-type C++ find glob exists only for non-workhorse finds, so a miss = no consumer to bake.
        if (typ == Type.tString) {
            return
        }
        let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_TABLE_FIND(typ)).glob())
        if (tab_method == null) {
            return
        }
        let null_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
        var call_args = array<LLVMValueRef>(types.ConstI32(uint64(typ)), null_ptr, null_ptr)
        let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_type, tab_find_accessor, call_args, "")
        LLVMBuildStore(builder, tab_ptr, tab_method)
    }

    def add_table_erase_call(index_t : TypeDeclPtr) {
        let typ = get_table_t(index_t)
        let null_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
        var call_args = array<LLVMValueRef>(types.ConstI32(uint64(typ)), null_ptr, null_ptr)
        let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_type, tab_erase_accessor, call_args, "")

        let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_TABLE_ERASE(typ)).glob())
        LLVMBuildStore(builder, tab_ptr, tab_method)
    }

    def override preVisitExprAt(expr : ExprAt?) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType) {
            add_table_at_call(subExprT)
        }
    }

    def override preVisitExprSetInsert(expr : ExprSetInsert?) {
        add_table_at_call(expr.arguments[0]._type)
    }

    def override preVisitExprKeyExists(expr : ExprKeyExists?) {
        add_table_find_call(expr.arguments[0]._type)
    }

    def override preVisitExprFind(expr : ExprFind?) {
        add_table_find_call(expr.arguments[0]._type)
    }

    def override preVisitExprSafeAt(expr : ExprSafeAt?) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType || (subExprT.isPointer && subExprT.firstType.isGoodTableType)) {
            var etype = subExprT.isPointer ? subExprT.firstType : subExprT
            add_table_find_call(etype)
        }
    }

    def override preVisitExprErase(expr : ExprErase?) {
        add_table_erase_call(expr.arguments[0]._type)
    }
}

def collect_external_functions(standalone_context : LLVMOpaqueValue?; ctx : LLVMContextRef; builder : LLVMBuilderRef, // nolint:STYLE038 — flat emission walk: one registration form per function class
                               mod : LLVMOpaqueModule ?; var types : PrimitiveTypes?;
                               var funcs : array<FunctionPtr>; var uids : UidNodes?;
                               var used_modules : table<string; bool>&;
                               dynamic_modules : table<string; bool>;
                               prog : Program?; register_all_modules : bool = false) : bool {
    var extern_resolver = new CollectExternVisitor(standalone_context, ctx, builder, mod, types, uids, register_all_modules, LlvmJitMode.EXE)
    extern_resolver.dynamic_modules := dynamic_modules
    make_visitor(*extern_resolver) $(adapter_resolve) {
        ast_gc_guard() {
            for (fn in funcs) {
                visit(fn, adapter_resolve)
            }
            // Register builtins reachable only from top-level `let X = builtin()` (#2582); init_globals must JIT first so the globals exist.
            prog |> for_each_module() $(m) {
                if (m.moduleFlags.builtIn) return ;
                m |> for_each_global() $(var glob) {
                    if (glob.init != null && glob.flags.used) {
                        visit_expression(glob.init, adapter_resolve)
                    }
                }
            }
        }
        // Register [pinvoke]/[export] functions in tabMnLookup so they're findable by name at runtime.
        for (fn in funcs) {
            if (!fn.flags.builtIn && (fn.moreFlags.pinvoke || fn.flags.exports)) {
                extern_resolver.uid.reset(fn)
                extern_resolver.register_function(fn)
                extern_resolver.uid.reset(null)
            }
        }
    }
    let whole_lib = extern_resolver.needs_whole_lib
    for (mod_name in keys(extern_resolver.extern_modules)) {
        used_modules[mod_name] = true
    }
    // Finalize initialize_modules() via the same 3-tier resolve as inject_main, so a standalone exe doesn't double-register a module from two paths.
    let ib = extern_resolver.init_builder
    let t = extern_resolver.types
    let reg_dyn_mod_type = LLVMFunctionType(t.LLVMVoidPtrType(),
        fixed_array<LLVMTypeRef>(t.LLVMVoidPtrType(), t.LLVMVoidPtrType(), t.LLVMVoidPtrType())
    )
    var jit_reg_dyn_mod = LLVMGetNamedFunction(g_mod, "jit_register_dynamic_module_resolve")
    if (jit_reg_dyn_mod == null) {
        jit_reg_dyn_mod = LLVMAddFunctionWithType(g_mod, "jit_register_dynamic_module_resolve", reg_dyn_mod_type)
    }
    for_each_registered_dynamic_module() $(path, mod_name, das_name) {
        if (key_exists(used_modules, das_name)) {
            to_log(LOG_INFO, "LLVM EXE: include dynamic module: `{das_name}`\n")
            let rel = compute_modules_relative_suffix(path)
            let rel_str = get_string_constant_ptr(ib, rel)
            let abs_str = get_string_constant_ptr(ib, path)
            let name_str = get_string_constant_ptr(ib, mod_name)
            LLVMBuildCall2(ib, reg_dyn_mod_type, jit_reg_dyn_mod, fixed_array(rel_str, abs_str, name_str), "")
        }
    }
    let reg_native_path_type = LLVMFunctionType(t.t_void,
        fixed_array<LLVMTypeRef>(t.LLVMVoidPtrType(), t.LLVMVoidPtrType(), t.LLVMVoidPtrType())
    )
    var jit_reg_native_path = LLVMGetNamedFunction(g_mod, "jit_register_native_path")
    if (jit_reg_native_path == null) {
        jit_reg_native_path = LLVMAddFunctionWithType(g_mod, "jit_register_native_path", reg_native_path_type)
    }
    for_each_registered_native_path() $(mod_name, src_path, dst_path) {
        let mod_str = get_string_constant_ptr(ib, mod_name)
        let src_str = get_string_constant_ptr(ib, src_path)
        let dst_str = get_string_constant_ptr(ib, dst_path)
        LLVMBuildCall2(ib, reg_native_path_type, jit_reg_native_path, fixed_array(mod_str, src_str, dst_str), "")
    }
    if (!empty(extern_resolver.registered_modules) || !empty(used_modules)) {
        let init_done_type = LLVMFunctionType(t.t_void, array<LLVMTypeRef>())
        var jit_init_done = LLVMGetNamedFunction(g_mod, "jit_initialize_modules_done")
        if (jit_init_done == null) {
            jit_init_done = LLVMAddFunctionWithType(g_mod, "jit_initialize_modules_done", init_done_type)
        }
        LLVMBuildCall2(ib, init_done_type, jit_init_done, array<LLVMValueRef>(), "")
    }
    // Wasm: fill per-(handled-type, field) offset globals from the TARGET runtime's annotations (now registered) — each access reads a global, not a host-baked constant.
    if (!empty(g_handled_field_offset_globals)) {
        var hfo_fn = LLVMGetNamedFunction(g_mod, FN_JIT_GET_HANDLED_FIELD_OFFSET)
        let hfo_type = g_fn_types[FN_JIT_GET_HANDLED_FIELD_OFFSET]
        for (globName, hinfo in keys(g_handled_field_offset_globals), values(g_handled_field_offset_globals)) {
            var glob = LLVMGetNamedGlobal(g_mod, globName)
            if (glob == null) continue
            let mod_str = get_string_constant_ptr(ib, hinfo.modName)
            let type_str = get_string_constant_ptr(ib, hinfo.typeName)
            let field_str = get_string_constant_ptr(ib, hinfo.fieldName)
            let off_val = LLVMBuildCall2(ib, hfo_type, hfo_fn, fixed_array(mod_str, type_str, field_str), "")
            LLVMBuildStore(ib, off_val, glob)
        }
    }
    // --jit-check-abi: sweep every handled type used, emit host-vs-target size+offset checks, then a report/kaboom call.
    if (g_jit_check_handled_abi) {
        emit_handled_abi_check(ib, prog)
    }
    LLVMBuildRetVoid(ib)
    LLVMDisposeBuilder(ib)
    extern_resolver.uid = null
    extern_resolver.types = null
    unsafe { delete extern_resolver; }
    return whole_lib
}

// --jit-check-abi: emit into initialize_modules() a host-vs-target layout safe-check for every handled
// type referenced. Bakes HOST size + field offsets into check calls that resolve the TARGET annotation
// at runtime, record every divergence, then jit_handled_abi_check_report() dumps them all and aborts.
def private emit_handled_abi_check(ib : LLVMBuilderRef; prog : Program?) {
    let size_fn = LLVMGetNamedFunction(g_mod, FN_JIT_CHECK_HANDLED_TYPE_SIZE)
    let size_type = g_fn_types[FN_JIT_CHECK_HANDLED_TYPE_SIZE]
    let off_fn = LLVMGetNamedFunction(g_mod, FN_JIT_CHECK_HANDLED_FIELD_OFFSET)
    let off_type = g_fn_types[FN_JIT_CHECK_HANDLED_FIELD_OFFSET]
    var seen : table<string; bool>
    var n_types = 0
    var n_fields = 0
    prog |> for_each_module() $(m) {
        module_for_each_annotation(m) $(value) {
            if (!value.isBasicStructureAnnotation) return
            let modName = string(m.name)
            let typeName = string(value.name)
            let key = "{modName}`{typeName}"
            if (key_exists(seen, key)) return
            seen[key] = true
            let host_size = int(get_handled_type_size(unsafe(reinterpret<TypeAnnotation?>(value))))
            let mod_str = get_string_constant_ptr(ib, modName)
            let type_str = get_string_constant_ptr(ib, typeName)
            LLVMBuildCall2(ib, size_type, size_fn,
                fixed_array(mod_str, type_str, g_prim_t.ConstI32(uint64(host_size))), "")
            n_types ++
            for_each_field(*unsafe(reinterpret<BasicStructureAnnotation?>(value))) $(name, _cppName, _xtype, offset) {
                if (offset == -1u) return
                let field_str = get_string_constant_ptr(ib, name)
                LLVMBuildCall2(ib, off_type, off_fn,
                    fixed_array(mod_str, type_str, field_str, g_prim_t.ConstI32(uint64(offset))), "")
                n_fields ++
            }
        }
    }
    to_log(LOG_INFO, "ABI-CHECK sweep emitted: {n_types} handled types, {n_fields} fields\n")
    let rep_fn = LLVMGetNamedFunction(g_mod, FN_JIT_HANDLED_ABI_CHECK_REPORT)
    let rep_type = g_fn_types[FN_JIT_HANDLED_ABI_CHECK_REPORT]
    LLVMBuildCall2(ib, rep_type, rep_fn, array<LLVMValueRef>(), "")
}

// Suffix from the last "modules/" segment onwards; "" → caller falls back to abs path.
// to_generic_path normalizes Windows backslashes so the search needs only one form.
def private compute_modules_relative_suffix(p : string) : string {
    let pn = to_generic_path(p)
    let idx = rfind(pn, "/modules/")
    return idx < 0 ? "" : slice(pn, idx + 1)
}

// Walk up `start_dir` for a `.das_package` file; returns the dir containing it, or "" if none within
// MAX_LEVELS. Bounded so a stray .das at filesystem root doesn't scan the whole tree.
let private MAX_DAS_PACKAGE_WALKUP_LEVELS = 8

def private find_enclosing_das_package(start_dir : string) : string {
    var dir = to_generic_path(start_dir)
    for (_ in range(MAX_DAS_PACKAGE_WALKUP_LEVELS)) {
        if (empty(dir) || dir == "/" || dir == ".") return ""
        if (fexist("{dir}/.das_package")) return dir
        let last_slash = rfind(dir, "/")
        if (last_slash <= 0) return ""
        dir = slice(dir, 0, last_slash)
    }
    return ""
}

// JSON record types written by `daslang -exe -list-shared-modules <path>`.
// `commands.das` (in daspkg) defines structurally-identical types and parses
// the JSON via `from_JV`. Keep field names + types in sync if either side moves.
struct private SharedModuleEntry {
    rel : string                    //!< relative bundle path (modules/<X>/<X>.shared_module); "" if path lacks /modules/
    abs : string                    //!< absolute path on the build machine
    das_name : string               //!< daslang-side module name registered by the dylib
}

struct private DasModuleEntry {
    name : string                   //!< daslang module name
    file : string                   //!< absolute path to the .das source file
    package_dir : string            //!< absolute path to the enclosing daspkg package (dir holding .das_package)
}

struct private TuneScopeDep {
    name : string                   //!< [tune_scope] name
    manifest : string               //!< the app tune sidecar the winners live in
    tuner : string                  //!< tuner harness path ("" = none declared)
    complete : bool                 //!< every [tune] kernel of this scope has a fresh sidecar entry
    fallbacks : string              //!< fallback stamps selected by this same compile/host
}

struct private ReleaseDeps {
    shared_modules : array<SharedModuleEntry>
    das_modules : array<DasModuleEntry>
    tune_scopes : array<TuneScopeDep>   //!< daspkg release tunes incomplete scopes, then rebuilds
}

// Extract the package dir name from a "modules/<X>/.../foo.shared_module" suffix, so
// `-force-shared-module dasGlfw` matches by package name as well as daslang module name.
def private dylib_package_name(rel : string) : string {
    if (empty(rel)) return ""
    let after_modules = slice(rel, length("modules/"))
    let next_slash = find(after_modules, "/")
    return next_slash > 0 ? slice(after_modules, 0, next_slash) : after_modules
}

// Append `entry` unless its absolute path was already recorded — one .shared_module can host
// several daslang modules (dasStbImage: stbimage/raster/stbtruetype), which would otherwise duplicate.
def private push_shared_unique(entry : SharedModuleEntry; var seen : table<string; bool>; var deps : ReleaseDeps) {
    if (key_exists(seen, entry.abs)) return
    seen[entry.abs] = true
    deps.shared_modules |> push(entry)
}

// Declarative parse of daslang's release-deps flags; clargs derives --field-name, other argv flags ignored.
[CommandLineArgs]
struct private ReleaseDepsArgs {
    @clarg_doc = "Path to write the JSON release-deps file"
    list_shared_modules : string

    @clarg_doc = "Force-include a shared module by daslang or package name (repeatable)"
    force_shared_module : array<string>
}

def private write_release_deps(prog : Program?) {
    let argv <- get_program_args()
    var r <- parse_args(type<ReleaseDepsArgs>, argv)
    if (r |> is_err) {
        to_log(LOG_ERROR, "LLVM EXE: bad release-deps args: {r |> unwrap_err}\n")
        return
    }
    let cfg <- r |> move_unwrap
    if (empty(cfg.list_shared_modules)) return
    // Snapshot the dynamic-module registry: by daslang-side name + a parallel by-package-name lookup.
    var by_das : table<string; SharedModuleEntry>
    var by_pkg : table<string; SharedModuleEntry>
    for_each_registered_dynamic_module() $(path, _mod_name, das_name) {
        let abs = path
        let rel = compute_modules_relative_suffix(abs)
        let entry = SharedModuleEntry(rel = rel, abs = abs, das_name = das_name)
        by_das[entry.das_name] = entry
        let pkg = dylib_package_name(rel)
        if (!empty(pkg)) {
            by_pkg[pkg] = entry
        }
    }
    var deps : ReleaseDeps
    var shared_seen : table<string; bool>
    // Auto-detected: every program module that has a registered dylib.
    prog |> for_each_module() $(m) {
        let mod_name = string(m.name)
        if (key_exists(by_das, mod_name)) {
            push_shared_unique(by_das[mod_name], shared_seen, deps)
        }
        let file = string(m.fileName)
        if (!empty(file)) {
            let pkg_dir = find_enclosing_das_package(dir_name(file))
            if (!empty(pkg_dir)) {
                deps.das_modules |> push(DasModuleEntry(
                    name = mod_name,
                    file = file,
                    package_dir = pkg_dir))
            }
        }
    }
    // User-forced (--force-shared-module): match by daslang-side name OR package dir name.
    for (name in cfg.force_shared_module) {
        if (key_exists(by_das, name)) {
            push_shared_unique(by_das[name], shared_seen, deps)
        } elif (key_exists(by_pkg, name)) {
            push_shared_unique(by_pkg[name], shared_seen, deps)
        } else {
            to_log(LOG_WARNING, "LLVM EXE: --force-shared-module `{name}` not registered (typo? not loaded by daslang)\n")
        }
    }
    // Tune scopes + per-key completeness vs the app sidecar: daspkg release runs incomplete scopes' tuners and rebuilds.
    var tstat <- tune_scopes_status(prog)
    deps.tune_scopes |> reserve(length(tstat))
    for (ts in tstat) {
        deps.tune_scopes |> push(TuneScopeDep(name = ts.name, manifest = ts.manifest,
            tuner = ts.tuner, complete = ts.complete, fallbacks = ts.fallbacks))
    }
    delete tstat
    let dst_path = cfg.list_shared_modules
    fopen(dst_path, "wb") $(f) {
        if (f == null) {
            to_log(LOG_ERROR, "LLVM EXE: cannot write release deps to {dst_path}\n")
            return
        }
        fwrite(f, sprint_json(deps, true))
        fwrite(f, "\n")
    }
    to_log(LOG_INFO, "LLVM EXE: wrote release deps to {dst_path} (shared={length(deps.shared_modules)}, das={length(deps.das_modules)})\n")
}

// ONE registration call per jit_register_<CppClass> across every emission site — the C++ thunks are
// NOT idempotent (a second call dies "Module already created"). Reset per codegen run in inject_main.
var private g_exe_emitted_reg : table<string; bool>

// Emit one static-module registration call, DEPENDENCIES FIRST (a module's C++ init may bind externs
// typed against a dependency's handled types, so hash-ordered emission startup-crashes). Deduped via
// reg_visited (recursion), g_exe_emitted_reg (one call process-wide), and get-or-add (avoid LLVM rename).
def private emit_module_registration(m : Module?; dynamic_modules : table<string; bool>;
                                     var emitted_reg, reg_visited : table<string; bool>;
                                     builder : LLVMBuilderRef; g_mod : LLVMOpaqueModule?;
                                     register_mod_type : LLVMTypeRef; var has_cpp_modules : bool&) {
    if (m == null) {
        return
    }
    let mod_name = string(m.name)
    return if (reg_visited |> key_exists(mod_name))
    reg_visited[mod_name] = true
    // dylibs register via the runtime load path; ast_core / network_core / jit are prophylactic-skipped here (a real caller still gets them via ensure_module + needs_whole_lib).
    return if (dynamic_modules |> key_exists(mod_name)
        || mod_name == "ast_core" || mod_name == "network_core" || mod_name == "jit")
    module_for_each_dependency(m) $(var dep : Module?; var _pub : bool) {
        emit_module_registration(dep, dynamic_modules, emitted_reg, reg_visited,
            builder, g_mod, register_mod_type, has_cpp_modules)
    }
    let cpp_name = string(m.cppClassName)
    return if (empty(cpp_name))
    let reg_fn_name = "jit_register_{cpp_name}"
    return if (emitted_reg |> key_exists(reg_fn_name) || g_exe_emitted_reg |> key_exists(reg_fn_name))
    emitted_reg[reg_fn_name] = true
    g_exe_emitted_reg[reg_fn_name] = true
    has_cpp_modules = true
    to_log(LOG_INFO, "LLVM EXE: NEED_MODULE({cpp_name}) for `{mod_name}`\n")
    var reg_fn = LLVMGetNamedFunction(g_mod, reg_fn_name)
    if (reg_fn == null) {
        reg_fn = LLVMAddFunctionWithType(g_mod, reg_fn_name, register_mod_type)
    }
    LLVMBuildCall2(builder, register_mod_type, reg_fn, array<LLVMValueRef>(), "")
}

// Creates main function that initializes a standalone JIT context, registers
// all compiled functions, runs init scripts, then calls the program entry point.
def public inject_main(program_context : Context?; ctx : LLVMContextRef; // nolint:STYLE037,STYLE038 — the standalone-exe entry emitter: one emission block per runtime feature, in startup order
                       prog : Program ?; entry_point : string; mod : LLVMOpaqueModule?;
                       var types : PrimitiveTypes?, var uids : UidNodes?; strict : bool;
                       register_all_modules : bool = false) : tuple<fn : LLVMOpaqueValue?; link_whole_lib : bool> {
    g_exe_emitted_reg |> clear()   // per-codegen-run: one registration call per module thunk
    let builder = LLVMCreateBuilder()
    defer() {
        LLVMDisposeBuilder(builder)
    }
    // Collect ALL used functions from the program (not just JIT-compiled ones from `funcs`).
    // There shouldn't be any `das` functions in standalone_exe, otherwise it
    // will crash.
    var funcs : array<FunctionPtr>
    var has_no_jit = false
    var any_pinvoke = false
    var used_modules : table<string; bool>
    prog |> for_each_module() $(m) {
        if (m.name == "llvm_func" || m.name == "llvm_macro") return
        m |> for_each_function("") $(fun) {
            if (fun.flags.used) {
                if (!fun.flags.builtIn && fun.moreFlags.requestNoJit) {
                    has_no_jit = true
                    to_log(LOG_ERROR, "LLVM EXE: function '{fun.name}' is marked no_jit but standalone exe requires all functions to be JIT-compiled\n")
                }
                if (fun.moreFlags.pinvoke) {
                    any_pinvoke = true
                }
                used_modules[string(fun._module.name)] = true
                funcs |> emplace(fun)
            }
        }
    }
    if (strict && has_no_jit) {
        to_log(LOG_ERROR, "Cannot build standalone exe: some functions are no_jit in strict mode\n")
        return (fn = null, link_whole_lib = false)
    }

    // --jit-register-all-modules: compiler-driver exes may recompile targets needing UnitTest at
    // runtime. Force it only in main()'s dynamic-module loop below — NOT via used_modules, else
    // initialize_modules() emits a second register call ("Module 'UnitTest' already created").
    var force_dynamic_modules : table<string; bool>
    if (register_all_modules) {
        force_dynamic_modules["UnitTest"] = true
    }

    var start_fn_name = ""
    var no_return = true
    var bool_return = false
    for (fn in funcs) {
        if (fn.name == entry_point && fn.arguments.empty()) {
            assume fnmna = uids.get_dll_fn_name(fn).impl()
            if (fn.result.isVoid) {
                start_fn_name = fnmna
                no_return = true
            } elif (fn.result.baseType == Type.tInt) {
                start_fn_name = fnmna
                no_return = false
            } elif (fn.result.baseType == Type.tBool) {
                start_fn_name = fnmna
                no_return = false
                bool_return = true
            }
        }
    }
    if (start_fn_name |> empty()) {
        to_log(LOG_ERROR, "entrypoint `{entry_point}()` not found in input file.\n")
        return (fn = null, link_whole_lib = false)
    }

    let main_fn_type = LLVMFunctionType(types.t_int32,
        fixed_array<LLVMTypeRef>(
            types.t_int32,          // argc
            types.LLVMVoidPtrType() // argv
        )
    )
    // wasm32-emscripten: emcc's libstandalonewasm already defines `main`,
    // emit `__main_argc_argv` instead and let crt1 chain through.
    let main_sym = g_target_is_wasm ? "__main_argc_argv" : "main"
    let main_fn = LLVMAddFunctionWithType(mod, main_sym, main_fn_type)
    let entry = LLVMAppendBasicBlockInContext(ctx, main_fn, "entry")
    LLVMPositionBuilderAtEnd(builder, entry)

    // Collect dynamic module names — used by CollectExternVisitor to skip dlopen'd modules
    var dynamic_modules : table<string; bool>
    for_each_registered_dynamic_module() $(_path, _mod_name, das_name) {
        dynamic_modules[das_name] = true
    }

    // Init static modules — emit register_ClassName() for each used non-dynamic module
    let register_mod_type = LLVMFunctionType(types.LLVMVoidPtrType(), array<LLVMTypeRef>())
    let das_ensure_env_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
    var das_ensure_env = LLVMAddFunctionWithType(g_mod, "das_ensure_environment", das_ensure_env_type)
    LLVMBuildCall2(builder, das_ensure_env_type, das_ensure_env, array<LLVMValueRef>(), "")
    // Dynamic modules may addBuiltinDependency on math during ctor, so the math/strings bundle must
    // exist first. The duplicate call from initialize_modules() is a no-op via Module::require guards.
    // (Symbol is in the COMPILER lib — this flag implies a whole-lib link.)
    if (register_all_modules) {
        var reg_all_main = LLVMGetNamedFunction(g_mod, "jit_register_all_builtin_modules")
        if (reg_all_main == null) {
            reg_all_main = LLVMAddFunctionWithType(g_mod, "jit_register_all_builtin_modules", das_ensure_env_type)
        }
        LLVMBuildCall2(builder, das_ensure_env_type, reg_all_main, array<LLVMValueRef>(), "")
    }
    // A dynamic module's ctor may bind externs typed against ANY builtin handled type (C++ deps the
    // used-functions sweep can't see). So when the program ships a dynamic module, register every
    // builtin C++ module in the graph — except the compiler-lib pair (ast_core/network_core), used-gated.
    var ships_dynamic = false
    prog |> for_each_module() $(m) {
        if (dynamic_modules |> key_exists("{m.name}")) {
            ships_dynamic = true
        }
    }
    if (ships_dynamic) {
        prog |> for_each_module() $(m) {
            let mn = "{m.name}"
            // dynamic modules stay OUT of used_modules (initialize_modules() would double-register
            // → "Module 'X' already created"). jit stays out too — its ctor hard-requires ast_core,
            // which this prophylactic set excludes (SIGTRAP on Debug).
            if (!empty("{m.cppClassName}") && mn != "ast_core" && mn != "network_core"
                    && mn != "jit" && !(dynamic_modules |> key_exists(mn))) {
                used_modules[mn] = true
            }
        }
    }
    // `options rtti` makes handled RTTI types reachable through builtin machinery with no direct
    // CALL into rtti_core, but a dynamic module's C++ init typed against them needs Module_Rtti
    // registered first. The sweep can't see that, so pull rtti_core in whenever rtti is on.
    let wants_rtti = ((prog._options |> find_arg("rtti")) ?as tBool ?? false) || prog.policies.rtti
    if (wants_rtti) {
        used_modules["rtti_core"] = true
    }
    var has_cpp_modules = false
    // register_all_modules already emits jit_register_all_builtin_modules() above (all builtins,
    // idempotent). The sweep would re-emit an UNGUARDED jit_register_Module_X (REGISTER_MODULE_IN_
    // NAMESPACE lacks a Module::require guard) -> double-create at startup ("Module '$' already created").
    if (!register_all_modules) {
        var emitted_reg : table<string; bool>
        var reg_visited : table<string; bool>
        for (mod_name in keys(used_modules)) {
            if (key_exists(dynamic_modules, mod_name)) continue
            prog |> for_each_module() $(m) {
                if (m.name != mod_name) return
                emit_module_registration(m, dynamic_modules, emitted_reg, reg_visited,
                    builder, g_mod, register_mod_type, has_cpp_modules)
            }
        }
    }

    // Register dynamic modules loaded during compilation. The runtime helper tries <exe_dir>/<rel>
    // (release bundles), then <das_root>/<rel> (SDK/dev), then the baked absolute path (legacy).
    let reg_dyn_mod_type = LLVMFunctionType(types.LLVMVoidPtrType(),
        fixed_array<LLVMTypeRef>(
            types.LLVMVoidPtrType(), // const char * rel_path
            types.LLVMVoidPtrType(), // const char * fallback_abs_path
            types.LLVMVoidPtrType()  // const char * mod_name
        )
    )
    var jit_reg_dyn_mod = LLVMAddFunctionWithType(g_mod, "jit_register_dynamic_module_resolve", reg_dyn_mod_type)
    for_each_registered_dynamic_module() $(path, mod_name, das_name) {
        if (key_exists(used_modules, das_name) || key_exists(force_dynamic_modules, das_name)) {
            has_cpp_modules = true
            to_log(LOG_INFO, "LLVM EXE: include dynamic module: `{das_name}`\n")
            let rel = compute_modules_relative_suffix(path)
            let rel_str = get_string_constant_ptr(builder, rel)
            let abs_str = get_string_constant_ptr(builder, path)
            let name_str = get_string_constant_ptr(builder, mod_name)
            LLVMBuildCall2(builder, reg_dyn_mod_type, jit_reg_dyn_mod, fixed_array(rel_str, abs_str, name_str), "")
        }
    }
    // -list-shared-modules <path> side-effect: dump the release-deps JSON
    // for `daspkg release` to ship dylibs + traverse dep packages.
    write_release_deps(prog)

    // Register native paths (das include paths) that were loaded during compilation
    let reg_native_path_type = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(
            types.LLVMVoidPtrType(), // const char * mod_name
            types.LLVMVoidPtrType(), // const char * src_path
            types.LLVMVoidPtrType()  // const char * dst_path
        )
    )
    var jit_reg_native_path = LLVMAddFunctionWithType(g_mod, "jit_register_native_path", reg_native_path_type)
    for_each_registered_native_path() $(mod_name, src_path, dst_path) {
        let mod_str = get_string_constant_ptr(builder, mod_name)
        let src_str = get_string_constant_ptr(builder, src_path)
        let dst_str = get_string_constant_ptr(builder, dst_path)
        LLVMBuildCall2(builder, reg_native_path_type, jit_reg_native_path, fixed_array(mod_str, src_str, dst_str), "")
    }

    // Call initialize_modules() — built by CollectExternVisitor during collect_external_functions
    let init_modules_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
    var init_modules_fn = LLVMGetNamedFunction(g_mod, "initialize_modules")
    if (init_modules_fn == null) {
        init_modules_fn = LLVMAddFunctionWithType(g_mod, "initialize_modules", init_modules_type)
    }
    LLVMBuildCall2(builder, init_modules_type, init_modules_fn, array<LLVMValueRef>(), "")

    // Dynamic modules load Quiet (sibling DT_NEEDED ordering): retry after the last registration
    // and fatal on anything still unloadable, naming the module + dlopen error. Native only — on
    // wasm those modules also register via static thunks, so the failed resolve is BY DESIGN.
    if (ships_dynamic && !g_target_is_wasm) {
        let fin_dyn_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
        var fin_dyn = LLVMAddFunctionWithType(g_mod, "jit_finalize_dynamic_modules", fin_dyn_type)
        LLVMBuildCall2(builder, fin_dyn_type, fin_dyn, array<LLVMValueRef>(), "")
    }

    // Init argc, argv
    let set_cmd_args_type = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(
            types.t_int32,          // argc
            types.LLVMVoidPtrType() // argv
        )
    )
    var jit_set_cmd_args = LLVMAddFunctionWithType(
        g_mod, "jit_set_command_line_arguments", set_cmd_args_type
    )
    let argc = LLVMGetParam(main_fn, 0 |> uint);  // argc
    let argv = LLVMGetParam(main_fn, 1 |> uint);  // argv
    LLVMBuildCall2(builder, set_cmd_args_type,
        jit_set_cmd_args, fixed_array(argc, argv), "")


    // Determine the context stack size from options (mirrors Program::getContextStackSize)
    var context_stack_size = uint64(prog.policies.stack)
    let stack_opt = find_arg(prog._options, "stack")
    if (stack_opt is tInt) {
        context_stack_size = uint64(stack_opt as tInt)
    } elif (stack_opt is tUInt) {
        context_stack_size = uint64(stack_opt as tUInt)
    }

    // Declare jit_create_standalone_ctx
    let create_ctx_type = LLVMFunctionType(types.LLVMVoidPtrType(),
        fixed_array<LLVMTypeRef>(
            types.t_int64,         // totalVariables
            types.t_int64,         // totalFunctions
            types.t_int64,         // globalStringHeapSize
            types.t_int64,         // globalsSize
            types.t_int64,         // sharedSize
            types.t_int1,          // pinvoke
            types.t_int64,         // stackSize
        )
    )
    var jit_create_context = LLVMAddFunctionWithType(
        g_mod, "jit_create_standalone_ctx", create_ctx_type
    )
    var params = fixed_array(
        types.ConstI64(prog.totalVariables |> uint64),
        types.ConstI64(prog.totalFunctions |> uint64),
        types.ConstI64(prog.globalStringHeapSize |> uint64),
        types.ConstI64(get_context_globals_size(program_context)),
        types.ConstI64(get_context_shared_size(program_context)),
        types.ConstI1(any_pinvoke || prog.policies.threadlock_context || prog.policies.debugger),
        types.ConstI64(context_stack_size)
    )
    let global_context = LLVMBuildCall2(builder, create_ctx_type, jit_create_context, params, "")

    // Pre-build init_globals so its function-pointer globals exist before
    // collect_external_functions walks them (issue #2582). Actual init_globals(ctx)
    // call is emitted further down at its runtime position in main_fn.
    var init_glob : LLVMOpaqueValue?
    var init_glob_type : LLVMOpaqueType?
    var clone_init_glob : LLVMOpaqueValue?
    {
        var jit_flags : LlvmJitFlags
        jit_flags.emit_prologue = true
        jit_flags.need_di = false
        let pair = generate_globals_initialization_fn(program_context, prog, types, uids, jit_flags, LlvmJitMode.EXE)
        init_glob = pair._0
        init_glob_type = pair._1
        // Clone variant: inits non-shared globals only (a clone is not the shared owner;
        // re-initializing shared globals would corrupt the parent's shared buffer). Used by
        // the clone init script below so a jobque/thread/audio clone matches the interpreter.
        let cpair = generate_globals_initialization_fn(program_context, prog, types, uids, jit_flags, LlvmJitMode.EXE, "init_globals_clone", true)
        clone_init_glob = cpair._0
    }

    let sort_needs_whole_lib = collect_external_functions(global_context, ctx, builder, mod, types, funcs, uids, used_modules, dynamic_modules, prog, register_all_modules)
    // Determine if exe needs the whole compiler lib (rtti, ast, debugger, jit, sort-with-cblock)
    var needs_whole_lib = sort_needs_whole_lib
    // ast_core / network_core live in the compiler lib, so registering them pulls compiler-only
    // symbols -> whole-lib link. (rtti_core used to be here but its compile builtins moved to ast,
    // so it's now self-contained in the runtime lib.)
    for (mod_name in keys(used_modules)) {
        if (mod_name == "ast_core" || mod_name == "network_core") {
            needs_whole_lib = true
            break
        }
    }
    // jit_register_all_builtin_modules calls register_builtin_modules, which lives only in the
    // compiler lib — so this flag forces a whole-lib link (else "undefined reference" at link).
    if (register_all_modules) {
        needs_whole_lib = true
    }
    // Every .shared_module NEEDs the FULL liblibDaScriptDyn, and dlopen resolves that only from
    // libs already in-process or on the .so's own RUNPATH (never the exe's). A runtime-only exe
    // fails every module dlopen silently, so shipping a dynamic module forces the full-lib link.
    if (ships_dynamic) {
        needs_whole_lib = true
    }

    // Whole-compiler-lib link: register the fusion engine (needed by simulate). Skip on wasm —
    // cross-link only sees libDaScript_runtime.a (no jit_register_fusion), and exes never read it (#2805).
    if (needs_whole_lib && !g_target_is_wasm) {
        let reg_fusion_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
        let reg_fusion_fn = LLVMAddFunctionWithType(g_mod, "jit_register_fusion", reg_fusion_type)
        LLVMBuildCall2(builder, reg_fusion_type, reg_fusion_fn, array<LLVMValueRef>(), "")
    }

    // void jit_register_standalone_variable ( Context * ctx, uint64_t mnh, uint64_t offset )
    let init_global_var_type = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(
            types.LLVMVoidPtrType(),  // Context *
            types.t_int64,            // uint64_t mnh
            types.t_int64,            // uint64_t offset
        )
    )
    var init_global_var = LLVMAddFunctionWithType(
        g_mod, "jit_register_standalone_variable", init_global_var_type
    )
    let vars = program_context.totalVariables
    for (i in range(vars)) {
        var var_params = fixed_array(
            global_context,
            types.ConstI64(get_global_variable_mnh(program_context, i)),
            types.ConstI64(get_global_variable_offset(program_context, i)),
        )
        LLVMBuildCall2(builder, init_global_var_type, init_global_var, var_params, "")
    }


    var main_params = fixed_array<LLVMOpaqueValue?>(
        global_context
    )

    LLVMBuildCall2(builder, init_glob_type, init_glob, main_params, "")

    // Call [init]-annotated functions (e.g. DECS stage registration generated by [decs] macro)
    // for the MAIN context (inline, once).
    for (fn in funcs) {
        if (fn.flags.init) {
            let fn_impl_name = uids.get_dll_fn_name(fn).impl()
            let init_fn = LLVMGetNamedFunction(g_mod, fn_impl_name)
            if (init_fn != null) {
                LLVMBuildCall2(builder, g_fn_types[fn_impl_name], init_fn, main_params, "")
            }
        }
    }

    // Register a combined init script (init_globals + [init] functions) as jitInitScript, so a
    // CLONED context (jobque worker, thread, dasAudio mixer) re-runs the FULL init like the
    // interpreter's runInitScript. The main context ran both inline above and never calls it (no double-run).
    {
        let init_script_fn_type = LLVMFunctionType(types.t_void,
            fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType()))  // Context *
        let init_script_fn = LLVMAddFunctionWithType(g_mod, "__das_clone_init_script", init_script_fn_type)
        let isb = LLVMCreateBuilder()
        let isentry = LLVMAppendBasicBlockInContext(ctx, init_script_fn, "entry")
        LLVMPositionBuilderAtEnd(isb, isentry)
        let isctx = LLVMGetParam(init_script_fn, 0u)
        // init_globals_clone: skips shared globals so the clone doesn't corrupt the
        // owner's shared buffer (matches the interpreter's sharedOwner gate).
        LLVMBuildCall2(isb, init_glob_type, clone_init_glob, fixed_array(isctx), "")
        for (fn in funcs) {
            if (fn.flags.init) {
                let fn_impl_name = uids.get_dll_fn_name(fn).impl()
                let init_fn = LLVMGetNamedFunction(g_mod, fn_impl_name)
                if (init_fn != null) {
                    LLVMBuildCall2(isb, g_fn_types[fn_impl_name], init_fn, fixed_array(isctx), "")
                }
            }
        }
        LLVMBuildRetVoid(isb)
        LLVMDisposeBuilder(isb)

        let set_init_script_type = LLVMFunctionType(types.t_void,
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // Context *
                types.LLVMVoidPtrType(),  // JitInitScriptFn
            )
        )
        let jit_set_init_script = LLVMAddFunctionWithType(g_mod, "jit_set_init_script", set_init_script_type)
        LLVMBuildCall2(builder, set_init_script_type, jit_set_init_script,
            fixed_array(global_context, LLVMBuildPointerCast(builder, init_script_fn, types.LLVMVoidPtrType(), "")), "")
    }

    // Match the interpreter's WebLoop: on wasm a program exporting `update` runs as init() once +
    // update() per browser frame + shutdown() (the browser can't block in main's while-loop). `main`
    // stays the desktop driver, bypassed here — so the same .das cross-compiles unchanged for web.
    if (g_target_is_wasm) {
        var update_impl = ""
        var update_returns_value = false
        var update_count = 0
        var init_impl = ""
        var shutdown_impl = ""
        for (fn in funcs) {
            if (!fn.arguments.empty()) {
                continue
            }
            let is_void = fn.result.isVoid
            let is_val = fn.result.baseType == Type.tInt || fn.result.baseType == Type.tBool
            if (!is_void && !is_val) {
                continue
            }
            if (fn.name == "update") {
                update_impl = uids.get_dll_fn_name(fn).impl()
                update_returns_value = is_val
                update_count++
            } elif (fn.name == "init") {
                init_impl = uids.get_dll_fn_name(fn).impl()
            } elif (fn.name == "shutdown") {
                shutdown_impl = uids.get_dll_fn_name(fn).impl()
            }
        }
        if (update_count == 1 && LLVMGetNamedFunction(g_mod, update_impl) != null) {
            if (!empty(init_impl)) {
                let init_fn = LLVMGetNamedFunction(g_mod, init_impl)
                if (init_fn != null) {
                    LLVMBuildCall2(builder, g_fn_types[init_impl], init_fn, main_params, "")
                }
            }
            // void jit_run_web_lifecycle ( Context *, void * updateFn, int32_t updateReturnsValue, void * shutdownFn )
            let lifecycle_type = LLVMFunctionType(types.t_void,
                fixed_array<LLVMTypeRef>(
                    types.LLVMVoidPtrType(),  // Context *
                    types.LLVMVoidPtrType(),  // update fn ptr
                    types.t_int32,            // updateReturnsValue
                    types.LLVMVoidPtrType()   // shutdown fn ptr (nullable)
                )
            )
            var lifecycle_fn = LLVMAddFunctionWithType(g_mod, "jit_run_web_lifecycle", lifecycle_type)
            let update_ptr = LLVMBuildPointerCast(builder, LLVMGetNamedFunction(g_mod, update_impl), types.LLVMVoidPtrType(), "")
            var shutdown_ptr = LLVMConstNull(types.LLVMVoidPtrType())
            if (!empty(shutdown_impl)) {
                let sfn = LLVMGetNamedFunction(g_mod, shutdown_impl)
                if (sfn != null) {
                    shutdown_ptr = LLVMBuildPointerCast(builder, sfn, types.LLVMVoidPtrType(), "")
                }
            }
            let rv = update_returns_value ? 1 : 0
            LLVMBuildCall2(builder, lifecycle_type, lifecycle_fn,
                fixed_array(global_context, update_ptr, types.ConstI32(uint64(rv)), shutdown_ptr), "")
            // jit_run_web_lifecycle never returns (simulate_infinite_loop); the block
            // still needs a terminator, so ret 0 (unreachable in practice).
            LLVMBuildRet(builder, types.ConstI32(uint64(0)))
            to_log(LOG_INFO, "LLVM EXE: wasm browser lifecycle installed (init/update/shutdown); `main` bypassed\n")
            return (fn = main_fn, link_whole_lib = needs_whole_lib)
        }
    }

    // Route main through the runtime's catch boundary (jit_run_main_guarded) — a das panic must
    // PRINT and exit nonzero, not unwind out of the bare entry with no message. The guard owns
    // the void/int/bool result mapping (0/1/2).
    let guard_type = LLVMFunctionType(types.t_int32,
        fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(), types.LLVMVoidPtrType(), types.t_int32))
    var guard_fn = LLVMGetNamedFunction(g_mod, "jit_run_main_guarded")
    if (guard_fn == null) {
        guard_fn = LLVMAddFunctionWithType(g_mod, "jit_run_main_guarded", guard_type)
    }
    let guard_kind = no_return ? 0 : (bool_return ? 2 : 1)
    let start_ptr = LLVMBuildPointerCast(builder, LLVMGetNamedFunction(g_mod, start_fn_name), types.LLVMVoidPtrType(), "")
    var guard_params <- fixed_array(global_context, start_ptr, types.ConstI32(uint64(guard_kind)))
    let main_ret = LLVMBuildCall2(builder, guard_type, guard_fn, guard_params, "main_rc")
    // Drain debug agents, modules, AOT library, env — the interpreter's teardown minus the handle-leak
    // dump. Without it the static g_DebugAgents map dtor races ref_count_mutex at exit (#2583).
    let shutdown_fn_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
    var shutdown_fn = LLVMGetNamedFunction(g_mod, "jit_shutdown")
    if (shutdown_fn == null) {
        shutdown_fn = LLVMAddFunctionWithType(g_mod, "jit_shutdown", shutdown_fn_type)
    }
    LLVMBuildCall2(builder, shutdown_fn_type, shutdown_fn, array<LLVMValueRef>(), "")
    LLVMBuildRet(builder, main_ret)
    to_log(LOG_INFO, "LLVM JIT: standalone exe entry point generated '{start_fn_name}', {length(funcs)} functions\n")
    return (fn = main_fn, link_whole_lib = needs_whole_lib)
}
