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
                                    var uids : UidNodes?) : LLVMOpaqueValue? {
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
    extern_resolver.aot_ctx_arg = LLVMGetParam(ctor, 0u)
    make_visitor(*extern_resolver) $(adapter_resolve) {
        ast_gc_guard() {
            for (fn in funcs) {
                visit(fn, adapter_resolve)
            }
        }
    }
    // Fill the cross-module call/ctor target globs recorded during codegen with the
    // host Context's SimFunction* for each callee (by mangled-name hash).
    if (!(g_aot_func_globs |> empty())) {
        let ctx_arg = LLVMGetParam(ctor, 0u)
        for (fg in g_aot_func_globs) {
            if (fg.glob == null) {
                continue
            }
            var simfn = build_aot_get_func_by_mnh(b, types, fg.mnh, ctx_arg)
            // The glob is a pointer-to-fn-type; the SimFunction* (void*) bitcasts in.
            let elemT = LLVMGlobalGetValueType(fg.glob)
            var casted = LLVMBuildPointerCast(b, simfn, elemT, "")
            LLVMBuildStore(b, casted, fg.glob)
        }
    }
    LLVMBuildRetVoid(b)
    LLVMDisposeBuilder(b)
    return ctor
}


//! Build both offline-AOT-object ctors (the extern-resolver globinit + the das_aot_register ctor) and
//! return the (ctor, dtor) pair to append to llvm.global_ctors/dtors. Called by run_jit in emit_aot_object mode.
def public build_llvm_aot_ctors(ctx : LLVMContextRef; mod : LLVMOpaqueModule?; var types : PrimitiveTypes?; var funcs : array<FunctionPtr>; var uids : UidNodes?; prog : Program?) : tuple<LLVMOpaqueValue?, LLVMOpaqueValue?> {
    let glob_ctor = emit_aot_object_globinit(ctx, mod, types, funcs, uids)
    return emit_aot_register_ctor_dtor(types, funcs, uids, prog, glob_ctor)
}
