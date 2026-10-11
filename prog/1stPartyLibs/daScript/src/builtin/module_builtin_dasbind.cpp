#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_interop.h"

#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/simulate/aot_builtin_dasbind.h"

#include "daScript/misc/sysos.h"
#include "daScript/misc/env_cfg.h"

#include <cstring>
#include <memory>
#include <mutex>

#if DAS_BIND_EXTERNAL && defined(__aarch64__) && (defined(__APPLE__) || defined(__linux__))
#define DAS_BIND_ARM64_LAYOUT 1
#else
#define DAS_BIND_ARM64_LAYOUT 0
#endif
#if DAS_BIND_ARM64_LAYOUT && defined(__APPLE__)
#define DAS_BIND_ARM64_PACKED_STACK 1
#else
#define DAS_BIND_ARM64_PACKED_STACK 0
#endif

#if DAS_BIND_ARM64_LAYOUT
#if defined(__APPLE__)
#define DAS_ARM64_CALL_SYMBOL "_das_arm64_call"
#else
#define DAS_ARM64_CALL_SYMBOL "das_arm64_call"
#endif
// src/builtin/ARCHITECTURE.md#interpreter-extern-call
__asm__(
    ".text\n"
    ".p2align 2\n"
    ".globl " DAS_ARM64_CALL_SYMBOL "\n"
    DAS_ARM64_CALL_SYMBOL ":\n"
    "    stp x29, x30, [sp, #-32]!\n"
    "    stp x19, x20, [sp, #16]\n"
    "    mov x29, sp\n"
    "    mov x19, x0\n"
    "    mov x20, x5\n"
    "    sub sp, sp, x4\n"
    "    mov x9, #0\n"
    "1:  cmp x9, x4\n"
    "    b.hs 2f\n"
    "    ldr x10, [x3, x9]\n"
    "    str x10, [sp, x9]\n"
    "    add x9, x9, #8\n"
    "    b 1b\n"
    "2:  ldp d0, d1, [x2]\n"
    "    ldp d2, d3, [x2, #16]\n"
    "    ldp d4, d5, [x2, #32]\n"
    "    ldp d6, d7, [x2, #48]\n"
    "    ldp x6, x7, [x1, #48]\n"
    "    ldp x4, x5, [x1, #32]\n"
    "    ldp x2, x3, [x1, #16]\n"
    "    ldp x0, x1, [x1]\n"
    "    blr x19\n"
    "    str x0, [x20]\n"
    "    str d0, [x20, #8]\n"
    "    mov sp, x29\n"
    "    ldp x19, x20, [sp, #16]\n"
    "    ldp x29, x30, [sp], #32\n"
    "    ret\n"
);
struct Arm64Result { uint64_t x0, d0; };
extern "C" void das_arm64_call ( void * fn, const uint64_t * gpr, const uint64_t * fpr,
    const uint8_t * stack, uint64_t stackBytes, Arm64Result * out );
#endif

namespace das {

#if DAS_BIND_EXTERNAL

#ifdef _WIN32
    typedef uint64_t ( __stdcall * StdCallFunction )( ... );
    typedef uint64_t ( __cdecl * CdeclCallFunction )( ... );
    typedef uint64_t ( __stdcall * OpenglCallFunction )( ... );
#else
    // TODO: how to support this on MAC? Unix?
    // this is here so that it compiles on non-windows platforms
    typedef uint64_t (* StdCallFunction )( ... );
    typedef uint64_t (* CdeclCallFunction )( ... );
#endif

#if (defined(_MSC_VER) && !defined(_GAMING_XBOX) && !defined(_DURANGO)) || defined(__APPLE__)
    void * getFunction ( const char * fun, const char * lib ) {
        AllocTrackerInternalGuard atg;
        void * libhandle = nullptr;
        libhandle = getLibraryHandle(lib);
        if ( !libhandle ) {
            libhandle = loadDynamicLibrary(lib);
        }
        if ( !libhandle ) {
            return nullptr;
        }
        return getFunctionAddress(libhandle, fun);
    }

#if defined(_MSC_VER)
    typedef void * ( __stdcall * FN_wglGetProcAddress ) ( const char * );
    void * openGlGetFunctionAddress ( const char * name ) {
        auto _wglGetProcAddress = (FN_wglGetProcAddress) getFunction("wglGetProcAddress","Opengl32");
        if  ( _wglGetProcAddress!=nullptr ) {
            if ( auto fn = _wglGetProcAddress(name) ) {
                return fn;
            }
        }
        return getFunction(name,"Opengl32");
    }
#elif defined(__APPLE__)
    void * openGlGetFunctionAddress ( const char * name ) {
        auto libName = "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL";
        void * libhandle = nullptr;
        libhandle = getLibraryHandle(libName);
        return getFunctionAddress(libhandle, name);
    }
#else
    void * openGlGetFunctionAddress ( const char * name ) {
        return nullptr;
    }
#endif
#elif defined(__linux__) || defined __HAIKU__
    void * openGlGetFunctionAddress ( const char * name ) {
        auto libName = "libGL.so";
        void * libhandle = nullptr;
        libhandle = getLibraryHandle(libName);
        return getFunctionAddress(libhandle, name);
    }
#else
    void * openGlGetFunctionAddress ( const char * name ) {
        return nullptr;
    }
#endif

    typedef vec4f ( * FastCallWrapper ) ( void * fn, vec4f * args );

    struct FastCallExtraWrapper {
        int nargs, res, perm;
        FastCallWrapper wrapper;
    };

    enum { RES_INT = 0, RES_FLOAT = 1 };

    static bool isFloatClass ( const TypeDeclPtr & type ) {
        return !type->isRef() && (type->baseType==Type::tFloat || type->baseType==Type::tDouble);
    }

    __forceinline vec4f   Rx ( int64_t x ) { return v_cast_vec4f(v_splatsi64(x)); }

    #define AX(i)   (*(uint64_t *)(args+(i)))
    #define AD(i)   (*(double *)(args+(i)))

#ifdef _WIN32
#include "win_x86_64_wrapper.inc"
#else
#include "systemV_64_wrapper.inc"
#include "systemV_64_extra_wrapper.inc"

FastCallWrapper getExtraWrapper ( int nargs, int res, int perm ) {
    for ( auto & t : fastcall64_extra_table ) {
        if ( t.nargs==nargs && t.res==res && t.perm==perm ) {
            return t.wrapper;
        }
    }
    return nullptr;
}

#endif

    #undef  AX
    #undef  AD

    // src/builtin/ARCHITECTURE.md#interpreter-extern-call
    struct Arm64Layout {
        struct Slot { uint8_t argIndex, bytes; uint16_t offset; };
        uint8_t gprArg[8] = {}, fprArg[8] = {};
        int ngpr = 0, nfpr = 0;
        vector<Slot> stack;
        uint32_t stackBytes = 0;
        bool fpResult = false;
    };

#if DAS_BIND_ARM64_LAYOUT
    static uint8_t arm64StackArgumentBytes ( const TypeDeclPtr & type ) {
        if ( !DAS_BIND_ARM64_PACKED_STACK || type->isRef() ) return 8;
        auto size = type->getBaseSizeOf();
        return (size==1 || size==2 || size==4) ? uint8_t(size) : 8;
    }

    static const Arm64Layout * computeArm64Layout ( Function * fun ) {
        if ( fun->arguments.size()<=6 ) return nullptr;
        auto layout = make_unique<Arm64Layout>();
        uint32_t offset = 0;
        for ( int a=0, as=int(fun->arguments.size()); a<as; ++a ) {
            const auto & type = fun->arguments[a]->type;
            bool fp = isFloatClass(type);
            int & count = fp ? layout->nfpr : layout->ngpr;
            if ( count<8 ) {
                (fp ? layout->fprArg : layout->gprArg)[count++] = uint8_t(a);
            } else {
                uint8_t bytes = arm64StackArgumentBytes(type);
                offset = (offset + bytes - 1) & ~uint32_t(bytes - 1);
                layout->stack.push_back({uint8_t(a), bytes, uint16_t(offset)});
                offset += bytes;
            }
        }
        layout->stackBytes = (offset + 15) & ~15u;
        layout->fpResult = isFloatClass(fun->result);
        static mutex layoutsMutex;
        static vector<unique_ptr<Arm64Layout>> layouts;
        lock_guard<mutex> guard(layoutsMutex);
        layouts.push_back(das::move(layout));
        return layouts.back().get();
    }

    static vec4f arm64Call ( void * fn, vec4f * args, const Arm64Layout * layout ) {
        uint64_t gpr[8] = {}, fpr[8] = {};
        Arm64Result out = {};
        for ( int i=0; i!=layout->ngpr; ++i ) memcpy(gpr+i, args+layout->gprArg[i], 8);
        for ( int i=0; i!=layout->nfpr; ++i ) memcpy(fpr+i, args+layout->fprArg[i], 8);
        alignas(16) uint8_t stack[DAS_MAX_FUNCTION_ARGUMENTS*8 + 16] = {};
        for ( const auto & slot : layout->stack ) memcpy(stack+slot.offset, args+slot.argIndex, slot.bytes);
        das_arm64_call(fn, gpr, fpr, stack, layout->stackBytes, &out);
        uint64_t raw = layout->fpResult ? out.d0 : out.x0;
        int64_t bits;
        memcpy(&bits, &raw, sizeof(bits));
        return v_cast_vec4f(v_splatsi64(bits));
    }
#else
    static const Arm64Layout * computeArm64Layout ( Function * ) {
        return nullptr;
    }
#endif

    struct BoundFunction {
        void *  fun;
        string  name;
        string  library;
    };

    das_hash_map<uint64_t,BoundFunction>    g_dasBind;
    mutex                                   g_dasBindMutex;

    uint64_t lateBind ( const string & name, const string & library, void * fun ) {
        lock_guard<mutex> guard(g_dasBindMutex);
        uint64_t hn = hash_blockz64((const uint8_t *)name.c_str());
        uint64_t hl = hash_blockz64((const uint8_t *)library.c_str());
        uint64_t hf = hn ^ hl;
        // printf("%llx %llx %llx %s : %s -> %p\n", hn, hl, hf, name.c_str(), library.c_str(), fun);
        auto & bf = g_dasBind[hf];
        if ( !bf.name.empty() ) {
            DAS_VERIFY(bf.name==name && bf.library==library);
        } else {
            bf.name = name;
            bf.library = library;
            bf.fun = fun;
        }
        return hf;
    }

    template <typename TT>
    void withBind ( uint64_t index, TT && block ) {
        lock_guard<mutex> guard(g_dasBindMutex);
        auto it = g_dasBind.find(index);
        if ( it!=g_dasBind.end() ) {
            block(&it->second);
        } else {
            block(nullptr);
        }
    }

    das_hash_map<string,void *>    g_dasBindLib;
    mutex                          g_dasBindLibMutex;

#if defined(_WIN32)
    #define DAS_SHARED_LIBRARY_EXTENSION ".dll"
#elif defined(__APPLE__)
    #define DAS_SHARED_LIBRARY_EXTENSION ".dylib"
#else
    #define DAS_SHARED_LIBRARY_EXTENSION ".so"
#endif

    enum class ApiType {
        api_unknown,
        api_cdecl,
        api_stdcall,
        api_opengl
    };

    struct ExternBindArgs {
        string fn_name;
        string library;
        ApiType api = ApiType::api_unknown;
        bool late = false;
    };

    // DAS_DLL_PATH: extra directories to look in, for any host - not only the CLI, which also
    // takes -dll-path. Parsed once; ';'-separated on Windows, ':' elsewhere.
    static const vector<string> & dasDllSearchPathsFromEnv () {
        static vector<string> paths = [] {
            vector<string> res;
            if ( auto env = das_getenv("DAS_DLL_PATH") ) {
#if defined(_WIN32)
                const char sep = ';';
#else
                const char sep = ':';
#endif
                string all(env);
                size_t pos = 0;
                while ( pos <= all.length() ) {
                    auto next = all.find(sep, pos);
                    if ( next==string::npos ) next = all.length();
                    if ( next > pos ) res.push_back(all.substr(pos, next-pos));
                    pos = next + 1;
                }
            }
            return res;
        }();
        return paths;
    }

    static void * searchDynamicLibrary ( const string & library ) {
        void * libhandle = getLibraryHandle(library.c_str());
        if ( !libhandle ) {
            libhandle = loadDynamicLibrary(library.c_str());
            if (!libhandle) {
                if ( *daScriptEnvironment::bound && (*daScriptEnvironment::bound)->g_Program ) {
                    for ( auto & root : (*daScriptEnvironment::bound)->g_Program->policies.dll_search_paths ) {
                        libhandle = loadDynamicLibrary((root + "/" + library).c_str());
                        if ( libhandle ) break;
                    }
                    /* // we figure this out later
                    if ( !libhandle ) {
                        libhandle = loadDynamicLibrary(((*daScriptEnvironment::bound)->g_Program->policies.jit_path_to_shared_lib + "/" + library).c_str());
                    }
                    */
                }
                if ( !libhandle ) {
                    for ( auto & root : dasDllSearchPathsFromEnv() ) {
                        libhandle = loadDynamicLibrary((root + "/" + library).c_str());
                        if ( libhandle ) break;
                    }
                }
                if ( !libhandle ) {
                    libhandle = loadDynamicLibrary((getDasRoot() + "/lib/" + library).c_str());
                }
            }
        }
        return libhandle;
    }

    static void * bindDynamicLibrary ( const string & library ) {
        lock_guard<mutex> guard(g_dasBindLibMutex);
        // LLVM.dll pulls in libz3, whose static initializer leaks 40 bytes nothing here can free.
        AllocTrackerInternalGuard atg;
        auto it = g_dasBindLib.find(library);
        if ( it!=g_dasBindLib.end() ) {
            return it->second;
        } else {
            void * libhandle = searchDynamicLibrary(library);
            if ( !libhandle ) {
                // an [extern] may name its library without one, so every platform can spell the
                // same binding: add this one's extension and walk the same search again
                string ext = DAS_SHARED_LIBRARY_EXTENSION;
                if ( library.length()<ext.length() ||
                     library.compare(library.length()-ext.length(), ext.length(), ext)!=0 ) {
                    libhandle = searchDynamicLibrary(library + ext);
                }
            }
            g_dasBindLib[library] = libhandle;
            return libhandle;
        }
    }

    static void* getDllAddress(const string &lib, const string &func, bool is_opengl, string &err) {
        void *fnptr;
        if (is_opengl) {
            fnptr = openGlGetFunctionAddress(func.c_str());
            if ( !fnptr ) err = "failed to bind " + func;
        } else {
            void * libhandle = bindDynamicLibrary(lib);
            if ( !libhandle ) {
                auto dlErr = getDynamicLibraryError();
                err = "can't load library " + lib + (dlErr.empty() ? string() : (" (" + dlErr + ")"));
                return nullptr;
            }
            fnptr = getFunctionAddress(libhandle, func.c_str());
            if ( !fnptr ) err = "can't find function " + func + " in library " + lib;
        }
        return fnptr;
    }

    struct SimNode_DasBindCall : SimNode_ExtFuncCallBase {
        SimNode_DasBindCall ( const LineInfo & a, const char * fnName, uint64_t hc, FastCallWrapper wrp, void * fun, ApiType api_,
                              const Arm64Layout * layout_ )
            : SimNode_ExtFuncCallBase(a, fnName), code(hc), wrapper(wrp), fnptr(fun), api(api_), layout(layout_) {
        }
        void bind ( Context & context ) {
            string crash_and_burn;
            withBind(code,[&](BoundFunction * bf){
                if ( bf ) {
                    if ( !bf->fun ) {
                        bf->fun = getDllAddress(bf->library, bf->name, api == ApiType::api_opengl, crash_and_burn);
                    }
                    fnptr = bf->fun;
                } else {
                    crash_and_burn = "internal error. missing BoundFunction";
                }
            });
            if ( !crash_and_burn.empty() ) {
                char message[8192];   // src/builtin/ARCHITECTURE.md#message-crosses-panic-jump
                snprintf(message, sizeof(message), "%s", crash_and_burn.c_str());
                string().swap(crash_and_burn);
                context.throw_error_at(debugInfo, "%s", message);
            }
        }
        DAS_EVAL_ABI vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            if ( !fnptr ) {
                bind(context);
            }
            vec4f argValues[DAS_MAX_FUNCTION_ARGUMENTS];
            evalArgs(context, argValues);
#if DAS_BIND_ARM64_LAYOUT
            if ( layout ) return arm64Call(fnptr, argValues, layout);
#endif
            return wrapper(fnptr, argValues);
        }
        uint64_t code = 0;
        FastCallWrapper wrapper = nullptr;
        void * fnptr = nullptr;
        ApiType api = ApiType::api_unknown;
        const Arm64Layout * layout = nullptr;
    };

    FastCallWrapper getWrapper ( Function * fun, int nReg ) {
        int args = isFloatClass(fun->result) ? (1<<nReg) : 0;
        for ( int a=0, as=int(fun->arguments.size()); a<as; ++a ) {
            if ( a==nReg ) break;
            if ( isFloatClass(fun->arguments[a]->type) ) {
                args |= (1<<a);
            }
        }
        args += (2<<nReg) * int(fun->arguments.size());
        return fastcall64_table[args];
    }

    struct DasBindFunction : BuiltInFunction {
        void * dllAddress;
        FastCallWrapper wrapper;
        const Arm64Layout * layout;
        bool isLate;
        ApiType api;
        uint64_t hc;
        DasBindFunction ( const string & dasName, uint64_t _hc, void * addr, ExternBindArgs params, FastCallWrapper wrp,
                          const Arm64Layout * layout_ )
            : BuiltInFunction(dasName.c_str(), dasName.c_str())
            , dllAddress(addr), wrapper(wrp), layout(layout_) {
            callBased = true;
            isLate = params.late;
            api = params.api;
            hc = _hc;
        }
        void * getBuiltinAddress() const override {
            return dllAddress;
        }
        SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            const char * fnName = context.code->allocateName(this->name);
            return context.code->makeNode<SimNode_DasBindCall>(at, fnName, hc, wrapper, dllAddress, api, layout);
        }
    };

    static string mangleFunction(const string &fun, const string &lib, ApiType api) {
        return "__dasbind__" + fun + "@@" + lib + "#" + to_string(int(api));
    }

    struct DemangledFunction {
        string symbol;
        string library;
        ApiType api = ApiType::api_unknown;
    };

    static DemangledFunction demangleFunction(const char * mangledName) {
        const char * prefix = "@dasbind::__dasbind__";
        if ( strncmp(mangledName, prefix, 21) != 0 ) return {};
        const char * symbolStart = mangledName + 21;
        const char * sep = strstr(symbolStart, "@@");
        if ( !sep ) return {};
        string symbol(symbolStart, sep - symbolStart);
        const char * libStart = sep + 2;
        const char * hashPos = strchr(libStart, '#');
        if ( !hashPos ) return {};
        string library(libStart, hashPos - libStart);
        ApiType api = ApiType(atoi(hashPos + 1));
        return { symbol, library, api };
    }


    pair<bool, ExternBindArgs> parseExternArgs ( const AnnotationArgumentList & args, string &err ) {
        ExternBindArgs result;
        string platform_library;
        for ( auto & arg : args ) {
            if ( arg.name=="name" && arg.type==Type::tString ) {
                result.fn_name = arg.sValue;
            } else if ( arg.name=="library" && arg.type==Type::tString ) {
                result.library = arg.sValue;
            } else if ( arg.name=="WINAPI" || arg.name=="winapi" || arg.name=="stdcall" || arg.name=="__stdcall" || arg.name=="STDCALL" ) {
                result.api = ApiType::api_stdcall;
            } else if ( arg.name=="CDECL" || arg.name=="cdecl" || arg.name=="__cdecl" ) {
                result.api = ApiType::api_cdecl;
            } else if ( arg.name=="opengl" || arg.name=="OPENGL" ) {
                result.api = ApiType::api_opengl;
            } else if ( arg.name=="late" ) {
                result.late = true;
            }
#ifdef _WIN32
            else if ( arg.name=="windows_library" && arg.type==Type::tString ) {
                platform_library = arg.sValue;
            }
#elif defined(__APPLE__)
            else if ( arg.name=="macos_library" && arg.type==Type::tString ) {
                platform_library = arg.sValue;
            }
#elif defined(__linux__) || defined __HAIKU__
            else if ( arg.name=="linux_library" && arg.type==Type::tString ) {
                platform_library = arg.sValue;
            }
#endif
        }
        if ( !platform_library.empty() ) {
            result.library = platform_library;
        }
        if ( result.fn_name.empty() ) {
            err = "missing name";
            return {false, {}};
        }
        if ( result.api==ApiType::api_unknown ) {
            err = "need to specify calling convention (like stdcall (aka WINAPI),cdecl,etc)";
            return {false, {}};
        }
        return {true, result};
    }

    FastCallWrapper computeWrapper ( Function * fun ) {
#ifdef _WIN32
        return getWrapper(fun, 4);
#else
        if ( fun->arguments.size()>6 ) {
            int nargs = int(fun->arguments.size());
            int res = isFloatClass(fun->result) ? RES_FLOAT : RES_INT;
            int perm = 0;
            for ( size_t ai=0, ais=fun->arguments.size(); ai!=ais; ++ai ) {
                if ( isFloatClass(fun->arguments[ai]->type) ) {
                    perm |= (1<<int(ai));
                }
            }
            if ( perm>=(1<<6) ) {
                return getExtraWrapper(nargs, res, perm);
            }
        }
        return getWrapper(fun, 6);
#endif
    }

#endif

    struct ExternFunctionAnnotation : FunctionAnnotation {
        ExternFunctionAnnotation() : FunctionAnnotation("extern") { }
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string & err) override {
            err = "not supported for block";
            return false;
        }
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
#if !DAS_BIND_EXTERNAL
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string & err )  override {
            err = "daslang is configured with extern functions disabled";
            return false;
        }
#else

        bool verifyCallCorrect( const FunctionPtr & fun, const AnnotationArgumentList & /*args*/, string & err ) {
            if ( fun->arguments.size() >= MAX_WRAPPER_ARGUMENTS ) {
                err = "function has too many arguments for the current wrapper config";
                return false;
            }
            // check vector type arguments and result
            for ( const auto & arg : fun->arguments ) {
                if ( !arg->type->isRef() && arg->type->isVectorType() ) {
                    err += "argument " + arg->name + " is vector type, which is currently not supported by specified binding";
                    return false;
                }
            }
            if ( fun->result && !fun->result->isRef() && fun->result->isVectorType() ) {
                err = "function returns vector type, which is currently not supported by the specified binding";
                return false;
            }
#ifndef _WIN32
            if ( fun->arguments.size()>6 && !DAS_BIND_ARM64_LAYOUT ) {
                int perm=0;
                int nargs = int(fun->arguments.size());
                int res = isFloatClass(fun->result) ? RES_FLOAT : RES_INT;
                for ( size_t ai=0, ais=fun->arguments.size(); ai!=ais; ++ai ) {
                    if ( isFloatClass(fun->arguments[ai]->type) ) {
                        perm |= (1<<int(ai));
                    }
                }
                if ( perm>=(1<<6) ) {
                    auto wrp = getExtraWrapper(nargs,res,perm);
                    if ( !wrp ) {
                        string argText;
                        for ( int i=0; i!=MAX_WRAPPER_ARGUMENTS; ++i ) {
                            if ( perm & (1<<i) ) {
                                argText += "|ARG" + to_string(i) + "_D";
                            }
                        }
                        err = "Missing custom SystemV wrapper\n"
                            "nargs=" + to_string(nargs) + ", res=" + to_string(res) + ", perm=0" + argText + ";";
                        return false;
                    }
                }
            }
#endif
            return true;
        }

        virtual bool apply ( const FunctionPtr & fun, ModuleGroup &, AnnotationArgumentList & args, string & err )  override {
            if (!verifyCallCorrect(fun, args, err)) {
                return false;
            }
            fun->stub = true;
            fun->userScenario = true;
            fun->noAot = true;         // TODO: generate custom C++ to invoke the call directly
            fun->requestNoJit = true;  // the body is a placeholder - transformCall rewrites the call sites
            // parse annotation arguments
            auto [is_ok, ba] = parseExternArgs(args, err);
            if ( !is_ok ) {
                return false;
            }
            // src/ast/ARCHITECTURE.md#module-cache-read
            if ( auto program = daScriptEnvironment::getBound()->g_Program ) {
                if ( program->thisModule ) program->thisModule->noteRegistrationAtCompile();
            }
            return !registerProxy(fun, ba, err).empty();
        }

        // the proxy outlives the registrar's program: its types are clones, with no location
        static TypeDeclPtr proxyType ( const TypeDeclPtr & type, const LineInfo & bindAt ) {
            if ( !type ) return nullptr;
            auto clone = new TypeDecl(*type);
            clone->at = bindAt;
            clone->firstType = proxyType(type->firstType, bindAt);
            clone->secondType = proxyType(type->secondType, bindAt);
            for ( auto & argType : clone->argTypes ) {
                argType = proxyType(argType, bindAt);
            }
            return clone;
        }

        // registers the proxy a call to `fun` is retargeted to; the bind name, empty on failure
        string registerProxy ( Function * fun, const ExternBindArgs & ba, string & err ) {
            // resolve DLL and create a BuiltInFunction proxy for JIT
            void * funptr = nullptr;
            string bindName = mangleFunction(ba.fn_name, ba.library, ba.api);
            if ( !ba.late && ba.api!=ApiType::api_opengl ) {
                funptr = getDllAddress(ba.library, ba.fn_name, ba.api == ApiType::api_opengl, err);
                if ( !funptr ) return string();
            }
            auto wrp = computeWrapper(fun);
            uint64_t code = lateBind(ba.fn_name, ba.library, funptr);
            auto bif = new DasBindFunction(bindName, code, funptr, ba, wrp, computeArm64Layout(fun));
            const LineInfo & bindAt = cppBindingLineInfo(bindName.c_str());
            bif->at = bindAt;
            bif->result = proxyType(fun->result, bindAt);
            for ( auto & a : fun->arguments ) {
                auto newArg = new Variable(bindAt);
                newArg->name = a->name;
                newArg->type = proxyType(a->type, bindAt);
                bif->arguments.push_back(newArg);
            }
            bif->noAot = true;
            bif->userScenario = true;
            bif->sideEffectFlags = fun->sideEffectFlags | uint32_t(SideEffects::accessExternal);
            // and collect it
            bif->gc_collect(module->module_gc_root.get(), gc_root::gc_get_active_root());
            // and now try to add or replace the function in the module
            if ( !module->addFunction(bif, true) ) {
                module->replaceFunction(bif);
            }
            return bindName;
        }

        static bool sameSignature ( Function * proxy, Function * fun ) {
            if ( proxy->arguments.size() != fun->arguments.size() ) return false;
            if ( !proxy->result->isSameType(*fun->result, RefMatters::yes, ConstMatters::no, TemporaryMatters::no) ) return false;
            for ( size_t i=0, is=fun->arguments.size(); i!=is; ++i ) {
                if ( !proxy->arguments[i]->type->isSameType(*fun->arguments[i]->type, RefMatters::yes, ConstMatters::no, TemporaryMatters::no) ) return false;
            }
            return true;
        }

        // the bind name from the call target's own [extern] declaration (a cache-served
        // registrar never passed apply here), registering the proxy when the module lacks it;
        // empty for a non-extern, empty with `err` set when the bind fails
        string bindNameOf ( Function * fun, string & err ) {
            for ( auto & decl : fun->annotations ) {
                if ( decl->annotation != this ) continue;
                auto [is_ok, ba] = parseExternArgs(decl->arguments, err);
                if ( !is_ok ) return string();
                string bindName = mangleFunction(ba.fn_name, ba.library, ba.api);
                // one bind name per native symbol, one proxy per das signature
                if ( auto proxies = module->functionsByName.find(hash64z(bindName.c_str())) ) {
                    for ( auto * proxy : proxies->second ) {
                        if ( sameSignature(proxy, fun) ) return bindName;
                    }
                }
                if ( !verifyCallCorrect(fun, decl->arguments, err) ) return string();
                return registerProxy(fun, ba, err);
            }
            return string();
        }

        static bool needWrapArg(ExpressionPtr arg) {
            if ( arg->type->isString() ) {
                if ( arg->rtti_isCallFunc() ) {
                    auto pCall = static_cast<ExprCallFunc*>(arg);
                    if ( pCall->func->name!="safe_pass_string") {
                        return true;
                    }
                } else if ( strcmp(arg->__rtti,"ExprConstString")==0 ) {
                    auto str = static_cast<ExprConstString*>(arg);
                    if ( str->getValue().empty()) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
            return false;
        }

        virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & err ) override {
            if ( !call->func ) return nullptr;
            string bindName = bindNameOf(call->func, err);
            if ( !err.empty() ) return nullptr;    // the inferer reports it
            bool hasBindFunction = !bindName.empty();
            // check if any string args need wrapping
            auto needToTransform = any_of(call->arguments.begin(), call->arguments.end(), [](ExpressionPtr arg) {
                return needWrapArg(arg);
            });
            if ( !needToTransform && !hasBindFunction ) return nullptr;
            // clone call, optionally retargeting to the DasBindFunction
            ExpressionPtr newCallExpr = nullptr;
            DAS_ASSERTF(hasBindFunction, "an [extern] call's bind name must resolve, %s has none.", call->func->name.c_str());
            newCallExpr = new ExprCall(call->at, bindName);
            for ( auto & arg : call->arguments ) {
                static_cast<ExprCall*>(newCallExpr)->arguments.push_back(arg->clone());
            }
            // wrap string arguments with safe_pass_string
            auto & newArgs = static_cast<ExprCallFunc*>(newCallExpr)->arguments;
            for ( auto & arg : newArgs ) {
                if ( needWrapArg(arg) ) {
                    auto wrapCall = new ExprCall(arg->at,"safe_pass_string");
                    wrapCall->arguments.push_back(arg->clone());
                    arg = wrapCall;
                }
            }
            return newCallExpr;
        }
        // transformCall rewrites every CALL to the bound extern, so the body is reached only
        // without a call site to rewrite - an address-taken extern. Simulating still has to
        // SUCCEED (-aot-macros marks and simulates every function), but the body must refuse:
        // an empty one answers uninitialized stack.
        virtual SimNode * simulate ( Context * context, Function * fun, const AnnotationArgumentList &, string & ) override {
            if ( is_in_completion() ) return nullptr;
            struct SimNode_NoIndirect : SimNode {
                SimNode_NoIndirect ( const LineInfo & at ) : SimNode(at) {}
                virtual vec4f eval ( Context & ctx ) override {
                    ctx.throw_error_at(debugInfo, "a dasbind extern has no body to call - it is reached "
                        "through its call site, which transformCall rewrites, not through a function pointer");
                    return v_zero();
                }
            };
            return context->code->makeNode<SimNode_NoIndirect>(fun->at);
        }
#endif
    };

    char * safe_pass_string ( const char * str ) {
        return (char *)(str ? str : "");
    }

    // Resolver for JIT exe mode: parses __dasbind__<symbol>@@<library> from mangled name,
    // loads DLL and returns function address.
    void * dasbind_resolve ( const char * mangledName ) {
#if DAS_BIND_EXTERNAL
        auto dm = demangleFunction(mangledName);
        if ( dm.symbol.empty() ) return nullptr;
        string err;
        return getDllAddress(dm.library, dm.symbol, dm.api == ApiType::api_opengl, err);
#else
        (void)mangledName;
        return nullptr;
#endif
    }

    // The library handle dasbind's own extern machinery resolves (already-loaded check +
    // dll_search_paths + dasroot/lib fallbacks) — so das code needing raw entry points from a
    // bound library (the --jit-split-modules parallel emit) shares ONE resolution truth.
    void * dasbind_get_library ( const char * library ) {
#if DAS_BIND_EXTERNAL
        if ( !library || !*library ) return nullptr;
        return bindDynamicLibrary(library);
#else
        (void)library;
        return nullptr;
#endif
    }

    class Module_DASBIND : public Module {
    public:
        Module_DASBIND() : Module("dasbind") {
            DAS_PROFILE_SECTION("Module_DASBIND");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addAnnotation(new ExternFunctionAnnotation());
            addExtern<DAS_BIND_FUN(safe_pass_string)>(*this, lib, "safe_pass_string",
                SideEffects::accessExternal, "safe_pass_string")
                    ->args({"string"});
            addExtern<DAS_BIND_FUN(dasbind_resolve)>(*this, lib, "__dasbind_resolve",
                SideEffects::accessExternal, "dasbind_resolve")
                    ->args({"mangledName"});
            addExtern<DAS_BIND_FUN(dasbind_get_library)>(*this, lib, "__dasbind_get_library",
                SideEffects::accessExternal, "dasbind_get_library")
                    ->args({"library"});
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_dasbind.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_DASBIND,das);
