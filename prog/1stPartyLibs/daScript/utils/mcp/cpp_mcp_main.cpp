// Standalone AOT entry point for the C++-only MCP server (cpp-mcp).
//
// Compiles utils/mcp/cpp_main.das with AOT linking enabled and runs its
// main() — the stdio JSON-RPC loop. The AOT-generated C++ for cpp_main.das
// (and everything it requires) is linked into this binary and self-registers
// via AotListBase; with CodeOfPolicies::aot = true, simulate() swaps the
// interpreter nodes for the native functions.
//
// stdout IS the JSON-RPC framing pipe. On a clean compile (the shipped state)
// nothing is written to it before main() takes over, and cpp_main's own
// logger_install_hook keeps the server's diagnostics off stdout. Compile /
// simulate diagnostics (only on a broken build) are buffered and flushed to
// stderr — never stdout.
//
// Teardown mirrors the daslang tool's main (utils/daScript/main.cpp): the
// context is a heap ContextPtr destroyed *before* Module::Shutdown, which
// drains the jobque worker threads. Matching that order is what keeps the
// jobque + persistent_heap teardown crash-free.
//
// Built only when -DDAS_BUILD_CPP_MCP=ON; see CMakeLists.txt.

#include "daScript/daScript.h"
#include "daScript/daScriptModule.h"
#include "daScript/misc/das_common.h"     // SimulateWithErrReport
#include "daScript/misc/crash_handler.h"  // install_das_crash_handler

#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#include <direct.h>
#define das_getcwd _getcwd
#else
#include <unistd.h>
#define das_getcwd getcwd
#endif

using namespace das;

static bool cpp_mcp_file_exists(const string & p) {
    FILE * f = fopen(p.c_str(), "rb");
    if ( f ) { fclose(f); return true; }
    return false;
}

// Resolve where the bundled .das sources live. The AOT binary still compiles
// cpp_main.das (+ daslib) at startup (AOT only swaps SimNodes for native code),
// so the sources must be findable. Order: an explicit --root / -dasroot <dir>;
// else getDasRoot() derived from the exe path (the bundle layout <root>/bin/);
// else the cwd, if it happens to carry the sources (exe moved / run in place).
static void cpp_mcp_resolve_root(int argc, char * argv[]) {
    for ( int i = 1; i + 1 < argc; ++i ) {
        if ( strcmp(argv[i], "--root") == 0 || strcmp(argv[i], "-dasroot") == 0 ) {
            setDasRoot(argv[i + 1]);
            return;
        }
    }
    if ( cpp_mcp_file_exists(getDasRoot() + "/utils/mcp/cpp_main.das") ) return;
    char cwd[4096];
    if ( das_getcwd(cwd, sizeof(cwd)) ) {
        string c = cwd;
        if ( cpp_mcp_file_exists(c + "/utils/mcp/cpp_main.das") ) setDasRoot(c);
    }
}

int main(int argc, char * argv[]) {
    install_das_crash_handler();
    setCommandLineArguments(argc, argv);
    cpp_mcp_resolve_root(argc, argv);

    // Full builtin module set — same as the daslang tool. Notably this
    // includes jobque (new_thread / channels), which NEED_ALL_DEFAULT_MODULES
    // omits and which cpp_main's per-tool-call worker threads rely on.
    register_builtin_modules();
    Module::Initialize();

    int rc = 1;
    // Everything that owns module references — TextPrinter, file access, the
    // ModuleGroup, the program and its context — lives in this scope and is
    // destroyed at its close, BEFORE Module::Shutdown. ~ModuleGroup touches the
    // global module registry, so it must run while that registry is still
    // alive; destroying it after Module::Shutdown is an access violation
    // (matches the daslang tool / 13_aot.cpp ordering).
    {
        // A buffer, NOT TextPrinter: TextPrinter writes to stdout, which IS the
        // JSON-RPC framing pipe (corrupting it). Compile/simulate diagnostics
        // accumulate here and flush to stderr at the end of this scope (only a
        // broken build produces any).
        TextWriter tout;
        auto access = make_smart<FsFileAccess>();
        ModuleGroup dummyGroup;

        CodeOfPolicies policies;
        policies.aot = true;                        // link AOT functions during simulate
        policies.fail_on_no_aot = false;            // any non-AOT function falls back to interpreter
        policies.fail_on_lack_of_aot_export = false;

        auto program = compileDaScript(getDasRoot() + "/utils/mcp/cpp_main.das",
                                       access, tout, dummyGroup, policies);
        if (program && !program->failed()) {
            // Heap context owned by a smart pointer — destroyed at the end of
            // this scope, before Module::Shutdown (matches the daslang tool).
            auto pctx = SimulateWithErrReport(program, tout);
            if (pctx) {
                if (auto fnMain = pctx->findFunction("main")) {
                    pctx->restart();
                    pctx->evalWithCatch(fnMain, nullptr);
                    if (auto ex = pctx->getException()) {
                        tout << "EXCEPTION: " << ex << "\n";
                    } else {
                        rc = 0;
                    }
                } else {
                    tout << "cpp_main.das: main() not found\n";
                }
            }
        } else if (program) {
            for (auto & err : program->errors) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
        }
        // Flush any buffered diagnostics to stderr — never stdout (the pipe).
        if (!tout.empty()) {
            fputs(tout.c_str(), stderr);
            fflush(stderr);
        }
    }

    Module::Shutdown(false);
    return rc;
}
