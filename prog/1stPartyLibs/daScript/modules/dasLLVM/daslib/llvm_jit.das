options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = false
options relaxed_pointer_const
options unsafe_table_lookup = false
options no_global_variables = false
options stack = 4_194_304 // On huge files we need huge stack for jit

module llvm_jit shared private

require llvm/daslib/llvm_boost
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

// uncomment to enable debug
/*
options debugger
require daslib/debug
require daslib/ast_debug
require llvm/llvm_debug
*/

var active_filenames : table<string>

// VarInfo annotation type on c++ side is `vector`.
// They cannot be allocated as global variable and require
// Memory managment - ctors/dtors, same as FileInfo.
struct AnnotationVarInfoEntry {
    varinfo    : LLVMOpaqueValue?
    ann_global : LLVMOpaqueValue?
    count      : int
}
var annotation_varinfo_list : array<AnnotationVarInfoEntry>

[macro_function]
def private get_expr_ptr(expr : ExpressionPtr) {
    return get_ptr(expr)
}

struct private IteBlock {
    if_true  : LLVMOpaqueBasicBlock?
    if_false : LLVMOpaqueBasicBlock?
    if_end   : LLVMOpaqueBasicBlock?
    phi_true  : LLVMOpaqueBasicBlock?
    phi_false : LLVMOpaqueBasicBlock?
    if_true_terminates, if_false_terminates : bool
}

struct private LoopBlock {
    loop_start : LLVMOpaqueBasicBlock?
    loop_body  : LLVMOpaqueBasicBlock?
    loop_end   : LLVMOpaqueBasicBlock?
    loop_continue : LLVMOpaqueBasicBlock?
    need_loop  : LLVMOpaqueValue?
}

struct SLabel {
    index : int
    blk : LLVMOpaqueBasicBlock?
}

def set_private_linkage(value : LLVMOpaqueValue?) {
    LLVMSetLinkage(value, LLVMLinkage.LLVMPrivateLinkage)
    LLVMSetUnnamedAddress(value, LLVMUnnamedAddr.LLVMGlobalUnnamedAddr);
}

// Use internal linkage without unnamed address so symbol names are preserved in the
// object file's .symtab, making backtraces and debuggers (lldb/gdb) readable.
def set_debug_linkage(value : LLVMOpaqueValue?) {
    LLVMSetLinkage(value, LLVMLinkage.LLVMInternalLinkage)
}

def set_public_linkage(value : LLVMOpaqueValue?) {
    LLVMSetLinkage(value, LLVMLinkage.LLVMDLLExportLinkage)
    LLVMSetDLLStorageClass(value, LLVMDLLStorageClass.LLVMDLLExportStorageClass)
}

[macro_function]
def get_file_info_global(g_builder : LLVMOpaqueBuilder?; types : PrimitiveTypes?; name : string) : LLVMOpaqueValue? {
    let sz = typeinfo sizeof(type<FileInfo>)
    let file_info_type = LLVMArrayType(types.t_int8, uint(sz))
    let filename_var = "fileinfo_{name}"
    var fileInfo = LLVMGetNamedGlobal(g_mod, filename_var)
    if (fileInfo == null) {
        fileInfo = LLVMAddGlobal(g_mod, file_info_type, filename_var)
        set_private_linkage(fileInfo)
        var zero = LLVMConstInt(types.t_int8, 0 |> uint64(), 0)
        var zeros : array<LLVMValueRef>
        for (i in range(sz)) {
            zeros |> push(zero)
        }
        let zeroInitializer = LLVMConstArray(types.t_int8, array_data_ptr(zeros), sz |> uint())
        LLVMSetInitializer(fileInfo, zeroInitializer)
        active_filenames |> insert(name)
    }
    return fileInfo
}

/*
 * Initialize all file info names, fileInfo content still empty.
 * @names for all names will be create FileInfo ptr with correct filename
 */
[macro_function]
def generate_fileinfo_ctor_dtor(types : PrimitiveTypes; names : table<string>; jit_mode : bool) {
    var ctor_builder = LLVMCreateBuilderInContext(g_ctx)
    var dtor_builder = LLVMCreateBuilderInContext(g_ctx)
    defer <| ${
        LLVMDisposeBuilder(ctor_builder)
        LLVMDisposeBuilder(dtor_builder)
    }

    let functionType = LLVMFunctionType(types.t_void, null, 0u, 0)

    let constructor = LLVMAddFunctionWithType(g_mod, "fileinfo_constructor", functionType)
    let destructor = LLVMAddFunctionWithType(g_mod, "fileinfo_destructor", functionType)

    if (!jit_mode) {
        set_private_linkage(constructor)
        set_private_linkage(destructor)
    } else {
        set_public_linkage(constructor)
        set_public_linkage(destructor)
    }

    let entry_ctor = LLVMAppendBasicBlockInContext(g_ctx, constructor, "entry_ctor")
    LLVMPositionBuilderAtEnd(ctor_builder, entry_ctor)

    let entry_dtor = LLVMAppendBasicBlockInContext(g_ctx, destructor, "entry_dtor")
    LLVMPositionBuilderAtEnd(dtor_builder, entry_dtor)

    let void_ptr = types.LLVMVoidPtrType()

    var initializeFuncType = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(
            types.LLVMVoidPtrType(),
            LLVMPointerType(types.t_int8, 0u), // filename
        )
    )
    var freeFuncType = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(
            types.LLVMVoidPtrType(),
        )
    )

    var initializeFunc = LLVMAddFunctionWithType(g_mod, "jit_initialize_fileinfo", initializeFuncType)
    var freeFunc = LLVMAddFunctionWithType(g_mod, "jit_free_fileinfo", freeFuncType)

    LLVMAddGlobalMapping(g_engine, initializeFunc, get_jit_initialize_fileinfo())
    LLVMAddGlobalMapping(g_engine, freeFunc, get_jit_free_fileinfo())

    g_fn_types["jit_initialize_fileinfo"] = initializeFuncType
    g_fn_types["jit_free_fileinfo"] = freeFuncType

    for (name in keys(names)) {
        var variable = LLVMGetNamedGlobal(g_mod, "fileinfo_{name}")
        assert(variable != null)
        var callCtorArgs = array<LLVMValueRef>(
            ctor_builder |> LLVMBuildPointerCast(variable, void_ptr, ""),
            get_string_constant_ptr(ctor_builder, name)
        )
        LLVMBuildCall2(ctor_builder, initializeFuncType, initializeFunc, callCtorArgs, "")

        var callDtorArgs = array<LLVMValueRef>(
            dtor_builder |> LLVMBuildPointerCast(variable, void_ptr, ""),
        )
        LLVMBuildCall2(dtor_builder, freeFuncType, freeFunc, callDtorArgs, "")
    }
    LLVMBuildRetVoid(ctor_builder)
    LLVMBuildRetVoid(dtor_builder)

    return (constructor, destructor)
}

[macro_function]
def generate_annotations_ctor_dtor(types : PrimitiveTypes; var entries : array<AnnotationVarInfoEntry>; jit_mode : bool) {
    var ctor_builder = LLVMCreateBuilderInContext(g_ctx)
    var dtor_builder = LLVMCreateBuilderInContext(g_ctx)
    defer <| ${
        LLVMDisposeBuilder(ctor_builder)
        LLVMDisposeBuilder(dtor_builder)
    }
    let functionType = LLVMFunctionType(types.t_void, null, 0u, 0)
    let constructor = LLVMAddFunctionWithType(g_mod, "annotations_constructor", functionType)
    let destructor  = LLVMAddFunctionWithType(g_mod, "annotations_destructor",  functionType)
    if (!jit_mode) {
        set_private_linkage(constructor)
        set_private_linkage(destructor)
    } else {
        set_public_linkage(constructor)
        set_public_linkage(destructor)
    }
    let entry_ctor = LLVMAppendBasicBlockInContext(g_ctx, constructor, "entry_ctor")
    LLVMPositionBuilderAtEnd(ctor_builder, entry_ctor)
    let entry_dtor = LLVMAppendBasicBlockInContext(g_ctx, destructor,  "entry_dtor")
    LLVMPositionBuilderAtEnd(dtor_builder, entry_dtor)
    let void_ptr = types.LLVMVoidPtrType()
    var initFuncType = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(void_ptr, types.t_int32, void_ptr))
    var freeFuncType = LLVMFunctionType(types.t_void,
        fixed_array<LLVMTypeRef>(void_ptr))
    var initFunc = LLVMAddFunctionWithType(g_mod, "jit_initialize_varinfo_annotations", initFuncType)
    LLVMAddGlobalMapping(g_engine, initFunc, get_initialize_varinfo_annotations())
    var freeFunc = LLVMAddFunctionWithType(g_mod, "jit_free_varinfo_annotations",       freeFuncType)
    LLVMAddGlobalMapping(g_engine, freeFunc, get_free_varinfo_annotations())
    for (entry in entries) {
        var ctorArgs = array<LLVMValueRef>(
            ctor_builder |> LLVMBuildPointerCast(entry.varinfo,    void_ptr, ""),
            types.ConstI32(entry.count |> uint64()),
            ctor_builder |> LLVMBuildPointerCast(entry.ann_global, void_ptr, ""))
        LLVMBuildCall2(ctor_builder, initFuncType, initFunc, ctorArgs, "")
        var dtorArgs = array<LLVMValueRef>(
            dtor_builder |> LLVMBuildPointerCast(entry.varinfo,    void_ptr, ""))
        LLVMBuildCall2(dtor_builder, freeFuncType, freeFunc, dtorArgs, "")
    }
    LLVMBuildRetVoid(ctor_builder)
    LLVMBuildRetVoid(dtor_builder)
    return (constructor, destructor)
}

[macro_function]
def get_function_addr(var ctx : Context?; func : Function?) {
    let mangled_name = get_mangled_name(func)
    let MNH = hash(mangled_name)
    // return unsafe(reinterpret<void?>(get_function_address(MNH, *ctx)))
    let MNH_ADDR = get_function_address(MNH, *ctx)
    if (MNH_ADDR == 0 |> uint64) {
        failed("missing function pointer for {func.name}")
    }
    return unsafe(reinterpret<void?>(MNH_ADDR))
}



bitfield public LlvmJitFlags {
    // Whether prologue should be forced for every function.
    emit_prologue,
    // Whether we generating executable. If so, all symbols will be private.
    gen_exe,
    // Is debug info needed. Debug info not fully supported and not tested yet.
    need_di,
}

[macro]
class public LlvmJitVisitor : AstVisitor {
    adapter : smart_ptr<VisitorAdapter>
    e2v : table<Expression?; LLVMOpaqueValue?>
    v2v : table<Variable?; LLVMOpaqueValue?>
    v2t : table<LLVMOpaqueValue?; LLVMOpaqueType?>
    call2cmres : table<Expression?; LLVMOpaqueValue?>
    ffunc, wfunc : LLVMOpaqueValue?
    ite2blocks : table<Expression?; IteBlock>
    thisFunc : Function?
    uid : UidNodes? // function counter. Use it to create uid's for different names in dll.
    thisBlock : ExprBlock?
    subblocks : table<LLVMOpaqueBasicBlock?; LLVMOpaqueBasicBlock?>
    loop_stack : array<LoopBlock>
    block_stack : array<ExprBlock?>
    jit_context : Context?
    function_entry : LLVMOpaqueBasicBlock?
    function_body : LLVMOpaqueBasicBlock?
    monad2block : table<Expression?; LLVMOpaqueBasicBlock?>
    prologue : LLVMOpaqueValue?
    labels : table<int; LLVMOpaqueBasicBlock?>
    sortedLabels : array<SLabel>
    finallyBlocks : table<ExprBlock?; LLVMOpaqueBasicBlock?>
    afterFinallyBlocks : table<ExprBlock?; array<LLVMOpaqueBasicBlock?>>
    skipCall : table<ExprCall?>
    range2 : table<Expression?; LLVMOpaqueValue?>       // for loop - range - where to
    callBlock : LLVMOpaqueValue?
    g_builder : LLVMOpaqueBuilder?
    g_di_builder : LLVMOpaqueDIBuilder?
    di_file_location : table<string; LLVMOpaqueMetadata?>
    di_function : LLVMOpaqueMetadata?
    types : PrimitiveTypes -const? -const
    option_no_range_check : bool = false
    option_no_alias : bool = false
    option_no_capture : bool = false
    flags : LlvmJitFlags
    emit_prologue : bool = false
    own_di : bool = false
    ldu_hint : bool
    attributes : Attributes?

    def LlvmJitVisitor(ctx : Context?; types_ : PrimitiveTypes?; uids : UidNodes?; flags_ : LlvmJitFlags; dib : LLVMOpaqueDIBuilder?; attrs : Attributes?) {
        jit_context = ctx
        types = types_
        g_builder = LLVMCreateBuilderInContext(g_ctx)
        g_di_builder = dib
        flags = flags_
        attributes = attrs
        uid = uids
        if (flags.need_di) {
            own_di = true
            g_di_builder = LLVMCreateDIBuilder(g_mod)
            let producer = "daScript LLVM JIT"
            let runtime_version = 3u
        }
    }

    def finalize {
        if (own_di && g_di_builder != null) {
            LLVMDIBuilderFinalize(g_di_builder)
            LLVMDisposeDIBuilder(g_di_builder)
        }
        LLVMDisposeBuilder(g_builder)
    }

// Debug info
    def const debug_info_enabled() {
        return g_di_builder != null
    }

    def debug_before_function(fun : FunctionPtr) {
        if (debug_info_enabled()) {
            clear_builder_location()
            let unit = get_debug_file_location(fun.at)
            let implName = uid.get_dll_fn_name(fun).impl()
            var ty = LLVMDIBuilderCreateSubroutineType(     // TODO: generate correct function type
                g_di_builder,
                unit,
                null, // LLVMMetadataRef *ParameterTypes,
                0u, // unsigned NumParameterTypes,
                LLVMDIFlags.LLVMDIFlagZero
            )
            di_function = LLVMDIBuilderCreateFunction(
                    g_di_builder,
                    unit,
                    implName,
                    uint64(length(implName)),
                    "", // LinkageName
                    0ul,// LinkageNameLen
                    unit,
                    fun.at.line,
                    ty,
                    0, // IsLocalToUnit
                    1, // IsDefinition
                    0u, // ScopeLine
                    LLVMDIFlags.LLVMDIFlagZero, // Flags
                    LLVM_ENABLE_OPT_PASS ? 1 : 0
            )
            LLVMSetSubprogram(ffunc, di_function)
        }
    }

    def debug_after_function {
        clear_builder_location()
    }

    def set_globalvar_linkage(value : LLVMOpaqueValue?) {
        if (flags.need_di) {
            set_debug_linkage(value)
        } else {
            set_private_linkage(value)
        }
    }

    def get_debug_file_location(at : LineInfo) : LLVMOpaqueMetadata? {
        if (debug_info_enabled()) {
            let filename = at.fileInfo != null ? string(at.fileInfo.name) : ""
            return get_debug_file_location_by_name(filename)
        } else {
            return null
        }
    }

    def get_debug_file_location_by_name(filename : string) : LLVMOpaqueMetadata? {
        var meta & = unsafe(di_file_location[filename])
        if (meta == null) {
            let dir = dir_name(filename)
            meta = LLVMDIBuilderCreateFile(g_di_builder, filename, uint64(length(filename)), dir, uint64(length(dir)))   // todo: boost
        }
        return meta
    }

    def get_debug_location(at : LineInfo) : LLVMOpaqueMetadata? {
        if (debug_info_enabled()) {
            let file_meta = get_debug_file_location(at)
            return LLVMDIBuilderCreateDebugLocation(g_ctx, at.line, at.column, di_function, null)
        } else {
            return null
        }
    }

    def set_builder_location(at : LineInfo) {
        if (debug_info_enabled()) {
            if (di_function != null) {
                LLVMSetCurrentDebugLocation2(g_builder, get_debug_location(at))
            }
        }
    }

    def clear_builder_location {
        if (debug_info_enabled()) {
            LLVMSetCurrentDebugLocation2(g_builder, null)
        }
    }

// Expression
    def override preVisitExpression(expr : ExpressionPtr) : void {
        set_builder_location(expr.at)
    }

// Function
    def build_noalias_list(annotations : AnnotationList) : tuple<noalias : table<string>; nocapture : table<string>> {
        var noalias : table<string>
        var nocapture : table<string>
        for (ann in annotations) {
            if (ann.annotation.name == "hint") {
                for (arg in ann.arguments) {
                    if (arg.name == "noalias") {
                        if (arg.basicType == Type.tString) {
                            noalias |> insert(string(arg.sValue))
                        }
                    } elif (arg.name == "nocapture") {
                        if (arg.basicType == Type.tString) {
                            nocapture |> insert(string(arg.sValue))
                        }
                    }
                }
            }
        }
        return <-  (noalias, nocapture)
    }

    def append_basic_block(name : string) {
        return LLVMAppendBasicBlockInContext(g_ctx, ffunc, name)
    }

    def process_function_hints(annotations : AnnotationList; var arguments : dasvector`smart_ptr`Variable) {
        var alwaysInline = false
        var noInline = false
        var optNone = false
        var hot = false
        option_no_range_check = false
        option_no_alias = false
        option_no_capture = false
        for (ann in annotations) {
            if (ann.annotation.name == "hint") {
                for (arg in ann.arguments) {
                    if (arg.name == "alwaysinline") {
                        alwaysInline = true
                    }
                    if (arg.name == "noinline") {
                        noInline = true
                    } elif (arg.name == "optnone") {
                        optNone = true
                    } elif (arg.name == "hot") {
                        hot = true
                    } elif (arg.name == "unsafe_range_check") {
                        option_no_range_check = true
                    } elif (arg.name == "unsafe_alias") {
                        option_no_alias = true
                    } elif (arg.name == "unsafe_capture") {
                        option_no_capture = true
                    } elif (arg.name == "align16" && arg.basicType == Type.tString) {
                        for (a, ai in arguments, ucount()) {
                            if (a.name == arg.sValue) {
                                LLVMSetParamAlignment(LLVMGetParam(ffunc, ai), 16u)
                            }
                        }
                    } elif (arg.name == "vec3_ldu") {
                        LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = arg.bValue
                    }
                }
            }
        }
        if (alwaysInline) {
            LLVMAddAttributeToFunction(ffunc, attributes.alwaysinline)
        } elif (noInline) {
            LLVMAddAttributeToFunction(ffunc, attributes.noinline)
        }
        if (hot) {
            LLVMAddAttributeToFunction(ffunc, attributes.hot)
        }
        if (optNone) {
            if (alwaysInline) {
                failed("Attribute 'optnone' can't be mixed with 'alwaysinline'!")
            }
            LLVMAddAttributeToFunction(ffunc, attributes.optnone)
            LLVMAddAttributeToFunction(ffunc, attributes.noinline)
        }
    }

    def process_finally(expr : ExpressionPtr) {
        var blk <- collect_finally(expr, true)
        sort(blk) <| $(a, b) => uid.get_block_name_ptr(a).publ() < uid.get_block_name_ptr(b).publ()
        for (bl in blk) {
            finallyBlocks[bl] = append_basic_block("finally_block_at_{int(bl.at.line)}_")
        }
        if (length(blk) > 0) {
            at_function_entry <| $() {
                callBlock = LLVMBuildAlloca(g_builder, LLVMPointerType(types.t_int8, 0u), "call_block")
                LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(types.t_int8, 0u)), callBlock)
            }
        }
    }

    def process_labels(expr : ExpressionPtr) {
        var lls <- collect_labels(expr)
        sort(lls)
        for (ll in lls) {
            labels[ll] = append_basic_block("label_{ll}_")
        }
        sortedLabels |> reserve(length(labels))
        for (k, v in keys(labels), values(labels)) {
            sortedLabels |> push(SLabel(index = k, blk = v))
        }
        sort(sortedLabels) <| $(a, b) {
            return a.index < b.index
        }
    }

    def build_llvm_function_pair(fun_name : DllName; isCmres : bool; arguments : dasvector`smart_ptr`Variable; result : TypeDeclPtr; captureType : LLVMOpaqueType?; annotations : AnnotationList) {
        var inscope nat <- build_noalias_list(annotations)
        // implementation function
        var fun_args <-  [for (a in arguments); type_to_llvm_abi_type(a._type)]
        if (captureType != null) {
            fun_args |> push(LLVMPointerType(captureType, 0u)) // capture
        }
        fun_args |> push(types.LLVMVoidPtrType())  // context
        var fun_type : LLVMOpaqueType?
        if (isCmres) {
            fun_args |> push(types.LLVMVoidPtrType())  // CMRES
            fun_type = LLVMFunctionType(types.t_void, fun_args)
        } else {
            fun_type = LLVMFunctionType(type_to_llvm_abi_type(result), fun_args)
        }
        if (LLVMGetNamedFunction(g_mod, fun_name.impl()) != null) {
            to_log(LOG_ERROR, "LLVM JIT: Function {fun_name.impl()} somehow already present!")
            failed("Function {fun_name.impl()} somehow already present!")
        }
        ffunc = LLVMAddFunctionWithType(g_mod, fun_name.impl(), fun_type)
        if (flags.need_di) {
            set_debug_linkage(ffunc)
        } else {
            set_private_linkage(ffunc)
        }
        // setup parameter alignment
        for (a, ai in arguments, ucount()) {
            var param = LLVMGetParam(ffunc, ai)
            if (a._type.isRef) {
                LLVMSetParamAlignment(param, uint(a._type.alignOf))
            } elif (a._type.isPointer && a._type.firstType != null) {
                LLVMSetParamAlignment(param, uint(a._type.firstType.alignOf))
            }
            LLVMSetValueName(param, string(a.name))
            if ((a._type.isRef || a._type.isPointer || a._type.isString) && a._type.flags.constant) {
                LLVMAddAttributeToFunctionArgument(ffunc, ai, attributes.readonly)
            }
            if (option_no_alias || nat.noalias |> key_exists(string(a.name))) {
                LLVMAddAttributeToFunctionArgument(ffunc, ai, attributes.noalias)
            }
            if (option_no_capture || nat.nocapture |> key_exists(string(a.name))) {
                LLVMAddAttributeToFunctionArgument(ffunc, ai, attributes.nocapture)
            }
        }
        // noalias of the capture target, context, result
        var nai = uint(length(arguments))
        if (captureType != null) {
            LLVMAddAttributeToFunctionArgument(ffunc, nai, attributes.nocapture)
            LLVMAddAttributeToFunctionArgument(ffunc, nai++, attributes.noalias)  // capture target
        }
        LLVMAddAttributeToFunctionArgument(ffunc, nai, attributes.nocapture)
        LLVMAddAttributeToFunctionArgument(ffunc, nai++, attributes.noalias)      // context
        if (isCmres) {
            LLVMAddAttributeToFunctionArgument(ffunc, nai, attributes.nocapture)
            LLVMAddAttributeToFunctionArgument(ffunc, nai++, attributes.noalias)  // cmres
        }
        // alignment of the context and the result
        LLVMSetParamAlignment(get_context_param(), 16u)
        if (isCmres) {
            LLVMSetParamAlignment(get_cmres_param(), uint(result.alignOf))
        }
        // wrapper function
        var ret_type : LLVMOpaqueType?
        if (captureType != null) {
            ret_type = LLVMFunctionType(types.LLVMFloat4Type(),
                fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(),   // context
                LLVMPointerType(types.LLVMFloat4Type(), 0u),              // args
                types.LLVMVoidPtrType(),                // cmres
                LLVMPointerType(captureType, 0u)                    // block
            ))
        } else {
            ret_type = LLVMFunctionType(types.LLVMFloat4Type(),
                fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(),   // context
                LLVMPointerType(types.LLVMFloat4Type(), 0u),              // args
                types.LLVMVoidPtrType()                 // cmres
            ))
        }
        wfunc = LLVMAddFunctionWithType(g_mod, fun_name.publ(), ret_type)
        if (flags.gen_exe) {
            // todo: we shouldn't generate public function `wfunc`
            // at all when gen_exe enabled.
            // But it requires some refactoring.
            set_private_linkage(wfunc)
        } else {
            set_public_linkage(wfunc)
        }
        LLVMSetParamAlignment(LLVMGetParam(wfunc, 0u), 16u)
        LLVMSetParamAlignment(LLVMGetParam(wfunc, 1u), 16u)
        if (isCmres) {
            LLVMSetParamAlignment(LLVMGetParam(wfunc, 2u), uint(result.alignOf))
        }
        if (captureType != null) {
            LLVMSetParamAlignment(LLVMGetParam(wfunc, 3u), 16u)
        }
    }

    def add_llvm_functions(fun : FunctionPtr) {
        thisFunc = get_ptr(fun)
        let fnmna = uid.get_dll_fn_name(fun)
        build_llvm_function_pair(fnmna, isCMRES(fun), fun.arguments, fun.result, null, fun.annotations)
        thisFunc = null
    }

    def build_wrapper_function(fnmna : DllName; arguments : dasvector`smart_ptr`Variable; var result : TypeDeclPtr; isCmres : bool; captureType : LLVMOpaqueType?; at : LineInfo) {
        var wentry = LLVMAppendBasicBlockInContext(g_ctx, wfunc, "entry")
        LLVMPositionBuilderAtEnd(g_builder, wentry)
        var wargs : array<LLVMOpaqueValue?>
        var args = LLVMGetParam(wfunc, 1u)
        for (a, ai in arguments, count()) {
            var arg_ptr = LLVMBuildGEP2(g_builder, types.LLVMFloat4Type(), args, types.ConstI32(uint64(ai)), "arg_ptr_{ai}")
            var arg_type = type_to_llvm_abi_type(a._type)
            arg_ptr = LLVMBuildPointerCast(g_builder, arg_ptr, LLVMPointerType(arg_type, 0u), "")
            var arg_v : LLVMOpaqueValue?
            if (a._type.isVoid) {
                pass
            } elif (a._type.flags.ref) {
                arg_v = LLVMBuildLoadData2Aligned(g_builder, arg_type, arg_ptr, a._type.alignOf, "arg_ref_value_{ai}")
            } elif (!a._type.isRef) {
                arg_v = LLVMBuildLoadData2Aligned(g_builder, arg_type, arg_ptr, a._type.alignOf, "arg_value_{ai}")
            } else {
                arg_v = LLVMBuildLoadData2Aligned(g_builder, arg_type, arg_ptr, a._type.alignOf, "arg_any_{ai}")
            }
            if (arg_v != null) {
                wargs |> push(arg_v)
            }
        }
        if (captureType != null) {
            wargs |> push(LLVMGetParam(wfunc, 3u))  // block
        }
        wargs |> push(LLVMGetParam(wfunc, 0u)) // context
        if (isCmres) {
            wargs |> push(LLVMGetParam(wfunc, 2u)) // cmres
        }
        var ret_v = LLVMBuildCall2(g_builder, g_fn_types[(fnmna.impl())], ffunc, wargs, "")
        if (!result.isVoid) {
            if (isCmres) {
                LLVMBuildRet(g_builder, cast_ptr_to_vec4f(LLVMGetParam(wfunc, 2u)))
            } else {
                var tres = cast_to_vec4f(result, ret_v)
                if (tres != null) {
                    LLVMBuildRet(g_builder, tres)
                } else {
                    LLVMBuildRet(g_builder, LLVMGetUndef(types.LLVMFloat4Type()))
                }
            }
        } else {
            LLVMBuildRet(g_builder, LLVMGetUndef(types.LLVMFloat4Type()))
        }
    }

    def create_line_info_global(at : LineInfo; dummy : LLVMOpaqueValue?) {
        let uint8_ptr = LLVMPointerType(types.t_int8, 0u)
        var line_info_members = fixed_array(
            LLVMPointerType(types.t_int8, 0u), types.t_int32, types.t_int32, types.t_int32, types.t_int32
        )
        let line_info_type = StructType(types, line_info_members)
        let line_info_global = LLVMAddGlobal(g_mod, line_info_type, "li_{int(at.line)}_{int(at.column)}")
        set_globalvar_linkage(line_info_global)
        var init_values = fixed_array(g_builder |> LLVMBuildPointerCast(dummy, uint8_ptr, ""),
            types.ConstI32(at.column |> uint64()),
            types.ConstI32(at.line |> uint64()),
            types.ConstI32(at.last_column |> uint64()),
            types.ConstI32(at.last_line |> uint64())
        )
        let line_info_init = LLVMConstStructInContext(g_ctx, array_data_ptr(init_values), init_values |> length() |> uint(), 0)
        LLVMSetInitializer(line_info_global, line_info_init)
        return line_info_global
    }

    def create_line_info_ptr_global(line_info_global : LLVMOpaqueValue?) {
        let uint8_ptr = LLVMPointerType(LLVMPointerType(types.t_int8, 0u), 0u)
        let line_info_ptr_type = uint8_ptr
        let line_info_ptr_global = LLVMAddGlobal(g_mod, line_info_ptr_type, "li_ptr" /* no name */)
        set_globalvar_linkage(line_info_ptr_global)
        LLVMSetInitializer(line_info_ptr_global, line_info_global)
        return line_info_ptr_global
    }

    def instrument_and_map_global(at : LineInfo; line_info_ptr_global : LLVMOpaqueValue?) {
        let info_ptr = instrument_line_info(at) // pointer in 'code' of the context
        g_engine |> LLVMAddGlobalMapping(line_info_ptr_global, info_ptr)
        return line_info_ptr_global
    }

    def get_line_info_ptr(at : LineInfo) {
        let void_ptr = types.LLVMVoidPtrType()
        let fi = get_file_info_global(g_builder, types, at.fileInfo != null ? string(at.fileInfo.name) : "unknown_filename")
        let line_info_global = create_line_info_global(at, fi)
        let line_info_ptr_global = create_line_info_ptr_global(line_info_global)
        instrument_and_map_global(at, line_info_ptr_global)
        var loaded_ptr = LLVMBuildLoad2(g_builder, void_ptr, line_info_ptr_global, "")
        // return g_builder |> LLVMBuildPointerCast(loaded_ptr, void_ptr, "")
        return loaded_ptr
    }

    def build_function_entry(funAt : LineInfo;
                             bodyId : uint64; need_prologue : bool;
                             var totalStackSize : uint;
                             var arguments : dasvector`smart_ptr`Variable) {
        // now write to regular function
        function_entry = append_basic_block("entry")
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        // enter
        build_enter(funAt)
        // allocate prologue if need be
        if (need_prologue) {
            prologue = LLVMBuildAlloca(g_builder, g_t_stack_state, "prologue")
            prologue = LLVMBuildPointerCast(g_builder, prologue, LLVMPointerType(g_t_stack_state, 0u), "")
            var params = fixed_array(
                get_string_constant_ptr(g_builder, "{thisFunc.name}"),
                get_line_info_ptr(funAt),
                types.ConstI32(uint64(totalStackSize)),
                prologue,
                get_context_param()
            )
            var typ = g_fn_types[FN_JIT_PROLOGUE]
            LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_PROLOGUE), params, "")
        }
        // allocate arguments, which need to be promoted to local variables
        // NOTE:
        //  thisFunc hasMakeBlock is here just in case we need to capture argument variables
        //  in the future, we can optimize this out by checking, if particular variable is captured via block
        //  additionally this causes every block variable to be promoted - which may not be ideal since it would clutter the optimizer (or not?)
        var alwaysPromote = false
        if (thisBlock != null && thisBlock.blockFlags.hasMakeBlock) {
            alwaysPromote = true
        } elif (thisFunc != null && thisFunc.flags.hasMakeBlock) {
            alwaysPromote = true
        }
        for (a, ai in arguments, range(10050)) {
            if ((alwaysPromote || a.access_flags.access_ref || a.access_flags.access_pass) && !a._type.isRef) {
                var tryV = LLVMBuildAlloca(g_builder, type_to_llvm_abi_type(a._type), "var_{a.name}")
                LLVMSetAlignment(tryV, 16u)
                LLVMBuildStore(g_builder, LLVMGetParam(ffunc, uint(ai)), tryV)
                setV(a, tryV)
            }
        }
        // function body
        function_body = append_basic_block("body_{bodyId}")
        LLVMPositionBuilderAtEnd(g_builder, function_body)
    }

    def override canVisitFunctionArgumentInit(fun : Function?; arg : VariablePtr; value : ExpressionPtr) : bool {
        return false
    }

    def override preVisitFunction(var fun : FunctionPtr) : void {
        assert(g_builder != null, "missing builder")
        thisFunc = get_ptr(fun)
        uid.reset(get_ptr(fun))
        ldu_hint = LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS
        let fnmna = uid.get_dll_fn_name(fun)
        ffunc = LLVMGetNamedFunction(g_mod, fnmna.impl())
        wfunc = LLVMGetNamedFunction(g_mod, fnmna.publ())
        // build wrapper function
        build_wrapper_function(fnmna, fun.arguments, fun.result, isCMRES(fun), null, fun.at)
        build_function_entry(fun.at, hash(get_mangled_name(fun)),
                             fun.flags.hasMakeBlock || emit_prologue,
                             fun.totalStackSize, fun.arguments)
        process_function_hints(fun.annotations, fun.arguments)
        process_labels(fun.body)
        process_finally(fun.body)
        debug_before_function(fun)
        set_builder_location(fun.at)
    }

    def build_epilogue {
        if (prologue != null) {
            var params = fixed_array(
                prologue,
                get_context_param()
            )
            var typ = g_fn_types[FN_JIT_EPILOGUE]
            LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_EPILOGUE), params, "")
        }
    }

    def current_block_terminates {
        return LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(g_builder)) != null
    }

    def override visitFunction(fun : FunctionPtr) : FunctionPtr {
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        LLVMBuildBr(g_builder, function_body)
        debug_after_function()
        LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = ldu_hint
        ffunc = null
        thisFunc = null
        uid.reset(null)
        return fun
    }

// ExprBlock
    def add_default_terminator(blk : smart_ptr<ExprBlock>) {
        if (thisBlock == null && thisFunc != null && thisFunc.body != get_ptr(blk)) {
            return
        }
        if (thisBlock != null && thisBlock != get_ptr(blk)) {
            return
        }
        if (!current_block_terminates()) {
            call_all_finally_blocks("default_terminator", false)
            build_exit(blk.at)
            if (thisBlock != null && thisBlock.returnType != null && !thisBlock.returnType.isVoid) {
                LLVMBuildUnreachable(g_builder)
            } elif (thisBlock == null && thisFunc != null && thisFunc.result != null && !thisFunc.result.isVoid) {
                LLVMBuildUnreachable(g_builder)
            } else {
                build_epilogue()
                LLVMBuildRetVoid(g_builder)
            }
        }
    }

    def hasFinalSection(blk : ExprBlock?) {
        if (blk == null) {
            return false
        }
        if (length(blk.finalList) > 0) {
            return true
        }
        // TODO: do better test here. block with iterator, or over an array<> has a final section. maybe other??? but not all
        return blk.blockFlags.forLoop   // for loop always has final section
    }

    def has_any_finally_block(stopAtLoop : bool) {
        var len = length(block_stack)
        for (i in range(len)) {
            var blk = block_stack[len - 1 - i]
            if (stopAtLoop && blk.blockFlags.inTheLoop) {
                break
            }
            return true
        }
        return false
    }

    def call_all_finally_blocks(from_where : string; stopAtLoop : bool) {
        var len = length(block_stack)
        var anyFinallyBlocks = false
        for (i in range(len)) {
            var blk = block_stack[len - 1 - i]
            if (stopAtLoop && (blk.blockFlags.inTheLoop || blk.blockFlags.forLoop)) {
                break
            }
            if (blk |> hasFinalSection()) {
                var after = append_basic_block("{from_where}_after_call_finally_block_{int(blk.at.line)}")
                var after_ptr = LLVMBlockAddress(LLVMGetBasicBlockParent(after), after)
                LLVMBuildStore(g_builder, after_ptr, callBlock)
                var fblk = finallyBlocks[blk]
                LLVMBuildBr(g_builder, fblk)
                LLVMPositionBuilderAtEnd(g_builder, after)
                afterFinallyBlocks[blk] |> push(after)
                anyFinallyBlocks = true
            }
        }
        return anyFinallyBlocks
    }

    def override preVisitExprBlock(var blk : smart_ptr<ExprBlock>) : void {
        if (blk.blockFlags.inTheLoop) {
            blk.blockFlags |= ExprBlockFlags.finallyDisabled
        }
        block_stack |> push(get_ptr(blk))
    }

    def override preVisitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) : void {
        if (LLVM_DEBUG_TRACES && LLVM_DEBUG_LINE_TRACES) {
            if (!current_block_terminates()) {
                build_debug_trace(expr.at, FN_JIT_DEBUG_LINE)
            }
        }
    }

    def override preVisitExprBlockFinal(blk : smart_ptr<ExprBlock>) : void {
        add_default_terminator(blk)
        if (!blk.blockFlags.forLoop) {// for loop just does it
            if (finallyBlocks |> key_exists(get_ptr(blk))) {
                var fblk = finallyBlocks[get_ptr(blk)]
                if (!current_block_terminates()) {
                    LLVMBuildBr(g_builder, fblk)
                }
                LLVMPositionBuilderAtEnd(g_builder, fblk)
            }
        }
    }

    def jump_to_next_finally(blk : ExprBlock?) {
        afterFinallyBlocks  |> get(blk) <| $(AfterFinallyBlocks) {
            if (length(AfterFinallyBlocks) > 0) {
                var cc = LLVMBuildLoad2(g_builder, LLVMPointerType(types.t_int8, 0u), callBlock, "")
                var needCond = thisFunc.body != blk && thisBlock != blk
                var do_jump, skip_jump : LLVMOpaqueBasicBlock?
                if (needCond) {
                    do_jump = append_basic_block("do_jump_blk_line_{int(blk.at.line)}_")
                    skip_jump = append_basic_block("skip_jump_blk_line_{int(blk.at.line)}_")
                    var not_null = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, cc, LLVMConstNull(LLVMPointerType(types.t_int8, 0u)), "")
                    LLVMBuildCondBr(g_builder, not_null, do_jump, skip_jump)
                    LLVMPositionBuilderAtEnd(g_builder, do_jump)
                }
                var indb = LLVMBuildIndirectBr(g_builder, cc, uint(length(AfterFinallyBlocks)))
                for (bb in AfterFinallyBlocks) {
                    LLVMAddDestination(indb, bb)
                }
                if (skip_jump != null) {
                    LLVMPositionBuilderAtEnd(g_builder, skip_jump)
                }
            }
        }
    }

    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        if (length(blk.finalList) == 0) {// TODO: check for loop
            add_default_terminator(blk)
        } elif (!blk.blockFlags.inTheLoop) {
            jump_to_next_finally(get_ptr(blk))
        }
        block_stack |> pop()
        if (blk.blockFlags.inTheLoop) {
            blk.blockFlags ^= ExprBlockFlags.finallyDisabled
        }
        return blk
    }

// ExprLet
    def isCall2CMRES(expr : ExpressionPtr) {
        return false if (!(expr is ExprCall))
        let call = expr as ExprCall
        return false if (!isCMRES(call.func) || call.cmresAlias)
        return call.doesNotNeedSp || call.stackTop == 0u
    }

    def isInvoke2CMRES(expr : ExpressionPtr) {
        return false if (!(expr is ExprInvoke))
        let inv = expr as ExprInvoke
        return !inv.cmresAlias && isCMRESType(inv._type)
    }

    def make_call_to_cmres(expr : ExpressionPtr; v_ptr : LLVMOpaqueValue?) {
        if (isCall2CMRES(expr)) {
            call2cmres[get_ptr(expr)] = LLVMBuildPointerCast(g_builder, v_ptr, types.LLVMVoidPtrType(), "call_result_{(expr as ExprCall).func.name}")
            return true
        } elif (isInvoke2CMRES(expr)) {
            call2cmres[get_ptr(expr)] = LLVMBuildPointerCast(g_builder, v_ptr, types.LLVMVoidPtrType(), "invoke_result_{int(expr.at.line)}")
            return true
        }
        return false
    }

    def override preVisitExprLetVariable(expr : smart_ptr<ExprLet>; arg : VariablePtr; lastArg : bool) : void {
        var vtype, vptrtype : LLVMOpaqueType?
        if (arg._type.flags.ref) {
            vtype = get_type_pointer(arg._type)
            vptrtype = LLVMPointerType(vtype, 0u)
        } else {
            vtype = type_to_llvm_type(arg._type)
            vptrtype = get_type_pointer(arg._type)
        }
        var v_ptr : LLVMOpaqueValue?
        var vsize = arg._type.flags.ref ?  typeinfo sizeof(type<void?>) : arg._type.sizeOf
        var valign = arg._type.flags.ref ?  typeinfo alignof(type<void?>) : arg._type.alignOf

        if (arg.flags.aliasCMRES) {
            v_ptr = get_cmres_param()
            v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, vptrtype, "variable_cmres_{arg.name}")
        } else {
            at_function_entry <| $() {
                v_ptr = LLVMBuildAlloca(g_builder, vtype, "variable_{arg.name}")
                LLVMBuildMemSet(g_builder, v_ptr, 0ul, uint64(vsize), uint(valign))
                LLVMSetAlignment(v_ptr, 16u)
                v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, vptrtype, "variable_{arg.name}")
            }
        }
        setV(arg, v_ptr)
        if (arg.init == null) {
            LLVMBuildMemSet(g_builder, v_ptr, 0ul, uint64(vsize), uint(valign))
        } elif (make_call_to_cmres(arg.init, v_ptr)) {
            pass    // do nothing
        } elif (isMakeLocal(arg.init)) {
            var lptr = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(types.t_int8, 0u), "alias_make_local_{arg.name}")
            setE(arg.init, lptr)
        }
    }

    def override visitExprLetVariableInit(blk : smart_ptr<ExprLet>; arg : VariablePtr; expr : ExpressionPtr) : ExpressionPtr {
        var tdst = getV(arg)
        if (isCall2CMRES(expr)) {
            pass    // this variable is initialized via cmres call
        } elif (arg._type.flags.ref) {
            var src = getE(expr)
            LLVMBuildStore(g_builder, src, tdst)
        } elif (isMakeLocal(arg.init)) {
            pass
        } else {
            if (arg.flags.init_via_move && arg._type.canMove && !arg._type.isGoodBlockType) {
                build_move(tdst, expr, true)
            } else {
                build_copy(tdst, expr)
            }
        }
        return expr
    }

// ExprStringBuilder
    def override visitExprStringBuilder(expr : smart_ptr<ExprStringBuilder>) : ExpressionPtr {
        var args = build_array_of_arguments(expr.elements)
        var func = LLVMGetNamedFunction(g_mod, expr.stringBuilderFlags.isTempString ? FN_JIT_STRING_BUILDER_TEMP : FN_JIT_STRING_BUILDER)
        assert(func != null)
        // Evaluate all elements into a stack vec4f[] array
        // Build a global array of TypeInfo* for each element
        let nArgs = length(expr.elements) |> uint()
        let void_ptr = types.LLVMVoidPtrType()
        var types_arr_ptr : LLVMOpaqueValue?
        if (nArgs > 0u) {
            var tinfo_vals : array<LLVMValueRef>
            for (elem in expr.elements) {
                let tinfo = make_type_info_structure(*jit_context, elem._type)
                let tinfo_global = create_type_info_global(tinfo)
                tinfo_vals |> push <| unsafe(reinterpret<LLVMValueRef>(tinfo_global))
            }
            let arr_ty = LLVMArrayType(void_ptr, nArgs)
            var ti_arr = LLVMAddGlobal(g_mod, arr_ty, "string_node_{expr.at.line:d}")
            set_globalvar_linkage(ti_arr)
            ti_arr |> LLVMSetInitializer <| LLVMConstArray(void_ptr, array_data_ptr(tinfo_vals), nArgs)
            types_arr_ptr = ti_arr
        } else {
            types_arr_ptr = LLVMConstPointerNull(void_ptr)
        }
        var params <- array<LLVMOpaqueValue?>(
            get_context_param(),
            types.ConstI32(nArgs |> uint64()),
            types_arr_ptr,
            get_line_info_ptr(expr.at),
            args
        )
        var typ = g_fn_types[FN_JIT_STRING_BUILDER]
        setE(expr, LLVMBuildCall2(g_builder, typ, func, params, "string_builder"))
        return expr
    }

// ExprNew
    def check_zero(tsrc : LLVMOpaqueValue?) {
        var check_null_ptr = append_basic_block("check_null_ptr")
        var check_true = append_basic_block("check_true")
        var check_end = append_basic_block("check_end")
        LLVMBuildBr(g_builder, check_null_ptr)
        LLVMPositionBuilderAtEnd(g_builder, check_null_ptr)
        var cond = LLVMBuildIsNull(g_builder, tsrc, "null_check")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_true)
        return check_end
    }

    def check_ptr_zero(tsrc : LLVMOpaqueValue?; at : LineInfo; message : string = "dereferencing null pointer") {
        var check_end = check_zero(tsrc)
        build_exception(message, at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_end)
    }

    def check_ptr_zero_and_return(tsrc : LLVMOpaqueValue?; ret_val : LLVMOpaqueValue? = null) {
        var check_end = check_zero(tsrc)
        if (ret_val == null) {
            LLVMBuildRetVoid(g_builder)
        } else {
            LLVMBuildRet(g_builder, ret_val)
        }
        LLVMPositionBuilderAtEnd(g_builder, check_end)
    }

    def build_alloc(bytes : int; persistent : bool) {
        var params = fixed_array(
            types.ConstI32(uint64(bytes)),    // bytes
            get_context_param()
        )
        var typ = g_fn_types[persistent ? FN_JIT_ALLOC_PERSISTENT : FN_JIT_ALLOC_HEAP]
        var fn = LLVMGetNamedFunction(g_mod, persistent ? FN_JIT_ALLOC_PERSISTENT : FN_JIT_ALLOC_HEAP)
        return LLVMBuildCall2(g_builder, typ, fn, params, "")
    }

    def build_fixed_loop(count : int; blk : block<(var index : LLVMOpaqueValue?) : void>) {
        if (count == 0) {
            pass
        }
        if (count == 1) {
            invoke(blk, types.ConstI32(0ul))
        } else {
            var loop = LLVMBuildAlloca(g_builder, types.t_int32, "loop_var")
            LLVMBuildStore(g_builder, types.ConstI32(0ul), loop)
            var loop_bb = append_basic_block("loop_begin")
            var loop_end_bb = append_basic_block("loop_end")
            LLVMBuildBr(g_builder, loop_bb)
            LLVMPositionBuilderAtEnd(g_builder, loop_bb)
            invoke(blk, LLVMBuildLoad2(g_builder, types.t_int32, loop, ""))
            LLVMBuildStore(g_builder, LLVMBuildAdd(g_builder, LLVMBuildLoad2(g_builder, types.t_int32, loop, ""), types.ConstI32(1ul), ""), loop)
            var loop_cond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntULT, LLVMBuildLoad2(g_builder, types.t_int32, loop, ""), types.ConstI32(uint64(count)), "")
            LLVMBuildCondBr(g_builder, loop_cond, loop_bb, loop_end_bb)
            LLVMPositionBuilderAtEnd(g_builder, loop_end_bb)
        }
    }


    def new_entry(expr : smart_ptr<ExprNew>) {
        var new_ptr : LLVMOpaqueValue?
        var persistent = false
        if (expr.typeexpr.baseType == Type.tStructure) {
            persistent = expr.typeexpr.structType.flags.persistent
        }
        let bytes = expr.typeexpr.baseSizeOf
        new_ptr = build_alloc(bytes, persistent)
        check_ptr_zero(new_ptr, expr.at, "new returned null")
        if (expr.initializer) {
            call2cmres[get_expr_ptr(expr)] = new_ptr
            make_call(expr, false)
        } else {
            LLVMBuildMemSet(g_builder, new_ptr, 0ul, uint64(bytes), 16u)
        }
        return LLVMBuildPointerCast(g_builder, new_ptr, dim_element_type_to_llvm_type(expr._type), "new")
    }

    def get_new_handle_function(t : TypeDeclPtr; obj_name : DllName) : tuple<LLVMOpaqueValue?; LLVMOpaqueType?> {
        var new_handle = LLVMGetNamedGlobal(g_mod, obj_name.glob())
        let fn_type = LLVMFunctionType(types.LLVMVoidPtrType(),
                    fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType()))
        if (new_handle == null) {
            var new_handle_ptr = get_jit_new(t.annotation)
            if (new_handle_ptr == null) {
                failed_T(t, "missing new`handle function pointer for {describe(t)}")
                return tuple(null, null)
            }
            // void * new_handle ( Context * ) {
            new_handle = save_address_global(obj_name, new_handle_ptr, LLVMPointerType(fn_type, 0u))
            g_fn_types[obj_name.publ()] = fn_type
        } else {
            new_handle = get_address_global(obj_name, LLVMPointerType(fn_type, 0u))
        }
        return tuple(new_handle, fn_type)
    }

    def alloc_handle_type(typeexpr : TypeDeclPtr; obj_name_hint : DllName = DllName()) : LLVMOpaqueValue? {
        assert(typeexpr.isHandle)
        let obj_name = obj_name_hint.publ() |> empty() ? uid.get_new(typeexpr) : obj_name_hint
        let (fn_new_handle, typ) = get_new_handle_function(typeexpr, obj_name)
        if (fn_new_handle == null) {
            return LLVMConstNull(types.LLVMVoidPtrType())
        }
        var params = [get_context_param()]
        var ccall = LLVMBuildCall2(g_builder, typ, fn_new_handle, params, "call_{obj_name.publ()}")

        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.willreturn)
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.nounwind)
        LLVMAddCallSiteAttribute(ccall, 1 |> uint(), attributes.nocapture)

        return ccall
    }

    def override visitExprNew(expr : smart_ptr<ExprNew>) : ExpressionPtr {
        if (expr.typeexpr.isHandle) {
            var new_ptr = alloc_handle_type(expr.typeexpr)
            check_ptr_zero(new_ptr, expr.at, "new returned null")
            new_ptr = LLVMBuildPointerCast(g_builder, new_ptr, LLVMPointerType(type_to_llvm_type(expr.typeexpr), 0u), "")
            setE(expr, new_ptr)
        } else {
            if (expr.typeexpr.dim |> length > 0) {
                let count = expr.typeexpr.countOf
                var arr : LLVMOpaqueValue?
                at_function_entry <| $() {
                    arr = LLVMBuildAlloca(g_builder, type_to_llvm_type(expr._type), "new_arr")
                }
                build_fixed_loop(count) <| $(var index : LLVMOpaqueValue?) {
                    var elem_type = dim_element_type_to_llvm_type(expr._type)
                    var elem = LLVMBuildGEP2(g_builder, elem_type, arr, index, "")
                    var new_ptr : LLVMOpaqueValue?;
                    if (expr.typeexpr.baseType == Type.tHandle) {
                        var inscope elemT = clone_type(expr.typeexpr)
                        elemT.dim |> clear() // hack to get correct ptr to ctor
                        new_ptr = alloc_handle_type(elemT, uid.get_new(expr.typeexpr))
                        check_ptr_zero(new_ptr, expr.at, "new returned null")
                        new_ptr = LLVMBuildPointerCast(g_builder, new_ptr, LLVMPointerType(type_to_llvm_type(elemT), 0u), "")
                    } else {
                        new_ptr = new_entry(expr)
                    }
                    LLVMBuildStore(g_builder, new_ptr, elem)
                }
                setE(expr, arr)
            } else {
                setE(expr, new_entry(expr))
            }
        }
        return expr
    }

// ExprLooksLikeCall
    def override preVisitExprLooksLikeCall(expr : smart_ptr<ExprLooksLikeCall>) : void {
        pass // do nothing, works as is?
    }

// ExprCall
    def build_array_of_arguments(arguments : dasvector`smart_ptr`Expression; skip0 : bool = false) : LLVMOpaqueValue? {
        var nargs = length(arguments)
        var from_ai = 0
        if (skip0) {
            nargs --
            from_ai --
        }
        var args : LLVMOpaqueValue?
        if (nargs != 0) {
            at_function_entry <| $() {
                args = LLVMBuildAlloca(g_builder, LLVMArrayType(types.LLVMFloat4Type(), uint(nargs)), "args")
                LLVMSetAlignment(args, 16u)
                args = LLVMBuildPointerCast(g_builder, args, LLVMPointerType(types.LLVMFloat4Type(), 0u), "call_args_ptr")
            }
            for (a, ai in arguments, range(from_ai, 100500)) {
                if (ai < 0) {
                    continue
                }
                var arg_ptr = LLVMBuildGEP2(g_builder, types.LLVMFloat4Type(), args, types.ConstI32(uint64(ai)), "call_arg_{ai}_ptr")
                var arg_val = getE(a)
                var arg_cast_type = type_to_llvm_abi_type(a._type)
                if (a._type.isRef) {
                    arg_val = LLVMBuildPointerCast(g_builder, arg_val, arg_cast_type, "call_arg_{ai}_cast")
                } elif (need_z_or_s_ext(a._type) != null) {
                    // types must be zero/sign-extended to i32 before
                    // storing into the vec4f args slot to avoid garbage.
                    // Garbage can lead to incorrect conversion to bool.
                    if (a._type.isSignedInteger) {
                        arg_val = LLVMBuildSExt(g_builder, arg_val, types.t_int32, "sext_{ai}")
                    } else {
                        arg_val = LLVMBuildZExt(g_builder, arg_val, types.t_int32, "zext_{ai}")
                    }
                    arg_cast_type = types.t_int32
                }
                LLVMBuildStore2(g_builder, types, arg_cast_type, arg_val, arg_ptr)
            }
        } else {
            args = LLVMConstPointerNull(LLVMPointerType(types.LLVMFloat4Type(), 0u))
        }
        return args
    }

    def build_interop_call(func : Function?; fn_type : LLVMOpaqueType?; loaded_func : LLVMOpaqueValue?;
                           args : dasvector`smart_ptr`Expression; var params : array<LLVMOpaqueValue?>;
                           is_cmres : bool; name : string) {
        let start = is_cmres ? 1u : 0u
        var ccall = LLVMBuildCall2(g_builder, fn_type, loaded_func, params, name)
        if (func.sideEffectFlags == bitfield(0)) {
            LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.nounwind)     // TODO: better no-unwind?
            LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.willreturn)
        }
        for (arg, ai in args, ucount(start)) {
            let maybe_ext = need_z_or_s_ext(arg._type)
            if (!(is_cmres && ai == 0u) && maybe_ext != null) {
                LLVMAddCallSiteAttribute(ccall, ai + 1u, maybe_ext)
            }

            if (!(func.sideEffectFlags.modifyExternal || func.sideEffectFlags.modifyArgument || func.sideEffectFlags.invoke)) {
                if (arg._type.isRef) {
                    LLVMAddCallSiteAttribute(ccall, ai + 1u, attributes.nocapture)
                }
            }

        }
        return ccall
    }

    def need_z_or_s_ext(_type : TypeDeclPtr) {
        if (!_type.isRef && _type.sizeOf < 4 &&
            _type.baseType != Type.fakeContext && // incorrect size for fakeContext & fakeLineInfo
            _type.baseType != Type.fakeLineInfo) {
            if (_type.isSignedInteger) {
                return attributes.sext
            } else {
                // unsigned ints + bitfields + enums
                return attributes.zext
            }
        }
        return null
    }

    def register_extern_function(expr : smart_ptr<ExprCallFunc>) : tuple<LLVMOpaqueValue?, LLVMOpaqueType?> {
        let mangled_name = uid.get_dll_fn_name_ptr(expr.func)
        var ofunc = LLVMGetNamedGlobal(g_mod, mangled_name.glob())
        if (ofunc == null) {
            var arg_types : array<LLVMOpaqueType?>
            var res_type : LLVMOpaqueType?
            res_type = type_to_llvm_type(expr.func.result)
            if (expr.func.result.flags.ref) {
                res_type = LLVMPointerType(res_type, 0u)
            }
            let cmres = isCMRES(expr.func)
            if (cmres) {
                arg_types |> push(LLVMPointerType(res_type, 0u))
                res_type = types.t_void
            }
            for (arg in expr.func.arguments) {
                arg_types |> push(type_to_llvm_abi_type(arg._type))
            }
            if (expr.func.moreFlags.jitContextAndLineInfo) {
                arg_types |> push(types.LLVMVoidPtrType())  // context
                arg_types |> push(types.LLVMVoidPtrType())  // line info arg
            }
            let fn_type = LLVMFunctionType(res_type, arg_types)
            g_fn_types[mangled_name.publ()] = fn_type

            let fn_ptr_type = LLVMPointerType(fn_type, 0u)
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (FUNC_PTR == null) {
                failed_E(expr, "missing interop function pointer for {expr.func._module.name}::{expr.func.name} // {mangled_name}")
            }
            ofunc = save_address_global(mangled_name, FUNC_PTR, fn_ptr_type)
            return tuple(ofunc, fn_type)
        } else {
            ofunc = get_address_global(mangled_name, LLVMPointerType(g_fn_types[mangled_name.publ()], 0u))
            return tuple(ofunc, g_fn_types[mangled_name.publ()])
        }
    }

    def register_interop_function(expr : smart_ptr<ExprCallFunc>) : LLVMOpaqueValue? {
        let mangled_name = uid.get_dll_fn_name_ptr(expr.func)
        var ofunc = LLVMGetNamedGlobal(g_mod, mangled_name.glob())
        var fn_type = types.LLVMInteropFunctionType()
        let funcPtrType = LLVMPointerType(fn_type, 0u)
        if (ofunc == null) {
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (FUNC_PTR == null) {
                failed_E(expr, "missing interop function pointer for {expr.func._module.name}::{expr.func.name}")
            }
            ofunc = save_address_global(mangled_name, FUNC_PTR, funcPtrType)
        } else {
            ofunc = get_address_global(mangled_name, funcPtrType)
        }
        return ofunc
    }

    def build_call_params(expr : smart_ptr<ExprCallFunc>; extern_func : Function? = null) {
        var params : array<LLVMOpaqueValue?>

        for (a, ai in expr.arguments, count()) {
            assume funcArg = expr.func.arguments[ai]
            assume src = getE(a);
            if (funcArg._type.baseType == Type.anyArgument) {
                params |> push(cast_to_vec4f(a._type, src))
            } elif (a._type.baseType == Type.fakeContext) {
                params |> push(get_context_param())
            } elif (a._type.baseType == Type.fakeLineInfo) {
                params |> push(src)
            } elif (a._type.baseType == Type.tPointer) {
                var ptrType = type_to_llvm_abi_type(funcArg._type)
                var ptrValue = LLVMBuildPointerCast(g_builder, src, ptrType, "arg_{funcArg.name}_ptr")
                params |> push(ptrValue)
            } elif (a._type.baseType == Type.tStructure) {// possibly inherited structure
                var ptrType = type_to_llvm_abi_type(funcArg._type)
                var ptrValue = LLVMBuildPointerCast(g_builder, src, ptrType, "arg_{funcArg.name}_struct")
                params |> push(ptrValue)
            } elif ((a._type.baseType == Type.tArray || a._type.baseType == Type.tTable || a._type.dim |> length != 0) && (extern_func != null)) {
                var ptrType = type_to_llvm_abi_type(funcArg._type)
                var arg = LLVMBuildPointerCast(g_builder, src, ptrType, "any_array_{funcArg.name}_ptr")
                params |> push(arg)
            } elif (extern_func != null && extern_func.moreFlags.needStringCast && a._type.isString && !a._type.flags.ref) {
                // Wrap string argument for extern functions. Empty string in das is null.
                var res = LLVMBuildSelect(g_builder,
                    LLVMBuildIsNull(g_builder, src, ""),
                        get_empty_string(g_builder),
                        src,
                    "wrap_str")
                params |> push(res)
            } else {
                params |> push(src)
            }
        }
        return <- params
    }

    // All later loads will be replaced to the first one, assuming `val` is const.
    // We use it to optimize load of global pointers, since this pointers essentially
    // immutable in runtime
    def load_local_const(val : LLVMOpaqueValue?; type_ : LLVMOpaqueType?) {
        let load_inst = LLVMBuildLoad2(g_builder, type_, val, "")
        let msg = "invariant.load"
        LLVMSetMetadata(
            load_inst,
            LLVMGetMDKindIDInContext(g_ctx, msg, uint(length(msg))),
            LLVMMDNodeInContext(g_ctx, null, 0u))
        return load_inst
    }

    def get_address_global(name : DllName; typ : LLVMOpaqueType? = null) : LLVMOpaqueValue? {
        let nonnull_type = typ != null ? typ : types.LLVMVoidPtrType()
        let mb_global = LLVMGetNamedGlobal(g_mod, name.glob())
        if (mb_global == null) {
            return null
        }
        return load_local_const(mb_global, nonnull_type)
    }

    def save_address_global(name : DllName; address : void?; typ : LLVMOpaqueType? = null) {
        let nonnull_type = typ != null ? typ : types.LLVMVoidPtrType()
        var global_addr = LLVMGetNamedGlobal(g_mod, name.glob())
        if (global_addr == null) {
            global_addr = LLVMAddGlobal(g_mod, nonnull_type, name.glob())
            if (flags.gen_exe) {
                set_private_linkage(global_addr)
            } else {
                set_public_linkage(global_addr)
            }
            LLVMSetInitializer(global_addr, types.ConstPtr(address |> intptr, nonnull_type))
        } else {
            failed("Name duplicate {name.glob()}\n")
        }
        return load_local_const(global_addr, nonnull_type)
    }

    def set_meta_cconv(instr : LLVMOpaqueValue?; conv : string) {
        var values : LLVMOpaqueValue? [1]
        values[0] = LLVMMDStringInContext(g_ctx, conv, uint(length(conv)))
        let msg = "cconv"
        LLVMSetMetadata(
            instr,
            LLVMGetMDKindIDInContext(g_ctx, msg, uint(length(msg))),
            LLVMMDNodeInContext(g_ctx, array_data_ptr(values), 1u))
    }

    def make_call(expr : smart_ptr<ExprCallFunc>; doesNotNeedSp : bool) {
        // lookup intrinsics first
        var intrin_call = lookup_intinsic(g_ctx, g_builder, types, expr, [for (x in expr.arguments); getE(x)])
        if (intrin_call != null) {
            setE(expr, intrin_call)
            return
        }
        // get CMRES, if its local
        var cmresEval : LLVMOpaqueValue?
        if (isCMRES(expr.func)) {
            if (call2cmres |> key_exists(get_ptr(expr))) {
                cmresEval = call2cmres[get_expr_ptr(expr)]
            } elif (!doesNotNeedSp && expr.stackTop != 0u) {
                var vtype = type_to_llvm_type(expr._type)
                var v_ptr_type = get_type_pointer(expr._type)
                at_function_entry <| $() {
                    cmresEval = LLVMBuildAlloca(g_builder, vtype, "cmres_eval_{expr.name}")
                    LLVMSetAlignment(cmresEval, 16u)
                    cmresEval = LLVMBuildPointerCast(g_builder, cmresEval, v_ptr_type, "")
                }
            } else {
                cmresEval = LLVMConstPointerNull(types.LLVMVoidPtrType())
                failed_E(expr, "CMRES is not local, not supported yet")
            }
        }
        if (expr.func.flags.builtIn) {
            if (expr.func.flags.interopFn) {
                var args = build_array_of_arguments(expr.arguments)
                var func = register_interop_function(expr)

                var types_arr_ptr : LLVMOpaqueValue?
                let nArgs = expr.arguments |> length() |> uint
                if (nArgs > 0u) {
                    var tinfo_vals : array<LLVMValueRef>
                    for (elem in expr.arguments) {
                        let tinfo = make_type_info_structure(*jit_context, elem._type)
                        let tinfo_global = create_type_info_global(tinfo)
                        tinfo_vals |> push <| unsafe(reinterpret<LLVMValueRef>(tinfo_global))
                    }
                    let arr_ty = LLVMArrayType(types.LLVMVoidPtrType(), nArgs)
                    var ti_arr = LLVMAddGlobal(g_mod, arr_ty, "string_node_{expr.at.line:d}")
                    set_globalvar_linkage(ti_arr)
                    ti_arr |> LLVMSetInitializer <| LLVMConstArray(types.LLVMVoidPtrType(), array_data_ptr(tinfo_vals), nArgs)
                    types_arr_ptr = ti_arr
                } else {
                    types_arr_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
                }
                var jit_simnode_interop = LLVMGetNamedFunction(g_mod, "jit_simnode_interop")
                var simnode_interop_type = g_fn_types["jit_simnode_interop"]
                var typeinfo_memory : LLVMOpaqueValue?
                at_function_entry <| $() {
                    typeinfo_memory = LLVMBuildAlloca(g_builder,
                        LLVMArrayType(types.t_int8, SIZE_OF_SIMNODE_INTEROP), "SimNode")
                }

                LLVMBuildCall2(g_builder, simnode_interop_type,
                    jit_simnode_interop, fixed_array(
                        typeinfo_memory,
                        types.ConstI32(nArgs |> uint64),
                        types_arr_ptr
                    ), "")

                var params <- array<LLVMOpaqueValue?>(
                    get_context_param(),
                    typeinfo_memory,                    // Node *
                    args                                // args
                )
                // Load the function pointer
                var fn_type = types.LLVMInteropFunctionType()
                var ccall = LLVMBuildCall2(g_builder, fn_type, func, params, "")
                var jit_free_simnode_interop = LLVMGetNamedFunction(g_mod, "jit_free_simnode_interop")
                var simnode_free_interop_type = g_fn_types["jit_free_simnode_interop"]
                LLVMBuildCall2(g_builder, simnode_free_interop_type,
                    jit_free_simnode_interop, fixed_array(typeinfo_memory), "")
                if (expr.func.result.isVoid) {
                    setE(expr, ccall)
                } else {
                    var cres = cast_from_vec4f(expr, expr._type, ccall)
                    setE(expr, cres)
                }
            } else {
                var params <- build_call_params(expr, expr.func)
                var (func, typ) = register_extern_function(expr)
                if (expr.func.moreFlags.jitContextAndLineInfo) {
                    params |> push(get_context_param())
                    params |> push(get_line_info_ptr(expr.at))
                }
                if (cmresEval != null) {
                    var cmresPtr = LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(type_to_llvm_type(expr.func.result), 0u), "cmresPTR")
                    params |> push(cmresPtr, 0)
                    var ccall = build_interop_call(expr.func, typ, func, expr.arguments, params, true, "")
                    setE(expr, cmresEval)
                } else {
                    var ccall = build_interop_call(expr.func, typ, func, expr.arguments, params, false, "")
                    setE(expr, ccall)
                }
            }
        } else {
            if (expr.func.moreFlags.requestJit) {
                var params <- build_call_params(expr)
                params |> push(get_context_param())
                if (cmresEval != null) {
                    params |> push(LLVMBuildPointerCast(g_builder, cmresEval, types.LLVMVoidPtrType(), ""))
                }
                let fmna = uid.get_dll_fn_name_ptr(expr.func)
                var func = LLVMGetNamedFunction(g_mod, fmna.impl())
                var typ = g_fn_types[fmna.impl()]
                var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
                if (cmresEval != null) {
                    setE(expr, cmresEval)
                } else {
                    setE(expr, ccall)
                }
            } else {
                var args = build_array_of_arguments(expr.arguments)
                var glob_fn_ptr = get_address_global(uid.get_dll_fn_name_ptr(expr.func))
                if (glob_fn_ptr == null) {
                    var MNH_ADDR = get_function_addr(jit_context, expr.func)
                    glob_fn_ptr = save_address_global(uid.get_dll_fn_name_ptr(expr.func), MNH_ADDR)
                }
                var params : array<LLVMOpaqueValue?>
                params |> push(glob_fn_ptr)  // mnh
                params |> push(args)
                if (cmresEval != null) {
                    params |> push(LLVMBuildPointerCast(g_builder, cmresEval, types.LLVMVoidPtrType(), ""))
                }
                params |> push(get_context_param())
                var func = LLVMGetNamedFunction(g_mod, cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
                var typ = g_fn_types[cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
                var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
                if (cmresEval != null) {
                    setE(expr, cmresEval)
                } elif (expr.func.result.isVoid) {
                    setE(expr, ccall)
                } else {
                    var cres = cast_from_vec4f(expr, expr._type, ccall)
                    setE(expr, cres)
                }
            }
        }
    }

    def override visitExprCall(expr : smart_ptr<ExprCall>) : ExpressionPtr {
        make_call(expr, expr.doesNotNeedSp)
        return expr
    }

    def override canVisitCall(expr : ExprCall?) : bool {
        return !skipCall |> key_exists(expr)
    }

    def override canVisitWithAliasSubexpression(expr : smart_ptr<ExprAssume>) : bool {
        return false
    }

// ExprNullCoalescing
    def override preVisitExprNullCoalescing(expr : smart_ptr<ExprNullCoalescing>) : void {
        var blk = IteBlock(
            if_true   = append_basic_block("nullc_null_at_line_{int(expr.at.line)}_"),
            if_false  = append_basic_block("nullc_not_null_at_line_{int(expr.at.line)}_"),
            if_end    = append_basic_block("nullc_end_at_line_{int(expr.at.line)}_"),
            phi_true  = append_basic_block("nullc_phi_true_at_line_{int(expr.at.line)}_"),
            phi_false = append_basic_block("nullc_phi_false_at_line_{int(expr.at.line)}_")
        )
        ite2blocks[get_expr_ptr(expr)] = blk
    }

    def override preVisitExprNullCoalescingDefault(expr : smart_ptr<ExprNullCoalescing>; defval : ExpressionPtr) : void {
        var blk = ite2blocks[get_expr_ptr(expr)]
        var sE = getE(expr.subexpr)
        var cond = LLVMBuildIsNotNull(g_builder, sE, "")
        LLVMBuildCondBr(g_builder, cond, blk.if_false, blk.if_true)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_false)
        if (expr._type.isRef) {
            pass
        } else {
            setE(expr.subexpr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), sE, expr._type.alignOf, ""))
        }
        LLVMBuildBr(g_builder, blk.phi_true)
        LLVMPositionBuilderAtEnd(g_builder, blk.phi_true)
        LLVMBuildBr(g_builder, blk.if_end)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_true)
    }

    def override visitExprNullCoalescing(expr : smart_ptr<ExprNullCoalescing>) : ExpressionPtr {
        var blk = ite2blocks[get_expr_ptr(expr)]
        var sE = getE(expr.subexpr)
        var dE = getE(expr.defaultValue)
        if (expr._type.isPointer) {// could be null there, right? TODO: should we try ref
            sE = LLVMBuildPointerCast(g_builder, sE, LLVMPointerType(type_to_llvm_type(expr._type.firstType), 0u), "")
            dE = LLVMBuildPointerCast(g_builder, dE, LLVMPointerType(type_to_llvm_type(expr._type.firstType), 0u), "")
        }
        LLVMBuildBr(g_builder, blk.phi_false)
        LLVMPositionBuilderAtEnd(g_builder, blk.phi_false)
        LLVMBuildBr(g_builder, blk.if_end)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_end)
        var phi = LLVMBuildPhi(g_builder, type_to_llvm_abi_type(expr._type), "null_c")
        LLVMAddIncoming(phi, fixed_array(sE, dE), fixed_array(blk.phi_true, blk.phi_false))
        setE(expr, phi)
        return expr
    }

// ExprAt
    def check_range(tidx, maxIdx : LLVMOpaqueValue?; at : LineInfo; message : string) {
        if (option_no_range_check) {
            return
        }
        var check_null_ptr = append_basic_block("check_dim_range")
        var check_true = append_basic_block("check_true")
        var check_end = append_basic_block("check_end")
        LLVMBuildBr(g_builder, check_null_ptr)
        LLVMPositionBuilderAtEnd(g_builder, check_null_ptr)
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntUGE, tidx, maxIdx, "range_check")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_true)
        build_exception(message, at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_end)
    }

    def build_table_at(ttype : Type; tab : LLVMOpaqueValue?; key : LLVMOpaqueValue?; valueTypeSize : int, at : LineInfo) : LLVMOpaqueValue? {
        var params = fixed_array(
            tab,                                                        // table ptr
            key,                                                        // key
            types.ConstI32(uint64(valueTypeSize)),    // valueTypeSize
            get_context_param(),
            get_line_info_ptr(at)
        )
        var (tab_fun, typ) = get_table_at(g_builder, g_mod, ttype, types)
        let ccall = LLVMBuildCall2(g_builder, typ, tab_fun, params, "")
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.nounwind)     // TODO: better no-unwind?
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.willreturn)

        LLVMAddCallSiteAttribute(ccall, 1u, attributes.nocapture)
        LLVMAddCallSiteAttribute(ccall, 4u, attributes.nocapture)
        return ccall
    }

    def build_table_find(ttype : Type; tab : LLVMOpaqueValue?; key : LLVMOpaqueValue?; valueTypeSize : int) : LLVMOpaqueValue? {
        var params = fixed_array(
            tab,                                                        // table ptr
            key,                                                        // key
            types.ConstI32(uint64(valueTypeSize)),    // valueTypeSize
            get_context_param()
        )
        var (fun_ptr, typ) = get_table_find(g_builder, g_mod, ttype, types)
        let ccall = LLVMBuildCall2(g_builder, typ, fun_ptr, params, "")
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.nounwind)     // TODO: better no-unwind?
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.willreturn)

        LLVMAddCallSiteAttribute(ccall, 1u, attributes.nocapture)
        LLVMAddCallSiteAttribute(ccall, 4u, attributes.nocapture)
        return ccall
    }

    def build_array_index(var data, tidx : LLVMOpaqueValue?; elemType : TypeDeclPtr; name : string) {
        var ptr_idx : LLVMOpaqueValue?
        if (elemType.isVectorType && elemType.vectorDim == 3) {
            var data_bytes = LLVMBuildPointerCast(g_builder, data, LLVMPointerType(types.t_int8, 0u), "")
            var data_idx = LLVMBuildMul(g_builder, tidx, types.ConstI32(12ul), "")
            ptr_idx = LLVMBuildGEP2(g_builder, types.t_int8, data_bytes, data_idx, "")
            ptr_idx = LLVMBuildPointerCast(g_builder, ptr_idx, LLVMPointerType(type_to_llvm_type(elemType), 0u), name)
        } else {
            ptr_idx = LLVMBuildGEP2(g_builder, type_to_llvm_type(elemType), data, tidx, name)
        }
        return ptr_idx
    }

    def build_vector_index(var data, tidx : LLVMOpaqueValue?; elemType : TypeDeclPtr; name : string) {
        assert(elemType.isVectorType)
        var elType = base_type_to_llvm_type(elemType.vectorBaseType)
        if (!elemType.isRef) {
            return LLVMBuildExtractElement(g_builder, data, tidx, "vector_at_r2v")
        } else {
            return LLVMBuildGEP2(g_builder, elType, data, tidx, name)
        }
    }

    def override visitExprAt(expr : smart_ptr<ExprAt>) : ExpressionPtr {
        var tsrc = getE(expr.subexpr)
        var tidx = getE(expr.index)
        assume subExprT = expr.subexpr._type
        if (subExprT.isPointer) {
            if (thisFunc != null && !thisFunc.flags.unsafeDeref) {
                check_ptr_zero(tsrc, expr.at, "dereferencing null pointer, {expr.subexpr.describe()} is null")
            }
            var ptr_idx = build_array_index(tsrc, tidx, subExprT.firstType, "ptr_at")
            if (expr.atFlags.r2v) {
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_abi_type(expr._type),
                    ptr_idx, expr._type.alignOf, "at_r2v")
                setE(expr, at_r2v)
            } else {
                setE(expr, ptr_idx)
            }
        } elif (subExprT.dim |> length != 0) {
            var inscope etype <- clone_type(subExprT)
            assume vec = etype.dim
            let maxIndex = vec[0]
            for (i in range(1, length(vec))) {
                vec[i - 1] = vec[i]
            }
            vec |> resize(vec |> length - 1)
            check_range(tidx, types.ConstI32(uint64(maxIndex)), expr.at, "dim index out of range")
            var ptr_idx = build_array_index(tsrc, tidx, etype, "dim_at")
            if (expr.atFlags.r2v) {
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_abi_type(expr._type),
                    ptr_idx, expr._type.alignOf, "dim_at_r2v")
                setE(expr, at_r2v)
            } else {
                setE(expr, ptr_idx)
            }
        } elif (subExprT.isGoodArrayType) {
            var arr = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), tsrc, expr.subexpr._type.alignOf, "arr")
            var size = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY.SIZE), "array.size")
            check_range(tidx, size, expr.at, "array index out of range")
            var data = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY.DATA), "array.data")
            var ptr_idx = build_array_index(data, tidx, subExprT.firstType, "array_at")
            if (expr.atFlags.r2v) {
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type),
                    ptr_idx, expr._type.alignOf, "array_at_r2v")
                setE(expr, at_r2v)
            } else {
                setE(expr, ptr_idx)
            }
        } elif (subExprT.isGoodTableType) {
            var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "tab_ptr")
            var keyType : Type
            if (subExprT.firstType.isWorkhorseType) {
                keyType = subExprT.firstType.baseType
            } else {
                var inscope uvt <- get_underlying_value_type(subExprT.firstType)
                keyType = uvt.baseType
            }
            var at_tab = build_table_at(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf, expr.at)
            var tab = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), tsrc, expr.subexpr._type.alignOf, "tab")
            var data = LLVMBuildExtractValue(g_builder, tab, uint(JIT_TABLE.DATA), "table.data")
            var ptr_idx = LLVMBuildGEP2(g_builder, type_to_llvm_type(expr._type), data, at_tab, "table_at")
            if (expr.atFlags.r2v) {
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type),
                    ptr_idx, expr._type.alignOf, "array_at_r2v")
                setE(expr, at_r2v)
            } else {
                setE(expr, ptr_idx)
            }
        } elif (subExprT.isVectorType) {
            check_range(tidx, types.ConstI32(uint64(subExprT.vectorDim)), expr.at, "vector index out of range")
            if (expr.atFlags.r2v) {
                var lvec = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type),
                    tsrc, subExprT.alignOf, "vector_at")
                var at_r2v = LLVMBuildExtractElement(g_builder, lvec, tidx, "vector_at_r2v")
                setE(expr, at_r2v)
            } else {
                var ptr_idx = build_vector_index(tsrc, tidx, subExprT, "vector_at")
                setE(expr, ptr_idx)
            }
        } else {
            failed("unsupported visitExprAt type {describe(subExprT)} {describe(expr.at)}")
        }
        return expr
    }

// ExprSafeAt
    def override visitExprSafeAt(expr : smart_ptr<ExprSafeAt>) : ExpressionPtr {
        var tsrc = getE(expr.subexpr)
        var tidx = getE(expr.index)
        assume subExprT = expr.subexpr._type
        var res : LLVMOpaqueValue?;
        if (subExprT.dim |> length != 0 || (subExprT.isPointer && subExprT.firstType.dim |> length != 0)) {
            var inscope etype <- subExprT.isPointer ? clone_type(subExprT.firstType) : clone_type(subExprT)
            assume vec = etype.dim
            let maxIndex = vec[0]
            for (i in range(1, length(vec))) {
                vec[i - 1] = vec[i]
            }
            vec |> resize(vec |> length - 1)
            var resType = LLVMPointerType(type_to_llvm_type(etype), 0u)
            var if_not_null = subExprT.dim |> length == 0 ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(types.t_int1, 1ul, 0)
            res = build_select(if_not_null, resType) <| $() {
                var maxIdx = types.ConstI32(uint64(maxIndex))
                var if_range = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntULT, tidx, maxIdx, "dim_range")
                var subRes = build_select(if_range, resType) <| $() {
                    return build_array_index(tsrc, tidx, etype, "dim_at")
                }
                lpipe <| $() {
                    return LLVMConstNull(resType)
                }
                return subRes
            }
            lpipe <| $() {
                return LLVMConstNull(resType)
            }
        } elif (subExprT.isGoodArrayType || (subExprT.isPointer && subExprT.firstType.isGoodArrayType)) {
            var if_not_null = !subExprT.isGoodArrayType ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(types.t_int1, 1ul, 0)
            var inscope etype := subExprT.isPointer ? subExprT.firstType.firstType : subExprT.firstType
            var resType = LLVMPointerType(type_to_llvm_type(etype), 0u)
            res = build_select(if_not_null, resType) <| $() {
                var inscope arr_ty := expr.subexpr._type.isPointer ? expr.subexpr._type.firstType : expr.subexpr._type
                var arr = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(arr_ty), tsrc, arr_ty.alignOf, "arr")
                var size = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY.SIZE), "array.size")
                var if_range = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntULT, tidx, size, "array_range")
                var subRes = build_select(if_range, resType) <| $() {
                    var data = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY.DATA), "array.data")
                    return build_array_index(data, tidx, etype, "array_at")
                }
                lpipe <| $() {
                    return LLVMConstNull(resType)
                }
                return subRes
            }
            lpipe <| $() {
                return LLVMConstNull(resType)
            }
        } elif (subExprT.isGoodTableType || (subExprT.isPointer && subExprT.firstType.isGoodTableType)) {
            var if_not_null = !subExprT.isGoodTableType ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(types.t_int1, 1ul, 0)
            var inscope etype := subExprT.isPointer ? subExprT.firstType : subExprT
            var resType = LLVMPointerType(type_to_llvm_type(etype.secondType), 0u)
            res = build_select(if_not_null, resType) <| $() {
                var inscope tab_ty := expr.subexpr._type.isPointer ? expr.subexpr._type.firstType : expr.subexpr._type
                var tab = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(tab_ty), tsrc, tab_ty.alignOf, "tab")
                var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "tab_ptr")
                var keyType : Type
                if (etype.firstType.isWorkhorseType) {
                    keyType = etype.firstType.baseType
                } else {
                    var inscope uvt <- get_underlying_value_type(etype.firstType)
                    keyType = uvt.baseType
                }
                var at_tab = build_table_find(keyType, tab_ptr, tidx, etype.secondType.sizeOf)
                var if_found = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, at_tab, types.ConstI32(uint64(-1)), "table_found")
                var subRes = build_select(if_found, resType) <| $() {
                    var data = LLVMBuildExtractValue(g_builder, tab, uint(JIT_TABLE.DATA), "table.data")
                    return LLVMBuildGEP2(g_builder, type_to_llvm_type(etype.secondType), data, at_tab, "table_at")
                }
                lpipe <| $() {
                    return LLVMConstNull(resType)
                }
                return subRes
            }
            lpipe <| $() {
                return LLVMConstNull(resType)
            }
        } elif (subExprT.isVectorType || (subExprT.isPointer && subExprT.firstType.isVectorType)) {
            var if_not_null = !subExprT.isVectorType ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(types.t_int1, 1ul, 0)
            var inscope etype := subExprT.isPointer ? subExprT.firstType : subExprT
            var resType = LLVMPointerType(type_to_llvm_type(etype), 0u)
            res = build_select(if_not_null, resType) <| $() {
                var if_range = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntULT, tidx, types.ConstI32(uint64(etype.vectorDim)), "dim_range")
                var subRes = build_select(if_range, resType) <| $() {
                    if (expr.atFlags.r2v) {
                        var lvec = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type),
                            tsrc, subExprT.alignOf, "vector_at")
                        return LLVMBuildExtractElement(g_builder, lvec, tidx, "vector_at_r2v")
                    } elif (subExprT.isPointer) {
                        return build_array_index(tsrc, tidx, expr._type, "vector_at")
                    } else {
                        return build_vector_index(tsrc, tidx, etype, "vector_at")
                    }
                }
                lpipe <| $() {
                    return LLVMConstNull(resType)
                }
                return subRes
            }
            lpipe <| $() {
                return LLVMConstNull(resType)
            }
        } elif (subExprT.isPointer) {
            // pointer safe-at: null check + GEP, no bounds check
            var inscope etype := subExprT.firstType
            var resType = LLVMPointerType(type_to_llvm_type(etype), 0u)
            var if_not_null = LLVMBuildIsNotNull(g_builder, tsrc, "ptr_not_null")
            res = build_select(if_not_null, resType) <| $() {
                return build_array_index(tsrc, tidx, etype, "ptr_safe_at")
            }
            lpipe <| $() {
                return LLVMConstNull(resType)
            }
        } else {
            res = LLVMConstNull(type_to_llvm_type(expr._type))
            failed("unsupported visitExprSafeAt type {describe(subExprT)} {describe(expr.at)}")
        }
        setE(expr, res)
        return expr
    }

// ExprOp2
    def isReadModifyWriteOp2(name : das_string) {
        return (name == "+=" || name == "-=" || name == "*=" || name == "/=" || name == "%=" ||
            name == "&=" || name == "|=" || name == "^=" || name == "<<=" || name == ">>=" ||
            name == "<<<=" || name == ">>>=" || name == "&&=" || name == "||=" || name == "^^=")
    }

    def get_bool_compare_op(e : ExpressionPtr; op : string) {
        return LLVMIntPredicate.LLVMIntEQ   if (op == "==")
        return LLVMIntPredicate.LLVMIntNE   if (op == "!=")
        failed_E(e, "unsupported bool compare operator {op}")
        return LLVMIntPredicate.LLVMIntEQ
    }

    def get_int_compare_op(e : ExpressionPtr; op : string) {
        return LLVMIntPredicate.LLVMIntEQ   if (op == "==")
        return LLVMIntPredicate.LLVMIntNE   if (op == "!=")
        return LLVMIntPredicate.LLVMIntSLT  if (op == "<")
        return LLVMIntPredicate.LLVMIntSLE  if (op == "<=")
        return LLVMIntPredicate.LLVMIntSGT  if (op == ">")
        return LLVMIntPredicate.LLVMIntSGE  if (op == ">=")
        failed_E(e, "unsupported int compare operator {op}")
        return LLVMIntPredicate.LLVMIntEQ
    }

    def get_uint_compare_op(e : ExpressionPtr; op : string) {
        return LLVMIntPredicate.LLVMIntEQ   if (op == "==")
        return LLVMIntPredicate.LLVMIntNE   if (op == "!=")
        return LLVMIntPredicate.LLVMIntULT  if (op == "<")
        return LLVMIntPredicate.LLVMIntULE  if (op == "<=")
        return LLVMIntPredicate.LLVMIntUGT  if (op == ">")
        return LLVMIntPredicate.LLVMIntUGE  if (op == ">=")
        failed_E(e, "unsupported uint compare operator {op}")
        return LLVMIntPredicate.LLVMIntEQ
    }

    def get_float_compare_op(e : ExpressionPtr; op : string) {
        return LLVMRealPredicate.LLVMRealOEQ    if (op == "==")
        return LLVMRealPredicate.LLVMRealUNE    if (op == "!=")
        return LLVMRealPredicate.LLVMRealOLT    if (op == "<")
        return LLVMRealPredicate.LLVMRealOLE    if (op == "<=")
        return LLVMRealPredicate.LLVMRealOGT    if (op == ">")
        return LLVMRealPredicate.LLVMRealOGE    if (op == ">=")
        failed_E(e, "unsupported float compare operator {op}")
        return LLVMRealPredicate.LLVMRealOEQ
    }

    def check_divide_by_0(right : LLVMOpaqueValue?; at : LineInfo; opType : TypeDeclPtr, op : string) {
        var check_div_0 = append_basic_block("check_div_0")
        var check_true = append_basic_block("check_true")
        var check_end = append_basic_block("check_end")
        LLVMBuildBr(g_builder, check_div_0)
        LLVMPositionBuilderAtEnd(g_builder, check_div_0)
        var zero = LLVMConstNull(type_to_llvm_type(opType))
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, right, zero, "cmp_is_zero")
        if (opType.isVectorType) {
            cond = reduce_bit_vector(cond, opType.vectorDim, "!=")
        }
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_true)
        if (op == "%" || op == "%=") {
            build_exception("division by zero in modulo", at)
        } else {
            build_exception("division by zero", at)
        }
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_end)
    }

    def isExprIntNZ(expr : ExpressionPtr) {
        return true     if ((expr is ExprConstInt) && (expr as ExprConstInt).value != 0)
        return true     if ((expr is ExprConstUInt) && (expr as ExprConstUInt).value != 0u)
        return true     if ((expr is ExprConstInt64) && (expr as ExprConstInt64).value != 0l)
        return true     if ((expr is ExprConstUInt64) && (expr as ExprConstUInt64).value != 0ul)
        return false
    }

    def build_str_cmp(a, b : LLVMOpaqueValue?) : LLVMOpaqueValue? {
        var typ = g_fn_types[FN_JIT_STR_CMP]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_STR_CMP), fixed_array(a, b), "")
    }

    def build_str_cat(a, b : LLVMOpaqueValue?; at : LineInfo) : LLVMOpaqueValue? {
        var typ = g_fn_types[FN_JIT_STR_CAT]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_STR_CAT),
            fixed_array(a, b, get_context_param(), get_line_info_ptr(at)), "")
    }


    def reduce_bit_vector(i1v : LLVMOpaqueValue?; bits : int; op : string) {
        var result = LLVMBuildExtractElement(g_builder, i1v, types.ConstI32(0ul), "bit_0")
        for (b in range(1, bits)) {
            var bit = LLVMBuildExtractElement(g_builder, i1v, types.ConstI32(uint64(b)), "bit_{b}")
            if (op == "==") {
                result = LLVMBuildAnd(g_builder, result, bit, "")
            } elif (op == "!=") {
                result = LLVMBuildOr(g_builder, result, bit, "")
            } else {
                error("unsupported bit reduction operator {op}")
            }
        }
        return result
    }

    def visitExprOp2_Func(var expr : smart_ptr<ExprOp2>) {
        expr.arguments |> resize(2)
        expr.arguments[0] := expr.left
        expr.arguments[1] := expr.right
        make_call(expr, false)
    }

    def isNumeric(opType : TypeDeclPtr) {
        return opType.isInteger || (opType.isVectorType && (opType.vectorBaseType == Type.tInt || opType.vectorBaseType == Type.tUInt));
    }

    def isEnumeration(opType : TypeDeclPtr) {
        return (opType.baseType == Type.tEnumeration ||
               opType.baseType == Type.tEnumeration8 ||
               opType.baseType == Type.tEnumeration16 ||
               opType.baseType == Type.tEnumeration64)
    }

    def isBinary(opType : TypeDeclPtr) {
        return isNumeric(opType) || isEnumeration(opType)
    }

    def visitExprOp2_NonMonade(var expr : smart_ptr<ExprOp2>) {
        var left = getE(expr.left)
        var right = getE(expr.right)
        assume shiftType = expr.left._type
        assume rightType = expr.right._type
        var r2v_left = isReadModifyWriteOp2(expr.op) ? LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.left._type),
                                                            left, expr.left._type.alignOf, "") : null
        // note: we need to handle vector vs scalar operations here
        // the way we do it, is we expand scalar up to vector type
        var use_left_type = false
        if (expr.op == "*" || expr.op == "*=" || expr.op == "/" || expr.op == "/=") {
            if (shiftType.baseType != rightType.baseType) {// vec * scalar || scalar * vec, and so on
                if (rightType.isVectorType) {
                    assert(expr.op == "*" || expr.op == "/")// scalar * vec || scalar / vec only
                    left = expand_scalar(g_builder, left, rightType)
                } else {
                    right = expand_scalar(g_builder, right, shiftType)
                    use_left_type = true
                }
            }
        }
        assume opType = use_left_type ? expr.left._type : expr.right._type
        // note: we need to handle shift operations
        // the way we do it, is we expand scalar up to vector type
        if (expr.op == "<<" || expr.op == ">>" || expr.op == ">>>" || expr.op == "<<<" || expr.op == "<<=" || expr.op == ">>=" || expr.op == ">>>=" || expr.op == "<<<=") {
            // Create a mask value: (sizeof(element_type) * 8) - 1
            verify(isNumeric(opType))
            right = LLVMBuildAnd(g_builder, right, LLVMConstInt(type_to_llvm_type(opType), uint64((opType.sizeOf * 8) - 1), 0), "")
            right = build_broadcast_vector(g_builder, shiftType, right)
        }
        if (expr.op == "+") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildAdd(g_builder, left, right, ""))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildFAdd(g_builder, left, right, ""))
            } elif (opType.isString) {
                setE(expr, build_str_cat(left, right, expr.at))
            } else {
                failed_E(expr, "unsupported + type {describe(opType)}")
            }
        } elif (expr.op == "+=") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildAdd(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFAdd(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } elif (opType.isString) {
                setE(expr, LLVMBuildStoreAligned(g_builder, build_str_cat(r2v_left, right, expr.at), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported += type {describe(opType)}")
            }
        } elif (expr.op == "-") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildSub(g_builder, left, right, ""))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildFSub(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported - type {describe(opType)}")
            }
        } elif (expr.op == "-=") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSub(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFSub(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported -= type {describe(opType)}")
            }
        } elif (expr.op == "*") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildMul(g_builder, left, right, ""))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildFMul(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported * type {describe(opType)}")
            }
        } elif (expr.op == "*=") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildMul(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFMul(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported *= type {describe(opType)}")
            }
        } elif (expr.op == "/" || expr.op == "%" || expr.op == "/=" || expr.op == "%=") {
            // for integer division we check division by zero
            if (isNumeric(opType) && isNumeric(expr.left._type)) {
                if (!isExprIntNZ(expr.right)) {
                    check_divide_by_0(right, expr.at, opType, string(expr.op))
                }
            }
            if (expr.op == "/") {
                if (opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tInt)) {
                    setE(expr, LLVMBuildSDiv(g_builder, left, right, ""))
                } elif (opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tUInt)) {
                    setE(expr, LLVMBuildUDiv(g_builder, left, right, ""))
                } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                    setE(expr, LLVMBuildFDiv(g_builder, left, right, ""))
                } else {
                    failed_E(expr, "unsupported /= type {describe(opType)}")
                }
            } elif (expr.op == "/=") {
                if (opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tInt)) {
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSDiv(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                } elif (opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tUInt)) {
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildUDiv(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFDiv(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                } else {
                    failed_E(expr, "unsupported /= type {describe(opType)}")
                }
            } elif (expr.op == "%") {
                if (opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tInt)) {
                    if (opType.baseType == Type.tInt) {
                        setE(expr, LLVMBuildSRemInt32(g_builder, types, left, right, ""))
                    } else {
                        setE(expr, LLVMBuildSRem(g_builder, left, right, ""))
                    }
                } elif (opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tUInt)) {
                    if (opType.baseType == Type.tUInt) {
                        setE(expr, LLVMBuildURemUInt32(g_builder, types, left, right, ""))
                    } else {
                        setE(expr, LLVMBuildURem(g_builder, left, right, ""))
                    }
                } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                    setE(expr, LLVMBuildFRem(g_builder, left, right, ""))
                } else {
                    failed_E(expr, "unsupported % type {describe(opType)}")
                }
            } elif (expr.op == "%=") {
                if (opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tInt)) {
                    if (opType.baseType == Type.tInt) {
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSRemInt32(g_builder, types, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                    } else {
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSRem(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                    }
                } elif (opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType == Type.tUInt)) {
                    if (opType.baseType == Type.tUInt) {
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildURemUInt32(g_builder, types, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                    } else {
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildURem(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                    }
                } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFRem(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                } else {
                    failed_E(expr, "unsupported %= type {describe(opType)}")
                }
            } else {
                failed_E(expr, "unsupported division type operator {expr.op}")
            }
        } elif (expr.op == "<" || expr.op == ">" || expr.op == "<=" || expr.op == ">=" || expr.op == "==" || expr.op == "!=") {
            var opR : LLVMOpaqueValue?
            if (expr.left._type.isSignedInteger || (expr.left._type.isVectorType && (expr.left._type.vectorBaseType == Type.tInt || expr.left._type.vectorBaseType == Type.tInt64)) || expr.left._type.isEnum) {
                opR = LLVMBuildICmp(g_builder, get_int_compare_op(expr, string(expr.op)), left, right, "")
                if (expr.left._type.isVectorType) {
                    opR = reduce_bit_vector(opR, expr.left._type.vectorDim, string(expr.op))
                }
            } elif (expr.left._type.isUnsignedInteger || (expr.left._type.isVectorType && (expr.left._type.vectorBaseType == Type.tUInt || expr.left._type.vectorBaseType == Type.tUInt64))) {
                opR = LLVMBuildICmp(g_builder, get_uint_compare_op(expr, string(expr.op)), left, right, "")
                if (expr.left._type.isVectorType) {
                    opR = reduce_bit_vector(opR, expr.left._type.vectorDim, string(expr.op))
                }
            } elif (expr.left._type.isFloatOrDouble || (expr.left._type.isVectorType && expr.left._type.vectorBaseType == Type.tFloat)) {
                opR = LLVMBuildFCmp(g_builder, get_float_compare_op(expr, string(expr.op)), left, right, "")
                if (expr.left._type.isVectorType) {
                    opR = reduce_bit_vector(opR, expr.left._type.vectorDim, string(expr.op))
                }
            } elif (expr.left._type.baseType == Type.tBool) {
                opR = LLVMBuildICmp(g_builder, get_bool_compare_op(expr, string(expr.op)), left, right, "")
            } elif (expr.left._type.baseType == Type.tPointer || expr.left._type.baseType == Type.tLambda) {
                var lptr = LLVMBuildPointerCast(g_builder, left, LLVMPointerType(types.t_int8, 0u), "")
                var rptr = LLVMBuildPointerCast(g_builder, right, LLVMPointerType(types.t_int8, 0u), "")
                opR = LLVMBuildICmp(g_builder, get_uint_compare_op(expr, string(expr.op)), lptr, rptr, "")
            } elif (expr.left._type.baseType == Type.tString) {
                var cmpR = build_str_cmp(left, right)
                opR = LLVMBuildICmp(g_builder, get_int_compare_op(expr, string(expr.op)), cmpR, types.ConstI32(0ul), "")
            } elif (expr.left._type.baseType == Type.tFunction) {
                var lptr = (expr.left is ExprConstPtr) ? left : LLVMBuildExtractValue(g_builder, left, uint(JIT_FUNCTION.SIM_FUNCTION), "SIMFUNCTION")
                var rptr = (expr.right is ExprConstPtr) ? right : LLVMBuildExtractValue(g_builder, right, uint(JIT_FUNCTION.SIM_FUNCTION), "SIMFUNCTION")
                lptr = LLVMBuildPointerCast(g_builder, lptr, LLVMPointerType(types.t_int8, 0u), "")
                rptr = LLVMBuildPointerCast(g_builder, rptr, LLVMPointerType(types.t_int8, 0u), "")
                opR = LLVMBuildICmp(g_builder, get_uint_compare_op(expr, string(expr.op)), lptr, rptr, "")
            }
            if (opR == null) {
                opR = LLVMGetUndef(types.t_int8)
                failed_E(expr, "unsupported compare type {describe(expr.left._type)} {expr.op} {describe(expr.right._type)}")
            }
            setE(expr, opR)
        } elif (expr.op == "<<") {
            if (shiftType.isInteger || (shiftType.isVectorType && (shiftType.vectorBaseType == Type.tInt || shiftType.vectorBaseType == Type.tUInt))) {
                setE(expr, LLVMBuildShl(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported << type {describe(shiftType)}")
            }
        } elif (expr.op == "<<=") {
            if (shiftType.isInteger || (shiftType.isVectorType && (shiftType.vectorBaseType == Type.tInt || shiftType.vectorBaseType == Type.tUInt))) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildShl(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported <<= type {describe(shiftType)}")
            }
        } elif (expr.op == ">>") {
            if (shiftType.isSignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType == Type.tInt)) {
                setE(expr, LLVMBuildAShr(g_builder, left, right, ""))
            } elif (shiftType.isUnsignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType == Type.tUInt)) {
                setE(expr, LLVMBuildLShr(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported >> type {describe(shiftType)}")
            }
        } elif (expr.op == ">>=") {
            if (shiftType.isSignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType == Type.tInt)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildAShr(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } elif (shiftType.isUnsignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType == Type.tUInt)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildLShr(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported >>= type {describe(shiftType)}")
            }
        } elif (expr.op == ">>>" || expr.op == "<<<") {
            if (shiftType.isInteger) {
                var fshr_name = expr.op == ">>>" ? "llvm.fshr.i32" : "llvm.fshl.i32"
                if (shiftType.baseType == Type.tInt64 || shiftType.baseType == Type.tUInt64) {
                    fshr_name = expr.op == ">>>" ? "llvm.fshr.i64" : "llvm.fshl.i64"
                }
                var args <- [ left, left, right]
                var argTypes <- [ base_type_to_llvm_type(shiftType.baseType)]
                var id = LLVMLookupIntrinsicID(fshr_name)
                var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
                if (decl == null) {
                    failed_E(expr, "failed to get intrinsic {fshr_name}")
                } else {
                    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], argTypes[0], argTypes[0]])
                    setE(expr, LLVMBuildCall2(g_builder, typ, decl, args, string(expr.op)))
                }
            } else {
                failed_E(expr, "unsupported >>> type {opType}")
            }
        } elif (expr.op == ">>>=" || expr.op == "<<<=") {
            if (shiftType.isInteger) {
                var fshr_name = expr.op == ">>>=" ? "llvm.fshr.i32" : "llvm.fshl.i32"
                if (shiftType.baseType == Type.tInt64 || shiftType.baseType == Type.tUInt64) {
                    fshr_name = expr.op == ">>>=" ? "llvm.fshr.i64" : "llvm.fshl.i64"
                }
                var args <- [ r2v_left, r2v_left, right]
                var argTypes <- [ base_type_to_llvm_type(shiftType.baseType)]
                var id = LLVMLookupIntrinsicID(fshr_name)
                var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
                if (decl == null) {
                    failed_E(expr, "failed to get intrinsic {fshr_name}")
                } else {
                    var typ = LLVMFunctionType(argTypes[0], [ argTypes[0], argTypes[0], argTypes[0]])
                    var cll = LLVMBuildCall2(g_builder, typ, decl, args, string(expr.op))
                    setE(expr, LLVMBuildStoreAligned(g_builder, cll, left, uint(expr.left._type.alignOf)))
                }
            } else {
                failed_E(expr, "unsupported >>> type {opType}")
            }
        } elif (expr.op == "&") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildAnd(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported & type {opType}")
            }
        } elif (expr.op == "&=") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildAnd(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported &= type {opType}")
            }
        } elif (expr.op == "|") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildOr(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported | type {opType}")
            }
        } elif (expr.op == "|=") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildOr(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported |= type {opType}")
            }
        } elif (expr.op == "^") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildXor(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported ^ type {opType}")
            }
        } elif (expr.op == "^=") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildXor(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported ^= type {opType}")
            }
        } elif (expr.op == "^^") {
            if (opType.isBool) {
                setE(expr, LLVMBuildXor(g_builder, left, right, ""))
            } else {
                failed_E(expr, "unsupported ^^ type {opType}")
            }
        } elif (expr.op == "^^=") {
            if (opType.isBool) {
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildXor(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            } else {
                failed_E(expr, "unsupported ^^= type {opType}")
            }
        } else {
            failed_E(expr, "unsupported binary operator {expr.op}")
        }
    }

    def override preVisitExprOp2Right(expr : smart_ptr<ExprOp2>; right : ExpressionPtr) : void {
        if (isExprOp2_Func(expr)) {
            return
        }
        if (expr.op == "&&" || expr.op == "&&=" || expr.op == "||" || expr.op == "||=") {
            var monad_true = append_basic_block("monad_true_at_{int(expr.at.line)}_")
            var monad_false = append_basic_block("monad_true_at_{int(expr.at.line)}_")
            var monad_result, monad_cond : LLVMOpaqueValue?
            if (expr.op == "&&" || expr.op == "||") {
                at_function_entry <| $() {
                    monad_result = LLVMBuildAlloca(g_builder, types.t_int1, "{expr.op}_at_{int(expr.at.line)}_")
                }
                monad_cond = getE(expr.left)
                if (isEnumeration(expr.left._type)) {
                    monad_cond = LLVMBuildTruncOrBitCast(g_builder, getE(expr.left), types.t_int1, "if enum to_bool")
                } else {
                    monad_cond = getE(expr.left);
                }
            } else {
                monad_result = getE(expr.left)
                monad_cond = LLVMBuildLoad2(g_builder, types.t_int1, monad_result, "")
            }
            monad2block[get_expr_ptr(expr)] = monad_false
            setE(expr, monad_result)
            if (expr.op == "&&" || expr.op == "||") {
                LLVMBuildStore(g_builder, monad_cond, monad_result)
            }
            if (expr.op == "&&" || expr.op == "&&=") {
                LLVMBuildCondBr(g_builder, monad_cond, monad_true, monad_false)
            } else {
                LLVMBuildCondBr(g_builder, monad_cond, monad_false, monad_true)
            }
            LLVMPositionBuilderAtEnd(g_builder, monad_true)
        }
    }

    def override visitExprOp2(var expr : smart_ptr<ExprOp2>) : ExpressionPtr {
        if (isExprOp2_Func(expr)) {
            visitExprOp2_Func(expr)
        } elif (expr.op == "&&" || expr.op == "&&=" || expr.op == "||" || expr.op == "||=") {
            var monad_false = monad2block[get_expr_ptr(expr)]
            LLVMBuildStore(g_builder, getE(expr.right), getE(expr))
            LLVMBuildBr(g_builder, monad_false)
            LLVMPositionBuilderAtEnd(g_builder, monad_false)
            if (expr.op == "&&" || expr.op == "||") {
                setE(expr, LLVMBuildLoad2(g_builder, types.t_int1, getE(expr), ""))
            }
        } else {
            visitExprOp2_NonMonade(expr)
        }
        return expr
    }

// ExprOp3
    def override preVisitExprOp3(expr : smart_ptr<ExprOp3>) : void {
        var res : LLVMOpaqueValue?
        var before_if = append_basic_block("op3_cond_at_{int(expr.at.line)}_")
        LLVMBuildBr(g_builder, before_if)
        LLVMPositionBuilderAtEnd(g_builder, before_if)
        var blk = IteBlock(
            if_true  = append_basic_block("op3_true_at_line_{int(expr.at.line)}_"),
            if_false = append_basic_block("op3_false_at_line_{int(expr.at.line)}_"),
            if_end   = append_basic_block("op3_end_at_line_{int(expr.at.line)}_"),
            phi_true = append_basic_block("phi_true_at_line_{int(expr.at.line)}_"),
            phi_false = append_basic_block("phi_false_at_line_{int(expr.at.line)}_")
        )
        ite2blocks[get_expr_ptr(expr)] = blk
    }

    def override preVisitExprOp3Left(expr : smart_ptr<ExprOp3>; left : ExpressionPtr) : void {
        var blk = ite2blocks[get_expr_ptr(expr)]                        // we start if_true block
        LLVMBuildCondBr(g_builder, getE(expr.subexpr), blk.if_true, blk.if_false)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_true)
    }

    def override preVisitExprOp3Right(expr : smart_ptr<ExprOp3>; right : ExpressionPtr) : void {
        if (expr.left is ExprConstPtr) {
            let null_ptr = LLVMConstPointerNull(type_to_llvm_abi_type(expr._type))
            setE(expr.left, null_ptr)
        } elif (expr.left._type.isPointer) {
            // we need to cast pointer to the type of the right expression
            let cleft = LLVMBuildPointerCast(g_builder, getE(expr.left), type_to_llvm_type(expr._type), "")
            setE(expr.left, cleft)
        }
        var blk = ite2blocks[get_expr_ptr(expr)]
        LLVMBuildBr(g_builder, blk.phi_true)
        LLVMPositionBuilderAtEnd(g_builder, blk.phi_true)
        LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)   // after the end of if_true block we jump to if_end
        LLVMPositionBuilderAtEnd(g_builder, blk.if_false)
    }

    def override visitExprOp3(expr : smart_ptr<ExprOp3>) : ExpressionPtr {
        if (expr.right is ExprConstPtr) {
            let null_ptr = LLVMConstPointerNull(type_to_llvm_abi_type(expr._type))
            setE(expr.right, null_ptr)
        } elif (expr.right._type.isPointer) {
            // we need to cast pointer to the type of the left expression
            let cright = LLVMBuildPointerCast(g_builder, getE(expr.right), type_to_llvm_type(expr._type), "")
            setE(expr.right, cright)
        }
        var blk = ite2blocks[get_expr_ptr(expr)]
        LLVMBuildBr(g_builder, blk.phi_false)
        LLVMPositionBuilderAtEnd(g_builder, blk.phi_false)
        LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_end)
        var phi = LLVMBuildPhi(g_builder, type_to_llvm_abi_type(expr._type), "cond")
        LLVMAddIncoming(phi, fixed_array(getE(expr.left), getE(expr.right)), fixed_array(blk.phi_true, blk.phi_false))
        setE(expr, phi)
        return expr
    }

// ExprCopy
    def build_copy(tdst : LLVMOpaqueValue?; right : ExpressionPtr) {
        var tsrc = getE(right)
        var ralign = uint(right._type.alignOf)
        if (right._type.isRef) {
            if (right._type.isWorkhorseType) {
                var temp = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(right._type), tsrc, ralign |> int, "")
                if (right._type.isPointer) {
                    var compDst = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMTypeOf(temp), 0u), "")
                    LLVMBuildStoreAligned(g_builder, temp, compDst, ralign)
                } else {
                    LLVMBuildStoreAligned(g_builder, temp, tdst, ralign)
                }
            } else {
                let rsize = types.ConstI32(uint64(right._type.sizeOf))
                LLVMBuildMemCpy(g_builder, tdst, ralign, tsrc, ralign, rsize)
            }
        } else {
            if (right is ExprConstPtr) {
                let null_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
                LLVMBuildStore2Aligned(g_builder, types, types.LLVMVoidPtrType(), null_ptr, tdst, ralign)
            } elif (right._type.isPointer) {
                var compDst = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMTypeOf(tsrc), 0u), "")
                LLVMBuildStoreAligned(g_builder, tsrc, compDst, ralign)
            } else {
                LLVMBuildStoreAligned(g_builder, tsrc, tdst, ralign)
            }
        }
    }

    def override isRightFirstExprCopy(expr : smart_ptr<ExprCopy>) : bool {
        if (isCall2CMRES(expr.right) || isInvoke2CMRES(expr.right)) {
            return false
        }
        if (expr.copy_flags.takeOverRightStack) {
            return false
        }
        return true
    }

    def override preVisitExprCopyRight(expr : smart_ptr<ExprCopy>; right : ExpressionPtr) : void {
        if (isRightFirstExprCopy(expr)) {
            return // we do not need to do anything here
        }
        var tdst = getE(expr.left)
        if (make_call_to_cmres(expr.right, tdst)) {
            pass
        } elif (expr.copy_flags.takeOverRightStack) {
            assert(isMakeLocal(expr.right))
            var mk_ptr = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(types.t_int8, 0u), "")
            setE(expr.right, mk_ptr)
        }
    }

    def override visitExprCopy(expr : smart_ptr<ExprCopy>) : ExpressionPtr {
        if (isCall2CMRES(expr.right)) {
            pass    // already a call to cmres, no need to copy
        } else {
            var tdst = getE(expr.left)
            build_copy(tdst, expr.right)
        }
        return expr
    }

// ExprMove

    def build_move(tdst : LLVMOpaqueValue?; right : ExpressionPtr; needTest : bool) {
        var tsrc = getE(right)
        var ralign = uint(right._type.alignOf)
        if (right._type.isRef) {
            /*
            if right._type.isWorkhorseType
                var temp = LLVMBuildLoadDataAligned(g_builder, tsrc, ralign, "")
                LLVMBuildStoreAligned(g_builder, temp, tdst, ralign)
            */
            if (needTest) {
                var lptr = LLVMBuildPtrToInt(g_builder, tsrc, types.LLVMIntPtrType(), "")
                var rptr = LLVMBuildPtrToInt(g_builder, tdst, types.LLVMIntPtrType(), "")
                var cmpR = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, lptr, rptr, "")
                var move_start = append_basic_block("move_start")
                var move_end = append_basic_block("move_end")
                LLVMBuildCondBr(g_builder, cmpR, move_end, move_start)
                LLVMPositionBuilderAtEnd(g_builder, move_start)
                let rsize = types.ConstI32(uint64(right._type.sizeOf))
                LLVMBuildMemCpy(g_builder, tdst, ralign, tsrc, ralign, rsize)
                LLVMBuildMemSet(g_builder, tsrc, 0ul, right._type.sizeOf |> uint64(), ralign)
                LLVMBuildBr(g_builder, move_end)
                LLVMPositionBuilderAtEnd(g_builder, move_end)
            } else {
                let rsize = types.ConstI32(uint64(right._type.sizeOf))
                LLVMBuildMemCpy(g_builder, tdst, ralign, tsrc, ralign, rsize)
                LLVMBuildMemSet(g_builder, tsrc, 0ul, right._type.sizeOf |> uint64(), ralign)
            }
        } else {
            if (right is ExprConstPtr) {
                let null_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
                LLVMBuildStore2Aligned(g_builder, types, types.LLVMVoidPtrType(), null_ptr, tdst, ralign)
            } elif (right._type.isPointer) {
                var compDst = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMTypeOf(tsrc), 0u), "")
                LLVMBuildStoreAligned(g_builder, tsrc, compDst, ralign)
            } else {
                LLVMBuildStoreAligned(g_builder, tsrc, tdst, ralign)
            }
        }
    }

    def override isRightFirstExprMove(expr : smart_ptr<ExprMove>) : bool {
        if (isCall2CMRES(expr.right) || isInvoke2CMRES(expr.right)) {
            return false
        }
        if (expr.move_flags.takeOverRightStack) {
            return false
        }
        return true
    }

    def override preVisitExprMoveRight(expr : smart_ptr<ExprMove>; right : ExpressionPtr) : void {
        if (isRightFirstExprMove(expr)) {
            return // we do not need to do anything here
        }
        var tdst = getE(expr.left)
        if (make_call_to_cmres(expr.right, tdst)) {
            pass
        } elif (expr.move_flags.takeOverRightStack) {
            assert(isMakeLocal(expr.right))
            var mk_ptr = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(types.t_int8, 0u), "")
            setE(expr.right, mk_ptr)
        }
    }

    def override visitExprMove(expr : smart_ptr<ExprMove>) : ExpressionPtr {
        if (isCall2CMRES(expr.right)) {
            pass    // already a call to cmres, no need to copy
        } else {
            var tdst = getE(expr.left)
            build_move(tdst, expr.right, true)
        }
        return expr
    }

// ExprWhile
    def override preVisitExprWhile(expr : smart_ptr<ExprWhile>) : void {
        var lblk = LoopBlock(
            loop_start = append_basic_block("while_start"),
            loop_body = append_basic_block("while_body"),
            loop_end = append_basic_block("while_end")
        )
        loop_stack |> push(lblk)
        LLVMBuildBr(g_builder, lblk.loop_start)
        LLVMPositionBuilderAtEnd(g_builder, lblk.loop_start)
    }

    def override preVisitExprWhileBody(expr : smart_ptr<ExprWhile>; right : ExpressionPtr) : void {
        var lblk = loop_stack |> back()
        LLVMBuildCondBr(g_builder, getE(expr.cond), lblk.loop_body, lblk.loop_end)
        LLVMPositionBuilderAtEnd(g_builder, lblk.loop_body)
    }

    def override visitExprWhile(expr : smart_ptr<ExprWhile>) : ExpressionPtr {
        var lblk = loop_stack |> back()
        loop_stack |> pop()
        if (!current_block_terminates()) {
            LLVMBuildBr(g_builder, lblk.loop_start)
        }
        LLVMPositionBuilderAtEnd(g_builder, lblk.loop_end)
        if (expr.body is ExprBlock) {
            let body = expr.body as ExprBlock
            if (length(body.finalList) != 0) {
                LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(types.t_int8, 0u)), callBlock)
                LLVMBuildBr(g_builder, finallyBlocks[body])
                visit_finally(body, adapter)
                jump_to_next_finally(body)
            }
        }
        return expr
    }

// ExprTryCatch
    def override preVisitExprTryCatch(expr : smart_ptr<ExprTryCatch>) : void {
        unsupported(expr, "try-catch")
    }

// ExprIfThenElse
    /*
    if_cond_at:
        cond
        br cond, if_true, if_false
    if_true:
        true_expr
        br if_end
    if_false:
        false_expr
        br if_end
    if_end:
    */
    def override preVisitExprIfThenElse(expr : smart_ptr<ExprIfThenElse>) : void {
        var before_if = append_basic_block("if_cond_at_{int(expr.at.line)}_")
        LLVMBuildBr(g_builder, before_if)
        LLVMPositionBuilderAtEnd(g_builder, before_if)
        var blk = IteBlock(
            if_true  = append_basic_block("if_true_at_line_{int(expr.at.line)}_"),
            if_false = append_basic_block("if_false_at_line_{int(expr.at.line)}_"),
            if_end   = append_basic_block("if_end_at_line_{int(expr.at.line)}_")
        )
        ite2blocks[get_expr_ptr(expr)] = blk
    }

    def override preVisitExprIfThenElseIfBlock(expr : smart_ptr<ExprIfThenElse>; ifBlock : ExpressionPtr) : void {
        var blk = ite2blocks[get_expr_ptr(expr)]
        LLVMBuildCondBr(g_builder, getE(expr.cond), blk.if_true, blk.if_false)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_true)
    }

    def before_else(expr : smart_ptr<ExprIfThenElse>) : void {
        var blk & = unsafe(ite2blocks[get_expr_ptr(expr)])
        if (!current_block_terminates()) {
            LLVMBuildBr(g_builder, blk.if_end)
        } else {
            blk.if_true_terminates = true
        }
        LLVMPositionBuilderAtEnd(g_builder, blk.if_false)
    }

    def override preVisitExprIfThenElseElseBlock(expr : smart_ptr<ExprIfThenElse>; elseBlock : ExpressionPtr) : void {
        before_else(expr)
    }

    def override visitExprIfThenElse(expr : smart_ptr<ExprIfThenElse>) : ExpressionPtr {
        var blk & = unsafe(ite2blocks[get_expr_ptr(expr)])
        if (expr.if_false == null) {
            before_else(expr)
        }
        if (!current_block_terminates()) {
            LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)
        } else {
            blk.if_false_terminates = true
        }
        LLVMPositionBuilderAtEnd(g_builder, blk.if_end)
        if (blk.if_true_terminates && blk.if_false_terminates) {
            LLVMBuildUnreachable(g_builder)
        }
        return expr
    }

// ExprFor
    def build_array_lock(array_ptr : LLVMOpaqueValue?; at : LineInfo) {
        var params = fixed_array(
            LLVMBuildPointerCast(g_builder, array_ptr, types.LLVMVoidPtrType(), ""),             // array
            get_context_param(),
            get_line_info_ptr(at),
        )
        var typ = g_fn_types[FN_JIT_ARRAY_LOCK]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ARRAY_LOCK), params, "")
    }

    def build_array_unlock(array_ptr : LLVMOpaqueValue?; at : LineInfo) {
        var params = fixed_array(
            LLVMBuildPointerCast(g_builder, array_ptr, types.LLVMVoidPtrType(), ""), // array
            get_context_param(),                                                     // context
            get_line_info_ptr(at)                                                    // at
        )
        var typ = g_fn_types[FN_JIT_ARRAY_UNLOCK]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ARRAY_UNLOCK), params, "")
    }

    def build_iter_first(at : LineInfo; iter_ptr, data_ptr : LLVMOpaqueValue?) {
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, types.LLVMVoidPtrType(), "")
        var params = fixed_array(
            iter_ptr,               // sequence *
            vdata_ptr,              // data *
            get_context_param(),    // context
            get_line_info_ptr(at)   // at
        )
        var typ = g_fn_types[FN_JIT_ITERATOR_FIRST]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_FIRST), params, "")
    }

    def build_iter_next(at : LineInfo; iter_ptr, data_ptr : LLVMOpaqueValue?) {
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, types.LLVMVoidPtrType(), "")
        var params = fixed_array(
            iter_ptr,               // sequence *
            vdata_ptr,              // data *
            get_context_param(),    // context
            get_line_info_ptr(at)   // at
        )
        var typ = g_fn_types[FN_JIT_ITERATOR_NEXT]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_NEXT), params, "")
    }

    def build_iter_iterate(iter_ptr, data_ptr : LLVMOpaqueValue?) {
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, types.LLVMVoidPtrType(), "")
        var params = fixed_array(
            iter_ptr,           // sequence *
            vdata_ptr,          // data *
            get_context_param() // context
        )
        var typ = g_fn_types[FN_JIT_ITERATOR_ITERATE]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_ITERATE), params, "")
    }

    def build_iter_delete(iter_ptr : LLVMOpaqueValue?) {
        var params = fixed_array(
            iter_ptr,           // sequence *
            get_context_param() // context
        )
        var typ = g_fn_types[FN_JIT_ITERATOR_DELETE]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_DELETE), params, "")
    }

    def build_iter_close(iter_ptr, data_ptr : LLVMOpaqueValue?) {
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, types.LLVMVoidPtrType(), "")
        var params = fixed_array(
            iter_ptr,           // sequence *
            vdata_ptr,          // data *
            get_context_param() // context
        )
        var typ = g_fn_types[FN_JIT_ITERATOR_CLOSE]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_CLOSE), params, "")
    }

    def is_count_or_ucount(expr : ExpressionPtr) {
        if (!(expr is ExprCall)) {
            return false
        }
        var cf = expr as ExprCall
        return cf.func.flags.builtIn && cf.func._module.name == "$" && (cf.func.name == "count" || cf.func.name == "ucount")
    }

    def override preVisitExprFor(expr : smart_ptr<ExprFor>) : void {
        var lblk = LoopBlock(
            loop_start = append_basic_block("for_start"),
            loop_continue = append_basic_block("for_continue"),
            loop_end = append_basic_block("for_end")
        )
        at_function_entry <| $() {
            lblk.need_loop = LLVMBuildAlloca(g_builder, types.t_int1, "need_loop")
        }
        loop_stack |> push(lblk)
        for (ssrc in expr.sources) {
            if (is_count_or_ucount(ssrc)) {
                skipCall |> insert(ssrc as ExprCall)
            }
        }
        LLVMBuildStore(g_builder, LLVMConstInt(types.t_int1, 1ul, 0), lblk.need_loop) // need loop, yes
    }

    def override preVisitExprForVariable(expr : smart_ptr<ExprFor>; svar : VariablePtr; last : bool) : void {
        // add variable to be looked up
        var vtype = type_to_llvm_type(svar._type)
        var vptrtype = get_type_pointer(svar._type)
        if (svar._type.flags.ref) {
            vtype = vptrtype
            vptrtype = LLVMPointerType(vptrtype, 0u)
        }
        var v_ptr : LLVMOpaqueValue?
        at_function_entry <| $() {
            v_ptr = LLVMBuildAlloca(g_builder, vtype, "for_loop_variable_{svar.name}")
            LLVMSetAlignment(v_ptr, 16u)
            v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, vptrtype, "for_loop_variable_{svar.name}")
            setV(svar, v_ptr)
        }
        // initialize its memory with 0 (should we?)
        var vsize = svar._type.isRef ?  typeinfo sizeof(type<void?>) : svar._type.sizeOf
        LLVMBuildMemSet(g_builder, v_ptr, 0ul, uint64(vsize), 16u)
    }

    def override preVisitExprForBody(expr : smart_ptr<ExprFor>) : void {
        /*
            for ( int t=0; t!=totalCount; ++t ) {
                sources[t]->isOpen = true  // what is this?
                needLoop = sources[t]->first(context, pi[t]) && needLoop
                if ( context.stopFlags ) goto loopend
            }
        LOOP_START:
        */
        var lblk = loop_stack |> back()
        for (svar, ssrc in expr.iteratorVariables, expr.sources) {
            if (ssrc._type.isRange) {// range()->first()
                var srange = getE(ssrc)
                var sfrom = LLVMBuildExtractElement(g_builder, srange, types.ConstI32(0ul), "range_from_{svar.name}")
                var sto = LLVMBuildExtractElement(g_builder, srange, types.ConstI32(1ul), "range_to_{svar.name}")
                var cmp_from_to = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntSLT, sfrom, sto, "")
                var okay = append_basic_block("for_{svar.name}_not_empty")
                var not_okay = append_basic_block("for_{svar.name}_empty")
                LLVMBuildCondBr(g_builder, cmp_from_to, okay, not_okay)
                LLVMPositionBuilderAtEnd(g_builder, not_okay)
                LLVMBuildStore(g_builder, LLVMConstInt(types.t_int1, 0ul, 0), lblk.need_loop) // need loop, no
                LLVMBuildBr(g_builder, okay)
                LLVMPositionBuilderAtEnd(g_builder, okay)
                range2[get_ptr(ssrc)] = sto
                LLVMBuildStore(g_builder, sfrom, getV(svar))
            } elif (ssrc._type.dim |> length != 0) {// []->first()
                var inscope etype <- clone_type(ssrc._type)
                etype.dim |> pop()
                var element_type = type_to_llvm_type(svar._type)
                var sdim = getE(ssrc)
                var s0 = LLVMBuildGEP2(g_builder, element_type, sdim, types.ConstI32(0ul), "dim_0")
                LLVMBuildStore(g_builder, s0, getV(svar))
                let dimSize = ssrc._type.dim[ssrc._type.dim |> length - 1]
                var tail = types.ConstI32(uint64(dimSize - 1))
                var sto = build_array_index(sdim, tail, etype, "last_element")
                sto = LLVMBuildPtrToInt(g_builder, sto, types.LLVMIntPtrType(), "dim_end")
                range2[get_ptr(ssrc)] = sto
            } elif (ssrc._type.isGoodArrayType) {// {}->first()
                var arr = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(ssrc._type), getE(ssrc), ssrc._type.alignOf, "array")
                var size = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY.SIZE), "array.size")
                var cmp_from_to = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, size, types.ConstI32(0ul), "")
                var okay = append_basic_block("for_{svar.name}_not_empty")
                var not_okay = append_basic_block("for_{svar.name}_empty")
                LLVMBuildCondBr(g_builder, cmp_from_to, okay, not_okay)
                LLVMPositionBuilderAtEnd(g_builder, not_okay)
                LLVMBuildStore(g_builder, LLVMConstInt(types.t_int1, 0ul, 0), lblk.need_loop) // need loop, no
                LLVMBuildBr(g_builder, okay)
                LLVMPositionBuilderAtEnd(g_builder, okay)
                build_array_lock(getE(ssrc), expr.at)
                var data = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY.DATA), "array.data")
                LLVMBuildStore(g_builder, data, getV(svar))
                var tail = LLVMBuildSub(g_builder, size, types.ConstI32(1ul), "")
                var sto = build_array_index(data, tail, ssrc._type.firstType, "last_element")
                sto = LLVMBuildPtrToInt(g_builder, sto, types.LLVMIntPtrType(), "array_end")
                range2[get_ptr(ssrc)] = sto
            } elif (ssrc._type.isIterator) {
                if (is_count_or_ucount(ssrc)) {
                    var ccount = ssrc as ExprCall
                    visit(ccount.arguments[0], adapter)
                    visit(ccount.arguments[1], adapter)
                    LLVMBuildStore(g_builder, getE(ccount.arguments[0]), getV(svar))
                } else {
                    var seq = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(ssrc._type), getE(ssrc), ssrc._type.alignOf, "")
                    var piter = LLVMBuildExtractValue(g_builder, seq, uint(JIT_SEQUENCE.ITERATOR), "")
                    var cmp_iter = LLVMBuildIsNull(g_builder, piter, "")
                    var okay = append_basic_block("for_{svar.name}_not_empty")
                    var not_okay = append_basic_block("for_{svar.name}_empty")
                    LLVMBuildCondBr(g_builder, cmp_iter, not_okay, okay)
                    LLVMPositionBuilderAtEnd(g_builder, not_okay)
                    LLVMBuildStore(g_builder, LLVMConstInt(types.t_int1, 0ul, 0), lblk.need_loop) // need loop, no
                    LLVMBuildBr(g_builder, okay)
                    LLVMPositionBuilderAtEnd(g_builder, okay)
                    // fun ->first
                    okay = append_basic_block("for_{svar.name}_first_okay")
                    not_okay = append_basic_block("for_{svar.name}_first_failed")
                    var rcond = build_iter_first(ssrc.at, getE(ssrc), getV(svar))
                    LLVMBuildCondBr(g_builder, rcond, okay, not_okay)
                    LLVMPositionBuilderAtEnd(g_builder, not_okay)
                    LLVMBuildStore(g_builder, LLVMConstInt(types.t_int1, 0ul, 0), lblk.need_loop) // need loop, no
                    LLVMBuildBr(g_builder, okay)
                    LLVMPositionBuilderAtEnd(g_builder, okay)
                }
            } elif (ssrc._type.isString) {
                at_function_entry <| $() {
                    // Let's reuse range for string iterator.
                    range2[get_ptr(ssrc)] = LLVMBuildAlloca(g_builder, LLVMPointerType(types.t_int8, 0u), "loop_iter")
                }
                var s_ptr = getE(ssrc)
                // Check if it's das-optimized empty string (just nullptr).
                var is_empty_str = LLVMBuildIsNull(g_builder, s_ptr, "null_check")
                var not_empty_str = append_basic_block("not_empty_str")
                LLVMBuildCondBr(g_builder, is_empty_str, lblk.loop_end, not_empty_str)

                LLVMPositionBuilderAtEnd(g_builder, not_empty_str)

                LLVMBuildStore(g_builder, s_ptr, range2[get_ptr(ssrc)])
                var cur_chr = LLVMBuildLoad2(g_builder, types.t_int8, s_ptr, "cur_chr")
                let zero_i8 = types.ConstI8(0 |> int8)
                let is_null = LLVMBuildICmp(g_builder,
                    LLVMIntPredicate.LLVMIntNE, cur_chr, zero_i8, "is_null")
                // Just in case check whether string is equal to '\0', e.g. this
                // is native string.
                LLVMBuildStore(g_builder, is_null, lblk.need_loop)

                assume svar_t = svar._type
                assert(svar_t.baseType == Type.tInt)
                // Cast to i32, since char is i32 in das.
                var res = LLVMBuildSExt(g_builder, cur_chr, type_to_llvm_type(svar_t), "sext_cast")
                LLVMBuildStore(g_builder, res, getV(svar))
            } else {
                failed_E(ssrc, "unsupported loop source {describe(ssrc._type)}")
            }
        }
        var needLoop = LLVMBuildLoad2(g_builder, types.t_int1, lblk.need_loop, "")
        LLVMBuildCondBr(g_builder, needLoop, lblk.loop_start, lblk.loop_end)
        LLVMPositionBuilderAtEnd(g_builder, lblk.loop_start)
    }

    def override visitExprFor(expr : smart_ptr<ExprFor>) : ExpressionPtr {
        var lblk = loop_stack |> back()
        loop_stack |> pop()
        /*
        LOOP_CONTINUE:
            for ( int t=0; t!=totalCount; ++t ){
                if ( !sources[t]->next(context, pi[t]) ) goto loopend
                if ( context.stopFlags ) goto loopend
            }
        LOOP_END:
        */
        if (!current_block_terminates()) {
            LLVMBuildBr(g_builder, lblk.loop_continue)                           // LOOP CONTINUE
        }
        LLVMPositionBuilderAtEnd(g_builder, lblk.loop_continue)
        for (svar, ssrc in expr.iteratorVariables, expr.sources) {
            if (ssrc._type.isRange) {// range()->next()
                var isRange64 = ssrc._type.baseType == Type.tRange64 || ssrc._type.baseType == Type.tURange64
                var svar_v = LLVMBuildLoad2(g_builder, isRange64 ? types.t_int64 : types.t_int32, getV(svar), "")
                svar_v = LLVMBuildAdd(g_builder, svar_v, LLVMConstInt(isRange64 ? types.t_int64 : types.t_int32, 1ul, 0), "")
                LLVMBuildStore(g_builder, svar_v, getV(svar))
                var sto = range2[get_ptr(ssrc)]
                var rcond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, svar_v, sto, "")
                var nextOk = append_basic_block("for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, rcond, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder, nextOk)
            } elif (ssrc._type.dim |> length != 0) {// []->next()
                var inscope etype <- clone_type(ssrc._type)
                etype.dim |> pop()
                var svar_v = LLVMBuildLoad2(g_builder, LLVMPointerType(type_to_llvm_type(svar._type), 0u), getV(svar), "")
                var svar_i = build_array_index(svar_v, types.ConstI32(1ul), etype, "next_element")
                LLVMBuildStore(g_builder, svar_i, getV(svar))
                var sto = range2[get_ptr(ssrc)]
                svar_i = LLVMBuildPtrToInt(g_builder, svar_v, types.LLVMIntPtrType(), "")
                var rcond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, svar_i, sto, "")
                var nextOk = append_basic_block("for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, rcond, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder, nextOk)
            } elif (ssrc._type.isGoodArrayType) {// {}->next()
                var svar_v = LLVMBuildLoad2(g_builder, LLVMPointerType(type_to_llvm_type(svar._type), 0u), getV(svar), "")
                var svar_i = build_array_index(svar_v, types.ConstI32(1ul), ssrc._type.firstType, "next_element")
                LLVMBuildStore(g_builder, svar_i, getV(svar))
                var sto = range2[get_ptr(ssrc)]
                svar_i = LLVMBuildPtrToInt(g_builder, svar_v, types.LLVMIntPtrType(), "")
                var rcond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, svar_i, sto, "")
                var nextOk = append_basic_block("for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, rcond, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder, nextOk)
            } elif (ssrc._type.isIterator) {// iterator->next()
                if (is_count_or_ucount(ssrc)) {
                    var ccount = ssrc as ExprCall
                    var vvar = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(svar._type), getV(svar), svar._type.alignOf, "")
                    var vadd = LLVMBuildAdd(g_builder, vvar, getE(ccount.arguments[1]), "")
                    LLVMBuildStore(g_builder, vadd, getV(svar))
                } else {
                    var rcond = build_iter_next(ssrc.at, getE(ssrc), getV(svar))
                    var nextOk = append_basic_block("for_{svar.name}_next_ok")
                    LLVMBuildCondBr(g_builder, rcond, nextOk, lblk.loop_end)
                    LLVMPositionBuilderAtEnd(g_builder, nextOk)
                }
            } elif (ssrc._type.isString) {
                var svar_val = LLVMBuildLoad2(g_builder, LLVMPointerType(types.t_int8, 0u), range2[get_ptr(ssrc)], "")
                var svar_i = LLVMBuildGEP2(g_builder, types.t_int8, svar_val, types.ConstI32(1ul), "next_element")
                var val = LLVMBuildLoad2(g_builder, types.t_int8, svar_i, "")
                let zero_i8 = types.ConstI8(0 |> int8)
                var is_empty_str = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, val, zero_i8, "is_zero")
                var nextOk = append_basic_block("for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, is_empty_str, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder, nextOk)

                LLVMBuildStore(g_builder, svar_i, range2[get_ptr(ssrc)])
                assert(svar._type.baseType == Type.tInt)
                var res = LLVMBuildSExt(g_builder, val, type_to_llvm_type(svar._type), "sext_cast")
                LLVMBuildStore(g_builder, res, getV(svar))
            } else {
                failed_E(ssrc, "unsupported loop source {describe(ssrc._type)}")
            }
        }
        LLVMBuildBr(g_builder, lblk.loop_start)
        /*
        LOOP_END:
            evalFinal(context)
            for ( int t=0; t!=totalCount; ++t ) {
                sources[t]->close(context, pi[t])
            }
        */
        LLVMPositionBuilderAtEnd(g_builder, lblk.loop_end)                  // LOOP END
        // in for loop closing iterators is included in the finally section
        let body = expr.body as ExprBlock
        var fblk = finallyBlocks[body]
        LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(types.t_int8, 0u)), callBlock)
        LLVMBuildBr(g_builder, fblk)
        LLVMPositionBuilderAtEnd(g_builder, fblk)
        // close all iterators
        let len = length(expr.iteratorVariables)
        for (i in range(len)) {
            let ri = len - i - 1
            let svar & = unsafe(expr.iteratorVariables[ri])
            let ssrc & = unsafe(expr.sources[ri])
            if (ssrc._type.isGoodArrayType) {
                build_array_unlock(getE(ssrc), ssrc.at)
            } elif (ssrc._type.isIterator) {
                if (is_count_or_ucount(ssrc)) {
                    // we do nothing for count
                    pass
                } else {
                    build_iter_close(getE(ssrc), getV(svar))
                }
            }
        }
        // now, final block if there is anything there
        if (length(body.finalList) != 0) {
            visit_finally(body, adapter)
        }
        jump_to_next_finally(body)
        return expr
    }

// ExprMakeVariant
    def override preVisitExprMakeVariant(expr : smart_ptr<ExprMakeVariant>) : void {
        var mkv_ptr : LLVMOpaqueValue?
        if (expr.makeFlags.useCMRES) {
            mkv_ptr = get_cmres_param()
            mkv_ptr = LLVMBuildPointerCast(g_builder, mkv_ptr, LLVMPointerType(types.t_int8, 0u), "")
            if (expr.extraOffset != 0u) {
                mkv_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mkv_ptr, types.ConstI32(uint64(expr.extraOffset)), "")
            }
        } else {
            mkv_ptr = tryE(expr) <| $() {
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <| $() {
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mkv_local_sp_{expr.stackTop}")
                    LLVMSetAlignment(s_ptr, 16u)
                    s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(types.t_int8, 0u), "mkv_local_sp_{expr.stackTop}")
                }
                return s_ptr
            }
            if (expr.extraOffset != 0u) {
                mkv_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mkv_ptr, types.ConstI32(uint64(expr.extraOffset)), "mkv_local_sp_{expr.stackTop}_eo_{expr.extraOffset}")
            }
        }
        let stride = expr.makeType.stride
        if (stride != 0 && length(expr.variants) == 0) {
            let total = int(expr.variants |> length)
            let bytes = max(total, 1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mkv_ptr, 0ul, uint64(bytes), align)
        }
        setE(expr, mkv_ptr)
    }

    def override preVisitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) : void {
        var field_index = find_argument_index(expr.makeType, string(decl.name))
        if (field_index == -1) {
            failed_E(expr, "variant field {decl.name} not found")
            return
        }
        let field_offset = get_variant_field_offset(expr.makeType, field_index)
        let stride = expr.makeType.stride
        let offset =  index * stride
        var v_ptr = getE(expr)
        verify(v_ptr != null && LLVMIsUndef(v_ptr) == 0, "make variant field, but no pointer")
        let index_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "")
        LLVMBuildStore2(g_builder, types, types.t_int32, types.ConstI32(uint64(field_index)), index_ptr)
        let field_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset + field_offset)), "")
        if (decl.value |> isMakeLocal) {
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(decl.value))
            if (!expr.makeFlags.useCMRES) {
                val.extraOffset = uint(offset + field_offset) // + expr.extraOffset
                setE(decl.value, v_ptr)
            }
        } elif (make_call_to_cmres(decl.value, field_ptr)) {
            pass
        }
    }

    def override visitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) : MakeFieldDeclPtr {
        var field_index = find_argument_index(expr.makeType, string(decl.name))
        if (field_index == -1) {
            failed_E(expr, "variant field {decl.name} not found")
            return decl
        }
        let field_offset = get_variant_field_offset(expr.makeType, field_index)
        let stride = expr.makeType.stride
        let offset =  index * stride
        var v_ptr = getE(expr)
        v_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset + field_offset)), "[{index}].{decl.name}")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr.makeType.argTypes[field_index]), "")
        if (decl.value |> isMakeLocal) {
            pass // do nothing. we already took care of makeLocal in preVisitExprMakeVariantField
        } elif (isCall2CMRES(decl.value)) {
            pass // do nothing. we already handled this in preVisitExprMakeVariantField
        } else {
            if (decl.flags.moveSemantics) {
                build_move(v_ptr, decl.value, false)   // TODO: verify
            } else {
                build_copy(v_ptr, decl.value)
            }
        }
        return decl
    }

    def override visitExprMakeVariant(expr : smart_ptr<ExprMakeVariant>) : ExpressionPtr {
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr
    }

// ExprMakeStruct
    def callConstructor(expr : smart_ptr<ExprMakeStruct>; cmresEval : LLVMOpaqueValue?) {
        assume expr_func = expr.constructor
        if (expr_func.moreFlags.requestJit) {
            var params : array<LLVMOpaqueValue?>
            params |> push(get_context_param())
            params |> push(LLVMBuildPointerCast(g_builder, cmresEval, types.LLVMVoidPtrType(), ""))
            let fmna = uid.get_dll_fn_name_ptr(expr_func)
            var func = LLVMGetNamedFunction(g_mod, fmna.impl())
            var typ = g_fn_types[fmna.impl()]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
        } else {
            let name = uid.get_ctor(expr)
            var ctor = get_address_global(name)
            if (ctor == null) {
                var MNH_ADDR = get_function_addr(jit_context, expr_func)
                ctor = save_address_global(name, MNH_ADDR)
            }
            var params : array<LLVMOpaqueValue?>
            params |> push(ctor)  // mnh
            params |> push(LLVMConstPointerNull(LLVMPointerType(types.LLVMFloat4Type(), 0u))) // args
            params |> push(LLVMBuildPointerCast(g_builder, cmresEval, types.LLVMVoidPtrType(), ""))
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
        }
    }

    def override preVisitExprMakeStruct(expr : smart_ptr<ExprMakeStruct>) : void {
        var mks_ptr : LLVMOpaqueValue?
        if (expr.makeFlags.useCMRES) {
            mks_ptr = get_cmres_param()
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(types.t_int8, 0u), "")
            if (expr.extraOffset != 0u) {
                mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(expr.extraOffset)), "")
            }
        } else {
            mks_ptr = tryE(expr) <| $() {
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <| $() {
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mks_local_sp_{expr.stackTop}")
                    LLVMSetAlignment(s_ptr, 16u)
                }
                    // s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(types.t_int8,0u), "mks_local_sp_{expr.stackTop}")
                return s_ptr
            }
            if (expr.extraOffset != 0u) {
                mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(expr.extraOffset)), "mks_local_sp_{expr.stackTop}_eo_{expr.extraOffset}")
            }
        }
        let stride = expr.makeType.stride
        let emptyEmbeddedTuple = expr.makeType.baseType == Type.tTuple && length(expr.structs) == 0
        let partiallyInitStruct = !expr.makeFlags.doesNotNeedInit && !expr.makeFlags.initAllFields
        if ((partiallyInitStruct || emptyEmbeddedTuple) && stride != 0) {
            let total = int(expr.structs |> length)
            let bytes = max(total, 1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mks_ptr, 0ul, uint64(bytes), align)
        }
        setE(expr, mks_ptr)
    }

    def override preVisitExprMakeStructIndex(expr : smart_ptr<ExprMakeStruct>; index : int; last : bool) : void {
        if (expr.constructor != null) {
            var mks_ptr = getE(expr)
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(types.t_int8, 0u), "")
            let stride = expr.makeType.stride
            let offset = index * stride
            mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(offset)), "mks_index_{index}")
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(type_to_llvm_type(expr.makeType), 0u), "")
            callConstructor(expr, mks_ptr)
        }
    }

    def override visitMakeStructureBlock(expr : smart_ptr<ExprMakeStruct>; blk : ExpressionPtr) : ExpressionPtr {
        if (expr._block != null) {
            var params : array<LLVMOpaqueValue?>

            var block_ptr = getE(expr._block)
            block_ptr = LLVMBuildPointerCast(g_builder, block_ptr, LLVMPointerType(types.t_int8, 0u), "")
            params |> push(block_ptr)
        // put put the result of ExprMakeStruct as the sole argument to the block
            var mks_ptr = getE(expr)
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(types.t_int8, 0u), "")

            var args : LLVMOpaqueValue?
            at_function_entry <| $() {
                args = LLVMBuildAlloca(g_builder, LLVMArrayType(types.LLVMFloat4Type(), 1u), "args")
                LLVMSetAlignment(args, 16u)
                args = LLVMBuildPointerCast(g_builder, args, LLVMPointerType(types.LLVMFloat4Type(), 0u), "call_args_ptr")
            }
            var arg_ptr = LLVMBuildGEP2(g_builder, types.LLVMFloat4Type(), args, types.ConstI32(0 |> uint64()), "call_arg__ptr")
            var arg_val = mks_ptr
            LLVMBuildStore2(g_builder, types, types.LLVMVoidPtrType(), arg_val, arg_ptr)

            params |> push(args)
            params |> push(get_context_param())

            var func = LLVMGetNamedFunction(g_mod, FN_JIT_INVOKE_BLOCK)
            var typ = g_fn_types[FN_JIT_INVOKE_BLOCK]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
        }
        return blk
    }

    def override preVisitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) : void {
        var field_offset : int
        if (expr.makeType.isHandle) {
            var pann = expr.makeType.annotation
            var ann <- unsafe(reinterpret<smart_ptr<TypeAnnotation>> pann)
            field_offset = int(get_handled_type_field_offset(ann, string(decl.name)))
        } else {
            var field = find_structure_field(expr.makeType.structType, string(decl.name))
            if (field == null) {
                failed_E(expr, "field {decl.name} not found")
                return
            }
            field_offset = field.offset
        }
        let stride = expr.makeType.stride
        let offset =  index * stride + field_offset
        var v_ptr = getE(expr)
        verify(v_ptr != null && LLVMIsUndef(v_ptr) == 0, "make struct field, but no pointer")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(types.t_int8, 0u), "")
        let field_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "mks_{decl.name}")
        if (decl.value |> isMakeLocal) {
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(decl.value))
            if (!expr.makeFlags.useCMRES) {
                val.extraOffset = uint(offset) // + expr.extraOffset
                setE(decl.value, v_ptr)
            }
        } elif (make_call_to_cmres(decl.value, field_ptr)) {
            pass
        }
    }

    def override visitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) : MakeFieldDeclPtr {
        var field_offset : int
        var inscope field_type : TypeDeclPtr
        if (expr.makeType.isHandle) {
            var pann = expr.makeType.annotation
            var ann <- unsafe(reinterpret<smart_ptr<TypeAnnotation>> pann)
            field_offset = int(get_handled_type_field_offset(ann, string(decl.name)))
            field_type |> move_new <| get_handled_type_field_type_declaration(ann, string(decl.name), false)
        } else {
            var field = find_structure_field(expr.makeType.structType, string(decl.name))
            if (field == null) {
                failed_E(expr, "field {decl.name} not found")
                return decl
            }
            field_offset = field.offset
            field_type := field._type
        }
        let stride = expr.makeType.stride
        let offset =  index * stride + field_offset
        var v_ptr = getE(expr)
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(types.t_int8, 0u), "")
        v_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "_[{index}].{decl.name}")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(field_type), "")
        if (decl.value |> isMakeLocal) {
            pass // do nothing. we already took care of makeLocal in preVisitExprMakeStructField
        } elif (isCall2CMRES(decl.value)) {
            pass // do nothing. we already handled this in preVisitExprMakeStructField
        } else {
            if (decl.flags.moveSemantics) {
                build_move(v_ptr, decl.value, false)   // TODO: verify
            } else {
                build_copy(v_ptr, decl.value)
            }
        }
        return decl
    }

    def override visitExprMakeStruct(expr : smart_ptr<ExprMakeStruct>) : ExpressionPtr {
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr
    }

// ExprMakeArray
    def override preVisitExprMakeArray(expr : smart_ptr<ExprMakeArray>) : void {
        var mks_ptr : LLVMOpaqueValue?
        if (expr.makeFlags.useCMRES) {
            mks_ptr = get_cmres_param()
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(types.t_int8, 0u), "")
            if (expr.extraOffset != 0u) {
                mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(expr.extraOffset)), "ema_{expr.at.line}")
            }
        } else {
            mks_ptr = tryE(expr) <| $() {
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <| $() {
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mka_local_sp_{expr.stackTop}_at_{int(expr.at.line)}_")
                    LLVMSetAlignment(s_ptr, 16u)
                    s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(types.t_int8, 0u), "mka_local_sp_{expr.stackTop}_at_{int(expr.at.line)}_")
                }
                return s_ptr
            }
            if (expr.extraOffset != 0u) {
                mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(expr.extraOffset)), "mka_local_sp_{expr.stackTop}_eo_{expr.extraOffset}_at_{int(expr.at.line)}_")
            }
        }
        let stride = expr.makeType.stride
        if (!expr.makeFlags.doesNotNeedInit && !expr.makeFlags.initAllFields && stride != 0) {
            let total = int(expr.values |> length)
            let bytes = max(total, 1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mks_ptr, 0ul, uint64(bytes), align)
        }
        setE(expr, mks_ptr)
    }

    def override preVisitExprMakeArrayIndex(expr : smart_ptr<ExprMakeArray>; index : int; init : ExpressionPtr; last : bool) : void {
        let stride = expr.makeType.stride
        let offset =  index * stride
        var v_ptr = getE(expr)
        verify(v_ptr != null && LLVMIsUndef(v_ptr) == 0, "make array index, but no pointer")
        let record_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "ema_ind_{index}_{expr.at.line}")
        if (init |> isMakeLocal) {
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(init))
            if (!expr.makeFlags.useCMRES) {
                val.extraOffset = uint(offset) // + expr.extraOffset
                setE(init, v_ptr)
            }
        } elif (make_call_to_cmres(init, record_ptr)) {
            pass
        }
    }

    def override visitExprMakeArrayIndex(expr : smart_ptr<ExprMakeArray>; index : int; init : ExpressionPtr; last : bool) : ExpressionPtr {
        let stride = expr.makeType.stride
        let offset =  index * stride
        var v_ptr = getE(expr)
        v_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "[{index}]")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr.recordType), "")
        if (init |> isMakeLocal) {
            pass // do nothing. we already took care of makeLocal in preVisitExprMakeArrayField
        } elif (isCall2CMRES(init)) {
            pass // do nothing. we already handled this in preVisitExprMakeArrayField
        } else {
            if (expr.recordType.canCopy) {
                build_copy(v_ptr, init)
            } else {
                build_move(v_ptr, init, false)
            }
        }
        return init
    }

    def override visitExprMakeArray(expr : smart_ptr<ExprMakeArray>) : ExpressionPtr {
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr
    }

// ExprMakeTuple
    def override preVisitExprMakeTuple(expr : smart_ptr<ExprMakeTuple>) : void {
        var mks_ptr : LLVMOpaqueValue?
        if (expr.makeFlags.useCMRES) {
            mks_ptr = get_cmres_param()
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(types.t_int8, 0u), "")
            if (expr.extraOffset != 0u) {
                mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(expr.extraOffset)), "")
            }
        } else {
            mks_ptr = tryE(expr) <| $() {
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <| $() {
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mkt_local_sp_{expr.stackTop}")
                    LLVMSetAlignment(s_ptr, 16u)
                    s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(types.t_int8, 0u), "mkt_local_sp_{expr.stackTop}")
                }
                return s_ptr
            }
            if (expr.extraOffset != 0u) {
                mks_ptr = LLVMBuildGEP2(g_builder, types.t_int8, mks_ptr, types.ConstI32(uint64(expr.extraOffset)), "mkt_local_sp_{expr.stackTop}_eo_{expr.extraOffset}")
            }
        }
        let stride = expr.makeType.stride
        if (!expr.makeFlags.doesNotNeedInit && !expr.makeFlags.initAllFields && stride != 0) {
            let total = int(expr.values |> length)
            let bytes = max(total, 1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mks_ptr, 0ul, uint64(bytes), align)
        }
        setE(expr, mks_ptr)
    }

    def override preVisitExprMakeTupleIndex(expr : smart_ptr<ExprMakeTuple>; index : int; init : ExpressionPtr; last : bool) : void {
        let offset = get_tuple_field_offset(expr.makeType, index)
        var v_ptr = getE(expr)
        verify(v_ptr != null && LLVMIsUndef(v_ptr) == 0, "make struct field, but no pointer")
        let field_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "")
        if (init |> isMakeLocal) {
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(init))
            if (!expr.makeFlags.useCMRES) {
                val.extraOffset = uint(offset) // + expr.extraOffset
                setE(init, v_ptr)
            }
        } elif (make_call_to_cmres(init, field_ptr)) {
            pass
        }
    }

    def tuple_field_name(t : TypeDeclPtr; index : int) {
        if (!t.isTuple) {
            failed_T(t, "not a tuple {describe(t)}")
            return "!!not_a_tuple!!"
        }
        return t.argNames |> length != 0 ? "{t.argNames[index]}" : "{describe(t.argTypes[index])}"
    }

    def override visitExprMakeTupleIndex(expr : smart_ptr<ExprMakeTuple>; index : int; init : ExpressionPtr; last : bool) : ExpressionPtr {
        let offset = get_tuple_field_offset(expr.makeType, index)
        var v_ptr = getE(expr)
        let argName = "_{tuple_field_name(expr.makeType,index)}"
        v_ptr = LLVMBuildGEP2(g_builder, types.t_int8, v_ptr, types.ConstI32(uint64(offset)), "tuple_{index}{argName}")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr.makeType.argTypes[index]), "")
        if (init |> isMakeLocal) {
            pass // do nothing. we already took care of makeLocal in preVisitExprMakeStructField
        } elif (isCall2CMRES(init)) {
            pass // do nothing. we already handled this in preVisitExprMakeStructField
        } else {
            if (expr.makeType.canCopy) {
                build_copy(v_ptr, init)
            } else {
                build_move(v_ptr, init, false)
            }
        }
        return init
    }

    def override visitExprMakeTuple(expr : smart_ptr<ExprMakeTuple>) : ExpressionPtr {
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr
    }

// ExprArrayComprehension
    def override preVisitExprArrayComprehension(expr : smart_ptr<ExprArrayComprehension>) : void {
        unsupported(expr, "array comprehension")
    }

// ExprPtr2Ref
    def override visitExprPtr2Ref(expr : smart_ptr<ExprPtr2Ref>) : ExpressionPtr {
        var tsrc = getE(expr.subexpr)
        if (thisFunc != null && !thisFunc.flags.unsafeDeref) {
            check_ptr_zero(tsrc, expr.at, "dereferencing null pointer, {expr.subexpr.describe()} is null")
        }
        setE(expr, tsrc)
        return expr
    }

// ExprLabel
    def override preVisitExprLabel(expr : smart_ptr<ExprLabel>) : void {
        var lab = labels[expr.labelName]
        if (lab == null) {
            failed_E(expr, "label '{expr.labelName}' not found")
            return
        }
        if (!current_block_terminates()) {
            LLVMBuildBr(g_builder, lab)
        }
        append_label_block(lab)
        LLVMPositionBuilderAtEnd(g_builder, lab)
    }

// ExprGoto
    def override visitExprGoto(expr : smart_ptr<ExprGoto>) : ExpressionPtr {
        if (expr.subexpr != null) {
            var labI = getE(expr.subexpr)
            for (kv in sortedLabels) {
                var nextLab = append_basic_block("not_label_{kv.index}")
                let isL = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, labI, types.ConstI32(uint64(kv.index)), "")
                LLVMBuildCondBr(g_builder, isL, kv.blk, nextLab)
                LLVMPositionBuilderAtEnd(g_builder, nextLab)
            }
            build_exception("invalid label index", expr.at)
        } else {
            var lab = labels[expr.labelName]
            if (lab == null) {
                failed_E(expr, "goto '{expr.labelName}', label not found")
            } else {
                LLVMBuildBr(g_builder, lab)
            }
        }
        return expr
    }

// ExprRef2Value
    def override visitExprRef2Value(expr : smart_ptr<ExprRef2Value>) : ExpressionPtr {
        setE(expr) <| LLVMBuildLoadData2Aligned(g_builder,
            type_to_llvm_type(expr.subexpr._type), getE(expr.subexpr), expr._type.alignOf, "r2v")
        return expr
    }

// ExprRef2Ptr
    def override visitExprRef2Ptr(expr : smart_ptr<ExprRef2Ptr>) : ExpressionPtr {
        var tsrc = getE(expr.subexpr)
        setE(expr, tsrc)
        return expr
    }

// ExprAddr
    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) : void {
        var MNH_ADDR = get_function_addr(jit_context, expr.func)
        var rfun = LLVMGetUndef(g_t_function)
        let fn_ptr = save_address_global(uid.get_addr(expr), MNH_ADDR, LLVMPointerType(g_t_simFunction, 0u))
        rfun = LLVMBuildInsertValue(g_builder, rfun, fn_ptr, 0u, "")
        setE(expr, rfun)
    }

// ExprAssert
    def override visitExprAssert(expr : smart_ptr<ExprAssert>) : ExpressionPtr {
        var cond = getE(expr.arguments[0])
        var message = "assert failed"
        if (length(expr.arguments) > 1) {
            message = string((expr.arguments[1] as ExprConstString).value)
        }
        var params = fixed_array(
            get_string_constant_ptr(g_builder, message + " at {describe(expr.at)}"),
            get_context_param(),
            get_line_info_ptr(expr.at)
        )
        var check_assert = append_basic_block("assert")
        var assert_ok = append_basic_block("assert_ok")
        var assert_failed = append_basic_block("assert_failed")
        LLVMBuildBr(g_builder, check_assert)
        LLVMPositionBuilderAtEnd(g_builder, check_assert)
        LLVMBuildCondBr(g_builder, cond, assert_ok, assert_failed)
        LLVMPositionBuilderAtEnd(g_builder, assert_failed)
        var typ = g_fn_types[FN_JIT_EXCEPTION]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_EXCEPTION), params, "")
        LLVMBuildBr(g_builder, assert_ok)
        LLVMPositionBuilderAtEnd(g_builder, assert_ok)
        return expr
    }

    def get_llvm_type_for_typeinfo() : LLVMTypeRef {
        let void_ptr = types.LLVMVoidPtrType()
        return StructType(types) <| fixed_array(
            void_ptr,        // union StructInfo *, EnumInfo *, TypeAnnotation *
            void_ptr,        // TypeInfo * firstType
            void_ptr,        // TypeInfo * secondType
            void_ptr,        // TypeInfo ** argTypes
            void_ptr,        // const char ** argNames
            void_ptr,        // uint32_t * dim
            types.t_int64, // uint64_t hash
            types.t_int32, // Type type (enum Type : uint32_t)
            types.t_int32, // uint32_t enum flags: ref, refType, canCopy, etc
            types.t_int32, // uint32_t size
            types.t_int32, // uint32_t argCount
            types.t_int32  // uint32_t dimSize
        )
    }

    def get_llvm_type_for_varinfo() : LLVMTypeRef {
        let void_ptr = types.LLVMVoidPtrType()
        return StructType(types) <| fixed_array(
        // Typeinfo fields
            void_ptr,        // union StructInfo *, EnumInfo *, TypeAnnotation *
            void_ptr,        // TypeInfo * firstType
            void_ptr,        // TypeInfo * secondType
            void_ptr,        // TypeInfo ** argTypes
            void_ptr,        // const char ** argNames
            void_ptr,        // uint32_t * dim
            types.t_int64, // uint64_t hash
            types.t_int32, // Type type (enum Type : uint32_t)
            types.t_int32, // uint32_t enum flags: ref, refType, canCopy, etc
            types.t_int32, // uint32_t size
            types.t_int32, // uint32_t argCount
            types.t_int32, // uint32_t dimSize

        // Own fields
            types.LLVMFloat4Type(), // union { vec4f value; char * sValue }
            void_ptr,         // const char * name
            void_ptr,         // void * annotation_arguments
            types.t_int32,  // uint32_t offset
            types.t_int32   // uint32_t nextGcField
        )
    }

    def get_llvm_type_for_structinfo() : LLVMTypeRef {
        let void_ptr = types.LLVMVoidPtrType()
        return StructType(types) <| fixed_array(
            void_ptr,        // const char* name
            void_ptr,        // const char* module_name
            void_ptr,        // VarInfo **  fields
            void_ptr,        // void *      annotation_list

            types.t_int64, // uint64_t hash
            types.t_int64, // uint64_t init_mnh

            types.t_int32, // uint32_t flags
            types.t_int32, // uint32_t count
            types.t_int32, // uint32_t size
            types.t_int32)
    }

    def get_llvm_type_for_enumvalueinfo() : LLVMTypeRef {
        let void_ptr = types.LLVMVoidPtrType()
        return StructType(types) <| fixed_array(
            void_ptr,        // const char* name
            types.t_int64)
    }

    def get_llvm_type_for_annotation_arg_pod() : LLVMTypeRef {
        let void_ptr = types.LLVMVoidPtrType()
        return StructType(types) <| fixed_array(
            void_ptr,       // const char * name
            void_ptr,       // const char * sValue
            types.t_int32,  // int32_t type
            types.t_int32)  // int32_t iValue  (covers bool/int/float union)
    }

    def get_llvm_type_for_enuminfo(len : uint) : LLVMTypeRef {
        let void_ptr = types.LLVMVoidPtrType()
        return StructType(types) <| fixed_array(
            void_ptr,        // const char* name
            void_ptr,        // const char* module_name
            void_ptr,        // EnumValueInfo **  fields
            types.t_int32, // uint32_t count
            types.t_int64)
    }

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    def make_initializer(ty : LLVMTypeRef; values : array<LLVMValueRef>) {
        var init = LLVMGetUndef(ty)
        for (i in urange(values |> length())) {
            init = g_builder |> LLVMBuildInsertValue(init, values[i], i, "")
        }
        return init
    }

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    def create_enumvalueinfo_global_array(ei : EnumValueInfo ??; len : uint) {
        var array_init : array<LLVMValueRef>
        let ty = get_llvm_type_for_enumvalueinfo()
        for (i in range(len)) {
            assume evi = unsafe(ei[i])
            var init_fields = fixed_array(
                get_string_constant_ptr(g_builder, evi.name),
                LLVMConstInt(types.t_int64, evi.value  |> uint64(), 0)
            )
            let evi_global = LLVMAddGlobal(g_mod, ty, "enumvalueinfo")
            set_globalvar_linkage(evi_global)
            evi_global |> LLVMSetInitializer <| LLVMConstNamedStruct(ty, array_data_ptr(init_fields), 2u)
            array_init |> push <| unsafe(reinterpret<LLVMValueRef>(evi_global))
        }
        let ptr_ty = LLVMPointerType(ty, 0u)
        let arr_global = LLVMAddGlobal(g_mod, LLVMArrayType(ptr_ty, len), "enumvalueinfo_ptr_array")
        set_globalvar_linkage(arr_global)
        arr_global |> LLVMSetInitializer <| LLVMConstArray(ptr_ty, array_data_ptr(array_init), len)
        return g_builder |> LLVMBuildPointerCast(arr_global, types.LLVMVoidPtrType(), "mycast")
    }

    def get_typeinfo_fields(ti : TypeInfo?) : array<LLVMValueRef> {
        var init_values = rec_create_type_info_global(ti)
        init_values |> push <| LLVMConstInt(types.t_int64, ti.hash       |> uint64(), 0)
        init_values |> push <| types.ConstI32(ti.basicType  |> uint64())
        init_values |> push <| types.ConstI32(ti.flags      |> uint64())
        init_values |> push <| types.ConstI32(ti.size       |> uint64())
        init_values |> push <| types.ConstI32(ti.argCount   |> uint64())
        init_values |> push <| types.ConstI32(ti.dimSize    |> uint64())
        return <- init_values
    }

    def get_value_for_varinfo(vi : VarInfo?) : LLVMValueRef {
        // get_llvm_type_for_typeinfo(); // VarInfo : TypeInfo
        // types.LLVMFloat4Type(); // union { vec4f value; char * sValue }
        // void_ptr;         // const char * name
        // void_ptr;         // void * annotation_arguments
        // types.t_int32;  // uint32_t offset
        // types.t_int32   // uint32_t nextGcField
        var init_values <- get_typeinfo_fields(unsafe(reinterpret<TypeInfo?> vi))

        var x = types.ConstReal(0f |> double())
        var y = types.ConstReal(0f |> double())
        var z = types.ConstReal(0f |> double())
        var w = types.ConstReal(0f |> double())
        init_values |> push <| LLVMBuildFloat4_xyzw(g_builder, types, x, y, z, w, "float4")
        init_values |> push <| get_string_constant_ptr(g_builder, vi.name)
        init_values |> push <| LLVMConstPointerNull(types.LLVMVoidPtrType())
        init_values |> push <| types.ConstI32(vi.offset       |> uint64())
        init_values |> push <| types.ConstI32(vi.nextGcField  |> uint64())
        let ty = get_llvm_type_for_varinfo()
        return LLVMConstNamedStruct(ty, array_data_ptr(init_values), init_values |> length |> uint())
    }

    def create_struct_fields_global_array(ei : VarInfo ??; len : uint) {
        // not cached, probably don't need it
        var array_init : array<LLVMValueRef>
        let ty = get_llvm_type_for_varinfo()
        for (i in range(len)) {
            let varinfo = g_mod |> LLVMAddGlobal(ty, "varinfo")
            set_globalvar_linkage(varinfo)
            let vi = unsafe(ei[i])
            varinfo |> LLVMSetInitializer <| get_value_for_varinfo(vi)
            array_init |> push <| unsafe(reinterpret<LLVMValueRef>(varinfo))
            if (vi.annotation_arguments != null) {
                let ann_ty = get_llvm_type_for_annotation_arg_pod()
                var ann_vals : array<LLVMValueRef>
                for (arg in *vi.annotation_arguments) {
                    var fields = fixed_array(
                        get_string_constant_ptr(g_builder, string(arg.name)),
                        get_string_constant_ptr(g_builder, string(arg.sValue)),
                        types.ConstI32(uint64(int(arg.basicType))),
                        types.ConstI32(uint64(uint(arg.iValue))))
                    ann_vals |> push <| LLVMConstNamedStruct(ann_ty, array_data_ptr(fields), 4u)
                }
                if (!(ann_vals |> empty())) {
                    let n = ann_vals |> length() |> uint()
                    let ann_global = LLVMAddGlobal(g_mod, LLVMArrayType(ann_ty, n), "varinfo_annotations")
                    set_globalvar_linkage(ann_global)
                    ann_global |> LLVMSetInitializer <| LLVMConstArray(ann_ty, array_data_ptr(ann_vals), n)
                    annotation_varinfo_list |> push <| AnnotationVarInfoEntry(
                        varinfo = varinfo,
                        ann_global = ann_global,
                        count = int(n))
                }
            }
        }
        let my_struct_array_type = LLVMArrayType(LLVMPointerType(ty, 0u), len |> uint())
        let varinfo_array = LLVMAddGlobal(g_mod, my_struct_array_type, "varinfo_ptr_array")
        set_globalvar_linkage(varinfo_array)
        let initializer = LLVMConstArray(LLVMPointerType(ty, 0u), array_data_ptr(array_init), len |> uint())
        LLVMSetInitializer(varinfo_array) <| initializer

        return g_builder |> LLVMBuildPointerCast(varinfo_array, types.LLVMVoidPtrType(), "mycast")
    }

    def create_enuminfo_global(ei : EnumInfo?) {
        if (enuminfo_cache |> key_exists <| ei) {
            return enuminfo_cache[ei]
        }
        var ty = get_llvm_type_for_enuminfo(ei.count)
        var struct_info_global = LLVMAddGlobal(g_mod, ty, "enuminfo_{ei.module_name}_{ei.name}")
        set_globalvar_linkage(struct_info_global)
        enuminfo_cache[ei] = struct_info_global
        var init_values <- [
            get_string_constant_ptr(g_builder, ei.name),
            get_string_constant_ptr(g_builder, ei.module_name),
            create_enumvalueinfo_global_array(ei.fields, ei.count),
            types.ConstI32(ei.count |> uint64()),
            LLVMConstInt(types.t_int64, ei.hash  |> uint64(), 0)
        ]
        struct_info_global |> LLVMSetInitializer() <| make_initializer(ty, init_values)
        return struct_info_global
    }

    def create_struct_global(si : StructInfo?) {
        if (structinfo_cache |> key_exists <| si) {
            return structinfo_cache[si]
        }
        var ty = get_llvm_type_for_structinfo()
        var struct_info_global = LLVMAddGlobal(g_mod, ty, "si_{si.name}")
        set_globalvar_linkage(struct_info_global)
        structinfo_cache[si] = struct_info_global
        var void_ptr = types.LLVMVoidPtrType()
        var init_values <- [
            get_string_constant_ptr(g_builder, si.name),
            get_string_constant_ptr(g_builder, si.module_name),
            create_struct_fields_global_array(si.fields, si.count),
            LLVMConstPointerNull(void_ptr), // NOTE: annotations are not used right now

            LLVMConstInt(types.t_int64, si.hash     |> uint64(), 0),
            LLVMConstInt(types.t_int64, si.init_mnh |> uint64(), 0),

            types.ConstI32(si.flags |> uint64()),
            types.ConstI32(si.count |> uint64()),
            types.ConstI32(si.size  |> uint64()),
            types.ConstI32(si.firstGcField |> uint64())]
        struct_info_global |> LLVMSetInitializer() <| make_initializer(ty, init_values)
        return struct_info_global
    }

    def create_struct_global_pointer(si : StructInfo?) {
        var tinfo_global = create_struct_global(si)
        // g_engine |> LLVMAddGlobalMapping(tinfo_global, si)
        return tinfo_global
    }

    def create_enum_global_pointer(si : EnumInfo?) {
        var tinfo_global = create_enuminfo_global(si)
        // g_engine |> LLVMAddGlobalMapping(tinfo_global, si)
        return tinfo_global
    }

    def create_argtypes_array(argtypes : TypeInfo ??; len : uint) {
        var array_init : array<LLVMValueRef>
        if (len == 0u) {
            return LLVMConstPointerNull(LLVMPointerType(types.t_int8, 0u))
        }
        for (i in range(len)) {
            let evi = unsafe(argtypes[i]) |> create_type_info_global()
            array_init |> push <| unsafe(reinterpret<LLVMValueRef>(evi))
        }
        let element_t = types.LLVMVoidPtrType();
        let tinfo_array_ty = LLVMArrayType(element_t, len)
        var global = LLVMAddGlobal(g_mod, tinfo_array_ty, "argtypes")
        set_globalvar_linkage(global)
        global |> LLVMSetInitializer <| LLVMConstArray(element_t, array_data_ptr(array_init), len)
        return global
    }

    def create_argnames_array(argnames : string const?; len : uint) {
        if (len == 0u) {
            return LLVMConstPointerNull(LLVMPointerType(types.t_int8, 0u))
        }
        var array_init : array<LLVMValueRef>
        var ty = LLVMPointerType(types.t_int8, 0u)
        for (i in range(len)) {
            array_init |> push <| get_string_constant_ptr(g_builder, unsafe(argnames[i]))
        }
        let tinfo_array_ty = LLVMArrayType(ty, len)
        var global = LLVMAddGlobal(g_mod, tinfo_array_ty, "argnames")
        set_globalvar_linkage(global)
        global |> LLVMSetInitializer <| LLVMConstArray(ty, array_data_ptr(array_init), len)
        return global
    }

    def rec_create_type_info_global(info : TypeInfo?) {
        var initValues = array<LLVMValueRef>()
        initValues |> resize(6)
        for (i in range(6)) {
            initValues[i] = LLVMConstPointerNull(types.LLVMVoidPtrType())
        }
        if (info.basicType == Type.tHandle) {
            let resolvent = "~{info.annotation._module.name}::{info.annotation.name}"
            var llvm_resolvent = g_builder |> get_string_constant_ptr(resolvent)
            LLVMSetAlignment(llvm_resolvent, 2u)
            // now do this:
            //      info->annotation_or_name =  ((TypeAnnotation*)(intptr_t(annName)|1));
            let one = LLVMConstInt(types.t_int64, 1 |> uint64(), 0)
            let ptrToInt = LLVMBuildPtrToInt(g_builder, llvm_resolvent, types.t_int64, "ptrToInt")
            // Or constexpr are forbidden. Since we have alignment we can use add.
            let modifiedPtr = LLVMBuildAdd(g_builder, ptrToInt, one, "modifiedPtr")
            let intToPtr = LLVMBuildIntToPtr(g_builder, modifiedPtr, types.LLVMVoidPtrType(), "intToPtr")

            initValues[0] = intToPtr
        } elif (info.basicType == Type.tStructure) {
            var t = create_struct_global_pointer(info.structType)
            initValues[0] = t
            if (info.firstType != null) {
                initValues[1] = create_type_info_global(info.firstType)
            }
        } elif (info.basicType == Type.tEnumeration || info.basicType == Type.tEnumeration8 ||
                info.basicType == Type.tEnumeration16 || info.basicType == Type.tEnumeration64) {
            initValues[0] = create_enum_global_pointer(info.enumType)
        } elif (info.basicType == Type.tPointer || info.basicType == Type.tArray) {
            initValues[1] = create_type_info_global(info.firstType)
        } elif (info.basicType == Type.tTable) {
            initValues[1] = create_type_info_global(info.firstType)
            initValues[2] = create_type_info_global(info.secondType)
        } elif (info.basicType == Type.tTuple || info.basicType == Type.tVariant) {
            var aa = create_argtypes_array(info.argTypes, info.argCount)
            var an = create_argnames_array(info.argNames, info.argNames == null ? 0u : info.argCount)
            initValues[3] = aa
            initValues[4] = an
        } elif (info.basicType == Type.tFunction || info.basicType == Type.tLambda || info.basicType == Type.tBlock) {
            var aa = create_argtypes_array(info.argTypes, info.argCount)
            var an = create_argnames_array(info.argNames, info.argCount)
            initValues[1] = create_type_info_global(info.firstType)
            initValues[3] = aa
            initValues[4] = an
        } elif (info.basicType == Type.tBitfield || info.basicType == Type.tBitfield8 ||
                info.basicType == Type.tBitfield16 || info.basicType == Type.tBitfield64) {
            var an = create_argnames_array(info.argNames, info.argNames == null ? 0u : info.argCount)
            initValues[4] = an
        } elif (info.basicType == Type.tIterator) {
            initValues[1] = create_type_info_global(info.firstType)
        }
        // Fill dim pointer (slot 5), needed for fixed-size arrays like int[3].
        if (info.dimSize > 0u) {
            var dim_vals : array<LLVMValueRef>
            for (i in range(info.dimSize)) {
                dim_vals |> push <| types.ConstI32(unsafe(info.dim[i]) |> uint64())
            }
            let dim_arr_ty = LLVMArrayType(types.t_int32, info.dimSize)
            var dim_global = LLVMAddGlobal(g_mod, dim_arr_ty, "dim")
            set_globalvar_linkage(dim_global)
            dim_global |> LLVMSetInitializer <| LLVMConstArray(types.t_int32, array_data_ptr(dim_vals), info.dimSize)
            initValues[5] = dim_global
        }
        return <- initValues
    }

    def create_type_info_global(ti : TypeInfo?) {
        if (ti == null) {
            return LLVMConstPointerNull(types.LLVMVoidPtrType())
        }
        if (typeinfo_cache |> key_exists <| ti) {
            return typeinfo_cache[ti]
        }

        var type_info_type = get_llvm_type_for_typeinfo()
        let type_info_global = LLVMAddGlobal(g_mod, type_info_type, "ti")
        set_globalvar_linkage(type_info_global)

        typeinfo_cache[ti] = type_info_global

        var init_values <- get_typeinfo_fields(ti)
        var type_info_init = LLVMGetUndef(type_info_type)
        for (i in urange(init_values |> length())) {
            type_info_init = g_builder |> LLVMBuildInsertValue(type_info_init, init_values[i], i, "")
        }
        LLVMSetInitializer(type_info_global, type_info_init)
        return g_builder |> LLVMBuildPointerCast(type_info_global, types.LLVMVoidPtrType(), "")
    }

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    // cache maps pointer to pointer to global variable
    structinfo_cache : table<StructInfo?; LLVMValueRef>
    typeinfo_cache :   table<TypeInfo?; LLVMValueRef>
    enuminfo_cache :   table<EnumInfo?; LLVMValueRef>
    varinfo_cache :    table<VarInfo?; LLVMValueRef>

// ExprDebug
    def override visitExprDebug(expr : smart_ptr<ExprDebug>) : ExpressionPtr {
        // // void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * )
        var message = ""
        if (length(expr.arguments) > 1) {
            message = string((expr.arguments[1] as ExprConstString).value)
        }
        var tinfo = make_type_info_structure(*jit_context, expr.arguments[0]._type)
        var tinfo_global = create_type_info_global(tinfo)
        var params <- fixed_array(
            cast_to_vec4f(expr.arguments[0]._type, getE(expr.arguments[0])),
            tinfo_global,
            get_string_constant_ptr(g_builder, message),
            get_context_param(),
            get_line_info_ptr(expr.at)   // at
        )
        var typ = g_fn_types[FN_JIT_DEBUG]
        var dbg = LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_DEBUG), params, "")
        setE(expr, dbg)
        return expr
    }

// ExprInvoke
    def override canVisitExprLooksLikeCallArgument(expr : smart_ptr<ExprLooksLikeCall>; arg : ExpressionPtr; last : bool) : bool {
        if (expr is ExprInvoke) {
            if (length(expr.arguments) >= 1 && expr.arguments[0] == arg && (expr as ExprInvoke).isInvokeMethod) {
                return false
            }
        }
        return true
    }

    def override visitExprInvoke(expr : smart_ptr<ExprInvoke>) : ExpressionPtr {
        assume blockT = expr.arguments[0]._type
        var cmresEval : LLVMOpaqueValue?
        var cmresEvalPtr : LLVMOpaqueValue?
        if (isCMRESType(expr._type)) {
            if (call2cmres |> key_exists(get_ptr(expr))) {
                cmresEval = call2cmres[get_expr_ptr(expr)]
                cmresEvalPtr = LLVMBuildPointerCast(g_builder, cmresEval, types.LLVMVoidPtrType(), "invoke_cmres_eval_{int(expr.at.line)}_ptr")
            } else {
                var vtype = type_to_llvm_type(expr._type)
                at_function_entry <| $() {
                    cmresEval = LLVMBuildAlloca(g_builder, vtype, "invoke_cmres_eval_{int(expr.at.line)}_var")
                    LLVMSetAlignment(cmresEval, 16u)
                    cmresEvalPtr = LLVMBuildPointerCast(g_builder, cmresEval, types.LLVMVoidPtrType(), "invoke_cmres_eval_{int(expr.at.line)}_ptr")
                }
            }
        }
        if (blockT.isGoodFunctionType && expr.isInvokeMethod) {
            var methodOffset = -1
            if (expr.arguments[0] is ExprField) {
                var efield = expr.arguments[0] as ExprField
                methodOffset = efield.field.offset
            } elif (expr.arguments[0] is ExprRef2Value) {
                var er2v = expr.arguments[0] as ExprRef2Value
                if (er2v.subexpr is ExprField) {
                    var efield = er2v.subexpr as ExprField
                    methodOffset = efield.field.offset
                } else {
                    failed_E(expr, "method call expects field and got {describe(expr.arguments[0])}")
                }
            } else {
                failed_E(expr, "method call expects field and got {describe(expr.arguments[0])}")
            }
            var args = build_array_of_arguments(expr.arguments, true)
            var classPtr = getE(expr.arguments[1])
            classPtr = LLVMBuildPointerCast(g_builder, classPtr, LLVMPointerType(types.t_int8, 0u), "")
            classPtr = LLVMBuildGEP2(g_builder, types.t_int8, classPtr, types.ConstI32(uint64(methodOffset)), "")
            var FUNC = LLVMBuildPointerCast(g_builder, classPtr, LLVMPointerType(g_t_function, 0u), "")
            FUNC = LLVMBuildLoad2(g_builder, g_t_function, FUNC, "FUNC")
            var SIMFUNCTION = LLVMBuildExtractValue(g_builder, FUNC, uint(JIT_FUNCTION.SIM_FUNCTION), "SIMFUNCTION")
            SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, types.LLVMVoidPtrType(), "")
            check_ptr_zero(SIMFUNCTION, expr.at, "invoke null method")
            var params : array<LLVMOpaqueValue?>
            params |> push(SIMFUNCTION)
            params |> push(args)
            if (cmresEval != null) {
                params |> push(cmresEvalPtr)
            }
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if (cmresEval != null) {
                setE(expr, cmresEval)
            } elif (blockT.firstType.isVoid) {
                setE(expr, ccall)
            } else {
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
            }
        } elif (blockT.isGoodFunctionType) {
            var args = build_array_of_arguments(expr.arguments, true)
            var FUNC = getE(expr.arguments[0])
            var SIMFUNCTION = LLVMBuildExtractValue(g_builder, FUNC, uint(JIT_FUNCTION.SIM_FUNCTION), "SIMFUNCTION")
            SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, types.LLVMVoidPtrType(), "")
            check_ptr_zero(SIMFUNCTION, expr.at, "invoke null function")
            var params : array<LLVMOpaqueValue?>
            params |> push(SIMFUNCTION)
            params |> push(args)
            if (cmresEval != null) {
                params |> push(cmresEvalPtr)
            }
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if (cmresEval != null) {
                setE(expr, cmresEval)
            } elif (blockT.firstType.isVoid) {
                setE(expr, ccall)
            } else {
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
            }
        } elif (blockT.isGoodLambdaType) {
            var args = build_array_of_arguments(expr.arguments, false)
            var LAMBDA = getE(expr.arguments[0])
            check_ptr_zero(LAMBDA, expr.at, "invoke null lambda")
            LAMBDA = LLVMBuildPointerCast(g_builder, LAMBDA, LLVMPointerType(g_t_lambda, 0u), "LAMBDA ON HEAP")
            // debug("Lambda type: {describe(LLVMTypeOf(LAMBDA))}, {expr.at} : {describe(expr)}")
            LAMBDA = LLVMBuildLoad2(g_builder, g_t_lambda, LAMBDA, "LAMBDA")
            var SIMFUNCTION = LLVMBuildExtractValue(g_builder, LAMBDA, uint(JIT_LAMBDA.EVAL), "EVAL")
            SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, types.LLVMVoidPtrType(), "")
            var params : array<LLVMOpaqueValue?>
            params |> push(SIMFUNCTION)
            params |> push(args)
            if (cmresEval != null) {
                params |> push(cmresEvalPtr)
            }
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval != null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if (cmresEval != null) {
                setE(expr, cmresEval)
            } elif (blockT.firstType.isVoid) {
                setE(expr, ccall)
            } else {
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
            }
        } elif (blockT.isGoodBlockType) {
            var args = build_array_of_arguments(expr.arguments, true)
            var blk = getE(expr.arguments[0])
            check_ptr_zero(blk, expr.at, "invoke null block")
            var params : array<LLVMOpaqueValue?>
            params |> push(blk)
            params |> push(args)
            if (cmresEval != null) {
                params |> push(cmresEvalPtr)
            }
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval != null ? FN_JIT_INVOKE_BLOCK_WITH_CMRES : FN_JIT_INVOKE_BLOCK)
            var typ = g_fn_types[cmresEval != null ? FN_JIT_INVOKE_BLOCK_WITH_CMRES : FN_JIT_INVOKE_BLOCK]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if (cmresEval != null) {
                setE(expr, cmresEval)
            } elif (blockT.firstType == null || blockT.firstType.isVoid) {
                setE(expr, ccall)
            } else {
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
            }
        } else {
            failed_E(expr, "invoke({describe(blockT)},...) is not supported yet")
        }
        return expr
    }

// ExprErase
    def build_table_erase(ttype : Type; tab : LLVMOpaqueValue?; key : LLVMOpaqueValue?; valueTypeSize : int) : LLVMOpaqueValue? {
        var params = fixed_array(
            tab,                                                        // table ptr
            key,                                                        // key
            types.ConstI32(uint64(valueTypeSize)),    // valueTypeSize
            get_context_param()
        )
        var (fun_ptr, typ) = get_table_erase(g_builder, g_mod, ttype, types)
        let ccall = LLVMBuildCall2(g_builder, typ, fun_ptr, params, "")
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.nounwind)     // TODO: better no-unwind?
        LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.willreturn)

        LLVMAddCallSiteAttribute(ccall, 1u, attributes.nocapture)
        LLVMAddCallSiteAttribute(ccall, 4u, attributes.nocapture)

        return ccall
    }

    def override visitExprErase(expr : smart_ptr<ExprErase>) : ExpressionPtr {
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "tab_ptr")
        var keyType : Type
        if (subExprT.firstType.isWorkhorseType) {
            keyType = subExprT.firstType.baseType
        } else {
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        }
        var res = build_table_erase(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
        setE(expr, res)
        return expr
    }

// ExprSetInsert
    def override visitExprSetInsert(expr : smart_ptr<ExprSetInsert>) : ExpressionPtr {
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "tab_ptr")
        var keyType : Type
        if (subExprT.firstType.isWorkhorseType) {
            keyType = subExprT.firstType.baseType
        } else {
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        }
        var res = build_table_at(keyType, tab_ptr, tidx, 0, expr.at)
        setE(expr, res)
        return expr
    }

// ExprFind
    def override visitExprFind(expr : smart_ptr<ExprFind>) : ExpressionPtr {
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "tab_ptr")
        var keyType : Type
        if (subExprT.firstType.isWorkhorseType) {
            keyType = subExprT.firstType.baseType
        } else {
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        }
        var at_tab = build_table_find(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, at_tab, types.ConstI32(uint64(-1)), "")
        var resType = LLVMPointerType(type_to_llvm_type(subExprT.secondType), 0u)
        var res = build_select(cond, resType) <| $() {
            var inscope tab_ty := expr.arguments[0]._type.isPointer ? expr.arguments[0]._type.firstType : expr.arguments[0]._type
            var tab = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(tab_ty), tsrc, tab_ty.alignOf, "tab")
            var data = LLVMBuildExtractValue(g_builder, tab, uint(JIT_TABLE.DATA), "table.data")
            return LLVMBuildGEP2(g_builder, type_to_llvm_type(subExprT.secondType), data, at_tab, "table_at")
        }
        lpipe <| $() {
            return LLVMConstPointerNull(resType)
        }
        setE(expr, res)
        return expr
    }

// ExprKeyExists
    def override visitExprKeyExists(expr : smart_ptr<ExprKeyExists>) : ExpressionPtr {
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "tab_ptr")
        var keyType : Type
        if (subExprT.firstType.isWorkhorseType) {
            keyType = subExprT.firstType.baseType
        } else {
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        }
        var at_tab = build_table_find(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
        var res = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, at_tab, types.ConstI32(uint64(-1)), "")
        setE(expr, res)
        return expr
    }

// ExprAscend
    def override preVisitExprAscend(expr : smart_ptr<ExprAscend>) : void {
        var bytes = expr.subexpr._type.sizeOf
        let needTypeInfo = expr.ascendFlags.needTypeInfo
        if (needTypeInfo) {
            bytes += 16
        }
        var mks_ptr : LLVMOpaqueValue?
        var needMemset = true
        if (expr.subexpr._type.isHandle) {
            mks_ptr = alloc_handle_type(expr.subexpr._type, uid.get_ascend_new(expr))
            needMemset = false
        } else {
            mks_ptr = build_alloc(bytes, false)
        }
        check_ptr_zero(mks_ptr, expr.at, "new [[...]] returned null")
        var memset_bytes = bytes
        if (needTypeInfo) {
            var tinfo = make_type_info_structure(*jit_context, expr.subexpr._type)
            let tptr = create_type_info_global(tinfo)
            LLVMBuildStore2(g_builder, types, types.LLVMVoidPtrType(), tptr, mks_ptr)
            // skip 16 bytes in mks_ptr
            var p2int = LLVMBuildPtrToInt(g_builder, mks_ptr, types.LLVMIntPtrType(), "")
            var add16 = LLVMBuildAdd(g_builder, p2int, LLVMConstInt(types.LLVMIntPtrType(), uint64(16), 0), "")
            mks_ptr = LLVMBuildIntToPtr(g_builder, add16, types.LLVMVoidPtrType(), "")
            memset_bytes -= 16
        }
        if (needMemset) {
            LLVMBuildMemSet(g_builder, mks_ptr, 0ul, uint64(memset_bytes), 16u)
        }
        if (expr.ascendFlags.useStackRef) {
            let stackref_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(types.t_int8, 0u), "")
            setE(expr.subexpr, stackref_ptr)
        } elif (!expr.subexpr._type.isHandle) {
            call2cmres[get_ptr(expr.subexpr)] = mks_ptr
        }
        mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, type_to_llvm_type(expr._type), "")
        setE(expr, mks_ptr)
    }

// ExprCast
    def override visitExprCast(expr : smart_ptr<ExprCast>) : ExpressionPtr {
        var res : LLVMOpaqueValue?
        assume ect = expr._type
        assume subT = expr.subexpr._type
        assume subE = getE(expr.subexpr)
        if (ect.isRef && subT.isRef) {
            res = LLVMBuildPointerCast(g_builder, subE, LLVMPointerType(type_to_llvm_type(ect), 0u), "")
        } elif (ect.isPointer && (subT.isPointer || subT.isRefType)) {
            res = LLVMBuildPointerCast(g_builder, subE, type_to_llvm_type(ect), "cast_p")
        } elif (length(ect.dim) != 0 && subT.isPointer) {
            res = LLVMBuildPointerCast(g_builder, subE, LLVMPointerType(type_to_llvm_type(ect), 0u), "")
        } elif (ect.isInteger && (ect.baseType == Type.tInt64 || ect.baseType == Type.tUInt64) && subT.isPointer) {
            res = LLVMBuildPtrToInt(g_builder, subE, type_to_llvm_type(ect), "cast_p_i")
        } elif ((ect.isPointer || ect.isString) && subT.isInteger) {
            res = LLVMBuildIntToPtr(g_builder, subE, type_to_llvm_type(ect), "cast_i_p")
        } elif (!ect.isRefType && !subT.isRefType) {
            if (ect.sizeOf == subT.sizeOf) {
                res = LLVMBuildBitCast(g_builder, subE, type_to_llvm_type(ect), "cast_r")
            } elif (ect.sizeOf > subT.sizeOf) {
                if (subT.isSignedInteger) {
                    res = LLVMBuildSExt(g_builder, subE, type_to_llvm_type(ect), "sext_cast")
                } else {
                    res = LLVMBuildZExt(g_builder, subE, type_to_llvm_type(ect), "zext_cast")
                }
            } else {
                res = LLVMBuildTrunc(g_builder, subE, type_to_llvm_type(ect), "trunc_cast")
            }
        } else {
            res = LLVMGetUndef(type_to_llvm_type(ect))
            failed_E(expr, "cast<{describe(expr.castType)}> is not supported yet in {describe(expr)} {describe(subT)} -> {describe(ect)}")
        }
        setE(expr, res)
        return expr
    }

// ExprDelete
    def build_lambda_delete(lmb : LLVMOpaqueValue?; at : LineInfo) {
        var args : LLVMOpaqueValue?
        at_function_entry <| $() {
            args = LLVMBuildAlloca(g_builder, LLVMArrayType(types.LLVMFloat4Type(), 1u), "args")
            LLVMSetAlignment(args, 16u)
            args = LLVMBuildPointerCast(g_builder, args, LLVMPointerType(types.LLVMFloat4Type(), 0u), "delete lambda args ptr")
        }
        var is_zero = check_zero(lmb)
        var end = append_basic_block("end")
        LLVMBuildBr(g_builder, end)
        LLVMPositionBuilderAtEnd(g_builder, is_zero)
        // build arguments (1, and its 'capture')
        let ai = 0
        var arg_ptr = LLVMBuildGEP2(g_builder, types.LLVMFloat4Type(), args, types.ConstI32(uint64(ai)), "call_arg_{ai}_ptr")
        var arg_val = lmb
        var arg_cast_type = types.LLVMVoidPtrType()
        LLVMBuildStore2(g_builder, types, arg_cast_type, arg_val, arg_ptr)
        // now invoke
        var LAMBDA = LLVMBuildPointerCast(g_builder, lmb, LLVMPointerType(g_t_lambda, 0u), "LAMBDA ON HEAP")
        LAMBDA = LLVMBuildLoad2(g_builder, g_t_lambda, LAMBDA, "LAMBDA")
        var SIMFUNCTION = LLVMBuildExtractValue(g_builder, LAMBDA, uint(JIT_LAMBDA.FINALIZER), "FINALIZER")
        SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, types.LLVMVoidPtrType(), "")
        var params : array<LLVMOpaqueValue?>
        params |> push(SIMFUNCTION)
        params |> push(args)
        params |> push(get_context_param())
        var func = LLVMGetNamedFunction(g_mod, FN_JIT_CALL_OR_FASTCALL)
        var typ = g_fn_types[FN_JIT_CALL_OR_FASTCALL]
        LLVMBuildCall2(g_builder, typ, func, params, "delete_lambda")
        LLVMBuildBr(g_builder, end)
        LLVMPositionBuilderAtEnd(g_builder, end)
    }

    def get_delete_handle_function(t : TypeDeclPtr) : LLVMOpaqueValue? {
        let func_name = uid.get_delete_type(t)
        var delete_handle = LLVMGetNamedGlobal(g_mod, func_name.glob())
        let fn_type = LLVMFunctionType(types.t_void,
                fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(), types.LLVMVoidPtrType()))
        if (delete_handle == null) {
            var delete_handle_ptr = get_jit_delete(t.annotation)
            if (delete_handle_ptr == null) {
                failed_T(t, "missing delete`handle function pointer for {describe(t)}")
                return null
            }
            // void delete_handle ( void *; Context * ) {
            delete_handle = save_address_global(func_name, delete_handle_ptr, LLVMPointerType(fn_type, 0u))
            g_fn_types[func_name.publ()] = fn_type
        } else {
            delete_handle = get_address_global(func_name, LLVMPointerType(fn_type, 0u))
        }
        return delete_handle
    }

    def get_clone_handle_function(t : TypeDeclPtr) : LLVMOpaqueValue? {
        let func_name = uid.get_clone_type(t)
        var clone_handle = LLVMGetNamedGlobal(g_mod, func_name.glob())
        let fn_type = LLVMFunctionType(types.t_void,
                fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(), types.LLVMVoidPtrType()))
        if (clone_handle == null) {
            var clone_handle_ptr = get_jit_clone(t.annotation)
            if (clone_handle_ptr == null) {
                failed_T(t, "missing clone`handle function pointer for {describe(t)}")
                return null
            }
            // void clone_handle ( void * dst, void * src )
            clone_handle = save_address_global(func_name, clone_handle_ptr, LLVMPointerType(fn_type, 0u))
            g_fn_types[func_name.publ()] = fn_type
        } else {
            clone_handle = get_address_global(func_name, LLVMPointerType(fn_type, 0u))
        }
        return clone_handle
    }

    def override visitExprClone(expr : smart_ptr<ExprClone>) : ExpressionPtr {
        assume leftT = expr.left._type
        if (leftT.isHandle) {
            var tdst = getE(expr.left)
            var tsrc = getE(expr.right)
            if (!expr.right._type.isRef && expr.right._type.annotation != null && !expr.right._type.annotation.isRefType) {
                // r-value source — need to take address via alloca
                var tmp = LLVMBuildAlloca(g_builder, type_to_llvm_type(expr.right._type), "clone_src_tmp")
                LLVMBuildStore(g_builder, tsrc, tmp)
                tsrc = tmp
            }
            var clone_fn = get_clone_handle_function(leftT)
            if (clone_fn != null) {
                let fn_type = g_fn_types[uid.get_clone_type(leftT).publ()]
                var params <- fixed_array(
                    LLVMBuildPointerCast(g_builder, tdst, types.LLVMVoidPtrType(), ""),
                    LLVMBuildPointerCast(g_builder, tsrc, types.LLVMVoidPtrType(), "")
                )
                LLVMBuildCall2(g_builder, fn_type, clone_fn, params, "")
            }
        } elif (leftT.canCopy) {
            build_copy(getE(expr.left), expr.right)
        } else {
            failed_E(expr, "clone of {describe(leftT)} is not supported yet")
        }
        return expr
    }

    def override visitExprDelete(expr : smart_ptr<ExprDelete>) : ExpressionPtr {
        assume subexpr = expr.subexpr
        assume subT = expr.subexpr._type
        let total = subT.countOf
        if (total != 1) {
            failed_E(expr, "delete of multiple elements is not supported yet {describe(subT)} [{total}]")
            return expr
        }
        var se = getE(subexpr)
        if (subT.isPointer) {
            assume firstT = expr.subexpr._type.firstType
            if (firstT.isStructure) {
                if (!firstT.structType.flags.isClass) {
                    var structSize = firstT.sizeOf
                    let persistent = firstT.structType.flags.persistent
                    let isLambda = firstT.structType.flags.isLambda
                    var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(types.LLVMVoidPtrType(), 0u), "ppStruct")
                    var ps = LLVMBuildLoad2(g_builder, types.LLVMVoidPtrType(), pps, "pStruct")
                    if (persistent) {
                        var params = fixed_array(
                            ps,
                            get_context_param()
                        )
                        var typ = g_fn_types[FN_JIT_FREE_PERSISTENT]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_PERSISTENT), params, "")
                    } elif (isLambda) {
                        structSize += 16
                        var p2int = LLVMBuildPtrToInt(g_builder, ps, types.LLVMIntPtrType(), "")
                        var sub16 = LLVMBuildSub(g_builder, p2int, LLVMConstInt(types.LLVMIntPtrType(), uint64(16), 0), "")
                        var lmb = LLVMBuildIntToPtr(g_builder, sub16, types.LLVMVoidPtrType(), "")
                        var params = fixed_array(
                            lmb,
                            types.ConstI32(uint64(structSize)),
                            get_context_param()
                        )
                        var typ = g_fn_types[FN_JIT_FREE_HEAP]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                    } else {
                        var params = fixed_array(
                            ps,
                            types.ConstI32(uint64(structSize)),
                            get_context_param()
                        )
                        var typ = g_fn_types[FN_JIT_FREE_HEAP]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                    }
                    LLVMBuildStore(g_builder, LLVMConstNull(types.LLVMVoidPtrType()), pps)
                } else {
                    var structSize = getE(expr.sizeexpr)
                    let persistent = firstT.structType.flags.persistent
                    var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(types.LLVMVoidPtrType(), 0u), "ppClass")
                    var ps = LLVMBuildLoad2(g_builder, types.LLVMVoidPtrType(), pps, "pClass")
                    if (persistent) {
                        var params = fixed_array(
                            ps,
                            get_context_param()
                        )
                        var typ = g_fn_types[FN_JIT_FREE_PERSISTENT]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_PERSISTENT), params, "")
                    } else {
                        var params = fixed_array(
                            ps,
                            structSize,
                            get_context_param()
                        )
                        var typ = g_fn_types[FN_JIT_FREE_HEAP]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                    }
                    LLVMBuildStore(g_builder, LLVMConstNull(types.LLVMVoidPtrType()), pps)
                }
            } elif (firstT.isHandle) {
                let fn_delete_handle = get_delete_handle_function(firstT)
                if (fn_delete_handle == null) {
                    return expr
                }
                var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(types.LLVMVoidPtrType(), 0u), "")
                var ps = LLVMBuildLoad2(g_builder, types.LLVMVoidPtrType(), pps, "delete`handle`{describe(firstT)}")
                var params = fixed_array(
                    ps,
                    get_context_param()
                )
                let typ = g_fn_types[uid.get_delete_type(firstT).publ()]
                let ccall = LLVMBuildCall2(g_builder, typ, fn_delete_handle, params, "")
                LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.willreturn)
                LLVMAddCallSiteAttribute(ccall, -1 |> uint(), attributes.nounwind)
                LLVMAddCallSiteAttribute(ccall, 1 |> uint(), attributes.nocapture)
                LLVMBuildStore(g_builder, LLVMConstNull(types.LLVMVoidPtrType()), pps)
            } elif (firstT.isVariant || firstT.isTuple) {
                var structSize = firstT.sizeOf
                var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(types.LLVMVoidPtrType(), 0u), "ppData")
                var ps = LLVMBuildLoad2(g_builder, types.LLVMVoidPtrType(), pps, "pData")
                var params = fixed_array(
                    ps,
                    types.ConstI32(uint64(structSize)),
                    get_context_param()
                )
                var typ = g_fn_types[FN_JIT_FREE_HEAP]
                LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                LLVMBuildStore(g_builder, LLVMConstNull(types.LLVMVoidPtrType()), pps)
            } else {
                failed_E(expr, "delete of pointer non-structure type is not supported yet {describe(subT)}")
            }
        } elif (subT.isGoodLambdaType) {
            var lse = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), se, subT.alignOf, "lambda_r2v")
            build_lambda_delete(lse, expr.at)
        } else {
            failed_E(expr, "delete of non-pointer type is not supported yet {describe(subT)}")
        }
        return expr
    }

// ExprVar
    def get_global_offset_by_mnh(mnh : uint64; shared_mnh : bool; name : string) {
        var params = fixed_array(
            LLVMConstInt(types.t_int64, mnh, 0),
            get_context_param()
        )
        var func = LLVMGetNamedFunction(g_mod, shared_mnh ? FN_JIT_GET_SHARED_MNH : FN_JIT_GET_GLOBAL_MNH)
        var typ = g_fn_types[shared_mnh ? FN_JIT_GET_SHARED_MNH : FN_JIT_GET_GLOBAL_MNH]
        var res = LLVMBuildCall2(g_builder, typ, func, params, name)
        return res
    }

    def override visitExprVar(expr : smart_ptr<ExprVar>) : ExpressionPtr {
        assume varType = expr.variable._type
        if (expr.varFlags._block) {
            var tryV = tryGetV(expr.variable)
            if (tryV != null) {
                if (expr.varFlags.r2v) {
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.variable._type), tryV, expr._type.alignOf, "var_block_r2v_{expr.variable.name}"))
                } else {
                    setE(expr, tryV)
                }
                return expr
            }
            var arg = LLVMGetParam(ffunc, uint(expr.argumentIndex))
            if (expr.varFlags.r2v) {
                if (varType.flags.ref) {
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.variable._type), arg, expr._type.alignOf, "var_block_ref_r2v_{expr.variable.name}"))
                } else {
                    setE(expr, arg)
                }
            } else {
                setE(expr, arg)
            }
        } elif (expr.varFlags.local) { // local variables
            var var_ptr = getV(expr.variable)
            if (expr.variable.flags.aliasCMRES) {
                pass
            }
            if (varType.flags.ref) {
                var_ptr = LLVMBuildLoad2(g_builder, LLVMPointerType(type_to_llvm_type(expr._type), 0u), var_ptr, "")
            }
            if (expr.varFlags.r2v) {
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), var_ptr, expr._type.alignOf, "local_var_r2v_{expr.variable.name}"))
            } else {
                setE(expr, var_ptr)
            }
        } elif (expr.varFlags.argument) { // arguments
            var tryV = tryGetV(expr.variable)
            if (tryV != null) {
                if (expr.varFlags.r2v) {
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), tryV, expr._type.alignOf, "arg_var_r2v_{expr.variable.name}"))
                } else {
                    setE(expr, tryV)
                }
                return expr
            }
            var arg = LLVMGetParam(ffunc, uint(expr.argumentIndex))
            if (expr.varFlags.r2v && varType.flags.ref) {
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), arg, expr._type.alignOf, "arg_var_ref_r2v_{expr.variable.name}"))
            } else {
                setE(expr, arg)
            }
        } else {
            var tryV = tryGetV(expr.variable)
            if (tryV == null) {
                at_function_entry <| $() {
                    let isSolidContext = expr.variable._module.moduleFlags.isSolidContext
                    var vptr : LLVMOpaqueValue?
                    if (!isSolidContext) {
                        let mnh = hash(get_mangled_name(expr.variable))
                        vptr = get_global_offset_by_mnh(mnh, expr.variable.flags.global_shared, "global_var_{expr.variable.name}_mnh_ptr")
                        vptr = LLVMBuildPointerCast(g_builder, vptr, get_type_pointer(expr.variable._type), "global_var_{expr.variable.name}")
                    } else {
                        var ctx_ptr = LLVMBuildPointerCast(g_builder, get_context_param(), LLVMPointerType(types.t_int8, 0u), "")
                        let coffset = expr.variable.flags.global_shared ? CONTEXT_OFFSET_OF_SHARED : CONTEXT_OFFSET_OF_GLOBALS
                        var global_ptr = LLVMBuildGEP2(g_builder, types.t_int8, ctx_ptr, types.ConstI32(uint64(coffset)), "")
                        global_ptr = LLVMBuildLoad2(g_builder, LLVMPointerType(types.t_int8, 0u), global_ptr, "")
                        vptr = LLVMBuildGEP2(g_builder, types.t_int8, global_ptr, types.ConstI32(uint64(expr.variable.stackTop)), "")
                    }
                    tryV = LLVMBuildPointerCast(g_builder, vptr, get_type_pointer(expr.variable._type), "global_var_{expr.variable.name}")
                }
            }
            if (expr.varFlags.r2v) {
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), tryV, expr._type.alignOf, "global_var_r2v_{expr.variable.name}"))
            } else {
                setE(expr, tryV)
            }
        }
        return expr
    }

// ExprField
    def variant_field_name(t : TypeDeclPtr; index : int) {
        if (!t.isVariant) {
            failed_T(t, "not a variant {describe(t)}")
            return "!!not_a_variant!!"
        }
        return t.argNames |> length != 0 ? "{t.argNames[index]}" : "{describe(t.argTypes[index])}"
    }

    def override visitExprField(expr : smart_ptr<ExprField>) : ExpressionPtr {
        var field_ptr : LLVMOpaqueValue?;
        assume value_t = expr.value._type
        if (value_t.isBitfield) {
            let mask = 1u << uint(expr.fieldIndex)
            var llvm_mask : LLVMOpaqueValue? = null
            var llvm_zero : LLVMOpaqueValue? = null
            if (value_t.baseType == Type.tBitfield) {
                llvm_mask = types.ConstI32(uint64(mask))
                llvm_zero = types.ConstI32(uint64(0))
            } elif (value_t.baseType == Type.tBitfield8) {
                llvm_mask = types.ConstI8(mask |> int8)
                llvm_zero = types.ConstI8(0 |> int8)
            } elif (value_t.baseType == Type.tBitfield16) {
                llvm_mask = types.ConstI16(mask |> int16)
                llvm_zero = types.ConstI16(0 |> int16)
            } elif (value_t.baseType == Type.tBitfield64) {
                llvm_mask = types.ConstI64(uint64(mask))
                llvm_zero = types.ConstI64(uint64(0))
            }
            let mand = LLVMBuildAnd(g_builder, getE(expr.value), llvm_mask, "")
            let if_not_zero = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, mand, llvm_zero, "")
            var res_type = type_to_llvm_type(expr._type)
            var res = build_select(if_not_zero, res_type) <| $() {
                return LLVMConstInt(types.t_int1, uint64(1), 0)
            }
            lpipe <| $() {
                return LLVMConstInt(types.t_int1, uint64(0), 0)
            }
            field_ptr = res
        } else {
            var field_offset = 0U
            var field_type : LLVMOpaqueType?
            var field_name : string;
            var val_ptr = getE(expr.value)
            if (expr.value._type.isPointer && !expr.derefFlags.unsafeDeref) {
                check_ptr_zero(val_ptr, expr.at, "dereferencing null pointer, {expr.value.describe()} is null")
            }
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(types.t_int8, 0u), "")
            if (expr.field == null && expr.fieldIndex == -1) {
                field_offset = expr.annotation |> get_handled_type_field_offset(string(expr.name))
                if (field_offset == -1U) {
                    setE(expr, LLVMGetUndef(type_to_llvm_type(expr._type)))
                    failed_E(expr, "field {expr.name} offset is -1")
                    return expr
                }
                field_type = get_type_pointer(expr._type);
                field_name = "{describe(expr.value._type,false,false,false)}.{expr.name}";
            } elif (expr.value._type.isTuple || (expr.value._type.isPointer && expr.value._type.firstType.isTuple)) {
                assert(expr.fieldIndex != -1)
                assume tt = expr.value._type.isPointer ? expr.value._type.firstType : expr.value._type;
                field_offset = get_tuple_field_offset(tt, expr.fieldIndex) |> uint()
                let argName = tuple_field_name(tt, expr.fieldIndex)
                field_type = get_type_pointer(tt.argTypes[expr.fieldIndex]);
                field_name = "{describe(tt,false,false,false)}.{argName}";
            } elif (expr.value._type.isVariant || (expr.value._type.isPointer && expr.value._type.firstType.isVariant)) {
                assume tv = expr.value._type.isPointer ? expr.value._type.firstType : expr.value._type;
                assert(expr.fieldIndex != -1)
                field_offset = get_variant_field_offset(tv, expr.fieldIndex) |> uint()
                let argName = variant_field_name(tv, expr.fieldIndex)
                field_type = get_type_pointer(tv.argTypes[expr.fieldIndex]);
                field_name = "{describe(tv,false,false,false)}.{argName}";
            } else {
                assert(expr.field != null)
                field_offset = expr.field.offset |> uint();
                field_type = get_type_pointer(expr.field._type)
                field_name = "{describe(expr.value._type,false,false,false)}.{expr.field.name}"
            }
            field_ptr = LLVMBuildGEP2(g_builder, types.t_int8, val_ptr, types.ConstI32(uint64(field_offset)), "")
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, field_type, field_name)
        }

        if (expr.fieldFlags.r2v) {
            assert(!expr.value._type.isBitfield) // todo: remove once sure it correct
            setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), field_ptr, expr._type.alignOf, ""))
        } else {
            setE(expr, field_ptr)
        }

        return expr
    }

// ExprSafeField
    def override visitExprSafeField(expr : smart_ptr<ExprSafeField>) : ExpressionPtr {
        // unsupported(expr, "safe field access is not supported yet")
        var fieldOffset = -1
        if (expr.annotation == null) {
            if (expr.fieldIndex != -1) {
                if (expr.value._type.firstType.isVariant) {
                    fieldOffset = expr.value._type.firstType |> get_variant_field_offset(expr.fieldIndex)
                } else {
                    fieldOffset = expr.value._type.firstType |> get_tuple_field_offset(expr.fieldIndex)
                }
            } else {
                fieldOffset = expr.field.offset
            }
            assert(fieldOffset >= 0, "field offset is somehow not there")
        } else {
            fieldOffset = int(expr.annotation |> get_handled_type_field_offset(string(expr.name)))
            if (fieldOffset == -1) {
                setE(expr, LLVMGetUndef(type_to_llvm_type(expr._type)))
                failed_E(expr, "field {expr.name} offset is -1")
                return expr
            }
        }
        if (expr.skipQQ) {
            var val_ptr = getE(expr.value)
            var cond = LLVMBuildIsNotNull(g_builder, val_ptr, "")
            var res_type = type_to_llvm_type(expr._type)
            var res = build_select(cond, res_type) <| $() {
                val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(types.t_int8, 0u), "")
                var field_ptr = LLVMBuildGEP2(g_builder, types.t_int8, val_ptr, types.ConstI32(uint64(fieldOffset)), "")
                field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, LLVMPointerType(res_type, 0u), ".{expr.name}")
                field_ptr = LLVMBuildLoadData2Aligned(g_builder, res_type, field_ptr, expr._type.alignOf, "")
                return field_ptr
            }
            lpipe <| $() {
                return LLVMConstPointerNull(res_type)
            }
            setE(expr, res)
        } else {
            var val_ptr = getE(expr.value)
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(types.t_int8, 0u), "")
            var field_ptr = LLVMBuildGEP2(g_builder, types.t_int8, val_ptr, types.ConstI32(uint64(fieldOffset)), "")
            var res_type = type_to_llvm_type(expr._type)
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, res_type, ".{expr.name}")
            var cond = LLVMBuildIsNotNull(g_builder, val_ptr, "")
            var res = LLVMBuildSelect(g_builder, cond, field_ptr, LLVMConstPointerNull(res_type), "")
            setE(expr, res)
        }
        return expr
    }

// ExprSwizzle
    def override visitExprSwizzle(expr : smart_ptr<ExprSwizzle>) : ExpressionPtr {
        var val = getE(expr.value)
        if (expr._type.flags.ref) {
            var val_ptr = LLVMBuildBitCast(g_builder, val, get_basetype_pointer(expr.value._type.vectorBaseType), "swizzle_ref")
            val_ptr = LLVMBuildGEP2(g_builder, base_type_to_llvm_type(expr.value._type.vectorBaseType),
                    val_ptr, types.ConstI32(uint64(expr.fields[0])), "")
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, get_type_pointer(expr._type), "")
            setE(expr, val_ptr)
        } else {
            var isSequencial = true
            for (i in range(1, length(expr.fields))) {
                if ((int(expr.fields[i - 1]) + 1) != int(expr.fields[i])) {
                    isSequencial = false
                    break
                }
            }
            if (expr.value._type.flags.ref && isSequencial) {
                var val_ptr = LLVMBuildBitCast(g_builder, val, get_basetype_pointer(expr.value._type.vectorBaseType), "swizzle")
                val_ptr = LLVMBuildGEP2(g_builder, base_type_to_llvm_type(expr.value._type.vectorBaseType),
                        val_ptr, types.ConstI32(uint64(expr.fields[0])), "")
                val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, get_type_pointer(expr._type), "")
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), val_ptr, expr.value._type.alignOf, "swizzle_{expr.fields}"))
            } else {
                if (expr.fields |> length == 1) {
                    let fi = expr.fields[0]
                    let res = LLVMBuildExtractElement(g_builder, val, types.ConstI32(uint64(fi)), "")
                    setE(expr, res)
                } else {
                    var res = LLVMGetUndef(type_to_llvm_type(expr._type))
                    for (i, fi in count(), expr.fields) {
                        var f = LLVMBuildExtractElement(g_builder, val, types.ConstI32(uint64(fi)), "")
                        res = LLVMBuildInsertElement(g_builder, res, f, types.ConstI32(uint64(i)), "")
                    }
                    setE(expr, res)
                }
            }
        }
        return expr
    }

// ExprIsVariant
    def override visitExprIsVariant(expr : smart_ptr<ExprIsVariant>) : ExpressionPtr {
        var field_index = find_argument_index(expr.value._type, string(expr.name))
        if (field_index == -1) {
            failed_E(expr, "variant field {expr.name} not found")
            return expr
        }
        var vptr = getE(expr.value)
        var vint = LLVMBuildLoad2(g_builder, types.t_int32, vptr, "variant_index")
        let cond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, vint, types.ConstI32(uint64(field_index)), "is_variant")
        setE(expr, cond)
        return expr
    }

// ExprAsVariant
    def check_variant_index(tidx : LLVMOpaqueValue?; at : LineInfo; vidx : int; message : string) {
        var cmpIdx = types.ConstI32(uint64(vidx))
        var check_null_ptr = append_basic_block("check_variant_index")
        var check_true = append_basic_block("check_true")
        var check_end = append_basic_block("check_end")
        LLVMBuildBr(g_builder, check_null_ptr)
        LLVMPositionBuilderAtEnd(g_builder, check_null_ptr)
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntNE, tidx, cmpIdx, "index_check")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_true)
        build_exception(message, at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_end)
    }

    def override visitExprAsVariant(expr : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        var field_index = find_argument_index(expr.value._type, string(expr.name))
        if (field_index == -1) {
            failed_E(expr, "variant field {expr.name} not found")
            return expr
        }
        var vptr = getE(expr.value)
        var vint = LLVMBuildLoad2(g_builder, types.t_int32, vptr, "variant_index")
        check_variant_index(vint, expr.at, field_index, "variant mismatch, expecting {expr.value._type.argNames[field_index]}")
        let field_offset = get_variant_field_offset(expr.value._type, field_index)
        var fptr = LLVMBuildPointerCast(g_builder, vptr, LLVMPointerType(types.t_int8, 0u), "")
        fptr = LLVMBuildGEP2(g_builder, types.t_int8, fptr, types.ConstI32(uint64(field_offset)), "")
        fptr = LLVMBuildPointerCast(g_builder, fptr, LLVMPointerType(type_to_llvm_type(expr._type), 0u), "variant_field_ptr")
        if (expr.fieldFlags.r2v) {
            setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), fptr, expr._type.alignOf, "variant_field"))
        } else {
            setE(expr, fptr)
        }
        return expr
    }

// ExprSafeAsVariant
    def override visitExprSafeAsVariant(expr : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        assume valueT = expr.value._type
        var vptr = getE(expr.value)
        var field_index = find_argument_index(valueT.isPointer ? valueT.firstType : valueT, string(expr.name))
        if (field_index == -1) {
            failed_E(expr, "variant field {expr.name} not found")
            return expr
        }
        var if_not_null = valueT.isPointer ? LLVMBuildIsNotNull(g_builder, vptr, "") : LLVMConstInt(types.t_int1, 1ul, 0)
        var resType = type_to_llvm_type(expr._type)
        var res = build_select(if_not_null, resType) <| $() {
            var vint = LLVMBuildLoad2(g_builder, types.t_int32, vptr, "variant_index")
            var fidx = types.ConstI32(uint64(field_index))
            var if_index = LLVMBuildICmp(g_builder, LLVMIntPredicate.LLVMIntEQ, vint, fidx, "index_check")
            var subRes = build_select(if_index, resType) <| $() {
                let field_offset = get_variant_field_offset(valueT.isPointer ? valueT.firstType : valueT, field_index)
                var fptr = LLVMBuildPointerCast(g_builder, vptr, LLVMPointerType(types.t_int8, 0u), "")
                fptr = LLVMBuildGEP2(g_builder, types.t_int8, fptr, types.ConstI32(uint64(field_offset)), "")
                return LLVMBuildPointerCast(g_builder, fptr, resType, "variant_field_ptr")
            }
            lpipe <| $() {
                return LLVMConstNull(resType)
            }
            return subRes
        }
        lpipe <| $() {
            return LLVMConstNull(resType)
        }
        setE(expr, res)
        return expr
    }

// ExprOp1
    def visitExprOp1_Func(var expr : smart_ptr<ExprOp1>) {
        expr.arguments |> resize(1)
        expr.arguments[0] := expr.subexpr
        make_call(expr, false)
    }

    def override visitExprOp1(var expr : smart_ptr<ExprOp1>) : ExpressionPtr {
        if (isExprOp1_Func(expr)) {
            visitExprOp1_Func(expr)
            return expr
        }
        var sexpr = getE(expr.subexpr)
        assume opType = expr.subexpr._type
        if (expr.op == "+++" || expr.op == "++") {
            var expr_t = base_type_to_llvm_type(opType.baseType)
            var sexpr_v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), sexpr, expr.subexpr._type.alignOf, "")
            var res_v : LLVMOpaqueValue?
            if (isNumeric(opType)) {
                res_v = LLVMBuildAdd(g_builder, sexpr_v, LLVMConstInt(expr_t, 1ul, 0), "")
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                res_v = LLVMBuildFAdd(g_builder, sexpr_v, LLVMConstReal(expr_t, 1.lf), "")
            } else {
                failed_E(expr, "unsupported ++ type {opType}")
            }
            LLVMBuildStoreAligned(g_builder, res_v, sexpr, uint(expr.subexpr._type.alignOf))
            let is_prefix = expr.op == "++" ? true : false;
            setE(expr, is_prefix ? res_v : sexpr_v)
        } elif (expr.op == "---" || expr.op == "--") {
            var expr_t = base_type_to_llvm_type(opType.baseType)
            var sexpr_v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), sexpr, expr.subexpr._type.alignOf, "")
            var res_v : LLVMOpaqueValue?
            if (isNumeric(opType)) {
                res_v = LLVMBuildSub(g_builder, sexpr_v, LLVMConstInt(expr_t, 1ul, 0), "")
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                res_v = LLVMBuildFSub(g_builder, sexpr_v, LLVMConstReal(expr_t, 1.lf), "")
            } else {
                failed_E(expr, "unsupported -- type {opType}")
            }
            LLVMBuildStoreAligned(g_builder, res_v, sexpr, uint(expr.subexpr._type.alignOf))
            let is_prefix = expr.op == "--" ? true : false;
            setE(expr, is_prefix ? res_v : sexpr_v)
        } elif (expr.op == "~") {
            if (isBinary(opType)) {
                setE(expr, LLVMBuildNot(g_builder, sexpr, ""))
            } else {
                failed_E(expr, "unsupported ~ type {opType}")
            }
        } elif (expr.op == "!") {
            if (opType.isBool) {
                setE(expr, LLVMBuildNot(g_builder, sexpr, ""))
            } else {
                failed_E(expr, "unsupported ! type {opType}")
            }
        } elif (expr.op == "-") {
            if (isNumeric(opType)) {
                setE(expr, LLVMBuildNeg(g_builder, sexpr, ""))
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, LLVMBuildFNeg(g_builder, sexpr, ""))
            } else {
                failed_E(expr, "unsupported - type {opType}")
            }
        } elif (expr.op == "+") {
            if (isNumeric(opType)) {
                setE(expr, sexpr)
            } elif (opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType == Type.tFloat)) {
                setE(expr, sexpr)
            } else {
                failed_E(expr, "unsupported + type {opType}")
            }
        } else {
            failed_E(expr, "unsupported unary operator {expr.op}")
        }
        return expr
    }

// ExprReturn
    def pre_visit_expr_return(expr : smart_ptr<ExprReturn>; isCmres : bool) {
        if (isCmres && expr.returnFlags.returnCallCMRES) {
            make_call_to_cmres(expr.subexpr, get_cmres_param())
        }
    }

    def override preVisitExprReturn(expr : smart_ptr<ExprReturn>) {
        if (thisBlock != null) {
            pre_visit_expr_return(expr, isCMRESType(thisBlock.returnType))
        } elif (thisFunc != null) {
            pre_visit_expr_return(expr, isCMRES(thisFunc))
        }
    }

    def build_enter(at : LineInfo) {
        if (LLVM_DEBUG_TRACES) {
            build_debug_trace(at, FN_JIT_DEBUG_ENTER)
        }
    }

    def build_exit(at : LineInfo) {
        if (LLVM_DEBUG_TRACES) {
            build_debug_trace(at, FN_JIT_DEBUG_EXIT)
        }
    }

    def build_debug_trace(at : LineInfo; func_name : string) {
        var message : string
        if (thisFunc != null) {
            message = thisFunc |> get_mangled_name
        } elif (thisBlock != null) {
            message = "$block"
        }
        var params = fixed_array(
            get_string_constant_ptr(g_builder, message),   // message
            get_context_param(),                           // Context
            get_line_info_ptr(at)
        )
        var typ = g_fn_types[func_name]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, func_name), params, "")
    }

    def visit_expr_return(expr : smart_ptr<ExprReturn>; returnType : TypeDeclPtr; isCmres : bool) {
        var expr_res : LLVMOpaqueValue?
        if (!returnType.isVoid) {
            if (isCmres) {
                pass
            } elif (returnType.isPointer && (expr.subexpr is ExprConstPtr)) {// return null
                pass
            } elif (returnType.isPointer) {
                var resPtr = getE(expr.subexpr)
                if (expr.subexpr._type.flags.ref && !returnType.flags.ref) {
                    // if source is ptr&, we empty it
                    resPtr = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(returnType), resPtr, expr.subexpr._type.alignOf, "result_before_move")
                    if (expr.returnFlags.moveSemantics) {
                        var subPtrType = LLVMPointerType(type_to_llvm_type(expr.subexpr._type.firstType), 0u)
                        LLVMBuildStore(g_builder, LLVMConstNull(subPtrType), getE(expr.subexpr))
                    }
                }
                // cast pointer type, becasue we may need to cast void* onto returntype*.
                // TODO: check? (we don't check, we just cast for now)
                var cres = LLVMBuildPointerCast(g_builder, resPtr, type_to_llvm_abi_type(returnType), "")
                expr_res = cres
            } elif (expr.subexpr._type.flags.ref && expr.returnFlags.moveSemantics && expr.subexpr._type.isWorkhorseType) {
                // return <- blah // where blah is string and such
                var lres = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), getE(expr.subexpr), expr.subexpr._type.alignOf, "")
                expr_res = lres
            } else {
                expr_res = getE(expr.subexpr)
            }
        }

        var any_afb =  has_any_finally_block(false)
        if (isCmres && any_afb) {
            var temp_res : LLVMOpaqueValue?
            var subPtrType = LLVMPointerType(type_to_llvm_type(returnType), 0u)
            at_function_entry <| $() {
                temp_res = LLVMBuildAlloca(g_builder, subPtrType, "return_cmres_temp_")
                LLVMBuildStore(g_builder, LLVMConstNull(subPtrType), temp_res)
            }
            LLVMBuildStore(g_builder, getE(expr.subexpr), temp_res)
            setE(expr.subexpr, temp_res)
        }

        var has_afb : bool
        if (expr_res != null) {
            has_afb = any_afb
            if (has_afb) {
                var temp_res : LLVMOpaqueValue?
                at_function_entry <| $() {
                    temp_res = LLVMBuildAlloca(g_builder, type_to_llvm_abi_type(returnType), "return_temp_")
                }
                LLVMBuildStore(g_builder, expr_res, temp_res)
                expr_res = temp_res
            }
        }

        // Make move (and copy), before finalize.
        // So finalize won't have access to returned value.
        if (isCmres) {
            assert(!returnType.isVoid)
            if (!expr.returnFlags.returnCMRES) {
                if (any_afb) {
                    var subPtrType = LLVMPointerType(type_to_llvm_type(returnType), 0u)
                    var temp_load = LLVMBuildLoad2(g_builder, subPtrType, getE(expr.subexpr), "return_cmres_temp_load_")
                    setE(expr.subexpr, temp_load)
                }
                if (expr.returnFlags.moveSemantics) {
                    build_move(get_cmres_param(), expr.subexpr, true)
                } else {
                    build_copy(get_cmres_param(), expr.subexpr)
                }
            }
        }

        call_all_finally_blocks("return", false)
        build_exit(expr.at)

        if (has_afb) {
            expr_res = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_abi_type(returnType), expr_res, returnType.alignOf, "return_temp_load_")
        }

        build_epilogue()

        if (returnType.isVoid || isCmres) {
            LLVMBuildRetVoid(g_builder)
        } else {
            if (returnType.isPointer && (expr.subexpr is ExprConstPtr)) {// return null
                LLVMBuildRet(g_builder, LLVMConstPointerNull(type_to_llvm_abi_type(returnType)))
            } elif (returnType.isPointer) {
                LLVMBuildRet(g_builder, expr_res)
            } elif (expr.returnFlags.moveSemantics && expr.subexpr._type.isWorkhorseType) {
                LLVMBuildRet(g_builder, expr_res)
            } else {
                LLVMBuildRet(g_builder, expr_res)
            }
        }
    }

    def override visitExprReturn(expr : smart_ptr<ExprReturn>) : ExpressionPtr {
        if (thisBlock != null) {
            visit_expr_return(expr, thisBlock.returnType, isCMRESType(thisBlock.returnType))
        } elif (thisFunc != null) {
            visit_expr_return(expr, thisFunc.result, isCMRES(thisFunc))
        }
        return expr
    }

// ExprBreak
    def override preVisitExprBreak(expr : smart_ptr<ExprBreak>) : void {
        call_all_finally_blocks("break", true)
        if (callBlock != null) {
            LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(types.t_int8, 0u)), callBlock)
        }
        var lblk = loop_stack |> back()
        LLVMBuildBr(g_builder, lblk.loop_end)
    }

// ExprContinue
    def override preVisitExprContinue(expr : smart_ptr<ExprContinue>) : void {
        call_all_finally_blocks("continue", true)
        if (callBlock != null) {
            LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(types.t_int8, 0u)), callBlock)
        }
        var lblk = loop_stack |> back()
        LLVMBuildBr(g_builder, lblk.loop_continue != null ? lblk.loop_continue : lblk.loop_start)
    }

// ExprMakeBlock
    def pre_make_block(var expr : smart_ptr<ExprBlock>; func : Function?; captureType : LLVMOpaqueType?) : DllName {
        thisFunc = func
        thisBlock = get_ptr(expr)
        var fnmna = uid.get_block_name(expr)
        let isCmres = isCMRESType(expr._type)
        build_llvm_function_pair(fnmna, isCmres, thisBlock.arguments, thisBlock.returnType, captureType, expr.annotations)
        // function pair
        ffunc = LLVMGetNamedFunction(g_mod, fnmna.impl())
        wfunc = LLVMGetNamedFunction(g_mod, fnmna.publ())
        // build wrapper function
        build_wrapper_function(fnmna, thisBlock.arguments, thisBlock.returnType, isCmres, captureType, thisBlock.at)
        build_function_entry(expr.at, hash(fnmna),
            thisBlock.blockFlags.hasMakeBlock || emit_prologue,
            thisFunc.totalStackSize, thisBlock.arguments)
        process_function_hints(thisBlock.annotations, thisBlock.arguments)
        process_labels(expr)
        process_finally(expr)
        return <- fnmna
    }

    def post_make_block(var blk : smart_ptr<ExprBlock>) {
        add_default_terminator(blk)
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        LLVMBuildBr(g_builder, function_body)
        thisBlock = null
        thisFunc = null
    }

    def make_block_function(expr : smart_ptr<ExprMakeBlock>; captured : array<CapturedVariable>; first_field_index : int; captureType : LLVMOpaqueType?) : tuple<func : LLVMOpaqueValue?; impl : LLVMOpaqueValue?> {
        var astVisitor = new LlvmJitVisitor(jit_context, types, uid, flags, g_di_builder, attributes)
        unsafe {
            astVisitor.adapter <- make_visitor(*astVisitor)
        }
        let fmna = (*astVisitor)->pre_make_block(unsafe(reinterpret<smart_ptr<ExprBlock>> expr._block), thisFunc, captureType)
        var cblk = (*astVisitor)->before_function_entry()
        var carg = (*astVisitor)->get_capture_param()
        carg = LLVMBuildLoad2(astVisitor.g_builder, captureType, carg, "capture")
        for (c, ai in captured, count()) {
            var arg_i = LLVMBuildExtractValue(astVisitor.g_builder, carg, uint(ai + first_field_index), "var_{c.variable.name}_index_{ai}_stindex_{ai+first_field_index}")
            if (c.eref) {
                var arg_t = LLVMPointerType(type_to_llvm_type(c.variable._type), 0u)
                var arg_p = LLVMBuildAlloca(astVisitor.g_builder, arg_t, "var_{c.variable.name}_index_{ai}_stindex_{ai+first_field_index}_var")
                LLVMBuildStore(astVisitor.g_builder, arg_i, arg_p)
                (*astVisitor)->setV_ptr(c.variable, arg_p)
            } else {
                (*astVisitor)->setV_ptr(c.variable, arg_i)
            }
        }
        (*astVisitor)->after_function_entry(cblk)
        visit(expr._block, astVisitor.adapter)
        (*astVisitor)->post_make_block(unsafe(reinterpret<smart_ptr<ExprBlock>> expr._block))
        let result = LLVMGetNamedFunction(g_mod, fmna.publ())
        let impl = LLVMGetNamedFunction(g_mod, fmna.impl())
        astVisitor.adapter := null
        unsafe {
            delete astVisitor
        }
        return (result, impl)
    }

    def override canVisitMakeBlockBody(expr : smart_ptr<ExprMakeBlock>) : bool {
        return false
    }

    def override preVisitExprMakeBlock(expr : smart_ptr<ExprMakeBlock>) : void {
        var captured <- capture_block(expr._block)
        sort(captured) <| $(a, b) => string(a.variable.name) < string(b.variable.name)
        // build block type
        var blk_fields <- build_block_type()
        blk_fields |> push(LLVMArrayType(types.LLVMFloat4Type(), 10u))
        var first_field_index = length(blk_fields)
        for (c in captured) {
            blk_fields |> push(LLVMPointerType(type_to_llvm_type(c.variable._type), 0u))
        }
        var this_block_type = StructType(types, blk_fields)
        // allocate block, set captured fields
        var blk : LLVMOpaqueValue?
        at_function_entry <| $() {
            blk = LLVMBuildAlloca(g_builder, this_block_type, "block_at_{int(expr.at.line)}_")
        }
        var blkv = LLVMGetUndef(this_block_type)
        for (c, idx in captured, count()) {
            var v_ptr = getV_ptr(c.variable, false)
            if (v_ptr == null) {
                v_ptr = getE_ptr(c.expression, false)
            }
            // TODO:
            //  recapture block arguments
            if (v_ptr == null) {
                if (thisBlock != null) {
                    // its a function argument
                    for (a in thisBlock.arguments) {
                        if (a == c.variable) {
                            v_ptr = LLVMGetParam(ffunc, uint(c.expression.argumentIndex))
                            break
                        }
                    }
                } elif (thisFunc != null) {
                    // its a function argument
                    for (a in thisFunc.arguments) {
                        if (a == c.variable) {
                            v_ptr = LLVMGetParam(ffunc, uint(c.expression.argumentIndex))
                            break
                        }
                    }
                }
            } else {
                /*
                    var a : int& = ...
                    invoke <| a // we capture ref as ref, unless its argument. then ref is ref already
                */
                if (c.variable._type.flags.ref && !c.expression.varFlags.argument && !c.expression.varFlags._block) {
                    v_ptr = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_abi_type(c.variable._type), v_ptr, c.variable._type.alignOf, "{c.variable.name}_ref")
                    c.eref = true
                }
            }

            if (v_ptr == null) {
                failed_E(expr, "capture {c.variable.name} not found")
                return
            }
            blkv = LLVMBuildInsertValue(g_builder, blkv, v_ptr, uint(idx + first_field_index), "capture_{c.variable.name}")
        }
        LLVMBuildStore(g_builder, blkv, blk)

        // make block function
        var blk_make = make_block_function(expr, captured, first_field_index, this_block_type)

        // call jit_make_block
        var blk_ptr = LLVMBuildPointerCast(g_builder, blk, LLVMPointerType(g_t_block, 0u), "")
        let argStackTop = (expr._block as ExprBlock).stackTop
        let annotationData = (expr._block as ExprBlock).annotationData |> uint64()
        var ann_data_ptr : LLVMOpaqueValue? = null
        if (annotationData != 0 |> uint64()) {
            ann_data_ptr = LLVMBuildPtrToInt(g_builder, save_address_global(uid.get_block_ann(expr), unsafe(reinterpret<void?>(annotationData))), types.LLVMIntPtrType(), "")
        } else {
            // Just an optimization.
            ann_data_ptr = LLVMConstInt(types.t_int64, 0 |> uint64(), 0)
        }
        let null_ptr = LLVMConstPointerNull(types.LLVMVoidPtrType())
        let func_ptr = LLVMBuildPointerCast(g_builder, blk_make.func, types.LLVMVoidPtrType(), "")
        let impl_ptr = LLVMBuildPointerCast(g_builder, blk_make.impl, types.LLVMVoidPtrType(), "")
        var params = fixed_array(
            blk_ptr,                                                    // blk
            types.ConstI32(uint64(argStackTop)),                        // argStackTop
            ann_data_ptr,                                               // annotation data
            func_ptr,                                                   // bodyNode
            impl_ptr,                                                   // jitImpl
            null_ptr,                                                   // TODO: funcInfo
            get_line_info_ptr(expr.at),                                 // LineInfo
            get_context_param()                                         // Context
        )
        var typ = g_fn_types[FN_JIT_MAKE_BLOCK]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_MAKE_BLOCK), params, "")
        setE(expr, blk_ptr)
    }

// ExprMemZero
    def override visitExprMemZero(expr : smart_ptr<ExprMemZero>) : ExpressionPtr {
        assume subexpr = expr.arguments[0]
        var ptr = getE(subexpr)
        ptr = LLVMBuildPointerCast(g_builder, ptr, LLVMPointerType(types.t_int8, 0u), "")
        assume sizeOf = subexpr._type.sizeOf
        assume alignOf = subexpr._type.alignOf
        LLVMBuildMemSet(g_builder, ptr, 0ul, uint64(sizeOf), uint(alignOf))
        return expr
    }

// ExprTypeDecl
    def override visitExprTypeDecl(expr : smart_ptr<ExprTypeDecl>) : ExpressionPtr {
        // replace with undef once ExprTypeDecl usage forbidden in runtime
        setE(expr, LLVMConstNull(type_to_llvm_abi_type(expr._type)))
        return expr
    }

// ExprConstPtr
    def override visitExprConstPtr(expr : smart_ptr<ExprConstPtr>) : ExpressionPtr {
        setE(expr, LLVMConstPointerNull(types.LLVMVoidPtrType()))
        return expr
    }

// ExprConstEnumeration
    def override visitExprConstEnumeration(expr : smart_ptr<ExprConstEnumeration>) : ExpressionPtr {
        var value = find_enum_value(unsafe(reinterpret<EnumerationPtr> expr._type.enumType), string(expr.value))
        setE(expr, LLVMConstInt(base_type_to_llvm_type(expr._type.enumType.baseType), uint64(value), 0))
        return expr
    }

// ExprConstBitfield
    def override visitExprConstBitfield(expr : smart_ptr<ExprConstBitfield>) : ExpressionPtr {
        setE(expr, LLVMConstInt(base_type_to_llvm_type(expr._type.baseType), uint64(expr.value), 0))
        return expr
    }

// ExprConstInt8
    def override visitExprConstInt8(expr : smart_ptr<ExprConstInt8>) : ExpressionPtr {
        setE(expr, types.ConstI8(expr.value))
        setE(expr, LLVMConstInt(types.t_int8, uint64(expr.value), 0))
        return expr
    }

// ExprConstInt16
    def override visitExprConstInt16(expr : smart_ptr<ExprConstInt16>) : ExpressionPtr {
        setE(expr, types.ConstI16(expr.value))
        return expr
    }

// ExprConstInt64
    def override visitExprConstInt64(expr : smart_ptr<ExprConstInt64>) : ExpressionPtr {
        setE(expr, types.ConstI64(expr.value |> uint64()))
        return expr
    }

// ExprConstInt
    def override visitExprConstInt(expr : smart_ptr<ExprConstInt>) : ExpressionPtr {
        setE(expr, types.ConstI32(expr.value |> uint64()))
        return expr
    }

// ExprConstInt2
    def override visitExprConstInt2(expr : smart_ptr<ExprConstInt2>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        setE(expr, LLVMBuildInt2_xy(g_builder, types, x, y, "int2"))
        return expr
    }

// ExprConstInt3
    def override visitExprConstInt3(expr : smart_ptr<ExprConstInt3>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        var z = types.ConstI32(uint64(expr.value.z))
        setE(expr, LLVMBuildInt3_xyz(g_builder, types, x, y, z, "int3"))
        return expr
    }

// ExprConstInt4
    def override visitExprConstInt4(expr : smart_ptr<ExprConstInt4>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        var z = types.ConstI32(uint64(expr.value.z))
        var w = types.ConstI32(uint64(expr.value.w))
        setE(expr, LLVMBuildInt4_xyzw(g_builder, types, x, y, z, w, "int4"))
        return expr
    }

// ExprConstUInt8
    def override visitExprConstUInt8(expr : smart_ptr<ExprConstUInt8>) : ExpressionPtr {
        setE(expr, LLVMConstInt(types.t_int8, uint64(expr.value), 0))
        return expr
    }

// ExprConstUInt16
    def override visitExprConstUInt16(expr : smart_ptr<ExprConstUInt16>) : ExpressionPtr {
        setE(expr, LLVMConstInt(types.t_int16, uint64(expr.value), 0))
        return expr
    }

// ExprConstUInt64
    def override visitExprConstUInt64(expr : smart_ptr<ExprConstUInt64>) : ExpressionPtr {
        setE(expr, types.ConstI64(expr.value |> uint64()))
        return expr
    }

// ExprConstUInt
    def override visitExprConstUInt(expr : smart_ptr<ExprConstUInt>) : ExpressionPtr {
        setE(expr, types.ConstI32(expr.value |> uint64()))
        return expr
    }

// ExprConstUInt2
    def override visitExprConstUInt2(expr : smart_ptr<ExprConstUInt2>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        setE(expr, LLVMBuildInt2_xy(g_builder, types, x, y, "uint2"))
        return expr
    }

// ExprConstUInt3
    def override visitExprConstUInt3(expr : smart_ptr<ExprConstUInt3>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        var z = types.ConstI32(uint64(expr.value.z))
        setE(expr, LLVMBuildInt3_xyz(g_builder, types, x, y, z, "uint3"))
        return expr
    }

// ExprConstUInt4
    def override visitExprConstUInt4(expr : smart_ptr<ExprConstUInt4>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        var z = types.ConstI32(uint64(expr.value.z))
        var w = types.ConstI32(uint64(expr.value.w))
        setE(expr, LLVMBuildInt4_xyzw(g_builder, types, x, y, z, w, "uint4"))
        return expr
    }

// ExprConstRange
    def override visitExprConstRange(expr : smart_ptr<ExprConstRange>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        setE(expr, LLVMBuildInt2_xy(g_builder, types, x, y, "range"))
        return expr
    }

// ExprConstURange
    def override visitExprConstURange(expr : smart_ptr<ExprConstURange>) : ExpressionPtr {
        var x = types.ConstI32(uint64(expr.value.x))
        var y = types.ConstI32(uint64(expr.value.y))
        setE(expr, LLVMBuildInt2_xy(g_builder, types, x, y, "urange"))
        return expr
    }

// ExprConstRange64
    def override visitExprConstRange64(expr : smart_ptr<ExprConstRange64>) : ExpressionPtr {
        var x = types.ConstI64(uint64(expr.value.x))
        var y = types.ConstI64(uint64(expr.value.y))
        setE(expr, LLVMBuildAnyInt2_xy(g_builder, types, types.LLVMRange64Type(), x, y, "range64"))
        return expr
    }

// ExprConstURange64
    def override visitExprConstURange64(expr : smart_ptr<ExprConstURange64>) : ExpressionPtr {
        var x = types.ConstI64(expr.value.x |> uint64())
        var y = types.ConstI64(expr.value.y |> uint64())
        setE(expr, LLVMBuildAnyInt2_xy(g_builder, types, types.LLVMRange64Type(), x, y, "urange64"))
        return expr
    }

// ExprConstBool
    def override visitExprConstBool(expr : smart_ptr<ExprConstBool>) : ExpressionPtr {
        setE(expr, LLVMConstInt(types.t_int1, expr.value ? 1ul : 0ul, 0))
        return expr
    }

// ExprConstFloat
    def override visitExprConstFloat(expr : smart_ptr<ExprConstFloat>) : ExpressionPtr {
        setE(expr, types.ConstReal(expr.value |> double()))
        return expr
    }

// ExprConstFloat2
    def override visitExprConstFloat2(expr : smart_ptr<ExprConstFloat2>) : ExpressionPtr {
        var x = types.ConstReal(expr.value.x |> double())
        var y = types.ConstReal(expr.value.y |> double())
        setE(expr, LLVMBuildFloat2_xy(g_builder, types, x, y, "float2"))
        return expr
    }

// ExprConstFloat3
    def override visitExprConstFloat3(expr : smart_ptr<ExprConstFloat3>) : ExpressionPtr {
        var x = types.ConstReal(expr.value.x |> double())
        var y = types.ConstReal(expr.value.y |> double())
        var z = types.ConstReal(expr.value.z |> double())
        setE(expr, LLVMBuildFloat3_xyz(g_builder, types, x, y, z, "float3"))
        return expr
    }

// ExprConstFloat4
    def override visitExprConstFloat4(expr : smart_ptr<ExprConstFloat4>) : ExpressionPtr {
        var x = types.ConstReal(expr.value.x |> double())
        var y = types.ConstReal(expr.value.y |> double())
        var z = types.ConstReal(expr.value.z |> double())
        var w = types.ConstReal(expr.value.w |> double())
        setE(expr, LLVMBuildFloat4_xyzw(g_builder, types, x, y, z, w, "float4"))
        return expr
    }

// ExprConstString
    def override visitExprConstString(expr : smart_ptr<ExprConstString>) : ExpressionPtr {
        setE(expr, get_string_constant_ptr(g_builder, string(expr.value)))
        return expr
    }

// ExprConstDouble
    def override visitExprConstDouble(expr : smart_ptr<ExprConstDouble>) : ExpressionPtr {
        setE(expr, types.ConstDouble(expr.value))
        return expr
    }

// ExprFakeContext
    def override visitExprFakeContext(expr : smart_ptr<ExprFakeContext>) : ExpressionPtr {
        setE(expr, LLVMGetParam(ffunc, 0u))
        return expr
    }

// ExprFakeLineInfo
    def override visitExprFakeLineInfo(expr : smart_ptr<ExprFakeLineInfo>) : ExpressionPtr {
        setE(expr, get_line_info_ptr(expr.at))
        return expr
    }

    def override visitExprTypeInfo(expr : smart_ptr<ExprTypeInfo>) : ExpressionPtr {
        if (expr.trait == "ast_typedecl") {
            let mangled_name = expr.typeexpr |> get_mangled_name()
            let hash_value = hash(mangled_name)
            var params = fixed_array(
                types.ConstI64(hash_value |> uint64()), // hash
                get_context_param(),                    // context
                get_line_info_ptr(expr.at)              // lineinfo
            )
            var typ = g_fn_types[FN_JIT_AST_TYPEDECL]
            var cll = LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_AST_TYPEDECL), params, "")
            setE(expr, cll)
        } elif (expr.trait == "rtti_classinfo" || expr.trait == "rtti_typeinfo") {
            var tptr = create_type_info_global(make_type_info_structure(*jit_context, expr.typeexpr))
            setE(expr, tptr)
        } else {
            failed_E(expr, "typeinfo({expr.trait}) is not supported yet")
        }
        return expr
    }

///////
// misc
///////

    def cast_from_vec4f_ptr(v4f : LLVMOpaqueValue?; ptr_type : LLVMOpaqueType?) : LLVMOpaqueValue? {
        var tres : LLVMOpaqueValue?
        if (typeinfo sizeof(type<void?>) == 8) {
            var v2 = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(types.t_int64, 2u), "")
            var tbit = LLVMBuildExtractElement(g_builder, v2, types.ConstI32(0ul), "")
            tres = LLVMBuildIntToPtr(g_builder, tbit, ptr_type, "vec4f_to_ptr")
        } else {
            var tbit = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            tres = LLVMBuildBitCast(g_builder, tbit, types.t_int32, "")
            tres = LLVMBuildIntToPtr(g_builder, tres, ptr_type, "vec4f_to_ptr")
        }
        return tres
    }

    def cast_from_vec4f_workhorse(etype : TypeDeclPtr; expr : ExpressionPtr; v4f : LLVMOpaqueValue?) {
        let bt = etype.baseType
        if (bt == Type.tInt || bt == Type.tUInt || bt == Type.tBitfield || bt == Type.tEnumeration) {
            var tbit = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            return LLVMBuildBitCast(g_builder, tbit, types.t_int32, "vec4f_to_int")
        } elif (bt == Type.tInt64 || bt == Type.tUInt64 || bt == Type.tEnumeration64 || bt == Type.tBitfield64) {
            var tvec = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(types.t_int64, 2u), "")
            var tbit = LLVMBuildExtractElement(g_builder, tvec, types.ConstI32(0ul), "")
            return tbit
        } elif (bt == Type.tInt8 || bt == Type.tUInt8 || bt == Type.tEnumeration8 || bt == Type.tBitfield8) {
            var tbit = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var tres = LLVMBuildBitCast(g_builder, tbit, types.t_int32, "vec4f_to_int")
            return LLVMBuildTruncOrBitCast(g_builder, tres, types.t_int8, "int_to_int8")
        } elif (bt == Type.tInt16 || bt == Type.tUInt16 || bt == Type.tEnumeration16 || bt == Type.tBitfield16) {
            var tbit = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var tres = LLVMBuildBitCast(g_builder, tbit, types.t_int32, "vec4f_to_int")
            return LLVMBuildTruncOrBitCast(g_builder, tres, types.t_int16, "int_to_int16")
        } elif (bt == Type.tFloat) {
            return LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "vec4f_to_float")
        } elif (bt == Type.tDouble) {
            var tvec = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(types.t_double, 2u), "")
            return LLVMBuildExtractElement(g_builder, tvec, types.ConstI32(0ul), "vec4f_to_double")
        } elif (bt == Type.tFloat2) {
            var vec = LLVMGetUndef(types.LLVMFloat2Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(1ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, types.ConstI32(0ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, types.ConstI32(1ul), "vec4f_to_float2")
            return vec
        } elif (bt == Type.tFloat3) {
            var vec = LLVMGetUndef(types.LLVMFloat3Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(1ul), "")
            var z = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(2ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, types.ConstI32(0ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, types.ConstI32(1ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, z, types.ConstI32(2ul), "vec4f_to_float3")
            return vec
        } elif (bt == Type.tFloat4 || bt == Type.anyArgument) {
            return v4f
        } elif (bt == Type.tInt2 || bt == Type.tUInt2 || bt == Type.tRange || bt == Type.tURange) {
            var vec = LLVMGetUndef(types.LLVMFloat2Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(1ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, types.ConstI32(0ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, types.ConstI32(1ul), "")
            return LLVMBuildBitCast(g_builder, vec, types.LLVMInt2Type(), "vec4f_to_int2")
        } elif (bt == Type.tInt3 || bt == Type.tUInt3) {
            var vec = LLVMGetUndef(types.LLVMFloat3Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(1ul), "")
            var z = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(2ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, types.ConstI32(0ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, types.ConstI32(1ul), "")
            vec = LLVMBuildInsertElement(g_builder, vec, z, types.ConstI32(2ul), "vec4f_to_float3")
            return LLVMBuildBitCast(g_builder, vec, types.LLVMInt3Type(), "vec4f_to_int2")
        } elif (bt == Type.tInt4 || bt == Type.tUInt4) {
            return LLVMBuildBitCast(g_builder, v4f, types.LLVMInt4Type(), "vec4f_to_int4")
        } elif (bt == Type.tBool) {
            var tbit = LLVMBuildExtractElement(g_builder, v4f, types.ConstI32(0ul), "")
            var tres = LLVMBuildBitCast(g_builder, tbit, types.t_int32, "")
            return LLVMBuildTruncOrBitCast(g_builder, tres, types.t_int1, "vec4f_to_bool")
        } elif (bt == Type.tString || bt == Type.tPointer) {
            var ptr_type = bt == Type.tString ? types.get_type_string() : type_to_llvm_type(etype)
            return cast_from_vec4f_ptr(v4f, ptr_type)
        } elif (bt == Type.tRange64 || bt == Type.tURange64) {
            return LLVMBuildBitCast(g_builder, v4f, types.LLVMRange64Type(), "vec4f_to_range64")
        } else {
            failed_E(expr, "unsupported cast_from_vec4f cast {describe(etype)} workhorse")
        }
        return null
    }

    def cast_from_vec4f(expr : ExpressionPtr; etype : TypeDeclPtr; v4f : LLVMOpaqueValue?) {
        var tres : LLVMOpaqueValue?
        if (etype.baseType == Type.anyArgument) {
            tres = v4f
        } elif (etype.isRef) {
            var ptr_type = LLVMPointerType(type_to_llvm_type(etype), 0u)
            tres = cast_from_vec4f_ptr(v4f, ptr_type)
        } elif (etype.isWorkhorseType) {
            tres = cast_from_vec4f_workhorse(etype, expr, v4f)
        } elif (etype.isHandle) {
            var inscope tp <- get_underlying_value_type(etype)
            if (tp == null) {
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(etype)} {describe(expr.at)}");
            } elif (!tp.isWorkhorseType && tp.baseType != Type.anyArgument) {
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(etype)} {describe(expr.at)}")
            }
            tres = cast_from_vec4f_workhorse(tp, expr, v4f)
        } elif (etype.isPointer) {
            var ptr_type = type_to_llvm_type(etype)
            tres = cast_from_vec4f_ptr(v4f, ptr_type)
        } elif (etype.baseType == Type.tFunction) {
            let int_vector = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(types.t_int64, 2u), "vec_to_int_vector")
            let func_bits = LLVMBuildExtractElement(g_builder, int_vector, types.ConstI64(0ul), "func_bits")
            tres = LLVMBuildIntToPtr(g_builder, func_bits, LLVMPointerType(types.t_int8, 0u), "function_ptr")
        } elif (etype.baseType == Type.tLambda) {
            tres = cast_from_vec4f_ptr(v4f, types.LLVMVoidPtrType())
        } else {
            failed_E(expr, "unsupported cast_from_vec4f type {describe(etype)}")
        }
        if (tres == null) {
            tres = LLVMGetUndef(types.LLVMFloat4Type())
        }
        return tres
    }

    def cast_ptr_to_vec4f(tsrc : LLVMOpaqueValue?) {
        static_if (typeinfo sizeof(type<void?>) == 8) {
            var vvec = LLVMGetUndef(LLVMVectorType(types.t_int64, 2u))
            var tval = LLVMBuildPtrToInt(g_builder, tsrc, types.t_int64, "")
            var vsrc = LLVMBuildInsertElement(g_builder, vvec, tval, types.ConstI32(0ul), "")
            return LLVMBuildBitCast(g_builder, vsrc, types.LLVMFloat4Type(), "ptr_to_vec4f")
        } else {
            var tval = LLVMBuildPtrToInt(g_builder, tsrc, types.t_int32, "")
            var tbit = LLVMBuildBitCast(g_builder, tval, types.t_float, "")
            return LLVMBuildInsertElement(g_builder, vres, tbit, types.ConstI32(0ul), "ptr_to_vec4f")
        }
    }

    def cast_to_vec4f(etype : TypeDeclPtr; tsrc : LLVMOpaqueValue?) {
        var vres = LLVMGetUndef(types.LLVMFloat4Type())
        var tres : LLVMOpaqueValue?
        if (etype.isRef) {
            tres = cast_ptr_to_vec4f(tsrc)
        } elif (etype.isWorkhorseType) {
            let bt = etype.baseType
            if (bt == Type.tInt || bt == Type.tUInt || bt == Type.tBitfield || bt == Type.tEnumeration) {
                var tbit = LLVMBuildBitCast(g_builder, tsrc, types.t_float, "tbit")
                tres = LLVMBuildInsertElement(g_builder, vres, tbit, types.ConstI32(0ul), "return_result")
            } elif (bt == Type.tBool || bt == Type.tInt8 || bt == Type.tInt16 ||
                                        bt == Type.tUInt8 || bt == Type.tUInt16 ||
                                        bt == Type.tEnumeration8 || bt == Type.tEnumeration16 ||
                                        bt == Type.tBitfield8 || bt == Type.tBitfield16) {
                var tibit = LLVMBuildZExtOrBitCast(g_builder, tsrc, types.t_int32, "int_bit")
                var tbit = LLVMBuildBitCast(g_builder, tibit, types.t_float, "tbit")
                tres = LLVMBuildInsertElement(g_builder, vres, tbit, types.ConstI32(0ul), "return_result")
            } elif (bt == Type.tInt64 || bt == Type.tUInt64) {
                var vvec = LLVMGetUndef(LLVMVectorType(types.t_int64, 2u))
                var vsrc = LLVMBuildInsertElement(g_builder, vvec, tsrc, types.ConstI32(0ul), "int64_vec")
                tres = LLVMBuildBitCast(g_builder, vsrc, types.LLVMFloat4Type(), "return_result")
            } elif (bt == Type.tFloat) {
                tres = LLVMBuildInsertElement(g_builder, vres, tsrc, types.ConstI32(0ul), "return_result")
            } elif (bt == Type.tDouble) {
                var vvec = LLVMGetUndef(LLVMVectorType(types.t_double, 2u))
                var vsrc = LLVMBuildInsertElement(g_builder, vvec, tsrc, types.ConstI32(0ul), "double_vec")
                tres = LLVMBuildBitCast(g_builder, vsrc, types.LLVMFloat4Type(), "return_result")
            } elif (bt == Type.tInt2 || bt == Type.tUInt2 || bt == Type.tRange || bt == Type.tURange) {
                var tbit = LLVMBuildShuffleVector(g_builder, types, tsrc, tsrc, [ 0, 1, -1, -1], "int2_vec")
                tres = LLVMBuildBitCast(g_builder, tbit, types.LLVMFloat4Type(), "return_result")
            } elif (bt == Type.tInt3 || bt == Type.tUInt3) {
                var tbit = LLVMBuildShuffleVector(g_builder, types, tsrc, tsrc, [ 0, 1, 2, -1], "int3_vec")
                tres = LLVMBuildBitCast(g_builder, tbit, types.LLVMFloat4Type(), "return_result")
            } elif (bt == Type.tInt4 || bt == Type.tUInt4 || bt == Type.tRange64 || bt == Type.tURange64) {
                tres = LLVMBuildBitCast(g_builder, tsrc, types.LLVMFloat4Type(), "return_result")
            } elif (bt == Type.tFloat2) {
                tres = LLVMBuildShuffleVector(g_builder, types, tsrc, tsrc, [ 0, 1, -1, -1], "return_result")
            } elif (bt == Type.tFloat3) {
                tres = LLVMBuildShuffleVector(g_builder, types, tsrc, tsrc, [ 0, 1, 2, -1], "return_result")
            } elif (bt == Type.tFloat4) {
                tres = tsrc
            } elif (bt == Type.tString || bt == Type.tPointer) {
                tres = cast_ptr_to_vec4f(tsrc)
            } else {
                failed("unsupported workhorse cast_to_vec4f cast {describe(etype)} aka {etype.baseType}")
            }
        } elif (etype.baseType == Type.anyArgument) {
            tres = tsrc
        } elif (etype.isFunction) {
            assert(typeinfo sizeof(type<void?>) == 8)
            var vvec = LLVMGetUndef(LLVMVectorType(types.t_int64, 2u))
            var tbits = LLVMBuildExtractValue(g_builder, tsrc, 0u, "")
            tbits = LLVMBuildPtrToInt(g_builder, tbits, types.t_int64, "")
            var vsrc = LLVMBuildInsertElement(g_builder, vvec, tbits, types.ConstI32(0ul), "function")
            tres = LLVMBuildBitCast(g_builder, vsrc, types.LLVMFloat4Type(), "return_result")
        } elif (etype.isHandle) {
            var inscope tp <- get_underlying_value_type(etype)
            if (tp == null) {
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(etype)}")
            } elif (!tp.isWorkhorseType && tp.baseType != Type.anyArgument) {
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(etype)}")
            }
            tres = cast_to_vec4f(tp, tsrc)
        } elif (etype.baseType == Type.tLambda) {
            tres = cast_ptr_to_vec4f(tsrc)
        } elif (etype.baseType == Type.tPointer) {
            tres = cast_ptr_to_vec4f(tsrc)
        } else {
            failed("unsupported cast_to_vec4f type {describe(etype)}")
        }
        return tres
    }

    def get_capture_param : LLVMOpaqueValue ? {
        if (thisBlock != null) {
            return LLVMGetParam(ffunc, uint(thisBlock.arguments |> length))
        } else {
            panic("not in block")
            return null
        }
    }

    def get_context_param : LLVMOpaqueValue ? {
        if (thisBlock != null) {
            return LLVMGetParam(ffunc, uint(thisBlock.arguments |> length + 1))
        } elif (thisFunc != null) {
            return LLVMGetParam(ffunc, uint(thisFunc.arguments |> length))
        } else {
            panic("not in block or function")
            return null
        }
    }

    def get_cmres_param : LLVMOpaqueValue ? {
        if (thisBlock != null) {
            return LLVMGetParam(ffunc, uint(thisBlock.arguments |> length + 2))
        } elif (thisFunc != null) {
            return LLVMGetParam(ffunc, uint(thisFunc.arguments |> length + 1))
        } else {
            panic("not in block or function")
            return null
        }
    }

    def build_select(cond : LLVMOpaqueValue?; typ : LLVMOpaqueType?; if_true, if_false : block<LLVMOpaqueValue?>) : LLVMOpaqueValue? {
        if (LLVMIsConstant(cond) != 0) {
            var val = LLVMConstIntGetZExtValue(cond)
            return val != 0ul ? invoke(if_true) : invoke(if_false)
        }
        var check_true = append_basic_block("check_true")
        var check_false = append_basic_block("check_false")
        var check_end = append_basic_block("check_end")
        var phi_true = append_basic_block("phi_true")
        var phi_false = append_basic_block("phi_false")
        LLVMBuildCondBr(g_builder, cond, check_true, check_false)
        LLVMPositionBuilderAtEnd(g_builder, check_true)
        var pt = invoke(if_true)
        LLVMBuildBr(g_builder, phi_true)
        LLVMPositionBuilderAtEnd(g_builder, phi_true)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_false)
        var pf = invoke(if_false)
        LLVMBuildBr(g_builder, phi_false)
        LLVMPositionBuilderAtEnd(g_builder, phi_false)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder, check_end)
        var phi = LLVMBuildPhi(g_builder, typ, "select")
        LLVMAddIncoming(phi, fixed_array(pt, pf), fixed_array(phi_true, phi_false))
        return phi
    }

    def build_exception(message : string; at : LineInfo) {
        var params = fixed_array(
            get_string_constant_ptr(g_builder, message),   // message
            get_context_param(),
            get_line_info_ptr(at)
        )
        var typ = g_fn_types[FN_JIT_EXCEPTION]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_EXCEPTION), params, "")
    }

    def at_function_entry(blk : block) {
        var current_block = LLVMGetInsertBlock(g_builder)
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        invoke(blk)
        LLVMPositionBuilderAtEnd(g_builder, current_block)
    }

    def before_function_entry {
        var current_block = LLVMGetInsertBlock(g_builder)
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        return current_block
    }

    def after_function_entry(cblk : LLVMOpaqueBasicBlock?) {
        LLVMPositionBuilderAtEnd(g_builder, cblk)
    }

    def append_label_block(blk : LLVMOpaqueBasicBlock?) {
        subblocks[blk] = blk
        return blk
    }

    def setE(e : ExpressionPtr; ov : LLVMOpaqueValue?) {
        e2v[get_ptr(e)] = ov
    }

    def unsupported(e : ExpressionPtr; message : string) {
        failed_E(e, "unsupported expression {e.__rtti}; {message}")
        setE(e, LLVMGetUndef(types.t_int32))
    }

    def getE_ptr(e : Expression?; canFail : bool = true) {
        var res : LLVMOpaqueValue?
        e2v |> get(e) <| $(val) {
            res = val
        }
        if (res == null && canFail) {
            res = LLVMGetUndef(types.t_int32)
            failed_E(e, "unresolved expression at {describe(e.at)}")
        }
        return res
    }

    def getE(e : ExpressionPtr) {
        var res : LLVMOpaqueValue?
        e2v |> get(get_ptr(e)) <| $(val) {
            res = val
        }
        if (res == null) {
            res = LLVMGetUndef(types.t_int32)
            failed_E(e, "unresolved expression {describe(e)}")
        }
        return res
    }

    def tryE(e : ExpressionPtr; blk : block<LLVMOpaqueValue?>) {
        var res : LLVMOpaqueValue?
        e2v |> get(get_ptr(e)) <| $(val) {
            res = val
        }
        if (res == null) {
            res = invoke(blk)
        }
        return res
    }

    def setV(e : VariablePtr; ov : LLVMOpaqueValue?) {
        v2v[get_ptr(e)] = ov
    }

    def setV_ptr(e : Variable?; ov : LLVMOpaqueValue?) {
        v2v[e] = ov
    }

    def getV_ptr(e : Variable?; canFail : bool = true) {
        var res : LLVMOpaqueValue?
        v2v |> get(e) <| $(val) {
            res = val
        }
        if (res == null && canFail) {
            res = LLVMGetUndef(types.t_int32)
            failed_V(e, "unresolved variable at line {int(e.at.line)}")
        }
        return res
    }

    def getV(e : VariablePtr) {
        var res : LLVMOpaqueValue?
        v2v |> get(get_ptr(e)) <| $(val) {
            res = val
        }
        if (res == null) {
            res = LLVMGetUndef(types.t_int32)
            failed_V(e, "unresolved variable {describe(e)}")
        }
        return res
    }

    def tryGetV(e : VariablePtr) {
        var res : LLVMOpaqueValue?
        v2v |> get(get_ptr(e)) <| $(val) {
            res = val
        }
        return res
    }
}


[macro_function]
def private get_llvm_function_type(func : FunctionPtr; types : PrimitiveTypes?) {
    var arg_types : array<LLVMOpaqueType?>
    var res_type : LLVMOpaqueType?
    res_type = type_to_llvm_type(func.result)
    if (func.result.flags.ref) {
        res_type = LLVMPointerType(res_type, 0u)
    }
    let cmres = isCMRES(func)
    if (cmres) {
        arg_types |> push(LLVMPointerType(res_type, 0u))
        res_type = types.t_void
    }
    for (arg in func.arguments) {
        arg_types |> push(type_to_llvm_abi_type(arg._type))
    }
    if (func.moreFlags.jitContextAndLineInfo) {
        arg_types |> push(types.LLVMVoidPtrType())  // context
        arg_types |> push(types.LLVMVoidPtrType())  // line info arg
    }
    return LLVMFunctionType(res_type, arg_types)
}

[macro_function]
def private add_ctor_dtor_function(ctor_dtor : array<tuple<LLVMOpaqueValue?, LLVMOpaqueValue?>>; types : PrimitiveTypes?) {
    let total_sz = ctor_dtor |> length() |> uint
    let priority = 0 |> uint64()
    let dataPointerType = LLVMPointerType(types.t_int8, 0u)
    let funcType = LLVMPointerType(LLVMFunctionType(types.t_void, null, 0u, 0), 0u)
    var ctorTypes = fixed_array(types.t_int32, funcType, dataPointerType)
    var dtorTypes = fixed_array(types.t_int32, funcType, dataPointerType)


    let ctorStructType = StructType(types, ctorTypes)
    let dtorStructType = StructType(types, dtorTypes)

    var ctorFields : array<LLVMOpaqueValue?>
    var dtorFields : array<LLVMOpaqueValue?>

    var ctor_entries = array<LLVMOpaqueValue?>()
    var dtor_entries = array<LLVMOpaqueValue?>()

    var i = 0
    for ((ctor, dtor) in ctor_dtor) {
        ctorFields |> push(types.ConstI32(priority))
        var c := ctor
        ctorFields |> push(c)
        ctorFields |> push(LLVMConstPointerNull(dataPointerType))
        ctor_entries |> push(LLVMConstNamedStruct(ctorStructType, unsafe(addr(ctorFields[i])), 3u))

        dtorFields |> push(types.ConstI32(priority))
        var d := dtor
        dtorFields |> push(d)
        dtorFields |> push(LLVMConstPointerNull(dataPointerType))
        dtor_entries |> push(LLVMConstNamedStruct(dtorStructType, unsafe(addr(dtorFields[i])), 3u))
        i = i + 3
    }

    var ctorStruct = LLVMConstNamedStruct(ctorStructType, array_data_ptr(ctorFields), 3u)
    var dtorStruct = LLVMConstNamedStruct(dtorStructType, array_data_ptr(dtorFields), 3u)

    let ctorArray = LLVMAddGlobal(g_mod, LLVMArrayType(ctorStructType, total_sz), "llvm.global_ctors")
    let dtorArray = LLVMAddGlobal(g_mod, LLVMArrayType(dtorStructType, total_sz), "llvm.global_dtors")

    LLVMSetLinkage(ctorArray, LLVMLinkage.LLVMAppendingLinkage)
    LLVMSetLinkage(dtorArray, LLVMLinkage.LLVMAppendingLinkage)

    LLVMSetInitializer(ctorArray, LLVMConstArray(ctorStructType, array_data_ptr(ctor_entries), total_sz))
    LLVMSetInitializer(dtorArray, LLVMConstArray(dtorStructType, array_data_ptr(dtor_entries), total_sz))
}

// We need to overwrite address-like dependencies in .dll before start.
// It's best to just avoid all such dependencies
class public ResolveExternVisitor : AstVisitor {
    dll : DLLHandle?
    types : PrimitiveTypes?
    jit_context : Context?
    @do_not_delete thisFunc : Function?
    @do_not_delete thisBlock : array<ExprBlock?>
    uid : UidNodes?

    def override preVisitFunction(var fun : FunctionPtr) : void {
        uid.reset(get_ptr(fun))
        thisFunc = get_ptr(fun)
    }

    def override visitFunction(fun : FunctionPtr) : FunctionPtr {
        thisFunc = null
        uid.reset(null)
        return fun
    }


    def ResolveExternVisitor(var _dll : DLLHandle?; var _types : PrimitiveTypes?; ctx : Context?; uids : UidNodes?) {
        dll = _dll
        types = _types
        jit_context = ctx
        uid = uids
    }

    def make_call(expr : smart_ptr<ExprCallFunc>) {
        if (expr.func == null) { return ; }
        if (has_intrinsic(expr)) {
            return
        }

        if (expr.func.flags.builtIn) {
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (FUNC_PTR == null) {
                failed_E(expr, "missing interop function pointer for {expr.func._module.name}::{expr.func.name} // {get_mangled_name(expr.func)}")
            }
            dll.set_glob_address(uid.get_dll_fn_name_ptr(expr.func), FUNC_PTR)
        } else {
            if (!expr.func.moreFlags.requestJit) {
                var MNH_ADDR = get_function_addr(jit_context, expr.func)
                dll.set_glob_address(uid.get_dll_fn_name_ptr(expr.func), MNH_ADDR)
            }
        }
    }
    def override preVisitExprNew(expr : smart_ptr<ExprNew>) {
        if (expr.typeexpr.isHandle) {
            var new_handle_ptr = get_jit_new(expr.typeexpr.annotation)
            if (new_handle_ptr == null) {
                failed_T(expr.typeexpr, "missing new`handle function pointer for {describe(expr.typeexpr)}")
                return
            }
            dll.set_glob_address(uid.get_new(expr.typeexpr), new_handle_ptr)
        } else {
            if (expr.typeexpr.dim |> length > 0) {
                if (expr.typeexpr.baseType == Type.tHandle) {
                    var inscope elemT = clone_type(expr.typeexpr)
                    elemT.dim |> clear() // hack to get correct ptr to ctor
                    var new_handle_ptr = get_jit_new(elemT.annotation)
                    if (new_handle_ptr == null) {
                        failed_T(elemT, "missing new`handle function pointer for {describe(elemT)}")
                        return
                    }
                    dll.set_glob_address(uid.get_new(expr.typeexpr), new_handle_ptr)
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

    def override preVisitExprDelete(var expr : smart_ptr<ExprDelete>) {
        assume subexpr = expr.subexpr
        assume subT = expr.subexpr._type
        if (subT.isPointer) {
            assume firstT = expr.subexpr._type.firstType
            if (firstT.isHandle) {
                var delete_handle_ptr = get_jit_delete(firstT.annotation)
                if (delete_handle_ptr == null) {
                    failed_T(firstT, "missing delete`handle function pointer for {describe(firstT)}")
                    return
                }
                dll.set_glob_address(uid.get_delete_type(firstT), delete_handle_ptr)
            }
        }
    }

    def override preVisitExprClone(expr : smart_ptr<ExprClone>) : void {
        assume leftT = expr.left._type
        if (leftT.isHandle) {
            var clone_handle_ptr = get_jit_clone(leftT.annotation)
            if (clone_handle_ptr == null) {
                failed_T(leftT, "missing clone`handle function pointer for {describe(leftT)}")
                return
            }
            dll.set_glob_address(uid.get_clone_type(leftT), clone_handle_ptr)
        }
    }

    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) {
        var MNH_ADDR = get_function_addr(jit_context, expr.func)
        dll.set_glob_address(uid.get_addr(expr), MNH_ADDR)
    }

    def override preVisitExprMakeStructIndex(expr : smart_ptr<ExprMakeStruct>; index : int; last : bool) : void {
        // one time is enough?
        if (expr.constructor != null && !expr.constructor.moreFlags.requestJit) {
            var MNH_ADDR = get_function_addr(jit_context, expr.constructor)
            dll.set_glob_address(uid.get_ctor(expr), MNH_ADDR)
        }
    }

    def override preVisitExprAscend(expr : smart_ptr<ExprAscend>) : void {
        if (expr.subexpr._type.isHandle) {
            assume firstT = expr.subexpr._type
            var new_handle_ptr = get_jit_new(firstT.annotation)
            if (new_handle_ptr == null) {
                failed_T(firstT, "missing new`handle function pointer for {describe(firstT)}")
                return
            }
            dll.set_glob_address(uid.get_ascend_new(expr), new_handle_ptr)
        }
    }

    def override preVisitExprMakeBlock(expr : smart_ptr<ExprMakeBlock>) : void {
        thisBlock |> push <| get_ptr(unsafe(reinterpret<smart_ptr<ExprBlock>> expr._block))

        let annotationData = (expr._block as ExprBlock).annotationData
        if (annotationData != 0 |> uint64()) {
            dll.set_glob_address(uid.get_block_ann(expr), unsafe(reinterpret<void?>(annotationData)))
        }
    }

    def override visitExprMakeBlock(expr : smart_ptr<ExprMakeBlock>) : ExpressionPtr {
        thisBlock |> pop()
        return expr
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

    def override preVisitExprAt(expr : smart_ptr<ExprAt>) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType) {
            let t = get_table_t(subExprT)
            dll.set_glob_address(DllName(FN_JIT_TABLE_AT(t)), get_jit_table_at(unsafe(reinterpret<int> t)))
        }
    }

    def override preVisitExprSetInsert(expr : smart_ptr<ExprSetInsert>) {
        let t = get_table_t(expr.arguments[0]._type)
        dll.set_glob_address(DllName(FN_JIT_TABLE_AT(t)), get_jit_table_at(unsafe(reinterpret<int> t)))
    }

    def override preVisitExprKeyExists(expr : smart_ptr<ExprKeyExists>) {
        let t = get_table_t(expr.arguments[0]._type)
        dll.set_glob_address(DllName(FN_JIT_TABLE_FIND(t)), get_jit_table_find(unsafe(reinterpret<int> t)))
    }

    def override preVisitExprFind(expr : smart_ptr<ExprFind>) {
        let t = get_table_t(expr.arguments[0]._type)
        dll.set_glob_address(DllName(FN_JIT_TABLE_FIND(t)), get_jit_table_find(unsafe(reinterpret<int> t)))
    }

    def override preVisitExprSafeAt(expr : smart_ptr<ExprSafeAt>) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType || (subExprT.isPointer && subExprT.firstType.isGoodTableType)) {
            var inscope etype := subExprT.isPointer ? subExprT.firstType : subExprT
            let t = get_table_t(etype)
            dll.set_glob_address(DllName(FN_JIT_TABLE_FIND(t)), get_jit_table_find(unsafe(reinterpret<int> t)))
        }
    }

    def override preVisitExprErase(expr : smart_ptr<ExprErase>) {
        let t = get_table_t(expr.arguments[0]._type)
        dll.set_glob_address(DllName(FN_JIT_TABLE_ERASE(t)), get_jit_table_erase(unsafe(reinterpret<int> t)))
    }

}

def public get_dll_missing(fns : array<FunctionPtr>; disabled : array<FunctionPtr>; var dll : DLLHandle?; var uids : UidNodes?) {
    verify(!(is_in_completion() || is_compiling_macros() || is_folding()))

    var missing = array<Function?>()
    var disabled_exists = array<Function?>()
    if (dll.handle == null) {
        for (fn in fns) {
            var ptr := fn.get_ptr()
            missing |> push_clone(ptr)
        }
    } else {
        for (fn in fns) {
            let dll_name = uids.get_dll_fn_name(fn)
            var written_hash = dll.get_function_hash(dll_name)
            // debug("dll_function_aot_hash: {dll_function_aot_hash}")
            if (written_hash == null || get_function_aot_hash(fn |> get_ptr) != *written_hash) {
                var tmp = fn.get_ptr()
                missing |> push_clone(tmp)
            }
        }
        for (fn in disabled) {
            let dll_name = uids.get_dll_fn_name(fn)
            var written_hash = dll.get_function_hash(dll_name)
            // debug("dll_function_aot_hash: {dll_function_aot_hash}")
            if (written_hash != null) {
                var tmp = fn.get_ptr()
                disabled_exists |> push_clone(tmp)
            }
        }
    }
    return <- (missing, disabled_exists)
}

// Main function. Generates LLVM IR for `FunctionPtr`.
[macro_function]
def public generate_llvm(ctx : Context?; types : PrimitiveTypes?; uids : UidNodes?;
                         attrs : Attributes?; fn : FunctionPtr; flags : LlvmJitFlags) {
    verify(!(is_in_completion() || is_compiling_macros() || is_folding()))

    let dll_name = uids.get_dll_fn_name(fn)
    if (!flags.gen_exe) {
        // No need in public symbols when generating executable
        let globalVar = LLVMAddGlobal(g_mod, types.t_int64, dll_name.id())
        set_public_linkage(globalVar)
        let aot_hash : uint64 = get_function_aot_hash(fn |> get_ptr)
        // debug("initializer hash for {dll_name.publ()}: {aot_hash}")
        globalVar |> LLVMSetInitializer(types.ConstI64(aot_hash))
    }
    var astVisitor = new LlvmJitVisitor(ctx, types, uids, flags, null, attrs)
    unsafe {
        astVisitor.adapter <- make_visitor(*astVisitor)
    }
    visit(fn, astVisitor.adapter)
    if (length(g_errors) > 0) {
        let errors = g_errors |> join("\n")
        delete g_errors
        to_log(LOG_ERROR, "LLVM JIT FAILED:\n{errors}\n")
        g_failed = true
    }
    astVisitor.adapter := null
    unsafe {
        delete astVisitor
    }
    return true
}

// Generates function to initialize all global variables.
def public generate_globals_initialization_fn(ctx : Context?; prog : Program?;
                                              var types : PrimitiveTypes?;
                                              uids : UidNodes?;
                                              flags : LlvmJitFlags;
                                              fn_name : string = "init_globals") {
    let attrs = new Attributes(g_ctx)
    var visitor = new LlvmJitVisitor(ctx, types, uids, flags, null, attrs)
    var inscope adapter = make_visitor(*visitor)
    visitor.adapter := adapter

    let globals_fn_type = LLVMFunctionType(types.t_void,
        array<LLVMTypeRef>(types.LLVMVoidPtrType())
    )
    let globals_fn = LLVMAddFunctionWithType(g_mod, fn_name, globals_fn_type)
    if (flags.need_di) {
        set_debug_linkage(globals_fn)
    } else {
        set_private_linkage(globals_fn)
    }
    visitor.ffunc = globals_fn
    let entry = LLVMAppendBasicBlockInContext(g_ctx, globals_fn, "entry")
    visitor.function_entry = entry
    LLVMPositionBuilderAtEnd(visitor.g_builder, entry)
    let ctx_param = LLVMGetParam(globals_fn, 0u)
    prog |> for_each_module <| $(mod) {
        mod |> for_each_global() <| $(glob) {
            if (glob.init != null && glob.flags.used) {
                visit_expression(glob.init, adapter)
                let isSolidContext = glob._module.moduleFlags.isSolidContext
                var vptr : LLVMOpaqueValue?
                if (!isSolidContext) {
                    let mnh = hash(get_mangled_name(glob))
                    var params = fixed_array(
                        LLVMConstInt(types.t_int64, mnh, 0),
                        ctx_param
                    )
                    var func = LLVMGetNamedFunction(g_mod, glob.flags.global_shared ? FN_JIT_GET_SHARED_MNH : FN_JIT_GET_GLOBAL_MNH)
                    var typ = g_fn_types[glob.flags.global_shared ? FN_JIT_GET_SHARED_MNH : FN_JIT_GET_GLOBAL_MNH]
                    vptr = LLVMBuildCall2(visitor.g_builder, typ, func, params, "global_var_{glob.name}_mnh_ptr")
                    vptr = LLVMBuildPointerCast(visitor.g_builder, vptr, get_type_pointer(glob._type), "global_var_{glob.name}")
                    if (glob._type.isString) {
                        vptr = LLVMBuildLoadData2Aligned(visitor.g_builder, type_to_llvm_type(glob._type), vptr, glob._type.alignOf, "global_var_r2v_{glob.name}")
                    }

                } else {
                    var ctx_ptr = LLVMBuildPointerCast(visitor.g_builder, ctx_param, LLVMPointerType(types.t_int8, 0u), "")
                    let coffset = glob.flags.global_shared ? CONTEXT_OFFSET_OF_SHARED : CONTEXT_OFFSET_OF_GLOBALS
                    var global_ptr = LLVMBuildGEP2(visitor.g_builder, types.t_int8, ctx_ptr, types.ConstI32(uint64(coffset)), "")
                    global_ptr = LLVMBuildLoad2(visitor.g_builder, LLVMPointerType(types.t_int8, 0u), global_ptr, "")
                    vptr = LLVMBuildGEP2(visitor.g_builder, types.t_int8, global_ptr, types.ConstI32(uint64(glob.stackTop)), "")
                }
                visitor.build_copy(vptr, glob.init)
            }
        }
    }
    LLVMBuildRetVoid(visitor.g_builder)
    visitor.adapter := null
    unsafe {
        delete visitor
    }
    return (globals_fn, globals_fn_type)

    // Visit global variables
    // And generate their initialization
}

// Retrieves previously generated function address, from `dll`
// or directly from LLVM.
[macro_function]
def public generate_llvm_code(var dll : DLLHandle?; fmna : DllName; dll_enabled : bool) {
    verify(!(is_in_completion() || is_compiling_macros() || is_folding()))
    assert(!g_failed)
    var code : void? = null
    if (dll_enabled) {
        assert(dll.handle != null)
        code = dll.get_function_address(fmna)
        if (code != null) {
            return code
        }
    }
    if (dll_enabled) {
        to_log(LOG_ERROR, "Something wrong! Non dll function used in dll mode {fmna.publ()}\n")
    }
    let fn_code = LLVMGetFunctionAddress(g_engine, fmna.publ())
    code = unsafe(reinterpret<void?> fn_code)
    if (code == null) {
        to_log(LOG_ERROR, "LLVM JIT FAILED: not generated & not found in dll {fmna.publ()}\n")
    }
    return code
}

// During JIT compilation we create some complex
// global types (fileInfo, varAnnotations),
// that can't be const initialized.
// This function creates single global constructor
// that will be called at the startup of compiled
// object file.
[macro_function]
def public generate_global_ctors_dtors(types : PrimitiveTypes?; jit_mode : bool) {
    var ctor_dtor : array<tuple<LLVMOpaqueValue?, LLVMOpaqueValue?>>

    // FileInfo
    ctor_dtor |> push(generate_fileinfo_ctor_dtor(*types, active_filenames, jit_mode))
    // VarInfo
    ctor_dtor |> push(generate_annotations_ctor_dtor(*types, annotation_varinfo_list, jit_mode))
    ctor_dtor |> add_ctor_dtor_function(types)

    active_filenames = table<string>()
    annotation_varinfo_list |> clear()
}

[macro_function]
def public optimize_llvm_module(opt_level : uint; size_level : uint; inline_threshold : uint) {
    return if (is_in_completion() || is_compiling_macros() || is_folding())
    return if (g_failed || !LLVM_ENABLE_OPT_PASS)
    var pmb = LLVMPassManagerBuilderCreate()

    LLVMPassManagerBuilderSetOptLevel(pmb, opt_level)
    LLVMPassManagerBuilderSetSizeLevel(pmb, size_level)
    LLVMPassManagerBuilderUseInlinerWithThreshold(pmb, inline_threshold)

    var pm = LLVMCreatePassManager()
    LLVMPassManagerBuilderPopulateModulePassManager(pmb, pm)
    // LLVMAddLoopVectorizePass(pm)
    // LLVMAddSLPVectorizePass(pm)
    // LLVMPassManagerBuilderPopulateLTOPassManager(pmb, pm, 1, 1)
    LLVMRunPassManager(pm, g_mod)
    LLVMDisposePassManager(pm)
    LLVMPassManagerBuilderDispose(pmb)
}

class public DisableJitVisitor : AstVisitor {
    disable : bool
    uids : table<uint64, uint64>
    def TypeInfoVisitor {
        disable = false
    }
    def override preVisitExprTypeInfo(expr : smart_ptr<ExprTypeInfo>) : void {
        if (expr.trait == "ast_typedecl" || expr.trait == "rtti_classinfo" || expr.trait == "rtti_typeinfo") {
            pass
        } else {
            to_log(LOG_WARNING, "LLVM JIT: unsupported typeinfo trait `{expr.trait}` at {expr.at}, falling back to interpreter. Please report at https://github.com/GaijinEntertainment/daScript/issues\n")
            disable = true
        }
    }

    def override preVisitExprForBody(expr : smart_ptr<ExprFor>) : void {
        for (svar, ssrc in expr.iteratorVariables, expr.sources) {
            assume ssrc_t = ssrc._type
            if (ssrc_t.isRange || ssrc_t.dim |> length != 0 ||
                ssrc_t.isGoodArrayType || ssrc_t.isIterator ||
                ssrc_t.isString) {
            } else {
                assert(false,
                    "We shouldn't be here, all loop sources expected to be supported"
                )
            }
        }
    }

    def override preVisitExprClone(expr : smart_ptr<ExprClone>) : void {
        assume leftT = expr.left._type
        if (leftT.isHandle) {
            if (get_jit_clone(leftT.annotation) == null) {
                to_log(LOG_WARNING, "LLVM JIT: clone of handled type `{describe(leftT)}` is not supported at {expr.at}, falling back to interpreter. Please report at https://github.com/GaijinEntertainment/daScript/issues\n")
                disable = true
            }
        } elif (!leftT.canCopy) {
            to_log(LOG_WARNING, "LLVM JIT: clone of `{describe(leftT)}` is not supported at {expr.at}, falling back to interpreter\n")
            disable = true
        }
    }

    def override preVisitExprAt(expr : smart_ptr<ExprAt>) : void {
        if (expr.subexpr._type.isHandle) {
            let type_str = describe(expr.subexpr._type)
            to_log(LOG_WARNING,
                "LLVM JIT: native operator `.[]` should be implemented for " +
                "type {type_str} to be used in JIT, keeping interpreter mode.\n")
            disable = true
        }
    }

    def override preVisitExprForSource(var expr : smart_ptr<ExprFor>, var source : ExpressionPtr, last : bool) {
        if (source._type.isHandle) {
            let type_str = describe(source)
            to_log(LOG_WARNING,
                "LLVM JIT: `each` should be implemented for " +
                "type {type_str} to be used in JIT, keeping interpreter mode.\n")
            disable = true
        }
    }

    def disable_builtin_call(expr : smart_ptr<ExprCallFunc>) {
        if (expr.func.flags.builtIn && !expr.func.flags.interopFn && get_builtin_function_address(expr.func) == null) {
            if (!has_intrinsic(expr)) {
                to_log(LOG_WARNING, "LLVM JIT: unsupported builtin `{expr.func._module.name}::{expr.func.name}` at {expr.at}, falling back to interpreter.\n")
                disable = true
            }
        }
    }

    def override preVisitExprOp1(expr : smart_ptr<ExprOp1>) : void {
        if (isExprOp1_Func(expr)) {
            disable_builtin_call(expr)
        }
    }


    def override preVisitExprOp2(expr : smart_ptr<ExprOp2>) : void {
        if (isExprOp2_Func(expr)) {
            disable_builtin_call(expr)
        }
    }

    def override preVisitExprCall(expr : smart_ptr<ExprCall>) : void {
        disable_builtin_call(expr)
    }
}

[_macro, macro_function]
def init_llvm_jit_module_options() {
    if (is_compiling_macros_in_module("llvm_jit")) {
        this_module() |> add_module_option("jit_vec3_ldu", Type.tBool)
    }
}
