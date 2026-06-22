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
require daslib/json_boost
require jit
require daslib/fio

module llvm_exe shared private

bitfield TabOperation {
    at,
    find,
    erase
}

class CollectExternVisitor : AstVisitor {
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
    trap_typ : LLVMOpaqueType?
    trap_ptr : LLVMOpaqueValue?
    reg_standalone_fn_type : LLVMOpaqueType?
    reg_standalone_fn : LLVMOpaqueValue?
    builder : LLVMBuilderRef
    ctx : LLVMContextRef
    mod : LLVMOpaqueModule?
    types : PrimitiveTypes -const? -const
    seen : table<string; bool>
    any_unimplemented : bool
    needs_whole_lib : bool
    extern_modules : table<string; bool>
    registered_modules : table<string; bool>
    dynamic_modules : table<string; bool>
    register_mod_type : LLVMOpaqueType?
    init_fn : LLVMOpaqueValue?
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


    def CollectExternVisitor(standalone_context : LLVMOpaqueValue?; _ctx : LLVMContextRef;
                             _builder : LLVMBuilderRef,
                             _mod : LLVMOpaqueModule?; var _types : PrimitiveTypes?,
                             uids : UidNodes?; register_all : bool) {
        any_unimplemented = false
        needs_whole_lib = false
        uid := uids
        ctx = _ctx
        standalone_ctx = standalone_context
        mod = _mod
        types = _types
        builder = _builder
        register_mod_type = LLVMFunctionType(types.LLVMVoidPtrType(), array<LLVMTypeRef>())

        // Create initialize_modules() function — filled incrementally as we discover needed modules
        let init_fn_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
        init_fn = LLVMGetNamedFunction(g_mod, "initialize_modules")
        let init_entry = LLVMAppendBasicBlockInContext(ctx, init_fn, "entry")
        init_builder = LLVMCreateBuilder()
        LLVMPositionBuilderAtEnd(init_builder, init_entry)
        // Always call das_ensure_environment first
        let env_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
        var env_fn = LLVMGetNamedFunction(g_mod, "das_ensure_environment")
        if (env_fn == null) {
            env_fn = LLVMAddFunctionWithType(g_mod, "das_ensure_environment", env_type)
        }
        LLVMBuildCall2(init_builder, env_type, env_fn, array<LLVMValueRef>(), "")
        // Optionally register ALL builtin native modules (math, fio, dasbind, ...)
        // so a self-contained compiler-driver exe can recompile arbitrary daslang
        // at runtime. Referencing this symbol also force-links those modules in.
        // register_builtin_modules() covers exactly the set below, so mark them as
        // already-registered to suppress the per-module emission (otherwise each
        // would be registered twice -> "Module '...' already created").
        if (register_all) {
            var reg_all = LLVMGetNamedFunction(g_mod, "jit_register_all_builtin_modules")
            if (reg_all == null) {
                reg_all = LLVMAddFunctionWithType(g_mod, "jit_register_all_builtin_modules", env_type)
            }
            LLVMBuildCall2(init_builder, env_type, reg_all, array<LLVMValueRef>(), "")
            for (m in fixed_array("$", "math", "strings", "rtti_core", "ast_core", "jit",
                                  "debugapi", "network_core", "uriparser", "jobque",
                                  "fio_core", "dasbind")) {
                registered_modules[m] = true
            }
        } else {
            // Always register Module_BuiltIn ($) and Module_Strings (strings)
            var reg_builtin = LLVMAddFunctionWithType(g_mod, "jit_register_Module_BuiltIn", register_mod_type)
            LLVMBuildCall2(init_builder, register_mod_type, reg_builtin, array<LLVMValueRef>(), "")
            registered_modules["$"] = true
            var reg_strings = LLVMAddFunctionWithType(g_mod, "jit_register_Module_Strings", register_mod_type)
            LLVMBuildCall2(init_builder, register_mod_type, reg_strings, array<LLVMValueRef>(), "")
            registered_modules["strings"] = true
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

        // void * get_jit_string_table_at_with_hash ( ) — the string at routes through this
        // precomputed-hash global (grow fallback), so the standalone exe bakes it. String find is
        // fully inline (no glob).
        reg_extern_tab_wh_type = LLVMFunctionType(types.LLVMVoidPtrType(), [])
        tab_at_wh_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_string_table_at_with_hash", reg_extern_tab_wh_type)

        // String insert-after-packed-miss global (the inline packed string find calls it on a
        // miss; nullary accessor). Without baking it in the exe, the glob keeps its compile-process
        // address and faults under ASLR (works on Windows only because the DLL loads at the same base).
        tab_at_apm_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_string_table_at_after_packed_miss", reg_extern_tab_wh_type)

        trap_typ = LLVMFunctionType(types.t_void, [])
        trap_ptr = LLVMAddFunctionWithType(g_mod, "jit_trap", trap_typ)

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
        if (m == null) return
        let mod_name = string(m.name)
        if (key_exists(registered_modules, mod_name) || key_exists(dynamic_modules, mod_name)) return
        registered_modules[mod_name] = true
        // First ensure all dependencies of this module are registered
        module_for_each_dependency(m) $(var dep : Module?; var _pub : bool) {
            ensure_module(dep)
        }
        let cpp_name = string(m.cppClassName)
        if (empty(cpp_name)) return
        let reg_fn_name = "jit_register_{cpp_name}"
        to_log(LOG_INFO, "LLVM EXE: NEED_MODULE({cpp_name}) for `{mod_name}`\n")
        var reg_fn = LLVMAddFunctionWithType(g_mod, reg_fn_name, register_mod_type)
        LLVMBuildCall2(init_builder, register_mod_type, reg_fn, array<LLVMValueRef>(), "")
    }

    def make_call(expr : ExprCallFunc?) {
        if (expr.func == null || has_intrinsic(expr)) return
        if (expr.func.flags.builtIn) {
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (FUNC_PTR == null) return
            let name = uid.get_dll_fn_name_ptr(expr.func)
            if (initialize(name)) {
                // For each used extern builtin function, emit a call to a registration
                // function that will assign to the global variable pointer to a function
                // at startup.
                let global_var = LLVMGetNamedGlobal(g_mod, name.glob())
                // JIT may not emit some globals due to optimizations.
                // Now we have it only for `count`, see:
                // d901e741b164f6328660178e8ed3ae406d78c8e1.
                // Simplest way is to skip global if it wasn't created, which
                // means it was never accessed.
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

    def add_unimplemented(name : DllName; expr : ExpressionPtr) {
        // Trap will not always help. At least try.
        to_log(LOG_WARNING, "LLVM JIT: Emit trap for {name.glob()} at {describe(expr.at)}.\n")
        let global_func_ptr = LLVMGetNamedGlobal(g_mod, name.glob());
        LLVMSetInitializer(global_func_ptr, trap_ptr)
        any_unimplemented = true
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
                // the handle gate lives on the chain element (the head is tFixedArray);
                // borrow-walk — this pass runs outside ast_gc_guard, clones would leak
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

    def override preVisitExprMakeBlock(expr : ExprMakeBlock?) : void {
        let annotationData = (expr._block as ExprBlock).annotationData
        if (annotationData != 0 |> uint64()) {
            add_unimplemented(uid.get_block_ann(expr), expr)
            // Apparently annotationData cannot be `nonnull` for
            // regular das program. And this Visitor called only on regular das.
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
            // string keys route through the precomputed-hash at (see build_table_at); bake that
            // glob plus its after-packed-miss insert helper
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
        // String + workhorse find are fully inline (no glob to bake). The per-type C++ find glob is
        // only created when build_table_find_call codegens a non-workhorse find (vectors / structs),
        // so when GetNamedGlobal misses there is no consumer and nothing to bake.
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

def collect_external_functions(standalone_context : LLVMOpaqueValue?; ctx : LLVMContextRef; builder : LLVMBuilderRef,
                               mod : LLVMOpaqueModule?; var types : PrimitiveTypes?;
                               var funcs : array<FunctionPtr>; var uids : UidNodes?;
                               var used_modules : table<string; bool>&;
                               dynamic_modules : table<string; bool>;
                               prog : Program?; register_all_modules : bool = false) : tuple<any_unimplemented : bool; needs_whole_lib : bool> {
    var extern_resolver = new CollectExternVisitor(standalone_context, ctx, builder, mod, types, uids, register_all_modules)
    extern_resolver.dynamic_modules := dynamic_modules
    make_visitor(*extern_resolver) $(adapter_resolve) {
        ast_gc_guard() {
            for (fn in funcs) {
                visit(fn, adapter_resolve)
            }
            // Register builtins reachable only from top-level `let X = builtin()`
            // (issue #2582). init_globals must be JIT-compiled first so the
            // function-pointer globals exist — make_call bails on missing globals.
            prog |> for_each_module() $(m) {
                if (m.moduleFlags.builtIn) return ;
                m |> for_each_global() $(var glob) {
                    if (glob.init != null && glob.flags.used) {
                        visit_expression(glob.init, adapter_resolve)
                    }
                }
            }
        }
        // Register [pinvoke] and [export] functions in tabMnLookup so they can be
        // found by name at runtime (e.g. via invoke_in_context).
        for (fn in funcs) {
            if (!fn.flags.builtIn && (fn.moreFlags.pinvoke || fn.flags.exports)) {
                extern_resolver.uid.reset(fn)
                extern_resolver.register_function(fn)
                extern_resolver.uid.reset(null)
            }
        }
    }
    let any_unimp = extern_resolver.any_unimplemented
    let whole_lib = extern_resolver.needs_whole_lib
    for (mod_name in keys(extern_resolver.extern_modules)) {
        used_modules[mod_name] = true
    }
    // Finalize initialize_modules(): emit dynamic modules, native paths, Module::Initialize, ret.
    // Use the same 3-tier resolve helper as inject_main so a standalone exe doesn't
    // double-register the same module from two different filesystem paths (the
    // resolved bundle path here AND a baked absolute somewhere else).
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
    LLVMBuildRetVoid(ib)
    LLVMDisposeBuilder(ib)
    extern_resolver.uid = null
    extern_resolver.types = null
    unsafe { delete extern_resolver; }
    return (any_unimplemented = any_unimp, needs_whole_lib = whole_lib)
}

// Suffix from the last "modules/" segment onwards; "" → caller falls back to abs path.
// to_generic_path normalizes Windows backslashes so the search needs only one form.
def private compute_modules_relative_suffix(p : string) : string {
    let pn = to_generic_path(p)
    let idx = rfind(pn, "/modules/")
    return idx < 0 ? "" : slice(pn, idx + 1)
}

// Walk up `start_dir` looking for a `.das_package` file. Returns the absolute
// path to the package (the dir containing `.das_package`) or "" if no ancestor
// has one within MAX_LEVELS. Bounded so a stray .das at filesystem root doesn't
// scan the whole tree. Used to identify daspkg-package boundaries among the
// program's .das modules.
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

struct private ReleaseDeps {
    shared_modules : array<SharedModuleEntry>
    das_modules : array<DasModuleEntry>
}

// If `-list-shared-modules <path>` was passed on the daslang command line,
// write a JSON `ReleaseDeps` describing what `daspkg release` should ship.
//
// Detection walks `program_for_each_module(prog)` and, for each program module:
//   1) Looks the daslang module name up in `for_each_registered_dynamic_module()`
//      to find an associated .shared_module dylib. Found → emit a SharedModuleEntry,
//      deduped by absolute dylib path (one .shared_module can host several
//      daslang modules, e.g. dasStbImage hosts `stbimage`, `raster`, `stbtruetype`).
//   2) If `m.fileName` is non-empty AND its directory walks up to a `.das_package`
//      file (via `find_enclosing_das_package`), emits a DasModuleEntry tagged with
//      `package_dir`. This filter drops daslib stdlib (no .das_package above) and
//      SDK-shipped modules under `<das_root>/modules/<X>/` (no .das_package either),
//      keeping only daspkg-installed deps. The reader (`cmd_release`) further skips
//      entries whose `package_dir` equals the project root being released.
// Extract the package directory name from a "modules/<X>/.../foo.shared_module"
// suffix — used to let `-force-shared-module dasGlfw` match by package name as
// well as by the daslang-side module name.
def private dylib_package_name(rel : string) : string {
    if (empty(rel)) return ""
    let after_modules = slice(rel, length("modules/"))
    let next_slash = find(after_modules, "/")
    return next_slash > 0 ? slice(after_modules, 0, next_slash) : after_modules
}

// Helper: append `entry` to deps.shared_modules unless its absolute path was
// already recorded. Deduping by absolute dylib path is necessary because one
// .shared_module file can host several daslang modules (dasStbImage hosts
// stbimage, raster, stbtruetype) and each would otherwise produce a duplicate.
def private push_shared_unique(entry : SharedModuleEntry; var seen : table<string; bool>; var deps : ReleaseDeps) {
    if (key_exists(seen, entry.abs)) return
    seen[entry.abs] = true
    deps.shared_modules |> push(entry)
}

// Declarative parsing of daslang's release-deps flags. The fields here are
// matched against argv directly (clargs auto-derives `--field-name` long
// flags); unknown flags in argv (every other daslang option) are silently
// ignored by parse_args.
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
    // Snapshot the dynamic-module registry once; key by daslang-side name, with
    // a parallel by-package-name lookup so `--force-shared-module dasGlfw` works
    // alongside `--force-shared-module dasglfw` (the daslang-side name).
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
    // User-forced (release_shared_module(name) → --force-shared-module <name>).
    // Match by daslang-side name OR package directory name. Lookup happens here
    // (in the daslang process compiling the user program) so the registry has
    // every dylib daslang scanned, not just the ones daspkg itself uses.
    for (name in cfg.force_shared_module) {
        if (key_exists(by_das, name)) {
            push_shared_unique(by_das[name], shared_seen, deps)
        } elif (key_exists(by_pkg, name)) {
            push_shared_unique(by_pkg[name], shared_seen, deps)
        } else {
            to_log(LOG_WARNING, "LLVM EXE: --force-shared-module `{name}` not registered (typo? not loaded by daslang)\n")
        }
    }
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

// Creates main function that initializes a standalone JIT context, registers
// all compiled functions, runs init scripts, then calls the program entry point.
def public inject_main(program_context : Context?; ctx : LLVMContextRef;
                       prog : Program?; entry_point : string; mod : LLVMOpaqueModule?;
                       var types : PrimitiveTypes?, var uids : UidNodes?; strict : bool;
                       register_all_modules : bool = false) : tuple<fn : LLVMOpaqueValue?; link_whole_lib : bool> {

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

    // --jit-register-all-modules: compiler-driver exes (e.g. utils/jit) may
    // recompile targets that require UnitTest at runtime. It's a dynamic module
    // the -exe target itself doesn't reference, so neither used_modules nor
    // extern_modules picks it up. Force the registration only in main()'s
    // dynamic-module loop (below) — NOT via used_modules, otherwise
    // initialize_modules() emits a second jit_register_dynamic_module_resolve
    // call → "Module 'UnitTest' already created" at runtime.
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
    // Dynamic modules (e.g. UnitTest) may declare addBuiltinDependency on math
    // during ctor, so the math/strings/... bundle must exist before any dynamic
    // module loads. Emit jit_register_all_builtin_modules here; the duplicate
    // call from initialize_modules() is a no-op via Module::require guards.
    if (register_all_modules) {
        var reg_all_main = LLVMGetNamedFunction(g_mod, "jit_register_all_builtin_modules")
        if (reg_all_main == null) {
            reg_all_main = LLVMAddFunctionWithType(g_mod, "jit_register_all_builtin_modules", das_ensure_env_type)
        }
        LLVMBuildCall2(builder, das_ensure_env_type, reg_all_main, array<LLVMValueRef>(), "")
    }
    var has_cpp_modules = false
    for (mod_name in keys(used_modules)) {
        if (key_exists(dynamic_modules, mod_name)) continue
        prog |> for_each_module() $(m) {
            if (m.name != mod_name) return
            if (!empty(m.cppClassName)) {
                has_cpp_modules = true
                let reg_fn_name = "jit_register_{m.cppClassName}"
                to_log(LOG_INFO, "LLVM EXE: NEED_MODULE({m.cppClassName}) for `{m.name}`\n")
                var reg_fn = LLVMAddFunctionWithType(g_mod, reg_fn_name, register_mod_type)
                LLVMBuildCall2(builder, register_mod_type, reg_fn, array<LLVMValueRef>(), "")
            }
        }
    }

    // Register dynamic modules that were loaded during compilation. The runtime
    // helper tries <exe_dir>/<rel_path> (daspkg release bundles), then
    // <das_root>/<rel_path> (SDK install / local dev), then the baked absolute
    // path (legacy fallback).
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
    {
        var jit_flags : LlvmJitFlags
        jit_flags.emit_prologue = true
        jit_flags.gen_exe = true
        jit_flags.need_di = false
        let pair = generate_globals_initialization_fn(program_context, prog, types, uids, jit_flags)
        init_glob = pair._0
        init_glob_type = pair._1
    }

    let (any_unimplemented, sort_needs_whole_lib) = collect_external_functions(global_context, ctx, builder, mod, types, funcs, uids, used_modules, dynamic_modules, prog, register_all_modules)
    if (strict && any_unimplemented) {
        to_log(LOG_ERROR, "Some of the nodes above are unimplemented!\n")
        return (fn = null, link_whole_lib = false)
    }
    // Determine if exe needs the whole compiler lib (rtti, ast, debugger, jit, sort-with-cblock)
    var needs_whole_lib = sort_needs_whole_lib
    for (mod_name in keys(used_modules)) {
        if (mod_name == "rtti_core") {
            needs_whole_lib = true
            break
        }
    }
    // jit_register_all_builtin_modules lives in libDaScriptDyn_runtime but calls
    // register_builtin_modules which only exists in libDaScriptDyn (compiler lib).
    // Without the compiler lib, the standalone exe link fails with
    //   "undefined reference to `das::register_builtin_modules()`".
    if (register_all_modules) {
        needs_whole_lib = true
    }

    // When linking whole compiler lib, register fusion engine (needed by simulate).
    // Skip on wasm: cross-link only sees libDaScript_runtime.a (no
    // jit_register_fusion symbol), and standalone exes never read the pointers
    // it sets anyway — ShutdownStandalone passes resetFusion=false, see #2805.
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

    {
        let init_globals = LLVMBuildCall2(builder, init_glob_type, init_glob, main_params, "")
        // Register init_globals as jitInitScript so cloned contexts (jobque) can re-run it
        let set_init_script_type = LLVMFunctionType(types.t_void,
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // Context *
                types.LLVMVoidPtrType(),  // JitInitScriptFn
            )
        )
        let jit_set_init_script = LLVMAddFunctionWithType(g_mod, "jit_set_init_script", set_init_script_type)
        LLVMBuildCall2(builder, set_init_script_type, jit_set_init_script,
            fixed_array(global_context, LLVMBuildPointerCast(builder, init_glob, types.LLVMVoidPtrType(), "")), "")
    }

    // Call [init]-annotated functions (e.g. DECS stage registration generated by [decs] macro)
    for (fn in funcs) {
        if (fn.flags.init) {
            let fn_impl_name = uids.get_dll_fn_name(fn).impl()
            let init_fn = LLVMGetNamedFunction(g_mod, fn_impl_name)
            if (init_fn != null) {
                LLVMBuildCall2(builder, g_fn_types[fn_impl_name], init_fn, main_params, "")
            }
        }
    }

    let main_ret = LLVMBuildCall2(builder, g_fn_types[start_fn_name], LLVMGetNamedFunction(g_mod, start_fn_name), main_params, "")
    // Drain debug agents, modules, AOT library, env — same teardown the interpreter
    // does, minus the handle-leak dump (shipped binaries should be quiet by default).
    // Without this the static g_DebugAgents map dtor races ref_count_mutex during
    // __cxa_finalize_ranges (issue #2583).
    let shutdown_fn_type = LLVMFunctionType(types.t_void, array<LLVMTypeRef>())
    var shutdown_fn = LLVMGetNamedFunction(g_mod, "jit_shutdown")
    if (shutdown_fn == null) {
        shutdown_fn = LLVMAddFunctionWithType(g_mod, "jit_shutdown", shutdown_fn_type)
    }
    LLVMBuildCall2(builder, shutdown_fn_type, shutdown_fn, array<LLVMValueRef>(), "")
    if (no_return) {
        LLVMBuildRet(builder, types.ConstI32(uint64(0)))
    } elif (bool_return) {
        // bool main → exit code: true → 0 (success), false → 1 (failure).
        let exit_code = LLVMBuildSelect(builder, main_ret,
            types.ConstI32(uint64(0)), types.ConstI32(uint64(1)), "main_exit")
        LLVMBuildRet(builder, exit_code)
    } else {
        LLVMBuildRet(builder, main_ret)
    }
    to_log(LOG_INFO, "LLVM JIT: standalone exe entry point generated '{start_fn_name}', {length(funcs)} functions\n")
    return (fn = main_fn, link_whole_lib = needs_whole_lib)
}
