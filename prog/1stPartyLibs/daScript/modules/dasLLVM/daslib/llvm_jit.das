options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = false

module llvm_jit shared private

require llvm/llvm_boost
require llvm/llvm_jit_intrin
require llvm/llvm_jit_common
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/safe_addr
require daslib/strings_boost
require daslib/defer
require daslib/lpipe
require math
require jit
require fio

// uncomment to enable debug
/*
options debugger
require daslib/debug
require daslib/ast_debug
require llvm/llvm_debug
*/

var modules2mnh : table<Module?; array<string>>
var created_fileinfos: table<FileInfo?; FileInfo?>

[macro_function]
def private get_expr_ptr ( expr:ExpressionPtr )
    return get_ptr(expr)

struct private IteBlock
    if_true  : LLVMOpaqueBasicBlock?
    if_false : LLVMOpaqueBasicBlock?
    if_end   : LLVMOpaqueBasicBlock?
    phi_true  : LLVMOpaqueBasicBlock?
    phi_false : LLVMOpaqueBasicBlock?
    if_true_terminates, if_false_terminates : bool

struct private LoopBlock
    loop_start : LLVMOpaqueBasicBlock?
    loop_body  : LLVMOpaqueBasicBlock?
    loop_end   : LLVMOpaqueBasicBlock?
    loop_continue : LLVMOpaqueBasicBlock?
    need_loop  : LLVMOpaqueValue?

struct SLabel
    index : int
    blk : LLVMOpaqueBasicBlock?

[macro]
class LlvmJitVisitor : AstVisitor
    adapter : smart_ptr<VisitorAdapter>
    e2v : table<Expression?;LLVMOpaqueValue?>
    v2v : table<Variable?;LLVMOpaqueValue?>
    v2t : table<LLVMOpaqueValue?;LLVMOpaqueType?>
    call2cmres : table<Expression?;LLVMOpaqueValue?>
    ffunc, wfunc : LLVMOpaqueValue?
    ite2blocks : table<Expression?; IteBlock>
    thisFunc : Function?
    thisBlock : ExprBlock?
    subblocks : table<LLVMOpaqueBasicBlock?; LLVMOpaqueBasicBlock?>
    loop_stack : array<LoopBlock>
    block_stack : array<ExprBlock?>
    jit_context : Context?
    function_entry : LLVMOpaqueBasicBlock?
    function_body : LLVMOpaqueBasicBlock?
    monad2block : table<Expression?;LLVMOpaqueBasicBlock?>
    prologue : LLVMOpaqueValue?
    labels : table<int;LLVMOpaqueBasicBlock?>
    sortedLabels : array<SLabel>
    finallyBlocks : table<ExprBlock?; LLVMOpaqueBasicBlock?>
    [[skip_field_lock_check]] afterFinallyBlocks : table<ExprBlock?;array<LLVMOpaqueBasicBlock?>>
    skipCall : table<ExprCall?>
    range2 : table<Expression?; LLVMOpaqueValue?>       // for loop - range - where to
    callBlock : LLVMOpaqueValue?
    g_builder : LLVMOpaqueBuilder?
    g_di_builder : LLVMOpaqueDIBuilder?
    di_file_location : table<string;LLVMOpaqueMetadata?>
    di_function : LLVMOpaqueMetadata?
    di_cu : LLVMOpaqueMetadata?
    option_no_range_check : bool = false
    option_no_alias : bool = false
    option_no_capture : bool = false
    own_di : bool = false
    ldu_hint : array<bool>

    def LlvmJitVisitor ( at:LineInfo; ctx:Context?; need_di:bool; dib:LLVMOpaqueDIBuilder?; dicu:LLVMOpaqueMetadata? )
        jit_context = ctx
        g_builder = LLVMCreateBuilder()
        g_di_builder = dib
        di_cu = dicu
        if LLVM_DEBUG_INFO && need_di
            own_di = true
            g_di_builder = LLVMCreateDIBuilder(g_mod)
            let producer = "daScript LLVM JIT"
            let runtime_version = 3u
            di_cu = LLVMDIBuilderCreateCompileUnit(
                g_di_builder,
                LLVMDWARFSourceLanguage C,
                get_debug_file_location(at),
                producer,
                uint64(length(producer)),
                LLVM_ENABLE_OPT_PASS ? 1 : 0, // isOptimized,
                "", // const char *Flags,
                0ul, // size_t FlagsLen,
                runtime_version, // unsigned RuntimeVer,
                "", // const char *SplitName,
                0ul, // size_t SplitNameLen,
                LLVMDWARFEmissionKind LLVMDWARFEmissionFull, // LLVMDWARFEmissionKind Kind,
                0u, // unsigned DWOId,
                0, // LLVMBool SplitDebugInlining,
                0, // LLVMBool DebugInfoForProfiling,
                "", // const char *SysRoot,
                0ul, // size_t SysRootLen,
                "", // const char *SDK,
                0ul // size_t SDKLen
                )

    def finalize
        if LLVM_DEBUG_INFO
            if own_di && g_di_builder!=null
                LLVMDIBuilderFinalize(g_di_builder)
                LLVMDisposeDIBuilder(g_di_builder)
        LLVMDisposeBuilder(g_builder)

// Debug info
    def debug_before_function ( fun:FunctionPtr )
        if LLVM_DEBUG_INFO
            clear_builder_location()
            let unit = get_debug_file_location(fun.at)
            let funName = get_mangled_name(fun)
            var ty = LLVMDIBuilderCreateSubroutineType(     // TODO: generate correct function type
                g_di_builder,
                unit,
                null, // LLVMMetadataRef *ParameterTypes,
                0u, // unsigned NumParameterTypes,
                LLVMDIFlags LLVMDIFlagZero
            )
            let implName = "{funName} implementation"
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
                    LLVMDIFlags LLVMDIFlagZero, // Flags
                    LLVM_ENABLE_OPT_PASS ? 1 : 0
            )
            LLVMSetSubprogram(ffunc, di_function)

    def debug_after_function
        clear_builder_location()

    def get_debug_file_location ( at:LineInfo ) : LLVMOpaqueMetadata?
        if LLVM_DEBUG_INFO
            let filename = at.fileInfo!=null ? string(at.fileInfo.name) : ""
            return get_debug_file_location_by_name(filename)
        else
            return null

    def get_debug_file_location_by_name ( filename:string ) : LLVMOpaqueMetadata?
        var meta & = unsafe(di_file_location[filename])
        if meta==null
            let dir = dir_name(filename)
            meta = LLVMDIBuilderCreateFile(g_di_builder, filename, uint64(length(filename)), dir, uint64(length(dir)))   // todo: boost
        return meta

    def get_debug_location ( at:LineInfo ) : LLVMOpaqueMetadata?
        if LLVM_DEBUG_INFO
            let file_meta = get_debug_file_location(at)
            return LLVMDIBuilderCreateDebugLocation(LLVMGetGlobalContext(), at.line, at.column, di_function, null)
        else
            return null

    def set_builder_location ( at:LineInfo )
        if LLVM_DEBUG_INFO
            if di_function != null
                LLVMSetCurrentDebugLocation2(g_builder, get_debug_location(at))

    def clear_builder_location
        if LLVM_DEBUG_INFO
            LLVMSetCurrentDebugLocation2(g_builder, null)

// Expression
    def override preVisitExpression(expr:ExpressionPtr) : void
        set_builder_location(expr.at)

// Function
    def build_noalias_list ( annotations:AnnotationList ) : tuple<noalias:table<string>;nocapture:table<string>>
        var noalias : table<string>
        var nocapture : table<string>
        for ann in annotations
            if ann.annotation.name=="hint"
                for arg in ann.arguments
                    if arg.name=="noalias"
                        if arg.basicType == Type tString
                            noalias |> insert(string(arg.sValue))
                    elif arg.name=="nocapture"
                        if arg.basicType == Type tString
                            nocapture |> insert(string(arg.sValue))
        return <- [[auto noalias,nocapture]]

    def process_function_hints ( annotations:AnnotationList; var arguments:dasvector`smart_ptr`Variable )
        var alwaysInline = false
        var noInline = false
        var hot = false
        option_no_range_check = false
        option_no_alias = false
        option_no_capture = false
        for ann in annotations
            if ann.annotation.name=="hint"
                for arg in ann.arguments
                    if arg.name=="alwaysinline"
                        alwaysInline = true
                    if arg.name=="noinline"
                        noInline = true
                    elif arg.name=="hot"
                        hot = true
                    elif arg.name=="unsafe_range_check"
                        option_no_range_check = true
                    elif arg.name=="unsafe_alias"
                        option_no_alias = true
                    elif arg.name=="unsafe_capture"
                        option_no_capture = true
                    elif arg.name=="align16" && arg.basicType == Type tString
                        for a,ai in arguments,ucount()
                            if a.name==arg.sValue
                                LLVMSetParamAlignment(LLVMGetParam(ffunc,ai), 16u)
                    elif arg.name=="vec3_ldu"
                        LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = arg.bValue
        if alwaysInline
            LLVMAddAttributeToFunction(ffunc, LLVMGetEnumAttribute("alwaysinline"))
        elif noInline
            LLVMAddAttributeToFunction(ffunc, LLVMGetEnumAttribute("noinline"))
        if hot
            LLVMAddAttributeToFunction(ffunc, LLVMGetEnumAttribute("hot"))

    def process_finally ( expr:ExpressionPtr )
        var blk <- collect_finally(expr,true)
        for bl in blk
            finallyBlocks[bl] = LLVMAppendBasicBlock(ffunc, "finally_block_at_{int(bl.at.line)}_")
        if length(blk) > 0
            at_function_entry <|
                callBlock = LLVMBuildAlloca(g_builder, LLVMPointerType(LLVMInt8Type(),0u), "call_block")
                LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(),0u)), callBlock)

    def process_labels ( expr:ExpressionPtr )
        var lls <- collect_labels(expr)
        for ll in lls
            labels[ll] = LLVMAppendBasicBlock(ffunc, "label_{ll}_")
        sortedLabels |> reserve(length(labels))
        for k,v in keys(labels),values(labels)
            sortedLabels |> push([[SLabel index=k,blk=v]])
        sort ( sortedLabels ) <| $ (a, b)
            return a.index < b.index

    def build_llvm_function_pair ( fun_name:string; isCmres:bool; var arguments:dasvector`smart_ptr`Variable; var result:TypeDeclPtr; captureType:LLVMOpaqueType?; annotations:AnnotationList )
        var inscope nat <- build_noalias_list(annotations)
        // implementation function
        var fun_args <-  [{for a in arguments; type_to_llvm_abi_type(a._type)}]
        if captureType!=null
            fun_args |> push(LLVMPointerType(captureType,0u))  // capture
        fun_args |> push(LLVMPointerType(LLVMVoidType(), 0u))  // context
        var fun_type : LLVMOpaqueType?
        if isCmres
            fun_args |> push(LLVMPointerType(LLVMVoidType(), 0u))  // CMRES
            fun_type = LLVMFunctionType(LLVMVoidType(), fun_args)
        else
            fun_type = LLVMFunctionType(type_to_llvm_abi_type(result), fun_args)
        ffunc = LLVMAddFunctionWithType(g_mod, "{fun_name} implementation", fun_type)
        LLVMSetLinkage(ffunc, LLVMLinkage LLVMDLLExportLinkage)
        LLVMSetDLLStorageClass(ffunc, LLVMDLLStorageClass LLVMDLLExportStorageClass);
        // setup parameter alignment
        var readonly_attr = LLVMGetEnumAttribute("readonly")
        var noalias_attr = LLVMGetEnumAttribute("noalias")
        var nocapture_attr = LLVMGetEnumAttribute("nocapture")
        for a,ai in arguments,ucount()
            if a._type.isRef
                LLVMSetParamAlignment(LLVMGetParam(ffunc,ai), uint(a._type.alignOf))
            elif a._type.isPointer && a._type.firstType!=null
                LLVMSetParamAlignment(LLVMGetParam(ffunc,ai), uint(a._type.firstType.alignOf))
            if (a._type.isRef || a._type.isPointer || a._type.isString) && a._type.flags.constant
                LLVMAddAttributeToFunctionArgument(ffunc, ai, readonly_attr)
            if option_no_alias || nat.noalias |> key_exists(string(a.name))
                LLVMAddAttributeToFunctionArgument(ffunc, ai, noalias_attr)
            if option_no_capture || nat.nocapture |> key_exists(string(a.name))
                LLVMAddAttributeToFunctionArgument(ffunc, ai, nocapture_attr)
        // noalias of the capture target, context, result
        var nai = uint(length(arguments))
        if captureType!=null
            LLVMAddAttributeToFunctionArgument(ffunc, nai, nocapture_attr)
            LLVMAddAttributeToFunctionArgument(ffunc, nai++, noalias_attr)  // capture target
        LLVMAddAttributeToFunctionArgument(ffunc, nai, nocapture_attr)
        LLVMAddAttributeToFunctionArgument(ffunc, nai++, noalias_attr)      // context
        if isCmres
            LLVMAddAttributeToFunctionArgument(ffunc, nai, nocapture_attr)
            LLVMAddAttributeToFunctionArgument(ffunc, nai++, noalias_attr)  // cmres
        // alignment of the context and the result
        LLVMSetParamAlignment(get_context_param(), 16u)
        if isCmres
            LLVMSetParamAlignment(get_cmres_param(), uint(result.alignOf))
        // wrapper function
        var ret_type : LLVMOpaqueType?
        if captureType!=null
            ret_type = LLVMFunctionType(LLVMFloat4Type(),
                [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u);   // context
                LLVMPointerType(LLVMFloat4Type(),0u);               // args
                LLVMPointerType(LLVMVoidType(),0u);                 // cmres
                LLVMPointerType(captureType,0u)                     // block
            ]])
        else
            ret_type = LLVMFunctionType(LLVMFloat4Type(),
                [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u);   // context
                LLVMPointerType(LLVMFloat4Type(),0u);               // args
                LLVMPointerType(LLVMVoidType(),0u)                  // cmres
            ]])
        wfunc = LLVMAddFunctionWithType(g_mod, fun_name, ret_type)
        LLVMSetLinkage(wfunc, LLVMLinkage LLVMDLLExportLinkage)
        LLVMSetDLLStorageClass(wfunc, LLVMDLLStorageClass LLVMDLLExportStorageClass)
        LLVMSetParamAlignment(LLVMGetParam(wfunc,0u), 16u)
        LLVMSetParamAlignment(LLVMGetParam(wfunc,1u), 16u)
        if isCmres
            LLVMSetParamAlignment(LLVMGetParam(wfunc,2u), uint(result.alignOf))
        if captureType!=null
            LLVMSetParamAlignment(LLVMGetParam(wfunc,3u), 16u)

    def add_llvm_functions ( var fun : FunctionPtr )
        thisFunc = get_ptr(fun)
        let fnmna = get_mangled_name(fun)
        build_llvm_function_pair(fnmna, isCMRES(fun), fun.arguments, fun.result, null, fun.annotations)
        thisFunc = null

    def build_wrapper_funcion(fnmna: string; arguments:dasvector`smart_ptr`Variable; var result:TypeDeclPtr; isCmres:bool; captureType:LLVMOpaqueType?; at:LineInfo )
        var wentry = LLVMAppendBasicBlock(wfunc, "entry")
        LLVMPositionBuilderAtEnd(g_builder, wentry)
        var wargs : array<LLVMOpaqueValue?>
        var args = LLVMGetParam(wfunc,1u)
        for a,ai in arguments, count()
            var arg_ptr = LLVMBuildGEP2(g_builder, LLVMFloat4Type(), args, LLVMConstInt(LLVMInt32Type(), uint64(ai), 0), "arg_ptr_{ai}")
            var arg_type = type_to_llvm_abi_type(a._type)
            arg_ptr = LLVMBuildPointerCast(g_builder, arg_ptr, LLVMPointerType(arg_type, 0u), "")
            var arg_v : LLVMOpaqueValue?
            if a._type.isVoid
                pass
            elif a._type.flags.ref
                arg_v = LLVMBuildLoad2(g_builder, arg_type, arg_ptr, "arg_ref_value_{ai}")
            elif !a._type.isRef
                arg_v = LLVMBuildLoad2(g_builder, arg_type, arg_ptr, "arg_value_{ai}")
            else
                arg_v = LLVMBuildLoad2(g_builder, arg_type, arg_ptr, "arg_any_{ai}")
            if arg_v != null
                wargs |> push(arg_v)
        if captureType!=null
            wargs |> push(LLVMGetParam(wfunc,3u))   // block
        wargs |> push(LLVMGetParam(wfunc,0u))  // context
        if isCmres
            wargs |> push(LLVMGetParam(wfunc,2u))  // cmres
        var ret_v = LLVMBuildCall2(g_builder, g_fn_types[("{fnmna} implementation")], ffunc, wargs, "")
        if !result.isVoid
            if isCmres
                LLVMBuildRet(g_builder, cast_ptr_to_vec4f(LLVMGetParam(wfunc,2u)))
            else
                var tres = cast_to_vec4f(result, ret_v)
                if tres != null
                    LLVMBuildRet(g_builder, tres)
                else
                    LLVMBuildRet(g_builder, LLVMGetUndef(LLVMFloat4Type()))
        else
            LLVMBuildRet(g_builder, LLVMGetUndef(LLVMFloat4Type()))

    def get_first_element_addr ( dummy: LLVMOpaqueValue?; typ: LLVMTypeRef )
        let zero = LLVMConstInt(LLVMInt32Type(), 0 |> uint64(), 0)
        var indices = [[auto zero; zero]]
        return g_builder |> LLVMBuildGEP2(typ, dummy, unsafe(addr(indices[0])), 2u, "")

    def get_file_info_global ()
        let sz = typeinfo(sizeof type<FileInfo>)
        let file_info_type = LLVMArrayType(LLVMInt8Type(), uint(sz))
        var dummy = LLVMGetNamedGlobal(g_mod, "fileinfo_dummy")
        if dummy == null
            dummy = LLVMAddGlobal(g_mod, file_info_type, "fileinfo_dummy")
            var zero = LLVMConstInt(LLVMInt8Type(), 0 |> uint64(), 0)
            var zeros : array<LLVMValueRef>
            for i in range(sz)
                zeros |> push(zero)
            let zeroInitializer = LLVMConstArray(LLVMInt8Type(), unsafe(addr(zeros[0])), sz |> uint())
            LLVMSetInitializer(dummy, zeroInitializer)
        return dummy |> get_first_element_addr(LLVMArrayType(LLVMInt8Type(), uint(sz)))

    def create_line_info_global ( at: LineInfo; dummy: LLVMOpaqueValue? )
        let uint8_ptr = LLVMPointerType(LLVMInt8Type(), 0u)
        var line_info_members = [[auto
            LLVMPointerType(LLVMInt8Type(), 0u); LLVMInt32Type(); LLVMInt32Type(); LLVMInt32Type(); LLVMInt32Type()
        ]]
        let line_info_type = LLVMStructType(unsafe(addr(line_info_members[0])), line_info_members |> length() |> uint(), 0)
        let line_info_global = LLVMAddGlobal(g_mod, line_info_type, "" /* no name */)
        var init_values =  [[auto g_builder |> LLVMBuildPointerCast(dummy, uint8_ptr, "");
            LLVMConstInt(LLVMInt32Type(), at.column |> uint64(), 0);
            LLVMConstInt(LLVMInt32Type(), at.line |> uint64(), 0);
            LLVMConstInt(LLVMInt32Type(), at.last_column |> uint64(), 0);
            LLVMConstInt(LLVMInt32Type(), at.last_line |> uint64(), 0)
        ]]
        var line_info_init = LLVMGetUndef(line_info_type)
        for i in urange(init_values |> length())
            line_info_init = g_builder |> LLVMBuildInsertValue(line_info_init, init_values[i], i, "")
        // let LineInfoInit = LLVMConstStruct(unsafe(addr(InitValues[0])), InitValues |> length() |> uint(), 0)
        LLVMSetInitializer(line_info_global, line_info_init)
        return line_info_global

    def create_line_info_ptr_global ( line_info_global: LLVMOpaqueValue? )
        // let ptr_to_line_info_global = get_first_element_addr(line_info_global)
        let uint8_ptr = LLVMPointerType(LLVMPointerType(LLVMInt8Type(), 0u), 0u)
        let line_info_ptr_type = uint8_ptr
        // print("Type of LineInfoPtrType : '{LineInfoPtrType |> describe}'\n")
        // print("Type of ptrToLineInfoGlobal : '{ptrToLineInfoGlobal |> LLVMTypeOf() |> describe()}'\n")
        let line_info_ptr_global = LLVMAddGlobal(g_mod, line_info_ptr_type, "" /* no name */)
        LLVMSetInitializer(line_info_ptr_global, line_info_global)
        return line_info_ptr_global

    def instrument_and_map_global ( at: LineInfo; line_info_ptr_global: LLVMOpaqueValue? )
        let info_ptr = instrument_line_info(at) // pointer in 'code' of the context
        g_engine |> LLVMAddGlobalMapping(line_info_ptr_global, info_ptr)
        return line_info_ptr_global

    def get_line_info_ptr ( at:LineInfo )
        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        let dummy = get_file_info_global()
        let line_info_global = create_line_info_global(at, dummy)
        let line_info_ptr_global = create_line_info_ptr_global(line_info_global)
        instrument_and_map_global(at, line_info_ptr_global)
        var loaded_ptr = LLVMBuildLoad2(g_builder, void_ptr, line_info_ptr_global, "")
        // return g_builder |> LLVMBuildPointerCast(loaded_ptr, void_ptr, "")
        return loaded_ptr

    def build_function_entry(funAt:LineInfo; bodyId:uint64; hasMakeBlock:bool; var totalStackSize:uint; var arguments:dasvector`smart_ptr`Variable)
        // now write to regular function
        function_entry = LLVMAppendBasicBlock(ffunc, "entry")
        LLVMPositionBuilderAtEnd(g_builder,function_entry)
        // enter
        build_enter(funAt)
        // allocate prologue if need be
        if hasMakeBlock
            prologue = LLVMBuildAlloca(g_builder, g_t_stack_state, "prologue")
            prologue = LLVMBuildPointerCast(g_builder, prologue, LLVMPointerType(g_t_stack_state,0u), "")
            var params = [[auto
                get_line_info_ptr(funAt);
                LLVMConstInt(LLVMInt32Type(), uint64(totalStackSize), 0);
                prologue;
                get_context_param()
            ]]
            var typ = g_fn_types[FN_JIT_PROLOGUE]
            LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_PROLOGUE), params, "")
        // allocate arguments, which need to be promoted to local variables
        // NOTE:
        //  thisFunc hasMakeBlock is here just in case we need to capture argument variables
        //  in the future, we can optimize this out by checking, if particular variable is captured via block
        //  additionally this causes every block variable to be promoted - which may not be ideal since it would clutter the optimizer (or not?)
        var alwaysPromote = false
        if thisBlock!=null && thisBlock.blockFlags.hasMakeBlock
            alwaysPromote = true
        elif thisFunc!=null && thisFunc.flags.hasMakeBlock
            alwaysPromote = true
        for a,ai in arguments,range(10050)
            if (alwaysPromote || a.access_flags.access_ref || a.access_flags.access_pass) && !a._type.isRef
                var tryV = LLVMBuildAlloca(g_builder, type_to_llvm_abi_type(a._type), "var_{a.name}")
                LLVMSetAlignment(tryV, 16u)
                LLVMBuildStore(g_builder, LLVMGetParam(ffunc,uint(ai)), tryV)
                setV(a, tryV)
        // function body
        function_body = LLVMAppendBasicBlock(ffunc, "body_{bodyId}")
        LLVMPositionBuilderAtEnd(g_builder,function_body)

    def override canVisitFunctionArgumentInit (fun:Function?; arg:VariablePtr;value:ExpressionPtr) : bool
        return false

    def override preVisitFunction(var fun:FunctionPtr) : void
        assert(g_builder!=null, "missing builder")
        thisFunc = get_ptr(fun)
        ldu_hint |> push(LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS)
        let fnmna = get_mangled_name(fun)
        ffunc = LLVMGetNamedFunction(g_mod, "{fnmna} implementation")
        wfunc = LLVMGetNamedFunction(g_mod, fnmna)
        // build wrapper function
        build_wrapper_funcion(fnmna, fun.arguments, fun.result, isCMRES(fun), null, fun.at)
        build_function_entry(fun.at, intptr(fun), fun.flags.hasMakeBlock, fun.totalStackSize, fun.arguments)
        process_function_hints(fun.annotations, fun.arguments)
        process_labels(fun.body)
        process_finally(fun.body)
        debug_before_function(fun)
        set_builder_location(fun.at)

    def build_epilogue
        if prologue != null
            var params = [[auto
                prologue;
                get_context_param()
            ]]
            var typ = g_fn_types[FN_JIT_EPILOGUE]
            LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_EPILOGUE), params, "")

    def current_block_terminates
        return LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(g_builder))!=null

    def override visitFunction(fun:FunctionPtr) : FunctionPtr
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        LLVMBuildBr(g_builder, function_body)
        debug_after_function()
        ldu_hint |> pop()
        ffunc = null
        thisFunc = null
        return fun

// ExprBlock
    def add_default_terminator(blk:smart_ptr<ExprBlock>)
        if thisBlock==null && thisFunc!=null && thisFunc.body!=get_ptr(blk)
            return
        if thisBlock!=null && thisBlock!=get_ptr(blk)
            return
        if !current_block_terminates()
            call_all_finally_blocks("default_terminator", false)
            build_exit(blk.at)
            if thisBlock!=null && thisBlock.returnType!=null && !thisBlock.returnType.isVoid
                LLVMBuildUnreachable(g_builder)
            elif thisBlock==null && thisFunc!=null && thisFunc.result!=null && !thisFunc.result.isVoid
                LLVMBuildUnreachable(g_builder)
            else
                build_epilogue()
                LLVMBuildRetVoid(g_builder)

    def hasFinalSection ( blk:ExprBlock? )
        if blk==null
            return false
        if length(blk.finalList)>0
            return true
        // TODO: do better test here. block with iterator, or over an array<> has a final section. maybe other??? but not all
        return blk.blockFlags.forLoop   // for loop always has final section

    def has_any_finally_block(stopAtLoop:bool)
        var len = length(block_stack)
        for i in range(len)
            var blk = block_stack[len-1-i]
            if stopAtLoop && blk.blockFlags.inTheLoop
                break
            return true
        return false

    def call_all_finally_blocks(from_where:string; stopAtLoop:bool)
        var len = length(block_stack)
        var anyFinallyBlocks = false
        for i in range(len)
            var blk = block_stack[len-1-i]
            if stopAtLoop && (blk.blockFlags.inTheLoop || blk.blockFlags.forLoop)
                break
            if blk |> hasFinalSection()
                var after = LLVMAppendBasicBlock(ffunc, "{from_where}_after_call_finally_block_{int(blk.at.line)}_")
                var after_ptr = LLVMBlockAddress(LLVMGetBasicBlockParent(after),after)
                LLVMBuildStore(g_builder, after_ptr, callBlock)
                var fblk = finallyBlocks[get_ptr(blk)]
                LLVMBuildBr(g_builder, fblk)
                LLVMPositionBuilderAtEnd(g_builder, after)
                afterFinallyBlocks[get_ptr(blk)] |> push(after)
                anyFinallyBlocks = true
        return anyFinallyBlocks

    def override preVisitExprBlock(var blk:smart_ptr<ExprBlock>) : void
        if blk.blockFlags.inTheLoop
            blk.blockFlags |= ExprBlockFlags finallyDisabled
        block_stack |> push(get_ptr(blk))

    def override preVisitExprBlockExpression(blk:smart_ptr<ExprBlock>;expr:ExpressionPtr): void
        if LLVM_DEBUG_TRACES && LLVM_DEBUG_LINE_TRACES
            if !current_block_terminates()
                build_debug_trace(expr.at, FN_JIT_DEBUG_LINE)

    def override preVisitExprBlockFinal(blk:smart_ptr<ExprBlock>): void
        add_default_terminator(blk)
        if !blk.blockFlags.forLoop                              // for loop just does it
            if finallyBlocks |> key_exists(get_ptr(blk))
                var fblk = finallyBlocks[get_ptr(blk)]
                if !current_block_terminates()
                    LLVMBuildBr(g_builder, fblk)
                LLVMPositionBuilderAtEnd(g_builder, fblk)

    def jump_to_next_finally ( blk:ExprBlock? )
        afterFinallyBlocks  |> get(blk) <| $ ( AfterFinallyBlocks )
            if length(AfterFinallyBlocks)>0
                var cc = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMInt8Type(),0u), callBlock, "")
                var needCond = thisFunc.body!=get_ptr(blk) && thisBlock!=get_ptr(blk)
                var do_jump, skip_jump : LLVMOpaqueBasicBlock?
                if needCond
                    do_jump = LLVMAppendBasicBlock(ffunc, "do_jump_blk_line_{int(blk.at.line)}_")
                    skip_jump = LLVMAppendBasicBlock(ffunc, "skip_jump_blk_line_{int(blk.at.line)}_")
                    var not_null = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, cc, LLVMConstNull(LLVMPointerType(LLVMInt8Type(),0u)), "")
                    LLVMBuildCondBr(g_builder, not_null, do_jump, skip_jump)
                    LLVMPositionBuilderAtEnd(g_builder, do_jump)
                var indb = LLVMBuildIndirectBr(g_builder, cc, uint(length(AfterFinallyBlocks)))
                for bb in AfterFinallyBlocks
                    LLVMAddDestination(indb, bb)
                if skip_jump!=null
                    LLVMPositionBuilderAtEnd(g_builder, skip_jump)

    def override visitExprBlock(var blk:smart_ptr<ExprBlock>) : ExpressionPtr
        if length(blk.finalList)==0         // TODO: check for loop
            add_default_terminator(blk)
        elif !blk.blockFlags.inTheLoop
            jump_to_next_finally(get_ptr(blk))
        block_stack |> pop()
        if blk.blockFlags.inTheLoop
            blk.blockFlags ^= ExprBlockFlags finallyDisabled
        return blk

// ExprLet
    def isCall2CMRES ( expr:ExpressionPtr )
        return false if !(expr is ExprCall)
        let call = expr as ExprCall
        return false if !isCMRES(call.func)
        return call.doesNotNeedSp || call.stackTop==0u

    def make_call_to_cmres ( expr:ExpressionPtr; v_ptr:LLVMOpaqueValue? )
        if isCall2CMRES(expr)
            call2cmres[get_ptr(expr)] = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(LLVMVoidType(),0u), "call_result_{(expr as ExprCall).func.name}")
            return true
        elif (expr is ExprInvoke) && isCMRESType(expr._type)
            call2cmres[get_ptr(expr)] = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(LLVMVoidType(),0u), "invoke_result_{int(expr.at.line)}")
            return true
        return false

    def override preVisitExprLetVariable(expr:smart_ptr<ExprLet>;arg:VariablePtr;lastArg:bool) : void
        var vtype, vptrtype : LLVMOpaqueType?
        if arg._type.flags.ref
            vtype = get_type_pointer(arg._type)
            vptrtype = LLVMPointerType(vtype,0u)
        else
            vtype = type_to_llvm_type(arg._type)
            if arg._type.isGoodBlockType
                vptrtype = vtype
            else
                vptrtype = get_type_pointer(arg._type)
        var v_ptr : LLVMOpaqueValue?
        if arg.flags.aliasCMRES
            v_ptr = get_cmres_param()
            v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, vptrtype, "variable_cmres_{arg.name}")
        else
            at_function_entry <|
                v_ptr = LLVMBuildAlloca(g_builder, vtype, "variable_{arg.name}")
                LLVMSetAlignment(v_ptr, 16u)
                v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, vptrtype, "variable_{arg.name}")
        setV(arg, v_ptr)
        if arg.init == null
            var vsize = arg._type.flags.ref ?  typeinfo(sizeof type<void?>) : arg._type.sizeOf
            var valign = arg._type.flags.ref ?  typeinfo(alignof type<void?>) : arg._type.alignOf
            LLVMBuildMemSet(g_builder, v_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(vsize), 0), uint(valign))
        elif make_call_to_cmres(arg.init, v_ptr)
            pass    // do nothing
        elif isMakeLocal(arg.init)
            var lptr = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(LLVMInt8Type(),0u), "alias_make_local_{arg.name}")
            setE(arg.init, lptr)

    def override visitExprLetVariableInit(blk:smart_ptr<ExprLet>;arg:VariablePtr;expr:ExpressionPtr) : ExpressionPtr
        var tdst = getV(arg)
        if isCall2CMRES(expr)
            pass    // this variable is initialized via cmres call
        elif arg._type.flags.ref
            var src = getE(expr)
            LLVMBuildStore(g_builder, src, tdst)
        elif isMakeLocal(arg.init)
            pass
        else
            if arg.flags.init_via_move && arg._type.canMove && !arg._type.isGoodBlockType
                build_move(tdst, expr, true)
            else
                build_copy(tdst, expr)
        return expr

// ExprStringBuilder
    def override visitExprStringBuilder(expr:smart_ptr<ExprStringBuilder>) : ExpressionPtr
        var args = build_array_of_arguments(expr.elements)
        var func = LLVMGetNamedFunction(g_mod, expr.stringBuilderFlags.isTempString ? FN_JIT_STRING_BUILDER_TEMP : FN_JIT_STRING_BUILDER)
        assert(func!=null)
        let NODE_PTR = make_interop_node(*jit_context,get_ptr(expr))
        if NODE_PTR==null
            failed_E(expr, "missing interop function node pointer for String Builder")
        let NODE_ADDR = intptr(NODE_PTR)
        var params <- [{LLVMOpaqueValue?
            get_context_param();
            LLVMConstIntToPtr(LLVMConstInt(LLVMInt64Type(), NODE_ADDR, 0), LLVMPointerType(LLVMVoidType(), 0u)); // Node *
            args                                                   // args
        }]
        var typ = g_fn_types[FN_JIT_STRING_BUILDER]
        setE(expr, LLVMBuildCall2(g_builder, typ, func, params, "string_builder"))
        return expr

// ExprNew
    def check_ptr_zero ( tsrc:LLVMOpaqueValue?; at:LineInfo; message:string="dereferencing null pointer")
        var check_null_ptr = LLVMAppendBasicBlock(ffunc,"check_null_ptr")
        var check_true = LLVMAppendBasicBlock(ffunc,"check_true")
        var check_end = LLVMAppendBasicBlock(ffunc,"check_end")
        LLVMBuildBr(g_builder, check_null_ptr)
        LLVMPositionBuilderAtEnd(g_builder,check_null_ptr)
        var cond = LLVMBuildIsNull(g_builder, tsrc, "null_check")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_true)
        build_exception(message, at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_end)

    def build_alloc ( bytes:int; persistent:bool )
        var params = [[auto
            LLVMConstInt(LLVMInt32Type(), uint64(bytes), 0);    // bytes
            get_context_param()
        ]]
        var typ = g_fn_types[persistent ? FN_JIT_ALLOC_PERSISTENT : FN_JIT_ALLOC_HEAP]
        var fn = LLVMGetNamedFunction(g_mod, persistent ? FN_JIT_ALLOC_PERSISTENT : FN_JIT_ALLOC_HEAP)
        return LLVMBuildCall2(g_builder, typ, fn, params, "")

    def build_fixed_loop ( count:int; blk:block<(var index:LLVMOpaqueValue?):void> )
        if count == 0
            pass
        if count == 1
            invoke ( blk, LLVMConstInt(LLVMInt32Type(), 0ul, 0) )
        else
            var loop = LLVMBuildAlloca(g_builder, LLVMInt32Type(), "loop_var")
            LLVMBuildStore(g_builder, LLVMConstInt(LLVMInt32Type(), 0ul, 0), loop)
            var loop_bb = LLVMAppendBasicBlock(ffunc,"loop_begin")
            var loop_end_bb = LLVMAppendBasicBlock(ffunc,"loop_end")
            LLVMBuildBr(g_builder, loop_bb)
            LLVMPositionBuilderAtEnd(g_builder,loop_bb)
            invoke ( blk, LLVMBuildLoad2(g_builder,LLVMInt32Type(),loop,""))
            LLVMBuildStore(g_builder, LLVMBuildAdd(g_builder, LLVMBuildLoad2(g_builder,LLVMInt32Type(),loop,""), LLVMConstInt(LLVMInt32Type(), 1ul, 0), ""), loop)
            var loop_cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntULT, LLVMBuildLoad2(g_builder,LLVMInt32Type(),loop,""), LLVMConstInt(LLVMInt32Type(), uint64(count), 0), "")
            LLVMBuildCondBr(g_builder, loop_cond, loop_bb, loop_end_bb)
            LLVMPositionBuilderAtEnd(g_builder,loop_end_bb)

    def new_entry(expr:smart_ptr<ExprNew>)
        var new_ptr : LLVMOpaqueValue?
        var persistent = false
        if expr.typeexpr.baseType == Type tStructure
            persistent = expr.typeexpr.structType.flags.persistent
        let bytes = expr.typeexpr.baseSizeOf
        new_ptr = build_alloc(bytes, persistent)
        check_ptr_zero(new_ptr, expr.at, "new returned null")
        if expr.initializer
            call2cmres[get_ptr(ExpressionPtr(expr))] = new_ptr
            make_call(expr, false)
        else
            LLVMBuildMemSet(g_builder, new_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(bytes), 0), 16u)
        return LLVMBuildPointerCast(g_builder, new_ptr, dim_element_type_to_llvm_type(expr._type), "new")

    def get_new_handle_function ( t:TypeDeclPtr ) : LLVMOpaqueValue?
        let mangeld_name = get_mangled_name(t)
        let func_name = "new`handle`{mangeld_name}"
        var new_handle = LLVMGetNamedFunction(g_mod, func_name)
        if new_handle == null
            var new_handle_ptr = get_jit_new(t)
            if new_handle_ptr == null
                failed_T(t, "missing new`handle function pointer for {describe(t)}")
                return null
            // void * new_handle ( Context * ) {
            new_handle = LLVMAddFunctionWithType(g_mod, func_name,
                LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
                    [[LLVMTypeRef[] LLVMPointerType(LLVMVoidType(),0u)]]))
            LLVMAddGlobalMapping(g_engine, new_handle, new_handle_ptr)
            var nounwind = LLVMGetEnumAttribute("nounwind")
            var willreturn = LLVMGetEnumAttribute("willreturn")
            var nocapture = LLVMGetEnumAttribute("nocapture")
            LLVMAddAttributesToFunction(new_handle, [[auto nounwind; willreturn]])
            LLVMAddAttributeToFunctionArgumentRange(new_handle, urange(0,1), nocapture)
        return new_handle

    def override visitExprNew(expr:smart_ptr<ExprNew>) : ExpressionPtr
        if expr.typeexpr.baseType==Type tHandle
            let fn_new_handle = get_new_handle_function(expr.typeexpr)
            if fn_new_handle == null
                setE(expr, LLVMConstNull(LLVMPointerType(LLVMVoidType(),0u)))
                return expr
            var params = [[auto[]
                get_context_param()
            ]]
            var mangled_name = expr.typeexpr |> get_mangled_name()
            var typ = g_fn_types[("new`handle`{mangled_name}")]
            var new_ptr = LLVMBuildCall2(g_builder, typ, fn_new_handle, params, "new`handle`{describe(expr.typeexpr)}")
            check_ptr_zero(new_ptr, expr.at, "new returned null")
            new_ptr = LLVMBuildPointerCast(g_builder, new_ptr, LLVMPointerType(type_to_llvm_type(expr.typeexpr),0u), "")
            setE(expr, new_ptr)
        else
            if expr.typeexpr.dim |> length > 0
                let count = expr.typeexpr.countOf
                var arr : LLVMOpaqueValue?
                at_function_entry <|
                    arr = LLVMBuildAlloca(g_builder, type_to_llvm_type(expr._type), "new_arr")
                build_fixed_loop ( count ) <| $ ( var index:LLVMOpaqueValue? )
                    var elem_type = dim_element_type_to_llvm_type(expr._type)
                    var elem = LLVMBuildGEP2(g_builder, elem_type, arr, index, "")
                    var new_ptr = new_entry(expr)
                    LLVMBuildStore(g_builder, new_ptr, elem)
                setE(expr, arr)
            else
                setE(expr, new_entry(expr))
        return expr

// ExprLooksLikeCall
    def override preVisitExprLooksLikeCall(expr:smart_ptr<ExprLooksLikeCall>): void
        pass // do nothing, works as is?

// ExprCall
    def build_array_of_arguments ( arguments:dasvector`smart_ptr`Expression; skip0:bool = false ) : LLVMOpaqueValue?
        var nargs = length(arguments)
        var from_ai = 0
        if skip0
            nargs --
            from_ai --
        var args : LLVMOpaqueValue?
        if nargs != 0
            at_function_entry <|
                args = LLVMBuildAlloca(g_builder, LLVMArrayType(LLVMFloat4Type(), uint(nargs)), "args")
                LLVMSetAlignment(args, 16u)
                args = LLVMBuildPointerCast(g_builder, args, LLVMPointerType(LLVMFloat4Type(), 0u), "call_args_ptr")
            for a,ai in arguments,range(from_ai,100500)
                if ai<0
                    continue
                var arg_ptr = LLVMBuildGEP2(g_builder, LLVMFloat4Type(), args, LLVMConstInt(LLVMInt32Type(), uint64(ai), 0), "call_arg_{ai}_ptr")
                var arg_val = getE(a)
                var arg_cast_type = type_to_llvm_abi_type(a._type)
                if a._type.isRef
                    arg_val = LLVMBuildPointerCast(g_builder, arg_val, arg_cast_type, "call_arg_{ai}_cast")
                LLVMBuildStore2(g_builder, arg_cast_type, arg_val, arg_ptr)
        else
            args = LLVMConstPointerNull(LLVMPointerType(LLVMFloat4Type(), 0u))
        return args

    def register_extern_function ( expr:smart_ptr<ExprCallFunc> ) : LLVMOpaqueValue?
        let mangled_name = get_mangled_name(FunctionPtr(expr.func))
        var ofunc = LLVMGetNamedFunction(g_mod, mangled_name)
        if ofunc == null
            var arg_types : array<LLVMOpaqueType?>
            var res_type : LLVMOpaqueType?
            res_type = type_to_llvm_type(expr.func.result)
            if expr.func.result.flags.ref
                res_type = LLVMPointerType(res_type,0u)
            let cmres = isCMRES(expr.func)
            if cmres
                arg_types |> push(LLVMPointerType(res_type,0u))
                res_type = LLVMVoidType()
            for arg in expr.func.arguments
                arg_types |> push(type_to_llvm_abi_type(arg._type))
            if expr.func.moreFlags.jitContextAndLineInfo
                arg_types |> push(LLVMPointerType(LLVMVoidType(),0u))   // context
                arg_types |> push(LLVMPointerType(LLVMVoidType(),0u))   // line info arg
            ofunc = LLVMAddFunctionWithType(g_mod, mangled_name, LLVMFunctionType(res_type,arg_types))
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if FUNC_PTR==null
                failed_E(expr, "missing interop function pointer for {expr.func._module.name}::{expr.func.name} // {mangled_name}")
            LLVMAddGlobalMapping(g_engine, ofunc, FUNC_PTR)
            if expr.func.sideEffectFlags == bitfield(0)
                LLVMAddAttributeToFunction(ofunc, LLVMGetEnumAttribute("nounwind"))     // TODO: better no-unwind?
                LLVMAddAttributeToFunction(ofunc, LLVMGetEnumAttribute("willreturn"))
            if !expr.func.sideEffectFlags.modifyExternal
                var nocapture_attr = LLVMGetEnumAttribute("nocapture")
                var ao = cmres ? 1u : 0u
                for arg,ai in expr.func.arguments,ucount()
                    if arg._type.isRef
                        LLVMAddAttributeToFunctionArgument(ofunc, ai + ao, nocapture_attr)
                if cmres
                    LLVMAddAttributeToFunctionArgument(ofunc, 0u, nocapture_attr)
        return ofunc

    def register_interop_function ( expr:smart_ptr<ExprCallFunc> ) : LLVMOpaqueValue?
        let mangled_name = get_mangled_name(FunctionPtr(expr.func))
        var ofunc = LLVMGetNamedGlobal(g_mod, mangled_name)
        if ofunc == null
            modules2mnh[expr.func._module] |> push(FunctionPtr(expr.func) |> get_mangled_name)
            var fn_type = LLVMInteropFunctionType()
            let funcPtrType = LLVMPointerType(fn_type, 0u)
            ofunc = LLVMAddGlobal(g_mod, funcPtrType, "{mangled_name}")
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if FUNC_PTR==null
                failed_E(expr, "missing interop function pointer for {expr.func._module.name}::{expr.func.name}")
            let funcPtrValue = LLVMConstIntToPtr(LLVMConstInt(LLVMIntPtrType(), FUNC_PTR |> intptr(), 0), funcPtrType)
            LLVMSetInitializer(ofunc, funcPtrValue)
            LLVMAddGlobalMapping(g_engine, ofunc, FUNC_PTR)
        return ofunc

    def build_call_params ( expr:smart_ptr<ExprCallFunc>; extern_func : Function? = null )
        var params : array<LLVMOpaqueValue?>
        for a,ai in expr.arguments,count()
            if a._type.baseType==Type fakeContext
                params |> push(get_context_param())
            elif a._type.baseType==Type fakeLineInfo
                var pinfo = getE(a)
                params |> push(pinfo)
            elif a._type.baseType==Type tPointer
                assume funcArg = expr.func.arguments[ai]
                var ptrType = type_to_llvm_abi_type(funcArg._type)
                var ptrValue = LLVMBuildPointerCast(g_builder, getE(a), ptrType, "arg_{funcArg.name}_ptr")
                params |> push( ptrValue )
            elif a._type.baseType==Type tStructure  // possibly inherited structure
                assume funcArg = expr.func.arguments[ai]
                var ptrType = type_to_llvm_abi_type(funcArg._type)
                var ptrValue = LLVMBuildPointerCast(g_builder, getE(a), ptrType, "arg_{funcArg.name}_struct")
                params |> push( ptrValue )
            elif (a._type.baseType==Type tArray || a._type.baseType==Type tTable) && (extern_func != null)
                assume funcArg = expr.func.arguments[ai]
                var destType = type_to_llvm_abi_type(funcArg._type)
                var arg = LLVMBuildPointerCast(g_builder, getE(a), destType, "any_array_{funcArg.name}_ptr")
                params |> push(arg)
            else
                params |> push(getE(a))
        return <- params

    def get_function_addr ( func:Function? )
        var MNH_ADDR : uint64
        unsafe
            let mangled_name = get_mangled_name(FunctionPtr(func))
            let MNH = hash(mangled_name)
            MNH_ADDR = get_function_address(MNH,*jit_context)
        return MNH_ADDR

    def set_meta_cconv ( instr:LLVMOpaqueValue?; conv:string )
        var values : LLVMOpaqueValue? [1]
        values[0] = LLVMMDString(conv, uint(length(conv)))
        let msg = "cconv"
        LLVMSetMetadata(
            instr,
            LLVMGetMDKindID(msg, uint(length(msg))),
            LLVMMDNode(unsafe(addr(values[0])), 1u))

    def make_call ( expr:smart_ptr<ExprCallFunc>; doesNotNeedSp:bool )
        // lookup intrinsics first
        var intrin_call = lookup_intinsic(g_builder, expr, [{for x in expr.arguments; getE(x)}])
        if intrin_call!=null
            setE(expr, intrin_call)
            return
        // get CMRES, if its local
        var cmresEval : LLVMOpaqueValue?
        if isCMRES(expr.func)
            if call2cmres |> key_exists(get_ptr(expr))
                cmresEval = call2cmres[get_ptr(ExpressionPtr(expr))]
            elif !doesNotNeedSp && expr.stackTop!=0u
                var vtype = type_to_llvm_type(expr._type)
                var v_ptr_type = get_type_pointer(expr._type)
                at_function_entry <|
                    cmresEval = LLVMBuildAlloca(g_builder, vtype, "cmres_eval_{expr.name}")
                    LLVMSetAlignment(cmresEval, 16u)
                    cmresEval = LLVMBuildPointerCast(g_builder, cmresEval, v_ptr_type, "")
            else
                cmresEval = LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(),0u))
                failed_E(expr, "CMRES is not local, not supported yet")
        if expr.func.flags.builtIn
            if expr.func.flags.interopFn
                var args = build_array_of_arguments(expr.arguments)
                var func = register_interop_function(expr)
                let NODE_PTR = make_interop_node(*jit_context,get_ptr(expr))
                if NODE_PTR==null
                    failed_E(expr, "missing interop function node pointer for {expr.func.name}")
                let NODE_ADDR = intptr(NODE_PTR)
                var params <- [{LLVMOpaqueValue?
                    get_context_param();
                    LLVMConstIntToPtr(LLVMConstInt(LLVMInt64Type(), NODE_ADDR, 0), LLVMPointerType(LLVMVoidType(), 0u)); // Node *
                    args                                                   // args
                }]
                // Load the function pointer
                var fn_type = LLVMInteropFunctionType()
                var loaded_func = LLVMBuildLoad2(g_builder, LLVMPointerType(fn_type, 0u), func, "");
                var ccall = LLVMBuildCall2(g_builder, fn_type, loaded_func, params, "")
                if expr.func.result.isVoid
                    setE(expr, ccall)
                else
                    var cres = cast_from_vec4f(expr, expr._type, ccall)
                    setE(expr, cres)
            else
                var params <- build_call_params(expr, expr.func)
                var func = register_extern_function(expr)
                var typ = g_fn_types[FunctionPtr(expr.func) |> get_mangled_name()]
                if expr.func.moreFlags.jitContextAndLineInfo
                    params |> push(get_context_param())
                    params |> push(LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(),0u)))
                if cmresEval!=null
                    var cmresPtr = LLVMBuildPointerCast(g_builder,cmresEval,LLVMPointerType(type_to_llvm_type(expr.func.result),0u),"cmresPTR")
                    params |> push(cmresPtr, 0)
                    var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
                    setE(expr, cmresEval)
                else
                    var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
                    setE(expr, ccall)
        else
            if expr.func.moreFlags.requestJit
                var params <- build_call_params(expr)
                params |> push(get_context_param())
                if cmresEval!=null
                    params |> push(LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(LLVMVoidType(),0u), ""))
                let fmna = get_mangled_name(FunctionPtr(expr.func))
                var func = LLVMGetNamedFunction(g_mod, "{fmna} implementation")
                var typ = g_fn_types[("{fmna} implementation")]
                var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
                if cmresEval != null
                    setE(expr, cmresEval)
                else
                    setE(expr, ccall)
            else
                var args = build_array_of_arguments(expr.arguments)
                var MNH_ADDR = get_function_addr(expr.func)
                if MNH_ADDR==0ul
                    failed_E(expr, "missing function pointer for {expr.func.name}")
                var params : array<LLVMOpaqueValue?>
                params |> push(LLVMConstIntToPtr(LLVMConstInt(LLVMInt64Type(), MNH_ADDR, 0), LLVMPointerType(LLVMVoidType(), 0u)))  // mnh
                params |> push(args)
                if cmresEval!=null
                    params |> push(LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(LLVMVoidType(),0u), ""))
                params |> push(get_context_param())
                var func = LLVMGetNamedFunction(g_mod, cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
                var typ = g_fn_types[cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
                var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
                if cmresEval != null
                    setE(expr, cmresEval)
                elif expr.func.result.isVoid
                    setE(expr, ccall)
                else
                    var cres = cast_from_vec4f(expr, expr._type, ccall)
                    setE(expr, cres)

    def override visitExprCall(expr:smart_ptr<ExprCall>) : ExpressionPtr
        make_call(expr, expr.doesNotNeedSp)
        return expr

    def override canVisitCall ( expr:ExprCall? ) : bool
        return !skipCall |> key_exists(expr)

// ExprNullCoalescing
    def override preVisitExprNullCoalescing(expr:smart_ptr<ExprNullCoalescing>): void
        var blk = [[IteBlock
            if_true   = LLVMAppendBasicBlock(ffunc,"nullc_null_at_line_{int(expr.at.line)}_"),
            if_false  = LLVMAppendBasicBlock(ffunc,"nullc_not_null_at_line_{int(expr.at.line)}_"),
            if_end    = LLVMAppendBasicBlock(ffunc,"nullc_end_at_line_{int(expr.at.line)}_"),
            phi_true  = LLVMAppendBasicBlock(ffunc,"nullc_phi_true_at_line_{int(expr.at.line)}_"),
            phi_false = LLVMAppendBasicBlock(ffunc,"nullc_phi_false_at_line_{int(expr.at.line)}_")
        ]]
        ite2blocks[get_expr_ptr(expr)] = blk

    def override preVisitExprNullCoalescingDefault(expr:smart_ptr<ExprNullCoalescing>;defval:ExpressionPtr): void
        var blk = ite2blocks[get_expr_ptr(expr)]
        var sE = getE(expr.subexpr)
        var cond = LLVMBuildIsNotNull(g_builder, sE, "")
        LLVMBuildCondBr(g_builder, cond, blk.if_false, blk.if_true)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_false)
        if expr._type.isRef
            pass
        elif expr._type.isHandle
            unsupported(expr, "null coalescing for handles")
        else
            setE(expr.subexpr, LLVMBuildLoad2(g_builder, type_to_llvm_type(expr._type), sE, ""))
        LLVMBuildBr(g_builder, blk.phi_true)
        LLVMPositionBuilderAtEnd(g_builder, blk.phi_true)
        LLVMBuildBr(g_builder, blk.if_end)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_true)

    def override visitExprNullCoalescing(expr:smart_ptr<ExprNullCoalescing>) : ExpressionPtr
        var blk = ite2blocks[get_expr_ptr(expr)]
        var sE = getE(expr.subexpr)
        var dE = getE(expr.defaultValue)
        if expr._type.isPointer // could be null there, right? TODO: should we try ref
            sE = LLVMBuildPointerCast(g_builder, sE, LLVMPointerType(type_to_llvm_type(expr._type.firstType), 0u), "")
            dE = LLVMBuildPointerCast(g_builder, dE, LLVMPointerType(type_to_llvm_type(expr._type.firstType), 0u), "")
        LLVMBuildBr(g_builder, blk.phi_false)
        LLVMPositionBuilderAtEnd(g_builder, blk.phi_false)
        LLVMBuildBr(g_builder, blk.if_end)
        LLVMPositionBuilderAtEnd(g_builder, blk.if_end)
        var phi = LLVMBuildPhi(g_builder, type_to_llvm_abi_type(expr._type), "null_c")
        LLVMAddIncoming(phi, [[auto sE; dE]], [[auto blk.phi_true; blk.phi_false]])
        setE(expr, phi)
        return expr

// ExprAt
    def check_range ( tidx, maxIdx:LLVMOpaqueValue?; at:LineInfo; message:string )
        if option_no_range_check
            return
        var check_null_ptr = LLVMAppendBasicBlock(ffunc,"check_dim_range")
        var check_true = LLVMAppendBasicBlock(ffunc,"check_true")
        var check_end = LLVMAppendBasicBlock(ffunc,"check_end")
        LLVMBuildBr(g_builder, check_null_ptr)
        LLVMPositionBuilderAtEnd(g_builder,check_null_ptr)
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntUGE, tidx, maxIdx, "range_check")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_true)
        build_exception(message, at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_end)

    def build_table_at ( ttype:Type; tab:LLVMOpaqueValue?; key:LLVMOpaqueValue?; valueTypeSize:int ) : LLVMOpaqueValue?
        var params = [[auto
            tab;                                                        // table ptr
            key;                                                        // key
            LLVMConstInt(LLVMInt32Type(), uint64(valueTypeSize), 0);    // valueTypeSize
            get_context_param()
        ]]
        var tab_fun = LLVMGetNamedFunction(g_mod, FN_JIT_TABLE_AT(ttype))
        if tab_fun==null
            error("table_at {ttype} function not found")
            return LLVMGetUndef(LLVMInt32Type())
        var typ = g_fn_types[FN_JIT_TABLE_AT(ttype)]
        return LLVMBuildCall2(g_builder, typ, tab_fun, params, "")

    def build_table_find ( ttype:Type; tab:LLVMOpaqueValue?; key:LLVMOpaqueValue?; valueTypeSize:int ) : LLVMOpaqueValue?
        var params = [[auto
            tab;                                                        // table ptr
            key;                                                        // key
            LLVMConstInt(LLVMInt32Type(), uint64(valueTypeSize), 0);    // valueTypeSize
            get_context_param()
        ]]
        var tab_fun = LLVMGetNamedFunction(g_mod, FN_JIT_TABLE_FIND(ttype))
        if tab_fun==null
            error("table_find {ttype} function not found")
            return LLVMGetUndef(LLVMInt32Type())
        var typ = g_fn_types[FN_JIT_TABLE_FIND(ttype)]
        return LLVMBuildCall2(g_builder, typ, tab_fun, params, "")

    def build_array_index(var data,tidx:LLVMOpaqueValue?; elemType:TypeDeclPtr; name:string)
        var ptr_idx : LLVMOpaqueValue?
        if elemType.isVectorType && elemType.vectorDim==3
            var data_bytes = LLVMBuildPointerCast(g_builder, data, LLVMPointerType(LLVMInt8Type(), 0u), "")
            var data_idx = LLVMBuildMul(g_builder, tidx, LLVMConstInt(LLVMInt32Type(), 12ul, 0), "")
            ptr_idx = LLVMBuildGEP2(g_builder, LLVMInt8Type(), data_bytes, data_idx, "")
            ptr_idx = LLVMBuildPointerCast(g_builder, ptr_idx, LLVMPointerType(type_to_llvm_type(elemType), 0u), name)
        else
            ptr_idx = LLVMBuildGEP2(g_builder, type_to_llvm_type(elemType), data, tidx, name)
        return ptr_idx

    def override visitExprAt(expr:smart_ptr<ExprAt>) : ExpressionPtr
        var tsrc = getE(expr.subexpr)
        var tidx = getE(expr.index)
        assume subExprT = expr.subexpr._type
        if subExprT.isPointer
            if thisFunc!=null && !thisFunc.flags.unsafeDeref
                check_ptr_zero(tsrc, expr.at)
            var ptr_idx = build_array_index(tsrc, tidx, subExprT.firstType, "ptr_at")
            if expr.atFlags.r2v
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_abi_type(expr._type),
                    ptr_idx, uint(expr._type.alignOf), "at_r2v")
                setE(expr, at_r2v)
            else
                setE(expr, ptr_idx)
        elif subExprT.dim |> length != 0
            var inscope etype <- clone_type(subExprT)
            assume vec = etype.dim
            let maxIndex = vec[vec |> length-1]
            for i in range(1, length(vec))
                vec[i - 1] = vec[i]
            vec |> resize( vec |> length - 1 )
            check_range(tidx, LLVMConstInt(LLVMInt32Type(), uint64(maxIndex), 0), expr.at, "dim index out of range")
            var ptr_idx = build_array_index(tsrc, tidx, etype, "dim_at")
            if expr.atFlags.r2v
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_abi_type(expr._type),
                    ptr_idx, uint(expr._type.alignOf), "dim_at_r2v")
                setE(expr, at_r2v)
            else
                setE(expr, ptr_idx)
        elif subExprT.isGoodArrayType
            var arr = LLVMBuildLoad2(g_builder, type_to_llvm_type(expr.subexpr._type), tsrc, "arr")
            var size = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY SIZE), "array.size")
            check_range(tidx, size, expr.at, "array index out of range")
            var data = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY DATA), "array.data")
            var ptr_idx = build_array_index(data, tidx, subExprT.firstType, "array_at")
            if expr.atFlags.r2v
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type),
                    ptr_idx, uint(expr._type.alignOf), "array_at_r2v")
                setE(expr, at_r2v)
            else
                setE(expr, ptr_idx)
        elif subExprT.isGoodTableType
            var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, LLVMPointerType(LLVMVoidType(),0u), "tab_ptr")
            var keyType : Type
            if subExprT.firstType.isWorkhorseType
                keyType = subExprT.firstType.baseType
            else
                var inscope uvt <- get_underlying_value_type(subExprT.firstType)
                keyType = uvt.baseType
            var at_tab = build_table_at(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
            var tab = LLVMBuildLoad2(g_builder, type_to_llvm_type(expr.subexpr._type), tsrc, "tab")
            var data = LLVMBuildExtractValue(g_builder, tab, uint(JIT_TABLE DATA), "table.data")
            var ptr_idx = LLVMBuildGEP2(g_builder, type_to_llvm_type(expr._type), data, at_tab, "table_at")
            if expr.atFlags.r2v
                var at_r2v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type),
                    ptr_idx, uint(expr._type.alignOf), "array_at_r2v")
                setE(expr, at_r2v)
            else
                setE(expr, ptr_idx)
        elif subExprT.isVectorType
            check_range(tidx, LLVMConstInt(LLVMInt32Type(), uint64(subExprT.vectorDim), 0), expr.at, "vector index out of range")
            if expr.atFlags.r2v
                var lvec = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type),
                    tsrc, uint(subExprT.alignOf), "vector_at")
                var at_r2v = LLVMBuildExtractElement(g_builder, lvec, tidx, "vector_at_r2v")
                setE(expr, at_r2v)
            else
                var ptr_idx = build_array_index(tsrc, tidx, expr._type, "vector_at")
                setE(expr, ptr_idx)
        else
            failed("unsupported visitExprAt type {describe(subExprT)}")
        return expr

// ExprSafeAt
    def override visitExprSafeAt(expr:smart_ptr<ExprSafeAt>) : ExpressionPtr
        var tsrc = getE(expr.subexpr)
        var tidx = getE(expr.index)
        assume subExprT = expr.subexpr._type
        if subExprT.dim |> length != 0 || (subExprT.isPointer && subExprT.firstType.dim |> length != 0)
            var inscope etype <- subExprT.isPointer ? clone_type(subExprT.firstType) : clone_type(subExprT)
            assume vec = etype.dim
            let maxIndex = vec[vec |> length-1]
            for i in range(1, length(vec))
                vec[i - 1] = vec[i]
            vec |> resize( vec |> length - 1 )
            var resType = LLVMPointerType(type_to_llvm_type(etype),0u)
            var if_not_null = subExprT.dim |> length==0 ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(LLVMInt1Type(), 1ul, 0)
            var res = build_select(if_not_null, resType) <|
                var maxIdx = LLVMConstInt(LLVMInt32Type(), uint64(maxIndex), 0)
                var if_range = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntULT, tidx, maxIdx, "dim_range")
                var subRes = build_select(if_range, resType) <|
                    return build_array_index(tsrc, tidx, etype, "dim_at")
                lpipe <|
                    return LLVMConstNull(resType)
                return subRes
            lpipe <|
                return LLVMConstNull(resType)
            setE(expr, res)
        elif subExprT.isGoodArrayType || (subExprT.isPointer && subExprT.firstType.isGoodArrayType)
            var if_not_null = !subExprT.isGoodArrayType ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(LLVMInt1Type(), 1ul, 0)
            var inscope etype := subExprT.isPointer ? subExprT.firstType.firstType : subExprT.firstType
            var resType = LLVMPointerType(type_to_llvm_type(etype),0u)
            var res = build_select(if_not_null, resType) <|
                var inscope arr_ty := expr.subexpr._type.isPointer ? expr.subexpr._type.firstType : expr.subexpr._type
                var arr = LLVMBuildLoad2(g_builder, type_to_llvm_type(arr_ty), tsrc, "arr")
                var size = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY SIZE), "array.size")
                var if_range = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntULT, tidx, size, "array_range")
                var subRes = build_select(if_range, resType) <|
                    var data = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY DATA), "array.data")
                    return build_array_index(data, tidx, etype, "array_at")
                lpipe <|
                    return LLVMConstNull(resType)
                return subRes
            lpipe <|
                return LLVMConstNull(resType)
            setE(expr, res)
        elif subExprT.isGoodTableType || (subExprT.isPointer && subExprT.firstType.isGoodTableType)
            var if_not_null = !subExprT.isGoodTableType ? LLVMBuildIsNotNull(g_builder, tsrc, "") : LLVMConstInt(LLVMInt1Type(), 1ul, 0)
            var inscope etype := subExprT.isPointer ? subExprT.firstType : subExprT
            var resType = LLVMPointerType(type_to_llvm_type(etype.secondType),0u)
            var res = build_select(if_not_null, resType) <|
                var inscope tab_ty := expr.subexpr._type.isPointer ? expr.subexpr._type.firstType : expr.subexpr._type
                var tab = LLVMBuildLoad2(g_builder, type_to_llvm_type(tab_ty), tsrc, "tab")
                var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, LLVMPointerType(LLVMVoidType(),0u), "tab_ptr")
                var keyType : Type
                if etype.firstType.isWorkhorseType
                    keyType = etype.firstType.baseType
                else
                    var inscope uvt <- get_underlying_value_type(etype.firstType)
                    keyType = uvt.baseType
                var at_tab = build_table_find(keyType, tab_ptr, tidx, etype.secondType.sizeOf)
                var if_found = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, at_tab, LLVMConstInt(LLVMInt32Type(), uint64(-1), 0), "table_found")
                var subRes = build_select(if_found, resType) <|
                    var data = LLVMBuildExtractValue(g_builder, tab, uint(JIT_TABLE DATA), "table.data")
                    return LLVMBuildGEP2(g_builder, type_to_llvm_type(etype.secondType), data, at_tab, "table_at")
                lpipe <|
                    return LLVMConstNull(resType)
                return subRes
            lpipe <|
                return LLVMConstNull(resType)
            setE(expr, res)
        else
            setE(expr, LLVMConstNull(type_to_llvm_type(expr._type)))
            failed("unsupported visitExprSafeAt type {describe(subExprT)}")
        return expr

// ExprOp2
    def isReadModifyWriteOp2 ( name:das_string )
        return (name=="+=" || name=="-=" || name=="*=" || name=="/=" || name=="%=" ||
            name=="&=" || name=="|=" || name=="^=" || name=="<<=" || name==">>=" ||
            name=="<<<=" || name==">>>=" || name=="&&=" || name=="||=" || name=="^^=")

    def get_bool_compare_op ( e:ExpressionPtr; op:string )
        return LLVMIntPredicate LLVMIntEQ   if op=="=="
        return LLVMIntPredicate LLVMIntNE   if op=="!="
        failed_E(e, "unsupported bool compare operator {op}")
        return LLVMIntPredicate LLVMIntEQ

    def get_int_compare_op ( e:ExpressionPtr; op:string )
        return LLVMIntPredicate LLVMIntEQ   if op=="=="
        return LLVMIntPredicate LLVMIntNE   if op=="!="
        return LLVMIntPredicate LLVMIntSLT  if op=="<"
        return LLVMIntPredicate LLVMIntSLE  if op=="<="
        return LLVMIntPredicate LLVMIntSGT  if op==">"
        return LLVMIntPredicate LLVMIntSGE  if op==">="
        failed_E(e, "unsupported int compare operator {op}")
        return LLVMIntPredicate LLVMIntEQ

    def get_uint_compare_op ( e:ExpressionPtr; op:string )
        return LLVMIntPredicate LLVMIntEQ   if op=="=="
        return LLVMIntPredicate LLVMIntNE   if op=="!="
        return LLVMIntPredicate LLVMIntULT  if op=="<"
        return LLVMIntPredicate LLVMIntULE  if op=="<="
        return LLVMIntPredicate LLVMIntUGT  if op==">"
        return LLVMIntPredicate LLVMIntUGE  if op==">="
        failed_E(e, "unsupported uint compare operator {op}")
        return LLVMIntPredicate LLVMIntEQ

    def get_float_compare_op ( e:ExpressionPtr; op:string )
        return LLVMRealPredicate LLVMRealOEQ    if op=="=="
        return LLVMRealPredicate LLVMRealUNE    if op=="!="
        return LLVMRealPredicate LLVMRealOLT    if op=="<"
        return LLVMRealPredicate LLVMRealOLE    if op=="<="
        return LLVMRealPredicate LLVMRealOGT    if op==">"
        return LLVMRealPredicate LLVMRealOGE    if op==">="
        failed_E(e, "unsupported float compare operator {op}")
        return LLVMRealPredicate LLVMRealOEQ

    def check_divide_by_0 ( right:LLVMOpaqueValue?; at:LineInfo; opType:TypeDeclPtr )
        var check_div_0 = LLVMAppendBasicBlock(ffunc,"check_div_0")
        var check_true = LLVMAppendBasicBlock(ffunc,"check_true")
        var check_end = LLVMAppendBasicBlock(ffunc,"check_end")
        LLVMBuildBr(g_builder, check_div_0)
        LLVMPositionBuilderAtEnd(g_builder,check_div_0)
        let zero =  LLVMConstInt(base_type_to_llvm_type(opType.baseType), 0ul, 0)
        let cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, right, zero, "cmp_is_zero")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_true)
        build_exception("division by zero", at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_end)

    def isExprIntNZ ( expr:ExpressionPtr )
        return true     if (expr is ExprConstInt) && (expr as ExprConstInt).value!=0
        return true     if (expr is ExprConstUInt) && (expr as ExprConstUInt).value!=0u
        return true     if (expr is ExprConstInt64) && (expr as ExprConstInt64).value!=0l
        return true     if (expr is ExprConstUInt64) && (expr as ExprConstUInt64).value!=0ul
        return false

    def build_str_cmp ( a,b:LLVMOpaqueValue? ) : LLVMOpaqueValue?
        var typ = g_fn_types[FN_JIT_STR_CMP]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod,FN_JIT_STR_CMP), [[auto a;b]], "")

    def build_str_cat ( a,b:LLVMOpaqueValue? ) : LLVMOpaqueValue?
        var typ = g_fn_types[FN_JIT_STR_CAT]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod,FN_JIT_STR_CAT), [[auto a;b;get_context_param()]], "")


    def reduce_bit_vector ( i1v:LLVMOpaqueValue?; bits:int; op:string )
        var result = LLVMBuildExtractElement(g_builder, i1v, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "bit_0")
        for b in range(1,bits)
            var bit = LLVMBuildExtractElement(g_builder, i1v, LLVMConstInt(LLVMInt32Type(), uint64(b), 0), "bit_{b}")
            if op=="=="
                result = LLVMBuildAnd(g_builder, result, bit, "")
            elif op=="!="
                result = LLVMBuildOr(g_builder, result, bit, "")
            else
                error("unsupported bit reduction operator {op}")
        return result

    def visitExprOp2_Func(var expr:smart_ptr<ExprOp2>)
        expr.arguments |> resize(2)
        expr.arguments[0] := expr.left
        expr.arguments[1] := expr.right
        make_call(expr, false)

    def visitExprOp2_NonMonade(var expr:smart_ptr<ExprOp2>)
        var left = getE(expr.left)
        var right = getE(expr.right)
        assume opType = expr.right._type
        assume shiftType = expr.left._type
        var r2v_left = isReadModifyWriteOp2(expr.op) ? LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.left._type),
                                                            left, uint(expr.left._type.alignOf), "") : null
        // note: we need to handle vector vs scalar operations here
        // the way we do it, is we expand scalar up to vector type
        if expr.op=="*" || expr.op=="*=" || expr.op=="/" || expr.op=="/="
            if expr.left._type.baseType != expr.right._type.baseType // vec * scalar || scalar * vec, and so on
                if opType.isVectorType
                    assert(expr.op=="*" || expr.op=="/")    // scalar * vec || scalar / vec only
                    left = expand_scalar(g_builder, left, opType)
                else
                    right = expand_scalar(g_builder, right, expr.left._type)
        // note: we need to handle shift operations
        // the way we do it, is we expand scalar up to vector type
        if expr.op=="<<" || expr.op==">>" || expr.op==">>>" || expr.op=="<<<" || expr.op=="<<=" || expr.op==">>=" || expr.op==">>>=" || expr.op=="<<<="
            right = build_broadcast_vector(g_builder, shiftType, right)
        if expr.op=="+"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildAdd(g_builder, left, right, ""))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildFAdd(g_builder, left, right, ""))
            elif opType.isString
                setE(expr, build_str_cat(left, right))
            else
                failed_E(expr, "unsupported + type {describe(opType)}")
        elif expr.op=="+="
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildAdd(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFAdd(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            elif opType.isString
                setE(expr, LLVMBuildStoreAligned(g_builder, build_str_cat(r2v_left, right), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported += type {describe(opType)}")
        elif expr.op=="-"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildSub(g_builder, left, right, ""))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildFSub(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported - type {describe(opType)}")
        elif expr.op=="-="
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSub(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFSub(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported -= type {describe(opType)}")
        elif expr.op=="*"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildMul(g_builder, left, right, ""))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildFMul(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported * type {describe(opType)}")
        elif expr.op=="*="
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildMul(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFMul(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported *= type {describe(opType)}")
        elif expr.op=="/" || expr.op=="%" || expr.op=="/=" || expr.op=="%="
            // for integer division we check division by zero
            if opType.isInteger && expr.left._type.isInteger
                if !isExprIntNZ(expr.right)
                    check_divide_by_0(right, expr.at, opType)
            if expr.op=="/"
                if opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tInt)
                    setE(expr, LLVMBuildSDiv(g_builder, left, right, ""))
                elif opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tUInt)
                    setE(expr, LLVMBuildUDiv(g_builder, left, right, ""))
                elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                    setE(expr, LLVMBuildFDiv(g_builder, left, right, ""))
                else
                    failed_E(expr, "unsupported /= type {describe(opType)}")
            elif expr.op=="/="
                if opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tInt)
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSDiv(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                elif opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tUInt)
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildUDiv(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFDiv(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                else
                    failed_E(expr, "unsupported /= type {describe(opType)}")
            elif expr.op=="%"
                if opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tInt)
                    if opType.baseType==Type tInt
                        setE(expr, LLVMBuildSRemInt32(g_builder, left, right, ""))
                    else
                        setE(expr, LLVMBuildSRem(g_builder, left, right, ""))
                elif opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tUInt)
                    if opType.baseType==Type tUInt
                        setE(expr, LLVMBuildURemUInt32(g_builder, left, right, ""))
                    else
                        setE(expr, LLVMBuildURem(g_builder, left, right, ""))
                elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                    setE(expr, LLVMBuildFRem(g_builder, left, right, ""))
                else
                    failed_E(expr, "unsupported % type {describe(opType)}")
            elif expr.op=="%="
                if opType.isSignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tInt)
                    if opType.baseType==Type tInt
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSRemInt32(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                    else
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildSRem(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                elif opType.isUnsignedInteger || (opType.isVectorType && opType.vectorBaseType==Type tUInt)
                    if opType.baseType==Type tUInt
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildURemUInt32(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                    else
                        setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildURem(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                    setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildFRem(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
                else
                    failed_E(expr, "unsupported %= type {describe(opType)}")
            else
                failed_E(expr, "unsupported division type operator {expr.op}")
        elif expr.op=="<" || expr.op==">" || expr.op=="<=" || expr.op==">=" || expr.op=="==" || expr.op=="!="
            var opR : LLVMOpaqueValue?
            if expr.left._type.isSignedInteger || (expr.left._type.isVectorType && (expr.left._type.vectorBaseType==Type tInt || expr.left._type.vectorBaseType==Type tInt64)) || expr.left._type.isEnum
                opR = LLVMBuildICmp(g_builder, get_int_compare_op(expr,string(expr.op)), left, right, "")
                if expr.left._type.isVectorType
                    opR = reduce_bit_vector(opR, expr.left._type.vectorDim, string(expr.op))
            elif expr.left._type.isUnsignedInteger || (expr.left._type.isVectorType && (expr.left._type.vectorBaseType==Type tUInt || expr.left._type.vectorBaseType==Type tUInt64))
                opR = LLVMBuildICmp(g_builder, get_uint_compare_op(expr,string(expr.op)), left, right, "")
                if expr.left._type.isVectorType
                    opR = reduce_bit_vector(opR, expr.left._type.vectorDim, string(expr.op))
            elif expr.left._type.isFloatOrDouble || (expr.left._type.isVectorType && expr.left._type.vectorBaseType==Type tFloat)
                opR = LLVMBuildFCmp(g_builder, get_float_compare_op(expr,string(expr.op)), left, right, "")
                if expr.left._type.isVectorType
                    opR = reduce_bit_vector(opR, expr.left._type.vectorDim, string(expr.op))
            elif expr.left._type.baseType == Type tBool
                opR = LLVMBuildICmp(g_builder, get_bool_compare_op(expr,string(expr.op)), left, right, "")
            elif expr.left._type.baseType == Type tPointer || expr.left._type.baseType == Type tLambda
                var lptr = LLVMBuildPointerCast(g_builder, left, LLVMPointerType(LLVMInt8Type(), 0u), "")
                var rptr = LLVMBuildPointerCast(g_builder, right, LLVMPointerType(LLVMInt8Type(), 0u), "")
                opR = LLVMBuildICmp(g_builder, get_uint_compare_op(expr,string(expr.op)), lptr, rptr, "")
            elif expr.left._type.baseType == Type tString
                var cmpR = build_str_cmp(left, right)
                opR = LLVMBuildICmp(g_builder, get_int_compare_op(expr,string(expr.op)), cmpR, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            elif expr.left._type.baseType == Type tFunction
                var lptr = (expr.left is ExprConstPtr) ? left : LLVMBuildExtractValue(g_builder, left, uint(JIT_FUNCTION SIM_FUNCTION), "SIMFUNCTION")
                var rptr = (expr.right is ExprConstPtr) ? right : LLVMBuildExtractValue(g_builder, right, uint(JIT_FUNCTION SIM_FUNCTION), "SIMFUNCTION")
                lptr = LLVMBuildPointerCast(g_builder, lptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
                rptr = LLVMBuildPointerCast(g_builder, rptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
                opR = LLVMBuildICmp(g_builder, get_uint_compare_op(expr,string(expr.op)), lptr, rptr, "")
            if opR == null
                opR = LLVMGetUndef(LLVMInt8Type())
                failed_E(expr, "unsupported compare type {describe(expr.left._type)} {expr.op} {describe(expr.right._type)}")
            setE(expr, opR)
        elif expr.op=="<<"
            if shiftType.isInteger || (shiftType.isVectorType && (shiftType.vectorBaseType==Type tInt || shiftType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildShl(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported << type {describe(shiftType)}")
        elif expr.op=="<<="
            if shiftType.isInteger || (shiftType.isVectorType && (shiftType.vectorBaseType==Type tInt || shiftType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildShl(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported <<= type {describe(shiftType)}")
        elif expr.op==">>"
            if shiftType.isSignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType==Type tInt)
                setE(expr, LLVMBuildAShr(g_builder, left, right, ""))
            elif shiftType.isUnsignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType==Type tUInt)
                setE(expr, LLVMBuildLShr(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported >> type {describe(shiftType)}")
        elif expr.op==">>="
            if shiftType.isSignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType==Type tInt)
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildAShr(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            elif shiftType.isUnsignedInteger || (shiftType.isVectorType && shiftType.vectorBaseType==Type tUInt)
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildLShr(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported >>= type {describe(shiftType)}")
        elif expr.op==">>>" || expr.op=="<<<"
            if shiftType.isInteger
                var fshr_name = expr.op==">>>" ? "llvm.fshr.i32" : "llvm.fshl.i32"
                if shiftType.baseType==Type tInt64 || shiftType.baseType==Type tUInt64
                    fshr_name = expr.op==">>>" ? "llvm.fshr.i64" : "llvm.fshl.i64"
                var args <- [{auto left; left; right}]
                var argTypes <- [{auto base_type_to_llvm_type(shiftType.baseType)}]
                var id = LLVMLookupIntrinsicID(fshr_name)
                var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
                if decl==null
                    failed_E(expr, "failed to get intrinsic {fshr_name}")
                else
                    var typ = LLVMFunctionType(argTypes[0], [{auto argTypes[0]; argTypes[0]; argTypes[0]}])
                    setE(expr, LLVMBuildCall2(g_builder, typ, decl, args, string(expr.op)))
            else
                failed_E(expr, "unsupported >>> type {opType}")
        elif expr.op==">>>=" || expr.op=="<<<="
            if shiftType.isInteger
                var fshr_name = expr.op==">>>=" ? "llvm.fshr.i32" : "llvm.fshl.i32"
                if shiftType.baseType==Type tInt64 || shiftType.baseType==Type tUInt64
                    fshr_name = expr.op==">>>=" ? "llvm.fshr.i64" : "llvm.fshl.i64"
                var args <- [{auto r2v_left; r2v_left; right}]
                var argTypes <- [{auto base_type_to_llvm_type(shiftType.baseType)}]
                var id = LLVMLookupIntrinsicID(fshr_name)
                var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
                if decl==null
                    failed_E(expr, "failed to get intrinsic {fshr_name}")
                else
                    var typ = LLVMFunctionType(argTypes[0], [{auto argTypes[0]; argTypes[0]; argTypes[0]}])
                    var cll = LLVMBuildCall2(g_builder, typ, decl, args, string(expr.op))
                    setE(expr, LLVMBuildStoreAligned(g_builder, cll, left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported >>> type {opType}")
        elif expr.op=="&"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildAnd(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported & type {opType}")
        elif expr.op=="&="
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildAnd(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported &= type {opType}")
        elif expr.op=="|"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildOr(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported | type {opType}")
        elif expr.op=="|="
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildOr(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported |= type {opType}")
        elif expr.op=="^"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildXor(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported ^ type {opType}")
        elif expr.op=="^="
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildXor(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported ^= type {opType}")
        elif expr.op=="^^"
            if opType.isBool
                setE(expr, LLVMBuildXor(g_builder, left, right, ""))
            else
                failed_E(expr, "unsupported ^^ type {opType}")
        elif expr.op=="^^="
            if opType.isBool
                setE(expr, LLVMBuildStoreAligned(g_builder, LLVMBuildXor(g_builder, r2v_left, right, ""), left, uint(expr.left._type.alignOf)))
            else
                failed_E(expr, "unsupported ^^= type {opType}")
        else
            failed_E(expr, "unsupported binary operator {expr.op}")

    def isExprOp2_Func(expr:smart_ptr<ExprOp2>)
        if !expr.func.flags.builtIn
            return true
        if expr.left._type.isHandle || expr.right._type.isHandle
            return true
        return false

    def override preVisitExprOp2Right(expr:smart_ptr<ExprOp2>;right:ExpressionPtr): void
        if isExprOp2_Func(expr)
            return
        if expr.op=="&&" || expr.op=="&&=" || expr.op=="||" || expr.op=="||="
            var monad_true = LLVMAppendBasicBlock(ffunc,"monad_true_at_{int(expr.at.line)}_")
            var monad_false = LLVMAppendBasicBlock(ffunc,"monad_true_at_{int(expr.at.line)}_")
            var monad_result, monad_cond : LLVMOpaqueValue?
            if expr.op=="&&" || expr.op=="||"
                at_function_entry <|
                    monad_result = LLVMBuildAlloca(g_builder, LLVMInt1Type(), "{expr.op}_at_{int(expr.at.line)}_")
                monad_cond = getE(expr.left)
            else
                monad_result = getE(expr.left)
                monad_cond = LLVMBuildLoad2(g_builder, LLVMInt1Type(), monad_result, "")
            monad2block[get_ptr(ExpressionPtr(expr))] = monad_false
            setE(expr, monad_result)
            if expr.op=="&&" || expr.op=="||"
                LLVMBuildStore(g_builder, monad_cond, monad_result)
            if expr.op=="&&" || expr.op=="&&="
                LLVMBuildCondBr(g_builder, monad_cond, monad_true, monad_false)
            else
                LLVMBuildCondBr(g_builder, monad_cond, monad_false, monad_true)
            LLVMPositionBuilderAtEnd(g_builder,monad_true)

    def override visitExprOp2(var expr:smart_ptr<ExprOp2>) : ExpressionPtr
        if isExprOp2_Func(expr)
            visitExprOp2_Func(expr)
        elif expr.op=="&&" || expr.op=="&&=" || expr.op=="||" || expr.op=="||="
            var monad_false = monad2block[get_ptr(ExpressionPtr(expr))]
            LLVMBuildStore(g_builder, getE(expr.right), getE(expr))
            LLVMBuildBr(g_builder, monad_false)
            LLVMPositionBuilderAtEnd(g_builder,monad_false)
            if expr.op=="&&" || expr.op=="||"
                setE(expr, LLVMBuildLoad2(g_builder, LLVMInt1Type(), getE(expr), ""))
        else
            visitExprOp2_NonMonade(expr)
        return expr

// ExprOp3
    def override preVisitExprOp3(expr:smart_ptr<ExprOp3>): void
        var res : LLVMOpaqueValue?
        var before_if = LLVMAppendBasicBlock(ffunc,"op3_cond_at_{int(expr.at.line)}_")
        LLVMBuildBr(g_builder, before_if)
        LLVMPositionBuilderAtEnd(g_builder,before_if)
        var blk = [[IteBlock
            if_true  = LLVMAppendBasicBlock(ffunc,"op3_true_at_line_{int(expr.at.line)}_"),
            if_false = LLVMAppendBasicBlock(ffunc,"op3_false_at_line_{int(expr.at.line)}_"),
            if_end   = LLVMAppendBasicBlock(ffunc,"op3_end_at_line_{int(expr.at.line)}_"),
            phi_true = LLVMAppendBasicBlock(ffunc,"phi_true_at_line_{int(expr.at.line)}_"),
            phi_false= LLVMAppendBasicBlock(ffunc,"phi_false_at_line_{int(expr.at.line)}_")
        ]]
        ite2blocks[get_expr_ptr(expr)] = blk

    def override preVisitExprOp3Left(expr:smart_ptr<ExprOp3>;left:ExpressionPtr): void
        var blk = ite2blocks[get_expr_ptr(expr)]                        // we start if_true block
        LLVMBuildCondBr(g_builder, getE(expr.subexpr), blk.if_true, blk.if_false)
        LLVMPositionBuilderAtEnd(g_builder,blk.if_true)

    def override preVisitExprOp3Right(expr:smart_ptr<ExprOp3>;right:ExpressionPtr): void
        if expr.left is ExprConstPtr
            let null_ptr = LLVMConstPointerNull(type_to_llvm_abi_type(expr._type))
            setE(expr.left, null_ptr)
        elif expr.left._type.isPointer
            // we need to cast pointer to the type of the right expression
            let cleft = LLVMBuildPointerCast(g_builder, getE(expr.left), type_to_llvm_type(expr._type), "")
            setE(expr.left, cleft)
        var blk = ite2blocks[get_expr_ptr(expr)]
        LLVMBuildBr(g_builder, blk.phi_true)
        LLVMPositionBuilderAtEnd(g_builder,blk.phi_true)
        LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)   // after the end of if_true block we jump to if_end
        LLVMPositionBuilderAtEnd(g_builder,blk.if_false)

    def override visitExprOp3(expr:smart_ptr<ExprOp3>) : ExpressionPtr
        if expr.right is ExprConstPtr
            let null_ptr = LLVMConstPointerNull(type_to_llvm_abi_type(expr._type))
            setE(expr.right, null_ptr)
        elif expr.right._type.isPointer
            // we need to cast pointer to the type of the left expression
            let cright = LLVMBuildPointerCast(g_builder, getE(expr.right), type_to_llvm_type(expr._type), "")
            setE(expr.right, cright)
        var blk = ite2blocks[get_expr_ptr(expr)]
        LLVMBuildBr(g_builder, blk.phi_false)
        LLVMPositionBuilderAtEnd(g_builder,blk.phi_false)
        LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)
        LLVMPositionBuilderAtEnd(g_builder,blk.if_end)
        var phi = LLVMBuildPhi(g_builder, type_to_llvm_abi_type(expr._type), "cond")
        LLVMAddIncoming(phi, [[auto getE(expr.left); getE(expr.right)]], [[auto blk.phi_true; blk.phi_false]])
        setE(expr, phi)
        return expr

// ExprCopy
    def build_copy ( tdst:LLVMOpaqueValue?; right:ExpressionPtr )
        var tsrc = getE(right)
        var ralign = uint(right._type.alignOf)
        if right._type.isRef
            if right._type.isWorkhorseType
                var temp = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(right._type), tsrc, ralign, "")
                if right._type.isPointer
                    var compDst = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMTypeOf(temp),0u), "")
                    LLVMBuildStoreAligned(g_builder, temp, compDst, ralign)
                else
                    LLVMBuildStoreAligned(g_builder, temp, tdst, ralign)
            else
                let rsize = LLVMConstInt(LLVMInt32Type(), uint64(right._type.sizeOf), 0)
                LLVMBuildMemCpy(g_builder, tdst, ralign, tsrc, ralign, rsize)
        else
            if right is ExprConstPtr
                let null_ptr = LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(),0u))
                LLVMBuildStore2Aligned(g_builder, LLVMPointerType(LLVMVoidType(),0u), null_ptr, tdst, ralign)
            elif right._type.isPointer
                var compDst = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMTypeOf(tsrc),0u), "")
                LLVMBuildStoreAligned(g_builder, tsrc, compDst, ralign)
            else
                LLVMBuildStoreAligned(g_builder, tsrc, tdst, ralign)

    def override preVisitExprCopyRight(expr:smart_ptr<ExprCopy>;right:ExpressionPtr): void
        var tdst = getE(expr.left)
        if make_call_to_cmres(expr.right, tdst)
            pass
        elif expr.copy_flags.takeOverRightStack
            assert(isMakeLocal(expr.right))
            var mk_ptr = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMInt8Type(),0u), "")
            setE(expr.right, mk_ptr)

    def override visitExprCopy(expr:smart_ptr<ExprCopy>) : ExpressionPtr
        if isCall2CMRES(expr.right)
            pass    // already a call to cmres, no need to copy
        else
            var tdst = getE(expr.left)
            build_copy(tdst, expr.right)
        return expr

// ExprMove

    def build_move ( tdst:LLVMOpaqueValue?; right:ExpressionPtr; needTest:bool )
        var tsrc = getE(right)
        var ralign = uint(right._type.alignOf)
        if right._type.isRef
            /*
            if right._type.isWorkhorseType
                var temp = LLVMBuildLoadDataAligned(g_builder, tsrc, ralign, "")
                LLVMBuildStoreAligned(g_builder, temp, tdst, ralign)
            */
            if needTest
                var lptr = LLVMBuildPtrToInt(g_builder, tsrc, LLVMIntPtrType(), "")
                var rptr = LLVMBuildPtrToInt(g_builder, tdst, LLVMIntPtrType(), "")
                var cmpR = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, lptr, rptr, "")
                var move_start = LLVMAppendBasicBlock(ffunc, "move_start")
                var move_end = LLVMAppendBasicBlock(ffunc, "move_end")
                LLVMBuildCondBr(g_builder, cmpR, move_end, move_start)
                LLVMPositionBuilderAtEnd(g_builder,move_start)
                let rsize = LLVMConstInt(LLVMInt32Type(), uint64(right._type.sizeOf), 0)
                LLVMBuildMemCpy(g_builder, tdst, ralign, tsrc, ralign, rsize)
                LLVMBuildMemSet(g_builder, tsrc, LLVMConstInt(LLVMInt8Type(), 0ul, 0), rsize, ralign)
                LLVMBuildBr(g_builder, move_end)
                LLVMPositionBuilderAtEnd(g_builder,move_end)
            else
                let rsize = LLVMConstInt(LLVMInt32Type(), uint64(right._type.sizeOf), 0)
                LLVMBuildMemCpy(g_builder, tdst, ralign, tsrc, ralign, rsize)
                LLVMBuildMemSet(g_builder, tsrc, LLVMConstInt(LLVMInt8Type(), 0ul, 0), rsize, ralign)
        else
            if right is ExprConstPtr
                let null_ptr = LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(),0u))
                LLVMBuildStore2Aligned(g_builder, LLVMPointerType(LLVMVoidType(),0u), null_ptr, tdst, ralign)
            elif right._type.isPointer
                var compDst = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMTypeOf(tsrc),0u), "")
                LLVMBuildStoreAligned(g_builder, tsrc, compDst, ralign)
            else
                LLVMBuildStoreAligned(g_builder, tsrc, tdst, ralign)

    def override preVisitExprMoveRight(expr:smart_ptr<ExprMove>;right:ExpressionPtr): void
        var tdst = getE(expr.left)
        if make_call_to_cmres(expr.right, tdst)
            pass
        elif expr.move_flags.takeOverRightStack
            assert(isMakeLocal(expr.right))
            var mk_ptr = LLVMBuildPointerCast(g_builder, tdst, LLVMPointerType(LLVMInt8Type(),0u), "")
            setE(expr.right, mk_ptr)

    def override visitExprMove(expr:smart_ptr<ExprMove>) : ExpressionPtr
        if isCall2CMRES(expr.right)
            pass    // already a call to cmres, no need to copy
        else
            var tdst = getE(expr.left)
            build_move(tdst, expr.right, true)
        return expr

// ExprWhile
    def override preVisitExprWhile(expr:smart_ptr<ExprWhile>) : void
        var lblk = [[LoopBlock
            loop_start = LLVMAppendBasicBlock(ffunc, "while_start"),
            loop_body = LLVMAppendBasicBlock(ffunc, "while_body"),
            loop_end = LLVMAppendBasicBlock(ffunc, "while_end")
        ]]
        loop_stack |> push(lblk)
        LLVMBuildBr(g_builder, lblk.loop_start)
        LLVMPositionBuilderAtEnd(g_builder,lblk.loop_start)

    def override preVisitExprWhileBody(expr:smart_ptr<ExprWhile>;right:ExpressionPtr) : void
        var lblk = loop_stack |> back()
        LLVMBuildCondBr(g_builder, getE(expr.cond), lblk.loop_body, lblk.loop_end)
        LLVMPositionBuilderAtEnd(g_builder,lblk.loop_body)

    def override visitExprWhile(expr:smart_ptr<ExprWhile>) : ExpressionPtr
        var lblk = loop_stack |> back()
        loop_stack |> pop()
        if !current_block_terminates()
            LLVMBuildBr(g_builder, lblk.loop_start)
        LLVMPositionBuilderAtEnd(g_builder,lblk.loop_end)
        if expr.body is ExprBlock
            let body = expr.body as ExprBlock
            if length(body.finalList) != 0
                LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(),0u)), callBlock)
                LLVMBuildBr(g_builder, finallyBlocks[get_ptr(body)])
                visit_finally(body, adapter)
                jump_to_next_finally(body)
        return expr

// ExprTryCatch
    def override preVisitExprTryCatch(expr:smart_ptr<ExprTryCatch>) : void
        unsupported(expr, "try-catch")

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
    def override preVisitExprIfThenElse(expr:smart_ptr<ExprIfThenElse>) : void
        var before_if = LLVMAppendBasicBlock(ffunc,"if_cond_at_{int(expr.at.line)}_")
        LLVMBuildBr(g_builder, before_if)
        LLVMPositionBuilderAtEnd(g_builder,before_if)
        var blk = [[IteBlock
            if_true  = LLVMAppendBasicBlock(ffunc,"if_true_at_line_{int(expr.at.line)}_"),
            if_false = LLVMAppendBasicBlock(ffunc,"if_false_at_line_{int(expr.at.line)}_"),
            if_end   = LLVMAppendBasicBlock(ffunc,"if_end_at_line_{int(expr.at.line)}_")
        ]]
        ite2blocks[get_expr_ptr(expr)] = blk

    def override preVisitExprIfThenElseIfBlock(expr:smart_ptr<ExprIfThenElse>;ifBlock:ExpressionPtr) : void
        var blk = ite2blocks[get_expr_ptr(expr)]
        LLVMBuildCondBr(g_builder, getE(expr.cond), blk.if_true, blk.if_false)
        LLVMPositionBuilderAtEnd(g_builder,blk.if_true)

    def before_else(expr:smart_ptr<ExprIfThenElse>) : void
        var blk & = unsafe(ite2blocks[get_expr_ptr(expr)])
        if !current_block_terminates()
            LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)
        else
            blk.if_true_terminates = true
        LLVMPositionBuilderAtEnd(g_builder,blk.if_false)

    def override preVisitExprIfThenElseElseBlock(expr:smart_ptr<ExprIfThenElse>;elseBlock:ExpressionPtr) : void
        before_else(expr)

    def override visitExprIfThenElse(expr:smart_ptr<ExprIfThenElse>) : ExpressionPtr
        var blk & = unsafe(ite2blocks[get_expr_ptr(expr)])
        if expr.if_false == null
            before_else(expr)
        if !current_block_terminates()
            LLVMBuildBr(g_builder, ite2blocks[get_expr_ptr(expr)].if_end)
        else
            blk.if_false_terminates = true
        LLVMPositionBuilderAtEnd(g_builder,blk.if_end)
        if blk.if_true_terminates && blk.if_false_terminates
            LLVMBuildUnreachable(g_builder)
        return expr

// ExprFor
    def build_array_lock ( array_ptr:LLVMOpaqueValue? )
        var params = [[auto
            LLVMBuildPointerCast(g_builder, array_ptr, LLVMPointerType(LLVMVoidType(),0u), "");              // array
            get_context_param()
        ]]
        var typ = g_fn_types[FN_JIT_ARRAY_LOCK]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ARRAY_LOCK), params, "")

    def build_array_unlock ( array_ptr:LLVMOpaqueValue? )
        var params = [[auto
            LLVMBuildPointerCast(g_builder, array_ptr, LLVMPointerType(LLVMVoidType(),0u), "");              // array
            get_context_param()
        ]]
        var typ = g_fn_types[FN_JIT_ARRAY_UNLOCK]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ARRAY_UNLOCK), params, "")

    def build_iter_first ( at:LineInfo; iter_ptr,data_ptr:LLVMOpaqueValue? )
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, LLVMPointerType(LLVMVoidType(),0u), "")
        var params = [[auto
            iter_ptr;               // sequence *
            vdata_ptr;              // data *
            get_context_param();    // context
            get_line_info_ptr(at)   // at
        ]]
        var typ = g_fn_types[FN_JIT_ITERATOR_FIRST]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_FIRST), params, "")

    def build_iter_next ( at:LineInfo; iter_ptr,data_ptr:LLVMOpaqueValue? )
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, LLVMPointerType(LLVMVoidType(),0u), "")
        var params = [[auto
            iter_ptr;               // sequence *
            vdata_ptr;              // data *
            get_context_param();    // context
            get_line_info_ptr(at)   // at
        ]]
        var typ = g_fn_types[FN_JIT_ITERATOR_NEXT]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_NEXT), params, "")

    def build_iter_iterate ( iter_ptr,data_ptr:LLVMOpaqueValue? )
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, LLVMPointerType(LLVMVoidType(),0u), "")
        var params = [[auto
            iter_ptr;           // sequence *
            vdata_ptr;          // data *
            get_context_param() // context
        ]]
        var typ = g_fn_types[FN_JIT_ITERATOR_ITERATE]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_ITERATE), params, "")

    def build_iter_delete ( iter_ptr:LLVMOpaqueValue? )
        var params = [[auto
            iter_ptr;           // sequence *
            get_context_param() // context
        ]]
        var typ = g_fn_types[FN_JIT_ITERATOR_DELETE]
        return LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_DELETE), params, "")

    def build_iter_close ( iter_ptr,data_ptr:LLVMOpaqueValue? )
        let vdata_ptr = LLVMBuildPointerCast(g_builder, data_ptr, LLVMPointerType(LLVMVoidType(),0u), "")
        var params = [[auto
            iter_ptr;           // sequence *
            vdata_ptr;          // data *
            get_context_param() // context
        ]]
        var typ = g_fn_types[FN_JIT_ITERATOR_CLOSE]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_ITERATOR_CLOSE), params, "")

    def is_count_or_ucount ( expr : ExpressionPtr )
        if !(expr is ExprCall)
            return false
        var cf = expr as ExprCall
        return cf.func.flags.builtIn && cf.func._module.name=="$" && (cf.func.name=="count" || cf.func.name=="ucount")

    def override preVisitExprFor(expr:smart_ptr<ExprFor>) : void
        var lblk = [[LoopBlock
            loop_start = LLVMAppendBasicBlock(ffunc, "for_start"),
            loop_continue = LLVMAppendBasicBlock(ffunc, "for_continue"),
            loop_end = LLVMAppendBasicBlock(ffunc, "for_end")
        ]]
        at_function_entry <|
            lblk.need_loop = LLVMBuildAlloca(g_builder, LLVMInt1Type(), "need_loop")
        loop_stack |> push(lblk)
        for ssrc in expr.sources
            if is_count_or_ucount(ssrc)
                skipCall |> insert(ssrc as ExprCall)
        LLVMBuildStore(g_builder, LLVMConstInt(LLVMInt1Type(), 1ul, 0), lblk.need_loop) // need loop, yes

    def override preVisitExprForVariable(expr:smart_ptr<ExprFor>;svar:VariablePtr;last:bool) : void
        // add variable to be looked up
        var vtype = type_to_llvm_type(svar._type)
        var vptrtype = get_type_pointer(svar._type)
        if svar._type.isRef
            vtype = vptrtype
            vptrtype = LLVMPointerType(vptrtype,0u)
        var v_ptr : LLVMOpaqueValue?
        at_function_entry <|
            v_ptr = LLVMBuildAlloca(g_builder, vtype, "for_loop_variable_{svar.name}")
            LLVMSetAlignment(v_ptr, 16u)
            v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, vptrtype, "for_loop_variable_{svar.name}")
            setV(svar, v_ptr)
        // initialize its memory with 0 (should we?)
        var vsize = svar._type.isRef ?  typeinfo(sizeof type<void?>) : svar._type.sizeOf
        LLVMBuildMemSet(g_builder, v_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(vsize), 0), 16u)

    def override preVisitExprForBody(expr:smart_ptr<ExprFor>) : void
        /*
            for ( int t=0; t!=totalCount; ++t ) {
                sources[t]->isOpen = true;  // what is this?
                needLoop = sources[t]->first(context, pi[t]) && needLoop;
                if ( context.stopFlags ) goto loopend;
            }
        LOOP_START:
        */
        var lblk = loop_stack |> back()
        for svar,ssrc in expr.iteratorVariables,expr.sources
            if ssrc._type.isRange           // range()->first()
                var srange = getE(ssrc)
                var sfrom = LLVMBuildExtractElement(g_builder, srange, LLVMConstInt(LLVMInt32Type(),0ul,0), "range_from_{svar.name}")
                var sto = LLVMBuildExtractElement(g_builder, srange, LLVMConstInt(LLVMInt32Type(),1ul,0), "range_to_{svar.name}")
                var cmp_from_to = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntSLT, sfrom, sto, "")
                var okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_not_empty")
                var not_okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_empty")
                LLVMBuildCondBr(g_builder, cmp_from_to, okay, not_okay)
                LLVMPositionBuilderAtEnd(g_builder,not_okay)
                LLVMBuildStore(g_builder, LLVMConstInt(LLVMInt1Type(), 0ul, 0), lblk.need_loop) // need loop, no
                LLVMBuildBr(g_builder,okay)
                LLVMPositionBuilderAtEnd(g_builder,okay)
                range2[get_ptr(ssrc)] = sto
                LLVMBuildStore(g_builder, sfrom, getV(svar))
            elif ssrc._type.dim |> length != 0  // []->first()
                var inscope etype <- clone_type(ssrc._type)
                etype.dim |> pop()
                var element_type = type_to_llvm_type(svar._type)
                var sdim = getE(ssrc)
                var s0 = LLVMBuildGEP2(g_builder, element_type, sdim, LLVMConstInt(LLVMInt32Type(),0ul,0), "dim_0")
                LLVMBuildStore(g_builder, s0, getV(svar))
                let dimSize = ssrc._type.dim[ssrc._type.dim |> length-1]
                var tail = LLVMConstInt(LLVMInt32Type(),uint64(dimSize-1),0)
                var sto = build_array_index(sdim, tail, etype, "last_element")
                sto = LLVMBuildPtrToInt(g_builder, sto, LLVMIntPtrType(), "dim_end")
                range2[get_ptr(ssrc)] = sto
            elif ssrc._type.isGoodArrayType     // {}->first()
                var arr = LLVMBuildLoad2(g_builder, type_to_llvm_type(ssrc._type), getE(ssrc), "array")
                var size = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY SIZE), "array.size")
                var cmp_from_to = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, size, LLVMConstInt(LLVMInt32Type(),0ul,0), "")
                var okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_not_empty")
                var not_okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_empty")
                LLVMBuildCondBr(g_builder, cmp_from_to, okay, not_okay)
                LLVMPositionBuilderAtEnd(g_builder,not_okay)
                LLVMBuildStore(g_builder, LLVMConstInt(LLVMInt1Type(), 0ul, 0), lblk.need_loop) // need loop, no
                LLVMBuildBr(g_builder,okay)
                LLVMPositionBuilderAtEnd(g_builder,okay)
                build_array_lock(getE(ssrc))
                var data = LLVMBuildExtractValue(g_builder, arr, uint(JIT_ARRAY DATA), "array.data")
                LLVMBuildStore(g_builder, data, getV(svar))
                var tail = LLVMBuildSub(g_builder, size, LLVMConstInt(LLVMInt32Type(),1ul,0), "")
                var sto = build_array_index(data, tail, ssrc._type.firstType, "last_element")
                sto = LLVMBuildPtrToInt(g_builder, sto, LLVMIntPtrType(), "array_end")
                range2[get_ptr(ssrc)] = sto
            elif ssrc._type.isIterator
                if is_count_or_ucount(ssrc)
                    var ccount = ssrc as ExprCall
                    visit(ccount.arguments[0], adapter)
                    visit(ccount.arguments[1], adapter)
                    LLVMBuildStore(g_builder, getE(ccount.arguments[0]), getV(svar))
                else
                    var seq = LLVMBuildLoad2(g_builder, type_to_llvm_type(ssrc._type), getE(ssrc), "")
                    var piter = LLVMBuildExtractValue(g_builder, seq, uint(JIT_SEQUENCE ITERATOR), "")
                    var cmp_iter = LLVMBuildIsNull(g_builder, piter, "")
                    var okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_not_empty")
                    var not_okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_empty")
                    LLVMBuildCondBr(g_builder, cmp_iter, not_okay, okay)
                    LLVMPositionBuilderAtEnd(g_builder,not_okay)
                    LLVMBuildStore(g_builder, LLVMConstInt(LLVMInt1Type(), 0ul, 0), lblk.need_loop) // need loop, no
                    LLVMBuildBr(g_builder,okay)
                    LLVMPositionBuilderAtEnd(g_builder,okay)
                    // fun ->first
                    okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_first_okay")
                    not_okay = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_first_failed")
                    var rcond = build_iter_first(ssrc.at, getE(ssrc), getV(svar))
                    LLVMBuildCondBr(g_builder, rcond, okay, not_okay)
                    LLVMPositionBuilderAtEnd(g_builder,not_okay)
                    LLVMBuildStore(g_builder, LLVMConstInt(LLVMInt1Type(), 0ul, 0), lblk.need_loop) // need loop, no
                    LLVMBuildBr(g_builder,okay)
                    LLVMPositionBuilderAtEnd(g_builder,okay)
            else
                failed_E(ssrc, "unsupported loop source {describe(ssrc._type)}")
        var needLoop = LLVMBuildLoad2(g_builder, LLVMInt1Type(), lblk.need_loop, "")
        LLVMBuildCondBr(g_builder, needLoop, lblk.loop_start, lblk.loop_end)
        LLVMPositionBuilderAtEnd(g_builder,lblk.loop_start)

    def override visitExprFor(expr:smart_ptr<ExprFor>) : ExpressionPtr
        var lblk = loop_stack |> back()
        loop_stack |> pop()
        /*
        LOOP_CONTINUE:
            for ( int t=0; t!=totalCount; ++t ){
                if ( !sources[t]->next(context, pi[t]) ) goto loopend;
                if ( context.stopFlags ) goto loopend;
            }
        LOOP_END:
        */
        if !current_block_terminates()
            LLVMBuildBr(g_builder, lblk.loop_continue)                           // LOOP CONTINUE
        LLVMPositionBuilderAtEnd(g_builder,lblk.loop_continue)
        for svar,ssrc in expr.iteratorVariables,expr.sources
            if ssrc._type.isRange           // range()->next()
                var isRange64 = ssrc._type.baseType==Type tRange64 || ssrc._type.baseType==Type tURange64
                var svar_v = LLVMBuildLoad2(g_builder, isRange64 ? LLVMInt64Type() : LLVMInt32Type(), getV(svar), "")
                svar_v = LLVMBuildAdd(g_builder, svar_v, LLVMConstInt(isRange64 ? LLVMInt64Type() : LLVMInt32Type(), 1ul, 0), "")
                LLVMBuildStore(g_builder, svar_v, getV(svar))
                var sto = range2[get_ptr(ssrc)]
                var rcond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, svar_v, sto, "")
                var nextOk = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, rcond, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder,nextOk)
            elif ssrc._type.dim |> length != 0 // []->next()
                var inscope etype <- clone_type(ssrc._type)
                etype.dim |> pop()
                var svar_v = LLVMBuildLoad2(g_builder, LLVMPointerType(type_to_llvm_type(svar._type), 0u), getV(svar), "")
                var svar_i = build_array_index(svar_v, LLVMConstInt(LLVMInt32Type(),1ul,0), etype, "next_element")
                LLVMBuildStore(g_builder, svar_i, getV(svar))
                var sto = range2[get_ptr(ssrc)]
                svar_i = LLVMBuildPtrToInt(g_builder, svar_v, LLVMIntPtrType(), "")
                var rcond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, svar_i, sto, "")
                var nextOk = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, rcond, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder,nextOk)
            elif ssrc._type.isGoodArrayType // {}->next()
                var svar_v = LLVMBuildLoad2(g_builder, LLVMPointerType(type_to_llvm_type(svar._type), 0u), getV(svar), "")
                var svar_i = build_array_index(svar_v, LLVMConstInt(LLVMInt32Type(),1ul,0), ssrc._type.firstType, "next_element")
                LLVMBuildStore(g_builder, svar_i, getV(svar))
                var sto = range2[get_ptr(ssrc)]
                svar_i = LLVMBuildPtrToInt(g_builder, svar_v, LLVMIntPtrType(), "")
                var rcond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, svar_i, sto, "")
                var nextOk = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_next_ok")
                LLVMBuildCondBr(g_builder, rcond, lblk.loop_end, nextOk)
                LLVMPositionBuilderAtEnd(g_builder,nextOk)
            elif ssrc._type.isIterator   // iterator->next()
                if is_count_or_ucount(ssrc)
                    var ccount = ssrc as ExprCall
                    var vvar = LLVMBuildLoad2(g_builder, type_to_llvm_type(svar._type), getV(svar), "")
                    var vadd = LLVMBuildAdd(g_builder, vvar, getE(ccount.arguments[1]), "")
                    LLVMBuildStore(g_builder, vadd, getV(svar))
                else
                    var rcond = build_iter_next(ssrc.at, getE(ssrc), getV(svar))
                    var nextOk = LLVMAppendBasicBlock(ffunc, "for_{svar.name}_next_ok")
                    LLVMBuildCondBr(g_builder, rcond, nextOk, lblk.loop_end)
                    LLVMPositionBuilderAtEnd(g_builder,nextOk)
            else
                failed_E(ssrc, "unsupported loop source {describe(ssrc._type)}")
        LLVMBuildBr(g_builder, lblk.loop_start)
        /*
        LOOP_END:
            evalFinal(context);
            for ( int t=0; t!=totalCount; ++t ) {
                sources[t]->close(context, pi[t]);
            }
        */
        LLVMPositionBuilderAtEnd(g_builder,lblk.loop_end)                   // LOOP END
        // in for loop closing iterators is included in the finally section
        let body = expr.body as ExprBlock
        var fblk = finallyBlocks[get_ptr(body)]
        LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(),0u)), callBlock)
        LLVMBuildBr(g_builder, fblk)
        LLVMPositionBuilderAtEnd(g_builder, fblk)
        // close all iterators
        let len = length(expr.iteratorVariables)
        for i in range(len)
            let ri = len - i - 1
            let svar & = unsafe(expr.iteratorVariables[ri])
            let ssrc & = unsafe(expr.sources[ri])
            if ssrc._type.isGoodArrayType
                build_array_unlock(getE(ssrc))
            elif ssrc._type.isIterator
                if is_count_or_ucount(ssrc)
                    // we do nothing for count
                    pass
                else
                    build_iter_close(getE(ssrc), getV(svar))
        // now, final block if there is anythign there
        if length(body.finalList) != 0
            visit_finally(body, adapter)
        jump_to_next_finally(body)
        return expr

// ExprMakeVariant
    def override preVisitExprMakeVariant(expr:smart_ptr<ExprMakeVariant>) : void
        var mkv_ptr : LLVMOpaqueValue?
        if expr.makeFlags.useCMRES
            mkv_ptr = get_cmres_param()
            mkv_ptr = LLVMBuildPointerCast(g_builder, mkv_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            if expr.extraOffset != 0u
                mkv_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mkv_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "")
        else
            mkv_ptr = tryE(expr) <|
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <|
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mkv_local_sp_{expr.stackTop}")
                    LLVMSetAlignment(s_ptr, 16u)
                    s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(LLVMInt8Type(),0u), "mkv_local_sp_{expr.stackTop}")
                return s_ptr
            if expr.extraOffset != 0u
                mkv_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mkv_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "mkv_local_sp_{expr.stackTop}_eo_{expr.extraOffset}")
        let stride = expr.makeType.stride
        if stride!=0 && length(expr.variants)==0
            let total = int(expr.variants |> length)
            let bytes = max(total,1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mkv_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(bytes), 0), align)
        setE(expr, mkv_ptr)

    def override preVisitExprMakeVariantField(expr:smart_ptr<ExprMakeVariant>;index:int;decl:MakeFieldDeclPtr;last:bool) : void
        var field_index = find_argument_index(expr.makeType, string(decl.name))
        if field_index==-1
            failed_E(expr, "variant field {decl.name} not found")
            return
        let field_offset = get_variant_field_offset(expr.makeType,field_index)
        let stride = expr.makeType.stride
        let offset =  index*stride
        var v_ptr = getE(expr)
        verify(v_ptr!=null && LLVMIsUndef(v_ptr)==0,"make variant field, but no pointer")
        let index_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "")
        LLVMBuildStore2(g_builder, LLVMInt32Type(), LLVMConstInt(LLVMInt32Type(), uint64(field_index), 0), index_ptr)
        let field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset + field_offset), 0), "")
        if decl.value |> isMakeLocal
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(decl.value))
            val.extraOffset = uint(offset + field_offset) // + expr.extraOffset
            if expr.makeFlags.useCMRES
                pass
            elif expr.makeFlags.useStackRef
                setE(decl.value, v_ptr)
            else
                setE(decl.value, v_ptr)
        elif make_call_to_cmres ( decl.value, field_ptr )
            pass

    def override visitExprMakeVariantField(expr:smart_ptr<ExprMakeVariant>;index:int;decl:MakeFieldDeclPtr;last:bool) : MakeFieldDeclPtr
        var field_index = find_argument_index(expr.makeType, string(decl.name))
        if field_index==-1
            failed_E(expr, "variant field {decl.name} not found")
            return decl
        let field_offset = get_variant_field_offset(expr.makeType,field_index)
        let stride = expr.makeType.stride
        let offset =  index*stride
        var v_ptr = getE(expr)
        v_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset+field_offset), 0), "[{index}].{decl.name}")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr.makeType.argTypes[field_index]), "")
        if decl.value |> isMakeLocal
            pass // do nothign. we already taken care of makeLocal in preVisitExprMakeVariantField
        elif isCall2CMRES(decl.value)
            pass // do nothing. we already handle this in preVisitExprMakeVariantField
        else
            if decl.flags.moveSemantics
                build_move(v_ptr, decl.value, false)   // TODO: verify
            else
                build_copy(v_ptr, decl.value)
        return decl

    def override visitExprMakeVariant(expr:smart_ptr<ExprMakeVariant>) : ExpressionPtr
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr

// ExprMakeStruct
    def callConstructor ( expr:smart_ptr<ExprMakeStruct>; cmresEval:LLVMOpaqueValue? )
        assume expr_func = expr.constructor
        if expr_func.moreFlags.requestJit
            var params : array<LLVMOpaqueValue?>
            params |> push(get_context_param())
            params |> push(LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(LLVMVoidType(),0u), ""))
            let fmna = get_mangled_name(FunctionPtr(expr_func))
            var func = LLVMGetNamedFunction(g_mod, "{fmna} implementation")
            var typ = g_fn_types[("{fmna} implementation")]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
            setE(expr, cmresEval)
        else
            var MNH_ADDR = get_function_addr(expr_func)
            if MNH_ADDR==0ul
                failed_E(expr, "missing function pointer for {expr_func.name}")
            var params : array<LLVMOpaqueValue?>
            params |> push(LLVMConstIntToPtr(LLVMConstInt(LLVMInt64Type(), MNH_ADDR, 0), LLVMPointerType(LLVMVoidType(), 0u)))  // mnh
            params |> push(LLVMConstPointerNull(LLVMPointerType(LLVMFloat4Type(), 0u))) // args
            params |> push(LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(LLVMVoidType(),0u), ""))
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "")
            setE(expr, cmresEval)

    def override preVisitExprMakeStruct(expr:smart_ptr<ExprMakeStruct>) : void
        var mks_ptr : LLVMOpaqueValue?
        if expr.makeFlags.useCMRES
            mks_ptr = get_cmres_param()
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            if expr.extraOffset != 0u
                mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "")
        else
            mks_ptr = tryE(expr) <|
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <|
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mks_local_sp_{expr.stackTop}")
                    LLVMSetAlignment(s_ptr, 16u)
                    // s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(LLVMInt8Type(),0u), "mks_local_sp_{expr.stackTop}")
                return s_ptr
            if expr.extraOffset != 0u
                mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "mks_local_sp_{expr.stackTop}_eo_{expr.extraOffset}")
        let stride = expr.makeType.stride
        if !expr.makeFlags.doesNotNeedInit && !expr.makeFlags.initAllFields && stride!=0
            let total = int(expr.structs |> length)
            let bytes = max(total,1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mks_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(bytes), 0), align)
        setE(expr, mks_ptr)

    def override preVisitExprMakeStructIndex(expr:smart_ptr<ExprMakeStruct>;index:int;last:bool) : void
        if expr.constructor != null
            var mks_ptr = getE(expr)
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            let stride = expr.makeType.stride
            let offset = index*stride
            mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "")
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(type_to_llvm_type(expr.makeType),0u), "")
            callConstructor(expr, mks_ptr)

    def override visitMakeStructureBlock(expr:smart_ptr<ExprMakeStruct>;blk:ExpressionPtr) : ExpressionPtr
        if expr._block != null
            var params : array<LLVMOpaqueValue?>

            var block_ptr = getE(expr._block)
            block_ptr = LLVMBuildPointerCast(g_builder, block_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            params |> push(block_ptr)
        // put put the result of ExprMakeStruct as the sole argument to the block
            var mks_ptr = getE(expr)
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")

            var args : LLVMOpaqueValue?
            at_function_entry <|
                args = LLVMBuildAlloca(g_builder, LLVMArrayType(LLVMFloat4Type(), 1u), "args")
                LLVMSetAlignment(args, 16u)
                args = LLVMBuildPointerCast(g_builder, args, LLVMPointerType(LLVMFloat4Type(), 0u), "call_args_ptr")
            var arg_ptr = LLVMBuildGEP2(g_builder, LLVMFloat4Type(), args, LLVMConstInt(LLVMInt32Type(), 0 |> uint64(), 0), "call_arg__ptr")
            var arg_val = mks_ptr
            LLVMBuildStore2(g_builder, LLVMPointerType(LLVMVoidType(), 0u), arg_val, arg_ptr)

            params |> push(args)
            params |> push(get_context_param())

            var func = LLVMGetNamedFunction(g_mod, FN_JIT_INVOKE_BLOCK)
            var typ = g_fn_types[FN_JIT_INVOKE_BLOCK]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
        return blk

    def override preVisitExprMakeStructField(expr:smart_ptr<ExprMakeStruct>;index:int;decl:MakeFieldDeclPtr;last:bool) : void
        var field_offset : int
        if expr.makeType.isHandle
            var pann = expr.makeType.annotation
            var ann <- unsafe(reinterpret<smart_ptr<TypeAnnotation>> pann)
            field_offset = int(get_handled_type_field_offset(ann, string(decl.name)))
        else
            var field = find_structure_field(expr.makeType.structType, string(decl.name))
            if field==null
                failed_E(expr, "field {decl.name} not found")
                return
            field_offset = field.offset
        let stride = expr.makeType.stride
        let offset =  index*stride + field_offset
        var v_ptr = getE(expr)
        verify(v_ptr!=null && LLVMIsUndef(v_ptr)==0,"make struct field, but no pointer")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
        let field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "")
        if decl.value |> isMakeLocal
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(decl.value))
            val.extraOffset = uint(offset) // + expr.extraOffset
            if expr.makeFlags.useCMRES
                pass
            elif expr.makeFlags.useStackRef
                setE(decl.value, v_ptr)
            else
                setE(decl.value, v_ptr)
        elif make_call_to_cmres ( decl.value, field_ptr )
            pass

    def override visitExprMakeStructField(expr:smart_ptr<ExprMakeStruct>;index:int;decl:MakeFieldDeclPtr;last:bool) : MakeFieldDeclPtr
        var field_offset : int
        var inscope field_type : TypeDeclPtr
        if expr.makeType.isHandle
            var pann = expr.makeType.annotation
            var ann <- unsafe(reinterpret<smart_ptr<TypeAnnotation>> pann)
            field_offset = int(get_handled_type_field_offset(ann, string(decl.name)))
            field_type |> move_new <| get_handled_type_field_type_declaration(ann, string(decl.name), false)
        else
            var field = find_structure_field(expr.makeType.structType, string(decl.name))
            if field==null
                failed_E(expr, "field {decl.name} not found")
                return decl
            field_offset = field.offset
            field_type := field._type
        let stride = expr.makeType.stride
        let offset =  index*stride + field_offset
        var v_ptr = getE(expr)
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
        v_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "[{index}].{decl.name}")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(field_type), "")
        if decl.value |> isMakeLocal
            pass // do nothign. we already taken care of makeLocal in preVisitExprMakeStructField
        elif isCall2CMRES(decl.value)
            pass // do nothing. we already handle this in preVisitExprMakeStructField
        else
            if decl.flags.moveSemantics
                build_move(v_ptr, decl.value, false)   // TODO: verify
            else
                build_copy(v_ptr, decl.value)
        return decl

    def override visitExprMakeStruct(expr:smart_ptr<ExprMakeStruct>) : ExpressionPtr
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr

// ExprMakeArray
    def override preVisitExprMakeArray(expr:smart_ptr<ExprMakeArray>) : void
        var mks_ptr : LLVMOpaqueValue?
        if expr.makeFlags.useCMRES
            mks_ptr = get_cmres_param()
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            if expr.extraOffset != 0u
                mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "")
        else
            mks_ptr = tryE(expr) <|
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <|
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mka_local_sp_{expr.stackTop}_at_{int(expr.at.line)}_")
                    LLVMSetAlignment(s_ptr, 16u)
                    s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(LLVMInt8Type(),0u), "mka_local_sp_{expr.stackTop}_at_{int(expr.at.line)}_")
                return s_ptr
            if expr.extraOffset != 0u
                mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "mka_local_sp_{expr.stackTop}_eo_{expr.extraOffset}_at_{int(expr.at.line)}_")
        let stride = expr.makeType.stride
        if !expr.makeFlags.doesNotNeedInit && !expr.makeFlags.initAllFields && stride!=0
            let total = int(expr.values |> length)
            let bytes = max(total,1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mks_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(bytes), 0), align)
        setE(expr, mks_ptr)

    def override preVisitExprMakeArrayIndex(expr:smart_ptr<ExprMakeArray>;index:int;init:ExpressionPtr;last:bool) : void
        let stride = expr.makeType.stride
        let offset =  index*stride
        var v_ptr = getE(expr)
        verify(v_ptr!=null && LLVMIsUndef(v_ptr)==0,"make array index, but no pointer")
        let record_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "")
        if init |> isMakeLocal
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(init))
            val.extraOffset = uint(offset) // + expr.extraOffset
            if expr.makeFlags.useCMRES
                pass
            elif expr.makeFlags.useStackRef
                setE(init, v_ptr)
            else
                setE(init, v_ptr)
        elif make_call_to_cmres ( init, record_ptr )
            pass

    def override visitExprMakeArrayIndex(expr:smart_ptr<ExprMakeArray>;index:int;init:ExpressionPtr;last:bool) : ExpressionPtr
        let stride = expr.makeType.stride
        let offset =  index*stride
        var v_ptr = getE(expr)
        v_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "[{index}]")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr.recordType), "")
        if init |> isMakeLocal
            pass // do nothign. we already taken care of makeLocal in preVisitExprMakeArrayField
        elif isCall2CMRES(init)
            pass // do nothing. we already handle this in preVisitExprMakeArrayField
        else
            if expr.recordType.canCopy
                build_copy(v_ptr, init)
            else
                build_move(v_ptr, init, false)
        return init

    def override visitExprMakeArray(expr:smart_ptr<ExprMakeArray>) : ExpressionPtr
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr

// ExprMakeTuple
    def override preVisitExprMakeTuple(expr:smart_ptr<ExprMakeTuple>) : void
        var mks_ptr : LLVMOpaqueValue?
        if expr.makeFlags.useCMRES
            mks_ptr = get_cmres_param()
            mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            if expr.extraOffset != 0u
                mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "")
        else
            mks_ptr = tryE(expr) <|
                var vtype = type_to_llvm_type(expr._type)
                var s_ptr : LLVMOpaqueValue?
                at_function_entry <|
                    s_ptr = LLVMBuildAlloca(g_builder, vtype, "mkt_local_sp_{expr.stackTop}")
                    LLVMSetAlignment(s_ptr, 16u)
                    s_ptr = LLVMBuildPointerCast(g_builder, s_ptr, LLVMPointerType(LLVMInt8Type(),0u), "mkt_local_sp_{expr.stackTop}")
                return s_ptr
            if expr.extraOffset != 0u
                mks_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), mks_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.extraOffset), 0), "mkt_local_sp_{expr.stackTop}_eo_{expr.extraOffset}")
        let stride = expr.makeType.stride
        if !expr.makeFlags.doesNotNeedInit && !expr.makeFlags.initAllFields && stride!=0
            let total = int(expr.values |> length)
            let bytes = max(total,1) * stride
            let align = uint(expr.makeType.alignOf)
            LLVMBuildMemSet(g_builder, mks_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(bytes), 0), align)
        setE(expr, mks_ptr)

    def override preVisitExprMakeTupleIndex(expr:smart_ptr<ExprMakeTuple>;index:int;init:ExpressionPtr;last:bool) : void
        let offset = get_tuple_field_offset(expr.makeType,index)
        var v_ptr = getE(expr)
        verify(v_ptr!=null && LLVMIsUndef(v_ptr)==0,"make struct field, but no pointer")
        let field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "")
        if init |> isMakeLocal
            var val = unsafe(reinterpret<ExprMakeLocal?> get_ptr(init))
            val.extraOffset = uint(offset) // + expr.extraOffset
            if expr.makeFlags.useCMRES
                pass
            elif expr.makeFlags.useStackRef
                setE(init, v_ptr)
            else
                setE(init, v_ptr)
        elif make_call_to_cmres ( init, field_ptr )
            pass

    def tuple_field_name ( t:TypeDeclPtr; index:int )
        if !t.isTuple
            failed_T(t, "not a tuple {describe(t)}")
            return "!!not_a_tuple!!"
        return t.argNames |> length!=0 ? "{t.argNames[index]}" : "{describe(t.argTypes[index])}"

    def override visitExprMakeTupleIndex(expr:smart_ptr<ExprMakeTuple>;index:int;init:ExpressionPtr;last:bool) : ExpressionPtr
        let offset = get_tuple_field_offset(expr.makeType,index)
        var v_ptr = getE(expr)
        let argName = "_{tuple_field_name(expr.makeType,index)}"
        v_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), v_ptr, LLVMConstInt(LLVMInt32Type(), uint64(offset), 0), "tuple_{index}{argName}")
        v_ptr = LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr.makeType.argTypes[index]), "")
        if init |> isMakeLocal
            pass // do nothign. we already taken care of makeLocal in preVisitExprMakeStructField
        elif isCall2CMRES(init)
            pass // do nothing. we already handle this in preVisitExprMakeStructField
        else
            if expr.makeType.canCopy
                build_copy(v_ptr, init)
            else
                build_move(v_ptr, init, false)
        return init

    def override visitExprMakeTuple(expr:smart_ptr<ExprMakeTuple>) : ExpressionPtr
        var v_ptr = getE(expr)
        setE(expr, LLVMBuildPointerCast(g_builder, v_ptr, get_type_pointer(expr._type), ""))
        return expr

// ExprArrayComprehension
    def override preVisitExprArrayComprehension(expr:smart_ptr<ExprArrayComprehension>) : void
        unsupported(expr, "array comprehension")

// ExprPtr2Ref
    def override visitExprPtr2Ref(expr:smart_ptr<ExprPtr2Ref>) : ExpressionPtr
        var tsrc = getE(expr.subexpr)
        if thisFunc!=null && !thisFunc.flags.unsafeDeref
            check_ptr_zero(tsrc, expr.at)
        setE(expr, tsrc)
        return expr

// ExprLabel
    def override preVisitExprLabel(expr:smart_ptr<ExprLabel>) : void
        var lab = labels[expr.labelName]
        if lab==null
            failed_E(expr, "label '{expr.labelName}' not found")
            return
        if !current_block_terminates()
            LLVMBuildBr(g_builder, lab)
        append_label_block(lab)
        LLVMPositionBuilderAtEnd(g_builder,lab)

// ExprGoto
    def override visitExprGoto(expr:smart_ptr<ExprGoto>) : ExpressionPtr
        if expr.subexpr!=null
            var labI = getE(expr.subexpr)
            for kv in sortedLabels
                var nextLab = LLVMAppendBasicBlock(ffunc,"not_label_{kv.index}")
                let isL = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, labI, LLVMConstInt(LLVMInt32Type(), uint64(kv.index), 0), "")
                LLVMBuildCondBr(g_builder, isL, kv.blk, nextLab)
                LLVMPositionBuilderAtEnd(g_builder,nextLab)
            build_exception("invalid label index", expr.at)
        else
            var lab = labels[expr.labelName]
            if lab==null
                failed_E(expr, "goto '{expr.labelName}', label not found")
            else
                LLVMBuildBr(g_builder, lab)
        return expr

// ExprRef2Value
    def override visitExprRef2Value(expr:smart_ptr<ExprRef2Value>) : ExpressionPtr
        setE(expr) <| LLVMBuildLoadData2Aligned(g_builder,
            type_to_llvm_type(expr.subexpr._type), getE(expr.subexpr), uint(expr._type.alignOf), "r2v")
        return expr

// ExprRef2Ptr
    def override visitExprRef2Ptr(expr:smart_ptr<ExprRef2Ptr>) : ExpressionPtr
        var tsrc = getE(expr.subexpr)
        setE(expr, tsrc)
        return expr

// ExprAddr
    def override preVisitExprAddr(expr:smart_ptr<ExprAddr>) : void
        var MNH_ADDR = get_function_addr(expr.func)
        if MNH_ADDR==0ul
            failed_E(expr, "missing function pointer for {expr.func.name}")
        var rfun = LLVMGetUndef(g_t_function)
        rfun = LLVMBuildInsertValue(g_builder, rfun, LLVMConstInt(LLVMInt64Type(), MNH_ADDR, 0), 0u, "")
        setE(expr, rfun)

// ExprAssert
    def override visitExprAssert(expr:smart_ptr<ExprAssert>) : ExpressionPtr
        var cond = getE(expr.arguments[0])
        var message = "assert failed"
        if length(expr.arguments) > 1
            message = string((expr.arguments[1] as ExprConstString).value)
        var params = [[auto
            get_string_constant_ptr(g_builder,message + " at {describe(expr.at)}");
            get_context_param();
            get_line_info_ptr(expr.at)
        ]]
        var check_assert = LLVMAppendBasicBlock(ffunc,"assert")
        var assert_ok = LLVMAppendBasicBlock(ffunc,"assert_ok")
        var assert_failed = LLVMAppendBasicBlock(ffunc,"assert_failed")
        LLVMBuildBr(g_builder, check_assert)
        LLVMPositionBuilderAtEnd(g_builder,check_assert)
        LLVMBuildCondBr(g_builder, cond, assert_ok, assert_failed)
        LLVMPositionBuilderAtEnd(g_builder,assert_failed)
        var typ = g_fn_types[FN_JIT_EXCEPTION]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_EXCEPTION), params, "")
        LLVMBuildBr(g_builder, assert_ok)
        LLVMPositionBuilderAtEnd(g_builder,assert_ok)
        return expr

    def get_llvm_type_for_typeinfo () : LLVMTypeRef
        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        return LLVMStructType() <| [[auto
            void_ptr;        // union StructInfo *, EnumInfo *, TypeAnnotation *
            void_ptr;        // TypeInfo * firstType
            void_ptr;        // TypeInfo * secondType
            void_ptr;        // TypeInfo ** argTypes
            void_ptr;        // const char ** argNames
            void_ptr;        // uint32_t * dim
            LLVMInt64Type(); // uint64_t hash
            LLVMInt32Type(); // Type type (enum Type : uint32_t)
            LLVMInt32Type(); // uint32_t enum flags: ref, refType, canCopy, etc
            LLVMInt32Type(); // uint32_t size
            LLVMInt32Type(); // uint32_t argCount
            LLVMInt32Type()  // uint32_t dimSize
        ]]

    def get_llvm_type_for_varinto () : LLVMTypeRef
        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        return LLVMStructType() <| [[auto
        // Typeinfo fields
            void_ptr;        // union StructInfo *, EnumInfo *, TypeAnnotation *
            void_ptr;        // TypeInfo * firstType
            void_ptr;        // TypeInfo * secondType
            void_ptr;        // TypeInfo ** argTypes
            void_ptr;        // const char ** argNames
            void_ptr;        // uint32_t * dim
            LLVMInt64Type(); // uint64_t hash
            LLVMInt32Type(); // Type type (enum Type : uint32_t)
            LLVMInt32Type(); // uint32_t enum flags: ref, refType, canCopy, etc
            LLVMInt32Type(); // uint32_t size
            LLVMInt32Type(); // uint32_t argCount
            LLVMInt32Type(); // uint32_t dimSize

        // Own fields
            LLVMFloat4Type(); // union { vec4f value; char * sValue }
            void_ptr;         // const char * name
            void_ptr;         // void * annotation_arguments
            LLVMInt32Type();  // uint32_t offset
            LLVMInt32Type()   // uint32_t nextGcField
        ]]

    def get_llvm_type_for_structinfo () : LLVMTypeRef
        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        return LLVMStructType() <| [[auto
            void_ptr;        // const char* name
            void_ptr;        // const char* module_name
            void_ptr;        // VarInfo **  fields
            void_ptr;        // void *      annotation_list

            LLVMInt64Type(); // uint64_t hash
            LLVMInt64Type(); // uint64_t init_mnh

            LLVMInt32Type(); // uint32_t flags
            LLVMInt32Type(); // uint32_t count
            LLVMInt32Type(); // uint32_t size
            LLVMInt32Type(); // uint32_t firstGcField
        ]]

    def get_llvm_type_for_enumvalueinfo () : LLVMTypeRef
        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        return LLVMStructType() <| [[auto
            void_ptr;        // const char* name
            LLVMInt64Type(); // uint64_t value
        ]]

    def get_llvm_type_for_enuminfo ( ) : LLVMTypeRef
        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        return LLVMStructType() <| [[auto
            void_ptr;        // const char* name
            void_ptr;        // const char* module_name
            void_ptr;        // EnumValueInfo **  fields
            LLVMInt32Type(); // uint32_t count
            LLVMInt64Type(); // uint64_t hash
        ]]

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    def make_initializer ( ty: LLVMTypeRef; values: array<LLVMValueRef> )
        var init = LLVMGetUndef(ty)
        for i in urange(values |> length())
            init = g_builder |> LLVMBuildInsertValue(init, values[i], i, "")
        return init

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    def create_enumvalueinfo_global_array ( ei: EnumValueInfo??; len: int )
        var array_init : array<LLVMValueRef>
        let ty = get_llvm_type_for_enumvalueinfo()
        for i in range(len)
            unsafe
                let evi& = ei[i]
                var init_fields = [[auto
                    get_string_constant_ptr(g_builder, evi.name);
                    LLVMConstInt(LLVMInt64Type(), evi.value  |> uint64(), 0)
                ]]
                array_init |> push <| LLVMConstNamedStruct(ty, unsafe(addr(init_fields[0])), 2u)

        let my_struct_array_type = LLVMArrayType(ty, len |> uint())
        return LLVMConstArray(my_struct_array_type, unsafe(addr(array_init[0])), len |> uint())

    def get_typeinfo_fields ( ti: TypeInfo? ) : array<LLVMValueRef>
        var init_values : array<LLVMValueRef>
        rec_create_type_info_global( ti, init_values )
        init_values |> push <| LLVMConstInt(LLVMInt64Type(), ti.hash       |> uint64(), 0)
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), ti.basicType  |> uint64(), 0)
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), ti.flags      |> uint64(), 0)
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), ti.size       |> uint64(), 0)
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), ti.argCount   |> uint64(), 0)
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), ti.dimSize    |> uint64(), 0)
        return <- init_values

    def get_value_for_varinfo ( vi : VarInfo? ) : LLVMValueRef
        // get_llvm_type_for_typeinfo(); // VarInfo : TypeInfo
        // LLVMFloat4Type(); // union { vec4f value; char * sValue }
        // void_ptr;         // const char * name
        // void_ptr;         // void * annotation_arguments
        // LLVMInt32Type();  // uint32_t offset
        // LLVMInt32Type()   // uint32_t nextGcField
        var init_values <- get_typeinfo_fields(unsafe(reinterpret<TypeInfo?> vi))

        var x = LLVMConstReal(LLVMFloatType(), double(0f))
        var y = LLVMConstReal(LLVMFloatType(), double(0f))
        var z = LLVMConstReal(LLVMFloatType(), double(0f))
        var w = LLVMConstReal(LLVMFloatType(), double(0f))
        init_values |> push <| LLVMBuildFloat4_xyzw(g_builder, x, y, z, w, "float4")
        init_values |> push <| get_string_constant_ptr(g_builder, vi.name)
        init_values |> push <| LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(), 0u))
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), vi.offset       |> uint64(), 0)
        init_values |> push <| LLVMConstInt(LLVMInt32Type(), vi.nextGcField  |> uint64(), 0)
        let ty = get_llvm_type_for_varinto()
        return LLVMConstNamedStruct(ty, unsafe(addr(init_values[0])), init_values |> length |> uint())

    def create_struct_fields_global_array ( ei: VarInfo??; len: int )
        // not cached, probably don't need it
        var array_init : array<LLVMValueRef>
        let ty = get_llvm_type_for_varinto()
        for i in range(len)
            let varinfo = g_mod |> LLVMAddGlobal(ty, "varinfo")
            varinfo |> LLVMSetInitializer <| get_value_for_varinfo(unsafe(ei[i]))
            array_init |> push <| varinfo
        let my_struct_array_type = LLVMArrayType(LLVMPointerType(ty, 0u), len |> uint())
        let varinfo_array = LLVMAddGlobal(g_mod, my_struct_array_type, "varinfo_ptr_array")
        let initializer = LLVMConstArray(LLVMPointerType(ty, 0u), unsafe(addr(array_init[0])), len |> uint())
        LLVMSetInitializer(varinfo_array) <| initializer

        return g_builder |> LLVMBuildPointerCast(varinfo_array, LLVMPointerType(LLVMVoidType(), 0u), "mycast")

    def create_enuminfo_global ( ei: EnumInfo? )
        if enuminfo_cache |> key_exists <| ei
            return enuminfo_cache[ei]
        var ty = get_llvm_type_for_enuminfo()
        var struct_info_global = LLVMAddGlobal(g_mod, ty, "" /* no name */)
        enuminfo_cache[ei] = struct_info_global
        var init_values <- [{auto
            get_string_constant_ptr(g_builder, ei.name);
            get_string_constant_ptr(g_builder, ei.module_name);
            create_enumvalueinfo_global_array(ei.fields, ei.count |> int());
            LLVMConstInt(LLVMInt32Type(), ei.count |> uint64(), 0);
            LLVMConstInt(LLVMInt64Type(), ei.hash     |> uint64(), 0)
        }]
        struct_info_global |> LLVMSetInitializer() <| make_initializer(ty, init_values)
        return struct_info_global

    def create_struct_global ( si: StructInfo? )
        if structinfo_cache |> key_exists <| si
            return structinfo_cache[si]
        var ty = get_llvm_type_for_structinfo()
        var struct_info_global = LLVMAddGlobal(g_mod, ty, "" /* no name */)
        structinfo_cache[si] = struct_info_global
        var void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        var init_values <- [{auto
            get_string_constant_ptr(g_builder, si.name);
            get_string_constant_ptr(g_builder, si.module_name);
            create_struct_fields_global_array(si.fields, si.count |> int());
            LLVMConstPointerNull(void_ptr); // NOTE: annotations are not used right now

            LLVMConstInt(LLVMInt64Type(), si.hash     |> uint64(), 0);
            LLVMConstInt(LLVMInt64Type(), si.init_mnh |> uint64(), 0);

            LLVMConstInt(LLVMInt32Type(), si.flags |> uint64(), 0);
            LLVMConstInt(LLVMInt32Type(), si.count |> uint64(), 0);
            LLVMConstInt(LLVMInt32Type(), si.size  |> uint64(), 0);
            LLVMConstInt(LLVMInt32Type(), si.firstGcField |> uint64(), 0);
        }]
        struct_info_global |> LLVMSetInitializer() <| make_initializer(ty, init_values)
        return struct_info_global

    def create_struct_global_pointer ( si: StructInfo? )
        var tinfo_global = create_struct_global(si)
        g_engine |> LLVMAddGlobalMapping(tinfo_global, si)
        return tinfo_global

    def create_enum_global_pointer ( si: EnumInfo? )
        var tinfo_global = create_enuminfo_global(si)
        // var tinfo_global_ptr = tinfo_global |> get_first_element_addr()
        g_engine |> LLVMAddGlobalMapping(tinfo_global, si)
        return tinfo_global

    def create_argtypes_array ( argtypes: TypeInfo??; len: int )
        var array_init : array<LLVMValueRef>
        let ty = get_llvm_type_for_typeinfo()
        for i in range(len)
            unsafe
                let evi = argtypes[i] |> create_type_info_global()
                array_init |> push <| evi
        let tinfo_array_ty = LLVMArrayType(ty, len |> uint())
        var global = LLVMAddGlobal(g_mod, tinfo_array_ty, "argtypes")
        global |> LLVMSetInitializer <| LLVMConstArray(tinfo_array_ty, unsafe(addr(array_init[0])), len |> uint())
        return global

    def create_argnames_array ( argnames: string const?; len: int )
        if len == 0
            return LLVMConstPointerNull(LLVMPointerType(LLVMInt8Type(), 0u))
        var array_init : array<LLVMValueRef>
        var ty = LLVMPointerType(LLVMInt8Type(), 0u)
        for i in range(len)
            unsafe
                array_init |> push <| get_string_constant_ptr(g_builder, argnames[i])
        let tinfo_array_ty = LLVMArrayType(ty, len |> uint())
        var global = LLVMAddGlobal(g_mod, tinfo_array_ty, "argnames")
        global |> LLVMSetInitializer <| LLVMConstArray(tinfo_array_ty, unsafe(addr(array_init[0])), len |> uint())
        return global

    def rec_create_type_info_global ( info: TypeInfo?; var initValues: array<LLVMValueRef> )
        initValues |> resize(6)
        for i in range (6)
            initValues[i] = LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(), 0u))

        if info.basicType==Type tHandle
            let resolvent = "~{info.annotation._module.name}::{info.annotation.name}"
            var llvm_resolvent = g_builder |> get_string_constant_ptr(resolvent)
            LLVMSetAlignment(llvm_resolvent, 2u)
            // now do this:
            //      info->annotation_or_name =  ((TypeAnnotation*)(intptr_t(annName)|1));
            let one = LLVMConstInt(LLVMInt64Type(), 1 |> uint64(), 0)
            let ptrToInt = LLVMBuildPtrToInt(g_builder, llvm_resolvent, LLVMInt64Type(), "ptrToInt")
            let modifiedPtr = LLVMBuildOr(g_builder, ptrToInt, one, "modifiedPtr")
            let intToPtr = LLVMBuildIntToPtr(g_builder, modifiedPtr, LLVMPointerType(LLVMVoidType(), 0u), "intToPtr");

            initValues[0] = intToPtr

        elif info.basicType==Type tStructure
            var t = create_struct_global_pointer(info.structType)
            initValues[0] = t

        elif info.basicType==Type tEnumeration || info.basicType==Type tEnumeration8 || info.basicType==Type tEnumeration16
            initValues[0] = create_enum_global_pointer(info.enumType)

        elif info.basicType==Type tPointer || info.basicType==Type tArray
            initValues[1] = create_type_info_global(info.firstType)

        elif info.basicType==Type tTable
            initValues[1] = create_type_info_global(info.firstType)
            initValues[2] = create_type_info_global(info.secondType)

        elif info.basicType==Type tTuple || info.basicType==Type tVariant
            var aa = create_argtypes_array(info.argTypes, info.argCount |> int())
            var an = create_argnames_array(info.argNames, info.argNames == null ? 0 : info.argCount |> int())
            initValues[3] = aa
            initValues[4] = an

        elif info.basicType==Type tFunction || info.basicType==Type tLambda || info.basicType==Type tBlock
            var aa = create_argtypes_array(info.argTypes, info.argCount |> int())
            var an = create_argnames_array(info.argNames, info.argCount |> int())
            initValues[1] = create_type_info_global(info.firstType)
            initValues[3] = aa
            initValues[4] = an

    def create_type_info_global ( ti: TypeInfo? )
        if typeinfo_cache |> key_exists <| ti
            return typeinfo_cache[ti]

        var type_info_type = get_llvm_type_for_typeinfo()
        let type_info_global = LLVMAddGlobal(g_mod, type_info_type, "" /* no name */)

        typeinfo_cache[ti] = type_info_global

        var init_values <- get_typeinfo_fields(ti)
        var type_info_init = LLVMGetUndef(type_info_type)
        for i in urange(init_values |> length())
            type_info_init = g_builder |> LLVMBuildInsertValue(type_info_init, init_values[i], i, "")
        // let LineInfoInit = LLVMConstStruct(unsafe(addr(InitValues[0])), InitValues |> length() |> uint(), 0)
        LLVMSetInitializer(type_info_global, type_info_init)
        return g_builder |> LLVMBuildPointerCast(type_info_global, LLVMPointerType(LLVMVoidType(), 0u), "")

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    // cache maps pointer to pointer to global variable
    structinfo_cache: table<StructInfo?; LLVMValueRef>
    typeinfo_cache:   table<TypeInfo?; LLVMValueRef>
    enuminfo_cache:   table<EnumInfo?; LLVMValueRef>
    varinfo_cache:    table<VarInfo?; LLVMValueRef>

// ExprDebug
    def override visitExprDebug(expr:smart_ptr<ExprDebug>) : ExpressionPtr
        // // void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * )
        var message = ""
        if length(expr.arguments) > 1
            message = string((expr.arguments[1] as ExprConstString).value)
        var tinfo = make_type_info_structure(*jit_context,expr.arguments[0]._type)
        var tinfo_global = create_type_info_global(tinfo)
        var params <- [[auto
            cast_to_vec4f(expr.arguments[0]._type, getE(expr.arguments[0]));
            tinfo_global;
            get_string_constant_ptr(g_builder,message);
            get_context_param();
            get_line_info_ptr(expr.at)   // at
        ]]
        var typ = g_fn_types[FN_JIT_DEBUG]
        var dbg = LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_DEBUG), params, "")
        setE(expr, dbg)
        return expr

// ExprInvoke
    def build_invoke_params ( expr:smart_ptr<ExprInvoke> )
        assume blockT = expr.arguments[0]._type
        var params : array<LLVMOpaqueValue?>
        for a,ai in expr.arguments,count()
            if ai==0        // skip first argument
                continue
            if a._type.baseType==Type fakeContext
                params |> push(get_context_param())
            elif a._type.baseType==Type fakeLineInfo
                var pinfo = getE(a)
                params |> push(pinfo)
            elif a._type.baseType==Type tPointer
                assume funcArg = blockT.argTypes[ai-1]
                var ptrType = type_to_llvm_abi_type(funcArg)
                var ptrValue = LLVMBuildPointerCast(g_builder, getE(a), ptrType, "arg_{ai-1}_ptr")
                params |> push( ptrValue )
            elif (a._type.baseType==Type tArray || a._type.baseType==Type tTable)
                assume funcArg = blockT.argTypes[ai-1]
                var destType = type_to_llvm_abi_type(funcArg)
                var arg = LLVMBuildPointerCast(g_builder, getE(a), destType, "any_array_{ai}_ptr")
                params |> push(arg)
            else
                params |> push(getE(a))
        return <- params

    def override visitExprInvoke(expr:smart_ptr<ExprInvoke>) : ExpressionPtr
        assume blockT = expr.arguments[0]._type
        var cmresEval : LLVMOpaqueValue?
        var cmresEvalPtr : LLVMOpaqueValue?
        if isCMRESType(expr._type)
            if call2cmres |> key_exists(get_ptr(expr))
                cmresEval = call2cmres[get_ptr(ExpressionPtr(expr))]
                cmresEvalPtr = LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(LLVMVoidType(),0u), "invoke_cmres_eval_{int(expr.at.line)}_ptr")
            else
                var vtype = type_to_llvm_type(expr._type)
                at_function_entry <|
                    cmresEval = LLVMBuildAlloca(g_builder, vtype, "invoke_cmres_eval_{int(expr.at.line)}_var")
                    LLVMSetAlignment(cmresEval, 16u)
                    cmresEvalPtr = LLVMBuildPointerCast(g_builder, cmresEval, LLVMPointerType(LLVMVoidType(),0u), "invoke_cmres_eval_{int(expr.at.line)}_ptr")
        if blockT.isGoodFunctionType
            var args = build_array_of_arguments(expr.arguments, true)
            var FUNC = getE(expr.arguments[0])
            var SIMFUNCTION = LLVMBuildExtractValue(g_builder, FUNC, uint(JIT_FUNCTION SIM_FUNCTION), "SIMFUNCTION")
            SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, LLVMPointerType(LLVMVoidType(), 0u), "")
            var params : array<LLVMOpaqueValue?>
            params |> push(SIMFUNCTION)
            params |> push(args)
            if cmresEval!=null
                params |> push(cmresEvalPtr)
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if cmresEval != null
                setE(expr, cmresEval)
            elif blockT.firstType.isVoid
                setE(expr, ccall)
            else
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
        elif blockT.isGoodLambdaType
            var args = build_array_of_arguments(expr.arguments, false)
            var LAMBDA = getE(expr.arguments[0])
            check_ptr_zero(LAMBDA, expr.at, "invoke null lambda")
            LAMBDA = LLVMBuildPointerCast(g_builder, LAMBDA, LLVMPointerType(g_t_lambda,0u), "LAMBDA ON HEAP")
            // debug("Lambda type: {describe(LLVMTypeOf(LAMBDA))}, {expr.at} : {describe(expr)}")
            LAMBDA = LLVMBuildLoad2(g_builder, g_t_lambda, LAMBDA, "LAMBDA")
            var SIMFUNCTION = LLVMBuildExtractValue(g_builder, LAMBDA, uint(JIT_LAMBDA EVAL), "EVAL")
            SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, LLVMPointerType(LLVMVoidType(), 0u), "")
            var params : array<LLVMOpaqueValue?>
            params |> push(SIMFUNCTION)
            params |> push(args)
            if cmresEval!=null
                params |> push(cmresEvalPtr)
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL)
            var typ = g_fn_types[cmresEval!=null ? FN_JIT_CALL_WITH_CMRES : FN_JIT_CALL_OR_FASTCALL]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if cmresEval != null
                setE(expr, cmresEval)
            elif blockT.firstType.isVoid
                setE(expr, ccall)
            else
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
        elif blockT.isGoodBlockType
            var args = build_array_of_arguments(expr.arguments, true)
            var blk = getE(expr.arguments[0])
            var params : array<LLVMOpaqueValue?>
            params |> push(blk)
            params |> push(args)
            if cmresEval!=null
                params |> push(cmresEvalPtr)
            params |> push(get_context_param())
            var func = LLVMGetNamedFunction(g_mod, cmresEval!=null ? FN_JIT_INVOKE_BLOCK_WITH_CMRES : FN_JIT_INVOKE_BLOCK)
            var typ = g_fn_types[cmresEval!=null ? FN_JIT_INVOKE_BLOCK_WITH_CMRES : FN_JIT_INVOKE_BLOCK]
            var ccall = LLVMBuildCall2(g_builder, typ, func, params, "invoke")
            if cmresEval != null
                setE(expr, cmresEval)
            elif blockT.firstType==null || blockT.firstType.isVoid
                setE(expr, ccall)
            else
                var cres = cast_from_vec4f(expr, expr._type, ccall)
                setE(expr, cres)
        else
            failed_E(expr, "invoke({describe(blockT)},...) is not supported yet")
        return expr

// ExprErase
    def build_table_erase ( ttype:Type; tab:LLVMOpaqueValue?; key:LLVMOpaqueValue?; valueTypeSize:int ) : LLVMOpaqueValue?
        var params = [[auto
            tab;                                                        // table ptr
            key;                                                        // key
            LLVMConstInt(LLVMInt32Type(), uint64(valueTypeSize), 0);    // valueTypeSize
            get_context_param()
        ]]
        var tab_fun = LLVMGetNamedFunction(g_mod, FN_JIT_TABLE_ERASE(ttype))
        if tab_fun==null
            error("table_erase {ttype} function not found")
            return LLVMGetUndef(LLVMInt32Type())
        var typ = g_fn_types[FN_JIT_TABLE_ERASE(ttype)]
        return LLVMBuildCall2(g_builder, typ, tab_fun, params, "")

    def override visitExprErase(expr:smart_ptr<ExprErase>) : ExpressionPtr
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, LLVMPointerType(LLVMVoidType(),0u), "tab_ptr")
        var keyType : Type
        if subExprT.firstType.isWorkhorseType
            keyType = subExprT.firstType.baseType
        else
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        var res = build_table_erase(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
        setE(expr, res)
        return expr

// ExprSetInsert
    def override visitExprSetInsert(expr:smart_ptr<ExprSetInsert>) : ExpressionPtr
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, LLVMPointerType(LLVMVoidType(),0u), "tab_ptr")
        var keyType : Type
        if subExprT.firstType.isWorkhorseType
            keyType = subExprT.firstType.baseType
        else
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        var res = build_table_at(keyType, tab_ptr, tidx, 0)
        setE(expr, res)
        return expr

// ExprFind
    def override visitExprFind(expr:smart_ptr<ExprFind>) : ExpressionPtr
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, LLVMPointerType(LLVMVoidType(),0u), "tab_ptr")
        var keyType : Type
        if subExprT.firstType.isWorkhorseType
            keyType = subExprT.firstType.baseType
        else
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        var at_tab = build_table_find(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, at_tab, LLVMConstInt(LLVMInt32Type(), uint64(-1), 0), "")
        var resType = LLVMPointerType(type_to_llvm_type(subExprT.secondType),0u)
        var res = build_select(cond, resType) <|
            var inscope tab_ty := expr.arguments[0]._type.isPointer ? expr.arguments[0]._type.firstType : expr.arguments[0]._type
            var tab = LLVMBuildLoad2(g_builder, type_to_llvm_type(tab_ty), tsrc, "tab")
            var data = LLVMBuildExtractValue(g_builder, tab, uint(JIT_TABLE DATA), "table.data")
            return LLVMBuildGEP2(g_builder, type_to_llvm_type(subExprT.secondType), data, at_tab, "table_at")
        lpipe <|
            return LLVMConstPointerNull(resType)
        setE(expr, res)
        return expr

// ExprKeyExists
    def override visitExprKeyExists(expr:smart_ptr<ExprKeyExists>) : ExpressionPtr
        var tsrc = getE(expr.arguments[0])
        var tidx = getE(expr.arguments[1])
        assume subExprT = expr.arguments[0]._type
        var tab_ptr = LLVMBuildPointerCast(g_builder, tsrc, LLVMPointerType(LLVMVoidType(),0u), "tab_ptr")
        var keyType : Type
        if subExprT.firstType.isWorkhorseType
            keyType = subExprT.firstType.baseType
        else
            var inscope uvt <- get_underlying_value_type(subExprT.firstType)
            keyType = uvt.baseType
        var at_tab = build_table_find(keyType, tab_ptr, tidx, subExprT.secondType.sizeOf)
        var res = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, at_tab, LLVMConstInt(LLVMInt32Type(), uint64(-1), 0), "")
        setE(expr, res)
        return expr

// ExprAscend
    def override preVisitExprAscend(expr:smart_ptr<ExprAscend>) : void
        var bytes = expr.subexpr._type.sizeOf
        let needTypeInfo = expr.ascendFlags.needTypeInfo
        if needTypeInfo
            bytes += 16
        var mks_ptr : LLVMOpaqueValue?
        var needMemset = true
        if expr.subexpr._type.baseType == Type tHandle
            let fn_new_handle = get_new_handle_function(expr.subexpr._type)
            if fn_new_handle == null
                setE(expr, LLVMConstNull(LLVMPointerType(LLVMVoidType(),0u)))
                return
            var params = [[auto[]
                get_context_param()
            ]]
            var mangled_name = expr.subexpr._type |> get_mangled_name()
            var typ = g_fn_types[("new`handle`{mangled_name}")]
            mks_ptr = LLVMBuildCall2(g_builder, typ, fn_new_handle, params, "new`handle`{describe(expr.subexpr._type)}")
            needMemset = false
        else
            mks_ptr = build_alloc(bytes, false)
        check_ptr_zero(mks_ptr, expr.at, "new [[...]] returned null")
        var memset_bytes = bytes
        if needTypeInfo
            var tinfo = make_type_info_structure(*jit_context,expr.subexpr._type)
            var tptr = LLVMConstIntToPtr(LLVMConstInt(LLVMInt64Type(), intptr(tinfo), 0), LLVMPointerType(LLVMVoidType(), 0u))
            LLVMBuildStore2(g_builder, LLVMPointerType(LLVMVoidType(), 0u), tptr, mks_ptr)
            // skip 16 bytes in mks_ptr
            var p2int = LLVMBuildPtrToInt(g_builder, mks_ptr, LLVMIntPtrType(), "")
            var add16 = LLVMBuildAdd(g_builder, p2int, LLVMConstInt(LLVMIntPtrType(), uint64(16), 0), "")
            mks_ptr = LLVMBuildIntToPtr(g_builder, add16, LLVMPointerType(LLVMVoidType(),0u), "")
            memset_bytes -= 16
        if needMemset
            LLVMBuildMemSet(g_builder, mks_ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(memset_bytes), 0), 16u)
        if expr.ascendFlags.useStackRef
            let stackref_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            setE(expr.subexpr, stackref_ptr)
        mks_ptr = LLVMBuildPointerCast(g_builder, mks_ptr, type_to_llvm_type(expr._type), "")
        setE(expr,mks_ptr)

// ExprCast
    def override visitExprCast(expr:smart_ptr<ExprCast>) : ExpressionPtr
        var res : LLVMOpaqueValue?
        assume ect = expr._type
        assume subT = expr.subexpr._type
        if ect.isRef && subT.isRef
            res = LLVMBuildPointerCast(g_builder, getE(expr.subexpr), LLVMPointerType(type_to_llvm_type(ect),0u), "")
        elif ect.isPointer && subT.isPointer
            res = LLVMBuildPointerCast(g_builder, getE(expr.subexpr), type_to_llvm_type(ect), "")
        elif length(ect.dim)!=0 && subT.isPointer
            res = LLVMBuildPointerCast(g_builder, getE(expr.subexpr), LLVMPointerType(type_to_llvm_type(ect),0u), "")
        elif ect.isInteger && (ect.baseType==Type tInt64 || ect.baseType==Type tUInt64) && subT.isPointer
            res = LLVMBuildPtrToInt(g_builder, getE(expr.subexpr), type_to_llvm_type(ect), "")
        elif !ect.isRefType && !subT.isRefType
            if ect.sizeOf == subT.sizeOf
                res = LLVMBuildBitCast(g_builder, getE(expr.subexpr), type_to_llvm_type(ect), "")
            else
                res = LLVMGetUndef(type_to_llvm_type(ect))
                failed_E(expr, "cast<{describe(expr.castType)}> is not supported, bitcast size mismatch")
        else
            res = LLVMGetUndef(type_to_llvm_type(ect))
            failed_E(expr, "cast<{describe(expr.castType)}> is not supported yet in {describe(expr)} {describe(subT)} -> {describe(ect)}")
        setE(expr, res)
        return expr

// ExprDelete
    def build_lambda_delete ( lmb:LLVMOpaqueValue?; at:LineInfo )
        var args : LLVMOpaqueValue?
        at_function_entry <|
            args = LLVMBuildAlloca(g_builder, LLVMArrayType(LLVMFloat4Type(), 1u), "args")
            LLVMSetAlignment(args, 16u)
            args = LLVMBuildPointerCast(g_builder, args, LLVMPointerType(LLVMFloat4Type(), 0u), "delete lambda args ptr")
        check_ptr_zero(lmb, at, "invoke null lambda")
        // build arguments (1, and its 'capture')
        let ai = 0
        var arg_ptr = LLVMBuildGEP2(g_builder, LLVMFloat4Type(), args, LLVMConstInt(LLVMInt32Type(), uint64(ai), 0), "call_arg_{ai}_ptr")
        var arg_val = lmb
        var arg_cast_type = LLVMPointerType(LLVMVoidType(), 0u)
        LLVMBuildStore2(g_builder, arg_cast_type, arg_val, arg_ptr)
        // now invoke
        var LAMBDA = LLVMBuildPointerCast(g_builder, lmb, LLVMPointerType(g_t_lambda,0u), "LAMBDA ON HEAP")
        LAMBDA = LLVMBuildLoad2(g_builder, g_t_lambda, LAMBDA, "LAMBDA")
        var SIMFUNCTION = LLVMBuildExtractValue(g_builder, LAMBDA, uint(JIT_LAMBDA FINALIZER), "FINALIZER")
        SIMFUNCTION = LLVMBuildPointerCast(g_builder, SIMFUNCTION, LLVMPointerType(LLVMVoidType(), 0u), "")
        var params : array<LLVMOpaqueValue?>
        params |> push(SIMFUNCTION)
        params |> push(args)
        params |> push(get_context_param())
        var func = LLVMGetNamedFunction(g_mod, FN_JIT_CALL_OR_FASTCALL)
        var typ = g_fn_types[FN_JIT_CALL_OR_FASTCALL]
        LLVMBuildCall2(g_builder, typ, func, params, "delete_lambda")

    def get_delete_handle_function ( t:TypeDeclPtr ) : LLVMOpaqueValue?
        let mangeld_name = get_mangled_name(t)
        let func_name = "delete`handle`{mangeld_name}"
        var delete_handle = LLVMGetNamedFunction(g_mod, func_name)
        if delete_handle == null
            var delete_handle_ptr = get_jit_delete(t)
            if delete_handle_ptr == null
                failed_T(t, "missing delete`handle function pointer for {describe(t)}")
                return null
            // void delete_handle ( void *; Context * ) {
            delete_handle = LLVMAddFunctionWithType(g_mod, func_name,
                LLVMFunctionType(LLVMVoidType(),
                    [[LLVMTypeRef[] LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
            LLVMAddGlobalMapping(g_engine, delete_handle, delete_handle_ptr)
            var nounwind = LLVMGetEnumAttribute("nounwind")
            var willreturn = LLVMGetEnumAttribute("willreturn")
            var nocapture = LLVMGetEnumAttribute("nocapture")
            LLVMAddAttributesToFunction(delete_handle, [[auto nounwind; willreturn]])
            LLVMAddAttributeToFunctionArgumentRange(delete_handle, urange(0,1), nocapture)
        return delete_handle

    def override visitExprDelete(expr:smart_ptr<ExprDelete>) : ExpressionPtr
        assume subexpr = expr.subexpr
        assume subT = expr.subexpr._type
        let total = subT.countOf
        if total != 1
            failed_E(expr, "delete of multiple elements is not supported yet {describe(subT)} [{total}]")
            return expr
        var se = getE(subexpr)
        if subT.isPointer
            assume firstT = expr.subexpr._type.firstType
            if firstT.isStructure
                if !firstT.structType.flags.isClass
                    var structSize = firstT.sizeOf
                    let persistent = firstT.structType.flags.persistent
                    let isLambda = firstT.structType.flags.isLambda
                    var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(LLVMPointerType(LLVMVoidType(),0u),0u), "ppStruct")
                    var ps = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMVoidType(),0u), pps, "pStruct")
                    if persistent
                        var params = [[auto
                            ps;
                            get_context_param()
                        ]]
                        var typ = g_fn_types[FN_JIT_FREE_PERSISTENT]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_PERSISTENT), params, "")
                    elif isLambda
                        structSize += 16
                        var p2int = LLVMBuildPtrToInt(g_builder, ps, LLVMIntPtrType(), "")
                        var sub16 = LLVMBuildSub(g_builder, p2int, LLVMConstInt(LLVMIntPtrType(), uint64(16), 0), "")
                        var lmb = LLVMBuildIntToPtr(g_builder, sub16, LLVMPointerType(LLVMVoidType(),0u), "")
                        var params = [[auto
                            lmb;
                            LLVMConstInt(LLVMInt32Type(), uint64(structSize), 0);
                            get_context_param()
                        ]]
                        var typ = g_fn_types[FN_JIT_FREE_HEAP]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                    else
                        var params = [[auto
                            ps;
                            LLVMConstInt(LLVMInt32Type(), uint64(structSize), 0);
                            get_context_param()
                        ]]
                        var typ = g_fn_types[FN_JIT_FREE_HEAP]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                    LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMVoidType(),0u)), pps)
                else
                    var structSize = getE(expr.sizeexpr)
                    let persistent = firstT.structType.flags.persistent
                    var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(LLVMPointerType(LLVMVoidType(),0u),0u), "ppClass")
                    var ps = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMVoidType(),0u), pps, "pClass")
                    if persistent
                        var params = [[auto
                            ps;
                            get_context_param()
                        ]]
                        var typ = g_fn_types[FN_JIT_FREE_PERSISTENT]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_PERSISTENT), params, "")
                    else
                        var params = [[auto
                            ps;
                            structSize;
                            get_context_param()
                        ]]
                        var typ = g_fn_types[FN_JIT_FREE_HEAP]
                        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                    LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMVoidType(),0u)), pps)
            elif firstT.isHandle
                let fn_delete_handle = get_delete_handle_function(firstT)
                if fn_delete_handle == null
                    return expr
                var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(LLVMPointerType(LLVMVoidType(),0u),0u), "")
                var ps = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMVoidType(),0u), pps, "delete`handle`{describe(firstT)}")
                var params = [[auto[]
                    ps;
                    get_context_param()
                ]]
                let mangeld_name = get_mangled_name(firstT)
                let typ = g_fn_types[("delete`handle`{mangeld_name}")]
                LLVMBuildCall2(g_builder, typ, fn_delete_handle, params, "")
                LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMVoidType(),0u)), pps)
            elif firstT.isVariant || firstT.isTuple
                var structSize = firstT.sizeOf
                var pps = LLVMBuildPointerCast(g_builder, se, LLVMPointerType(LLVMPointerType(LLVMVoidType(),0u),0u), "ppData")
                var ps = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMVoidType(),0u), pps, "pData")
                var params = [[auto
                    ps;
                    LLVMConstInt(LLVMInt32Type(), uint64(structSize), 0);
                    get_context_param()
                ]]
                var typ = g_fn_types[FN_JIT_FREE_HEAP]
                LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_FREE_HEAP), params, "")
                LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMVoidType(),0u)), pps)
            else
                failed_E(expr, "delete of pointer non-structure type is not supported yet {describe(subT)}")
        elif subT.isGoodLambdaType
            var lse = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), se, uint(subT.alignOf), "lambda_r2v")
            build_lambda_delete(lse, expr.at)
        else
            failed_E(expr, "delete of non-pointer type is not supported yet {describe(subT)}")
        return expr

// ExprVar
    def get_global_offset_by_mnh ( mnh:uint64; shared_mnh:bool; name:string )
        var params = [[auto
            LLVMConstInt(LLVMInt64Type(), mnh, 0);
            get_context_param()
        ]]
        var func = LLVMGetNamedFunction(g_mod, shared_mnh ? FN_JIT_GET_SHARED_MNH : FN_JIT_GET_GLOBAL_MNH)
        var typ = g_fn_types[shared_mnh ? FN_JIT_GET_SHARED_MNH : FN_JIT_GET_GLOBAL_MNH]
        var res = LLVMBuildCall2(g_builder, typ, func, params, name)
        return res

    def override visitExprVar(expr:smart_ptr<ExprVar>) : ExpressionPtr
        assume varType = expr.variable._type
        if expr.varFlags._block
            var tryV = tryGetV(expr.variable)
            if tryV != null
                if expr.varFlags.r2v
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.variable._type), tryV, uint(expr._type.alignOf), "var_block_r2v_{expr.variable.name}"))
                else
                    setE(expr, tryV)
                return expr
            var arg = LLVMGetParam(ffunc,uint(expr.argumentIndex))
            if expr.varFlags.r2v
                if varType.flags.ref
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.variable._type), arg, uint(expr._type.alignOf), "var_block_ref_r2v_{expr.variable.name}"))
                else
                    setE(expr, arg)
            else
                setE(expr, arg)
        elif expr.varFlags.local    // local varaibels
            var var_ptr = getV(expr.variable)
            if expr.variable.flags.aliasCMRES
                pass
            if varType.flags.ref
                var_ptr = LLVMBuildLoadData2Aligned(g_builder, LLVMPointerType(type_to_llvm_type(expr._type), 0u), var_ptr, uint(expr._type.alignOf), "")
            if expr.varFlags.r2v
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), var_ptr, uint(expr._type.alignOf), "local_var_r2v_{expr.variable.name}"))
            else
                setE(expr, var_ptr)
        elif expr.varFlags.argument // arguments
            var tryV = tryGetV(expr.variable)
            if tryV != null
                if expr.varFlags.r2v
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), tryV, uint(expr._type.alignOf), "arg_var_r2v_{expr.variable.name}"))
                else
                    setE(expr, tryV)
                return expr
            var arg = LLVMGetParam(ffunc,uint(expr.argumentIndex))
            if expr.varFlags.r2v
                if varType.flags.ref
                    setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), arg, uint(expr._type.alignOf), "arg_var_ref_r2v_{expr.variable.name}"))
                else
                    setE(expr, arg)
            else
                setE(expr, arg)
        else
            var tryV = tryGetV(expr.variable)
            if tryV == null
                at_function_entry <|
                    let isSolidContext = expr.variable._module.moduleFlags.isSolidContext
                    var vptr : LLVMOpaqueValue?
                    if !isSolidContext
                        let mnh = hash(get_mangled_name(expr.variable))
                        vptr = get_global_offset_by_mnh(mnh, expr.variable.flags.global_shared, "global_var_{expr.variable.name}_mnh_ptr")
                        vptr = LLVMBuildPointerCast(g_builder, vptr, get_type_pointer(expr.variable._type), "global_var_{expr.variable.name}")
                    else
                        var ctx_ptr = LLVMBuildPointerCast(g_builder, get_context_param(), LLVMPointerType(LLVMInt8Type(),0u), "")
                        let coffset = expr.variable.flags.global_shared ? CONTEXT_OFFSET_OF_SHARED : CONTEXT_OFFSET_OF_GLOBALS
                        var global_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), ctx_ptr, LLVMConstInt(LLVMInt32Type(), uint64(coffset), 0), "")
                        global_ptr = LLVMBuildLoad2(g_builder, LLVMPointerType(LLVMInt8Type(),0u), global_ptr, "")
                        vptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), global_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.variable.stackTop), 0), "")
                    tryV = LLVMBuildPointerCast(g_builder, vptr, get_type_pointer(expr.variable._type), "global_var_{expr.variable.name}")
            if expr.varFlags.r2v
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), tryV, uint(expr._type.alignOf), "global_var_r2v_{expr.variable.name}"))
            else
                setE(expr, tryV)
        return expr

// ExprField
    def variant_field_name ( t:TypeDeclPtr; index:int )
        if !t.isVariant
            failed_T(t, "not a variant {describe(t)}")
            return "!!not_a_variant!!"
        return t.argNames |> length!=0 ? "{t.argNames[index]}" : "{describe(t.argTypes[index])}"

    def override visitExprField(expr:smart_ptr<ExprField>) : ExpressionPtr
        if expr.value._type.isBitfield
            let mask = 1u << uint(expr.fieldIndex)
            let mand = LLVMBuildAnd(g_builder, getE(expr.value), LLVMConstInt(LLVMInt32Type(), uint64(mask), 0), "")
            let if_not_zero = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, mand, LLVMConstInt(LLVMInt32Type(), uint64(0), 0), "")
            var res_type = type_to_llvm_type(expr._type)
            var res = build_select(if_not_zero, res_type) <|
                return LLVMConstInt(LLVMInt1Type(), uint64(1), 0)
            lpipe <|
                return LLVMConstInt(LLVMInt1Type(), uint64(0), 0)
            setE(expr, res)
        elif expr.field==null && expr.fieldIndex==-1
            let field_offset = expr.annotation |> get_handled_type_field_offset(string(expr.name))
            if field_offset==-1U
                setE(expr, LLVMGetUndef(type_to_llvm_type(expr._type)))
                failed_E(expr, "field {expr.name} offset is -1")
                return expr
            var val_ptr = getE(expr.value)
            if expr.value._type.isPointer && !expr.derefFlags.unsafeDeref
                check_ptr_zero(val_ptr, expr.at)
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            var field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(field_offset), 0), "")
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, get_type_pointer(expr._type), "{describe(expr.value._type,false,false,false)}.{expr.name}")
            if expr.fieldFlags.r2v
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), field_ptr, uint(expr._type.alignOf), ""))
            else
                setE(expr, field_ptr)
        elif expr.value._type.isTuple
            assert(expr.fieldIndex!=-1)
            var val_ptr = getE(expr.value)
            if expr.value._type.isPointer && !expr.derefFlags.unsafeDeref
                check_ptr_zero(val_ptr, expr.at)
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            let field_offset = get_tuple_field_offset(expr.value._type, expr.fieldIndex)
            var field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(),val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(field_offset), 0), "")
            let argName = tuple_field_name(expr.value._type,expr.fieldIndex)
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, get_type_pointer(expr.value._type.argTypes[expr.fieldIndex]), "{describe(expr.value._type,false,false,false)}.{argName}")
            if expr.fieldFlags.r2v
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), field_ptr, uint(expr._type.alignOf), ""))
            else
                setE(expr, field_ptr)
        elif expr.value._type.isVariant
            assert(expr.fieldIndex!=-1)
            var val_ptr = getE(expr.value)
            if expr.value._type.isPointer && !expr.derefFlags.unsafeDeref
                check_ptr_zero(val_ptr, expr.at)
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            let field_offset = get_variant_field_offset(expr.value._type, expr.fieldIndex)
            var field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(field_offset), 0), "")
            let argName = variant_field_name(expr.value._type,expr.fieldIndex)
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, get_type_pointer(expr.value._type.argTypes[expr.fieldIndex]), "{describe(expr.value._type,false,false,false)}.{argName}")
            if expr.fieldFlags.r2v
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), field_ptr, uint(expr._type.alignOf), ""))
            else
                setE(expr, field_ptr)
        else
            assert(expr.field!=null)
            var val_ptr = getE(expr.value)
            if expr.value._type.isPointer && !expr.derefFlags.unsafeDeref
                check_ptr_zero(val_ptr, expr.at)
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            var field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.field.offset), 0), "")
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, get_type_pointer(expr.field._type), "{describe(expr.value._type,false,false,false)}.{expr.field.name}")
            if expr.fieldFlags.r2v
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), field_ptr, uint(expr._type.alignOf), ""))
            else
                setE(expr, field_ptr)
        return expr

// ExprSafeField
    def override visitExprSafeField(expr:smart_ptr<ExprSafeField>) : ExpressionPtr
        // unsupported(expr, "safe field access is not supported yet")
        var fieldOffset = -1
        if expr.annotation==null
            if expr.fieldIndex != - 1
                if expr.value._type.firstType.isVariant
                    fieldOffset = expr.value._type.firstType |> get_variant_field_offset(expr.fieldIndex)
                else
                    fieldOffset = expr.value._type.firstType |> get_tuple_field_offset(expr.fieldIndex)
            else
                fieldOffset = expr.field.offset
            assert(fieldOffset>=0,"field offset is somehow not there")
        else
            fieldOffset = int(expr.annotation |> get_handled_type_field_offset(string(expr.name)))
            if fieldOffset==-1
                setE(expr, LLVMGetUndef(type_to_llvm_type(expr._type)))
                failed_E(expr, "field {expr.name} offset is -1")
                return expr
        if expr.skipQQ
            var val_ptr = getE(expr.value)
            var cond = LLVMBuildIsNotNull(g_builder, val_ptr, "")
            var res_type = type_to_llvm_type(expr._type)
            var res = build_select(cond, res_type) <|
                val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
                var field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(fieldOffset), 0), "")
                field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, LLVMPointerType(res_type,0u), ".{expr.name}")
                field_ptr = LLVMBuildLoad2(g_builder, res_type, field_ptr, "")
                return field_ptr
            lpipe <|
                return  LLVMConstPointerNull(res_type)
            setE(expr, res)
        else
            var val_ptr = getE(expr.value)
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
            var field_ptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(fieldOffset), 0), "")
            var res_type = type_to_llvm_type(expr._type)
            field_ptr = LLVMBuildPointerCast(g_builder, field_ptr, res_type, ".{expr.name}")
            var cond = LLVMBuildIsNotNull(g_builder, val_ptr, "")
            var res = LLVMBuildSelect(g_builder, cond, field_ptr, LLVMConstPointerNull(res_type), "")
            setE(expr, res)
        return expr

// ExprSwizzle
    def override visitExprSwizzle(expr:smart_ptr<ExprSwizzle>) : ExpressionPtr
        var val = getE(expr.value)
        if expr._type.flags.ref
            var val_ptr = LLVMBuildBitCast(g_builder, val, get_basetype_pointer(expr.value._type.vectorBaseType), "")
            val_ptr = LLVMBuildGEP2(g_builder, base_type_to_llvm_type(expr.value._type.vectorBaseType),
                    val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.fields[0]), 0), "")
            val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, get_type_pointer(expr._type), "")
            setE(expr, val_ptr)
        else
            var isSequencial = true
            for i in range(1,length(expr.fields))
                if (int(expr.fields[i-1])+1)!=int(expr.fields[i])
                    isSequencial = false
                    break
            if expr.value._type.flags.ref && isSequencial
                var val_ptr = LLVMBuildBitCast(g_builder, val, get_basetype_pointer(expr.value._type.vectorBaseType), "")
                val_ptr = LLVMBuildGEP2(g_builder, base_type_to_llvm_type(expr.value._type.vectorBaseType),
                        val_ptr, LLVMConstInt(LLVMInt32Type(), uint64(expr.fields[0]), 0), "")
                val_ptr = LLVMBuildPointerCast(g_builder, val_ptr, get_type_pointer(expr._type), "")
                setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), val_ptr, uint(expr.value._type.alignOf), "swizzle_{expr.fields}"))
            else
                if expr.fields |> length == 1
                    let fi = expr.fields[0]
                    let res = LLVMBuildExtractElement(g_builder, val, LLVMConstInt(LLVMInt32Type(), uint64(fi), 0), "")
                    setE(expr, res)
                else
                    var res = LLVMGetUndef(type_to_llvm_type(expr._type))
                    for i,fi in count(),expr.fields
                        var f = LLVMBuildExtractElement(g_builder, val, LLVMConstInt(LLVMInt32Type(), uint64(fi), 0), "")
                        res = LLVMBuildInsertElement(g_builder, res, f, LLVMConstInt(LLVMInt32Type(), uint64(i), 0), "")
                    setE(expr, res)
        return expr

// ExprIsVariant
    def override visitExprIsVariant(expr:smart_ptr<ExprIsVariant>) : ExpressionPtr
        var field_index = find_argument_index(expr.value._type, string(expr.name))
        if field_index==-1
            failed_E(expr, "variant field {expr.name} not found")
            return expr
        var vptr = getE(expr.value)
        var vint = LLVMBuildLoad2(g_builder, LLVMInt32Type(), vptr, "variant_index")
        let cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, vint, LLVMConstInt(LLVMInt32Type(), uint64(field_index), 0), "is_variant")
        setE(expr, cond)
        return expr

// ExprAsVariant
    def check_variant_index ( tidx:LLVMOpaqueValue?; at:LineInfo; vidx:int; message:string )
        var cmpIdx = LLVMConstInt(LLVMInt32Type(), uint64(vidx), 0)
        var check_null_ptr = LLVMAppendBasicBlock(ffunc,"check_variant_index")
        var check_true = LLVMAppendBasicBlock(ffunc,"check_true")
        var check_end = LLVMAppendBasicBlock(ffunc,"check_end")
        LLVMBuildBr(g_builder, check_null_ptr)
        LLVMPositionBuilderAtEnd(g_builder,check_null_ptr)
        var cond = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntNE, tidx, cmpIdx, "index_check")
        LLVMBuildCondBr(g_builder, cond, check_true, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_true)
        build_exception(message,at)
        LLVMBuildBr(g_builder, check_end)
        LLVMPositionBuilderAtEnd(g_builder,check_end)

    def override visitExprAsVariant(expr:smart_ptr<ExprAsVariant>) : ExpressionPtr
        var field_index = find_argument_index(expr.value._type, string(expr.name))
        if field_index==-1
            failed_E(expr, "variant field {expr.name} not found")
            return expr
        var vptr = getE(expr.value)
        var vint = LLVMBuildLoad2(g_builder, LLVMInt32Type(), vptr, "variant_index")
        check_variant_index(vint, expr.at, field_index, "variant mismatch, expecting {expr.value._type.argNames[field_index]}")
        let field_offset = get_variant_field_offset(expr.value._type,field_index)
        var fptr = LLVMBuildPointerCast(g_builder, vptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
        fptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), fptr, LLVMConstInt(LLVMInt32Type(), uint64(field_offset), 0), "")
        fptr = LLVMBuildPointerCast(g_builder, fptr, LLVMPointerType(type_to_llvm_type(expr._type), 0u), "variant_field_ptr")
        if expr.fieldFlags.r2v
            setE(expr, LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr._type), fptr, uint(expr._type.alignOf), "variant_field"))
        else
            setE(expr, fptr)
        return expr

// ExprSafeAsVariant
    def override visitExprSafeAsVariant(expr:smart_ptr<ExprSafeAsVariant>) : ExpressionPtr
        assume valueT = expr.value._type
        var vptr = getE(expr.value)
        var field_index = find_argument_index(valueT.isPointer ? valueT.firstType : valueT, string(expr.name))
        if field_index==-1
            failed_E(expr, "variant field {expr.name} not found")
            return expr
        var if_not_null = valueT.isPointer ? LLVMBuildIsNotNull(g_builder, vptr, "") : LLVMConstInt(LLVMInt1Type(), 1ul, 0)
        var resType = type_to_llvm_type(expr._type)
        var res = build_select(if_not_null,resType) <|
            var vint = LLVMBuildLoad2(g_builder, LLVMInt32Type(), vptr, "variant_index")
            var fidx = LLVMConstInt(LLVMInt32Type(), uint64(field_index), 0)
            var if_index = LLVMBuildICmp(g_builder, LLVMIntPredicate LLVMIntEQ, vint, fidx, "index_check")
            var subRes = build_select(if_index,resType) <|
                let field_offset = get_variant_field_offset(valueT.isPointer ? valueT.firstType : valueT,field_index)
                var fptr = LLVMBuildPointerCast(g_builder, vptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
                fptr = LLVMBuildGEP2(g_builder, LLVMInt8Type(), fptr, LLVMConstInt(LLVMInt32Type(), uint64(field_offset), 0), "")
                return LLVMBuildPointerCast(g_builder, fptr, resType, "variant_field_ptr")
            lpipe <|
                return LLVMConstNull(resType)
            return subRes
        lpipe <|
            return LLVMConstNull(resType)
        setE(expr,res)
        return expr

// ExprOp1
    def isExprOp1_Func(expr:smart_ptr<ExprOp1>)
        if !expr.func.flags.builtIn
            return true
        if expr.subexpr._type.isHandle
            return true
        return false

    def visitExprOp1_Func(var expr:smart_ptr<ExprOp1>)
        expr.arguments |> resize(1)
        expr.arguments[0] := expr.subexpr
        make_call(expr, false)

    def override visitExprOp1(var expr:smart_ptr<ExprOp1>) : ExpressionPtr
        if isExprOp1_Func(expr)
            visitExprOp1_Func(expr)
            return expr
        var sexpr = getE(expr.subexpr)
        assume opType = expr.subexpr._type
        if expr.op=="+++" || expr.op=="++"
            var expr_t = base_type_to_llvm_type(opType.baseType)
            var sexpr_v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), sexpr, uint(expr.subexpr._type.alignOf), "")
            var res_v : LLVMOpaqueValue?
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                res_v = LLVMBuildAdd(g_builder, sexpr_v, LLVMConstInt(expr_t,1ul,0), "")
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                res_v = LLVMBuildFAdd(g_builder, sexpr_v, LLVMConstReal(expr_t,1.lf), "")
            else
                failed_E(expr, "unsupported ++ type {opType}")
            LLVMBuildStoreAligned(g_builder, res_v, sexpr, uint(expr.subexpr._type.alignOf))
            setE(expr, sexpr_v)
        elif expr.op=="---" || expr.op=="--"
            var expr_t = base_type_to_llvm_type(opType.baseType)
            var sexpr_v = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(expr.subexpr._type), sexpr, uint(expr.subexpr._type.alignOf), "")
            var res_v : LLVMOpaqueValue?
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                res_v = LLVMBuildSub(g_builder, sexpr_v, LLVMConstInt(expr_t,1ul,0), "")
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                res_v = LLVMBuildFSub(g_builder, sexpr_v, LLVMConstReal(expr_t,1.lf), "")
            else
                failed_E(expr, "unsupported -- type {opType}")
            LLVMBuildStoreAligned(g_builder, res_v, sexpr, uint(expr.subexpr._type.alignOf))
            setE(expr, sexpr_v)
        elif expr.op=="~"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildNot(g_builder, sexpr, ""))
            else
                failed_E(expr, "unsupported ~ type {opType}")
        elif expr.op=="!"
            if opType.isBool
                setE(expr, LLVMBuildNot(g_builder, sexpr, ""))
            else
                failed_E(expr, "unsupported ! type {opType}")
        elif expr.op=="-"
            if opType.isInteger || (opType.isVectorType && (opType.vectorBaseType==Type tInt || opType.vectorBaseType==Type tUInt))
                setE(expr, LLVMBuildNeg(g_builder, sexpr, ""))
            elif opType.isFloatOrDouble || (opType.isVectorType && opType.vectorBaseType==Type tFloat)
                setE(expr, LLVMBuildFNeg(g_builder, sexpr, ""))
            else
                failed_E(expr, "unsupported ~ type {opType}")
        else
            failed_E(expr, "unsupported unary operator {expr.op}")
        return expr

// ExprReturn
    def pre_visit_expr_return (expr:smart_ptr<ExprReturn>; isCmres:bool)
        if isCmres && expr.returnFlags.returnCallCMRES
            make_call_to_cmres(expr.subexpr, get_cmres_param())

    def override preVisitExprReturn(expr:smart_ptr<ExprReturn>)
        if thisBlock!=null
            pre_visit_expr_return(expr, isCMRESType(thisBlock.returnType))
        elif thisFunc!=null
            pre_visit_expr_return(expr, isCMRES(thisFunc))

    def build_enter  ( at:LineInfo )
        if LLVM_DEBUG_TRACES
            build_debug_trace(at, FN_JIT_DEBUG_ENTER)

    def build_exit  ( at:LineInfo )
        if LLVM_DEBUG_TRACES
            build_debug_trace(at, FN_JIT_DEBUG_EXIT)

    def build_debug_trace ( at:LineInfo; func_name:string )
        var message : string
        if thisFunc != null
            message = FunctionPtr(thisFunc) |> get_mangled_name
        elif thisBlock != null
            message = "$block"
        var params = [[auto
            get_string_constant_ptr(g_builder, message);   // message
            get_context_param();                           // Context
            get_line_info_ptr(at)
        ]]
        var typ = g_fn_types[func_name]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, func_name), params, "")

    def visit_expr_return(expr:smart_ptr<ExprReturn>; returnType:TypeDeclPtr; isCmres:bool)
        var expr_res : LLVMOpaqueValue?
        if !returnType.isVoid
            if isCmres
                pass
            elif returnType.isPointer && (expr.subexpr is ExprConstPtr) // return null
                pass
            elif returnType.isPointer
                var resPtr = getE(expr.subexpr)
                if expr.subexpr._type.flags.ref
                    // if source is ptr&, we empty it
                    resPtr = LLVMBuildLoadData2Aligned(g_builder, type_to_llvm_type(returnType), resPtr, uint(expr.subexpr._type.alignOf), "result_before_move")
                    if expr.returnFlags.moveSemantics
                        var subPtrType = LLVMPointerType(type_to_llvm_type(expr.subexpr._type.firstType), 0u)
                        LLVMBuildStore(g_builder, LLVMConstNull(subPtrType), getE(expr.subexpr))
                // cast pointer type, becasue we may need to cast void* onto returntype*.
                // TODO: check? (we don't check, we just cast for now)
                var cres = LLVMBuildPointerCast(g_builder, resPtr, type_to_llvm_abi_type(returnType), "")
                expr_res = cres
            elif expr.returnFlags.moveSemantics && expr.subexpr._type.isWorkhorseType
                // return <- blah // where blah is string and such
                var lres = LLVMBuildLoad2(g_builder, type_to_llvm_type(expr.subexpr._type), getE(expr.subexpr), "")
                expr_res = lres
            else
                expr_res = getE(expr.subexpr)

        var any_afb =  has_any_finally_block(false)
        if isCmres && any_afb
            var temp_res : LLVMOpaqueValue?
            var subPtrType = LLVMPointerType(type_to_llvm_type(returnType), 0u)
            at_function_entry <|
                temp_res = LLVMBuildAlloca(g_builder, subPtrType, "return_cmres_temp_")
                LLVMBuildStore(g_builder, LLVMConstNull(subPtrType), temp_res)
            LLVMBuildStore(g_builder, getE(expr.subexpr), temp_res)
            setE(expr.subexpr, temp_res)

        var has_afb : bool
        if expr_res != null
            has_afb = any_afb
            if has_afb
                var temp_res : LLVMOpaqueValue?
                at_function_entry <|
                    temp_res = LLVMBuildAlloca(g_builder, type_to_llvm_abi_type(returnType), "return_temp_")
                LLVMBuildStore(g_builder, expr_res, temp_res)
                expr_res = temp_res

        call_all_finally_blocks("return", false)
        build_exit(expr.at)

        if has_afb
            expr_res = LLVMBuildLoad2(g_builder, type_to_llvm_abi_type(returnType), expr_res, "return_temp_load_")

        build_epilogue()

        if !returnType.isVoid
            if isCmres
                if !expr.returnFlags.returnCMRES
                    if isCmres && any_afb
                        var subPtrType = LLVMPointerType(type_to_llvm_type(returnType), 0u)
                        var temp_load = LLVMBuildLoad2(g_builder, subPtrType, getE(expr.subexpr), "return_cmres_temp_load_")
                        setE(expr.subexpr, temp_load)
                    build_copy(get_cmres_param(),expr.subexpr)
                LLVMBuildRetVoid(g_builder)
            elif returnType.isPointer && (expr.subexpr is ExprConstPtr) // return null
                LLVMBuildRet(g_builder, LLVMConstPointerNull(type_to_llvm_abi_type(returnType)))
            elif returnType.isPointer
                LLVMBuildRet(g_builder, expr_res)
            elif expr.returnFlags.moveSemantics && expr.subexpr._type.isWorkhorseType
                LLVMBuildRet(g_builder, expr_res)
            else
                LLVMBuildRet(g_builder, expr_res)
        else
            LLVMBuildRetVoid(g_builder)

    def override visitExprReturn(expr:smart_ptr<ExprReturn>) : ExpressionPtr
        if thisBlock!=null
            visit_expr_return(expr, thisBlock.returnType, isCMRESType(thisBlock.returnType))
        elif thisFunc!=null
            visit_expr_return(expr, thisFunc.result, isCMRES(thisFunc))
        return expr

// ExprBreak
    def override preVisitExprBreak(expr:smart_ptr<ExprBreak>) : void
        call_all_finally_blocks("break", true)
        if callBlock!=null
            LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(),0u)), callBlock)
        var lblk = loop_stack |> back()
        LLVMBuildBr(g_builder, lblk.loop_end)

// ExprContinue
    def override preVisitExprContinue(expr:smart_ptr<ExprContinue>) : void
        call_all_finally_blocks("continue", true)
        if callBlock!=null
            LLVMBuildStore(g_builder, LLVMConstNull(LLVMPointerType(LLVMInt8Type(),0u)), callBlock)
        var lblk = loop_stack |> back()
        LLVMBuildBr(g_builder, lblk.loop_continue!=null ? lblk.loop_continue : lblk.loop_start)

// ExprMakeBlock
    def pre_make_block ( var expr:smart_ptr<ExprBlock>; func:Function?; captureType:LLVMOpaqueType? ) : string
        thisFunc = func
        thisBlock = get_ptr(expr)
        let mna = get_mangled_name(expr)
        let fnmna = "block_{intptr(expr)}_at_{int(expr.at.line)}{mna}"
        let isCmres = isCMRESType(expr._type)
        build_llvm_function_pair(fnmna, isCmres, thisBlock.arguments, thisBlock.returnType, captureType, expr.annotations)
        // function pair
        ffunc = LLVMGetNamedFunction(g_mod, "{fnmna} implementation")
        wfunc = LLVMGetNamedFunction(g_mod, fnmna)
        // build wrapper function
        build_wrapper_funcion(fnmna, thisBlock.arguments, thisBlock.returnType, isCmres, captureType, thisBlock.at)
        build_function_entry(expr.at, intptr(expr), thisBlock.blockFlags.hasMakeBlock, thisFunc.totalStackSize, thisBlock.arguments)
        process_function_hints(thisBlock.annotations, thisBlock.arguments)
        process_labels(expr)
        process_finally(expr)
        return fnmna

    def post_make_block ( var blk:smart_ptr<ExprBlock> )
        add_default_terminator(blk)
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        LLVMBuildBr(g_builder, function_body)
        thisBlock = null
        thisFunc = null

    def make_block_funcion(expr:smart_ptr<ExprMakeBlock>; capture:array<CapturedVariable>; first_field_index:int; captureType:LLVMOpaqueType? ) : tuple<func:LLVMOpaqueValue?;impl:LLVMOpaqueValue?>
        var astVisitor = new LlvmJitVisitor(expr.at, jit_context, false, g_di_builder, di_cu)
        var fmna : string
        unsafe
            astVisitor.adapter <- make_visitor(*astVisitor)
            fmna = (*astVisitor)->pre_make_block(reinterpret<smart_ptr<ExprBlock>> expr._block, thisFunc, captureType)
            var cblk = (*astVisitor)->before_function_entry()
            var carg = (*astVisitor)->get_capture_param()
            carg = LLVMBuildLoad2(astVisitor.g_builder, captureType, carg, "capture")
            for c,ai in capture,count()
                var arg_i = LLVMBuildExtractValue(astVisitor.g_builder, carg, uint(ai+first_field_index), "var_{c.variable.name}_index_{ai}_stindex_{ai+first_field_index}")
                if c.eref
                    var arg_t = LLVMPointerType(type_to_llvm_type(c.variable._type), 0u)
                    var arg_p = LLVMBuildAlloca(astVisitor.g_builder, arg_t, "var_{c.variable.name}_index_{ai}_stindex_{ai+first_field_index}_var")
                    LLVMBuildStore(astVisitor.g_builder, arg_i, arg_p)
                    (*astVisitor)->setV_ptr(c.variable, arg_p)
                else
                    (*astVisitor)->setV_ptr(c.variable, arg_i)
            (*astVisitor)->after_function_entry(cblk)
            visit(expr._block, astVisitor.adapter)
            (*astVisitor)->post_make_block(reinterpret<smart_ptr<ExprBlock>> expr._block)
        let result = LLVMGetNamedFunction(g_mod, fmna)
        let impl = LLVMGetNamedFunction(g_mod, "{fmna} implementation")
        astVisitor.adapter := null
        unsafe
            delete astVisitor
        return [[auto result,impl]]

    def override canVisitMakeBlockBody(expr:smart_ptr<ExprMakeBlock>) : bool
        return false

    def override preVisitExprMakeBlock(expr:smart_ptr<ExprMakeBlock>) : void
        var capture <- capture_block(expr._block)
        // build block type
        var blk_fields <- build_block_type()
        blk_fields |> push(LLVMArrayType(LLVMFloat4Type(), 8u))
        var first_field_index = length(blk_fields)
        for c in capture
            if c.variable._type.isGoodBlockType
                blk_fields |> push(type_to_llvm_type(c.variable._type))
            else
                blk_fields |> push(LLVMPointerType(type_to_llvm_type(c.variable._type), 0u))
        var this_block_type = LLVMStructType(blk_fields)
        // allocate block, set capture fields
        var blk : LLVMOpaqueValue?
        at_function_entry <|
            blk = LLVMBuildAlloca(g_builder, this_block_type, "block_at_{int(expr.at.line)}_")
        var blkv = LLVMGetUndef(this_block_type)
        for c,idx in capture,count()
            var v_ptr = getV_ptr(c.variable, false)
            if v_ptr == null
                v_ptr = getE_ptr(c.expression, false)
            // TODO:
            //  recapture block arguments
            if v_ptr == null
                if thisBlock != null
                    // its a function argument
                    for a in thisBlock.arguments
                        if a == c.variable
                            v_ptr = LLVMGetParam(ffunc, uint(c.expression.argumentIndex))
                            break
                elif thisFunc != null
                    // its a function argument
                    for a in thisFunc.arguments
                        if a == c.variable
                            v_ptr = LLVMGetParam(ffunc, uint(c.expression.argumentIndex))
                            break
            else
                /*
                    var a : int& = ...
                    invoke <| a // we capture ref as ref, unless its argument. then ref is ref already
                */
                if c.variable._type.flags.ref
                    v_ptr = LLVMBuildLoad2(g_builder, type_to_llvm_abi_type(c.variable._type), v_ptr, "")
                    c.eref = true

            if v_ptr == null
                failed_E(expr, "capture {c.variable.name} not found")
                return
            blkv = LLVMBuildInsertValue(g_builder, blkv, v_ptr, uint(idx + first_field_index), "capture_{c.variable.name}")
        LLVMBuildStore(g_builder, blkv, blk)

        // make block function
        var blk_make = make_block_funcion(expr, capture,first_field_index, this_block_type)

        // call jit_make_block
        var blk_ptr = LLVMBuildPointerCast(g_builder, blk, LLVMPointerType(g_t_block, 0u), "")
        let argStackTop = (expr._block as ExprBlock).stackTop
        let annotationData = (expr._block as ExprBlock).annotationData
        let null_ptr = LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(),0u))
        let func_ptr = LLVMBuildPointerCast(g_builder, blk_make.func, LLVMPointerType(LLVMVoidType(),0u), "")
        let impl_ptr = LLVMBuildPointerCast(g_builder, blk_make.impl, LLVMPointerType(LLVMVoidType(),0u), "")
        var params = [[auto
            blk_ptr;                                                    // blk
            LLVMConstInt(LLVMInt32Type(), uint64(argStackTop), 0);      // argStackTop
            LLVMConstInt(LLVMInt64Type(), uint64(annotationData), 0);   // annotation data
            func_ptr;                                                   // bodyNode
            impl_ptr;                                                   // jitImpl
            null_ptr;                                                   // TODO: funcInfo
            get_context_param()                                         // Context
        ]]
        var typ = g_fn_types[FN_JIT_MAKE_BLOCK]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_MAKE_BLOCK), params, "")
        setE(expr, blk_ptr)

// ExprMemZero
    def override visitExprMemZero(expr:smart_ptr<ExprMemZero>) : ExpressionPtr
        assume subexpr = expr.arguments[0]
        var ptr = getE(subexpr)
        ptr = LLVMBuildPointerCast(g_builder, ptr, LLVMPointerType(LLVMInt8Type(), 0u), "")
        assume sizeOf = subexpr._type.sizeOf
        assume alignOf = subexpr._type.alignOf
        LLVMBuildMemSet(g_builder, ptr, LLVMConstInt(LLVMInt8Type(), 0ul, 0), LLVMConstInt(LLVMInt32Type(), uint64(sizeOf), 0), uint(alignOf))
        return expr

// ExprTypeDecl
    def override visitExprTypeDecl(expr:smart_ptr<ExprTypeDecl>) : ExpressionPtr
        setE(expr, LLVMGetUndef(type_to_llvm_abi_type(expr._type)))
        return expr

// ExprConstPtr
    def override visitExprConstPtr(expr:smart_ptr<ExprConstPtr>) : ExpressionPtr
        setE(expr, LLVMConstPointerNull(LLVMPointerType(LLVMVoidType(),0u)))
        return expr

// ExprConstEnumeration
    def override visitExprConstEnumeration(expr:smart_ptr<ExprConstEnumeration>) : ExpressionPtr
        var value = find_enum_value(unsafe(reinterpret<EnumerationPtr> expr._type.enumType), string(expr.value))
        setE(expr, LLVMConstInt(base_type_to_llvm_type(expr._type.enumType.baseType), uint64(value), 0))
        return expr

// ExprConstBitfield
    def override visitExprConstBitfield(expr:smart_ptr<ExprConstBitfield>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt32Type(), uint64(expr.value), 0))
        return expr

// ExprConstInt8
    def override visitExprConstInt8(expr:smart_ptr<ExprConstInt8>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt8Type(), uint64(expr.value), 0))
        return expr

// ExprConstInt16
    def override visitExprConstInt16(expr:smart_ptr<ExprConstInt16>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt16Type(), uint64(expr.value), 0))
        return expr

// ExprConstInt64
    def override visitExprConstInt64(expr:smart_ptr<ExprConstInt64>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt64Type(), uint64(expr.value), 0))
        return expr

// ExprConstInt
    def override visitExprConstInt(expr:smart_ptr<ExprConstInt>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt32Type(), uint64(expr.value), 0))
        return expr

// ExprConstInt2
    def override visitExprConstInt2(expr:smart_ptr<ExprConstInt2>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        setE(expr, LLVMBuildInt2_xy(g_builder, x, y, "int2"))
        return expr

// ExprConstInt3
    def override visitExprConstInt3(expr:smart_ptr<ExprConstInt3>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        var z = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.z), 0)
        setE(expr, LLVMBuildInt3_xyz(g_builder, x, y, z, "int3"))
        return expr

// ExprConstInt4
    def override visitExprConstInt4(expr:smart_ptr<ExprConstInt4>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        var z = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.z), 0)
        var w = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.w), 0)
        setE(expr, LLVMBuildInt4_xyzw(g_builder, x, y, z, w, "int4"))
        return expr

// ExprConstUInt8
    def override visitExprConstUInt8(expr:smart_ptr<ExprConstUInt8>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt8Type(), uint64(expr.value), 0))
        return expr

// ExprConstUInt16
    def override visitExprConstUInt16(expr:smart_ptr<ExprConstUInt16>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt16Type(), uint64(expr.value), 0))
        return expr

// ExprConstUInt64
    def override visitExprConstUInt64(expr:smart_ptr<ExprConstUInt64>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt64Type(), uint64(expr.value), 0))
        return expr

// ExprConstUInt
    def override visitExprConstUInt(expr:smart_ptr<ExprConstUInt>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt32Type(), uint64(expr.value), 0))
        return expr

// ExprConstUInt2
    def override visitExprConstUInt2(expr:smart_ptr<ExprConstUInt2>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        setE(expr, LLVMBuildInt2_xy(g_builder, x, y, "uint2"))
        return expr

// ExprConstUInt3
    def override visitExprConstUInt3(expr:smart_ptr<ExprConstUInt3>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        var z = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.z), 0)
        setE(expr, LLVMBuildInt3_xyz(g_builder, x, y, z, "uint3"))
        return expr

// ExprConstUInt4
    def override visitExprConstUInt4(expr:smart_ptr<ExprConstUInt4>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        var z = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.z), 0)
        var w = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.w), 0)
        setE(expr, LLVMBuildInt4_xyzw(g_builder, x, y, z, w, "uint4"))
        return expr

// ExprConstRange
    def override visitExprConstRange(expr:smart_ptr<ExprConstRange>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        setE(expr, LLVMBuildInt2_xy(g_builder, x, y, "range"))
        return expr

// ExprConstURange
    def override visitExprConstURange(expr:smart_ptr<ExprConstURange>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt32Type(), uint64(expr.value.y), 0)
        setE(expr, LLVMBuildInt2_xy(g_builder, x, y, "urange"))
        return expr

// ExprConstRange64
    def override visitExprConstRange64(expr:smart_ptr<ExprConstRange64>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt64Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt64Type(), uint64(expr.value.y), 0)
        setE(expr, LLVMBuildAnyInt2_xy(g_builder, LLVMRange64Type(), x, y, "range64"))
        return expr

// ExprConstURange64
    def override visitExprConstURange64(expr:smart_ptr<ExprConstURange64>) : ExpressionPtr
        var x = LLVMConstInt(LLVMInt64Type(), uint64(expr.value.x), 0)
        var y = LLVMConstInt(LLVMInt64Type(), uint64(expr.value.y), 0)
        setE(expr, LLVMBuildAnyInt2_xy(g_builder, LLVMRange64Type(), x, y, "urange64"))
        return expr

// ExprConstBool
    def override visitExprConstBool(expr:smart_ptr<ExprConstBool>) : ExpressionPtr
        setE(expr, LLVMConstInt(LLVMInt1Type(), expr.value ? 1ul : 0ul, 0))
        return expr

// ExprConstFloat
    def override visitExprConstFloat(expr:smart_ptr<ExprConstFloat>) : ExpressionPtr
        setE(expr, LLVMConstReal(LLVMFloatType(), double(expr.value)))
        return expr

// ExprConstFloat2
    def override visitExprConstFloat2(expr:smart_ptr<ExprConstFloat2>) : ExpressionPtr
        var x = LLVMConstReal(LLVMFloatType(), double(expr.value.x))
        var y = LLVMConstReal(LLVMFloatType(), double(expr.value.y))
        setE(expr, LLVMBuildFloat2_xy(g_builder, x, y, "float2"))
        return expr

// ExprConstFloat3
    def override visitExprConstFloat3(expr:smart_ptr<ExprConstFloat3>) : ExpressionPtr
        var x = LLVMConstReal(LLVMFloatType(), double(expr.value.x))
        var y = LLVMConstReal(LLVMFloatType(), double(expr.value.y))
        var z = LLVMConstReal(LLVMFloatType(), double(expr.value.z))
        setE(expr, LLVMBuildFloat3_xyz(g_builder, x, y, z, "float3"))
        return expr

// ExprConstFloat4
    def override visitExprConstFloat4(expr:smart_ptr<ExprConstFloat4>) : ExpressionPtr
        var x = LLVMConstReal(LLVMFloatType(), double(expr.value.x))
        var y = LLVMConstReal(LLVMFloatType(), double(expr.value.y))
        var z = LLVMConstReal(LLVMFloatType(), double(expr.value.z))
        var w = LLVMConstReal(LLVMFloatType(), double(expr.value.w))
        setE(expr, LLVMBuildFloat4_xyzw(g_builder, x, y, z, w, "float4"))
        return expr

// ExprConstString
    def override visitExprConstString(expr:smart_ptr<ExprConstString>) : ExpressionPtr
        setE(expr, get_string_constant_ptr(g_builder, string(expr.value)))
        return expr

// ExprConstDouble
    def override visitExprConstDouble(expr:smart_ptr<ExprConstDouble>) : ExpressionPtr
        setE(expr, LLVMConstReal(LLVMDoubleType(), expr.value))
        return expr

// ExprFakeContext
    def override visitExprFakeContext(expr:smart_ptr<ExprFakeContext>) : ExpressionPtr
        setE(expr, LLVMGetParam(ffunc,0u))
        return expr

// ExprFakeLineInfo
    def override visitExprFakeLineInfo(expr:smart_ptr<ExprFakeLineInfo>) : ExpressionPtr
        setE(expr,get_line_info_ptr(expr.at))
        return expr

    def override visitExprTypeInfo(expr:smart_ptr<ExprTypeInfo>) : ExpressionPtr
        if expr.trait=="ast_typedecl"
            let mangled_name = expr.typeexpr |> get_mangled_name()
            let hash_value = hash(mangled_name)
            var params = [[auto
                LLVMConstInt(LLVMInt64Type(), uint64(hash_value), 0); // hash
                get_context_param();                                  // context
                get_line_info_ptr(expr.at)                            // lineinfo
            ]]
            var typ = g_fn_types[FN_JIT_AST_TYPEDECL]
            var cll = LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_AST_TYPEDECL), params, "")
            setE(expr, cll)
        elif expr.trait=="rtti_classinfo"
            var tinfo = make_type_info_structure(*jit_context,expr.typeexpr)
            var tptr = LLVMConstIntToPtr(LLVMConstInt(LLVMInt64Type(), intptr(tinfo), 0), LLVMPointerType(LLVMVoidType(), 0u))
            setE(expr, tptr)
        else
            failed_E(expr, "typeinfo({expr.trait}) is not supported yet")
        return expr

///////
// misc
///////

    def generate_fileinfo_constructor ()
        let functionType = LLVMFunctionType(LLVMVoidType(), null, 0u, 0)
        let constructor = LLVMAddFunctionWithType(g_mod, "fileinfo_constructor", functionType)

        let entry = LLVMAppendBasicBlock(constructor, "entry")
        LLVMPositionBuilderAtEnd(g_builder, entry)

        let void_ptr = LLVMPointerType(LLVMVoidType(), 0u)
        var firstElementPtr = g_builder |> LLVMBuildPointerCast(get_file_info_global(), void_ptr, "")
        var callArgs : array<LLVMValueRef>; callArgs |> push (firstElementPtr)

        let initializeFuncType = LLVMFunctionType(LLVMVoidType(), [[LLVMTypeRef[] LLVMPointerType(LLVMVoidType(), 0u) ]])
        let initializeFunc = LLVMAddFunctionWithType(g_mod, "jit_initialize_fileinfo", initializeFuncType)
        LLVMAddGlobalMapping(g_engine, initializeFunc, get_jit_initialize_fileinfo())

        var typ = g_fn_types["jit_initialize_fileinfo"]
        LLVMBuildCall2(g_builder, typ, initializeFunc, callArgs, "")
        LLVMBuildRetVoid(g_builder)

        return constructor

    def cast_from_vec4f_ptr ( v4f:LLVMOpaqueValue?; ptr_type:LLVMOpaqueType? ) : LLVMOpaqueValue?
        var tres : LLVMOpaqueValue?
        if typeinfo(sizeof type<void?>)==8
            var v2 = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(LLVMInt64Type(),2u), "")
            var tbit = LLVMBuildExtractElement(g_builder, v2, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            tres = LLVMBuildIntToPtr(g_builder, tbit, ptr_type, "vec4f_to_ptr")
        else
            var tbit = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            tres = LLVMBuildBitCast(g_builder, tbit, LLVMInt32Type(), "")
            tres = LLVMBuildIntToPtr(g_builder, tres, ptr_type, "vec4f_to_ptr")
        return tres

    def cast_from_vec4f_workhorse ( etype:TypeDeclPtr; expr:ExpressionPtr; v4f:LLVMOpaqueValue? )
        let bt = etype.baseType
        if bt==Type tInt || bt==Type tUInt || bt==Type tBitfield || bt==Type tEnumeration
            var tbit = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            return LLVMBuildBitCast(g_builder, tbit, LLVMInt32Type(), "vec4f_to_int")
        elif bt==Type tInt64 || bt==Type tUInt64
            var tvec = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(LLVMInt64Type(),2u), "")
            var tbit = LLVMBuildExtractElement(g_builder, tvec, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            return tbit
        elif bt==Type tInt8 || bt==Type tUInt8 || bt==Type tEnumeration8
            var tbit = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var tres = LLVMBuildBitCast(g_builder, tbit, LLVMInt32Type(), "vec4f_to_int")
            return  LLVMBuildTruncOrBitCast(g_builder, tres, LLVMInt8Type(), "int_to_int8")
        elif bt==Type tInt16 || bt==Type tUInt16 || bt==Type tEnumeration16
            var tbit = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var tres = LLVMBuildBitCast(g_builder, tbit, LLVMInt32Type(), "vec4f_to_int")
            return LLVMBuildTruncOrBitCast(g_builder, tres, LLVMInt16Type(), "int_to_int16")
        elif bt == Type tFloat
            return LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "vec4f_to_float")
        elif bt == Type tDouble
            var tvec = LLVMBuildBitCast(g_builder, v4f, LLVMVectorType(LLVMDoubleType(),2u), "")
            return LLVMBuildExtractElement(g_builder, tvec, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "vec4f_to_double")
        elif bt == Type tFloat2
            var vec = LLVMGetUndef(LLVMFloat2Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "vec4f_to_float2")
            return vec
        elif bt == Type tFloat3
            var vec = LLVMGetUndef(LLVMFloat3Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            var z = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, z, LLVMConstInt(LLVMInt32Type(), 2ul, 0), "vec4f_to_float3")
            return vec
        elif bt == Type tFloat4
            return v4f
        elif bt == Type tInt2 || bt == Type tUInt2 || bt == Type tRange || bt == Type tURange
            var vec = LLVMGetUndef(LLVMFloat2Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            return LLVMBuildBitCast(g_builder, vec, LLVMInt2Type(), "vec4f_to_int2")
        elif bt == Type tInt3 || bt == Type tUInt3
            var vec = LLVMGetUndef(LLVMFloat3Type())
            var x = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var y = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            var z = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 2ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, x, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, y, LLVMConstInt(LLVMInt32Type(), 1ul, 0), "")
            vec = LLVMBuildInsertElement(g_builder, vec, z, LLVMConstInt(LLVMInt32Type(), 2ul, 0), "vec4f_to_float3")
            return LLVMBuildBitCast(g_builder, vec, LLVMInt3Type(), "vec4f_to_int2")
        elif bt == Type tInt4 || bt == Type tUInt4
            return LLVMBuildBitCast(g_builder, v4f, LLVMInt4Type(), "vec4f_to_int4")
        elif bt == Type tBool
            var tbit = LLVMBuildExtractElement(g_builder, v4f, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            var tres = LLVMBuildBitCast(g_builder, tbit, LLVMInt32Type(), "")
            return LLVMBuildTruncOrBitCast(g_builder, tres, LLVMInt1Type(), "vec4f_to_bool")
        elif bt == Type tString || bt == Type tPointer
            var ptr_type = bt==Type tString ? get_type_string() : type_to_llvm_type(etype)
            return cast_from_vec4f_ptr(v4f, ptr_type)
        elif bt == Type tRange64 || bt == Type tURange64
            return LLVMBuildBitCast(g_builder, v4f, LLVMRange64Type(), "vec4f_to_range64")
        else
            failed_E(expr, "unsupported cast_from_vec4f cast {describe(etype)} workhorse")
        return null

    def cast_from_vec4f ( expr:ExpressionPtr; etype:TypeDeclPtr; v4f:LLVMOpaqueValue? )
        var tres : LLVMOpaqueValue?
        if etype.isWorkhorseType
            tres = cast_from_vec4f_workhorse(etype,expr,v4f)
        elif etype.isRef
            var ptr_type = LLVMPointerType(type_to_llvm_type(etype),0u)
            tres = cast_from_vec4f_ptr(v4f, ptr_type)
        elif etype.isHandle
            var inscope tp <- get_underlying_value_type(etype)
            if tp==null
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(etype)}")
            elif !tp.isWorkhorseType
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(etype)}")
            tres = cast_from_vec4f_workhorse(tp,expr,v4f)
        elif etype.isPointer
            var ptr_type = type_to_llvm_type(etype)
            tres = cast_from_vec4f_ptr(v4f, ptr_type)
        elif etype.baseType == Type tLambda
            tres = cast_from_vec4f_ptr(v4f, LLVMPointerType(LLVMVoidType(),0u))
        else
            failed_E(expr, "unsupported cast_from_vec4f type {describe(etype)}")
        if tres == null
            tres = LLVMGetUndef(LLVMFloat4Type())
        return tres

    def cast_ptr_to_vec4f ( tsrc:LLVMOpaqueValue? )
        static_if typeinfo(sizeof type<void?>)==8
            var vvec = LLVMGetUndef(LLVMVectorType(LLVMInt64Type(), 2u))
            var tval = LLVMBuildPtrToInt(g_builder, tsrc, LLVMInt64Type(), "")
            var vsrc = LLVMBuildInsertElement(g_builder, vvec, tval, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "")
            return LLVMBuildBitCast(g_builder, vsrc, LLVMFloat4Type(), "ptr_to_vec4f")
        else
            var tval = LLVMBuildPtrToInt(g_builder, tsrc, LLVMInt32Type(), "")
            var tbit = LLVMBuildBitCast(g_builder, tval, LLVMFloatType(), "")
            return LLVMBuildInsertElement(g_builder, vres, tbit, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "ptr_to_vec4f")

    def cast_to_vec4f ( etype:TypeDeclPtr; tsrc:LLVMOpaqueValue? )
        var vres = LLVMGetUndef(LLVMFloat4Type())
        var tres : LLVMOpaqueValue?
        if etype.isRef
            tres = cast_ptr_to_vec4f(tsrc)
        elif etype.isWorkhorseType
            let bt = etype.baseType
            if bt==Type tInt || bt==Type tUInt || bt==Type tBitfield || bt==Type tEnumeration
                var tbit = LLVMBuildBitCast(g_builder, tsrc, LLVMFloatType(), "tbit")
                tres = LLVMBuildInsertElement(g_builder, vres, tbit, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "return_result")
            elif bt==Type tBool || bt==Type tInt8 || bt==Type tInt16 || bt==Type tUInt8 || bt==Type tUInt16 || bt==Type tEnumeration8 || bt==Type tEnumeration16
                var tibit = LLVMBuildZExtOrBitCast(g_builder, tsrc, LLVMInt32Type(), "int_bit")
                var tbit = LLVMBuildBitCast(g_builder, tibit, LLVMFloatType(), "tbit")
                tres = LLVMBuildInsertElement(g_builder, vres, tbit, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "return_result")
            elif bt==Type tInt64 || bt==Type tUInt64
                var vvec = LLVMGetUndef(LLVMVectorType(LLVMInt64Type(), 2u))
                var vsrc = LLVMBuildInsertElement(g_builder, vvec, tsrc, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "int64_vec")
                tres = LLVMBuildBitCast(g_builder, vsrc, LLVMFloat4Type(), "return_result")
            elif bt==Type tFloat
                tres = LLVMBuildInsertElement(g_builder, vres, tsrc, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "return_result")
            elif bt==Type tDouble
                var vvec = LLVMGetUndef(LLVMVectorType(LLVMDoubleType(), 2u))
                var vsrc = LLVMBuildInsertElement(g_builder, vvec, tsrc, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "double_vec")
                tres = LLVMBuildBitCast(g_builder, vsrc, LLVMFloat4Type(), "return_result")
            elif bt==Type tInt2 || bt==Type tUInt2 || bt==Type tRange || bt==Type tURange
                var tbit = LLVMBuildShuffleVector(g_builder, tsrc, tsrc, [{auto 0;1;-1;-1}], "int2_vec")
                tres = LLVMBuildBitCast(g_builder, tbit, LLVMFloat4Type(), "return_result")
            elif bt==Type tInt3 || bt==Type tUInt3
                var tbit = LLVMBuildShuffleVector(g_builder, tsrc, tsrc, [{auto 0;1;2;-1}], "int3_vec")
                tres = LLVMBuildBitCast(g_builder, tbit, LLVMFloat4Type(), "return_result")
            elif bt==Type tInt4 || bt==Type tUInt4 || bt==Type tRange64 || bt==Type tURange64
                tres = LLVMBuildBitCast(g_builder, tsrc, LLVMFloat4Type(), "return_result")
            elif bt==Type tFloat2
                tres = LLVMBuildShuffleVector(g_builder, tsrc, tsrc, [{auto 0;1;-1;-1}], "return_result")
            elif bt==Type tFloat3
                tres = LLVMBuildShuffleVector(g_builder, tsrc, tsrc, [{auto 0;1;2;-1}], "return_result")
            elif bt==Type tFloat4
                tres = tsrc
            elif bt==Type tString || bt==Type tPointer
                tres = cast_ptr_to_vec4f(tsrc)
            else
                failed("unsupported workhorse cast_to_vec4f cast {describe(etype)} aka {etype.baseType}")
        elif etype.isFunction
            assert(typeinfo(sizeof type<void?>)==8)
            var vvec = LLVMGetUndef(LLVMVectorType(LLVMInt64Type(), 2u))
            var tbits = LLVMBuildExtractValue(g_builder, tsrc, 0u, "")
            tbits = LLVMBuildPtrToInt(g_builder, tbits, LLVMInt64Type(), "")
            var vsrc = LLVMBuildInsertElement(g_builder, vvec, tbits, LLVMConstInt(LLVMInt32Type(), 0ul, 0), "function")
            tres = LLVMBuildBitCast(g_builder, vsrc, LLVMFloat4Type(), "return_result")
        elif etype.isHandle
            var inscope tp <- get_underlying_value_type(etype)
            if tp==null
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(etype)}")
            elif !tp.isWorkhorseType
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(etype)}")
            tres = cast_to_vec4f(tp, tsrc)
        elif etype.baseType == Type tLambda
            tres = cast_ptr_to_vec4f(tsrc)
        elif etype.baseType == Type tPointer
            tres = cast_ptr_to_vec4f(tsrc)
        else
            failed("unsupported cast_to_vec4f type {describe(etype)}")
        return tres

    def get_capture_param : LLVMOpaqueValue?
        if thisBlock!=null
            return LLVMGetParam(ffunc,uint(thisBlock.arguments |> length))
        else
            panic("not in block")
            return null

    def get_context_param : LLVMOpaqueValue?
        if thisBlock!=null
            return LLVMGetParam(ffunc,uint(thisBlock.arguments |> length+1))
        elif thisFunc!=null
            return LLVMGetParam(ffunc,uint(thisFunc.arguments |> length))
        else
            panic("not in block or function")
            return null

    def get_cmres_param : LLVMOpaqueValue?
        if thisBlock!=null
            return LLVMGetParam(ffunc,uint(thisBlock.arguments |> length+2))
        elif thisFunc!=null
            return LLVMGetParam(ffunc,uint(thisFunc.arguments |> length+1))
        else
            panic("not in block or function")
            return null

    def build_select ( cond:LLVMOpaqueValue?; typ:LLVMOpaqueType?; if_true,if_false:block<LLVMOpaqueValue?> ) : LLVMOpaqueValue?
        if LLVMIsConstant(cond)!=0
            var val = LLVMConstIntGetZExtValue(cond)
            return val!=0ul ? invoke(if_true) : invoke(if_false)
        var check_true = LLVMAppendBasicBlock(ffunc,"check_true")
        var check_false = LLVMAppendBasicBlock(ffunc,"check_false")
        var check_end = LLVMAppendBasicBlock(ffunc,"check_end")
        var phi_true = LLVMAppendBasicBlock(ffunc, "phi_true")
        var phi_false = LLVMAppendBasicBlock(ffunc, "phi_false")
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
        LLVMAddIncoming(phi, [[auto pt; pf]], [[auto phi_true; phi_false]])
        return phi

    def build_exception ( message:string; at:LineInfo )
        var params = [[auto
            get_string_constant_ptr(g_builder, message);   // message
            get_context_param();
            get_line_info_ptr(at)
        ]]
        var typ = g_fn_types[FN_JIT_EXCEPTION]
        LLVMBuildCall2(g_builder, typ, LLVMGetNamedFunction(g_mod, FN_JIT_EXCEPTION), params, "")

    def at_function_entry ( blk:block )
        var current_block = LLVMGetInsertBlock(g_builder)
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        invoke(blk)
        LLVMPositionBuilderAtEnd(g_builder, current_block)

    def before_function_entry
        var current_block = LLVMGetInsertBlock(g_builder)
        LLVMPositionBuilderAtEnd(g_builder, function_entry)
        return current_block

    def after_function_entry(cblk:LLVMOpaqueBasicBlock?)
        LLVMPositionBuilderAtEnd(g_builder, cblk)

    def append_label_block ( blk:LLVMOpaqueBasicBlock? )
        subblocks[blk] = blk
        return blk

    def setE ( e : ExpressionPtr; ov:LLVMOpaqueValue? )
        e2v[get_ptr(e)] = ov

    def unsupported ( e:ExpressionPtr; message:string )
        failed_E(e, "unsupported expression {e.__rtti}; {message}")
        setE(e, LLVMGetUndef(LLVMInt32Type()))

    def getE_ptr ( e : Expression?; canFail:bool = true )
        var res : LLVMOpaqueValue?
        e2v |> get(e) <| $ ( val )
            res = val
        if res == null && canFail
            res = LLVMGetUndef(LLVMInt32Type())
            failed_E(e, "unresolved expression at {describe(e.at)}")
        return res

    def getE ( e : ExpressionPtr )
        var res : LLVMOpaqueValue?
        e2v |> get(get_ptr(e)) <| $ ( val )
            res = val
        if res == null
            res = LLVMGetUndef(LLVMInt32Type())
            failed_E(e, "unresolved expression {describe(e)}")
        return res

    def tryE ( e : ExpressionPtr; blk:block<LLVMOpaqueValue?> )
        var res : LLVMOpaqueValue?
        e2v |> get(get_ptr(e)) <| $ ( val )
            res = val
        if res == null
            res = invoke(blk)
        return res

    def setV ( e : VariablePtr; ov:LLVMOpaqueValue? )
        v2v[get_ptr(e)] = ov

    def setV_ptr ( e : Variable?; ov:LLVMOpaqueValue? )
        v2v[e] = ov

    def getV_ptr ( e : Variable?; canFail:bool = true )
        var res : LLVMOpaqueValue?
        v2v |> get(e) <| $ ( val )
            res = val
        if res == null && canFail
            res = LLVMGetUndef(LLVMInt32Type())
            failed_V(e, "unresolved variable at line {int(e.at.line)}")
        return res

    def getV ( e : VariablePtr )
        var res : LLVMOpaqueValue?
        v2v |> get(get_ptr(e)) <| $ ( val )
            res = val
        if res == null
            res = LLVMGetUndef(LLVMInt32Type())
            failed_V(e, "unresolved variable {describe(e)}")
        return res

    def tryGetV ( e : VariablePtr )
        var res : LLVMOpaqueValue?
        v2v |> get(get_ptr(e)) <| $ ( val )
            res = val
        return res

[macro_function]
def private get_llvm_function_type ( func : FunctionPtr )
    var arg_types : array<LLVMOpaqueType?>
    var res_type : LLVMOpaqueType?
    res_type = type_to_llvm_type(func.result)
    if func.result.flags.ref
        res_type = LLVMPointerType(res_type,0u)
    let cmres = isCMRES(func)
    if cmres
        arg_types |> push(LLVMPointerType(res_type,0u))
        res_type = LLVMVoidType()
    for arg in func.arguments
        arg_types |> push(type_to_llvm_abi_type(arg._type))
    if func.moreFlags.jitContextAndLineInfo
        arg_types |> push(LLVMPointerType(LLVMVoidType(),0u))   // context
        arg_types |> push(LLVMPointerType(LLVMVoidType(),0u))   // line info arg
    return LLVMFunctionType(res_type,arg_types)

[macro_function]
def private recreate_engine_extern_mappings ()
    // if we are here, then we are loading the dll and we will not recreate it
    for mod, externs in keys(modules2mnh), values(modules2mnh)
        for extern in externs
            let fn <- mod |> find_unique_function(extern)
            var func_ptr = g_dynamic_lib_handle |> get_function_address(extern)
            var func_addr = unsafe(reinterpret<void??> func_ptr)
            let FUNC_PTR = get_builtin_function_address(fn |> get_ptr())
            *func_addr = FUNC_PTR

[macro_function]
def private restore_modules2mnh ( )
    var mod_count = fetch_uint64_from_dll("mod_count")
    for i in mod_count |> range64()
        var mod_name = fetch_string_from_dll("mod_name_{i}")
        var mod_extern_cnt = fetch_uint64_from_dll("mod_extern_cnt_{mod_name}")
        var externs : array<string>
        for j in mod_extern_cnt |> range64()
            var extern_name = fetch_string_from_dll("mod_extern_{mod_name}_{j}")
            externs |> push (extern_name)
        let module_ = find_module(mod_name)
        modules2mnh[module_] <- externs

[macro_function]
def private restore_fileinfo_dummy ( )
    var sym = g_dynamic_lib_handle |> get_function_address("fileinfo_dummy")
    jit_initialize_fileinfo(sym)

def private create_fileinfo ( addr_ : void? )
    assert(g_dynamic_lib_handle != null, "g_dynamic_lib_handle is null")
    var sym = g_dynamic_lib_handle |> get_function_address("fileinfo_at_{addr_}")
    var sym_name = g_dynamic_lib_handle |> get_function_address("fileinfo_at_{addr_}_name")
    var val = unsafe(reinterpret<FileInfo?> (sym))
    var name = unsafe(reinterpret<string> (sym_name))
    recreate_fileinfo_name(val, name)
    return val

[macro_function]
def private fetch_lineinfo_from_dll ( name : string )
    assert(g_dynamic_lib_handle != null, "g_dynamic_lib_handle is null")
    var sym = g_dynamic_lib_handle |> get_function_address(name)
    var val = unsafe(reinterpret<LineInfo?> (sym))
    let fi = val.fileInfo
    if created_fileinfos[fi] == null
        created_fileinfos[fi] = create_fileinfo(unsafe(reinterpret<void?> fi))
    val.fileInfo = created_fileinfos[fi]
    return *val

[macro_function]
def private fetch_uint64_from_dll ( name : string )
    assert(g_dynamic_lib_handle != null, "g_dynamic_lib_handle is null")
    var sym = g_dynamic_lib_handle |> get_function_address(name)
    var val = unsafe(reinterpret<uint64?> (sym))
    return *val


[macro_function]
def private fetch_string_from_dll ( name : string )
    assert(g_dynamic_lib_handle != null, "g_dynamic_lib_handle is null")
    var sym = g_dynamic_lib_handle |> get_function_address(name)
    var val = unsafe(reinterpret<string?> (sym))
    return *val


[macro_function]
def private save_modules2mnh ( )
    var mod_count : uint64 = 0 |> uint64()
    for mod, externs in keys(modules2mnh), values(modules2mnh)
        mod_count += 1 |> uint64()
        var mod_extern_count : uint64 = externs |> length() |> uint64()
        add_uint64_global_variable("mod_extern_cnt_{mod.name}", mod_extern_count)
        add_string_global_variable("mod_name_{mod_count}", mod.name |> string())
        var extern_count : uint64 = 0 |> uint64()
        for extern in externs
            add_string_global_variable("mod_extern_{mod.name}_{extern_count}", extern)
            extern_count += 1 |> uint64()
    add_uint64_global_variable("mod_count", mod_count)

[macro_function]
def private add_string_global_variable ( name : string; value : string )
    let extern_name = build_string_constant(value)
    var global_var = add_global_variable(name, LLVMTypeOf(extern_name), extern_name)
    LLVMSetGlobalConstant(global_var, 1)
    return global_var

[macro_function]
def private add_uint64_global_variable ( name : string; init : uint64 )
    return add_global_variable(name, LLVMInt64Type(), LLVMConstInt(LLVMInt64Type(), init, 0))

[macro_function]
def private add_global_variable ( name : string; type_ : LLVMOpaqueType?; initializer : LLVMOpaqueValue? ): LLVMOpaqueValue?
    let globalVar = LLVMAddGlobal(g_mod, type_, name)
    LLVMSetLinkage(globalVar, LLVMLinkage LLVMDLLExportLinkage)
    LLVMSetDLLStorageClass(globalVar, LLVMDLLStorageClass LLVMDLLExportStorageClass)
    globalVar |> LLVMSetInitializer <| initializer
    return globalVar

[macro_function]
def private add_constructor_function ( func: LLVMOpaqueValue? )
    let priority = 0 |> uint64()
    let funcType = LLVMTypeOf(func)
    let dataPointerType = LLVMPointerType(LLVMInt8Type(), 0u)
    var ctorTypes = [[auto LLVMInt32Type(); funcType; dataPointerType]]

    var ctorFields = [[auto LLVMConstInt(LLVMInt32Type(), priority, 0); func; LLVMConstPointerNull(dataPointerType)]]


    let ctorStructType = LLVMStructType(unsafe(addr(ctorTypes[0])), 3u, 0)

    var ctorStruct = LLVMConstNamedStruct(ctorStructType, unsafe(addr(ctorFields[0])), 3u)
    let ctorArray = LLVMAddGlobal(g_mod, LLVMArrayType(ctorStructType, 1u), "llvm.global_ctors")
    LLVMSetLinkage(ctorArray, LLVMLinkage LLVMAppendingLinkage)
    LLVMSetInitializer(ctorArray, LLVMConstArray(ctorStructType, unsafe(addr(ctorStruct)), 1u))

[macro_function]
def private generate_llvm ( ctx:Context?; fn : FunctionPtr )
    return if is_in_completion() || is_compiling_macros() || is_folding()
    if g_dynamic_lib_handle != null
        var dll_function_aot_hash = g_dynamic_lib_handle |> get_function_address <| "{get_mangled_name(fn)} hash"
        // debug("dll_function_aot_hash: {dll_function_aot_hash}")
        var written_hash = unsafe(reinterpret<uint64?> (dll_function_aot_hash))
        if written_hash != null && get_function_aot_hash(fn |> get_ptr) == *written_hash
            debug("function {get_mangled_name(fn)} taken from the dll with aot_hash {*written_hash}")
            return
    g_loading_from_dll = false
    let globalVar = LLVMAddGlobal(g_mod, LLVMInt64Type(), "{get_mangled_name(fn)} hash")
    LLVMSetLinkage(globalVar, LLVMLinkage LLVMDLLExportLinkage)
    LLVMSetDLLStorageClass(globalVar, LLVMDLLStorageClass LLVMDLLExportStorageClass)
    let aot_hash : uint64 = get_function_aot_hash(fn |> get_ptr)
    // debug("initializer hash for {get_mangled_name(fn)}: {aot_hash}")
    globalVar |> LLVMSetInitializer(LLVMConstInt(LLVMInt64Type(), aot_hash, 0))
    var astVisitor = new LlvmJitVisitor(fn.at, ctx, true, null, null)
    unsafe
        astVisitor.adapter <- make_visitor(*astVisitor)
    visit(fn, astVisitor.adapter)
    if length(g_errors) > 0
        let errors = g_errors |> join("\n")
        delete g_errors
        to_log(LOG_ERROR,"LLVM JIT FAILED:\n{errors}\n")
        g_failed = true
    astVisitor.adapter := null
    unsafe
        delete astVisitor

[macro_function]
def private generate_llvm_code ( fmna:string )
    return null if is_in_completion() || is_compiling_macros() || is_folding()
    return null if g_failed
    var code = LLVMGetFunctionAddress(g_engine, fmna)
    return unsafe(reinterpret<void?> code) if code != 0ul
    if g_dynamic_lib_handle != null
        let result = g_dynamic_lib_handle |> get_function_address(fmna)
        if result == null
            to_log(LOG_ERROR, "LLVM JIT FAILED: {fmna}\n")
        return result
    to_log(LOG_ERROR, "LLVM JIT FAILED: {fmna}\n")
    return null

[macro_function]
def private optimize_llvm_module
    return if is_in_completion() || is_compiling_macros() || is_folding()
    return if g_failed || !LLVM_ENABLE_OPT_PASS
    var pmb = LLVMPassManagerBuilderCreate()
    LLVMPassManagerBuilderSetOptLevel(pmb, LLVM_OPT_LEVEL)
    LLVMPassManagerBuilderSetSizeLevel(pmb, LLVM_SIZE_LEVEL)
    LLVMPassManagerBuilderUseInlinerWithThreshold(pmb, LLVM_INLINE_THRESHOLD)
    var pm = LLVMCreatePassManager()
    LLVMPassManagerBuilderPopulateModulePassManager(pmb, pm)
    // LLVMAddLoopVectorizePass(pm)
    // LLVMAddSLPVectorizePass(pm)
    // LLVMPassManagerBuilderPopulateLTOPassManager(pmb, pm, 1, 1)
    LLVMRunPassManager(pm,g_mod)
    LLVMDisposePassManager(pm)
    LLVMPassManagerBuilderDispose(pmb)

class DisableJitVisitor : AstVisitor
    disable : bool
    def TypeInfoVisitor
        disable = false
    def override preVisitExprTypeInfo(expr:smart_ptr<ExprTypeInfo>) : void
        if expr.trait=="ast_typedecl" || expr.trait=="rtti_classinfo"
            pass
        else
            disable = true

[simulate_macro(name="jit_llvm")]
class JIT_LLVM : AstSimulateMacro
    def override simulate ( prog:Program?; var ctx:Context? ) : bool
        return true if is_in_completion() || is_compiling_macros() || is_folding()
        if !LLVM_JIT_ENABLED
            return true
        if !jit_enabled()
            error("jit is disabled. use `-jit` option for daScript, or enable jit via CodeOfPolicies")
            return false
        var funcs : array<FunctionPtr>
        var disableJitVisitor = new DisableJitVisitor()
        var inscope disableJitVisitorAdapter <- make_visitor(*disableJitVisitor)
        prog |> for_each_module <| $ ( mod )
            mod |> for_each_function("") <| $ ( fun )
                var rqj = fun.moreFlags.requestJit
                fun.moreFlags &= ~MoreFunctionFlags requestJit
                if fun.flags.used && (LLVM_JIT_ALL_FUNCTIONS || rqj)
                    if ! fun.moreFlags.requestNoJit
                        disableJitVisitor.disable = false
                        fun |> visit(disableJitVisitorAdapter)
                        if ( !disableJitVisitor.disable )
                            fun.moreFlags |= MoreFunctionFlags requestJit
                            funcs |> emplace(fun)
                        else
                            print("LLVM JIT: disabled {fun.name}\n")
        unsafe
            delete disableJitVisitor
        if !empty(funcs)
            let totalTime = ref_time_ticks()
            init_jit()
            var fake_visitor = new LlvmJitVisitor(funcs[0].at, ctx, false, null, null)
            for fun in funcs
                fake_visitor->add_llvm_functions(fun)
            if LLVM_ENABLE_DLL_LOADING
                // initialize fileinfo
                fake_visitor->generate_fileinfo_constructor() |> add_constructor_function()
                g_dynamic_lib_handle = load_dynamic_library(JIT_DLL_OUTPUT)
            unsafe
                delete fake_visitor
            for fun in funcs
                generate_llvm(ctx, fun)
            if !LLVMVerifyModule(g_mod, LLVMVerifierFailureAction LLVMPrintMessageAction, false)
                g_failed = true
            optimize_llvm_module()
            if LLVM_ENABLE_DLL_LOADING
                if g_dynamic_lib_handle != null
                    if g_loading_from_dll // had no problems with changed semantic hash
                        restore_modules2mnh()
                        recreate_engine_extern_mappings()
                    else
                        save_modules2mnh()
                        g_dynamic_lib_handle |> close_dynamic_library()
                        g_dynamic_lib_handle = null
            for fun in funcs
                var mangled_name = get_mangled_name(fun)
                let MNH = hash(mangled_name)
                var simfun = get_function_by_mangled_name_hash(MNH, *ctx)
                var gen = generate_llvm_code(mangled_name)
                unsafe
                    if gen != null
                        gen |> instrument_jit(simfun, *ctx)
                    else
                        remove_jit(simfun)
            finalize_jit()
            if LLVM_JIT_LOG
                let totalDt = double(get_time_usec(totalTime)) /1000000.0lf
                to_log(LOG_INFO, "LLVM JIT: {length(funcs)} functions in {totalDt} sec\n")
        return true


