#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"

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
#elif defined(__linux__)
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

    struct SimNode_ExtCall : SimNode {
        SimNode_ExtCall ( const LineInfo & a, uint64_t hc, FastCallWrapper wrp, void * fun ) : SimNode(a) {
            code = hc;
            wrapper = wrp;
            fnptr = fun;
        }
        SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(ExtCall);
            withBind(code,[&]( const BoundFunction * bf ){
                if ( bf ) {
                    auto Library = bf->library.c_str();
                    auto Name = bf->name.c_str();
                    V_ARG(Library);
                    V_ARG(Name);
                } else {
                    V_ARG(code);
                }
            });
            V_END();
        }
        DAS_EVAL_ABI vec4f eval ( Context & context ) override {
            context.result = wrapper(fnptr, context.abiArg);
            return context.result;
        }
        uint64_t code = 0;
        FastCallWrapper wrapper = nullptr;
        void * fnptr = nullptr;
    };

    struct SimNode_ExtCallOpenGL : SimNode_ExtCall {
        SimNode_ExtCallOpenGL ( const LineInfo & a, uint64_t hc, FastCallWrapper wrp, void * fun ) :
            SimNode_ExtCall(a,hc,wrp,fun) {
        }
        void bind ( Context & context ) {
            string crash_and_burn;
            withBind(code,[&](BoundFunction * bf){
                if ( bf ) {
                    if ( !bf->fun ) {
                        fnptr = openGlGetFunctionAddress(bf->name.c_str());
                        if ( fnptr ) {
                            bf->fun = fnptr;
                        } else {
                            crash_and_burn = "failed to bind " + bf->name;
                        }
                    } else {
                        fnptr = bf->fun;
                    }
                } else {
                    crash_and_burn = "internal error. missing BoundFunction";
                }
            });
            if ( !crash_and_burn.empty() ) context.throw_error_at(debugInfo, "%s",crash_and_burn.c_str());
        }
        DAS_EVAL_ABI vec4f eval ( Context & context ) {
            if ( !fnptr ) bind(context);
            context.result = wrapper(fnptr, context.abiArg);
            return context.result;
        }
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

#endif

    struct ExternFunctionAnnotation : FunctionAnnotation {
        ExternFunctionAnnotation() : FunctionAnnotation("extern") { }
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
            err = "daScript is configured with extern functions disabled";
            return false;
        }
#else
        virtual bool apply ( const FunctionPtr & fun, ModuleGroup &, const AnnotationArgumentList &, string & err )  override {
            if ( fun->arguments.size() >= MAX_WRAPPER_ARGUMENTS ) {
                err = "function has too many arguments for the current wrapper config";
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
                // printf("%s %i %i %i\n", fun->name.c_str(), nargs, res, perm);
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
            fun->userScenario = true;
            fun->noAot = true;      // TODO: generate custom C++ to invoke the call directly
            return true;
        }
        virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & ) override {
            if ( !call->func ) return nullptr;
            auto needToTransform = false;
            for ( auto & arg : call->arguments ) {
                if ( arg->type->isString() ) {
                    if ( arg->rtti_isCallFunc() ) {
                        auto pCall = static_pointer_cast<ExprCallFunc>(arg);
                        if ( pCall->func->name!="safe_pass_string") {
                            needToTransform = true;
                            break;
                        }
                    } else if ( strcmp(arg->__rtti,"ExprConstString")==0 ) {
                        // do nothing
                    } else {
                        needToTransform = true;
                        break;
                    }
                }
            }
            if ( needToTransform ) {
                auto newCall = static_pointer_cast<ExprCallFunc>(call->clone());
                for ( auto & arg : newCall->arguments ) {
                    bool needToWrap = false;
                    if ( arg->type->isString() ) {
                        if ( arg->rtti_isCallFunc() ) {
                            auto pCall = static_pointer_cast<ExprCallFunc>(arg);
                            if ( pCall->func->name=="safe_pass_string") {
                                needToWrap = true;
                            }
                        } else if ( strcmp(arg->__rtti,"ExprConstString")==0 ) {
                            // do nothing
                        } else {
                            needToWrap = true;
                        }
                    }
                    if ( needToWrap ) {
                        auto wrapCall = make_smart<ExprCall>(arg->at,"safe_pass_string");
                        wrapCall->arguments.push_back(arg->clone());
                        arg = wrapCall;
                    }
                }
                return newCall;
            }
            return nullptr;
        }
        virtual SimNode * simulate ( Context * context, Function * fun, const AnnotationArgumentList & args, string & err ) override {
            bool anyTypeErrors = false;
            for ( const auto & arg : fun->arguments ) {
                if ( !arg->type->isRef() && arg->type->isVectorType() ) {
                    err += "argument " + arg->name + " is vector type, which is currently not supported by specified binding";
                    anyTypeErrors = true;
                }
            }
#ifndef _MSC_VER
            int nargs=-1, res=0, perm=0;
            if ( fun->arguments.size()>6 ) {
                nargs = int(fun->arguments.size());
                res = ( fun->result->baseType==Type::tFloat || fun->result->baseType==Type::tDouble ) ? 0 : 1;
                for ( size_t ai=0, ais=fun->arguments.size(); ai!=ais; ++ai ) {
                    const auto & arg = fun->arguments[ai];
                    if ( arg->type->isSimpleType(Type::tFloat) || arg->type->isSimpleType(Type::tDouble) ) {
                        perm |= (1<<int(ai));
                    }
                }
                if ( perm<(1<<7) ) {    // nothing outside of the first 6 is float
                    nargs = -1;
                }
            }
#endif
            if ( fun->result && !fun->result->isRef() && fun->result->isVectorType() ) {
                err += "function returns vector type, which is currently not supported by the specified binding";
                anyTypeErrors = true;
            }
            if ( anyTypeErrors ) return nullptr;
            string fn_name;
            string library;
            string api;
            bool late = false;
            for ( auto & arg : args ) {
                        if ( arg.name=="name" && arg.type==Type::tString ) {
                    fn_name = arg.sValue;
                } else if ( arg.name=="library" && arg.type==Type::tString ) {
                    library = arg.sValue;
                } else if ( arg.name=="WINAPI" || arg.name=="winapi" || arg.name=="stdcall" || arg.name=="__stdcall" || arg.name=="STDCALL" ) {
                    api = "stdcall";
                } else if ( arg.name=="CDECL" || arg.name=="cdecl" || arg.name=="__cdecl" ) {
                    api = "cdecl";
                } else if ( arg.name=="opengl" || arg.name=="OPENGL" ) {
                    api = "opengl";
                } else if ( arg.name=="late" ) {
                    late = true;
                }
            }
            if ( fn_name.empty() ) {
                err = "missing name";
                return nullptr;
            }
            if ( api.empty() ) {
                err = "need to specify calling convention (like stdcall (aka WINAPI),cdecl,etc)";
                return nullptr;
            }
            void * libhandle = nullptr;
            if ( !library.empty() ) {
                libhandle = getLibraryHandle(library.c_str());
                if ( !libhandle ) {
                    libhandle = loadDynamicLibrary(library.c_str());
                }
                if ( !libhandle && !late ) {
                    err = "can't load library " + library;
                    return nullptr;
                }
            }
            void * funptr = nullptr;
            if ( !late ) {
                if ( api=="opengl" ) {
                    funptr = openGlGetFunctionAddress(fn_name.c_str());
                }
                if ( !funptr ) {
                    funptr = getFunctionAddress(libhandle, fn_name.c_str());
                }
                if ( !funptr && !late ) {
                    err = "can't find function " + fn_name + " in library " + library;
                    return nullptr;
                }
            }
            uint64_t code = lateBind(fn_name, library, funptr);
#ifdef _MSC_VER
            auto wrp = getWrapper(fun,4);
#else
            auto wrp = nargs==-1 ? getWrapper(fun,6) : getExtraWrapper(nargs,res,perm);
#endif
            if ( api=="stdcall") {
                return context->code->makeNode<SimNode_ExtCall>(fun->at,code,wrp,funptr);
            } else if ( api=="cdecl" ) {
                return context->code->makeNode<SimNode_ExtCall>(fun->at,code,wrp,funptr);
            } else if ( api=="opengl" ) {
                return context->code->makeNode<SimNode_ExtCallOpenGL>(fun->at,code,wrp,funptr);
            } else {
                err = "unsupported api " + api;
                return nullptr;
            }
        }
#endif
    };

    char * safe_pass_string ( const char * str ) {
        return (char *)(str ? str : "");
    }

    class Module_DASBIND : public Module {
    public:
        Module_DASBIND() : Module("dasbind") {
            DAS_PROFILE_SECTION("Module_DASBIND");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addAnnotation(make_smart<ExternFunctionAnnotation>());
            addExtern<DAS_BIND_FUN(safe_pass_string)>(*this, lib, "safe_pass_string",
                SideEffects::none, "safe_pass_string")
                    ->args({"string"});
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_dasbind.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_DASBIND,das);
