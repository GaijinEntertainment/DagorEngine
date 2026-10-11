#include "daScript/daScriptModule.h"
#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/misc/sysos.h"

#ifdef DAS_ENABLE_DYN_INCLUDES
#include "daScript/ast/dyn_modules.h"
#include "daScript/simulate/fs_file_info.h"
#endif

#include "daScript/ast/ast.h"                     // astTypeInfo
#include "daScript/ast/ast_handle.h"              // addConstant
#include "daScript/ast/ast_interop.h"             // addExternInline

#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_jit.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/aot_builtin_rtti.h"       // builtin_getFunctionByMnh
#include "daScript/simulate/debug_info.h"
#include "daScript/simulate/debug_print.h"
#include "daScript/simulate/hash.h"               // stringLength
#include "daScript/simulate/heap.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_visit_op.h"

#include "daScript/misc/fpe.h"
#include "daScript/misc/sysos.h"
#include "misc/include_fmt.h"

#include "module_builtin_rtti.h"
#include "jit_runtime.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <algorithm>

#include "daScript/misc/job_que.h"   // JobQue::get_num_threads - the split emit pool's auto count
#if !DAS_NO_FILEIO
#include <filesystem>
#endif

// MSVC ships popen/pclose under _popen/_pclose; alias once for the whole TU.
#if defined(_WIN32) || defined(_WIN64)
    #define popen _popen
    #define pclose _pclose
#endif

namespace das {
    // forward declarations from module_builtin_fio.cpp
    void * register_dynamic_module(const char *, const char *, int, Context *, LineInfoArg *);
    void register_native_path(const char *, const char *, const char *, Context *, LineInfoArg *);
    bool builtin_fexist ( const char * path );
    DAS_API void retry_pending_dynamic_modules();
    DAS_API int report_pending_dynamic_modules();



    uint64_t das_get_global_variable_offset( const Context * ctx, int id ) {
        return ctx->getGlobalVariable(id).offset;
    }

    uint64_t das_get_global_variable_mnh( const Context * ctx, int id ) {
        return ctx->getGlobalVariable(id).mangledNameHash;
    }

    const char * das_get_global_variable_name( const Context * ctx, int id ) {
        return ctx->getGlobalVariable(id).name;
    }

    void * das_get_global_variable_debug_info( const Context * ctx, int id ) {
        return (void *) ctx->getGlobalVariable(id).debugInfo;
    }

    int das_get_global_variable_shared( const Context * ctx, int id ) {
        return ctx->getGlobalVariable(id).shared ? 1 : 0;
    }

    int das_get_context_total_functions( const Context * ctx ) {
        return ctx->getTotalFunctions();
    }

    // the capture argument's type of a lambda body, null for any other function
    void * das_get_lambda_capture_debug_info( const Context * ctx, int id ) {
        auto fn = ctx->getFunction(id);
        if ( !fn ) return nullptr;
        auto finfo = fn->debugInfo;
        if ( !finfo || !finfo->fields || finfo->count < 1 ) return nullptr;
        auto arg = finfo->fields[0];
        if ( !arg || arg->type != Type::tStructure || !arg->structType ) return nullptr;
        if ( !(arg->structType->flags & StructInfo::flag_lambda) ) return nullptr;
        return (void *) arg;
    }

    uint64_t das_get_context_globals_size( const Context * ctx ) {
        return ctx->getGlobalSize();
    }

    uint64_t das_get_context_shared_size( const Context * ctx ) {
        return ctx->getSharedSize();
    }

    void * das_instrument_line_info ( const LineInfo & info, Context * context, LineInfoArg * at ) {
        LineInfo * info_ptr = (LineInfo *) context->code->allocate(sizeof(LineInfo));
        if ( !info_ptr ) context->throw_error_at(at, "can't instrument line info, out of code heap");
        *info_ptr = info;
        return (void *) info_ptr;
    }

    void das_recreate_fileinfo_name ( FileInfo * info, const char * name, Context *, LineInfoArg *  ) {
        info->name = string{ name };
    }

    bool check_file_present ( const char * filename ) {
        if ( FILE * file = fopen(filename, "r"); file == NULL ) {
            return false;
        } else {
            fclose(file);
            return true;
        }
    }

    struct LinkerPaths {
        string linker;
        string runtimeLibrary;  // always needed
        string compilerLibrary; // non-empty when linkWholeLib — exe also needs compiler lib
    };

    // Resolve a linker / tool path. Lookup order:
    //   1. `custom` — explicit override (non-null, non-empty)
    //   2. `<das_root>/bin/<bundledName>` if present on disk
    //   3. `fallback` — bare name picked up via PATH
    // Callers pass the platform-specific bundled name (e.g. `wasm-ld.exe` on
    // Windows, `wasm-ld` on POSIX, `emcc.bat` on Windows, etc.).
    static string find_linker(const char * custom, const string & bundledName, const string & fallback) {
        if ( custom != nullptr && custom[0] != '\0' ) return custom;
        string bundled = getDasRoot() + "/bin/" + bundledName;
        if ( check_file_present(bundled.c_str()) ) return bundled;
        return fallback;
    }

    // Host-toolchain JIT triple — when non-empty, daslib uses this instead of
    // LLVMGetDefaultTargetTriple() to drive JIT codegen.
    //
    // The prebuilt LLVM.dll shipped via dasLLVM's FetchContent is built with
    // MSVC clang-cl, so LLVMGetDefaultTargetTriple() always returns
    // x86_64-pc-windows-msvc regardless of what host compiler built
    // daslang.exe. On mingw daslang.exe, MSVC-ABI .o files reference
    // _fltused (msvcrt marker symbol) and other MSVC-CRT-isms that mingw's
    // ld.lld + libc CRT cannot satisfy. Forcing a mingw triple makes LLVM
    // emit a mingw-flavored .o with no MSVC marker symbols, links cleanly
    // against libDaScriptDyn_runtime.dll.a, and the resulting DLL calls
    // into the mingw daslang.exe via the same C ABI without CRT-mismatch
    // hazards.
    //
    // The arch suffix has to match the host — clang32 (i686), clang64
    // (x86_64), clangarm64 (aarch64), clangarm32 (armv7) all share the
    // mingw vendor/os/env suffix but differ on the arch component.
    // Detection uses the standard GCC/Clang arch predefined macros so
    // this works for both compilers on every supported mingw arch.
    const char * host_jit_triple ( ) {
    #if defined(_WIN32) && !defined(_MSC_VER)
        #if defined(__aarch64__) || defined(_M_ARM64)
            return "aarch64-w64-windows-gnu";
        #elif defined(__arm__) || defined(_M_ARM)
            return "armv7-w64-windows-gnu";
        #elif defined(__x86_64__) || defined(_M_X64)
            return "x86_64-w64-windows-gnu";
        #elif defined(__i386__) || defined(_M_IX86)
            return "i686-w64-windows-gnu";
        #else
            // Unknown mingw arch — fall back to LLVM's default and let
            // the user explicitly override via the customLinker path.
            return "";
        #endif
    #else
        return "";
    #endif
    }

    static LinkerPaths get_real_lib_linker_paths(const char * dasLib, const char * customLinker, bool isShared, bool linkWholeLib) {
        LinkerPaths result;
        result.linker = customLinker != nullptr ? customLinker : "";
        result.runtimeLibrary = dasLib != nullptr ? dasLib : "";

        if (result.linker.empty() || result.runtimeLibrary.empty()) {
            #if defined(_WIN32) || defined(_WIN64)
                // Two distinct Windows toolchains share the _WIN32 macro:
                //   MSVC (incl. clang-cl) → MSVC-style import libs (libFoo.lib),
                //     linked via lld-link.exe with /DLL /OUT: link.exe syntax.
                //     lld-link (LLVM ≥14) autodetects the MSVC + WinSDK lib
                //     paths itself (llvm/WindowsDriver, shared with clang), so
                //     no compiler driver sits in front of the link.
                //   mingw (clang-mingw / gcc-mingw) → mingw import libs
                //     (libFoo.dll.a), linked via clang.exe with -shared / -o
                //     syntax (Unix-like — clang.exe driver is identical to
                //     posix clang). The split keeps daslang.exe built with one
                //     toolchain talking to a JIT'd DLL built with the same one.
                #if defined(_MSC_VER)
                    if (result.linker.empty()) {
                        result.linker = find_linker(nullptr, "lld-link.exe", "lld-link");
                    }
                    if (result.runtimeLibrary.empty()) {
                        const auto path = get_prefix(getExecutableFileName());
                        const auto winCfg = path.substr(path.find_last_of("\\/") + 1);
                        const auto windowsConfig = (winCfg == "bin" ? "" : (winCfg + "/"));
                        result.runtimeLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn_runtime.lib";
                        if (linkWholeLib) {
                            result.compilerLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn.lib";
                        }
                    }
                #else
                    // mingw — note bundled clang-cl.exe (from dasLLVM prebuilt
                    // LLVM) is MSVC-flavored and cannot read .dll.a, so don't
                    // probe for it; default to "clang" on PATH (clang64/bin
                    // when daslang.exe was built with clang-mingw64).
                    if (result.linker.empty()) {
                        result.linker = "clang";
                    }
                    if (result.runtimeLibrary.empty()) {
                        const auto path = get_prefix(getExecutableFileName());
                        const auto winCfg = path.substr(path.find_last_of("\\/") + 1);
                        const auto windowsConfig = (winCfg == "bin" ? "" : (winCfg + "/"));
                        result.runtimeLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn_runtime.dll.a";
                        if (linkWholeLib) {
                            result.compilerLibrary = getDasRoot() + "/lib/" + windowsConfig + "libDaScriptDyn.dll.a";
                        }
                    }
                #endif
            #else
                if (result.linker.empty()) {
                    result.linker = "c++";
                }
                if (result.runtimeLibrary.empty()) {
                #if defined(__APPLE__)
                    result.runtimeLibrary = getDasRoot() + "/lib/liblibDaScriptDyn_runtime.dylib";
                    if (linkWholeLib) {
                        result.compilerLibrary = getDasRoot() + "/lib/liblibDaScriptDyn.dylib";
                    }
                #else
                    result.runtimeLibrary = getDasRoot() + "/lib/liblibDaScriptDyn_runtime.so";
                    if (linkWholeLib) {
                        result.compilerLibrary = getDasRoot() + "/lib/liblibDaScriptDyn.so";
                    }
                #endif
                }
            #endif
        }
        return result;
    }

#if (defined(_WIN32) || defined(__linux__) || defined(__APPLE__)) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    // Run a fully-formed linker command via popen, drain it to EOF keeping the
    // last 16KB of combined stdout/stderr (the diagnostic sits at the end; a
    // reader that stops early kills the linker with SIGPIPE), then log success
    // or a failure diagnostic (with the captured tail) through the daslang Context.
    //   cmd          — complete shell command, already includes 2>&1.
    //   artifactPath — output file path (logged on success/failure).
    //   artifactKind — short label for messages ("Library", "Wasm", ...).
    static bool run_link_cmd(const char * cmd, const char * artifactPath,
                             const char * artifactKind, Context * context) {
        FILE * fp = popen(cmd, "r");
        if ( fp == NULL ) {
            LOG(LogLevel::error) << "Failed to run command '" << cmd << "'\n";
            return false;
        }
        static constexpr size_t MAX_OUTPUT_SIZE = 16 * 1024;
        char buffer[1024];
        string output;
        while ( fgets(buffer, sizeof(buffer), fp) != NULL ) {
            output += buffer;
            if ( output.size() > MAX_OUTPUT_SIZE ) {
                output.erase(0, output.size() - MAX_OUTPUT_SIZE);
            }
        }
        auto li = LineInfoArg();
        if ( int status = pclose(fp); status != 0 ) {
            string msg = string("Failed to link ") + artifactKind + " " + artifactPath + ", command '" + cmd + "'\n";
            toLog(LogLevel::error, msg.c_str(), context, &li);
            string err = string("Output:\n") + output;
            toLog(LogLevel::error, err.c_str(), context, &li);
            return false;
        }
        string msg = string(artifactKind) + " " + artifactPath + " linked - ok\n";
        toLog(LogLevel::info, msg.c_str(), context, &li);
        return true;
    }

    bool create_shared_library ( const char * objFilePath, const char * libraryName, [[maybe_unused]] const char * dasLib, const char * customLinker, const char * extraLinkerArgs, bool isShared, bool linkWholeLib, [[maybe_unused]] bool debugInfo, Context *context ) {
        // cmd is built via fmt::format (heap-allocated std::string) rather
        // than a fixed stack buffer — long paths (deep build roots, spaces,
        // long extraLinkerArgs) can easily exceed a few hundred bytes, and
        // a fixed buffer would silently corrupt the stack on overflow.
        const char * extra = (extraLinkerArgs && extraLinkerArgs[0]) ? extraLinkerArgs : "";
        const auto paths = get_real_lib_linker_paths(dasLib, customLinker, isShared, linkWholeLib);
        const auto & linker = paths.linker;
        const auto & runtimeLibrary = paths.runtimeLibrary;
        const auto & compilerLibrary = paths.compilerLibrary;

        #if defined(_WIN32) || defined(_WIN64) || defined(__APPLE__)
        if (!check_file_present(runtimeLibrary.c_str())) {
            LOG(LogLevel::error) << "File '" << runtimeLibrary << "' , containing daslang runtime library, does not exist\n";
            return false;
        }
        #endif
        if (!check_file_present(objFilePath)) {
            LOG(LogLevel::error) << "File '" << objFilePath << "' , containing compiled definitions, does not exist\n";
            return false;
        }

        std::string cmd;
        #if defined(_WIN32) || defined(_WIN64)
            #if defined(_MSC_VER)
                // MSVC lld-link: link.exe-flavored args — /DLL marks shared,
                // /OUT: names the output, msvcrt.lib pulls the import CRT
                // (the JIT object carries no /DEFAULTLIB directive, so the CRT
                // must be named explicitly). `extra` is raw linker flags here,
                // not compiler-driver flags. Outer doubled quotes wrap the
                // whole command for cmd.exe because the linker path itself
                // contains spaces on Program Files installs.
                const auto linkerParam = isShared ? "/DLL" : "";
                // /DEBUG makes lld-link emit a PDB next to /OUT. Without CodeView in the
                // object it carries publics only; once the emitter attaches DI it upgrades
                // to full line info for the same flag.
                const auto debugParam = debugInfo ? "/DEBUG" : "";
                // Keep a compact linker map beside every JIT PE. A normal minidump deliberately
                // omits the process heap (and therefore model weights), while the map plus the
                // retained COFF object resolves generated module+RVA frames after a crash.
                std::string mapPath = libraryName;
                const auto slash = mapPath.find_last_of("/\\");
                const auto dot = mapPath.find_last_of('.');
                if ( dot == std::string::npos || (slash != std::string::npos && dot < slash) ) {
                    mapPath += ".map";
                } else {
                    mapPath.replace(dot, std::string::npos, ".map");
                }
                cmd = compilerLibrary.empty()
                    ? fmt::format(FMT_STRING("\"\"{}\" \"{}\" \"{}\" msvcrt.lib {} {} {} /OUT:\"{}\" /MAP:\"{}\" 2>&1\""), linker.c_str(), objFilePath, runtimeLibrary.c_str(), extra, linkerParam, debugParam, libraryName, mapPath.c_str())
                    : fmt::format(FMT_STRING("\"\"{}\" \"{}\" \"{}\" \"{}\" msvcrt.lib {} {} {} /OUT:\"{}\" /MAP:\"{}\" 2>&1\""), linker.c_str(), objFilePath, runtimeLibrary.c_str(), compilerLibrary.c_str(), extra, linkerParam, debugParam, libraryName, mapPath.c_str());
            #else
                // mingw clang/gcc: Unix-flavored driver, -shared/-o syntax.
                // No rpath on Windows (DLLs resolve via PATH / LoadLibrary
                // search order), so the linux branch's rpath escape is
                // dropped. The linux branch's -Wl,--no-as-needed is also
                // dropped: on ELF, --no-as-needed forces a DT_NEEDED entry
                // for libraries that the linker would otherwise mark as
                // not-required (lazy bind). PE/COFF has no such concept —
                // listed libraries are always recorded in the import table,
                // so the flag has no effect. Worse, lld on newer mingw
                // sysroots rejects --no-as-needed as an unknown argument
                // (older clangs / GNU ld silently ignore it, which is why
                // this only surfaces on fresh CI installs).
                // Outer doubled quotes `\"...2>&1\"` wrap the whole command
                // — popen → cmd.exe strips one quote pair, leaving the
                // inner per-arg quotes intact. Without this wrap cmd.exe
                // misparses the very first quoted token as the command name.
                const auto linkerParam = isShared ? "-shared" : "";
                cmd = compilerLibrary.empty()
                    ? fmt::format(FMT_STRING("\"\"{}\" {} -o \"{}\" \"{}\" \"{}\" {} 2>&1\""),
                                            linker.c_str(), linkerParam, libraryName, objFilePath, runtimeLibrary.c_str(), extra)
                    : fmt::format(FMT_STRING("\"\"{}\" {} -o \"{}\" \"{}\" \"{}\" \"{}\" {} 2>&1\""),
                                            linker.c_str(), linkerParam, libraryName, objFilePath, runtimeLibrary.c_str(), compilerLibrary.c_str(), extra);
            #endif
        #elif defined(__APPLE__)
            const auto linkerParam = isShared ? "-shared " : "";
            // installed layout is bin/ + lib/, hence the two @executable_path rpaths; the
            // build-tree path is last so the dev workflow works without copying dylibs.
            // The embedded `\" \"` splits the format-string's quotes into separate -Wl,-rpath args.
            const auto rpath = "-Wl,-rpath,@executable_path\" \"-Wl,-rpath,@executable_path/../lib\" \"-Wl,-rpath," + get_prefix(runtimeLibrary);
            cmd = compilerLibrary.empty()
                ? fmt::format(FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" {} 2>&1"), linker.c_str(), linkerParam, rpath.c_str(), libraryName, runtimeLibrary.c_str(), objFilePath, extra)
                : fmt::format(FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" \"{}\" {} 2>&1"), linker.c_str(), linkerParam, rpath.c_str(), libraryName, runtimeLibrary.c_str(), compilerLibrary.c_str(), objFilePath, extra);
        #else
            const auto linkerParam = isShared ? "-shared" : "";
            // installed layout is bin/ + lib/, hence the two $ORIGIN rpaths; the build-tree
            // path is last so the dev workflow works without copying. \$ keeps popen's shell
            // from expanding $ORIGIN; the embedded `\" \"` yields separate -Wl,-rpath args.
            const auto rpath = "-Wl,-rpath,\\$ORIGIN\" \"-Wl,-rpath,\\$ORIGIN/../lib\" \"-Wl,-rpath," + get_prefix(runtimeLibrary);
            cmd = compilerLibrary.empty()
                ? fmt::format(FMT_STRING("\"{}\" {} \"{}\" -o \"{}\" \"{}\" \"{}\" {} 2>&1"),
                                        linker.c_str(), linkerParam, rpath.c_str(), libraryName, objFilePath, runtimeLibrary.c_str(), extra)
                : fmt::format(FMT_STRING("\"{}\" {} \"{}\" -Wl,--no-as-needed -o \"{}\" \"{}\" \"{}\" \"{}\" {} 2>&1"),
                                        linker.c_str(), linkerParam, rpath.c_str(), libraryName, objFilePath, runtimeLibrary.c_str(), compilerLibrary.c_str(), extra);
        #endif
        return run_link_cmd(cmd.c_str(), libraryName, "Library", context);
    }

    bool create_static_library ( const char * objFilePath, const char * libraryName, const char * customTool, Context * context ) {
        if ( !check_file_present(objFilePath) ) {
            LOG(LogLevel::error) << "File '" << objFilePath << "' , containing compiled definitions, does not exist\n";
            return false;
        }
        remove(libraryName);
        std::string cmd;
        #if defined(_WIN32) || defined(_WIN64)
            #if defined(_MSC_VER)
                const auto tool = find_linker(customTool, "llvm-lib.exe", "lib");
                cmd = fmt::format(FMT_STRING("\"\"{}\" /nologo /OUT:\"{}\" \"{}\" 2>&1\""), tool.c_str(), libraryName, objFilePath);
            #else
                const auto tool = find_linker(customTool, "llvm-ar.exe", "ar");
                cmd = fmt::format(FMT_STRING("\"\"{}\" rcs \"{}\" \"{}\" 2>&1\""), tool.c_str(), libraryName, objFilePath);
            #endif
        #else
            const auto tool = find_linker(customTool, "llvm-ar", "ar");
            cmd = fmt::format(FMT_STRING("\"{}\" rcs \"{}\" \"{}\" 2>&1"), tool.c_str(), libraryName, objFilePath);
        #endif
        return run_link_cmd(cmd.c_str(), libraryName, "Archive", context);
    }
#else
    bool create_shared_library ( const char * objFilePath, const char * libraryName, [[maybe_unused]] const char * dasLib, const char * customLinker, const char * extraLinkerArgs, bool isShared, bool linkWholeLib, bool debugInfo, Context *context ) { return true; }
    bool create_static_library ( const char * objFilePath, const char * libraryName, const char * customTool, Context * context ) { return true; }
#endif

    // ===== --jit-split-modules parallel optimize+emit =====
    // LLVM-agnostic job runner: the das side resolves the LLVM C-API entry points through its own
    // dasbind registry (one resolution truth, libDaScript never links LLVM) and hands them over as
    // raw pointers. Each job owns its (module, target machine, pass-builder options) trio, so
    // LLVM's one-context-per-thread contract holds by construction. No shared mutable state and
    // no lock: workers claim jobs via one atomic fetch_add and write only their own job's slots;
    // Context is not thread-safe, so failures/walls are reported after join on the calling thread.
    struct JitParEmitJob {
        void * mod;
        void * tm;
        void * pbo;
        string pipeline;
        string objPath;
        double passesSec = 0.0;
        double emitSec = 0.0;
        string error;
    };
    static vector<JitParEmitJob> g_jitParEmitJobs;

    void jit_par_emit_begin () {
        g_jitParEmitJobs.clear();
    }

    void jit_par_emit_add ( void * mod, void * tm, void * pbo, const char * pipeline, const char * objPath ) {
        JitParEmitJob job;
        job.mod = mod;
        job.tm = tm;
        job.pbo = pbo;
        job.pipeline = pipeline ? pipeline : "";
        job.objPath = objPath ? objPath : "";
        g_jitParEmitJobs.emplace_back(std::move(job));
    }

    // threads <= 0 = auto (JobQue::get_num_threads - honors DAS_JOBQUE_THREADS, the das-side cap,
    // Apple P-core selection, and >64-CPU Windows processor groups); always clamped to [1, jobs].
    // fileType is LLVMCodeGenFileType (the das side passes the enum value). An empty pipeline
    // skips the pass run (the LLVM_ENABLE_OPT_PASS debug rail). Each module is verified AFTER
    // passes and BEFORE emission - the monolith ordering; a broken module never reaches ISel.
    // Fail-closed: any job failure reports every collected error and returns false; caller panics.
    bool jit_par_emit_run ( int32_t threads, int32_t fileType, void * fnRunPasses, void * fnVerifyModule,
                            void * fnEmitToFile, void * fnGetErrorMessage, void * fnDisposeErrorMessage,
                            void * fnDisposeMessage, bool logTimes, Context * context ) {
        // LLVMErrorRef LLVMRunPasses(M, Passes, TM, Options); LLVMBool LLVMVerifyModule(M, Action, char** Msg);
        // LLVMBool LLVMTargetMachineEmitToFile(T, M, Filename, codegen, char** Err)
        using RunPassesFn = void * (*) ( void *, const char *, void *, void * );
        using VerifyModuleFn = int32_t (*) ( void *, int32_t, char ** );
        using EmitToFileFn = int32_t (*) ( void *, void *, const char *, int32_t, char ** );
        using GetErrMsgFn = char * (*) ( void * );
        using DisposeFn = void (*) ( void * );
        auto runPasses = (RunPassesFn) fnRunPasses;
        auto verifyModule = (VerifyModuleFn) fnVerifyModule;
        auto emitToFile = (EmitToFileFn) fnEmitToFile;
        auto getErrMsg = (GetErrMsgFn) fnGetErrorMessage;
        auto disposeErrMsg = (DisposeFn) fnDisposeErrorMessage;
        auto disposeMsg = (DisposeFn) fnDisposeMessage;
        auto li = LineInfoArg();
        // the das caller validates each symbol by name first; this is the complete backstop so a
        // future caller can't run workers that leak diagnostics through null dispose/message fns
        if ( !runPasses || !verifyModule || !emitToFile || !getErrMsg || !disposeErrMsg || !disposeMsg
                || g_jitParEmitJobs.empty() ) {
            toLog(LogLevel::error, "jit_par_emit_run: missing entry points or no jobs\n", context, &li);
            g_jitParEmitJobs.clear();
            return false;
        }
        int nThreads = threads > 0 ? threads : JobQue::get_num_threads();
        nThreads = std::max(1, std::min(nThreads, int(g_jitParEmitJobs.size())));
        std::atomic<size_t> nextJob{0};
        constexpr int32_t LLVM_RETURN_STATUS_ACTION = 2;   // LLVMReturnStatusAction
        auto worker = [&]() {
            for ( ;; ) {
                size_t i = nextJob.fetch_add(1);
                if ( i >= g_jitParEmitJobs.size() ) break;
                auto & job = g_jitParEmitJobs[i];
                auto t0 = ref_time_ticks();
                if ( !job.pipeline.empty() ) {
                    void * err = runPasses(job.mod, job.pipeline.c_str(), job.tm, job.pbo);
                    if ( err ) {
                        char * msg = getErrMsg ? getErrMsg(err) : nullptr;   // consumes err
                        job.error = string("pass run failed: ") + (msg ? msg : "(no message)");
                        if ( msg && disposeErrMsg ) disposeErrMsg(msg);
                        continue;
                    }
                }
                char * vmsg = nullptr;
                if ( verifyModule(job.mod, LLVM_RETURN_STATUS_ACTION, &vmsg) != 0 ) {
                    job.error = string("post-optimize verify failed for ") + job.objPath + ": " + (vmsg ? vmsg : "(no message)");
                    if ( vmsg && disposeMsg ) disposeMsg(vmsg);
                    continue;
                }
                if ( vmsg && disposeMsg ) disposeMsg(vmsg);
                job.passesSec = double(get_time_usec(t0)) / 1000000.0;
                auto t1 = ref_time_ticks();
                char * emitErr = nullptr;
                if ( job.objPath.empty() ) {
                    job.emitSec = 0.0;
                    continue;
                }
                if ( emitToFile(job.tm, job.mod, job.objPath.c_str(), fileType, &emitErr) != 0 ) {
                    job.error = string("emit failed for ") + job.objPath + ": " + (emitErr ? emitErr : "(no message)");
                    if ( emitErr && disposeMsg ) disposeMsg(emitErr);
                    continue;
                }
                job.emitSec = double(get_time_usec(t1)) / 1000000.0;
            }
        };
        if ( nThreads == 1 ) {
            worker();
        } else {
            vector<std::thread> pool;
            pool.reserve(nThreads);
            for ( int t = 0; t != nThreads; ++t ) pool.emplace_back(worker);
            for ( auto & th : pool ) th.join();
        }
        bool ok = true;
        for ( auto & job : g_jitParEmitJobs ) {
            if ( !job.error.empty() ) {
                ok = false;
                string msg = string("LLVM JIT: parallel emit: ") + job.error + "\n";
                toLog(LogLevel::error, msg.c_str(), context, &li);
            } else if ( logTimes ) {
                auto msg = fmt::format(FMT_STRING("LLVM JIT time: job {} passes {:.6f} emit {:.6f}\n"),
                    job.objPath.c_str(), job.passesSec, job.emitSec);
                toLog(LogLevel::info, msg.c_str(), context, &li);
            }
        }
        g_jitParEmitJobs.clear();
        return ok;
    }

#if (defined(_WIN32) || defined(__linux__) || defined(__APPLE__)) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    // Cross-compile-link a wasm32 object into a standalone .wasm via emcc.
    //   runtimeLibPath — path to libDaScript_runtime.a (wasm32). Optional:
    //     present  → link runtime symbols (das_*, malloc, memcpy, …) in.
    //     missing  → link without; only safe for pure programs that touch
    //                no daslang runtime.
    //   customEmcc — explicit emcc override (nullptr/empty = resolve via
    //                bin/ then PATH).
    // emcc drives wasm-ld plus libc / wasi / wasm-exceptions glue, so output
    // is self-contained -sSTANDALONE_WASM (only wasi imports).
    bool link_wasm ( const char * objFilePath, const char * wasmPath,
                     const char * runtimeLibPath, const char * customEmcc,
                     bool memory64, Context * context ) {
        #if defined(_WIN32) || defined(_WIN64)
            const auto linker = find_linker(customEmcc, "emcc.bat", "emcc");
        #else
            const auto linker = find_linker(customEmcc, "emcc", "emcc");
        #endif
        if ( !check_file_present(objFilePath) ) {
            LOG(LogLevel::error) << "File '" << objFilePath << "' , containing wasm object, does not exist\n";
            return false;
        }
        const bool withRuntime = runtimeLibPath != nullptr && runtimeLibPath[0] != '\0'
                                 && check_file_present(runtimeLibPath);
        if ( runtimeLibPath != nullptr && runtimeLibPath[0] != '\0' && !withRuntime ) {
            LOG(LogLevel::warning) << "libDaScript_runtime archive '" << runtimeLibPath
                                   << "' not found - linking without; runtime refs will fail\n";
        }
        // -sSTANDALONE_WASM: emit self-contained .wasm with wasi imports only.
        // -fwasm-exceptions + -sWASM_LEGACY_EXCEPTIONS=0: match the runtime
        // archive's modern wasm EH, avoid emcc's JS invoke_* trampolines.
        // -sINITIAL_MEMORY=128MB: reserve up front so typical programs never pay
        // growth churn. Cost is virtual address space only — lazy paging keeps RSS
        // proportional to actual use.
        // -sABORTING_MALLOC=0: state the contract we rely on rather than inherit it.
        // Every das allocation path null-checks (LinearChunkAllocator::allocate ->
        // StringHeapAllocator::impl_allocateString -> context->throw_out_of_memory),
        // which only works while a failed malloc RETURNS. ALLOW_MEMORY_GROWTH happens
        // to imply this today; spelling it out keeps the guarantee if growth is ever
        // turned back off for a host that needs a fixed memory.
        // -sALLOW_MEMORY_GROWTH=1: 128MB is NOT enough for every program (the f2s
        // playground benchmark retains ~130MB of string heap and died at the cap).
        // Previously omitted because growth added an unsatisfiable
        // emscripten_notify_memory_growth import under a bare wasi host (#2805);
        // that is fixed upstream — verified on the pinned emsdk 5.0.7 that a
        // STANDALONE_WASM build with growth imports NOTHING from env, so wasmtime is
        // unaffected. Measured on that emsdk: usable heap 120MB -> 2040MB.
        const std::string runtimeArg = withRuntime ? fmt::format("\"{}\" ", runtimeLibPath) : "";
        // -sMEMORY64=2: wasm64 (8-byte pointer) object + runtime archive, memory lowered
        // to 32-bit at link so engines without memory64 (WebKit) run the module too.
        const char * mem64Arg = memory64 ? " -sMEMORY64=2" : "";
        // Windows: wrap the whole command in an extra outer quote pair
        // (`""emcc.bat" ... 2>&1"`). popen → cmd.exe strips the outermost pair,
        // leaving the inner quoted argv intact; without this wrap cmd.exe mangles
        // the quoted linker/paths and fails with "filename/directory syntax
        // incorrect". Same trick create_shared_library uses (see above).
        #if defined(_WIN32) || defined(_WIN64)
        const std::string cmd = fmt::format(
            FMT_STRING("\"\"{}\" \"{}\" {}-o \"{}\" -sSTANDALONE_WASM -fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=0 -sINITIAL_MEMORY=128MB -sABORTING_MALLOC=0 -sALLOW_MEMORY_GROWTH=1{} 2>&1\""),
            linker.c_str(), objFilePath, runtimeArg, wasmPath, mem64Arg);
        #else
        const std::string cmd = fmt::format(
            FMT_STRING("\"{}\" \"{}\" {}-o \"{}\" -sSTANDALONE_WASM -fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=0 -sINITIAL_MEMORY=128MB -sABORTING_MALLOC=0 -sALLOW_MEMORY_GROWTH=1{} 2>&1"),
            linker.c_str(), objFilePath, runtimeArg, wasmPath, mem64Arg);
        #endif
        return run_link_cmd(cmd.c_str(), wasmPath, "Wasm", context);
    }
#else
    bool link_wasm ( const char *, const char *, const char *, const char *, bool, Context * ) { return true; }
#endif

    void jit_set_jit_state(Context & context, void *shared_lib, void *llvm_ee, void *llvm_context) {
        context.deleteJITOnFinish.shared_lib = shared_lib;
        context.deleteJITOnFinish.llvm_ee = llvm_ee;
        context.deleteJITOnFinish.llvm_context = llvm_context;
    }

    void jit_get_jit_state(const TBlock<void,const void*, const void*, const void*> &block, Context * context, LineInfoArg * at) {
        vec4f args[3];
        args[0] = cast<void *>::from(context->deleteJITOnFinish.shared_lib);
        args[1] = cast<void *>::from(context->deleteJITOnFinish.llvm_ee);
        args[2] = cast<void *>::from(context->deleteJITOnFinish.llvm_context);
        context->invoke(block, args, nullptr, at);
    }

    class Module_Jit : public Module {
    public:
        Module_Jit() : Module("jit") {
            DAS_PROFILE_SECTION("Module_Jit");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti_core"));
            addBuiltinDependency(lib, Module::require("ast_core"));
            addExternInline<DAS_BIND_FUN(das_invoke_code)>(*this, lib, "invoke_code",
                SideEffects::worstDefault, "das_invoke_code")
                    ->args({"code","arguments","cmres","context"})->unsafeOperation = true;
            addExternInline<DAS_BIND_FUN(das_instrument_jit)>(*this, lib, "instrument_jit",
                SideEffects::worstDefault, "das_instrument_jit")
                    ->args({"code","function","at", "context"})->unsafeOperation = true;
            addExternInline<DAS_BIND_FUN(das_remove_jit)>(*this, lib, "remove_jit",
                SideEffects::worstDefault, "das_remove_jit")
                    ->args({"function"})->unsafeOperation = true;
            addExternInline<DAS_BIND_FUN(das_has_jit_fastpath)>(*this, lib, "has_jit_fastpath",
                SideEffects::none, "das_has_jit_fastpath")
                    ->args({"function"});
            addExternInline<DAS_BIND_FUN(das_instrument_line_info)>(*this, lib, "instrument_line_info",
                SideEffects::worstDefault, "das_instrument_line_info")
                    ->args({"info","context","at"});
            addExternInline<DAS_BIND_FUN(das_get_jit_exception)>(*this, lib, "get_jit_exception",
                SideEffects::none, "das_get_jit_exception");
            addExternInline<DAS_BIND_FUN(das_get_jit_call_or_fastcall)>(*this, lib, "get_jit_call_or_fastcall",
                SideEffects::none, "das_get_jit_call_or_fastcall");
            addExternInline<DAS_BIND_FUN(das_get_jit_call_with_cmres)>(*this, lib, "get_jit_call_with_cmres",
                SideEffects::none, "das_get_jit_call_with_cmres");
            addExternInline<DAS_BIND_FUN(das_get_jit_invoke_block)>(*this, lib, "get_jit_invoke_block",
                SideEffects::none, "das_get_jit_invoke_block");
            addExternInline<DAS_BIND_FUN(das_get_jit_invoke_block_with_cmres)>(*this, lib, "get_jit_invoke_block_with_cmres",
                SideEffects::none, "das_get_jit_invoke_block_with_cmres");
            addExternInline<DAS_BIND_FUN(das_get_jit_string_builder)>(*this, lib, "get_jit_string_builder",
                SideEffects::none, "das_get_jit_string_builder");
            addExternInline<DAS_BIND_FUN(das_get_jit_string_builder_temp)>(*this, lib, "get_jit_string_builder_temp",
                SideEffects::none, "das_get_jit_string_builder_temp");
            addExternInline<DAS_BIND_FUN(das_get_jit_simnode_interop)>(*this, lib, "get_jit_simnode_interop",
                SideEffects::none, "das_get_jit_simnode_interop");
            addExternInline<DAS_BIND_FUN(das_get_jit_free_simnode_interop)>(*this, lib, "get_jit_free_simnode_interop",
                SideEffects::none, "das_get_jit_free_simnode_interop");
            addExternInline<DAS_BIND_FUN(das_get_jit_init_extern_function)>(*this, lib, "get_jit_init_extern_function",
                SideEffects::none, "get_jit_init_extern_function");
            addExternInline<DAS_BIND_FUN(das_get_jit_get_global_mnh)>(*this, lib, "get_jit_get_global_mnh",
                SideEffects::none, "das_get_jit_get_global_mnh");
            addExternInline<DAS_BIND_FUN(das_get_jit_get_shared_mnh)>(*this, lib, "get_jit_get_shared_mnh",
                SideEffects::none, "das_get_jit_get_shared_mnh");
            addExternInline<DAS_BIND_FUN(das_get_jit_get_handled_field_offset)>(*this, lib, "get_jit_get_handled_field_offset",
                SideEffects::none, "das_get_jit_get_handled_field_offset");
            addExternInline<DAS_BIND_FUN(das_get_jit_check_handled_type_size)>(*this, lib, "get_jit_check_handled_type_size",
                SideEffects::none, "das_get_jit_check_handled_type_size");
            addExternInline<DAS_BIND_FUN(das_get_jit_check_handled_field_offset)>(*this, lib, "get_jit_check_handled_field_offset",
                SideEffects::none, "das_get_jit_check_handled_field_offset");
            addExternInline<DAS_BIND_FUN(das_get_jit_handled_abi_check_report)>(*this, lib, "get_jit_handled_abi_check_report",
                SideEffects::none, "das_get_jit_handled_abi_check_report");
            addExternInline<DAS_BIND_FUN(das_get_jit_alloc_heap)>(*this, lib, "get_jit_alloc_heap",
                SideEffects::none, "das_get_jit_alloc_heap");
            addExternInline<DAS_BIND_FUN(das_get_jit_alloc_persistent)>(*this, lib, "get_jit_alloc_persistent",
                SideEffects::none, "das_get_jit_alloc_persistent");
            addExternInline<DAS_BIND_FUN(das_get_jit_free_heap)>(*this, lib, "get_jit_free_heap",
                SideEffects::none, "das_get_jit_free_heap");
            addExternInline<DAS_BIND_FUN(das_get_jit_free_persistent)>(*this, lib, "get_jit_free_persistent",
                SideEffects::none, "das_get_jit_free_persistent");
            addExternInline<DAS_BIND_FUN(das_get_jit_array_resize)>(*this, lib, "get_jit_array_resize",
                SideEffects::none, "das_get_jit_array_resize");
            addExternInline<DAS_BIND_FUN(das_get_jit_table_at)>(*this, lib, "get_jit_table_at",
                SideEffects::none, "das_get_jit_table_at");
            addExternInline<DAS_BIND_FUN(das_get_jit_table_erase)>(*this, lib, "get_jit_table_erase",
                SideEffects::none, "das_get_jit_table_erase");
            addExternInline<DAS_BIND_FUN(das_get_jit_table_find)>(*this, lib, "get_jit_table_find",
                SideEffects::none, "das_get_jit_table_find");
            addExternInline<DAS_BIND_FUN(das_get_jit_string_table_at_with_hash)>(*this, lib, "get_jit_string_table_at_with_hash",
                SideEffects::none, "das_get_jit_string_table_at_with_hash");
            addExternInline<DAS_BIND_FUN(das_get_jit_string_table_at_after_packed_miss)>(*this, lib, "get_jit_string_table_at_after_packed_miss",
                SideEffects::none, "das_get_jit_string_table_at_after_packed_miss");
            addExternInline<DAS_BIND_FUN(das_get_global_variable_offset)>(*this, lib, "get_global_variable_offset",
                SideEffects::none, "das_get_global_variable_offset");
            addExternInline<DAS_BIND_FUN(das_get_global_variable_mnh)>(*this, lib, "get_global_variable_mnh",
                SideEffects::none, "das_get_global_variable_mnh");
            addExternInline<DAS_BIND_FUN(das_get_global_variable_name)>(*this, lib, "get_global_variable_name",
                SideEffects::none, "das_get_global_variable_name");
            addExternInline<DAS_BIND_FUN(das_get_global_variable_debug_info)>(*this, lib, "get_global_variable_debug_info",
                SideEffects::none, "das_get_global_variable_debug_info");
            addExternInline<DAS_BIND_FUN(das_get_global_variable_shared)>(*this, lib, "get_global_variable_shared",
                SideEffects::none, "das_get_global_variable_shared");
            addExternInline<DAS_BIND_FUN(das_get_context_total_functions)>(*this, lib, "get_context_total_functions",
                SideEffects::none, "das_get_context_total_functions");
            addExternInline<DAS_BIND_FUN(das_get_lambda_capture_debug_info)>(*this, lib, "get_lambda_capture_debug_info",
                SideEffects::none, "das_get_lambda_capture_debug_info");
            addExternInline<DAS_BIND_FUN(das_get_context_globals_size)>(*this, lib, "get_context_globals_size",
                SideEffects::none, "das_get_context_globals_size");
            addExternInline<DAS_BIND_FUN(das_get_context_shared_size)>(*this, lib, "get_context_shared_size",
                SideEffects::none, "das_get_context_shared_size");
            addExternInline<DAS_BIND_FUN(das_get_jit_str_cat)>(*this, lib, "get_jit_str_cat",
                SideEffects::none, "das_get_jit_str_cat");
            addExternInline<DAS_BIND_FUN(das_get_jit_make_block)>(*this, lib, "get_jit_make_block",
                SideEffects::none, "das_get_jit_make_block");
            addExternInline<DAS_BIND_FUN(das_get_jit_try_recover)>(*this, lib, "get_jit_try_recover",
                SideEffects::none, "das_get_jit_try_recover");
            addExternInline<DAS_BIND_FUN(das_get_jit_ad_by_sid)>(*this, lib, "get_jit_ad_by_sid",
                SideEffects::none, "das_get_jit_ad_by_sid");
            addExternInline<DAS_BIND_FUN(das_get_jit_debug)>(*this, lib, "get_jit_debug",
                SideEffects::none, "das_get_jit_debug");
            addExternInline<DAS_BIND_FUN(das_get_jit_iterator_iterate)>(*this, lib, "get_jit_iterator_iterate",
                SideEffects::none, "das_get_jit_iterator_iterate");
            addExternInline<DAS_BIND_FUN(das_get_jit_iterator_delete)>(*this, lib, "get_jit_iterator_delete",
                SideEffects::none, "das_get_jit_iterator_delete");
            addExternInline<DAS_BIND_FUN(das_get_jit_iterator_first)>(*this, lib, "get_jit_iterator_first",
                SideEffects::none, "das_get_jit_iterator_first");
            addExternInline<DAS_BIND_FUN(das_get_jit_iterator_next)>(*this, lib, "get_jit_iterator_next",
                SideEffects::none, "das_get_jit_iterator_next");
            addExternInline<DAS_BIND_FUN(das_get_jit_iterator_close)>(*this, lib, "get_jit_iterator_close",
                SideEffects::none, "das_get_jit_iterator_close");
            addExternInline<DAS_BIND_FUN(das_get_jit_ast_typedecl)>(*this, lib, "get_jit_ast_typedecl",
                SideEffects::none, "das_get_jit_ast_typedecl");
            addExternInline<DAS_BIND_FUN(das_get_jit_new)>(*this, lib,  "get_jit_new",
                SideEffects::none, "das_get_jit_new")->args({"ann"});
            addExternInline<DAS_BIND_FUN(das_get_jit_delete)>(*this, lib,  "get_jit_delete",
                SideEffects::none, "das_get_jit_delete")->args({"ann"});
            addExternInline<DAS_BIND_FUN(das_get_jit_clone)>(*this, lib, "get_jit_clone",
                SideEffects::none, "das_get_jit_clone")->args({"ann"});
            addExternInline<DAS_BIND_FUN(das_get_jit_each)>(*this, lib, "get_jit_each",
                SideEffects::none, "das_get_jit_each")->args({"ann"});
            addExternInline<DAS_BIND_FUN(das_get_jit_at)>(*this, lib, "get_jit_at",
                SideEffects::none, "das_get_jit_at")->args({"ann","indexType"});
            addExternInline<DAS_BIND_FUN(das_get_jit_debug_enter)>(*this, lib,  "get_jit_debug_enter",
                SideEffects::none, "das_get_jit_debug_enter");
            addExternInline<DAS_BIND_FUN(das_get_jit_debug_exit)>(*this, lib,  "get_jit_debug_exit",
                SideEffects::none, "das_get_jit_debug_exit");
            addExternInline<DAS_BIND_FUN(das_get_jit_debug_line)>(*this, lib,  "get_jit_debug_line",
                SideEffects::none, "das_get_jit_debug_line");
            addExternInline<DAS_BIND_FUN(das_get_jit_initialize_fileinfo)>(*this, lib,  "get_jit_initialize_fileinfo",
                SideEffects::none, "das_get_jit_initialize_fileinfo");
            addExternInline<DAS_BIND_FUN(das_get_jit_free_fileinfo)>(*this, lib,  "get_jit_free_fileinfo",
                SideEffects::none, "das_get_jit_free_fileinfo");
            addExternInline<DAS_BIND_FUN(das_recreate_fileinfo_name)>(*this, lib,  "recreate_fileinfo_name",
                SideEffects::worstDefault, "das_recreate_fileinfo_name");
            addExternInline<DAS_BIND_FUN(loadDynamicLibrary)>(*this, lib,  "load_dynamic_library",
                SideEffects::worstDefault, "loadDynamicLibrary")
                    ->args({"filename"});
            addExternInline<DAS_BIND_FUN(getFunctionAddress)>(*this, lib,  "get_function_address",
                SideEffects::worstDefault, "getFunctionAddress")
                    ->args({"library","name"});
            addExternInline<DAS_BIND_FUN(closeLibrary)>(*this, lib,  "close_dynamic_library",
                SideEffects::worstDefault, "closeLibrary")
                    ->args({"library"});
            addExternInline<DAS_BIND_FUN(create_shared_library)>(*this, lib,  "create_shared_library",
                SideEffects::worstDefault, "create_shared_library")
                    ->args({"objFilePath","libraryName","dasLib","customLinker","extraLinkerArgs","isShared","linkWholeLib","debugInfo","context"});
            addExternInline<DAS_BIND_FUN(create_static_library)>(*this, lib,  "create_static_library",
                SideEffects::worstDefault, "create_static_library")
                    ->args({"objFilePath","libraryName","customTool","context"});
            addExternInline<DAS_BIND_FUN(jit_par_emit_begin)>(*this, lib,  "jit_par_emit_begin",
                SideEffects::worstDefault, "jit_par_emit_begin");
            addExternInline<DAS_BIND_FUN(jit_par_emit_add)>(*this, lib,  "jit_par_emit_add",
                SideEffects::worstDefault, "jit_par_emit_add")
                    ->args({"mod","tm","pbo","pipeline","objPath"});
            addExternInline<DAS_BIND_FUN(jit_par_emit_run)>(*this, lib,  "jit_par_emit_run",
                SideEffects::worstDefault, "jit_par_emit_run")
                    ->args({"threads","fileType","fnRunPasses","fnVerifyModule","fnEmitToFile","fnGetErrorMessage","fnDisposeErrorMessage","fnDisposeMessage","logTimes","context"});
            addExternInline<DAS_BIND_FUN(host_jit_triple)>(*this, lib, "host_jit_triple",
                SideEffects::none, "host_jit_triple");
            addExternInline<DAS_BIND_FUN(link_wasm)>(*this, lib,  "link_wasm",
                SideEffects::worstDefault, "link_wasm")
                    ->args({"objFilePath","wasmPath","runtimeLibPath","customEmcc","memory64","context"});
            addExternInline<DAS_BIND_FUN(jit_set_jit_state)>(*this, lib,  "set_jit_state",
                SideEffects::worstDefault, "jit_set_jit_state")
                    ->args({"context","shared_lib","llvm_ee","llvm_ctx"});
            addExternInline<DAS_BIND_FUN(jit_get_jit_state)>(*this, lib,  "get_jit_state",
                SideEffects::worstDefault, "jit_get_jit_state")
                    ->args({"block","context","at"});
            addConstant<uint32_t>(*this, "SIZE_OF_PROLOGUE", uint32_t(sizeof(Prologue)));
            addConstant<uint32_t>(*this, "SIZE_OF_SIMNODE_INTEROP", uint32_t(sizeof(SimNode_AotInteropBase)));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_EVAL_TOP", uint32_t(uint32_t(offsetof(Context, stack) + offsetof(StackAllocator, evalTop))));
            addConstant<uint32_t>(*this, "ARRAY_MAGIC", uint32_t(DAS_ARRAY_MAGIC));
            // array_lock / table_lock skip shared or hopeless containers; mask for define_lock / define_unlock (llvm_jit_runtime.das)
            Array lockExempt;
            lockExempt.flags = 0;
            lockExempt.shared = true;
            lockExempt.hopeless = true;
            addConstant<uint32_t>(*this, "ARRAY_LOCK_EXEMPT_FLAGS", lockExempt.flags);
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_GLOBALS", uint32_t(uint32_t(offsetof(Context, globals))));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_STOP_FLAGS", uint32_t(uint32_t(offsetof(Context, stopFlags))));
            addConstant<uint32_t>(*this, "SIMFUNCTION_OFFSET_OF_JIT_FUNCTION", uint32_t(uint32_t(offsetof(SimFunction, jitFunction))));
            addConstant<uint32_t>(*this, "CONTEXT_OFFSET_OF_SHARED", uint32_t(uint32_t(offsetof(Context, shared))));
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_jit.h\"\n";
            tw << "#include \"daScript/misc/sysos.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Jit,das);
