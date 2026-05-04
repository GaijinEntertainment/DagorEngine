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
require daslib/debugger
require math
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
    reg_extern_tab_type : LLVMOpaqueType?
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
                             uids : UidNodes?) {
        any_unimplemented = false
        uid := uids
        ctx = _ctx
        standalone_ctx = standalone_context
        mod = _mod
        types = _types
        builder = _builder

        reg_extern_tab_type = LLVMFunctionType(types.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(
                types.t_int32,            // type
            )
        )
        tab_at_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_table_at", reg_extern_tab_type)
        tab_find_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_table_find", reg_extern_tab_type)
        tab_erase_accessor = LLVMAddFunctionWithType(g_mod, "get_jit_table_erase", reg_extern_tab_type)

        trap_typ = LLVMFunctionType(types.t_void, [])
        trap_ptr = LLVMAddFunctionWithType(g_mod, "jit_trap", trap_typ)

        reg_extern_fn_type = LLVMFunctionType(types.t_void,
            fixed_array<LLVMTypeRef>(
                types.LLVMVoidPtrType(),  // const char* module_name
                types.LLVMVoidPtrType(),  // const char* mangled_name
                types.LLVMVoidPtrType(),  // void**      ptr  (address of the function-pointer global)
            )
        )
        reg_extern_fn = LLVMAddFunctionWithType(g_mod, "jit_init_extern_function", reg_extern_fn_type)

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
            )
        )
        reg_standalone_fn = LLVMAddFunctionWithType(g_mod, "jit_register_standalone_function", reg_standalone_fn_type)
    }

    def override preVisitFunction(var fun : FunctionPtr) : void {
        uid.reset(get_ptr(fun))
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

    def make_call(expr : smart_ptr<ExprCallFunc>) {
        if (expr.func == null) { return ; }
        if (has_intrinsic(expr)) { return ; }
        if (expr.func.flags.builtIn) {
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (FUNC_PTR == null) { return ; }
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
                if (global_var == null) {
                    return
                }
                var call_args = array<LLVMValueRef>(
                    get_string_constant_ptr(builder, string(expr.func._module.name)),
                    get_string_constant_ptr(builder, get_mangled_name(expr.func)),
                    LLVMBuildPointerCast(builder, global_var, types.LLVMVoidPtrType(), ""),
                )
                LLVMBuildCall2(builder, reg_extern_fn_type, reg_extern_fn, call_args, "")
            }
        }
    }

    def get_annotation(ann : TypeAnnotation?) {
        var call_args = array<LLVMValueRef>(
            get_string_constant_ptr(builder, string(ann._module.name)),
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

    def override preVisitExprCall(expr : smart_ptr<ExprCall>) {
        make_call(expr)
    }

    def override preVisitExprOp2(var expr : smart_ptr<ExprOp2>) {
        if (isExprOp2_Func(expr)) {
            make_call(expr)
        }
    }

    def override preVisitExprOp1(var expr : smart_ptr<ExprOp1>) {
        if (isExprOp1_Func(expr)) {
            make_call(expr)
        }
    }

    def override preVisitExprNew(expr : smart_ptr<ExprNew>) {
        if (expr.typeexpr.isHandle) {
            let name = uid.get_new(expr.typeexpr)
            if (initialize(name)) {
                register_new_from_annotation(name, expr.typeexpr.annotation)
            }
        } else {
            if (expr.typeexpr.dim |> length > 0) {
                if (expr.typeexpr.baseType == Type.tHandle) {
                    var inscope elemT = clone_type(expr.typeexpr)
                    elemT.dim |> clear() // hack to get correct ptr to ctor
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

    def override preVisitExprDelete(var expr : smart_ptr<ExprDelete>) {
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

    def override preVisitExprClone(expr : smart_ptr<ExprClone>) : void {
        assume leftT = expr.left._type
        if (leftT.isHandle) {
            let name = uid.get_clone_type(leftT)
            if (initialize(name)) {
                register_clone_from_annotation(name, leftT.annotation)
            }
        }
    }

    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) {
        let fn = expr.func

        let fnmna = uid.get_dll_fn_name_ptr(expr.func)
        let fn_publ = LLVMGetNamedFunction(g_mod, fnmna.publ())
        let fn_ptr_cast = LLVMBuildPointerCast(builder, fn_publ, types.LLVMVoidPtrType(), "")
        var reg_args = fixed_array<LLVMOpaqueValue?>(
            standalone_ctx,
            types.ConstI64(fn.index |> uint64),
            get_string_constant_ptr(builder, string(fn.name)),
            get_string_constant_ptr(builder, string(fn |> get_mangled_name())),
            types.ConstI64(fn.getMangledNameHash),
            types.ConstI32(fn.totalStackSize |> uint64),
            fn_ptr_cast,
            types.ConstI1(isCMRES(fn)),
            types.ConstI1(fn.flags.fastCall),
            types.ConstI1(fn.moreFlags.pinvoke),
        )
        let fn_ptr = LLVMBuildCall2(builder, reg_standalone_fn_type, reg_standalone_fn, reg_args, "")

        let name = uid.get_addr(expr)
        let global_var = LLVMGetNamedGlobal(mod, name.glob())
        LLVMBuildStore(builder, fn_ptr, global_var)
    }

    def override preVisitExprMakeStructIndex(expr : smart_ptr<ExprMakeStruct>; index : int; last : bool) : void {
        if (expr.constructor != null && !expr.constructor.moreFlags.requestJit) {
            let fn = expr.constructor
            let fnmna = uid.get_dll_fn_name_ptr(fn)
            let fn_publ = LLVMGetNamedFunction(g_mod, fnmna.publ())
            let fn_ptr_cast = LLVMBuildPointerCast(builder, fn_publ, types.LLVMVoidPtrType(), "")
            var reg_args = fixed_array<LLVMOpaqueValue?>(
                standalone_ctx,
                types.ConstI64(fn.index |> uint64),
                get_string_constant_ptr(builder, string(fn.name)),
                get_string_constant_ptr(builder, string(fn |> get_mangled_name())),
                types.ConstI64(fn.getMangledNameHash),
                types.ConstI32(fn.totalStackSize |> uint64),
                fn_ptr_cast,
                types.ConstI1(isCMRES(fn)),
                types.ConstI1(fn.flags.fastCall),
                types.ConstI1(fn.moreFlags.pinvoke),
            )
            LLVMBuildCall2(builder, reg_standalone_fn_type, reg_standalone_fn, reg_args, "")
        }
    }

    def override preVisitExprMakeBlock(expr : smart_ptr<ExprMakeBlock>) : void {
        let annotationData = (expr._block as ExprBlock).annotationData
        if (annotationData != 0 |> uint64()) {
            add_unimplemented(uid.get_block_ann(expr), expr)
            // Apparently annotationData cannot be `nonnull` for
            // regular das program. And this Visitor called only on regular das.
        }
    }

    def private get_table_t(subexprT : TypeDeclPtr) {
        assume firstT = subexprT.firstType
        var keyType : Type
        if (firstT.isWorkhorseType) {
            return firstT.baseType
        } else {
            var inscope uvt <- get_underlying_value_type(firstT)
            return uvt.baseType
        }
    }

    def add_table_at_call(index_t : TypeDeclPtr) {
        let typ = get_table_t(index_t)
        var call_args = array<LLVMValueRef>(types.ConstI32(uint64(typ)))
        let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_type, tab_at_accessor, call_args, "")

        let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_TABLE_AT(typ)).glob())
        LLVMBuildStore(builder, tab_ptr, tab_method)
    }

    def add_table_find_call(index_t : TypeDeclPtr) {
        let typ = get_table_t(index_t)
        var call_args = array<LLVMValueRef>(types.ConstI32(uint64(typ)))
        let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_type, tab_find_accessor, call_args, "")

        let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_TABLE_FIND(typ)).glob())
        LLVMBuildStore(builder, tab_ptr, tab_method)
    }

    def add_table_erase_call(index_t : TypeDeclPtr) {
        let typ = get_table_t(index_t)
        var call_args = array<LLVMValueRef>(types.ConstI32(uint64(typ)))
        let tab_ptr = LLVMBuildCall2(builder, reg_extern_tab_type, tab_erase_accessor, call_args, "")

        let tab_method = LLVMGetNamedGlobal(g_mod, DllName(FN_JIT_TABLE_ERASE(typ)).glob())
        LLVMBuildStore(builder, tab_ptr, tab_method)
    }

    def override preVisitExprAt(expr : smart_ptr<ExprAt>) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType) {
            add_table_at_call(subExprT)
        }
    }

    def override preVisitExprSetInsert(expr : smart_ptr<ExprSetInsert>) {
        add_table_at_call(expr.arguments[0]._type)
    }

    def override preVisitExprKeyExists(expr : smart_ptr<ExprKeyExists>) {
        add_table_find_call(expr.arguments[0]._type)
    }

    def override preVisitExprFind(expr : smart_ptr<ExprFind>) {
        add_table_find_call(expr.arguments[0]._type)
    }

    def override preVisitExprSafeAt(expr : smart_ptr<ExprSafeAt>) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType || (subExprT.isPointer && subExprT.firstType.isGoodTableType)) {
            var inscope etype := subExprT.isPointer ? subExprT.firstType : subExprT
            add_table_find_call(etype)
        }
    }

    def override preVisitExprErase(expr : smart_ptr<ExprErase>) {
        add_table_erase_call(expr.arguments[0]._type)
    }
}

def collect_external_functions(standalone_context : LLVMOpaqueValue?; ctx : LLVMContextRef; builder : LLVMBuilderRef,
                               mod : LLVMOpaqueModule?; var types : PrimitiveTypes?;
                               funcs : array<FunctionPtr>; var uids : UidNodes?) {
    var extern_resolver = new CollectExternVisitor(standalone_context, ctx, builder, mod, types, uids)
    var inscope adapter_resolve = make_visitor(*extern_resolver)
    for (fn in funcs) {
        visit(fn, adapter_resolve)
    }
    var result = extern_resolver.any_unimplemented
    extern_resolver.uid = null
    extern_resolver.types = null
    unsafe { delete extern_resolver; }
    return <- result
}

// Creates main function that initializes a standalone JIT context, registers
// all compiled functions, runs init scripts, then calls the program entry point.
def public inject_main(program_context : Context?; ctx : LLVMContextRef; funcs : array<FunctionPtr>;
                       prog : Program?; entry_point : string; mod : LLVMOpaqueModule?;
                       var types : PrimitiveTypes?, var uids : UidNodes?; strict : bool) : LLVMOpaqueValue? {
    var start_fn_name = ""
    var no_return = true
    for (fn in funcs) {
        if (fn.name == entry_point && fn.arguments.empty()) {
            assume fnmna = uids.get_dll_fn_name(fn).impl()
            if (fn.result.isVoid) {
                start_fn_name = fnmna
                no_return = true
            } elif (fn.result.baseType == Type.tInt) {
                start_fn_name = fnmna
                no_return = false
            }
        }
    }
    if (start_fn_name |> empty()) {
        to_log(LOG_ERROR, "entrypoint `{entry_point}()` not found in input file.\n")
        return null
    }

    let main_fn_type = LLVMFunctionType(types.t_int32,
        fixed_array<LLVMTypeRef>(
            types.t_int32,          // argc
            types.LLVMVoidPtrType() // argv
        )
    )
    let main_fn = LLVMAddFunctionWithType(mod, "main", main_fn_type)
    let builder = LLVMCreateBuilder()
    defer <| ${
        LLVMDisposeBuilder(builder)
    }
    let entry = LLVMAppendBasicBlockInContext(ctx, main_fn, "entry")
    LLVMPositionBuilderAtEnd(builder, entry)

    // Init modules. Right now we init only builtin modules.
    // Custom modules is not implemented yet.
    let das_initialize_type = LLVMFunctionType(types.t_void,
        array<LLVMTypeRef>()
    )
    var das_initialize = LLVMAddFunctionWithType(g_mod, "jit_initialize_modules", das_initialize_type)
    LLVMBuildCall2(builder, das_initialize_type, das_initialize, array<LLVMValueRef>(), "")

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


    // Declare jit_create_standalone_ctx
    let create_ctx_type = LLVMFunctionType(types.LLVMVoidPtrType(),
        fixed_array<LLVMTypeRef>(
            types.t_int64,         // totalVariables
            types.t_int64,         // totalFunctions
            types.t_int64,         // globalStringHeapSize
            types.t_int1,          // pinvoke
        )
    )
    var jit_create_context = LLVMAddFunctionWithType(
        g_mod, "jit_create_standalone_ctx", create_ctx_type
    )
    var params = fixed_array(
        types.ConstI64(prog.totalVariables |> uint64),
        types.ConstI64(prog.totalFunctions |> uint64),
        types.ConstI64(prog.globalStringHeapSize |> uint64),
        types.ConstI1(false)
    )
    let global_context = LLVMBuildCall2(builder, create_ctx_type, jit_create_context, params, "")

    let any_unimplemented = collect_external_functions(global_context, ctx, builder, mod, types, funcs, uids)
    if (strict && any_unimplemented) {
        to_log(LOG_ERROR, "Some of the nodes above are unimplemented!\n")
        return null
    }


    let init_global_var_type = LLVMFunctionType(types.LLVMVoidPtrType(),
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
        // Init globals
        var jit_flags : LlvmJitFlags
        jit_flags.emit_prologue = true
        jit_flags.gen_exe = true
        jit_flags.need_di = false
        let (init_glob, init_glob_type) = generate_globals_initialization_fn(program_context, prog, types, uids, jit_flags)
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
    let main_ret = LLVMBuildCall2(builder, g_fn_types[start_fn_name], LLVMGetNamedFunction(g_mod, start_fn_name), main_params, "")
    if (no_return) {
        LLVMBuildRet(builder, types.ConstI32(uint64(0)))
    } else {
        LLVMBuildRet(builder, main_ret)
    }
    to_log(LOG_INFO, "LLVM JIT: standalone exe entry point generated '{start_fn_name}', {length(funcs)} functions\n")
    return main_fn
}
