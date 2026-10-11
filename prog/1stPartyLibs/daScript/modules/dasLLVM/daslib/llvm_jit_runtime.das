options gen2

options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options relaxed_pointer_const

module llvm_jit_runtime shared private

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dsl
require llvm/daslib/llvm_jit_common
require llvm/daslib/ir_dsl
require jit

//! The runtime entry points jitted code calls that are plain IR, the twins of src/builtin/jit_runtime.cpp:
//! each keeps its old C++ name and signature and is an internal function of every module that calls it.

//! the IR runtime helpers; each value's name is the helper's symbol
enum public JitRuntime {
    jit_str_cmp
    jit_array_lock
    jit_array_unlock
    jit_table_lock
    jit_table_unlock
    jit_prologue
    jit_epilogue
}

//! calls runtime helper `fn`, defining its body into g_mod on the module's first call
def public build_jit_runtime_call(b : LLVMBuilderRef; fn : JitRuntime; var args : LLVMOpaqueValue? []) : LLVMOpaqueValue? {
    var f = LLVMGetNamedFunction(g_mod, "{fn}")
    if (f == null) {
        f = define_jit_runtime_helper(fn)
    }
    return LLVMBuildCall2(b, LLVMGlobalGetValueType(f), f, args, "")
}

def private define_jit_runtime_helper(fn : JitRuntime) : LLVMOpaqueValue? {
    let name = "{fn}"
    if (fn == JitRuntime.jit_str_cmp) return define_str_cmp(name)
    if (fn == JitRuntime.jit_array_lock) return define_lock(name, "array")
    if (fn == JitRuntime.jit_table_lock) return define_lock(name, "table")
    if (fn == JitRuntime.jit_array_unlock) return define_unlock(name, "array")
    if (fn == JitRuntime.jit_table_unlock) return define_unlock(name, "table")
    if (fn == JitRuntime.jit_prologue) return define_prologue(name)
    return define_epilogue(name)
}

//! strcmp where a null string reads as ""
def private define_str_cmp(name : string) : LLVMOpaqueValue? {
    let strcmp_type = jit_fn_type($(sA, sB : void?) : int {})
    return ir_function(name, strcmp_type, -1, -1) {
        let a = select(is_null(arg(0)), text(""), arg(0))
        let b = select(is_null(arg(1)), text(""), arg(1))
        ret(ir(LLVMBuildCall2(ir_builder(), strcmp_type, declare_extern_fn("strcmp", strcmp_type), fixed_array(a.v, b.v), "")))
    }
}

//! a field of the lock header Array and Table share, on the helper's first argument
def private header(f : JIT_ARRAY) : Ir {
    return field(arg(0), jit_struct_type(type<tuple<data : void?; size : int64; capacity : int64; magic : int; lock : int; flags : int>>), int(f))
}

//! array_lock / table_lock (runtime_array.cpp, runtime_table.cpp)
def private define_lock(name, what : string) : LLVMOpaqueValue? {
    return ir_function(name, jit_fn_type($(arr, ctx, at : void?) : void {}), 1, 2) {
        return_if((load32(header(JIT_ARRAY.FLAGS)) & ir(ARRAY_LOCK_EXEMPT_FLAGS)) != ir(0u))
        let lock = load32(header(JIT_ARRAY.LOCK))
        let first = lock == ir(0u)
        throw_if(load32(header(JIT_ARRAY.MAGIC)) != select(first, ir(0u), ir(ARRAY_MAGIC)), select(first,
            text("{what} magic mismatch on first lock, was it moved or overwritten?"),
            text("{what} magic mismatch on lock, was it moved or overwritten?")))
        store(header(JIT_ARRAY.LOCK), lock + ir(1u))
        store(header(JIT_ARRAY.MAGIC), ir(ARRAY_MAGIC))
    }
}

//! array_unlock / table_unlock: the last unlock clears the magic
def private define_unlock(name, what : string) : LLVMOpaqueValue? {
    return ir_function(name, jit_fn_type($(arr, ctx, at : void?) : void {}), 1, 2) {
        return_if((load32(header(JIT_ARRAY.FLAGS)) & ir(ARRAY_LOCK_EXEMPT_FLAGS)) != ir(0u))
        throw_if(load32(header(JIT_ARRAY.MAGIC)) != ir(ARRAY_MAGIC), text("{what} magic mismatch on unlock, was it moved or overwritten?"))
        let lock = load32(header(JIT_ARRAY.LOCK))
        throw_if(lock == ir(0u), text("{what} lock underflow, was it moved or overwritten?"))
        let left = lock - ir(1u)
        store(header(JIT_ARRAY.LOCK), left)
        store(header(JIT_ARRAY.MAGIC), select(left == ir(0u), ir(0u), ir(ARRAY_MAGIC)))
    }
}

//! `__asan_poison_memory_region` / `__asan_unpoison_memory_region` over `size` bytes at `p` - the calls
//! StackAllocator's pop and push make on an asan host
def private asan_region(which : string; p, size : Ir) {
    let typ = jit_fn_type($(p : void?; size : uint64) : void {})
    LLVMBuildCall2(ir_builder(), typ, declare_extern_fn("__asan_{which}_memory_region", typ), fixed_array(p.v, size.v), "")
}

def private address(p : Ir) : Ir => ir(LLVMBuildPtrToInt(ir_builder(), p.v, g_prim_t.t_int64, ""))

//! StackAllocator::push (heap.h) into the caller's stack state, then the Prologue a stack walk reads (simulate.h);
//! StackAllocator's stack and stackTop are the pointers beside its evalTop
def private define_prologue(name : string) : LLVMOpaqueValue? {
    return ir_function(name, jit_fn_type($(funcName, funcLineInfo : void?; stackSize : int; stackState, ctx, at : void?) : void {}), 4, 5) {
        let evalTopP = offset(arg(4), CONTEXT_OFFSET_OF_EVAL_TOP)
        let stackTopP = ptr_slot(evalTopP, 1)
        let top = load_ptr(stackTopP)
        let newTop = offset(top, -zext64(arg(2)))
        throw_if(below(newTop, load_ptr(ptr_slot(evalTopP, -1))), text("stack overflow"))
        store(field(arg(3), g_t_stack_state, 0), load_ptr(evalTopP))
        store(field(arg(3), g_t_stack_state, 1), top)
        store(stackTopP, newTop)
        store(evalTopP, newTop)
        if (JIT_HOST_ASAN) {
            asan_region("unpoison", newTop, zext64(arg(2)))
        }
        let pt = jit_struct_type(type<tuple<info : void?; fileName : void?; functionLine : void?; stackSize : int; flags : int>>)
        let is_jit = ir(1u)
        for (i, v in range(5), fixed_array(ir(LLVMConstNull(g_prim_t.LLVMVoidPtrType())), arg(0), arg(1), arg(2), is_jit)) {
            store(field(newTop, pt, i), v)
        }
    }
}

//! StackAllocator::pop back to the state jit_prologue saved
def private define_epilogue(name : string) : LLVMOpaqueValue? {
    return ir_function(name, jit_fn_type($(stackState, ctx : void?) : void {}), 1, -1) {
        let evalTopP = offset(arg(1), CONTEXT_OFFSET_OF_EVAL_TOP)
        if (JIT_HOST_ASAN) {
            let top = load_ptr(ptr_slot(evalTopP, 1))
            let saved = load_ptr(field(arg(0), g_t_stack_state, 1))
            asan_region("poison", top, address(saved) - address(top))
        }
        store(evalTopP, load_ptr(field(arg(0), g_t_stack_state, 0)))
        store(ptr_slot(evalTopP, 1), load_ptr(field(arg(0), g_t_stack_state, 1)))
    }
}
