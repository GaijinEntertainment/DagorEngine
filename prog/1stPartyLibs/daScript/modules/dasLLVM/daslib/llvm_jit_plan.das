options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = false
options relaxed_pointer_const
options stack = 4_194_304
options no_global_variables = false

module llvm_jit_plan shared private

//! The half of the JIT driver a cache hit needs, with no emitter behind it: the option
//! resolution, the candidate set, the DLL key, the DLL probe and the install phase.
//! `llvm_jit_link` runs a hit from here alone and brings the emitter (`llvm_jit_run`) in for
//! a miss; the emitter builds on the same plan and the same install. The three host queries
//! the key folds - triple, CPU name, CPU features - are the one use of the LLVM bindings here.

require llvm/bindings/llvm_host
require llvm/daslib/llvm_dll_utils public
require llvm/daslib/llvm_env
require llvm/daslib/llvm_cpu_class
require llvm/daslib/llvm_jit_cli
require daslib/ast_boost
require daslib/strings_boost
require daslib/fio
require daslib/option
require daslib/result
require jit

// Bump this constant whenever LLVM codegen / linking logic changes in a way that
// invalidates cached DLLs (e.g. edits to llvm_jit.das, llvm_macro.das, llvm_jit_common.das,
// runtime helper ABI, default target triple). Cache filenames fold this in, so a bump
// makes every previously written DLL miss the cache on the next run and get GC'd.
let public LLVM_JIT_CODEGEN_VERSION : uint64 = 0x93ul   // 0x93: a string constant with null bytes emits every byte

let public JIT_FNV_PRIME : uint64 = 1099511628211ul

// `options fast_math` stamps every FP op with VALUE-SAFE flags (reassoc|nsz|contract) before opt —
// NON-bit-exact, default off, +8-10% e2e. nnan|ninf|arcp|afn are deliberately NOT stamped: engine
// kernels make real infs and arcp+ninf's rcp-estimate div computes inf*0=NaN (tests/jit/fast_math_specials.das).
var public g_jit_fast_math = false

//! `--jit-sanitize` / `options jit_sanitize`, resolved: `address` runs LLVM's asan pass over the emitted IR, `undefined`
//! its bounds check, which reports through the host's ubsan runtime
struct public JitSanitize {
    address : bool
    undefined : bool
}

let public JIT_UBSAN_RUNTIME_SYMBOL = "__ubsan_handle_local_out_of_bounds_abort"

//! whether this process carries `symbol` - a sanitizer runtime linked into the host exports its entry points
def public host_has_symbol(symbol : string) : bool {
    LLVMLoadLibraryPermanently("")
    return LLVMSearchForAddressOfSymbol(symbol) != null
}

//! the host runs the asan runtime: its context stack and array buffers are poisoned past what the C++ runtime handed out
let public JIT_HOST_ASAN = host_has_symbol("__asan_init")

let public LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT = false   // NOTE - this should be true, to match interpreter and AOT behavior
var public LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT

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

def public jit_sec(us : int) : double => double(us) / 1000000.0lf

// nolint:STYLE014
// Loop tune hints (vectorize / vectorize_width / unroll / unroll_count / interleave_count)
// live on ExprFor / ExprWhile annotations and reach ONLY the JIT — attach_loop_metadata
// turns them into LLVM !llvm.loop metadata. They never enter the interpreter SimNode tree
// that get_function_aot_hash walks, so the semantic hash is blind to them: a re-tuned
// kernel ([tuned] restamping a different perm from a changed box_profile.json) has an
// IDENTICAL aot hash and would silently reuse a stale DLL. Fold them into the JIT-only
// cache key here, alongside LLVM_JIT_CODEGEN_VERSION / g_jit_fast_math.
// Fold the ONE value field the arg's type says is live. bValue/iValue/fValue share a C++
// union — reading a non-live member is UB (a bool load of vectorize_width=8 aborts ubsan
// and lets clang -O2 miscompile: chunk workers died with SIGSEGV on linux Release CI).
// Type coverage stays total: any basicType re-keys the DLL via its own live field.
def public fold_annotation_args(args : AnnotationArgumentList; seed : uint64) : uint64 {
    var h = seed
    for (a in args) {
        h = (h ^ hash(a.name)) * JIT_FNV_PRIME
        h = (h ^ uint64(int(a.basicType))) * JIT_FNV_PRIME
        if (a.basicType == Type.tBool) {
            h = (h ^ (a.bValue ? 1ul : 0ul)) * JIT_FNV_PRIME
        } elif (a.basicType == Type.tInt) {
            h = (h ^ uint64(a.iValue)) * JIT_FNV_PRIME
        } elif (a.basicType == Type.tFloat) {
            h = (h ^ hash(a.fValue)) * JIT_FNV_PRIME
        } elif (a.basicType == Type.tString) {
            h = (h ^ hash(a.sValue)) * JIT_FNV_PRIME
        }
    }
    return h
}

class private LoopHintHashVisitor : AstVisitor {
    h : uint64
    def override preVisitExprFor(expr : ExprFor?) : void {
        h = fold_annotation_args(expr.annotations, h)
    }
    def override preVisitExprWhile(expr : ExprWhile?) : void {
        h = fold_annotation_args(expr.annotations, h)
    }
}

// Complete under jit_all_functions (the default): every used function is a top-level entry
// in `funcs`, so folding each function's own body covers the whole compiled program.
def public fold_loop_hints(funcs : array<FunctionPtr>; seed : uint64) : uint64 {
    var vis = new LoopHintHashVisitor()
    vis.h = seed
    make_visitor(*vis) $(adapter) {
        for (fn in funcs) {
            visit(fn.body, adapter)
        }
    }
    let r = vis.h
    unsafe { delete vis }
    return r
}

// [llvm_code] annotation args are generator params: they change emitted IR without touching the
// reference body, so the aot hash is blind — fold them like loop hints. A generator CODE change is
// invisible too: bump LLVM_JIT_CODEGEN_VERSION when a generator's emission changes for fixed args.
def public fold_llvm_code_hints(funcs : array<FunctionPtr>; seed : uint64) : uint64 {
    var h = seed
    for (fn in funcs) {
        for (ann in fn.annotations) {
            if (ann.annotation.name == "llvm_code") {
                h = (h ^ hash("llvm_code")) * JIT_FNV_PRIME
                h = fold_annotation_args(ann.arguments, h)
            }
        }
    }
    return h
}

// [hint(...)] args become LLVM function attributes (alwaysinline/noalias=/optnone/...) — backend-only,
// no SimNode, so the aot hash is blind to them exactly like loop hints; fold them into both keys
def public fold_function_hints(funcs : array<FunctionPtr>; seed : uint64) : uint64 {
    var h = seed
    for (fn in funcs) {
        for (ann in fn.annotations) {
            if (ann.annotation.name == "hint") {
                h = (h ^ hash("hint")) * JIT_FNV_PRIME
                h = fold_annotation_args(ann.arguments, h)
            }
        }
    }
    return h
}

// True when the program jits any [llvm_code] kernel — those emit host-specific IR (e.g. AVX2) a
// generic x86-64 exe can't legalize, so a -exe carrying them must target the current box. Scanned
// BEFORE target flags: the generators' decline predicates consult g_target_*, so order matters.
def private has_generated_kernel(prog : Program?; jit_all_functions : bool) : bool {
    var found = false
    prog |> for_each_module() $(mod) {
        mod |> for_each_function("") $(fun) {
            if (is_used(prog, fun) && (jit_all_functions || fun.moreFlags.requestJit) && !fun.moreFlags.requestNoJit) {
                for (ann in fun.annotations) {
                    if (ann.annotation.name == "llvm_code") {
                        found = true
                    }
                }
            }
        }
    }
    return found
}

// The environment/config salt shared by the whole-DLL key AND the per-partition obj cache key.
// ONE helper on purpose: a salt component folded into the DLL key but not the partition key
// would let a config change reuse stale cached partition objects.
def public jit_env_salt(opt_level : int; size_level : int; emit_prologue : bool; debug_info : bool; // nolint:STYLE037,STYLE038 - one flat fold per emission input; a split would hide which inputs key the cache
                        target_triple : string; split : bool; use_host_cpu : bool; sanitize : JitSanitize) : uint64 {
    var h = LLVM_JIT_CODEGEN_VERSION
    // host/binary identity: object format follows the triple (COFF vs ELF on one checkout), and the emitting binary bakes its Context offsets into every global access - neither is in any AST hash
    if (empty(target_triple)) {
        let host_triple = LLVMGetDefaultTargetTriple()
        h = (h ^ hash(host_triple)) * JIT_FNV_PRIME
        LLVMDisposeMessage(host_triple)
    }
    h = (h ^ uint64(CONTEXT_OFFSET_OF_GLOBALS)) * JIT_FNV_PRIME
    h = (h ^ uint64(CONTEXT_OFFSET_OF_SHARED)) * JIT_FNV_PRIME
    h = (h ^ uint64(CONTEXT_OFFSET_OF_STOP_FLAGS)) * JIT_FNV_PRIME
    h = (h ^ uint64(CONTEXT_OFFSET_OF_EVAL_TOP)) * JIT_FNV_PRIME
    h = (h ^ uint64(SIMFUNCTION_OFFSET_OF_JIT_FUNCTION)) * JIT_FNV_PRIME
    h = (h ^ uint64(ARRAY_MAGIC)) * JIT_FNV_PRIME
    h = (h ^ uint64(ARRAY_LOCK_EXEMPT_FLAGS)) * JIT_FNV_PRIME
    // full host-CPU identity when the TargetMachine bakes it - the cpuid bits below cover only the matrix-tier features; a wider box's object is an illegal instruction on a narrower one
    if (use_host_cpu) {
        let host_cpu = LLVMGetHostCPUName()
        let host_features = LLVMGetHostCPUFeatures()
        h = (h ^ hash(host_cpu)) * JIT_FNV_PRIME
        h = (h ^ hash(host_features)) * JIT_FNV_PRIME
        LLVMDisposeMessage(host_cpu)
        LLVMDisposeMessage(host_features)
    }
    // DAS_JIT_PROBE_LTO artifacts (bitcode + lld-LTO link) must never serve a normal run
    if (g_env_jit.jit_probe_lto) {
        h = (h ^ hash("lto_probe")) * JIT_FNV_PRIME
    }
    // options jit_vec3_ldu selects the vector-load emission shape
    h = (h ^ (LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS ? 1ul : 0ul)) * JIT_FNV_PRIME
    h = (h ^ uint64(opt_level)) * JIT_FNV_PRIME
    h = (h ^ uint64(size_level)) * JIT_FNV_PRIME
    h = (h ^ (emit_prologue ? 1ul : 0ul)) * JIT_FNV_PRIME
    h = (h ^ (debug_info ? 1ul : 0ul)) * JIT_FNV_PRIME
    h = (h ^ (g_jit_fast_math ? 1ul : 0ul)) * JIT_FNV_PRIME   // fast-math changes codegen → distinct cache
    h = (h ^ ((sanitize.address ? 1ul : 0ul) | (sanitize.undefined ? 2ul : 0ul) | (JIT_HOST_ASAN ? 4ul : 0ul))) * JIT_FNV_PRIME
    // split-built and monolith DLLs differ in inlining — never share a cache entry (bool only: partition count is data-driven)
    h = (h ^ (split ? 1ul : 0ul)) * JIT_FNV_PRIME
    // EH vs longjmp hosts emit different unwind tables — must not share a cache entry (panic would crash)
    h = (h ^ (das_is_exceptions_enabled() ? 1ul : 0ul)) * JIT_FNV_PRIME
    // AVX2 gates the x64 intrinsic table — an AVX2-jitted DLL must not cache-hit an AVX2-less host (illegal instr)
    h = (h ^ (cpu_supports("avx2") ? 1ul : 0ul)) * JIT_FNV_PRIME
    // Matrix-tier cpuid bits (vnni256/avx512bw/avx512vnni) + DAS_JIT_X64_FORCE_FEATURES key emission the same way
    var isa = 0ul
    if (cpu_supports("avxvnni")) {
        isa |= 1ul
    }
    if (cpu_supports("avx512f")) {
        isa |= 2ul
    }
    if (cpu_supports("avx512bw")) {
        isa |= 4ul
    }
    if (cpu_supports("avx512vl")) {
        isa |= 8ul
    }
    if (cpu_supports("avx512vnni")) {
        isa |= 16ul
    }
    if (cpu_supports("avx512vbmi")) {   // the grid formats' symbol lattice emits on it (grid = "vbmi")
        h = (h ^ 0x1Bul) * JIT_FNV_PRIME
    }
    if (cpu_supports("amx-tile") && cpu_supports("amx-int8")) {   // tune_cpu_class picks x86-amx, which selects grid decode forms
        h = (h ^ 0x1Cul) * JIT_FNV_PRIME
    }
    if (cpu_supports("avxvnniint8")) {
        isa |= 32ul
    }
    h = (h ^ isa) * JIT_FNV_PRIME
    let forced = g_env_jit.jit_x64_force_features
    if (!empty(forced)) {
        h = (h ^ hash(forced)) * JIT_FNV_PRIME
    }
    // aarch64 twin (DAS_JIT_ARM64_FORCE_FEATURES, e.g. "i8mm"): a forced artifact must not cache-hit a normal run
    let forced_arm = g_env_jit.jit_arm64_force_features
    if (!empty(forced_arm)) {
        h = (h ^ hash(forced_arm)) * JIT_FNV_PRIME
    }
    // aarch64 i8mm is an emission input (the +i8mm append): a warm pre-i8mm cache must not serve an i8mm host
    h = (h ^ (cpu_supports("i8mm") ? 64ul : 0ul)) * JIT_FNV_PRIME
    // a baseline class (DAS_JIT_BASELINE) picks the machine, the gates and the tune ladder - its artifact never serves a host run
    let baseline = jit_baseline_class()
    if (!empty(baseline)) {
        h = (h ^ hash(baseline)) * JIT_FNV_PRIME
    }
    // Cross-compile artifacts key on their triple — a --jit-target DLL must not cache-hit a host run (foreign-arch dlopen)
    if (!empty(target_triple)) {
        h = (h ^ hash(target_triple)) * JIT_FNV_PRIME
    }
    return h
}

def public jit_dll_basename(funcs : array<FunctionPtr>; opt_level : int; size_level : int;
                            emit_prologue : bool; debug_info : bool; target_triple : string; split : bool;
                            use_host_cpu : bool; sanitize : JitSanitize) : string {
    var h = LLVM_JIT_CODEGEN_VERSION
    for (fn in funcs) {
        h = (h ^ get_function_aot_hash(fn)) * JIT_FNV_PRIME
    }
    h = fold_loop_hints(funcs, h)   // JIT-only loop tune hints (see LoopHintHashVisitor)
    h = fold_llvm_code_hints(funcs, h)   // [llvm_code] generator params (annotation args)
    h = fold_function_hints(funcs, h)   // [hint(...)] function attributes (backend-only, aot-hash-blind)
    h = (h ^ jit_env_salt(opt_level, size_level, emit_prologue, debug_info, target_triple, split, use_host_cpu, sanitize)) * JIT_FNV_PRIME
    return "{h}"
}

def public jit_gc_stale_dlls(subdir : string; keep_basename : string; keep_names : table<string>; mine : table<string>) {
    // keep mask: the current DLL-hash prefix (dll/lib/exp/pdb/rsp + monolith .o) OR an exact keep_names entry (split obj cache's live partition objects - named by partition key, not dll hash)
    let keep_prefix = "{keep_basename}."
    dir(subdir) $(fname) {
        if (fname == "." || fname == ".." || fname |> starts_with(keep_prefix)
            || keep_names |> key_exists(fname) || !(mine |> key_exists(fname))) return
        let full_path = "{subdir}/{fname}"
        var err : string
        let ok = remove(full_path, err)
        if (ok && empty(err)) {
            to_log(LOG_INFO, "LLVM JIT: GC stale artifact {full_path}\n")
        } else {
            to_log(LOG_WARNING, "LLVM JIT: failed to GC stale artifact {full_path}: {err}\n")
        }
    }
}

struct public JitState {
    dll : void?
    ee : void?
    ctx : void?
}

// always-on summary (a silent multi-second jit is indistinguishable from a hang) + the
// log_compile_time per-phase breakdown; both artifact paths report through here
def public jit_log_report(funcs : array<FunctionPtr>; recompiled : bool; opt_level : int; tier_known : bool;
                          split : bool; split_parts : int; debug_info : bool; log_time : bool; totalTime : int64;
                          t_hash, t_init, t_declare, t_probe, t_irgen, t_opt, t_emit, t_install, t_finalize : int) {
    var tuneStamped = 0
    for (fn in funcs) {
        for (ann in fn.annotations) {
            if (ann.annotation.name == "llvm_code" && find_arg(ann.arguments, "tune_suffix") is tString) {
                tuneStamped++
                break
            }
        }
    }
    let tuneTail = tuneStamped > 0 ? " incl {tuneStamped} tune-stamped" : ""
    let tierTail = tier_known ? ", O{opt_level}" : ""
    // always announce split - a split-built DLL's profile is not comparable to a monolith build
    let splitTail = !split ? "" : (split_parts > 0 ? ", split {split_parts} modules" : ", split")
    let debugTail = debug_info ? ", --jit-debug" : ""
    let totalDt = double(get_time_usec(totalTime)) / 1000000.0lf
    to_log(LOG_INFO, "LLVM JIT: {length(funcs)} functions{tuneTail} in {totalDt} sec ({recompiled ? "codegen" : "cache"}{tierTail}{splitTail}{debugTail})\n")
    if (log_time) {
        to_log(LOG_INFO, "LLVM JIT time: hash {jit_sec(t_hash)} init {jit_sec(t_init)} declare {jit_sec(t_declare)} probe {jit_sec(t_probe)} irgen {jit_sec(t_irgen)} optimize {jit_sec(t_opt)} emit+link {jit_sec(t_emit)} install {jit_sec(t_install)} finalize {jit_sec(t_finalize)}\n")
    }
}

// Check if a function is a late-bound dasbind function (null builtin address, resolvable at runtime)
def public is_dasbind_late(func : Function?) : bool {
    if (func == null || !func.flags.builtIn || func._module.name != "dasbind") return false
    var hit = false
    peek(func.name) $(s) {
        hit = starts_with(s, "__dasbind__")
    }
    return hit
}

def public handle_each_is_jitable(src : ExpressionPtr) : bool {
    assume srcT = src._type
    if (srcT == null || !srcT.isHandle || srcT.annotation == null) {
        return false
    }
    return get_jit_each(srcT.annotation) != null
}

//! The jit passes a handled index in its own C ABI, so the index has to be a primitive the
//! emitter can put in a register. Anything else (a typed id, a struct key) gets no address and
//! keeps the interpreter, whatever the annotation offers. Returns -1 when not passable.
def public jit_index_base_type(ixT : TypeDeclPtr) : int {
    if (ixT == null || ixT.isRef) {
        return -1
    }
    let bt = ixT.baseType
    if (bt == Type.tInt || bt == Type.tUInt || bt == Type.tInt64 || bt == Type.tUInt64 ||
        bt == Type.tInt8 || bt == Type.tUInt8 || bt == Type.tInt16 || bt == Type.tUInt16 ||
        bt == Type.tFloat || bt == Type.tString) {
        return int(bt)
    }
    return -1
}

def public handle_at_is_jitable(expr : ExprAt?) : bool {
    assume subExprT = expr.subexpr._type
    if (!subExprT.isHandle || subExprT.annotation == null) {
        return false
    }
    let ibt = jit_index_base_type(expr.index._type)
    if (ibt < 0) {
        return false
    }
    return get_jit_at(subExprT.annotation, ibt) != null
}

def public loop_source_is_jitable(src : ExpressionPtr) : bool {
    assume srcT = src._type
    if (srcT == null) {
        return false
    }
    return (srcT.isRange || srcT.baseType == Type.tFixedArray || srcT.isGoodArrayType
        || srcT.isIterator || srcT.isString || handle_each_is_jitable(src))
}

//! the interpreter's address of `func` in `ctx`; null when the context holds no such function
def public interpreter_function_addr(var ctx : Context?; func : Function?) : void? {
    let MNH = hash(get_mangled_name(func))
    let MNH_ADDR = get_function_address(MNH, *ctx)
    return MNH_ADDR == 0ul ? null : unsafe(reinterpret<void?>(MNH_ADDR))
}

def public get_dll_missing(fns : array<FunctionPtr>; disabled : array<FunctionPtr>; var dll : DLLHandle?; var uids : UidNodes?) {
    verify(!(is_in_completion() || is_compiling_macros() || is_folding()))

    var missing = array<Function?>()
    missing |> reserve(length(fns))
    var disabled_exists = array<Function?>()
    if (dll.handle == null) {
        for (fn in fns) {
            missing |> push(unsafe(reinterpret<Function?>(fn)))
        }
    } else {
        for (fn in fns) {
            let dll_name = uids.get_dll_fn_name(fn)
            var written_hash = dll.get_function_hash(dll_name)
            if (written_hash == null || get_function_aot_hash(fn) != *written_hash) {
                missing |> push(unsafe(reinterpret<Function?>(fn)))
            }
        }
        for (fn in disabled) {
            let dll_name = uids.get_dll_fn_name(fn)
            var written_hash = dll.get_function_hash(dll_name)
            if (written_hash != null) {
                disabled_exists |> push(unsafe(reinterpret<Function?>(fn)))
            }
        }
    }
    return <- (missing, disabled_exists)
}

//! the candidate set against a DLL: a candidate the DLL holds under its own hash is bound; one
//! the DLL lacks was dropped by the emitter's content gate and stays interpreted; a candidate
//! held under another hash, or a `[no_jit]` function the DLL holds, is a miss
def public probe_dll(var candidates : array<FunctionPtr>; disabled : array<FunctionPtr>; var dll : DLLHandle?; var uids : UidNodes?) {
    verify(!(is_in_completion() || is_compiling_macros() || is_folding()))
    var bound : array<FunctionPtr>
    var mismatched = 0
    if (dll.handle != null) {
        for (fn in candidates) {
            var written_hash = dll.get_function_hash(uids.get_dll_fn_name(fn))
            if (written_hash == null) {
                continue
            }
            if (get_function_aot_hash(fn) != *written_hash) {
                mismatched++
            } else {
                bound |> push(fn)
            }
        }
        for (fn in disabled) {
            if (dll.get_function_hash(uids.get_dll_fn_name(fn)) != null) {
                mismatched++
            }
        }
    } else {
        mismatched = length(candidates)
    }
    return <- (bound <- bound, mismatched = mismatched)
}

//! the program's jit selection becomes `selected` and nothing else - the emitter and the extern
//! resolver read it to tell a call into the DLL from a call through a slot
def public mark_jit_selection(prog : Program?; var selected : array<FunctionPtr>) {
    clear_jit_selection(prog)
    for (fn in selected) {
        set_jit_selected(prog, fn, true)
    }
}

//! Fills the DLL's extern slots for the functions it holds - interop addresses of the builtins
//! called, interpreter addresses of the callees the DLL lacks, the handled-type and table helpers.
//! A slot the DLL lacks was lowered inline and is skipped; one nothing can fill lands in `errors`.
class public DllExternResolver : AstVisitor {
    @do_not_delete prog : Program?
    @do_not_delete dll : DLLHandle?
    @do_not_delete jit_context : Context?
    @do_not_delete thisFunc : Function?
    @do_not_delete thisBlock : array<ExprBlock?>
    @do_not_delete uid : UidNodes?
    errors : array<string>

    def override canVisitWithAliasSubexpression(expr : ExprAssume?) : bool {
        return false
    }

    def override preVisitFunction(var fun : FunctionPtr) : void {
        uid.reset(fun)
        thisFunc = fun
    }

    def override visitFunction(fun : FunctionPtr) : FunctionPtr {
        thisFunc = null
        uid.reset(null)
        return fun
    }

    def DllExternResolver(_prog : Program?; var _dll : DLLHandle?; ctx : Context?; uids : UidNodes?) {
        prog = _prog
        dll = _dll
        jit_context = ctx
        uid = uids
    }

    def private fail(msg : string) {
        errors |> push(msg)
    }

    //! `what` names the slot only on failure - the message is a `describe` the success path never pays for
    def private set_slot(name : DllName; ptr : void?; what : block<() : string>) {
        if (dll.set_glob_address(name, ptr) && ptr == null) {
            fail("missing {invoke(what)}")
        }
    }

    def make_call(expr : ExprCallFunc?) {
        if (expr.func == null) return
        let slot = uid.get_dll_fn_name_ptr(expr.func)
        if (expr.func.flags.builtIn) {
            let FUNC_PTR = get_builtin_function_address(expr.func)
            if (dll.set_glob_address(slot, FUNC_PTR) && FUNC_PTR == null && !is_dasbind_late(expr.func)) {
                fail("missing interop function pointer for {expr.func._module.name}::{expr.func.name} // {get_mangled_name(expr.func)} at {describe(expr.at)}")
            }
        } elif (!is_jit_selected(prog, expr.func)) {
            set_slot(slot, interpreter_function_addr(jit_context, expr.func)) {
                return "function pointer for {expr.func.name} at {describe(expr.at)}"
            }
        }
    }

    def override preVisitExprNew(expr : ExprNew?) {
        if (expr.typeexpr.isHandle) {
            set_slot(uid.get_new(expr.typeexpr), get_jit_new(expr.typeexpr.annotation)) {
                return "new`handle function pointer for {describe(expr.typeexpr)}"
            }
        } elif (expr.typeexpr.baseType == Type.tFixedArray) {
            // the handle gate lives on the chain element (the head is tFixedArray);
            // borrow-walk — this pass runs outside ast_gc_guard, clones would leak
            var elemT = expr.typeexpr
            while (elemT.baseType == Type.tFixedArray && elemT.firstType != null) {
                elemT = elemT.firstType
            }
            if (elemT.baseType == Type.tHandle) {
                set_slot(uid.get_new(expr.typeexpr), get_jit_new(elemT.annotation)) {
                    return "new`handle function pointer for {describe(expr.typeexpr)}"
                }
            } elif (expr.initializer) {
                make_call(expr)
            }
        } elif (expr.initializer) {
            make_call(expr)
        }
    }

    def override preVisitExprCall(expr : ExprCall?) {
        make_call(expr)
    }

    def override preVisitExprOp2(var expr : ExprOp2?) {
        if (expr.func != null) {
            make_call(expr)
        }
    }

    def override preVisitExprOp1(var expr : ExprOp1?) {
        if (expr.func != null) {
            make_call(expr)
        }
    }

    def override preVisitExprDelete(var expr : ExprDelete?) {
        assume subT = expr.subexpr._type
        if (subT.isPointer && subT.firstType.isHandle) {
            assume firstT = subT.firstType
            set_slot(uid.get_delete_type(firstT), get_jit_delete(firstT.annotation)) {
                return "delete`handle function pointer for {describe(firstT)}"
            }
        }
    }

    def override preVisitExprClone(expr : ExprClone?) : void {
        assume leftT = expr.left._type
        if (leftT.isHandle) {
            set_slot(uid.get_clone_type(leftT), get_jit_clone(leftT.annotation)) {
                return "clone`handle function pointer for {describe(leftT)}"
            }
        }
    }

    def override preVisitExprAddr(expr : ExprAddr?) {
        set_slot(uid.get_addr(expr), interpreter_function_addr(jit_context, expr.func)) {
            return "function pointer for {expr.func.name}"
        }
    }

    def override preVisitExprMakeStructIndex(expr : ExprMakeStruct?; index : int; last : bool) : void {
        if (expr.constructor != null && !is_jit_selected(prog, expr.constructor)) {
            set_slot(uid.get_ctor(expr), interpreter_function_addr(jit_context, expr.constructor)) {
                return "function pointer for {expr.constructor.name}"
            }
        }
    }

    def override preVisitExprAscend(expr : ExprAscend?) : void {
        if (expr.subexpr._type.isHandle) {
            assume firstT = expr.subexpr._type
            set_slot(uid.get_ascend_new(expr), get_jit_new(firstT.annotation)) {
                return "new`handle function pointer for {describe(firstT)}"
            }
        }
    }

    def override preVisitExprMakeBlock(expr : ExprMakeBlock?) : void {
        thisBlock |> push <| unsafe(reinterpret<ExprBlock?>(expr._block))
    }

    def override visitExprMakeBlock(expr : ExprMakeBlock?) : ExpressionPtr {
        thisBlock |> pop()
        return expr
    }

    // Pre-bind the JIT table-fn globals for the standalone-exe / cache-reload path. String
    // keys route through the precomputed-hash helpers (see build_table_find/at), so bind
    // those globals instead of the per-type ones.
    def bind_jit_table_find_glob(t : Type) {
        // String find is fully inline (no glob). Non-string binds the per-type C++ find glob — still
        // used by non-workhorse keys (vectors / structs) that route through build_table_find_call.
        if (t != Type.tString) {
            dll.set_glob_address(DllName(FN_JIT_TABLE_FIND(t)), get_jit_table_find(unsafe(reinterpret<int>(t))))
        }
    }
    def bind_jit_table_at_glob(t : Type) {
        if (t == Type.tString) {
            dll.set_glob_address(DllName(FN_JIT_STRING_TABLE_AT_WITH_HASH()), get_jit_string_table_at_with_hash())
            dll.set_glob_address(DllName(FN_JIT_STRING_TABLE_AT_AFTER_PACKED_MISS()), get_jit_string_table_at_after_packed_miss())
        } else {
            // Non-string keys are open-addressed: the inline probe handles find/at hits and routes a
            // miss to the regular C++ at (FN_JIT_TABLE_AT) — no packed after-miss helper needed.
            dll.set_glob_address(DllName(FN_JIT_TABLE_AT(t)), get_jit_table_at(unsafe(reinterpret<int>(t))))
        }
    }

    def override preVisitExprAt(expr : ExprAt?) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType) {
            bind_jit_table_at_glob(table_key_storage_base_type(subExprT.firstType))
        } elif (handle_at_is_jitable(expr)) {
            assume ixT = expr.index._type
            dll.set_glob_address(uid.get_at_type(subExprT, ixT),
                get_jit_at(subExprT.annotation, jit_index_base_type(ixT)))
        }
    }

    def override preVisitExprForSource(var expr : ExprFor?, var source : ExpressionPtr, last : bool) : void {
        if (handle_each_is_jitable(source)) {
            dll.set_glob_address(uid.get_each_type(source._type), get_jit_each(source._type.annotation))
        }
    }

    def override preVisitExprSetInsert(expr : ExprSetInsert?) {
        bind_jit_table_at_glob(table_key_storage_base_type(expr.arguments[0]._type.firstType))
    }

    def override preVisitExprKeyExists(expr : ExprKeyExists?) {
        bind_jit_table_find_glob(table_key_storage_base_type(expr.arguments[0]._type.firstType))
    }

    def override preVisitExprFind(expr : ExprFind?) {
        bind_jit_table_find_glob(table_key_storage_base_type(expr.arguments[0]._type.firstType))
    }

    def override preVisitExprSafeAt(expr : ExprSafeAt?) {
        assume subExprT = expr.subexpr._type
        if (subExprT.isGoodTableType || (subExprT.isPointer && subExprT.firstType.isGoodTableType)) {
            var etype = subExprT.isPointer ? subExprT.firstType : subExprT
            bind_jit_table_find_glob(table_key_storage_base_type(etype.firstType))
        }
    }

    def override preVisitExprErase(expr : ExprErase?) {
        let t = table_key_storage_base_type(expr.arguments[0]._type.firstType)
        dll.set_glob_address(DllName(FN_JIT_TABLE_ERASE(t)), get_jit_table_erase(unsafe(reinterpret<int>(t))))
    }
}

//! fills the DLL's extern slots for `funcs`; the errors, empty on success
def public resolve_dll_externs(prog : Program?; var dll : DLLHandle?; var ctx : Context?; var uids : UidNodes?; funcs : array<FunctionPtr>) : array<string> {
    var resolver = new DllExternResolver(prog, dll, ctx, uids)
    make_visitor(*resolver) $(adapter) {
        for (fn in funcs) {
            visit(fn, adapter)
        }
    }
    var errors <- resolver.errors
    unsafe {
        delete resolver
    }
    return <- errors
}

//! points every function's sim node at its code in the DLL
def public install_dll_functions(var dll : DLLHandle?; var ctx : Context?; var uids : UidNodes?; funcs : array<FunctionPtr>) {
    for (fun in funcs) {
        let simfun = get_function_by_mangled_name_hash(hash(get_mangled_name(fun)), *ctx)
        var gen = dll.get_function_address(uids.get_dll_fn_name(fun))
        if (gen != null) {
            gen |> unsafe(instrument_jit(simfun, fun.at, *ctx))
        } else {
            unsafe(remove_jit(simfun))
        }
    }
}

//! Everything `run_jit` decides before it touches LLVM: the resolved options, the candidate set
//! (every used function the policy selects, `[no_jit]` aside) and the content-addressed DLL name.
//! Its inputs are the program and the command line, so the link module and the emitter compute the same plan.
struct public JitPlan {
    aot_host : bool
    jit_all_functions : bool
    debug_info : bool
    dump_ir : bool
    dump_ir_path : string
    gen_exe : bool
    gen_lib : bool
    lib_static : bool
    lib_export_marked : bool
    lib_bindings : string
    exe_main : string
    compile_only : bool
    emit_aot_object : bool
    use_dll : bool
    split_requested : bool
    split_modules : int
    jit_split : bool
    obj_cache : bool
    jit_lto : bool
    opt_level : int
    size_level : int
    emit_prologue : bool
    exe_strict : bool
    config_out_path : string
    output_path : string
    path_to_shared_lib : string
    path_to_linker : string
    target_triple : string
    gen_wasm : bool
    runtime_lib_override : string
    emit_object : bool
    wasm_threads : bool
    check_abi : bool
    linker_string : string
    register_all_modules : bool
    baseline : string
    exe_host_cpu : bool
    use_host_cpu : bool
    log_jit_time : bool
    sanitize : JitSanitize
    dll_hash_basename : string
    candidates : array<FunctionPtr>
    disabled : array<FunctionPtr>
}

def public report_collect_errors(var plan : JitPlan) : bool {
    //! Logs every function the JIT would compile that reaches a heap collection without proof; true when the program must not be jitted.
    if (plan.gen_exe || plan.gen_lib) return false
    var failed = false
    for (fun in plan.candidates) {
        let why = collect_carrier_violation(fun)
        if (!empty(why)) {
            to_log(LOG_ERROR, "LLVM JIT: {describe(fun.at)}: function '{fun.name}' can reach heap_collect, and {why}; mark it [no_jit] to keep it interpreted\n")
            failed = true
        }
    }
    return failed
}

def private jit_selects(fun : FunctionPtr; rqj : bool; aot_host : bool; jit_all : bool; var ctx : Context?) : bool {
    if (rqj) return true
    if (!aot_host) return jit_all
    return !is_aot_function(get_function_by_mangled_name_hash(hash(get_mangled_name(fun)), *ctx))
}

[arch(at="../ARCHITECTURE_EXE.md#split-exe-lto")]
def private resolve_split_options(var plan : JitPlan; cli_opts : JitCliOptions; prog : Program?; announce : bool) {
    let split_option = prog._options |> find_arg("jit_split_modules")
    plan.split_requested = (cli_opts.split_modules |> is_some()) || !(split_option is nothing)
    plan.split_modules = cli_opts.split_modules |> unwrap_or(split_option ?as tInt ?? -1)
    //! an exe splits on its build command alone (the script pin serves the DLL path, where it is inert for an exe as before), and for the host only: the slots' comdats are the host's object format
    let exe_split = plan.gen_exe && !plan.gen_lib && !plan.gen_wasm && (cli_opts.split_modules |> is_some()) && empty(plan.target_triple)
    plan.jit_split = plan.split_modules != 0 && (plan.use_dll || exe_split)
    if (announce && (cli_opts.split_modules |> is_some()) && plan.split_modules != 0 && !plan.jit_split) {
        to_log(LOG_WARNING, "LLVM JIT: --jit-split-modules applies to the JIT DLL path and to a host-target -exe only - ignored for this artifact mode\n")
    }
    // --jit-obj-cache (on by default under the DLL split; a pinned jit_output_path disables it, no GC runs there)
    let obj_cache_default = (prog._options |> find_arg("jit_obj_cache")) ?as tBool ?? true
    plan.obj_cache = plan.jit_split && plan.use_dll && (string(prog.policies.jit_output_path) |> empty()) && (cli_opts.obj_cache |> unwrap_or(obj_cache_default ? 1 : 0)) != 0
    if (announce && (cli_opts.obj_cache |> is_some()) && !(plan.jit_split && plan.use_dll)) {
        to_log(LOG_WARNING, "LLVM JIT: --jit-obj-cache applies under --jit-split-modules dll mode only - ignored for this run\n")
    }
    let lto_requested = cli_opts.lto |> unwrap_or(false)
    plan.jit_lto = lto_requested && plan.jit_split && exe_split
    if (announce && lto_requested && !plan.jit_lto) {
        to_log(LOG_WARNING, "LLVM JIT: --jit-lto applies to a split -exe for the host only (-exe with --jit-split-modules, no --jit-target) - ignored for this run\n")
    }
}

//! an exe or a library links the runtime libraries, which a sanitizer host built under its sanitizer and which call its
//! runtime, beside the instrumentation the run asked for; the sanitizer runtimes are the compiler's own, so the link runs
//! through clang++ unless a linker is named. A static library is archived, not linked, and its host links the runtimes
def private link_with_sanitizers(var plan : JitPlan; host_ubsan : bool) {
    return if (!plan.gen_exe || plan.lib_static || plan.gen_wasm || !empty(plan.target_triple) || get_platform_name() == "windows")
    let wanted = fixed_array(plan.sanitize.address || JIT_HOST_ASAN, plan.sanitize.undefined || host_ubsan, host_has_symbol("__tsan_init"))
    let names <- [for (name, on in fixed_array("address", "undefined", "thread"), wanted); name; where on]
    return if (empty(names))
    plan.linker_string = "-fsanitize={join(names, ",")} {plan.linker_string}"
    if (empty(plan.path_to_linker)) {
        plan.path_to_linker = "clang++"
    }
}

def private resolve_sanitize_option(var plan : JitPlan; cli_opts : JitCliOptions; prog : Program?; announce : bool) {
    assume san = plan.sanitize
    let host_ubsan = host_has_symbol(JIT_UBSAN_RUNTIME_SYMBOL)
    let requested = cli_opts.sanitize |> unwrap_or((prog._options |> find_arg("jit_sanitize")) ?as tString ?? "")
    //! the in-memory engine resolves a static constructor by symbol, and asan's module constructor is internal
    let in_memory = !plan.use_dll && !plan.gen_exe && !plan.compile_only && !plan.emit_aot_object
    //! a sanitizer host instruments its jitted code with its own sanitizer, unless the run names a list - `none` names nothing
    let by_default = empty(requested)
    san.address = by_default && JIT_HOST_ASAN && !in_memory
    san.undefined = by_default && host_ubsan
    for (name in split(requested, ",")) {
        if (name == "address") {
            san.address = true
        } elif (name == "undefined") {
            san.undefined = true
        } elif (!empty(name) && name != "none") {
            panic("LLVM JIT: --jit-sanitize={name} names no sanitizer - address, undefined or none")
        }
    }
    if (san.address && in_memory) {
        panic("LLVM JIT: --jit-sanitize=address runs on the JIT DLL path only - the in-memory engine (-jit-no-cache, a static daslang) cannot run asan's module constructor")
    }
    //! the artifact loads into this process and calls its sanitizer runtime
    let loads_here = !plan.gen_exe && !plan.compile_only
    if (loads_here && ((san.address && !JIT_HOST_ASAN) || (san.undefined && !host_ubsan))) {
        panic("LLVM JIT: --jit-sanitize={requested} needs the sanitizer runtime in this process - build daslang with -DDAS_USE_SANITIZER, or pass --jit-compile-only")
    }
    if (announce && (san.address || san.undefined)) {
        let names = san.address && san.undefined ? "address,undefined" : (san.address ? "address" : "undefined")
        to_log(LOG_INFO, "LLVM JIT: --jit-sanitize={names}{by_default ? " (the host's sanitizer; --jit-sanitize=none turns it off)" : ""}\n")
    }
    link_with_sanitizers(plan, host_ubsan)
}

//! the plan for `prog`; `announce` prints the one-time lines (the baseline class, the exe host
//! CPU) - the first computation in a run passes true, a recomputation false
def public make_jit_plan(prog : Program?; var ctx : Context?; announce : bool) : JitPlan {
    let r <- parse_args(type<JitCliOptions>)
    return <- make_jit_plan(prog, ctx, announce, r |> unwrap_or_default)
}

def public make_jit_plan(prog : Program?; var ctx : Context?; announce : bool; cli_opts : JitCliOptions) : JitPlan {   // nolint:STYLE038 - one flat resolution of every emission input; a split would hide which inputs key the cache
    var plan = JitPlan(aot_host = prog.policies.aot && prog.policies.jit_enabled,
        jit_all_functions = prog.policies.jit_jit_all_functions,
        debug_info = cli_opts.debug_info |> unwrap_or(prog.policies.jit_debug_info),
        dump_ir = cli_opts.dump_ir |> unwrap_or(false),
        dump_ir_path = cli_opts.dump_ir_path |> unwrap_or(""))
    if (announce && prog.policies.jit_dll_mode && !das_is_dll_build()) {
        // a legitimate configuration (static daslang), but the cost is worth naming: no DLL
        // cache exists, so every -jit run pays full in-memory codegen
        to_log(LOG_WARNING, "LLVM JIT: dll mode requested but this daslang build is static - no DLL cache, " +
            "every run pays full in-memory codegen\n")
    }
    plan.gen_lib = cli_opts.lib |> unwrap_or((prog._options |> find_arg("jit_lib")) ?as tBool ?? false)
    plan.gen_exe = prog.policies.jit_exe_mode || plan.gen_lib
    plan.lib_static = cli_opts.lib_static |> unwrap_or(false)
    plan.lib_bindings = cli_opts.lib_bindings |> unwrap_or("")
    plan.lib_export_marked = cli_opts.lib_export_marked |> unwrap_or((prog._options |> find_arg("jit_lib_export_marked")) ?as tBool ?? false)
    if (announce && !plan.gen_lib) {
        if (cli_opts.lib_static |> is_some()) {
            to_log(LOG_WARNING, "LLVM JIT: --jit-lib-static applies to -lib only - ignored for this run\n")
        }
        if (cli_opts.lib_bindings |> is_some()) {
            to_log(LOG_WARNING, "LLVM JIT: --jit-lib-bindings applies to -lib only - ignored for this run\n")
        }
    }
    plan.exe_main = "main"
    // --jit-compile-only: build + verify + optimize (+dump), write/load/install nothing —
    // the program runs interpreted. Also bypasses the DLL cache probe, so the
    // generators always run (this is the cross-emission smoke rail).
    plan.compile_only = cli_opts.compile_only |> unwrap_or((prog._options |> find_arg("jit_compile_only")) ?as tBool ?? false)
    // Offline AOT: emit the whole used function set to a .o for static linking, with a
    // load ctor registering them into the AOT library. Mutually exclusive with dll
    // mode (no in-memory engine work, no instrument).
    plan.emit_aot_object = prog.policies.jit_emit_object
    plan.use_dll = prog.policies.jit_dll_mode && das_is_dll_build() && !plan.gen_exe && !plan.compile_only && !plan.emit_aot_object
    plan.opt_level = cli_opts.opt_level |> unwrap_or(prog.policies.jit_opt_level)
    plan.size_level = cli_opts.size_level |> unwrap_or(prog.policies.jit_size_level)
    // This policy must be set before compilation so stack-frame sizes include the prologue.
    plan.emit_prologue = prog.policies.emit_prologue
    // strict: a standalone exe with a no_jit function fails the build (collect_standalone_functions)
    plan.exe_strict = true
    plan.config_out_path = string(prog.policies.jit_output_path)
    plan.output_path = plan.config_out_path |> empty() ? ".jitted_scripts/{prog.thisNamespace}" : plan.config_out_path
    plan.path_to_shared_lib = string(prog.policies.jit_path_to_shared_lib)
    // CLI --jit-path-to-linker wins over the script-level policy.
    plan.path_to_linker = cli_opts.path_to_linker |> unwrap_or(string(prog.policies.jit_path_to_linker))
    // Cross-compile triple: --jit-target=<triple> CLI wins over the
    // script-level `options jit_target = "..."`.
    let opt_target = (prog._options |> find_arg("jit_target")) ?as tString ?? ""
    plan.target_triple = cli_opts.target |> unwrap_or(opt_target)
    // the compile-time target folds (get_target_triple, get_target_architecture_name) read the
    // command line, never this option: a script-level triple the command line does not carry
    // would emit for one target with every tier gate folded for the host
    if (!empty(plan.target_triple) && get_target_triple() != plan.target_triple) {
        panic("LLVM JIT: the target triple `{plan.target_triple}` is not on the command line - pass --jit-target={plan.target_triple} after the `--` separator (the compile-time target folds read argv, so `options jit_target` alone folds the host's tiers into a cross artifact)")
    }
    plan.gen_wasm = plan.gen_exe && (plan.target_triple |> starts_with("wasm"))
    // Optional CLI override for the wasm runtime archive location. Falls
    // back to `options jit_runtime_lib = "..."` then write_wasm's
    // find_runtime_lib() auto-locate under web/output/lib/.
    plan.runtime_lib_override = cli_opts.runtime_lib |> unwrap_or((prog._options |> find_arg("jit_runtime_lib")) ?as tString ?? "")
    // --jit-emit-object: emit the wasm object and skip the emcc link (the caller
    // links it, e.g. daspkg release wasm). Sidesteps the link entirely, including
    // the Windows popen-quoting failure path in link_wasm.
    plan.emit_object = cli_opts.emit_object |> unwrap_or(false)
    // --jit-threads: emit a -pthread-compatible wasm object (+atomics,+bulk-memory).
    plan.wasm_threads = cli_opts.threads |> unwrap_or(false)
    // --jit-check-abi: emit the handled-type host-vs-target layout check (a stderr warning on
    // drift) into initialize_modules().
    plan.check_abi = cli_opts.check_abi |> unwrap_or(false)
    // Extra args appended verbatim to the host linker cmd. CLI > script option.
    plan.linker_string = cli_opts.linker_string |> unwrap_or((prog._options |> find_arg("jit_linker_string")) ?as tString ?? "")
    plan.register_all_modules = cli_opts.register_all_modules |> unwrap_or(false)
    resolve_split_options(plan, cli_opts, prog, announce)
    if (plan.jit_lto && empty(plan.path_to_linker) && get_platform_name() != "windows") {
        plan.path_to_linker = "clang++"   //! a POSIX LTO link needs a clang driver over lld or a plugin-capable ld; `c++` on a GNU box drives neither
        if (announce) {
            to_log(LOG_INFO, "LLVM JIT: --jit-lto links through clang++ (--jit-path-to-linker names another driver)\n")
        }
    }

    // Set global options
    LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = prog._options |> find_arg("jit_vec3_ldu") ?as tBool ?? LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT
    // `options fast_math` (or policies.fast_math) is the one knob; `_jit_fast_math` kept as a legacy alias
    // a runtime built -ffast-math already relaxes the interpreter's own ops, so the JIT inherits
    // that unless the program says otherwise; `options fast_math` still wins either way
    g_jit_fast_math = (prog._options |> find_arg("fast_math") ?as tBool ?? (prog.policies.fast_math || HOST_FAST_MATH)) || (prog._options |> find_arg("_jit_fast_math") ?as tBool ?? false)
    resolve_sanitize_option(plan, cli_opts, prog, announce)

    // nolint:STYLE014
    // Standalone exes target a GENERIC CPU (write_exe use_host_cpu=false default), so
    // host cpuid features must not leak into their emission (host_features=false) —
    // EXCEPT a native exe carrying [llvm_code] kernels: a local-use artifact by
    // definition, it targets the current box in BOTH the flags and the machine (the ONE
    // truth, decided here — flags and machine must never diverge: host-F16C flags on a
    // generic machine lower fp16 through libcalls msvcrt never links, and generic flags
    // on a host machine decline every generated kernel to its reference body).
    // A baseline class (DAS_JIT_BASELINE) replaces the box on every rail - exe, DLL and AOT object
    // alike: the machine, the gates and the tune ladder all read the class, so the artifact runs
    // on every box of that class. It never crosses architectures; that is --jit-target's job.
    plan.baseline = jit_baseline_class()
    if (!empty(plan.baseline)) {
        let target_arch = (empty(plan.target_triple) ? get_architecture_name()
            : ((starts_with(plan.target_triple, "aarch64") || starts_with(plan.target_triple, "arm64")) ? "arm64"
            : ((starts_with(plan.target_triple, "x86_64") || starts_with(plan.target_triple, "amd64")) ? "x86_64" : plan.target_triple)))
        let berr = jit_baseline_error(target_arch)
        if (!empty(berr)) {
            panic("LLVM JIT: {berr}")
        }
        if (announce) {
            let row = jit_cpu_class_row(plan.baseline)
            to_log(LOG_INFO, "LLVM JIT: DAS_JIT_BASELINE={plan.baseline} - targeting the {plan.baseline} class (cpu {row.cpu}, features {row.features}) instead of this box\n")
        }
    }
    plan.exe_host_cpu = plan.gen_exe && empty(plan.target_triple) && empty(plan.baseline) && has_generated_kernel(prog, plan.jit_all_functions)
    if (announce && plan.exe_host_cpu) {
        to_log(LOG_INFO, "LLVM EXE: targeting the current box (CPU + features) - the build carries [llvm_code] kernels\n")
    }
    // AOT objects target the host CPU: the .o is statically linked into a binary that runs on this
    // box (same contract as the JIT), so bake host features — this force-appends +dotprod on aarch64,
    // without which the dasLLAMA Q8 SDOT kernels have no instruction to select and codegen aborts.
    plan.use_host_cpu = empty(plan.baseline) && (plan.emit_aot_object ? true : (!plan.gen_exe || plan.exe_host_cpu))
    plan.log_jit_time = prog._options |> find_arg("log_compile_time") ?as tBool ?? prog.policies.log_compile_time

    prog |> for_each_module() $(mod) {
        mod |> for_each_function("") $(fun) {
            // In object mode emit the whole used, non-noAot set (all modules +
            // per-program generic instantiations), mirroring C++ AOT whole-program
            // coverage; noAot functions interpret as Program::linkCppAot skips them.
            if (is_used(prog, fun) && (!plan.emit_aot_object || !fun.flags.noAot)
                    && get_function_by_mangled_name_hash(hash(get_mangled_name(fun)), *ctx) != null
                    && jit_selects(fun, fun.moreFlags.requestJit, plan.aot_host, plan.jit_all_functions, ctx)) {
                if (!fun.moreFlags.requestNoJit) {
                    plan.candidates |> emplace(fun)
                } else {
                    plan.disabled |> emplace(fun)
                }
            }
        }
    }
    // Content-address the DLL: distinct (AST + codegen version + opt flags) → distinct files, same
    // inputs → same filename → cache hit. Only in dll-mode with the default output path; a pinned
    // jit_output_path or an exe keeps the user's path.
    if (plan.use_dll && plan.config_out_path |> empty()) {
        plan.dll_hash_basename = jit_dll_basename(plan.candidates, plan.opt_level, plan.size_level, plan.emit_prologue, plan.debug_info, plan.target_triple, plan.jit_split, plan.use_host_cpu, plan.sanitize)
        plan.output_path = ".jitted_scripts/{prog.thisNamespace}/{plan.dll_hash_basename}"
    }
    return <- plan
}
