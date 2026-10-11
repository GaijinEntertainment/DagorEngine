options gen2
options indenting = 4
options strict_smart_pointers = false
options no_global_variables = false

module llvm_jit_code shared private

require llvm/bindings/llvm_struct
require llvm/daslib/llvm_jit_intrin
require daslib/ast_boost

//! JIT-side half of the `[llvm_code]` rail (the user-side annotation registers in
//! `daslib/tune.das`, forwarding to `llvm_code_apply` in `llvm/daslib/llvm_code.das`): a name-keyed registry of das generator functions that emit a
//! function's LLVM IR wholesale. Generator modules compile into the JIT's context via the
//! require list in `llvm/daslib/llvm_user_modules.das` and register themselves from `[init]`.

struct public LlvmCodeCtx {
    //! LLVM context / primitive types / builder — same handles the intrinsic emitters get
    jit : JitCtx
    //! the das function being generated: signature, argument types, annotations (read-only)
    fn : Function const?
    //! the LLVM function to fill; the ABI is already built — one typed parameter per das argument
    //! in order, then the context pointer, then the CMRES pointer for by-memory results. A generator
    //! must append and terminate its basic blocks, and must not touch `impl` if it declines.
    impl : LLVMOpaqueValue?
    impl_type : LLVMOpaqueType?
    //! the [llvm_code(...)] declaration; generator parameters live in `.arguments`
    //! (read with ast_boost `find_arg`)
    ann : AnnotationDeclaration const?
}

var private g_llvm_code_generators : table<string; function<(var gc : LlvmCodeCtx) : bool>>

//! Register a generator under `name` (convention: "module_name::function_name"). A generator
//! returns true when it emitted the function, false to decline — the JIT then compiles the
//! reference body normally. Report hard errors via llvm_jit_common `failed()` and return true.
def public register_llvm_code_generator(name : string; gen : function<(var gc : LlvmCodeCtx) : bool>) {
    if (g_llvm_code_generators |> key_exists(name)) {
        panic("register_llvm_code_generator: duplicate generator '{name}'")
    }
    g_llvm_code_generators |> insert(name, gen)
}

def public has_llvm_code_generator(name : string) : bool {
    return g_llvm_code_generators |> key_exists(name)
}

def public invoke_llvm_code_generator(name : string; var gc : LlvmCodeCtx) : bool {
    var generated = false
    g_llvm_code_generators |> get(name) $(gen) {
        generated = gen |> invoke(gc)
    }
    return generated
}

def public find_llvm_code_annotation(fn : FunctionPtr) : AnnotationDeclaration const? {
    for (ann in fn.annotations) {
        if (ann.annotation.name == "llvm_code") return ann
    }
    return null
}

def public llvm_code_generator_name(ann : AnnotationDeclaration const?) : string {
    for (arg in ann.arguments) {
        if (arg.name == "name" && arg.basicType == Type.tString) return string(arg.sValue)
    }
    return ""
}

//! The CPU class a fat-mode clone is stamped for (`tune_class=`), "" for every other function.
def public llvm_code_class_name(ann : AnnotationDeclaration const?) : string {
    for (arg in ann.arguments) {
        if (arg.name == "tune_class" && arg.basicType == Type.tString) return string(arg.sValue)
    }
    return ""
}
