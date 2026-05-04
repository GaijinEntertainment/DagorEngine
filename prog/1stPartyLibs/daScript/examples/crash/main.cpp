// Crash handler test — runs a daslang script that triggers a native crash
// inside a C++ bound function, so we can see what the crash handler reports.
//
// Build: part of DAS.sln (example_crash target)
// Run:   cd examples/crash && ../../bin/Release/example_crash.exe main.das

#include "daScript/daScript.h"
#include "daScript/misc/crash_handler.h"

using namespace das;

// This function deliberately crashes — null pointer write
void native_crash() {
    volatile int * p = nullptr;
    *p = 42;
}

// This function overflows the C++ stack
volatile int g_sink = 0;
void native_stack_overflow() {
    volatile char buf[4096];
    buf[0] = 1;
    g_sink += buf[0];
    native_stack_overflow();
    g_sink += buf[0];  // prevent tail-call optimization
}

class Module_Crash : public Module {
public:
    Module_Crash() : Module("crash") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addExtern<DAS_BIND_FUN(native_crash)>(*this, lib, "native_crash",
            SideEffects::modifyExternal, "native_crash");
        addExtern<DAS_BIND_FUN(native_stack_overflow)>(*this, lib, "native_stack_overflow",
            SideEffects::modifyExternal, "native_stack_overflow");
    }
};

REGISTER_MODULE(Module_Crash);

int main(int argc, char * argv[]) {
    install_das_crash_handler();
    if (argc < 2) {
        fprintf(stderr, "Usage: example_crash <script.das>\n");
        return 1;
    }
    NEED_ALL_DEFAULT_MODULES;
    NEED_MODULE(Module_Crash);
    Module::Initialize();
    {
        TextPrinter tout;
        ModuleGroup dummyLibGroup;
        auto fAccess = make_smart<FsFileAccess>();
        auto program = compileDaScript(argv[1], fAccess, tout, dummyLibGroup);
        if (program->failed()) {
            tout << "Compilation failed:\n";
            for (auto & err : program->errors) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
            Module::Shutdown();
            return 1;
        }
        Context ctx(program->getContextStackSize());
        if (!program->simulate(ctx, tout)) {
            tout << "Simulation failed:\n";
            for (auto & err : program->errors) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
            Module::Shutdown();
            return 1;
        }
        auto fnMain = ctx.findFunction("main");
        if (!fnMain) {
            tout << "Function 'main' not found\n";
            Module::Shutdown();
            return 1;
        }
        ctx.evalWithCatch(fnMain, nullptr);
        if (auto ex = ctx.getException()) {
            tout << "Exception: " << ex << "\n";
        }
    }
    Module::Shutdown();
    return 0;
}
