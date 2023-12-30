options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module llvm_jit_common shared private

require llvm/llvm_boost
require llvm_config

require daslib/ast_boost
require daslib/strings
require daslib/defer
require daslib/enum_trait
require fio
require jit

let public LLVM_JIT_ENABLED = llvm_config::llvm_jit_enabled()
let public LLVM_JIT_ALL_FUNCTIONS = llvm_config::llvm_jit_all_functions()

let public LLVM_DEBUG_RESULT = llvm_config::llvm_debug_result()
let public LLVM_DEBUG_EVERYTHING = llvm_config::llvm_debug_everything()

let public LLVM_JIT_LOG = false || LLVM_DEBUG_EVERYTHING
let public LLVM_DEBUG_INFO = false || LLVM_DEBUG_EVERYTHING

let public LLVM_JIT_ALWAYS_SOLID = llvm_config::llvm_jit_always_solid()
let public LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = (
    llvm_config::llvm_jit_allow_unaligned_vector_read_out_of_bounds()
)

let public LLVM_ENABLE_OPT_PASS = LLVM_DEBUG_EVERYTHING ? false : true
let public LLVM_OPT_LEVEL = LLVM_DEBUG_EVERYTHING ? 0u : 2u
let public LLVM_SIZE_LEVEL = LLVM_DEBUG_EVERYTHING ? 0u : 0u
let public LLVM_JIT_OPT_LEVEL = LLVM_DEBUG_EVERYTHING ? 0u : 2u
let public LLVM_LOG_RESULT = LLVM_DEBUG_EVERYTHING || LLVM_DEBUG_RESULT
let public LLVM_INLINE_THRESHOLD = 1024u // 225u

let public FN_JIT_EXCEPTION = "`jit`exception`"
let public FN_JIT_CALL_OR_FASTCALL = "`jit`call_or_fastcall`"
let public FN_JIT_CALL_WITH_CMRES = "`jit`call_with_cmres`"
let public FN_JIT_INVOKE_BLOCK = "`jit`invoke_block`"
let public FN_JIT_INVOKE_BLOCK_WITH_CMRES = "`jit`invoke_block_with_cmres`"
let public FN_JIT_STRING_BUILDER = "`jit`string_builder`"
let public FN_JIT_GET_GLOBAL_MNH = "`jit`get_global_mnh`"
let public FN_JIT_ALLOC_HEAP = "`jit`alloc_heap`"
let public FN_JIT_ALLOC_PERSISTENT = "`jit`alloc_persistent`"
let public FN_JIT_FREE_HEAP = "`jit`free_heap`"
let public FN_JIT_FREE_PERSISTENT = "`jit`free_persistent`"
let public FN_JIT_ARRAY_LOCK = "`jit`array_lock`"
let public FN_JIT_ARRAY_UNLOCK = "`jit`array_unlock`"
let public FN_JIT_STR_CMP = "`jit`str_cmp`"
let public FN_JIT_PROLOGUE = "`jit`prologue`"
let public FN_JIT_EPILOGUE = "`jit`epilogue`"
let public FN_JIT_MAKE_BLOCK = "`jit`make_block`"
let public FN_JIT_DEBUG = "`jit`debug`"
let public FN_JIT_ITERATOR_ITERATE = "`jit`iterator_iterate`"
let public FN_JIT_ITERATOR_DELETE = "`jit`iterator_delete`"
let public FN_JIT_ITERATOR_CLOSE = "`jit`iterator_close`"

struct JitEngine
    mod : LLVMOpaqueModule?
    engine : LLVMExecutionEngineRef

var private g_all_engines : array<JitEngine>

var public g_mod : LLVMOpaqueModule?
var public g_engine : LLVMExecutionEngineRef
var public g_failed : bool = false
var public g_errors : array<string>

var public g_t_simFunction : LLVMOpaqueType?
var public g_t_function : LLVMOpaqueType?
var public g_t_lambda : LLVMOpaqueType?
var public g_t_block : LLVMOpaqueType?
var public g_t_stack_state : LLVMOpaqueType?
var public g_t_iterator : LLVMOpaqueType?
var public g_t_sequence : LLVMOpaqueType?

var g_str2v : table<string; LLVMOpaqueValue?>

enum public JIT_ARRAY
    DATA
    SIZE
    CAPACITY
    LOOK
    FLAGS

enum public JIT_TABLE
    DATA
    SIZE
    CAPACITY
    LOOK
    FLAGS
    KEYS
    HASHES
    MAX_LOOKUPS
    SHIFT

enum public JIT_SIMFUNCTION
    NAME
    MANGLED_NAME
    CODE
    DEBUG_INFO
    MANGLED_NAME_HASH
    AOT_FUNCTION
    STACK_SIZE
    FLAGS

enum public JIT_FUNCTION
    SIM_FUNCTION

enum public JIT_LAMBDA
    EVAL
    FINALIZER

enum public JIT_BLOCK
    STACK_OFFSET
    ARGUMENTS_OFFSET
    BODY
    AOT_FUNCTION
    JIT_FUNCTION
    FUNCTION_ARGUMENTS
    INFO

enum public JIT_SEQUENCE
    ITERATOR

def public FN_JIT_TABLE_AT(t:Type)
    return "`jit`table_at`{t}`"

def public FN_JIT_TABLE_ERASE(t:Type)
    return "`jit`table_erase`{t}`"

def public FN_JIT_TABLE_FIND(t:Type)
    return "`jit`table_find`{t}`"


[macro_function]
def public init_jit
    return if is_in_completion() || is_compiling_macros()
    return if g_mod != null    // prevent double init
    LLVMContextSetOpaquePointers(LLVMGetGlobalContext(), 0)
    if LLVM_DEBUG_INFO
        set_context_diagnostics_to_log(LLVMGetGlobalContext())
    g_failed = false
    g_mod = LLVMModuleCreateWithName("llvm_jit_module")
    LLVMLinkInMCJIT()
    LLVMInitializeNativeTarget()
    LLVMInitializeNativeAsmPrinter()
    LLVMInitializeNativeAsmParser()
    LLVMCreateJITCompilerForModule(g_engine, g_mod, LLVM_JIT_OPT_LEVEL)
    static_if LLVM_DEBUG_EVERYTHING
        let hostcpu = LLVMGetHostCPUName()
        let features = LLVMGetHostCPUFeatures()
        print("LLVM host cpu: {hostcpu}\nLLVM host cpu features: {features}\n")
        LLVMDisposeMessage(hostcpu)
        LLVMDisposeMessage(features)
    var noreturn = LLVMGetEnumAttribute("noreturn")
    var readonly = LLVMGetEnumAttribute("readonly")
    var nounwind = LLVMGetEnumAttribute("nounwind")
    var willreturn = LLVMGetEnumAttribute("willreturn")
    var nocapture = LLVMGetEnumAttribute("nocapture")
    build_jit_types()
    // add default functions
    var jit_exception = LLVMAddFunction(g_mod, FN_JIT_EXCEPTION,    // jit_exception(text,context *)
        LLVMFunctionType(LLVMVoidType(),
            [[LLVMTypeRef get_type_string(); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_exception, get_jit_exception())
    LLVMAddAttributeToFunction(jit_exception, noreturn)
    LLVMAddAttributeToFunctionArgumentRange(jit_exception, urange(0,2), nocapture)
    // jit_call_or_fastcall(func,args *,context *)
    var jit_call_or_fastcall = LLVMAddFunction(g_mod, FN_JIT_CALL_OR_FASTCALL,
        LLVMFunctionType(LLVMFloat4Type(),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMFloat4Type(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_call_or_fastcall, get_jit_call_or_fastcall())
    LLVMAddAttributeToFunctionArgumentRange(jit_call_or_fastcall, urange(0,3), nocapture)
    LLVMAddAttributesToFunction(jit_call_or_fastcall, [[auto nounwind; willreturn]])
    // jit_call_with_cmres(func,args *,cmres *,context *)
    var jit_call_with_cmres = LLVMAddFunction(g_mod, FN_JIT_CALL_WITH_CMRES,
        LLVMFunctionType(LLVMFloat4Type(),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMFloat4Type(),0u); LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_call_with_cmres, get_jit_call_with_cmres())
    LLVMAddAttributeToFunctionArgumentRange(jit_call_with_cmres, urange(0,4), nocapture)
    LLVMAddAttributesToFunction(jit_call_with_cmres, [[auto nounwind; willreturn]])
    // vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context )
    var jit_invoke_block = LLVMAddFunction(g_mod, FN_JIT_INVOKE_BLOCK,
        LLVMFunctionType(LLVMFloat4Type(),
            [[LLVMTypeRef LLVMPointerType(g_t_block,0u); LLVMPointerType(LLVMFloat4Type(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_invoke_block, get_jit_invoke_block())
    LLVMAddAttributeToFunctionArgumentRange(jit_invoke_block, urange(0,3), nocapture)
    LLVMAddAttributesToFunction(jit_invoke_block, [[auto nounwind; willreturn]])
    // vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context )
    var jit_invoke_block_with_cmres = LLVMAddFunction(g_mod, FN_JIT_INVOKE_BLOCK_WITH_CMRES,
        LLVMFunctionType(LLVMFloat4Type(),
            [[LLVMTypeRef LLVMPointerType(g_t_block,0u); LLVMPointerType(LLVMFloat4Type(),0u); LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_invoke_block_with_cmres, get_jit_invoke_block_with_cmres())
    LLVMAddAttributeToFunctionArgumentRange(jit_invoke_block_with_cmres, urange(0,4), nocapture)
    LLVMAddAttributesToFunction(jit_invoke_block_with_cmres, [[auto nounwind; willreturn]])
    // jit_string_builder ( context *, node *, args * )
    var jit_string_builder = LLVMAddFunction(g_mod, FN_JIT_STRING_BUILDER,
        LLVMFunctionType(get_type_string(),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMFloat4Type(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_string_builder, get_jit_string_builder())
    LLVMAddAttributesToFunction(jit_string_builder, [[auto nounwind; willreturn; readonly]])
    LLVMAddAttributeToFunctionArgumentRange(jit_string_builder, urange(0,3), nocapture)
    // jit_get_global_mnh ( uint64_t mnh, context * )
    var jit_get_global_mnh = LLVMAddFunction(g_mod, FN_JIT_GET_GLOBAL_MNH,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMInt64Type(); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_get_global_mnh, get_jit_get_global_mnh())
    LLVMAddAttributesToFunction(jit_get_global_mnh, [[auto nounwind; willreturn; readonly]])
    LLVMAddAttributeToFunctionArgument(jit_get_global_mnh, 1u, nocapture)
    // jit_alloc_heap ( uint64_t mnh, context * )
    var jit_alloc_heap = LLVMAddFunction(g_mod, FN_JIT_ALLOC_HEAP,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMInt32Type(); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_alloc_heap, get_jit_alloc_heap())
    LLVMAddAttributesToFunction(jit_alloc_heap, [[auto nounwind; willreturn; readonly]])
    LLVMAddAttributeToFunctionArgument(jit_alloc_heap, 1u, nocapture)
    // jit_alloc_persistent ( uint64_t mnh, context * )
    var jit_alloc_persistent = LLVMAddFunction(g_mod, FN_JIT_ALLOC_PERSISTENT,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMInt32Type(); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_alloc_persistent, get_jit_alloc_persistent())
    LLVMAddAttributesToFunction(jit_alloc_persistent, [[auto nounwind; willreturn; readonly]])
    LLVMAddAttributeToFunctionArgument(jit_alloc_persistent, 1u, nocapture)
    // void jit_free_heap ( void * bytes, uint32_t size, Context * context )
    var jit_free_heap = LLVMAddFunction(g_mod, FN_JIT_FREE_HEAP,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMInt32Type(); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_free_heap, get_jit_free_heap())
    LLVMAddAttributesToFunction(jit_free_heap, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgument(jit_free_heap, 0u, nocapture)
    LLVMAddAttributeToFunctionArgument(jit_free_heap, 2u, nocapture)
    // void jit_free_persistent ( void * bytes, Context * context )
    var jit_free_persistent = LLVMAddFunction(g_mod, FN_JIT_FREE_PERSISTENT,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_free_persistent, get_jit_free_persistent())
    LLVMAddAttributesToFunction(jit_free_persistent, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_free_persistent, urange(0,2), nocapture)
    // void jit_array_lock ( Array & array, Context * context )
    var jit_array_lock = LLVMAddFunction(g_mod, FN_JIT_ARRAY_LOCK,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_array_lock, get_jit_array_lock())
    LLVMAddAttributesToFunction(jit_array_lock, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_array_lock, urange(0,2), nocapture)
    // void jit_array_unlock ( Array & array, Context * context )
    var jit_array_unlock = LLVMAddFunction(g_mod, FN_JIT_ARRAY_UNLOCK,
        LLVMFunctionType(LLVMPointerType(LLVMVoidType(),0u),
            [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_array_unlock, get_jit_array_unlock())
    LLVMAddAttributesToFunction(jit_array_unlock, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_array_unlock, urange(0,2), nocapture)
    // table functions
    for t in get_workhorse_types()
        // int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context )
        var jit_table_at = LLVMAddFunction(g_mod, FN_JIT_TABLE_AT(t),
            LLVMFunctionType(LLVMInt32Type(),   // int32_t result
                [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u);   // table ptr
                base_type_to_llvm_type(t);                          // key
                LLVMInt32Type();                                    // valueTypeSize
                LLVMPointerType(LLVMVoidType(),0u)                  // context
                ]]))
        LLVMAddGlobalMapping(g_engine, jit_table_at, get_jit_table_at(unsafe(reinterpret<int> t)))
        LLVMAddAttributesToFunction(jit_table_at, [[auto nounwind; willreturn]])
        LLVMAddAttributeToFunctionArgument(jit_table_at, 0u, nocapture)
        LLVMAddAttributeToFunctionArgument(jit_table_at, 3u, nocapture)
        // int32_t jit_table_find ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context )
        var jit_table_find = LLVMAddFunction(g_mod, FN_JIT_TABLE_FIND(t),
            LLVMFunctionType(LLVMInt32Type(),   // int32_t result
                [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u);   // table ptr
                base_type_to_llvm_type(t);                          // key
                LLVMInt32Type();                                    // valueTypeSize
                LLVMPointerType(LLVMVoidType(),0u)                  // context
                ]]))
        LLVMAddGlobalMapping(g_engine, jit_table_find, get_jit_table_find(unsafe(reinterpret<int> t)))
        LLVMAddAttributesToFunction(jit_table_find, [[auto nounwind; willreturn; readonly]])
        LLVMAddAttributeToFunctionArgument(jit_table_find, 0u, nocapture)
        LLVMAddAttributeToFunctionArgument(jit_table_find, 3u, nocapture)
        // bool jit_table_erase ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context )
        var jit_table_erase = LLVMAddFunction(g_mod, FN_JIT_TABLE_ERASE(t),
            LLVMFunctionType(LLVMInt1Type(),                        // bool result
                [[LLVMTypeRef LLVMPointerType(LLVMVoidType(),0u);   // table ptr
                base_type_to_llvm_type(t);                          // key
                LLVMInt32Type();                                    // valueTypeSize
                LLVMPointerType(LLVMVoidType(),0u)                  // context
                ]]))
        LLVMAddGlobalMapping(g_engine, jit_table_erase, get_jit_table_erase(unsafe(reinterpret<int> t)))
        LLVMAddAttributesToFunction(jit_table_erase, [[auto nounwind; willreturn]])
        LLVMAddAttributeToFunctionArgument(jit_table_erase, 0u, nocapture)
        LLVMAddAttributeToFunctionArgument(jit_table_erase, 3u, nocapture)
    // int jit_str_cmp ( char * a, char * b )
    var jit_str_cmp = LLVMAddFunction(g_mod, FN_JIT_STR_CMP,
        LLVMFunctionType(LLVMInt32Type(),
            [[LLVMTypeRef LLVMPointerType(LLVMInt8Type(),0u); LLVMPointerType(LLVMInt8Type(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_str_cmp, get_jit_str_cmp())
    LLVMAddAttributesToFunction(jit_str_cmp, [[auto nounwind; willreturn; readonly]])
    LLVMAddAttributeToFunctionArgumentRange(jit_str_cmp, urange(0,2), nocapture)
    // void jit_prologue ( int32_t stackSize, JitStackState * stackState, Context * context )
    var jit_prologue = LLVMAddFunction(g_mod, FN_JIT_PROLOGUE,
        LLVMFunctionType(LLVMVoidType(),
            [[LLVMTypeRef LLVMInt32Type(); LLVMPointerType(g_t_stack_state,0u); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_prologue, get_jit_prologue())
    LLVMAddAttributesToFunction(jit_prologue, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_prologue, urange(1,3), nocapture)
    // void jit_epilogue ( JitStackState * stackState, Context * context )
    var jit_epilogue = LLVMAddFunction(g_mod, FN_JIT_EPILOGUE,
        LLVMFunctionType(LLVMVoidType(),
            [[LLVMTypeRef LLVMPointerType(g_t_stack_state,0u); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_epilogue, get_jit_epilogue())
    LLVMAddAttributesToFunction(jit_epilogue, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_epilogue, urange(0,2), nocapture)
    // void jit_make_block ( Block * blk, int32_t argStackTop, void * bodyNode, void * jitImpl, void * funcInfo, Context * context )
    var jit_make_block = LLVMAddFunction(g_mod, FN_JIT_MAKE_BLOCK,
        LLVMFunctionType(LLVMVoidType(),
            [[LLVMTypeRef LLVMPointerType(g_t_block,0u); LLVMInt32Type(); LLVMPointerType(LLVMVoidType(),0u);
                LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u) ]]))
    LLVMAddGlobalMapping(g_engine, jit_make_block, get_jit_make_block())
    LLVMAddAttributesToFunction(jit_make_block, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgument(jit_make_block, 0u, nocapture)
    LLVMAddAttributeToFunctionArgumentRange(jit_make_block, urange(2,6), nocapture)
    // void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * )
    var jit_debug = LLVMAddFunction(g_mod, FN_JIT_DEBUG,
        LLVMFunctionType(LLVMVoidType(),
            [[LLVMTypeRef LLVMFloat4Type(); LLVMPointerType(LLVMVoidType(),0u); get_type_string(); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_debug, get_jit_debug())
    LLVMAddAttributesToFunction(jit_debug, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_debug, urange(1,4), nocapture)
    // bool builtin_iterator_iterate ( const Sequence & it, void * data, Context * context ) {
    var jit_iterator_iterate = LLVMAddFunction(g_mod, FN_JIT_ITERATOR_ITERATE,
        LLVMFunctionType(LLVMInt1Type(),
            [[LLVMTypeRef LLVMPointerType(g_t_sequence,0u); LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_iterator_iterate, get_jit_iterator_iterate())
    LLVMAddAttributesToFunction(jit_iterator_iterate, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_iterate, urange(0,3), nocapture)
    // void builtin_iterator_delete ( const Sequence & it, Context * context ) {
    var jit_iterator_delete = LLVMAddFunction(g_mod, FN_JIT_ITERATOR_DELETE,
        LLVMFunctionType(LLVMInt1Type(),
            [[LLVMTypeRef LLVMPointerType(g_t_sequence,0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_iterator_delete, get_jit_iterator_delete())
    LLVMAddAttributesToFunction(jit_iterator_delete, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_delete, urange(0,2), nocapture)
    // void builtin_iterator_close ( const Sequence & it, void * data, Context * context ) {
    var jit_iterator_close = LLVMAddFunction(g_mod, FN_JIT_ITERATOR_CLOSE,
        LLVMFunctionType(LLVMVoidType(),
            [[LLVMTypeRef LLVMPointerType(g_t_sequence,0u); LLVMPointerType(LLVMVoidType(),0u); LLVMPointerType(LLVMVoidType(),0u)]]))
    LLVMAddGlobalMapping(g_engine, jit_iterator_close, get_jit_iterator_close())
    LLVMAddAttributesToFunction(jit_iterator_close, [[auto nounwind; willreturn]])
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_close, urange(0,3), nocapture)

def private build_jit_types
    // simfunction type
    var simfunction_fields <- [[auto
        LLVMPointerType(LLVMInt8Type(),0u); // char * name
        LLVMPointerType(LLVMInt8Type(),0u); // char * mangledName
        LLVMPointerType(LLVMVoidType(),0u); // SimNode * code               // note void *
        LLVMPointerType(LLVMVoidType(),0u); // FuncInfo *  debugInfo        // note void *
        LLVMInt64Type();                    // uint64_t    mangledNameHash
        LLVMPointerType(LLVMVoidType(),0u); // void *      aotFunction
        LLVMInt32Type();                    // uint32_t    stackSize
        LLVMInt32Type()                     // uint32_t    flags
    ]]
    g_t_simFunction = LLVMStructType(simfunction_fields)
    // function type
    var function_fields <- [[auto[]
        LLVMPointerType(g_t_simFunction,0u) // SimFunction * func
    ]]
    g_t_function = LLVMStructType(function_fields)
    // lambda type
    var lambda_fields <- [[auto[]
        LLVMPointerType(g_t_simFunction,0u);    // SimFunction * __lambda
        LLVMPointerType(g_t_simFunction,0u)     // SimFunction * __finalize
    ]]
    g_t_lambda = LLVMStructType(lambda_fields)
    // block type
    var block_fields <- build_block_type()
    g_t_block = LLVMStructType(block_fields)
    // prologue\epilogue storage for the stack
    var stack_state_fields <- [[auto[]
        LLVMPointerType(LLVMInt8Type(),0u);    // char * EP
        LLVMPointerType(LLVMInt8Type(),0u)     // char * SP
    ]]
    g_t_stack_state = LLVMStructType(stack_state_fields)
    // iterator
    var iterator_fields <- [[auto[]
        LLVMPointerType(LLVMVoidType(),0u);    // __vtable
        LLVMInt1Type()                         // bool isOpen
    ]]
    g_t_iterator = LLVMStructType(iterator_fields)
    // sequence
    var sequence_fields <- [[auto[]
        LLVMPointerType(g_t_iterator,0u)        // iterator * iter
    ]]
    g_t_sequence = LLVMStructType(sequence_fields)

def public build_block_type
    return <- [{auto[]
        LLVMInt32Type();                        // uint32_t    stackOffset;
        LLVMInt32Type();                        // uint32_t    argumentsOffset;
        LLVMPointerType(LLVMVoidType(),0u);     // SimNode *   body;
        LLVMPointerType(LLVMVoidType(),0u);     // void *      aotFunction;
        LLVMPointerType(LLVMVoidType(),0u);     // void *      jitFunction;
        LLVMPointerType(LLVMFloat4Type(),0u);   // vec4f *     functionArguments;
        LLVMPointerType(LLVMVoidType(),0u)      // FuncInfo *  info;
    }]

[macro_function]
def public finalize_jit
    return if is_in_completion() || is_compiling_macros()
    return if g_mod == null    // prevent double finalize
    if LLVM_LOG_RESULT
        LLVMDumpModule(g_mod)
        fflush(fstderr())
    g_all_engines |> push([[ JitEngine
        mod <- g_mod,
        engine <- g_engine
    ]])
    unsafe
        delete g_str2v

[finalize]
def private finalize_all_jit
    for je in g_all_engines
        LLVMDisposeExecutionEngine(je.engine); je.engine = null
        g_t_function = null
        g_t_simFunction = null
        g_t_block = null
        g_t_lambda = null
        g_t_stack_state = null
        g_t_iterator = null
        g_t_sequence = null
        // TODO: figure out why this crashes
        // LLVMDisposeModule(je.mod); je.mod = null
    unsafe
        delete g_all_engines

def public build_string_constant ( message:string )
    if g_str2v |> key_exists(message)
        return g_str2v[message]
    let msg_size = uint(length(message))
    var strType = LLVMArrayType( LLVMInt8Type(), msg_size+1u )
    var str = LLVMAddGlobal(g_mod, strType, "")
    LLVMSetInitializer(str, LLVMConstString(message, msg_size, 0))
    LLVMSetGlobalConstant(str, 1)
    LLVMSetLinkage(str, LLVMLinkage LLVMPrivateLinkage)
    LLVMSetUnnamedAddress(str, LLVMUnnamedAddr LLVMGlobalUnnamedAddr)
    LLVMSetAlignment(str, 1u)
    g_str2v[message] = str
    return str

def public get_basetype_pointer ( t:Type ) : LLVMOpaqueType?
    return LLVMPointerType(base_type_to_llvm_type(t),0u)

def public get_type_pointer ( t:TypeDeclPtr ) : LLVMOpaqueType?
    if t==null
        return LLVMPointerType(LLVMVoidType(),0u)
    else
        return LLVMPointerType(type_to_llvm_type(t),0u)

def public get_type_string : LLVMOpaqueType?
    return LLVMPointerType(LLVMInt8Type(), 0u)

def public get_string_constant_ptr ( builder:LLVMOpaqueBuilder?; message:string )
    var str = build_string_constant(message)
    return LLVMBuildPointerCast(builder, str, get_type_string(), "string_constant {message}")

def public failed ( msg:string )
    static_if LLVM_DEBUG_EVERYTHING
        print("FAILED: {msg}\n")
    g_errors |> push("{msg}")

def public failed_E ( e:ExpressionPtr; msg:string )
    failed ( "{msg}. expr = {e.__rtti} at {describe(e.at)}")

def public failed_E ( e:Expression?; msg:string )
    failed ( "{msg}. expr = {e.__rtti} at {describe(e.at)}")

def public failed_V ( v:VariablePtr; msg:string )
    failed ( "{msg}. var = {describe(v)} at {describe(v.at)}")

def public failed_V ( v:Variable?; msg:string )
    failed ( "{msg}. var = {v.name} at {describe(v.at)}")

def public failed_T ( v:TypeDeclPtr; msg:string )
    failed ( "{msg}. var = {describe(v)} at {describe(v.at)}")

def public base_type_to_llvm_type ( t:Type )
    return LLVMInt32Type()      if t == Type tInt || t == Type tUInt || t == Type tBitfield || t == Type tEnumeration
    return LLVMInt64Type()      if t == Type tInt64 || t == Type tUInt64
    return LLVMInt16Type()      if t == Type tInt16 || t == Type tUInt16 || t == Type tEnumeration16
    return LLVMInt8Type()       if t == Type tInt8 || t == Type tUInt8 || t == Type tEnumeration8
    return LLVMFloatType()      if t == Type tFloat
    return LLVMDoubleType()     if t == Type tDouble
    return LLVMInt1Type()       if t == Type tBool
    return LLVMInt2Type()       if t == Type tInt2 || t == Type tUInt2 || t == Type tRange || t == Type tURange
    return LLVMInt3Type()       if t == Type tInt3 || t == Type tUInt3
    return LLVMInt4Type()       if t == Type tInt4 || t == Type tUInt4
    return LLVMRange64Type()    if t == Type tRange64 || t == Type tURange64
    return LLVMFloat2Type()     if t == Type tFloat2
    return LLVMFloat3Type()     if t == Type tFloat3
    return LLVMFloat4Type()     if t == Type tFloat4
    return get_type_string()    if t == Type tString
    return LLVMPointerType(LLVMVoidType(),0u) if t == Type tPointer
    failed("base_type_to_llvm_type: unsupported type {t}")
    return LLVMInt32Type()

def public type_to_llvm_abi_type ( t:TypeDeclPtr )
    if t.baseType==Type tBlock
        return LLVMPointerType(g_t_block, 0u)
    elif t.isRef
        return LLVMPointerType(type_to_llvm_type(t), 0u)
    elif t.isPointer
        return get_type_pointer(t.firstType)
    elif t.isWorkhorseType
        return base_type_to_llvm_type(t.baseType)
    elif t.baseType==Type tFunction || t.baseType==Type tLambda
        return type_to_llvm_type(t)
    elif t.isVoid
        return LLVMVoidType()
    elif t.baseType==Type fakeContext || t.baseType==Type fakeLineInfo
        return LLVMPointerType(LLVMVoidType(),0u)
    elif t.baseType==Type tHandle
        if t.isRefType
            failed("type_to_llvm_abi_type: internal error. unsupported type {describe(t)} at {describe(t.at)}")
            return LLVMPointerType(LLVMVoidType(),0u)
        else
            var inscope tp <- get_underlying_value_type(t)
            if tp==null
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(t)} at {describe(t.at)}")
                return LLVMInt32Type()
            elif !tp.isWorkhorseType
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(t)} at {describe(t.at)}")
                return LLVMInt32Type()
            return type_to_llvm_abi_type(tp)
    else
        failed("type_to_llvm_abi_type: unsupported type {describe(t)} at {describe(t.at)}")
        return LLVMInt32Type()

def public type_to_llvm_type ( t:TypeDeclPtr )
    var res : LLVMOpaqueType?
    if t.dim |> length != 0
        var inscope ndt <- clone_type(t)
        ndt.dim |> clear()
        res = type_to_llvm_type(ndt)
        for d in t.dim
            res = LLVMArrayType(res, uint(d))
    elif t.isPointer
        res = get_type_pointer(t.firstType)
    elif t.isWorkhorseType
        res = base_type_to_llvm_type(t.baseType)
    elif t.isStructure
        res = LLVMArrayType(LLVMInt8Type(), uint(t.sizeOf))
    elif t.isTuple
        res = LLVMArrayType(LLVMInt8Type(), uint(t.sizeOf))
    elif t.isVariant
        res = LLVMArrayType(LLVMInt8Type(), uint(t.sizeOf))
    elif t.baseType == Type tFunction
        res = g_t_function
    elif t.baseType == Type tLambda
        res = LLVMPointerType(g_t_lambda,0u)
    elif t.baseType == Type tIterator
        res = g_t_sequence
    elif t.baseType == Type tBlock
        res = LLVMPointerType(g_t_block,0u)
    elif t.baseType == Type tArray
        // array type
        var array_fields <- [[auto
            LLVMPointerType(LLVMVoidType(),0u); // char * data
            LLVMInt32Type();                    // uint32_t size
            LLVMInt32Type();                    // uint32_t capacity
            LLVMInt32Type();                    // uint32_t lock
            LLVMInt32Type()                     // uint32_t flags
        ]]
        if t.firstType!=null
            array_fields[int(JIT_ARRAY DATA)] = LLVMPointerType(type_to_llvm_type(t.firstType),0u)
        res = LLVMStructType(array_fields)
    elif t.baseType == Type tTable
        // table type
        var table_fields <- [[auto
            LLVMPointerType(LLVMVoidType(),0u); // char * data
            LLVMInt32Type();                    // uint32_t size
            LLVMInt32Type();                    // uint32_t capacity
            LLVMInt32Type();                    // uint32_t lock
            LLVMInt32Type();                    // uint32_t flags
            LLVMPointerType(LLVMVoidType(),0u); // char * keys
            LLVMPointerType(LLVMVoidType(),0u); // char * hashes
            LLVMInt32Type();                    // uint32_t maxLookup
            LLVMInt32Type()                     // uint32_t shift
        ]]
        if t.firstType!=null
            table_fields[int(JIT_TABLE KEYS)] = LLVMPointerType(type_to_llvm_type(t.firstType),0u)
        if t.secondType!=null
            table_fields[int(JIT_ARRAY DATA)] = LLVMPointerType(type_to_llvm_type(t.secondType),0u)
        res = LLVMStructType(table_fields)
    elif t.isVoid
        res = LLVMVoidType()
    elif t.isHandle
        if t.isRefType
            res = LLVMArrayType(LLVMInt8Type(), uint(t.sizeOf))
        else
            var inscope tp <- get_underlying_value_type(t)
            if tp==null
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(t)} at {describe(t.at)}")
                return LLVMInt32Type()
            elif !tp.isWorkhorseType
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(t)} at {describe(t.at)}")
                return LLVMInt32Type()
            res = type_to_llvm_type(tp)
    else
        failed("type_to_llvm_type: unsupported type {describe(t)} at {describe(t.at)}")
        res = LLVMInt32Type()
    return res

def public dim_element_type_to_llvm_type ( typ:TypeDeclPtr )
    if typ.dim |> length == 0
        return type_to_llvm_type(typ)
    else
        var inscope ndt <- clone_type(typ)
        ndt.dim |> clear()
        return type_to_llvm_type(ndt)

def public build_broadcast_vector ( builder:LLVMOpaqueBuilder?; opType:TypeDeclPtr; right:LLVMOpaqueValue? )
    if opType.isVectorType
        var vres = LLVMGetUndef(type_to_llvm_type(opType))
        for i in range(opType.vectorDim)
            vres = LLVMBuildInsertElement(builder, vres, right, LLVMConstInt(LLVMInt32Type(), uint64(i), 0), "")
        return vres
    else
        return right

def public LLVMBuildLoadDataAligned ( builder:LLVMOpaqueBuilder?; ptr:LLVMOpaqueValue?; alignment:uint; name:string )
    var typ = LLVMGetElementType(LLVMTypeOf(ptr))
    if LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS && LLVMIsVector3(typ)
        return LLVMBuildLoadVector3(builder, ptr, name)
    else
        return LLVMBuildLoadAligned(builder, ptr, alignment, name)

def public LLVMBuildLoadData2Aligned ( builder:LLVMOpaqueBuilder?; typ:LLVMOpaqueType?; ptr:LLVMOpaqueValue?; alignment:uint; name:string )
    if LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS && alignment!=16u && LLVMIsVector3(typ)
        return LLVMBuildLoadVector3(builder, ptr, name)
    else
        return LLVMBuildLoad2Aligned(builder, typ, ptr, alignment, name)

def public expand_scalar ( builder:LLVMOpaqueBuilder?; scalar:LLVMOpaqueValue?; vecType:TypeDeclPtr )
    var vec = LLVMGetUndef(type_to_llvm_type(vecType))
    for d in range(vecType.vectorDim)
        var idx = LLVMConstInt(LLVMInt32Type(), uint64(d), 0)
        vec = LLVMBuildInsertElement(builder, vec, scalar, idx, "")
    return vec

def public build_lifetime ( g_builder:LLVMOpaqueBuilder?; size:uint64; ptr:LLVMOpaqueValue?; begin:bool ) : LLVMOpaqueValue?
    let lifetime_name = begin ? "llvm.lifetime.start" : "llvm.lifetime.end"
    var args <- [{auto[]
        LLVMConstInt(LLVMInt64Type(), size, 0);
        ptr
    }]
    var argTypes <- [{auto LLVMTypeOf(ptr) }]
    var id = LLVMLookupIntrinsicID(lifetime_name)
    var decl = LLVMGetIntrinsicDeclaration(g_mod, id, argTypes)
    if decl==null
        failed("missing intrinsic {lifetime_name}")
        return null
    return LLVMBuildCall(g_builder, decl, args, "")