#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_interop.h"

#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/simulate/aot_builtin_dasbind.h"

#include "daScript/misc/sysos.h"

#include <mutex>

namespace das {

#if DAS_BIND_EXTERNAL

#ifdef _MSC_VER
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

    __forceinline vec4f   Rx ( int64_t x ) { return v_cast_vec4f(v_splatsi64(x)); }

    #define AX(i)   (*(uint64_t *)(args+(i)))
    #define AD(i)   (*(double *)(args+(i)))

#ifdef _MSC_VER
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

    static void * bindDynamicLibrary ( const string & library ) {
        lock_guard<mutex> guard(g_dasBindLibMutex);
        auto it = g_dasBindLib.find(library);
        if ( it!=g_dasBindLib.end() ) {
            return it->second;
        } else {
            void * libhandle = nullptr;
            libhandle = getLibraryHandle(library.c_str());
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
                        libhandle = loadDynamicLibrary((getDasRoot() + "/lib/" + library).c_str());
                    }
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
                err = "can't load library " + lib;
                return nullptr;
            }
            fnptr = getFunctionAddress(libhandle, func.c_str());
            if ( !fnptr ) err = "can't find function " + func + " in library " + lib;
        }
        return fnptr;
    }

    struct SimNode_DasBindCall : SimNode_ExtFuncCallBase {
        SimNode_DasBindCall ( const LineInfo & a, const char * fnName, uint64_t hc, FastCallWrapper wrp, void * fun, ApiType api_ )
            : SimNode_ExtFuncCallBase(a, fnName), code(hc), wrapper(wrp), fnptr(fun), api(api_) {
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
            if ( !crash_and_burn.empty() ) context.throw_error_at(debugInfo, "%s",crash_and_burn.c_str());
        }
        DAS_EVAL_ABI vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            if ( !fnptr ) {
                bind(context);
            }
            vec4f argValues[DAS_MAX_FUNCTION_ARGUMENTS];
            evalArgs(context, argValues);
            return wrapper(fnptr, argValues);
        }
        uint64_t code = 0;
        FastCallWrapper wrapper = nullptr;
        void * fnptr = nullptr;
        ApiType api = ApiType::api_unknown;
    };

    FastCallWrapper getWrapper ( Function * fun, int nReg ) {
        int args = ( fun->result->baseType==Type::tFloat || fun->result->baseType==Type::tDouble ) ? (1<<nReg) : 0;
        for ( int a=0, as=int(fun->arguments.size()); a<as; ++a ) {
            if ( a==4 ) break;
            auto tp = fun->arguments[a]->type->baseType;
            if ( tp==Type::tFloat || tp==Type::tDouble ) {
                args |= (1<<a);
            }
        }
        args += (2<<nReg) * int(fun->arguments.size());
        return fastcall64_table[args];
    }

    struct DasBindFunction : BuiltInFunction {
        void * dllAddress;
        FastCallWrapper wrapper;
        bool isLate;
        ApiType api;
        uint64_t hc;
        DasBindFunction ( const string & dasName, uint64_t _hc, void * addr, ExternBindArgs params, FastCallWrapper wrp )
            : BuiltInFunction(dasName.c_str(), dasName.c_str())
            , dllAddress(addr), wrapper(wrp) {
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
            return context.code->makeNode<SimNode_DasBindCall>(at, fnName, hc, wrapper, dllAddress, api);
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
#ifdef _MSC_VER
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
#ifdef _MSC_VER
        return getWrapper(fun, 4);
#else
        if ( fun->arguments.size()>6 ) {
            int nargs = int(fun->arguments.size());
            int res = ( fun->result->baseType==Type::tFloat || fun->result->baseType==Type::tDouble ) ? 0 : 1;
            int perm = 0;
            for ( size_t ai=0, ais=fun->arguments.size(); ai!=ais; ++ai ) {
                const auto & a = fun->arguments[ai];
                if ( a->type->isSimpleType(Type::tFloat) || a->type->isSimpleType(Type::tDouble) ) {
                    perm |= (1<<int(ai));
                }
            }
            if ( perm>=(1<<7) ) {
                return getExtraWrapper(nargs, res, perm);
            }
        }
        return getWrapper(fun, 6);
#endif
    }

#endif

    struct ExternFunctionAnnotation : FunctionAnnotation {
        ExternFunctionAnnotation() : FunctionAnnotation("extern") { }
        das_hash_map<const Function *, string> transformMap;
        virtual bool apply(ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & err) override {
            err = "not supported for block";
            return false;
        }
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &,const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
#if !DAS_BIND_EXTERNAL
        virtual bool apply ( const FunctionPtr & fun, ModuleGroup &, const AnnotationArgumentList &, string & err )  override {
            err = "daslang is configured with extern functions disabled";
            return false;
        }
#else

        bool verifyCallCorrect( const FunctionPtr & fun, const AnnotationArgumentList & args, string & err ) {
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
#ifndef _MSC_VER
            if ( fun->arguments.size()>6 ) {
                int perm=0;
                int nargs = int(fun->arguments.size());
                int res = ( fun->result->baseType==Type::tFloat || fun->result->baseType==Type::tDouble ) ? 0 : 1;
                for ( size_t ai=0, ais=fun->arguments.size(); ai!=ais; ++ai ) {
                    const auto & arg = fun->arguments[ai];
                    if ( arg->type->isSimpleType(Type::tFloat) || arg->type->isSimpleType(Type::tDouble) ) {
                        perm |= (1<<int(ai));
                    }
                }
                if ( perm>=(1<<7) ) {
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

        virtual bool apply ( const FunctionPtr & fun, ModuleGroup &, const AnnotationArgumentList & args, string & err )  override {
            if (is_in_completion()) {
                return false;
            }
            if (!verifyCallCorrect(fun, args, err)) {
                return false;
            }
            fun->stub = true;
            fun->userScenario = true;
            fun->noAot = true;         // TODO: generate custom C++ to invoke the call directly
            // parse annotation arguments
            auto [is_ok, ba] = parseExternArgs(args, err);
            if ( !is_ok ) {
                return false;
            }
            // resolve DLL and create a BuiltInFunction proxy for JIT
            void * funptr = nullptr;
            string bindName = mangleFunction(ba.fn_name, ba.library, ba.api);
            if ( !ba.late && ba.api!=ApiType::api_opengl ) {
                funptr = getDllAddress(ba.library, ba.fn_name, ba.api == ApiType::api_opengl, err);
                if ( !funptr ) return false;
            }
            auto wrp = computeWrapper(fun.get());
            uint64_t code = lateBind(ba.fn_name, ba.library, funptr);
            auto bif = make_smart<DasBindFunction>(bindName, code, funptr, ba, wrp);
            bif->result = fun->result;
            for ( auto & a : fun->arguments ) {
                auto newArg = make_smart<Variable>();
                newArg->name = a->name;
                newArg->type = a->type;
                bif->arguments.push_back(newArg);
            }
            bif->noAot = true;
            bif->userScenario = true;
            bif->sideEffectFlags = fun->sideEffectFlags | uint32_t(SideEffects::accessExternal);
            module->addFunction(bif, false);
            transformMap[fun.get()] = bindName;

            return true;
        }

        static bool needWrapArg(ExpressionPtr arg) {
            if ( arg->type->isString() ) {
                if ( arg->rtti_isCallFunc() ) {
                    auto pCall = static_pointer_cast<ExprCallFunc>(arg);
                    if ( pCall->func->name!="safe_pass_string") {
                        return true;
                    }
                } else if ( strcmp(arg->__rtti,"ExprConstString")==0 ) {
                    auto str = static_pointer_cast<ExprConstString>(arg);
                    if ( str->getValue().empty()) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
            return false;
        }

        virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & ) override {
            if ( !call->func ) return nullptr;
            auto it = transformMap.find(call->func);
            bool hasBindFunction = it != transformMap.end();
            // check if any string args need wrapping
            auto needToTransform = any_of(call->arguments.begin(), call->arguments.end(), [](ExpressionPtr arg) {
                return needWrapArg(arg);
            });
            if ( !needToTransform && !hasBindFunction ) return nullptr;
            // clone call, optionally retargeting to the DasBindFunction
            ExpressionPtr newCallExpr;
            DAS_ASSERTF(hasBindFunction, "All dasbind functions should have been mapped, %s is not mapped somehow.", call->func->name.c_str());
            newCallExpr = make_smart<ExprCall>(call->at, it->second);
            for ( auto & arg : call->arguments ) {
                static_cast<ExprCall*>(newCallExpr.get())->arguments.push_back(arg->clone());
            }
            // wrap string arguments with safe_pass_string
            auto & newArgs = static_cast<ExprCallFunc*>(newCallExpr.get())->arguments;
            for ( auto & arg : newArgs ) {
                if ( needWrapArg(arg) ) {
                    auto wrapCall = make_smart<ExprCall>(arg->at,"safe_pass_string");
                    wrapCall->arguments.push_back(arg->clone());
                    arg = wrapCall;
                }
            }
            return newCallExpr;
        }
        virtual SimNode * simulate ( Context * context, Function * fun, const AnnotationArgumentList & args, string & err ) override {
            if (is_in_completion()) return nullptr;
            DAS_FATAL_ERROR("Should be unreachable. We handled it in transformCall. Failed on: %s.", fun->name.c_str());
            // // All validation is in apply(). This path is only reached for late/opengl functions.
            // auto [is_ok, ba] = parseExternArgs(args, err);
            // DAS_ASSERTF(is_ok, "Should have failed in apply");
            // void * libhandle = nullptr;
            // if ( !ba.library.empty() ) {
            //     libhandle = bindDynamicLibrary(ba.library);
            //     if ( !libhandle && !ba.late && ba.api!=ApiType::api_opengl ) {
            //         err = "can't load library " + ba.library;
            //         return nullptr;
            //     }
            // }
            // void * funptr = nullptr;
            // if ( !ba.late ) {
            //     if ( ba.api==ApiType::api_opengl ) {
            //         funptr = openGlGetFunctionAddress(ba.fn_name.c_str());
            //     }
            //     if ( !funptr ) {
            //         funptr = getFunctionAddress(libhandle, ba.fn_name.c_str());
            //     }
            //     if ( !funptr ) {
            //         err = "can't find function " + ba.fn_name + " in library " + ba.library;
            //         return nullptr;
            //     }
            // }
            // uint64_t code = lateBind(ba.fn_name, ba.library, funptr);
            // auto wrp = computeWrapper(fun);
            // if ( ba.api==ApiType::api_opengl ) {
            //     return context->code->makeNode<SimNode_ExtCallOpenGL>(fun->at,code,wrp,funptr);
            // }
            // if ( ba.late ) {
            //     return context->code->makeNode<SimNode_ExtCallLate>(fun->at,code,wrp,funptr);
            // }
            // return context->code->makeNode<SimNode_ExtCall>(fun->at,code,wrp,funptr);
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
        return nullptr;
#endif
    }

    class Module_DASBIND : public Module {
    public:
        Module_DASBIND() : Module("dasbind") {
            DAS_PROFILE_SECTION("Module_DASBIND");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addAnnotation(make_smart<ExternFunctionAnnotation>());
            addExtern<DAS_BIND_FUN(safe_pass_string)>(*this, lib, "safe_pass_string",
                SideEffects::accessExternal, "safe_pass_string")
                    ->args({"string"});
            addExtern<DAS_BIND_FUN(dasbind_resolve)>(*this, lib, "__dasbind_resolve",
                SideEffects::accessExternal, "dasbind_resolve")
                    ->args({"mangledName"});
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_dasbind.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_DASBIND,das);
