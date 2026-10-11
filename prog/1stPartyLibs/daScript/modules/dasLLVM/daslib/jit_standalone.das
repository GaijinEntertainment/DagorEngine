options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = false
options relaxed_pointer_const
options unsafe_table_lookup = false
options no_global_variables = false

module jit_standalone shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_dll_utils
require llvm/daslib/llvm_dsl
require daslib/ast_boost
require daslib/option
require daslib/c_api_header
require daslib/safe_addr
require daslib/defer


def public lib_link_note(static_lib : bool; link_whole_lib : bool) : string {
    let windows = get_platform_name() == "windows"
    let rt_static = windows ? "libDaScript_runtime.lib" : "liblibDaScript_runtime.a"
    let cc_static = windows ? "libDaScript.lib" : "liblibDaScript.a"
    let rt_shared = windows ? "libDaScriptDyn_runtime.dll" : (get_platform_name() == "darwin" ? "liblibDaScriptDyn_runtime.dylib" : "liblibDaScriptDyn_runtime.so")
    if (static_lib) {
        let whole = link_whole_lib ? " Add <das_root>/lib/{cc_static} too - this library\nregisters a module that lives in the compiler library." : ""
        let sys = windows ? "" : " Also link the platform libraries the runtime needs:\n-lpthread -ldl -lm -lstdc++."
        return "Link this archive plus <das_root>/lib/{rt_static}.{whole}{sys}"
    }
    let whole = link_whole_lib ? " It also needs the compiler library beside it - this library\nregisters a module that lives there." : ""
    return "Link this shared library. At load it needs {rt_shared}, which it looks for beside itself,\nin ../lib, and in <das_root>/lib; otherwise put it on the loader path.{whole}"
}


def public write_das_bindings(var exports : array<CExport>; names : CNames;
                              input_path, output_path, requested : string; static_lib : bool) : bool {
    let bindings_path = requested == "auto" ? "{output_path}_c.das" : requested
    if (static_lib) {
        to_log(LOG_WARNING, "LLVM LIB: --jit-lib-bindings names a shared library - a static archive has nothing for dasbind to load\n")
    }
    let b = DasBindings(library = output_path)
    let cmd = "daslang -lib {input_path} -output {output_path}"
    if (!emit_das_bindings(exports, names, b, cmd, bindings_path)) {
        to_log(LOG_ERROR, "LLVM LIB: the library is written but its daslang bindings are not - {bindings_path}\n")
        return false
    }
    return true
}


def public new_function(ctx : LLVMContextRef; name : string; typ : LLVMTypeRef; exported : bool) : tuple<fn : LLVMOpaqueValue?; builder : LLVMBuilderRef> {
    let fn = LLVMAddFunctionWithType(g_mod, name, typ)
    if (exported) {
        set_public_linkage(fn)
    } else {
        set_private_linkage(fn)
    }
    let b = LLVMCreateBuilderInContext(ctx)
    LLVMPositionBuilderAtEnd(b, LLVMAppendBasicBlockInContext(ctx, fn, "entry"))
    return (fn = fn, builder = b)
}


struct private ThunkAbi {
    arg_types : array<LLVMTypeRef>
    arg_loads : array<bool>
    c_types : array<LLVMTypeRef>
    slot_type : LLVMOpaqueType?
    has_slot : bool
}


//! The three type lists one thunk needs: what the impl takes (das ABI), what C hands over, and
//! the result slot. `arg_loads[i]` marks a C pointer the impl wants BY VALUE - a vector or a
//! range - so that argument arrives as `*(T *)incoming`, not as the pointer itself.
def private thunk_abi(var e : CExport; var types : PrimitiveTypes?) : ThunkAbi {
    var abi : ThunkAbi
    abi.c_types |> push(types.LLVMVoidPtrType())
    for (arg, p in e.fn.arguments, e.params) {
        abi.arg_types |> push(type_to_llvm_abi_type(arg._type))
        abi.arg_loads |> push(p.by_pointer && !p.pointer_in_impl)
        abi.c_types |> push(p.by_pointer ? types.LLVMVoidPtrType()
            : (arg._type.isBool ? types.t_int8 : type_to_llvm_abi_type(arg._type)))
    }
    if (e.result.via_out) {
        abi.c_types |> push(types.LLVMVoidPtrType())
    }
    if (!e.fn.result.isVoid) {
        abi.has_slot = true
        abi.slot_type = (e.result.cmres ? types.LLVMVoidPtrType() : type_to_llvm_abi_type(e.fn.result))
    }
    return <- abi
}


def private frame_type(var abi : ThunkAbi; ctx : LLVMContextRef) : LLVMOpaqueType? {
    var fields <- [for (t in abi.arg_types); t]
    if (abi.has_slot) {
        fields |> push(abi.slot_type)
    }
    if (fields |> empty()) {
        fields |> push(g_prim_t.t_int32)
    }
    return LLVMStructTypeInContext(ctx, array_data_ptr(fields), uint(length(fields)), 0)
}


def private emit_trampoline(ctx : LLVMContextRef; var e : CExport; var uids : UidNodes?;
                            var types : PrimitiveTypes?; var abi : ThunkAbi; ft : LLVMOpaqueType?) : LLVMOpaqueValue? {
    let impl_name = uids.get_dll_fn_name_ptr(e.fn).impl()
    var impl = LLVMGetNamedFunction(g_mod, impl_name)
    if (impl == null) {
        return null
    }
    let tramp_type = jit_fn_type($(ctx, frame : void?) : void {})
    let made = new_function(ctx, "__das_lib_tramp_{e.c_name}", tramp_type, false)
    let b = made.builder
    defer() {
        LLVMDisposeBuilder(b)
    }
    var ctx_arg = LLVMGetParam(made.fn, 0u)
    var frame = LLVMBuildPointerCast(b, LLVMGetParam(made.fn, 1u), LLVMPointerType(ft, 0u), "frame")
    var args : array<LLVMOpaqueValue?>
    args |> reserve(length(abi.arg_types) + 2)
    for (at, i in abi.arg_types, count()) {
        let slot = LLVMBuildStructGEP2(b, ft, frame, uint(i), "arg_{i}")
        args |> push(LLVMBuildLoad2(b, at, slot, "argv_{i}"))
    }
    args |> push(ctx_arg)
    let slot_index = uint(length(abi.arg_types))
    if (e.result.cmres) {
        var slot_ptr = LLVMBuildStructGEP2(b, ft, frame, slot_index, "res_ptr")
        args |> push(LLVMBuildLoad2(b, types.LLVMVoidPtrType(), slot_ptr, "res"))
    }
    var ret = LLVMBuildCall2(b, g_fn_types[impl_name], impl, args, "")
    if (abi.has_slot && !e.result.cmres) {
        LLVMBuildStore(b, ret, LLVMBuildStructGEP2(b, ft, frame, slot_index, "res"))
    }
    LLVMBuildRetVoid(b)
    return made.fn
}


def private c_return_type(var e : CExport; var types : PrimitiveTypes?) : LLVMTypeRef {
    if (e.result.via_out || e.fn.result.isVoid) {
        return types.t_void
    }
    return e.fn.result.isBool ? types.t_int8 : type_to_llvm_abi_type(e.fn.result)
}


def private store_thunk_args(b : LLVMBuilderRef; var e : CExport; var abi : ThunkAbi;
                             fn : LLVMOpaqueValue?; ft : LLVMOpaqueType?; var types : PrimitiveTypes?; frame : LLVMOpaqueValue?) {
    for (arg, p, i in e.fn.arguments, e.params, count()) {
        var incoming = LLVMGetParam(fn, uint(i + 1))
        var slot = LLVMBuildStructGEP2(b, ft, frame, uint(i), "in_{i}")
        if (abi.arg_loads[i]) {
            var typed = LLVMBuildPointerCast(b, incoming, LLVMPointerType(abi.arg_types[i], 0u), "")
            incoming = LLVMBuildLoadData2Aligned(b, abi.arg_types[i], typed, arg._type.alignOf, "val_{i}")
        } elif (p.by_pointer) {
            incoming = LLVMBuildPointerCast(b, incoming, abi.arg_types[i], "")
        } elif (arg._type.isBool) {
            incoming = LLVMBuildICmp(b, LLVMIntPredicate.LLVMIntNE, incoming, types.ConstI8(int8(0)), "b_{i}")
        }
        LLVMBuildStore(b, incoming, slot)
    }
}


def public emit_thunk(ctx : LLVMContextRef; var e : CExport; var uids : UidNodes?; var types : PrimitiveTypes?;
                      export_all : bool) : Option<bool> {
    var abi <- thunk_abi(e, types)
    let ft = frame_type(abi, ctx)
    let tramp = emit_trampoline(ctx, e, uids, types, abi, ft)
    if (tramp == null) {
        if (export_all) {
            to_log(LOG_WARNING, "LLVM LIB: skipping `{e.fn.name}`: no jitted body to export to C\n")
            return some(true)
        }
        to_log(LOG_ERROR, "LLVM LIB: no jitted body for `{e.fn.name}` - it cannot be exported to C\n")
        return none(type<bool>)
    }
    if (LLVMGetNamedFunction(g_mod, e.c_name) != null) {
        to_log(LOG_ERROR, "LLVM LIB: C symbol `{e.c_name}` is already emitted; rename one with [export_c(name = \"...\")]\n")
        return none(type<bool>)
    }
    let made = new_function(ctx, e.c_name, LLVMFunctionType(c_return_type(e, types), abi.c_types), true)
    let b = made.builder
    defer() {
        LLVMDisposeBuilder(b)
    }
    var frame = LLVMBuildAlloca(b, ft, "frame")
    var cmres_slot : LLVMOpaqueValue?
    if (e.result.cmres) {
        cmres_slot = LLVMBuildAlloca(b, LLVMArrayType(types.t_int8, uint(e.fn.result.sizeOf)), "result")
        LLVMSetAlignment(cmres_slot, uint(e.fn.result.alignOf))
        LLVMBuildStore(b, LLVMBuildPointerCast(b, cmres_slot, types.LLVMVoidPtrType(), ""),
            LLVMBuildStructGEP2(b, ft, frame, uint(length(abi.arg_types)), "res_ptr"))
    }
    store_thunk_args(b, e, abi, made.fn, ft, types, frame)
    let guard_type = jit_fn_type($(ctx, tramp, frame : void?) : int {})
    var guard = declare_extern_fn("jit_lib_invoke_guarded", guard_type)
    var guard_args = array(LLVMGetParam(made.fn, 0u),
        LLVMBuildPointerCast(b, tramp, types.LLVMVoidPtrType(), ""),
        LLVMBuildPointerCast(b, frame, types.LLVMVoidPtrType(), ""))
    var ok = LLVMBuildCall2(b, guard_type, guard, guard_args, "ok")
    if (!abi.has_slot) {
        LLVMBuildRetVoid(b)
        return some(false)
    }
    let ok_bb = LLVMAppendBasicBlockInContext(ctx, made.fn, "ok")
    let fail_bb = LLVMAppendBasicBlockInContext(ctx, made.fn, "raised")
    var cond = LLVMBuildICmp(b, LLVMIntPredicate.LLVMIntNE, ok, types.ConstI32(0ul), "ran")
    LLVMBuildCondBr(b, cond, ok_bb, fail_bb)
    let slot_index = uint(length(abi.arg_types))
    LLVMPositionBuilderAtEnd(b, ok_bb)
    if (e.result.via_out) {
        var out_ptr = LLVMGetParam(made.fn, uint(length(e.params) + 1))
        var source = e.result.cmres ? cmres_slot : LLVMBuildStructGEP2(b, ft, frame, slot_index, "res")
        LLVMBuildMemCpy(b, out_ptr, 1u, source, uint(e.fn.result.alignOf), types.ConstI64(uint64(e.fn.result.sizeOf)))
        LLVMBuildRetVoid(b)
    } else {
        var value = LLVMBuildLoad2(b, abi.slot_type, LLVMBuildStructGEP2(b, ft, frame, slot_index, "res"), "value")
        LLVMBuildRet(b, e.fn.result.isBool ? LLVMBuildZExt(b, value, types.t_int8, "b") : value)
    }
    LLVMPositionBuilderAtEnd(b, fail_bb)
    if (e.result.via_out || e.fn.result.isVoid) {
        LLVMBuildRetVoid(b)
    } else {
        LLVMBuildRet(b, LLVMConstNull(c_return_type(e, types)))
    }
    return some(false)
}


def private emit_shutdown_trampoline(ctx : LLVMContextRef; var uids : UidNodes?;
                                     var funcs : array<FunctionPtr>) : LLVMOpaqueValue? {
    var shutdown_fns <- [for (fn in funcs); fn; where fn.flags.shutdown]
    if (shutdown_fns |> empty()) {
        return null
    }
    let made = new_function(ctx, "__das_lib_ctx_shutdown", jit_fn_type($(ctx, frame : void?) : void {}), false)
    let b = made.builder
    defer() {
        LLVMDisposeBuilder(b)
    }
    var ctx_arg = LLVMGetParam(made.fn, 0u)
    for (fn in shutdown_fns) {
        let impl_name = uids.get_dll_fn_name(fn).impl()
        var impl = LLVMGetNamedFunction(g_mod, impl_name)
        if (impl != null) {
            LLVMBuildCall2(b, g_fn_types[impl_name], impl, array(ctx_arg), "")
        }
    }
    LLVMBuildRetVoid(b)
    return made.fn
}


def private emit_destroy(ctx : LLVMContextRef; var types : PrimitiveTypes?; prefix : string;
                         shutdown_tramp : LLVMOpaqueValue?) {
    let void_ptr = types.LLVMVoidPtrType()
    let made = new_function(ctx, "{prefix}_destroy", jit_fn_type($(ctx : void?) : void {}), true)
    let b = made.builder
    defer() {
        LLVMDisposeBuilder(b)
    }
    var ctx_arg = LLVMGetParam(made.fn, 0u)
    if (shutdown_tramp != null) {
        var guard = declare_extern_fn("jit_lib_invoke_guarded", jit_fn_type($(ctx, tramp, frame : void?) : int {}))
        LLVMBuildCall2(b, jit_fn_type($(ctx, tramp, frame : void?) : int {}), guard,
            array(ctx_arg, LLVMBuildPointerCast(b, shutdown_tramp, void_ptr, ""), LLVMConstNull(void_ptr)), "")
    }
    let destroy_type = jit_fn_type($(ctx : void?) : void {})
    var destroy = declare_extern_fn("jit_destroy_standalone_ctx", destroy_type)
    LLVMBuildCall2(b, destroy_type, destroy, array(ctx_arg), "")
    LLVMBuildRetVoid(b)
}


def private emit_last_error(ctx : LLVMContextRef; var types : PrimitiveTypes?; prefix : string;
                            error_global : LLVMOpaqueValue?) {
    let void_ptr = types.LLVMVoidPtrType()
    let typ = jit_fn_type($(ctx : void?) : void? {})
    let made = new_function(ctx, "{prefix}_last_error", typ, true)
    let b = made.builder
    defer() {
        LLVMDisposeBuilder(b)
    }
    var ctx_arg = LLVMGetParam(made.fn, 0u)
    let from_slot = LLVMAppendBasicBlockInContext(ctx, made.fn, "from_slot")
    let from_ctx = LLVMAppendBasicBlockInContext(ctx, made.fn, "from_ctx")
    var no_ctx = LLVMBuildICmp(b, LLVMIntPredicate.LLVMIntEQ, ctx_arg, LLVMConstNull(void_ptr), "no_ctx")
    LLVMBuildCondBr(b, no_ctx, from_slot, from_ctx)
    LLVMPositionBuilderAtEnd(b, from_slot)
    LLVMBuildRet(b, LLVMBuildLoad2(b, void_ptr, error_global, "create_error"))
    LLVMPositionBuilderAtEnd(b, from_ctx)
    LLVMBuildRet(b, LLVMBuildCall2(b, typ, declare_extern_fn("jit_lib_last_error", typ), array(ctx_arg), ""))
}


def private emit_shutdown_runtime(ctx : LLVMContextRef; var types : PrimitiveTypes?; prefix : string;
                                  guard_global : LLVMOpaqueValue?) {
    let made = new_function(ctx, "{prefix}_shutdown_runtime", jit_fn_type($() : void {}), true)
    let b = made.builder
    defer() {
        LLVMDisposeBuilder(b)
    }
    let shutdown_type = jit_fn_type($(guard : int) : void {})
    var guard = LLVMBuildLoad2(b, types.t_int32, guard_global, "guard")
    LLVMBuildCall2(b, shutdown_type, declare_extern_fn("jit_lib_shutdown", shutdown_type), array(guard), "")
    LLVMBuildRetVoid(b)
}


def public emit_fixed_entries(ctx : LLVMContextRef; var types : PrimitiveTypes?; var uids : UidNodes?;
                              var funcs : array<FunctionPtr>; prefix : string;
                              guard_global, error_global : LLVMOpaqueValue?) {
    emit_destroy(ctx, types, prefix, emit_shutdown_trampoline(ctx, uids, funcs))
    emit_last_error(ctx, types, prefix, error_global)
    emit_shutdown_runtime(ctx, types, prefix, guard_global)
}
