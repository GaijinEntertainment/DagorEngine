// Native failure modes for examples/crash, as a loadable shared module so the same script runs
// under the interpreter and under -jit, and in a daspkg-released bundle.
//
// Everything here exists to fail on purpose. `volatile` and the g_sink accumulator are load-bearing:
// without them the optimizer folds a null store away or turns the recursion into a loop, and the
// harness silently stops reproducing the shape it claims to.

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"

#include <cstdint>      // uintptr_t (native_wild_read)
#include <cstdlib>      // abort
#include <exception>    // std::terminate
#include <stdexcept>    // std::runtime_error

using namespace das;

volatile int g_sink = 0;

// Access violation, write. The canonical 0xC0000005.
void native_null_write() {
    volatile int * p = nullptr;
    *p = 42;
}

// Access violation, read. Distinct from the write case in the WER record's parameter[0].
int32_t native_null_read() {
    volatile const int * p = nullptr;
    return *p;
}

// Reads one page below the guard — an AV at a non-null address, which is what a corrupted
// pointer usually looks like in the wild.
int32_t native_wild_read() {
    volatile const int * p = (volatile const int *)(uintptr_t)0xFFFFFFFFFFFFFFFFull;
    return *p;
}

// EXCEPTION_STACK_OVERFLOW (0xC00000FD). The in-process handler declines this one by design,
// so it is the case that proves whether the dump tier is working.
void native_stack_overflow() {
    volatile char buf[4096];
    buf[0] = 1;
    g_sink += buf[0];
    native_stack_overflow();
    g_sink += buf[0];               // defeat tail-call optimization
}

// std::terminate -> abort -> FAST_FAIL_FATAL_APP_EXIT (0xC0000409 subcode 7). Bypasses
// SetUnhandledExceptionFilter entirely, so no stack trace is printed — only WER sees it.
// This is the shape of the eight unexplained dasllama-server crashes.
void native_uncaught_exception() {
    throw std::runtime_error("deliberate uncaught C++ exception from examples/crash");
}

// Calls std::terminate directly. Discriminates two causes of a silent uncaught_exception:
// if our set_terminate handler fires here but not for a throw, the throw never reaches
// terminate; if it fires for neither, the handler is not visible to this module's CRT.
void native_terminate() {
    std::terminate();
}

// Direct abort(), same fast-fail path without an exception in flight.
void native_abort() {
    abort();
}

// Integer divide by zero — 0xC0000094, a fault class that is easy to confuse with a panic.
int32_t native_divide_by_zero() {
    volatile int zero = 0;
    return 1 / zero;
}

class Module_Crash : public Module {
public:
    Module_Crash() : Module("crash") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addExtern<DAS_BIND_FUN(native_null_write)>(*this, lib, "native_null_write",
            SideEffects::modifyExternal, "native_null_write");
        addExtern<DAS_BIND_FUN(native_null_read)>(*this, lib, "native_null_read",
            SideEffects::modifyExternal, "native_null_read");
        addExtern<DAS_BIND_FUN(native_wild_read)>(*this, lib, "native_wild_read",
            SideEffects::modifyExternal, "native_wild_read");
        addExtern<DAS_BIND_FUN(native_stack_overflow)>(*this, lib, "native_stack_overflow",
            SideEffects::modifyExternal, "native_stack_overflow");
        addExtern<DAS_BIND_FUN(native_uncaught_exception)>(*this, lib, "native_uncaught_exception",
            SideEffects::modifyExternal, "native_uncaught_exception");
        addExtern<DAS_BIND_FUN(native_terminate)>(*this, lib, "native_terminate",
            SideEffects::modifyExternal, "native_terminate");
        addExtern<DAS_BIND_FUN(native_abort)>(*this, lib, "native_abort",
            SideEffects::modifyExternal, "native_abort");
        addExtern<DAS_BIND_FUN(native_divide_by_zero)>(*this, lib, "native_divide_by_zero",
            SideEffects::modifyExternal, "native_divide_by_zero");
    }
};

REGISTER_MODULE(Module_Crash);
// Emits register_dyn_Module_Crash, which .das_module's register_dynamic_module looks up by name.
// Without it the DLL builds and loads but exposes no entry point.
REGISTER_DYN_MODULE(Module_Crash, Module_Crash);
