options gen2
options relaxed_pointer_const   // fixed_array<LLVMTypeRef> with const PrimitiveTypes fields needs it

module llvm_aot shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_exe
require llvm/daslib/llvm_dll_utils
require daslib/ast_boost
require daslib/rtti
require daslib/safe_addr
require strings

//! LLVM-AOT registration pass. After the JIT visitor has generated + optimized the module (as for a
//! normal exe), this adds the offline-AOT-object registration layer so a statically-linked .o binds
//! through Program::linkCppAot: a self-registering das_aot_register ctor plus the builtin/Func-value
//! glob re-resolution ctor (built by llvm_exe's extern resolver). All the AOT-object-only "register
//! it into the host" logic lives here; run_jit calls build_llvm_aot_ctors once, in emit_aot_object mode.

// The das_aot_register ctor: for each emitted function, emit das_aot_register(aotHash, publ) so
// the linked .o self-registers into the AOT library at load; also wire the glob re-resolution callback.
def private emit_aot_register_ctor_dtor(types : PrimitiveTypes?; funcs : array<FunctionPtr>; var uids : UidNodes?; _prog : Program?; glob_ctor : LLVMOpaqueValue?) : tuple<LLVMOpaqueValue?, LLVMOpaqueValue?> {
    let regType = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(types.t_int64, types.LLVMVoidPtrType()))
    var regFn = LLVMGetNamedFunction(g_mod, "das_aot_register")
    if (regFn == null) {
        regFn = LLVMAddFunctionWithType(g_mod, "das_aot_register", regType)  // linker binds it to daScript core
    }
    let functionType = LLVMFunctionType(types.t_void, null, 0u, 0)
    // Private: each object defines it, so public linkage would clash when many link together.
    let reg_ctor = LLVMAddFunctionWithType(g_mod, "das_aot_register_constructor", functionType)
    let reg_dtor = LLVMAddFunctionWithType(g_mod, "das_aot_register_destructor", functionType)
    set_private_linkage(reg_ctor)
    set_private_linkage(reg_dtor)
    var ctor_builder = LLVMCreateBuilderInContext(g_ctx)
    var dtor_builder = LLVMCreateBuilderInContext(g_ctx)
    LLVMPositionBuilderAtEnd(ctor_builder, LLVMAppendBasicBlockInContext(g_ctx, reg_ctor, "entry"))
    LLVMPositionBuilderAtEnd(dtor_builder, LLVMAppendBasicBlockInContext(g_ctx, reg_dtor, "entry"))
    for (fun in funcs) {
        var publ = LLVMGetNamedFunction(g_mod, uids->get_dll_fn_name(fun).publ())
        if (publ == null) {
            continue
        }
        let aot_hash = get_function_aot_hash(fun)
        var publ_ptr = LLVMBuildPointerCast(ctor_builder, publ, types.LLVMVoidPtrType(), "")
        var reg_args = array<LLVMValueRef>(types.ConstI64(aot_hash), publ_ptr)
        LLVMBuildCall2(ctor_builder, regType, regFn, reg_args, "")
    }
    // Record the glob re-resolution callback; the runtime runs it lazily at the first AotLibrary drain (not static-init).
    if (glob_ctor != null) {
        let giType = LLVMFunctionType(types.t_void, fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType()))
        var giFn = LLVMGetNamedFunction(g_mod, "das_aot_register_globinit")
        if (giFn == null) {
            giFn = LLVMAddFunctionWithType(g_mod, "das_aot_register_globinit", giType)
        }
        var gi_ptr = LLVMBuildPointerCast(ctor_builder, glob_ctor, types.LLVMVoidPtrType(), "")
        var gi_args = array<LLVMValueRef>(gi_ptr)
        LLVMBuildCall2(ctor_builder, giType, giFn, gi_args, "")
    }
    LLVMBuildRetVoid(ctor_builder)
    LLVMBuildRetVoid(dtor_builder)
    LLVMDisposeBuilder(ctor_builder)
    LLVMDisposeBuilder(dtor_builder)
    return (reg_ctor, reg_dtor)
}


// Build a load ctor that re-resolves this AOT object's builtin-accessor globals to the HOST's real
// helpers (offline objects leave them null+internal since codegen-process addresses are meaningless
// there) and points ExprAddr globals at their publ symbols. No module/function registration.
def public emit_aot_object_globinit(ctx : LLVMContextRef; mod : LLVMOpaqueModule?;
                                    var types : PrimitiveTypes?; var funcs : array<FunctionPtr>;
                                    var uids : UidNodes?; prog : Program?) : LLVMOpaqueValue? {
    // void das_aot_object_globinit_constructor(Context* ctx): ctx is used to
    // resolve Func-value globs to the host's SimFunction* (das_aot_get_func_by_mnh).
    let fnType = LLVMFunctionType(types.t_void, fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType()))
    let ctor = LLVMAddFunctionWithType(g_mod, "das_aot_object_globinit_constructor", fnType)
    LLVMSetLinkage(ctor, LLVMLinkage.LLVMPrivateLinkage)
    var b = LLVMCreateBuilderInContext(ctx)
    LLVMPositionBuilderAtEnd(b, LLVMAppendBasicBlockInContext(ctx, ctor, "entry"))
    // Aliases the caller's uids; do NOT delete it (deletion cascades into that shared uid, which the
    // caller still uses). It's a gc_node, reclaimed by the surrounding ast_gc_guard / process teardown.
    var extern_resolver = new CollectExternVisitor(null, ctx, b, mod, types, uids, false, LlvmJitMode.LLVM_AOT)
    extern_resolver.prog = prog     // the selection a constructor is checked against is the program's
    extern_resolver.aot_ctx_arg = LLVMGetParam(ctor, 0u)
    make_visitor(*extern_resolver) $(adapter_resolve) {
        ast_gc_guard() {
            for (fn in funcs) {
                visit(fn, adapter_resolve)
            }
        }
    }
    LLVMBuildRetVoid(b)
    LLVMDisposeBuilder(b)
    return ctor
}


//! Build both offline-AOT-object ctors (the extern-resolver globinit + the das_aot_register ctor) and
//! return the (ctor, dtor) pair to append to llvm.global_ctors/dtors. Called by run_jit in emit_aot_object mode.
def public build_llvm_aot_ctors(ctx : LLVMContextRef; mod : LLVMOpaqueModule?; var types : PrimitiveTypes?; var funcs : array<FunctionPtr>; var uids : UidNodes?; prog : Program?) : tuple<LLVMOpaqueValue?, LLVMOpaqueValue?> {
    let glob_ctor = emit_aot_object_globinit(ctx, mod, types, funcs, uids, prog)
    return emit_aot_register_ctor_dtor(types, funcs, uids, prog, glob_ctor)
}

//! jam links an AOT object in only through the aot_<obj>_pull symbol its pull list names.
def public emit_aot_pull_anchor(mod : LLVMOpaqueModule?; var types : PrimitiveTypes?; obj_name : string) {
    var anchor = LLVMAddGlobal(mod, types.t_int64, "aot_{obj_name}_pull")
    LLVMSetInitializer(anchor, LLVMConstInt(types.t_int64, 0ul, 0))
}

def public move_global_ctors_to_init_array(mod : LLVMOpaqueModule?; var types : PrimitiveTypes?; obj_name : string) {
    var ctors = LLVMGetNamedGlobal(mod, "llvm.global_ctors")
    if (ctors == null) return
    var list = LLVMGetInitializer(ctors)
    for (i in range(LLVMGetNumOperands(list))) {
        var entry = LLVMAddGlobal(mod, types.LLVMVoidPtrType(), "das_aot_init_{obj_name}_{i}")
        LLVMSetInitializer(entry, LLVMGetOperand(LLVMGetOperand(list, uint(i)), 1u))
        LLVMSetSection(entry, ".init_array")
    }
    LLVMDeleteGlobal(ctors)
}

//! Names the DllName.glob() address globals nothing fills at load, for the caller to panic on.
def public collect_unresolved_address_globals(mod : LLVMOpaqueModule?) : array<string> {
    var bad : array<string>
    var glob = LLVMGetFirstGlobal(mod)
    while (glob != null) {
        let init = LLVMGetInitializer(glob)
        if (init != null && LLVMIsNull(init) != 0) {
            var name_len = 0ul
            let name = LLVMGetValueName2(glob, safe_addr(name_len))
            if (name |> ends_with(" glob")) {
                var loaded = false
                var escaped = false
                var use = LLVMGetFirstUse(glob)
                while (use != null) {
                    let user = LLVMGetUser(use)
                    if (LLVMIsALoadInst(user) != null) {
                        loaded = true
                    } else {
                        escaped = true   // stored into, or the address itself is passed on
                    }
                    use = LLVMGetNextUse(use)
                }
                if (loaded && !escaped) {
                    bad |> push(clone_string(name))
                }
            }
        }
        glob = LLVMGetNextGlobal(glob)
    }
    return <- bad
}
