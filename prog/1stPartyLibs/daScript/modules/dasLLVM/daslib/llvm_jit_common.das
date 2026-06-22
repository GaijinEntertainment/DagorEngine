options gen2

options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options relaxed_pointer_const
options unsafe_table_lookup = false
options no_global_variables = false
options stack = 4_194_304 // On huge files we need huge stack for jit

module llvm_jit_common shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dll_utils
require llvm/daslib/llvm_targets

require daslib/ast_boost
require daslib/strings
require daslib/strings_boost
require daslib/defer
require daslib/enum_trait
require daslib/fio
require daslib/option
require jit

let public LLVM_DEBUG_RESULT = false
let public LLVM_DEBUG_EVERYTHING = false

let public LLVM_DEBUG_TRACES = false
let public LLVM_DEBUG_LINE_TRACES = false

let public LLVM_JIT_LOG = false || LLVM_DEBUG_EVERYTHING

let public LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT = false   // NOTE - this should be true, to match interpreter and AOT behavior
var public LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT

let public LLVM_ENABLE_OPT_PASS = LLVM_DEBUG_EVERYTHING ? false : true
let public LLVM_LOG_RESULT = LLVM_DEBUG_EVERYTHING || LLVM_DEBUG_RESULT
let public LLVM_INLINE_THRESHOLD = 1024u // 225u

// This functions should be extern "C" and their name should match with c-name.
let public FN_JIT_EXCEPTION         = "jit_exception"
let public FN_JIT_CALL_OR_FASTCALL  = "jit_call_or_fastcall"
let public FN_JIT_CALL_WITH_CMRES   = "jit_call_with_cmres"
let public FN_JIT_INVOKE_BLOCK      = "jit_invoke_block"
let public FN_JIT_INVOKE_BLOCK_WITH_CMRES = "jit_invoke_block_with_cmres"
let public FN_JIT_STRING_BUILDER    = "jit_string_builder"
let public FN_JIT_STRING_BUILDER_TEMP    = "jit_string_builder_temp"
let public FN_JIT_GET_GLOBAL_MNH    = "jit_get_global_mnh"
let public FN_JIT_GET_SHARED_MNH    = "jit_get_shared_mnh"
// Wasm-only path — see g_target_is_wasm in init_jit and the two sites in
// llvm_jit.das that emit a call instead of a baked-host-offset GEP+load.
let public FN_JIT_GET_GLOBALS_BASE  = "jit_get_globals_base"
let public FN_JIT_GET_SHARED_BASE   = "jit_get_shared_base"
let public FN_JIT_ALLOC_HEAP        = "jit_alloc_heap"
let public FN_JIT_ALLOC_PERSISTENT  = "jit_alloc_persistent"
let public FN_JIT_FREE_HEAP         = "jit_free_heap"
let public FN_JIT_FREE_PERSISTENT   = "jit_free_persistent"
let public FN_JIT_ARRAY_LOCK        = "jit_array_lock"
let public FN_JIT_ARRAY_UNLOCK      = "jit_array_unlock"
let public FN_JIT_STR_CMP           = "jit_str_cmp"
let public FN_JIT_STR_CAT           = "jit_str_cat"
let public FN_JIT_PROLOGUE          = "jit_prologue"
let public FN_JIT_EPILOGUE          = "jit_epilogue"
let public FN_JIT_MAKE_BLOCK        = "jit_make_block"
let public FN_JIT_DEBUG             = "jit_debug"
let public FN_JIT_ITERATOR_ITERATE  = "jit_iterator_iterate"
let public FN_JIT_ITERATOR_DELETE   = "jit_iterator_delete"
let public FN_JIT_ITERATOR_CLOSE    = "jit_iterator_close"
let public FN_JIT_ITERATOR_FIRST    = "jit_iterator_first"
let public FN_JIT_ITERATOR_NEXT     = "jit_iterator_next"
let public FN_JIT_DEBUG_ENTER       = "jit_debug_enter"
let public FN_JIT_DEBUG_EXIT        = "jit_debug_exit"
let public FN_JIT_DEBUG_LINE        = "jit_debug_line"
let public FN_JIT_AST_TYPEDECL      = "jit_ast_typedecl"
let public FN_JIT_INIT_EXTERN       = "jit_init_extern_function"

var public g_ctx : LLVMContextRef
var public g_prim_t : PrimitiveTypes?
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

var public g_str2v : table<string; LLVMOpaqueValue?>

class public Attributes {
    noreturn : LLVMOpaqueAttributeRef?
    readonly : LLVMOpaqueAttributeRef?
    optnone : LLVMOpaqueAttributeRef?
    nounwind : LLVMOpaqueAttributeRef?
    willreturn : LLVMOpaqueAttributeRef?
    nocapture : LLVMOpaqueAttributeRef?
    alwaysinline : LLVMOpaqueAttributeRef?
    noinline : LLVMOpaqueAttributeRef?
    hot : LLVMOpaqueAttributeRef?
    cold : LLVMOpaqueAttributeRef?
    mustprogress : LLVMOpaqueAttributeRef?
    minsize : LLVMOpaqueAttributeRef?
    nofree : LLVMOpaqueAttributeRef?
    norecurse : LLVMOpaqueAttributeRef?
    noalias : LLVMOpaqueAttributeRef?
    sext : LLVMOpaqueAttributeRef?
    zext : LLVMOpaqueAttributeRef?
    // memory(inaccessiblemem: readwrite) — for allocator-style runtime calls
    // LLVM 22 IRMemLocation layout: ArgMem=0(bits 0-1), InaccessibleMem=1(bits 2-3), each ModRef=3 → 3<<2=12
    memory_inaccessible_rw : LLVMOpaqueAttributeRef?
    // memory(argmem: readwrite, inaccessiblemem: readwrite) — for deallocator-style runtime calls
    // 3 | (3 << 2) = 15
    memory_arg_inaccessible_rw : LLVMOpaqueAttributeRef?

    def Attributes(ctx : LLVMContextRef) {
        noreturn = ctx |> LLVMGetEnumAttribute("noreturn")
        readonly = ctx |> LLVMGetEnumAttribute("readonly")
        optnone = ctx |> LLVMGetEnumAttribute("optnone")
        nounwind = ctx |> LLVMGetEnumAttribute("nounwind")
        willreturn = ctx |> LLVMGetEnumAttribute("willreturn")
        nocapture = ctx |> LLVMGetEnumAttribute("nocapture")
        alwaysinline = ctx |> LLVMGetEnumAttribute("alwaysinline")
        noinline = ctx |> LLVMGetEnumAttribute("noinline")
        hot = ctx |> LLVMGetEnumAttribute("hot")
        cold = ctx |> LLVMGetEnumAttribute("cold")
        mustprogress = ctx |> LLVMGetEnumAttribute("mustprogress")
        minsize = ctx |> LLVMGetEnumAttribute("minsize")
        nofree = ctx |> LLVMGetEnumAttribute("nofree")
        norecurse = ctx |> LLVMGetEnumAttribute("norecurse")
        noalias = ctx |> LLVMGetEnumAttribute("noalias")
        sext = ctx |> LLVMGetEnumAttribute("signext")
        zext = ctx |> LLVMGetEnumAttribute("zeroext")
        let memory_kind = LLVMGetEnumAttributeKindForName("memory")
        memory_inaccessible_rw = LLVMCreateEnumAttribute(ctx, memory_kind, 12ul)
        memory_arg_inaccessible_rw = LLVMCreateEnumAttribute(ctx, memory_kind, 15ul)
    }

}

// Indices MUST match the field order produced by `type_to_llvm_type` for `Type.tArray`
// below, which itself mirrors the C++ `das::Array` layout (data, size, capacity, magic,
// lock, flags, _pad_after_flags). The enum order was originally `LOOK, MAGIC` to match
// an older mirror that had `lock, magic` in that order — when the mirror was corrected
// to C++ order, the enum needed the same flip.
enum public JIT_ARRAY {
    DATA
    SIZE
    CAPACITY
    MAGIC
    LOCK
    FLAGS
    PAD_AFTER_FLAGS  // mirror Array's trailing alignment pad (see arraytype.h)
}

// Indices MUST match the field order produced by `type_to_llvm_type` for `Type.tTable`
// below — Array base fields, then keys/hashes/tombstones. Keep in sync with the C++
// `das::Table` layout (Table : Array).
enum public JIT_TABLE {
    DATA
    SIZE
    CAPACITY
    MAGIC
    LOCK
    FLAGS
    PAD_AFTER_FLAGS  // matches C++ sizeof(Array)==40 on 32-bit; Table::keys lands at 40
    KEYS
    HASHES
    TOMBSTONES
}

enum public JIT_SIMFUNCTION {
    NAME
    MANGLED_NAME
    CODE
    DEBUG_INFO
    MANGLED_NAME_HASH
    AOT_FUNCTION
    STACK_SIZE
    FLAGS
}

// Must match TABLE_MAX_LINEAR_CAPACITY in runtime_table.h. A packed table has exactly this
// many slots, so the inlined packed find scans a fixed count (and can vectorize it).
let public JIT_TABLE_MAX_LINEAR_CAPACITY = 8

enum public JIT_FUNCTION {
    SIM_FUNCTION
}

enum public JIT_LAMBDA {
    EVAL
    FINALIZER
}

enum public JIT_BLOCK {
    STACK_OFFSET
    ARGUMENTS_OFFSET
    BODY
    AOT_FUNCTION
    JIT_FUNCTION
    FUNCTION_ARGUMENTS
    INFO
}

enum public JIT_SEQUENCE {
    ITERATOR
}

// We use global variable to store pointer to jit table methods.
def public FN_JIT_TABLE_AT(t : Type) {
    return "`jit`table_at`{t}`"
}

def public FN_JIT_TABLE_ERASE(t : Type) {
    return "`jit`table_erase`{t}`"
}

def public FN_JIT_TABLE_FIND(t : Type) {
    return "`jit`table_find`{t}`"
}

// String-key at that takes a precomputed hash (string-only, no per-type matrix; the grow fallback
// for the inline large-string at). String find is fully inline — no C++ helper.
def public FN_JIT_STRING_TABLE_AT_WITH_HASH() {
    return "`jit`string_table_at_with_hash`"
}

def public FN_JIT_STRING_TABLE_AT_AFTER_PACKED_MISS() {
    return "`jit`string_table_at_after_packed_miss`"
}


def private add_table_linkage(val : LLVMOpaqueValue?) {
    // DLLExport storage so ResolveExternVisitor's set_glob_address (GetProcAddress
    // on Windows) can find the slot on cache-hit reload; otherwise #2802 stale-addr.
    LLVMSetLinkage(val, LLVMLinkage.LLVMDLLExportLinkage)
    LLVMSetDLLStorageClass(val, LLVMDLLStorageClass.LLVMDLLExportStorageClass)
}

def private get_or_create_table_fn(llvm_module : LLVMOpaqueModule?; var types : PrimitiveTypes?; fn_name : DllName; at_type : LLVMOpaqueType?; fn_ptr : void?) {
    var tab_fun : LLVMOpaqueValue? = null
    if (!g_fn_types |> key_exists(fn_name.publ())) {
        // create fn
        g_fn_types[fn_name.publ()] = at_type

        let at_ptr_type = LLVMPointerType(at_type, 0u)
        tab_fun = LLVMAddGlobal(llvm_module, LLVMPointerType(at_ptr_type, 0u), fn_name.glob())
        add_table_linkage(tab_fun)
        LLVMSetInitializer(tab_fun, types.ConstPtr(fn_ptr |> intptr(), at_ptr_type))
    } else {
        // we already created it
        tab_fun = LLVMGetNamedGlobal(llvm_module, fn_name.glob())
    }
    return tab_fun
}

def public get_table_at(g_builder : LLVMOpaqueBuilder?; llvm_module : LLVMOpaqueModule?; t : Type; var types : PrimitiveTypes?) {
    assume fn_ptr = get_jit_table_at(unsafe(reinterpret<int> t))
    // int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context )
    assume at_type = LLVMFunctionType(types.t_int32,   // int32_t result
        fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(), // table ptr
        base_type_to_llvm_type(t),                           // key
        types.t_int32,                                    // valueTypeSize
        types.LLVMVoidPtrType(),                          // context
        types.LLVMVoidPtrType(),                          // at
    ))
    let fn_name = DllName(FN_JIT_TABLE_AT(t))
    var tab_fun : LLVMOpaqueValue? = get_or_create_table_fn(llvm_module, types, fn_name, at_type, fn_ptr)
    assert(tab_fun != null)
    tab_fun = LLVMBuildLoad2(g_builder, LLVMPointerType(at_type, 0u), tab_fun, "func_at")
    return (tab_fun, at_type)
}

def public get_table_find(g_builder : LLVMOpaqueBuilder?; llvm_module : LLVMOpaqueModule?; t : Type; var types : PrimitiveTypes?) {
    assume fn_ptr = get_jit_table_find(unsafe(reinterpret<int> t))
    // int32_t jit_table_find ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context )
    let find_type = LLVMFunctionType(types.t_int32,   // int32_t result
            fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(),   // table ptr
            base_type_to_llvm_type(t),                          // key
            types.t_int32,                                    // valueTypeSize
            types.LLVMVoidPtrType()                 // context
            ))
    let fn_name = DllName(FN_JIT_TABLE_FIND(t))
    var tab_fun : LLVMOpaqueValue? = get_or_create_table_fn(llvm_module, types, fn_name, find_type, fn_ptr)
    assert(tab_fun != null)
    tab_fun = LLVMBuildLoad2(g_builder, LLVMPointerType(find_type, 0u), tab_fun, "func_find")
    return (tab_fun, find_type)
}

def public get_string_table_at_with_hash(g_builder : LLVMOpaqueBuilder?; llvm_module : LLVMOpaqueModule?; var types : PrimitiveTypes?) {
    assume fn_ptr = get_jit_string_table_at_with_hash()
    // int32_t jit_string_table_at_with_hash ( Table * tab, char * key, uint64_t hfn, int32_t valueTypeSize, Context * context, LineInfoArg * at )
    assume at_type = LLVMFunctionType(types.t_int32,   // int32_t result
        fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(), // table ptr
        base_type_to_llvm_type(Type.tString),                // key
        types.t_int64,                                    // precomputed hash
        types.t_int32,                                    // valueTypeSize
        types.LLVMVoidPtrType(),                          // context
        types.LLVMVoidPtrType(),                          // at
    ))
    let fn_name = DllName(FN_JIT_STRING_TABLE_AT_WITH_HASH())
    var tab_fun : LLVMOpaqueValue? = get_or_create_table_fn(llvm_module, types, fn_name, at_type, fn_ptr)
    assert(tab_fun != null)
    tab_fun = LLVMBuildLoad2(g_builder, LLVMPointerType(at_type, 0u), tab_fun, "func_at_wh")
    return (tab_fun, at_type)
}

def public get_string_table_at_after_packed_miss(g_builder : LLVMOpaqueBuilder?; llvm_module : LLVMOpaqueModule?; var types : PrimitiveTypes?) {
    assume fn_ptr = get_jit_string_table_at_after_packed_miss()
    // int32_t jit_string_table_at_after_packed_miss ( Table*, char* key, uint64_t hfn, int32_t valueTypeSize, Context*, LineInfoArg* )
    assume at_type = LLVMFunctionType(types.t_int32,
        fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(), // table ptr
        base_type_to_llvm_type(Type.tString),                // key
        types.t_int64,                                    // precomputed hash
        types.t_int32,                                    // valueTypeSize
        types.LLVMVoidPtrType(),                          // context
        types.LLVMVoidPtrType(),                          // at
    ))
    let fn_name = DllName(FN_JIT_STRING_TABLE_AT_AFTER_PACKED_MISS())
    var tab_fun : LLVMOpaqueValue? = get_or_create_table_fn(llvm_module, types, fn_name, at_type, fn_ptr)
    assert(tab_fun != null)
    tab_fun = LLVMBuildLoad2(g_builder, LLVMPointerType(at_type, 0u), tab_fun, "func_at_apm")
    return (tab_fun, at_type)
}

// Generic Table header (Array base + keys/hashes/tombstones) with pointer fields as void*,
// so it is independent of key/value type. Field order matches JIT_TABLE; used only to read
// capacity / hashes for the inlined packed find.
def public table_header_llvm_type(var types : PrimitiveTypes?) : LLVMOpaqueType? {
    return types |> StructType(fixed_array(
        types.LLVMVoidPtrType(),   // DATA
        types.t_int64,             // SIZE
        types.t_int64,             // CAPACITY
        types.t_int32,             // MAGIC
        types.t_int32,             // LOCK
        types.t_int32,             // FLAGS
        types.t_int32,             // PAD_AFTER_FLAGS
        types.LLVMVoidPtrType(),   // KEYS
        types.LLVMVoidPtrType(),   // HASHES (TableHashKey* = uint32*)
        types.t_int64))            // TOMBSTONES
}

def public get_table_erase(g_builder : LLVMOpaqueBuilder?; llvm_module : LLVMOpaqueModule?; t : Type; var types : PrimitiveTypes?) {
    let fn_name = DllName(FN_JIT_TABLE_ERASE(t))
    assume fn_ptr = get_jit_table_erase(unsafe(reinterpret<int> t))
    // bool jit_table_erase ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context, LineInfoArg * at )
    let erase_type = LLVMFunctionType(types.t_int1,                        // bool result
            fixed_array<LLVMTypeRef>(types.LLVMVoidPtrType(),   // table ptr
            base_type_to_llvm_type(t),                          // key
            types.t_int32,                                    // valueTypeSize
            types.LLVMVoidPtrType(),                          // context
            types.LLVMVoidPtrType()                           // at
            ))
    var tab_fun : LLVMOpaqueValue? = get_or_create_table_fn(llvm_module, types, fn_name, erase_type, fn_ptr)
    assert(tab_fun != null)
    tab_fun = LLVMBuildLoad2(g_builder, LLVMPointerType(erase_type, 0u), tab_fun, "func_erase")
    return (tab_fun, erase_type)
}

var public g_fn_types : table<string; LLVMTypeRef>

// Set by init_jit when target_triple is a wasm target. Cross-compile path's
// host-LLVM wasm32 codegen mishandles several user-provided hints (alwaysinline
// over complex inlined bodies, certain loop-vectorize / unroll metadata). When
// this flag is true, the LlvmJitVisitor skips ALL user hint application —
// function-level [hint(...)] AND for/while-loop hints — and falls back to the
// optimizer's default heuristics. Slight perf hit, but eliminates a class of
// wasm32-only miscompiles.
var public g_target_is_wasm = false

def public LLVMAddFunctionWithType(mod : LLVMModuleRef; name : string; typ : LLVMTypeRef) {
    var rv = LLVMAddFunction(mod, name, typ)
    g_fn_types[name] = typ
    return rv
}


var g_jit_clopts_parsed = false

// LLVM's cost-model thresholds for unroll / partial-unroll / unroll-and-jam
// are compiled-in cl::opt globals. opt(1) sets them indirectly via
// cl::ParseCommandLineOptions before parsing the -passes string; daslang's
// JIT path does not, so the same `default<O3>` pipeline lands different
// IR shapes (rolled outer loops, no unroll-and-jam) than `opt` produces
// on the same input.
//
// Bisected against the dasProfile benchmark suite (JIT mode, 16 tests):
//   * spectral-norm 2.77x, native 1.44x, queen 1.39x, sha256 1.17x
//   * dict/mandelbrot/exp marginal +1-3%
//   * 8 others within +-2%
//   * primes -9% (tight `for (i in 2..n) if (n%i==0) return false` loop —
//     partial unroll bloats the early-exit path without enabling vectorization)
//
// The three permission flags (-unroll-allow-partial + the unroll-and-jam
// pair) are load-bearing for spectral-norm/native. The threshold quartet
// is what pushes queen from 1.05x to 1.39x.
[macro_function]
def public init_jit_clopts() {
    return if (g_jit_clopts_parsed)
    g_jit_clopts_parsed = true
    var argv <- [
        "daslang-jit",
        "-unroll-threshold=400",
        "-unroll-partial-threshold=600",
        "-unroll-max-count=16",
        "-unroll-allow-partial=true",
        "-enable-unroll-and-jam=true",
        "-allow-unroll-and-jam=true"
    ]
    unsafe {
        LLVMParseCommandLineOptions(length(argv),
            reinterpret<string const?>(addr(argv[0])),
            "daslang JIT cl::opts")
    }
}

[macro_function]
def public init_jit(cg_opt_level : uint; target_triple : string = "") {
    // also prevent double init (g_mod != null)
    return if (is_in_completion() || is_compiling_macros() || g_mod != null)
    LLVMLinkInMCJIT()
    LLVMInitializeAllTargetInfos()
    LLVMInitializeAllTargets()
    LLVMInitializeAllTargetMCs()
    LLVMInitializeAllAsmPrinters()
    init_jit_clopts()
    g_ctx = LLVMContextCreate()
    g_prim_t = new PrimitiveTypes(g_ctx)
    g_failed = false
    g_fn_types |> clear()
    g_mod = LLVMModuleCreateWithNameInContext("llvm_jit_module", g_ctx)
    LLVMCreateJITCompilerForModule(g_engine, g_mod, cg_opt_level)
    // For cross-compile targets (wasm32, etc.) pin the module's data layout
    // to the TARGET's, not the host's, BEFORE any IR is built. LLVM constant-
    // folds GEPs eagerly using the module's current layout — if the host has
    // 8-byte pointers and the target has 4-byte, every for-loop end-pointer
    // GEP bakes in the wrong stride and the loop never terminates. The native
    // path (empty triple) keeps the host default.
    g_target_is_wasm = target_triple |> starts_with("wasm")
    if (!(target_triple |> empty())) {
        // wasm32 target isn't always part of LLVMInitializeAllTargets in
        // prebuilt LLVM.dll — call the explicit wasm initializer when needed.
        if (g_target_is_wasm) {
            LLVMInitializeWasmTarget()
        }
        // +simd128 / +nontrapping-fptoint is the feature set the wasm runtime
        // archive is compiled with (see web/CMakeLists.txt). Required on wasm
        // so vec4f returns are v128-in-register on both sides; meaningless
        // (and potentially a target-machine-creation failure) on other
        // cross-compile triples, so keep features empty there.
        let features = g_target_is_wasm ? "+simd128,+nontrapping-fptoint" : ""
        with_target_machine(target_triple, "", features, cg_opt_level) $(targetMachine : LLVMTargetMachineRef) {
            let dl = LLVMCreateTargetDataLayout(targetMachine)
            LLVMSetModuleDataLayout(g_mod, dl)
            LLVMSetTarget(g_mod, target_triple)
            LLVMDisposeTargetData(dl)
        }
    }
    static_if (LLVM_DEBUG_EVERYTHING) {
        let hostcpu = LLVMGetHostCPUName()
        let features = LLVMGetHostCPUFeatures()
        to_log(LOG_DEBUG, "LLVM JIT: host cpu: {hostcpu}\n")
        to_log(LOG_DEBUG, "LLVM JIT: host cpu features: {features}\n")
        LLVMDisposeMessage(hostcpu)
        LLVMDisposeMessage(features)
    }
    let attrs = new Attributes(g_ctx)
    assume noreturn = attrs.noreturn
    assume readonly = attrs.readonly
    assume nounwind = attrs.nounwind
    assume willreturn = attrs.willreturn
    assume nocapture = attrs.nocapture
    assume cold = attrs.cold
    assume nofree = attrs.nofree
    assume memory_inaccessible_rw = attrs.memory_inaccessible_rw
    assume memory_arg_inaccessible_rw = attrs.memory_arg_inaccessible_rw
    build_jit_types()

    // add default functions
    var jit_exception = LLVMAddFunctionWithType(g_mod, FN_JIT_EXCEPTION,    // jit_exception(text,context *,lineinfo *)
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(g_prim_t.get_type_string(), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_exception, get_jit_exception())
    LLVMAddAttributeToFunction(jit_exception, noreturn)
    // 'cold' tells opt that any branch ending in a jit_exception call is
    // unlikely (panic/assert paths). Pairs with noreturn so block-placement
    // sinks the failure block and keeps the hot path straight.
    LLVMAddAttributeToFunction(jit_exception, cold)
    LLVMAddAttributeToFunctionArgumentRange(jit_exception, urange(0, 2), nocapture)
    // jit_call_or_fastcall(func,args *,context *)
    var jit_call_or_fastcall = LLVMAddFunctionWithType(g_mod, FN_JIT_CALL_OR_FASTCALL,
        LLVMFunctionType(g_prim_t.LLVMFloat4Type(),
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_call_or_fastcall, get_jit_call_or_fastcall())
    LLVMAddAttributeToFunctionArgumentRange(jit_call_or_fastcall, urange(0, 3), nocapture)
    LLVMAddAttributesToFunction(jit_call_or_fastcall, fixed_array(nounwind, willreturn))
    // jit_call_with_cmres(func,args *,cmres *,context *)
    var jit_call_with_cmres = LLVMAddFunctionWithType(g_mod, FN_JIT_CALL_WITH_CMRES,
        LLVMFunctionType(g_prim_t.LLVMFloat4Type(),
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_call_with_cmres, get_jit_call_with_cmres())
    LLVMAddAttributeToFunctionArgumentRange(jit_call_with_cmres, urange(0, 4), nocapture)
    LLVMAddAttributesToFunction(jit_call_with_cmres, fixed_array(nounwind, willreturn))
    // vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context )
    var jit_invoke_block = LLVMAddFunctionWithType(g_mod, FN_JIT_INVOKE_BLOCK,
        LLVMFunctionType(g_prim_t.LLVMFloat4Type(),
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_block, 0u), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_invoke_block, get_jit_invoke_block())
    LLVMAddAttributeToFunctionArgumentRange(jit_invoke_block, urange(0, 3), nocapture)
    LLVMAddAttributesToFunction(jit_invoke_block, fixed_array(nounwind, willreturn))
    // vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context )
    var jit_invoke_block_with_cmres = LLVMAddFunctionWithType(g_mod, FN_JIT_INVOKE_BLOCK_WITH_CMRES,
        LLVMFunctionType(g_prim_t.LLVMFloat4Type(),
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_block, 0u), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_invoke_block_with_cmres, get_jit_invoke_block_with_cmres())
    LLVMAddAttributeToFunctionArgumentRange(jit_invoke_block_with_cmres, urange(0, 4), nocapture)
    LLVMAddAttributesToFunction(jit_invoke_block_with_cmres, fixed_array(nounwind, willreturn))
    // jit_string_builder ( context *, int32 nArgs, TypeInfo ** types, LineInfoArg * at, vec4f * args )
    var jit_string_builder = LLVMAddFunctionWithType(g_mod, FN_JIT_STRING_BUILDER,
        LLVMFunctionType(g_prim_t.get_type_string(),
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), g_prim_t.t_int32, g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType(), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u))))
    LLVMAddGlobalMapping(g_engine, jit_string_builder, get_jit_string_builder())
    LLVMAddAttributesToFunction(jit_string_builder, fixed_array(nounwind, willreturn))
    // jit_string_builder_temp ( context *, int32 nArgs, TypeInfo ** types, LineInfoArg * at, vec4f * args )
    var jit_string_builder_temp = LLVMAddFunctionWithType(g_mod, FN_JIT_STRING_BUILDER_TEMP,
        LLVMFunctionType(g_prim_t.get_type_string(),
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), g_prim_t.t_int32, g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType(), LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u))))
    LLVMAddGlobalMapping(g_engine, jit_string_builder_temp, get_jit_string_builder_temp())
    LLVMAddAttributesToFunction(jit_string_builder_temp, fixed_array(nounwind, willreturn))
    // void * jit_get_global_mnh ( uint64_t mnh, context * )
    var jit_get_global_mnh = LLVMAddFunctionWithType(g_mod, FN_JIT_GET_GLOBAL_MNH,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.t_int64, g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_get_global_mnh, get_jit_get_global_mnh())
    LLVMAddAttributesToFunction(jit_get_global_mnh, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgument(jit_get_global_mnh, 1u, nocapture)
    // void * jit_get_shared_mnh ( uint64_t mnh, context * )
    var jit_get_shared_mnh = LLVMAddFunctionWithType(g_mod, FN_JIT_GET_SHARED_MNH,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.t_int64, g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_get_shared_mnh, get_jit_get_shared_mnh())
    LLVMAddAttributesToFunction(jit_get_shared_mnh, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgument(jit_get_shared_mnh, 1u, nocapture)
    // Wasm-only: void * jit_get_globals_base ( Context * ); same shape for shared.
    // Declared unconditionally (zero cost if unused); only called from llvm_jit.das
    // when g_target_is_wasm. The native path keeps the inlined GEP+load — these
    // helpers exist so the wasm32 runtime archive (which knows its own Context
    // layout) resolves the right field offset.
    var jit_get_globals_base = LLVMAddFunctionWithType(g_mod, FN_JIT_GET_GLOBALS_BASE,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_get_globals_base, get_jit_get_globals_base())
    LLVMAddAttributesToFunction(jit_get_globals_base, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgument(jit_get_globals_base, 0u, nocapture)
    var jit_get_shared_base = LLVMAddFunctionWithType(g_mod, FN_JIT_GET_SHARED_BASE,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_get_shared_base, get_jit_get_shared_base())
    LLVMAddAttributesToFunction(jit_get_shared_base, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgument(jit_get_shared_base, 0u, nocapture)
    // jit_alloc_heap ( uint64_t mnh, context * )
    var jit_alloc_heap = LLVMAddFunctionWithType(g_mod, FN_JIT_ALLOC_HEAP,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.t_int32, g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_alloc_heap, get_jit_alloc_heap())
    LLVMAddAttributesToFunction(jit_alloc_heap, fixed_array(nounwind, willreturn, nofree, memory_inaccessible_rw))
    LLVMAddAttributeToFunctionArgument(jit_alloc_heap, 1u, nocapture)
    // jit_alloc_persistent ( uint64_t mnh, context * )
    var jit_alloc_persistent = LLVMAddFunctionWithType(g_mod, FN_JIT_ALLOC_PERSISTENT,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.t_int32, g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_alloc_persistent, get_jit_alloc_persistent())
    LLVMAddAttributesToFunction(jit_alloc_persistent, fixed_array(nounwind, willreturn, nofree, memory_inaccessible_rw))
    LLVMAddAttributeToFunctionArgument(jit_alloc_persistent, 1u, nocapture)
    // void jit_free_heap ( void * bytes, uint32_t size, Context * context )
    var jit_free_heap = LLVMAddFunctionWithType(g_mod, FN_JIT_FREE_HEAP,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), g_prim_t.t_int32, g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_free_heap, get_jit_free_heap())
    LLVMAddAttributesToFunction(jit_free_heap, fixed_array(nounwind, willreturn, memory_arg_inaccessible_rw))
    LLVMAddAttributeToFunctionArgument(jit_free_heap, 0u, nocapture)
    LLVMAddAttributeToFunctionArgument(jit_free_heap, 2u, nocapture)
    // void jit_free_persistent ( void * bytes, Context * context )
    var jit_free_persistent = LLVMAddFunctionWithType(g_mod, FN_JIT_FREE_PERSISTENT,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_free_persistent, get_jit_free_persistent())
    LLVMAddAttributesToFunction(jit_free_persistent, fixed_array(nounwind, willreturn, memory_arg_inaccessible_rw))
    LLVMAddAttributeToFunctionArgumentRange(jit_free_persistent, urange(0, 2), nocapture)
    // void jit_array_lock ( Array & array, Context * context, LineInfoArg * at )
    var jit_array_lock = LLVMAddFunctionWithType(g_mod, FN_JIT_ARRAY_LOCK,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_array_lock, get_jit_array_lock())
    LLVMAddAttributesToFunction(jit_array_lock, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_array_lock, urange(0, 2), nocapture)
    // void jit_array_unlock ( Array & array, Context * context, LineInfoArg * at )
    var jit_array_unlock = LLVMAddFunctionWithType(g_mod, FN_JIT_ARRAY_UNLOCK,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(
                g_prim_t.LLVMVoidPtrType(), // arr
                g_prim_t.LLVMVoidPtrType(), // ctx
                g_prim_t.LLVMVoidPtrType(), // lineInfo
            )
        ))
    LLVMAddGlobalMapping(g_engine, jit_array_unlock, get_jit_array_unlock())
    LLVMAddAttributesToFunction(jit_array_unlock, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_array_unlock, urange(0, 2), nocapture)

    // int jit_str_cmp ( char * a, char * b )
    var jit_str_cmp = LLVMAddFunctionWithType(g_mod, FN_JIT_STR_CMP,
        LLVMFunctionType(g_prim_t.t_int32,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_prim_t.t_int8, 0u), LLVMPointerType(g_prim_t.t_int8, 0u))))
    LLVMAddGlobalMapping(g_engine, jit_str_cmp, get_jit_str_cmp())
    LLVMAddAttributesToFunction(jit_str_cmp, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_str_cmp, urange(0, 2), nocapture)
    // char * jit_str_cat ( char * a, char * b, Copntext * context )
    var jit_str_cat = LLVMAddFunctionWithType(g_mod, FN_JIT_STR_CAT,
        LLVMFunctionType(LLVMPointerType(g_prim_t.t_int8, 0u),
            fixed_array<LLVMTypeRef>(
                LLVMPointerType(g_prim_t.t_int8, 0u), // sA
                LLVMPointerType(g_prim_t.t_int8, 0u), // sB
                g_prim_t.LLVMVoidPtrType(),           // ctx
                g_prim_t.LLVMVoidPtrType(),           // lineInfo
            )
        ))
    LLVMAddGlobalMapping(g_engine, jit_str_cat, get_jit_str_cat())
    LLVMAddAttributesToFunction(jit_str_cat, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_str_cat, urange(0, 3), nocapture)
    // void jit_prologue ( char * funcName, void * funcLineInfo, int32_t stackSize, JitStackState * stackState, Context * context, LineInfoArg * at )
    var jit_prologue = LLVMAddFunctionWithType(g_mod, FN_JIT_PROLOGUE,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(
                g_prim_t.get_type_string(),    // funcName
                g_prim_t.LLVMVoidPtrType(),    // funcLineInfo
                g_prim_t.t_int32,              // stackSize
                LLVMPointerType(g_t_stack_state, 0u), // stackState
                g_prim_t.LLVMVoidPtrType(),    // context
                g_prim_t.LLVMVoidPtrType()     // at
            )
        )
    )
    LLVMAddGlobalMapping(g_engine, jit_prologue, get_jit_prologue())
    LLVMAddAttributesToFunction(jit_prologue, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_prologue, urange(3, 6), nocapture)
    // void jit_epilogue ( JitStackState * stackState, Context * context )
    var jit_epilogue = LLVMAddFunctionWithType(g_mod, FN_JIT_EPILOGUE,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_stack_state, 0u), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_epilogue, get_jit_epilogue())
    LLVMAddAttributesToFunction(jit_epilogue, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_epilogue, urange(0, 2), nocapture)
    // void jit_make_block ( Block * blk, int32_t argStackTop, uint64_t annotationData, void * bodyNode, void * jitImpl, void * funcInfo, Context * context )
    var jit_make_block = LLVMAddFunctionWithType(g_mod, FN_JIT_MAKE_BLOCK,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(
                LLVMPointerType(g_t_block, 0u), // blk
                g_prim_t.t_int32,               // argStackTop
                g_prim_t.t_int64,               // ad
                g_prim_t.LLVMVoidPtrType(),     // bodyNode
                g_prim_t.LLVMVoidPtrType(),     // jitImpl
                g_prim_t.LLVMVoidPtrType(),     // funcInfo
                g_prim_t.LLVMVoidPtrType(),     // lineInfo
                g_prim_t.LLVMVoidPtrType()      // context
            )))
    LLVMAddGlobalMapping(g_engine, jit_make_block, get_jit_make_block())
    LLVMAddAttributesToFunction(jit_make_block, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgument(jit_make_block, 0u, nocapture)
    LLVMAddAttributeToFunctionArgumentRange(jit_make_block, urange(3, 7), nocapture)
    // void jit_debug ( vec4f res, TypeInfo * typeInfo, char * message, Context * context, LineInfoArg * at )
    var jit_debug = LLVMAddFunctionWithType(g_mod, FN_JIT_DEBUG,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(g_prim_t.LLVMFloat4Type(), g_prim_t.LLVMVoidPtrType(), g_prim_t.get_type_string(), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_debug, get_jit_debug())
    LLVMAddAttributesToFunction(jit_debug, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_debug, urange(1, 5), nocapture)
    // bool builtin_iterator_iterate ( const Sequence & it, void * data, Context * context ) {
    var jit_iterator_iterate = LLVMAddFunctionWithType(g_mod, FN_JIT_ITERATOR_ITERATE,
        LLVMFunctionType(g_prim_t.t_int1,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_sequence, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_iterator_iterate, get_jit_iterator_iterate())
    LLVMAddAttributesToFunction(jit_iterator_iterate, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_iterate, urange(0, 3), nocapture)
    // void builtin_iterator_delete ( const Sequence & it, Context * context ) {
    var jit_iterator_delete = LLVMAddFunctionWithType(g_mod, FN_JIT_ITERATOR_DELETE,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_sequence, 0u), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_iterator_delete, get_jit_iterator_delete())
    LLVMAddAttributesToFunction(jit_iterator_delete, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_delete, urange(0, 2), nocapture)
    // bool builtin_iterator_first ( const Sequence & it, void * data, Context * context, LineInfoArg * at ) {
    var jit_iterator_first = LLVMAddFunctionWithType(g_mod, FN_JIT_ITERATOR_FIRST,
        LLVMFunctionType(g_prim_t.t_int1,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_sequence, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_iterator_first, get_jit_iterator_first())
    LLVMAddAttributesToFunction(jit_iterator_first, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_first, urange(0, 4), nocapture)
    // bool builtin_iterator_next ( const Sequence & it, void * data, Context * context, LineInfoArg * at ) {
    var jit_iterator_next = LLVMAddFunctionWithType(g_mod, FN_JIT_ITERATOR_NEXT,
        LLVMFunctionType(g_prim_t.t_int1,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_sequence, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    // void jit_simnode_interop ( void * ptr, int argCount, TypeInfo ** types )
    let simnode_interop_type = LLVMFunctionType(g_prim_t.t_void,
        fixed_array<LLVMTypeRef>(
            g_prim_t.LLVMVoidPtrType(), // ptr (placement-new buffer for SimNode_AotInteropBase)
            g_prim_t.t_int32,           // argCount
            g_prim_t.LLVMVoidPtrType()  // types
        )
    )
    var jit_simnode_interop = LLVMAddFunctionWithType(
        g_mod, "jit_simnode_interop", simnode_interop_type
    )
    LLVMAddGlobalMapping(g_engine, jit_simnode_interop, get_jit_simnode_interop())
    let simnode_free_interop_type = LLVMFunctionType(g_prim_t.t_void,
        fixed_array<LLVMTypeRef>(
            g_prim_t.LLVMVoidPtrType(), // Node
        )
    )
    var jit_free_simnode_interop = LLVMAddFunctionWithType(
        g_mod, "jit_free_simnode_interop", simnode_free_interop_type
    )
    LLVMAddGlobalMapping(g_engine, jit_free_simnode_interop, get_jit_free_simnode_interop())
    LLVMAddGlobalMapping(g_engine, jit_iterator_next, get_jit_iterator_next())
    LLVMAddAttributesToFunction(jit_iterator_next, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_next, urange(0, 4), nocapture)
    // void builtin_iterator_close ( const Sequence & it, void * data, Context * context ) {
    var jit_iterator_close = LLVMAddFunctionWithType(g_mod, FN_JIT_ITERATOR_CLOSE,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_t_sequence, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_iterator_close, get_jit_iterator_close())
    LLVMAddAttributesToFunction(jit_iterator_close, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_iterator_close, urange(0, 3), nocapture)
    // void das_jit_debug_enter ( char * message, Context * context, LineInfoArg * at ) {
    var jit_debug_enter = LLVMAddFunctionWithType(g_mod, FN_JIT_DEBUG_ENTER,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_prim_t.t_int8, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_debug_enter, get_jit_debug_enter())
    LLVMAddAttributesToFunction(jit_debug_enter, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_debug_enter, urange(0, 3), nocapture)
    // void das_jit_debug_exit ( char * message, Context * context, LineInfoArg * at ) {
    var jit_debug_exit = LLVMAddFunctionWithType(g_mod, FN_JIT_DEBUG_EXIT,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_prim_t.t_int8, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_debug_exit, get_jit_debug_exit())
    LLVMAddAttributesToFunction(jit_debug_exit, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_debug_exit, urange(0, 3), nocapture)
    // void das_jit_debug_line ( char * message, Context * context, LineInfoArg * at ) {
    var jit_debug_line = LLVMAddFunctionWithType(g_mod, FN_JIT_DEBUG_LINE,
        LLVMFunctionType(g_prim_t.t_void,
            fixed_array<LLVMTypeRef>(LLVMPointerType(g_prim_t.t_int8, 0u), g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_debug_line, get_jit_debug_line())
    LLVMAddAttributesToFunction(jit_debug_line, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_debug_line, urange(0, 3), nocapture)
    // void * jit_ast_typedecl ( uint64_t hash, Context * context, LineInfoArg * at )
    var jit_ast_typedecl = LLVMAddFunctionWithType(g_mod, FN_JIT_AST_TYPEDECL,
        LLVMFunctionType(g_prim_t.LLVMVoidPtrType(),
            fixed_array<LLVMTypeRef>(g_prim_t.t_int64, g_prim_t.LLVMVoidPtrType(), g_prim_t.LLVMVoidPtrType())))
    LLVMAddGlobalMapping(g_engine, jit_ast_typedecl, get_jit_ast_typedecl())
    LLVMAddAttributesToFunction(jit_ast_typedecl, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_ast_typedecl, urange(1, 3), nocapture)
    // void jit_init_extern_function ( const char * moduleName, const char * funcMangledName, void ** dllGlobal )
    let jit_init_extern_type = LLVMFunctionType(g_prim_t.t_void,
        fixed_array<LLVMTypeRef>(
            g_prim_t.LLVMVoidPtrType(),  // const char * moduleName
            g_prim_t.LLVMVoidPtrType(),  // const char * funcMangledName
            g_prim_t.LLVMVoidPtrType()   // void ** dllGlobal
        ))
    var jit_init_extern = LLVMAddFunctionWithType(g_mod, FN_JIT_INIT_EXTERN, jit_init_extern_type)
    g_fn_types[FN_JIT_INIT_EXTERN] = jit_init_extern_type
    LLVMAddGlobalMapping(g_engine, jit_init_extern, get_jit_init_extern_function())
    LLVMAddAttributesToFunction(jit_init_extern, fixed_array(nounwind, willreturn))
    LLVMAddAttributeToFunctionArgumentRange(jit_init_extern, urange(0, 3), nocapture)
}


def private build_jit_types {
    // simfunction type
    var simfunction_fields <- fixed_array(
        LLVMPointerType(g_prim_t.t_int8, 0u),// char * name
        LLVMPointerType(g_prim_t.t_int8, 0u),// char * mangledName
        g_prim_t.LLVMVoidPtrType(),// SimNode * code               // note void *
        g_prim_t.LLVMVoidPtrType(),// FuncInfo *  debugInfo        // note void *
        g_prim_t.t_int64,                    // uint64_t    mangledNameHash
        g_prim_t.LLVMVoidPtrType(),// void *      aotFunction
        g_prim_t.t_int32,                    // uint32_t    stackSize
        g_prim_t.t_int32                     // uint32_t    flags
    )
    g_t_simFunction = g_prim_t |> StructType(simfunction_fields)
    // function type
    var function_fields <- fixed_array(
        LLVMPointerType(g_t_simFunction, 0u)// SimFunction * func
    )
    g_t_function = g_prim_t |> StructType(function_fields)
    // lambda type
    var lambda_fields <- fixed_array(
        LLVMPointerType(g_t_simFunction, 0u),   // SimFunction * __lambda
        LLVMPointerType(g_t_simFunction, 0u)    // SimFunction * __finalize
    )
    g_t_lambda = g_prim_t |> StructType(lambda_fields)
    // block type
    var block_fields <- build_block_type()
    g_t_block = g_prim_t |> StructType(block_fields)
    // prologue\epilogue storage for the stack
    var stack_state_fields <- fixed_array(
        LLVMPointerType(g_prim_t.t_int8, 0u),   // char * EP
        LLVMPointerType(g_prim_t.t_int8, 0u)    // char * SP
    )
    g_t_stack_state = g_prim_t |> StructType(stack_state_fields)
    // iterator
    var iterator_fields <- fixed_array(
        g_prim_t.LLVMVoidPtrType(),   // __vtable
        g_prim_t.t_int1                         // bool isOpen
    )
    g_t_iterator = g_prim_t |> StructType(iterator_fields)
    // sequence
    var sequence_fields <- fixed_array(
        LLVMPointerType(g_t_iterator, 0u)       // iterator * iter
    )
    g_t_sequence = g_prim_t |> StructType(sequence_fields)
}

def public build_block_type {
    return <- [
        g_prim_t.t_int32,                                // uint32_t    stackOffset;
        g_prim_t.t_int32,                                // uint32_t    argumentsOffset;
        g_prim_t.LLVMVoidPtrType(),                      // SimNode *   body;
        g_prim_t.LLVMVoidPtrType(),                      // void *      aotFunction;
        g_prim_t.LLVMVoidPtrType(),                      // void *      jitFunction;
        LLVMPointerType(g_prim_t.LLVMFloat4Type(), 0u),  // vec4f *     functionArguments;
        g_prim_t.LLVMVoidPtrType()                       // FuncInfo *  info;
    ]
}

// opt_level: 0..3 — picks LLVMCodeGenOptLevel for codegen-side passes
// use_host_cpu: when true, plumb the host CPU name + feature set into the
//   TargetMachine. Without this, TargetTransformInfo returns a generic
//   baseline cost model, which throttles loop unrolling, register-pressure
//   estimation, and vectorization decisions for the IR-level pipeline
//   driven by LLVMRunPasses. Use true for JIT and JIT-DLL-cache emission
//   (the artifact only ever runs on this host); use false for redistributable
//   exe/dll emission where the binary must run on any compatible CPU.
// Build a TargetMachine for an arbitrary triple/cpu/features. Caller owns
// triple/cpu/features (we do not LLVMDisposeMessage them — they are plain
// das `string`s here, not heap-owned LLVM messages).
[macro_function]
def public with_target_machine(triple, cpu, features : string; opt_level : uint;
                               blk : block<(LLVMTargetMachineRef) : void>) {
    var target : LLVMTarget?
    let rc = LLVMGetTargetFromTriple(triple, unsafe(addr(target)), null)
    if (rc != 0 || target == null) {
        panic("LLVM: unknown / unsupported target triple '{triple}'")
    }

    let cg_level = (
        (opt_level >= 3u) ? LLVMCodeGenOptLevel.LLVMCodeGenLevelAggressive
      : (opt_level >= 2u) ? LLVMCodeGenOptLevel.LLVMCodeGenLevelDefault
      : (opt_level >= 1u) ? LLVMCodeGenOptLevel.LLVMCodeGenLevelLess
                          : LLVMCodeGenOptLevel.LLVMCodeGenLevelNone)

    let targetMachine = LLVMCreateTargetMachine(
        target,
        triple,
        cpu,
        features,
        cg_level,
        LLVMRelocMode.LLVMRelocPIC, // PIC in order to have shared object
        LLVMCodeModel.Default
    )

    invoke(blk, targetMachine)
    LLVMDisposeTargetMachine(targetMachine)
}

[macro_function]
def public with_default_target_machine(opt_level : uint; use_host_cpu : bool;
                                       blk : block<(LLVMTargetMachineRef) : void>) {
    LLVMInitializeAllTargetInfos()
    LLVMInitializeAllTargets()
    LLVMInitializeAllTargetMCs()
    LLVMInitializeAllAsmPrinters()

    // host_jit_triple() returns the daslang-host toolchain's triple when it
    // differs from LLVM.dll's default. On clang-mingw64, the prebuilt LLVM.dll
    // (MSVC clang-cl-built) defaults to x86_64-pc-windows-msvc — emitting
    // MSVC-ABI .o files that mingw's linker can't satisfy (_fltused etc).
    // Empty string means "use LLVM default" (existing MSVC / linux / macOS
    // builds where LLVM.dll's triple already matches the host).
    let host_triple = host_jit_triple()
    var triple_msg : string
    var triple_owned_by_llvm = false
    if (empty(host_triple)) {
        triple_msg = LLVMGetDefaultTargetTriple()
        triple_owned_by_llvm = true
    } else {
        triple_msg = host_triple
    }
    let cpu_msg = use_host_cpu ? LLVMGetHostCPUName() : ""
    let features_msg = use_host_cpu ? LLVMGetHostCPUFeatures() : ""

    with_target_machine(triple_msg, cpu_msg, features_msg, opt_level, blk)

    if (use_host_cpu) {
        LLVMDisposeMessage(cpu_msg)
        LLVMDisposeMessage(features_msg)
    }
    if (triple_owned_by_llvm) {
        LLVMDisposeMessage(triple_msg)
    }
}

// @mod - module to dump to dll
// @out_path - folder and name of file, without extension
// @path_to_dascript_lib - path to library, required on Windows. Has no effect on Linux.
// @path_to_linker - path to clang-cl on Windows. By default we'll use clang-cl from LLVM package
// on Windows, default otherwise.
def public write_dll(mod : LLVMOpaqueModule?; out_path : string; path_to_dascript_lib, path_to_linker, linker_string : string; link_whole_lib : bool) {
    // JIT DLL cache: emitted artifact only ever runs on this host.
    with_default_target_machine(3u, true) $(targetMachine : LLVMTargetMachineRef) {
        let error : string?
        let file = add_obj_extension(out_path)
        let filetype = LLVMCodeGenFileType.LLVMObjectFile
        LLVMTargetMachineEmitToFile(targetMachine, mod, file, filetype, error)
        let ok = create_shared_library(file, add_dll_extension(out_path), "{path_to_dascript_lib}", "{path_to_linker}", "{linker_string}", true, link_whole_lib)
        if (!ok) {
            panic("write_dll: link failed for {out_path}")
        }
    }
}

def public write_exe(mod : LLVMOpaqueModule?; out_path : string; path_to_dascript_lib, path_to_linker, linker_string : string; link_whole_lib : bool) {
    // Standalone exe: must run on any compatible CPU, keep generic.
    with_default_target_machine(3u, false) $(targetMachine : LLVMTargetMachineRef) {
        let error : string?
        let file = add_obj_extension(out_path)
        let filetype = LLVMCodeGenFileType.LLVMObjectFile
        LLVMTargetMachineEmitToFile(targetMachine, mod, file, filetype, error)
        let ok = create_shared_library(file, add_exe_extension(out_path), "{path_to_dascript_lib}", "{path_to_linker}", "{linker_string}", false, link_whole_lib)
        if (!ok) {
            panic("write_exe: link failed for {out_path}")
        }
    }
}

// Locate the wasm32 libDaScript_runtime.a. If `override_path` is non-empty it
// wins (caller-supplied path, e.g. from --jit-runtime-lib CLI flag) and a
// missing file logs a warning. Otherwise auto-locates the archive at
// <das_root>/web/output/lib/liblibDaScript_runtime.a.
def private find_runtime_lib(override_path : string) : Option<string> {
    if (!(override_path |> empty())) {
        if (stat(override_path).is_valid) return some(override_path)
        to_log(LOG_WARNING, "wasm: --jit-runtime-lib path does not exist: {override_path}\n")
        return none(type<string>)
    }
    let candidate = "{get_das_root()}/web/output/lib/liblibDaScript_runtime.a"
    return stat(candidate).is_valid ? some(candidate) : none(type<string>)
}

// Cross-compile a module to wasm32 and link to a runnable .wasm via emcc.
// triple defaults to wasm32-unknown-emscripten (matches the runtime archive).
// path_to_emcc may be empty; link_wasm falls back to bin/emcc then PATH.
// needs_runtime gates inclusion of libDaScript_runtime.a in the link:
// true → archive is linked when found, otherwise warning + link without;
// false → archive is omitted unconditionally (pure programs).
// explicit_runtime is an optional caller-supplied archive path.
def public write_wasm(mod : LLVMOpaqueModule?; out_path : string; triple : string; path_to_emcc : string; needs_runtime : bool; explicit_runtime : string = "") {
    LLVMInitializeWasmTarget()
    let runtime_lib = find_runtime_lib(explicit_runtime)
    if (needs_runtime && is_none(runtime_lib)) {
        to_log(LOG_WARNING, "wasm: runtime symbols referenced but libDaScript_runtime.a not found at web/output/lib/ — run the web/ emscripten build to link in the runtime, or pass --jit-runtime-lib=<path>\n")
    }
    let real_triple = (!(triple |> empty()) ? triple : "wasm32-unknown-emscripten")
    // Match the feature set web/CMakeLists.txt compiles the runtime archive with
    // (-msimd128 -mnontrapping-fptoint). Without +simd128 the host LLVM lowers
    // vec4f returns via sret while emcc returns them as native v128, causing
    // function-signature mismatches at wasm-ld link time and `unreachable`
    // traps at runtime on any helper that returns vec4f (jit_invoke_block_*,
    // jit_call_*).
    let features = "+simd128,+nontrapping-fptoint"
    with_target_machine(real_triple, "", features, 2u) $(targetMachine : LLVMTargetMachineRef) {
        // Pin the module's data layout + triple so codegen sizes pointers as
        // 32-bit. Without this the module inherits host layout and produces
        // wrong-sized loads/stores in the emitted wasm.
        let dl = LLVMCreateTargetDataLayout(targetMachine)
        LLVMSetModuleDataLayout(mod, dl)
        LLVMSetTarget(mod, real_triple)
        LLVMDisposeTargetData(dl)

        let error : string?
        let obj = add_obj_extension(out_path)
        let filetype = LLVMCodeGenFileType.LLVMObjectFile
        LLVMTargetMachineEmitToFile(targetMachine, mod, obj, filetype, error)
        let runtime_arg = needs_runtime ? (runtime_lib ?? "") : ""
        let ok = link_wasm(obj, "{out_path}.wasm", runtime_arg, path_to_emcc)
        if (!ok) {
            panic("write_wasm: link failed for {out_path}.wasm")
        }
    }
}

def public build_string_constant(message : string) {
    if (g_str2v |> key_exists(message)) return g_str2v[message]
    let msg_size = uint(length(message))
    var strType = LLVMArrayType(g_prim_t.t_int8, msg_size + 1u)
    // Prefix all string variable names to not intersect with function names.
    var str = LLVMAddGlobal(g_mod, strType, ".str.{message}")
    LLVMSetInitializer(str, LLVMConstStringInContext(g_ctx, message, msg_size, 0))
    LLVMSetGlobalConstant(str, 1)
    LLVMSetLinkage(str, LLVMLinkage.LLVMPrivateLinkage)
    LLVMSetUnnamedAddress(str, LLVMUnnamedAddr.LLVMGlobalUnnamedAddr)
    LLVMSetAlignment(str, 1u)
    g_str2v[message] = str
    return str
}

def public get_basetype_pointer(t : Type) : LLVMOpaqueType? {
    return LLVMPointerType(base_type_to_llvm_type(t), 0u)
}

def public get_type_pointer(t : TypeDeclPtr) : LLVMOpaqueType? {
    if (t == null || t.isVoid) {
        return g_prim_t.LLVMVoidPtrType()
    } else {
        return LLVMPointerType(type_to_llvm_type(t), 0u)
    }
}

def public get_empty_string(builder : LLVMOpaqueBuilder?) {
    return LLVMBuildPointerCast(builder, build_string_constant(""), g_prim_t.get_type_string(), "empty string_constant")
}

def public get_string_constant_ptr(builder : LLVMOpaqueBuilder?; message : string) {
    if (empty(message)) return LLVMConstPointerNull(g_prim_t.get_type_string())
    var str = build_string_constant(message)
    return LLVMBuildPointerCast(builder, str, g_prim_t.get_type_string(), "string_constant {message}")
}

def public failed(msg : string) {
    to_log(LOG_ERROR, "LLVM JIT: {msg}\n")
    g_errors |> push("{msg}")
}

def public failed_E(e : Expression?; msg : string) {
    failed("{msg}. expr = {e.__rtti} at {describe(e.at)}")
}

def public failed_V(v : VariablePtr; msg : string) {
    failed("{msg}. var = {describe(v)} at {describe(v.at)}")
}

def public failed_T(v : TypeDeclPtr; msg : string) {
    failed("{msg}. var = {describe(v)} at {describe(v.at)}")
}

def public base_type_to_llvm_type(t : Type) {
    return g_prim_t.t_int32 if (t == Type.tInt || t == Type.tUInt || t == Type.tBitfield || t == Type.tEnumeration)
    return g_prim_t.t_int64 if (t == Type.tInt64 || t == Type.tUInt64 || t == Type.tBitfield64 || t == Type.tEnumeration64)
    return g_prim_t.t_int16 if (t == Type.tInt16 || t == Type.tUInt16 || t == Type.tBitfield16 || t == Type.tEnumeration16)
    return g_prim_t.t_int8 if (t == Type.tInt8 || t == Type.tUInt8 || t == Type.tBitfield8 || t == Type.tEnumeration8)
    return g_prim_t.t_float if (t == Type.tFloat)
    return g_prim_t.t_double if (t == Type.tDouble)
    return g_prim_t.t_int1 if (t == Type.tBool)
    return g_prim_t.LLVMInt2Type() if (t == Type.tInt2 || t == Type.tUInt2 || t == Type.tRange || t == Type.tURange)
    return g_prim_t.LLVMInt3Type() if (t == Type.tInt3 || t == Type.tUInt3)
    return g_prim_t.LLVMInt4Type() if (t == Type.tInt4 || t == Type.tUInt4)
    return g_prim_t.LLVMRange64Type() if (t == Type.tRange64 || t == Type.tURange64)
    return g_prim_t.LLVMFloat2Type() if (t == Type.tFloat2)
    return g_prim_t.LLVMFloat3Type() if (t == Type.tFloat3)
    return g_prim_t.LLVMFloat4Type() if (t == Type.tFloat4)
    return g_prim_t.get_type_string() if (t == Type.tString)
    return g_prim_t.LLVMVoidPtrType() if (t == Type.tPointer)
    failed("base_type_to_llvm_type: unsupported type {t}")
    return g_prim_t.t_int32
}

def public type_to_llvm_abi_type(t : TypeDeclPtr) {
    if (t.baseType == Type.tBlock) {
        return LLVMPointerType(g_t_block, 0u)
    } elif (t.isRef) {
        return LLVMPointerType(type_to_llvm_type(t), 0u)
    } elif (t.isPointer) {
        return get_type_pointer(t.firstType)
    } elif (t.isWorkhorseType) {
        return base_type_to_llvm_type(t.baseType)
    } elif (t.baseType == Type.tFunction || t.baseType == Type.tLambda) {
        return type_to_llvm_type(t)
    } elif (t.isVoid) {
        return g_prim_t.t_void
    } elif (t.isAnyType) {
        return g_prim_t.LLVMFloat4Type()
    } elif (t.baseType == Type.fakeContext || t.baseType == Type.fakeLineInfo) {
        return g_prim_t.LLVMVoidPtrType()
    } elif (t.baseType == Type.tHandle) {
        if (t.isRefType) {
            failed("type_to_llvm_abi_type: internal error. unsupported type {describe(t)} at {describe(t.at)}")
            return g_prim_t.LLVMVoidPtrType()
        } else {
            var tp = get_underlying_value_type(t)
            if (tp == null) {
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(t)} at {describe(t.at)}")
                return g_prim_t.LLVMFloat4Type()
            } elif (!tp.isWorkhorseType && tp.baseType != Type.anyArgument) {
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(t)} at {describe(t.at)}")
                return g_prim_t.LLVMFloat4Type()
            }
            return type_to_llvm_abi_type(tp)
        }
    } elif (t.baseType == Type.anyArgument) {
        return g_prim_t.LLVMFloat4Type()
    } else {
        failed("type_to_llvm_abi_type: unsupported type {describe(t)} at {describe(t.at)}")
        return g_prim_t.t_int32
    }
}

def public type_to_llvm_type(t : TypeDeclPtr) {
    var res : LLVMOpaqueType?
    if (t.baseType == Type.tFixedArray && t.firstType != null) {
        // natural recursion: [outer x [inner x elem]] — same nesting the reversed flat walk built
        res = LLVMArrayType(type_to_llvm_type(t.firstType), uint(t.fixedDim))
    } elif (t.isPointer) {
        res = get_type_pointer(t.firstType)
    } elif (t.isWorkhorseType) {
        res = base_type_to_llvm_type(t.baseType)
    } elif (t.isStructure) {
        res = LLVMArrayType(g_prim_t.t_int8, uint(t.sizeOf))
    } elif (t.isTuple) {
        res = LLVMArrayType(g_prim_t.t_int8, uint(t.sizeOf))
    } elif (t.isVariant) {
        res = LLVMArrayType(g_prim_t.t_int8, uint(t.sizeOf))
    } elif (t.baseType == Type.tFunction) {
        res = g_t_function
    } elif (t.baseType == Type.tLambda) {
        res = g_prim_t.LLVMVoidPtrType()
    } elif (t.baseType == Type.tIterator) {
        res = g_t_sequence
    } elif (t.baseType == Type.tBlock) {
        res = g_t_block
    } elif (t.baseType == Type.tArray) {
        // array type — must mirror Array layout in arraytype.h byte-for-byte
        var array_fields <- fixed_array(
            g_prim_t.LLVMVoidPtrType(),// char * data
            g_prim_t.t_int64,                    // uint64_t size
            g_prim_t.t_int64,                    // uint64_t capacity
            g_prim_t.t_int32,                    // uint32_t magic
            g_prim_t.t_int32,                    // uint32_t lock
            g_prim_t.t_int32,                    // uint32_t flags
            g_prim_t.t_int32                     // _pad_after_flags (mirror trailing alignment pad)
        )
        if (t.firstType != null) {
            array_fields[int(JIT_ARRAY.DATA)] = LLVMPointerType(type_to_llvm_type(t.firstType), 0u)
        }
        res = g_prim_t |> StructType(array_fields)
    } elif (t.baseType == Type.tTable) {
        // table type — must mirror Table layout (Array base + keys/hashes/tombstones).
        // The explicit _pad_after_flags slot forces keys onto an 8-aligned offset on
        // 32-bit ABIs, matching the C++ Table (where Table::keys lives at sizeof(Array)==40).
        var table_fields <- fixed_array(
            g_prim_t.LLVMVoidPtrType(),// char * data
            g_prim_t.t_int64,                    // uint64_t size
            g_prim_t.t_int64,                    // uint64_t capacity
            g_prim_t.t_int32,                    // uint32_t magic
            g_prim_t.t_int32,                    // uint32_t lock
            g_prim_t.t_int32,                    // uint32_t flags
            g_prim_t.t_int32,                    // _pad_after_flags
            g_prim_t.LLVMVoidPtrType(),// char * keys
            g_prim_t.LLVMVoidPtrType(),// TableHashKey * hashes
            g_prim_t.t_int64)                    // uint64_t tombstones
        if (t.firstType != null) {
            table_fields[int(JIT_TABLE.KEYS)] = LLVMPointerType(type_to_llvm_type(t.firstType), 0u)
        }
        if (t.secondType != null) {
            table_fields[int(JIT_ARRAY.DATA)] = t.secondType.isVoid ? g_prim_t.LLVMVoidPtrType() : LLVMPointerType(type_to_llvm_type(t.secondType), 0u)
        }
        res = g_prim_t |> StructType(table_fields)
    } elif (t.isVoid) {
        res = g_prim_t.t_void
    } elif (t.isAnyType) {
        res = g_prim_t.LLVMFloat4Type()
    } elif (t.isHandle) {
        if (t.isRefType) {
            res = LLVMArrayType(g_prim_t.t_int8, uint(t.sizeOf))
        } else {
            var tp = get_underlying_value_type(t)
            if (tp == null) {
                failed("internal integration error.`get_underlying_value_type` returned null for {describe(t)} at {describe(t.at)}")
                return g_prim_t.t_int32
            } elif (!tp.isWorkhorseType && tp.baseType != Type.anyArgument) {
                failed("internal error. expecting workhorse type from `get_underlying_value_type` for {describe(t)} at {describe(t.at)}")
                return g_prim_t.t_int32
            }
            res = type_to_llvm_type(tp)
        }
    } else {
        failed("type_to_llvm_type: unsupported type {describe(t)} at {describe(t.at)}")
        res = g_prim_t.t_int32
    }
    return res
}

def public dim_element_type_to_llvm_type(typ : TypeDeclPtr) {
    if (typ.baseType != Type.tFixedArray) {
        return type_to_llvm_type(typ)
    } else {
        var ndt = clone_type(typ)
        while (ndt.baseType == Type.tFixedArray && ndt.firstType != null) {   // scalar element at the chain leaf
            ndt = clone_type(ndt.firstType)
        }
        return type_to_llvm_type(ndt)
    }
}

def public build_broadcast_vector(builder : LLVMOpaqueBuilder?; opType : TypeDeclPtr; right : LLVMOpaqueValue?) {
    if (opType.isVectorType) {
        var vres = LLVMGetUndef(type_to_llvm_type(opType))
        for (i in range(opType.vectorDim)) {
            vres = LLVMBuildInsertElement(builder, vres, right, g_prim_t.ConstI32(uint64(i)), "")
        }
        return vres
    } else {
        return right
    }
}

def public LLVMBuildLoadData2Aligned(builder : LLVMOpaqueBuilder?; var typ : LLVMOpaqueType?; ptr : LLVMOpaqueValue?; alignment : int; name : string) {
    if (!LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS && alignment != 16 && LLVMIsVector3(typ)) {
        return LLVMBuildLoadVector3(builder, g_prim_t, typ, ptr, name)
    } else {
        return LLVMBuildLoad2Aligned(builder, typ, ptr, alignment |> uint, name)
    }
}

def public LLVMBuildMemSet(builder : LLVMOpaqueBuilder?; ptr : LLVMOpaqueValue?; value : uint64; length : uint64; alignment : uint) {
    LLVMBuildMemSet(builder, ptr, LLVMConstInt(g_prim_t.t_int8, value, 0), g_prim_t.ConstI32(length), alignment)
}

def public expand_scalar(builder : LLVMOpaqueBuilder?; scalar : LLVMOpaqueValue?; vecType : TypeDeclPtr) {
    var vec = LLVMGetUndef(type_to_llvm_type(vecType))
    for (d in range(vecType.vectorDim)) {
        var idx = g_prim_t.ConstI32(uint64(d))
        vec = LLVMBuildInsertElement(builder, vec, scalar, idx, "")
    }
    return vec
}

