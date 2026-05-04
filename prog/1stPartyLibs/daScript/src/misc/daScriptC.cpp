#include "daScript/misc/platform.h"

#include "daScript/daScript.h"
#include "daScript/daScriptC.h"
#include "daScript/ast/dyn_modules.h"
#include "daScript/misc/sysos.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/misc/free_list.h"

using namespace das;

namespace das {
    struct SimNode_CFuncCall : SimNode_ExtFuncCallBase {
        SimNode_CFuncCall ( const LineInfo & at, const char * fnName, das_interop_function * FN )
            : SimNode_ExtFuncCallBase(at,fnName) { fn = FN; }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            vec4f * args = (vec4f *)(alloca(nArguments * sizeof(vec4f)));
            evalArgs(context, args);
            return (*fn)((das_context *)&context,(das_node *)this,args);
        }
        das_interop_function * fn;
    };

    class CFunction : public BuiltInFunction {
    public:
        __forceinline CFunction(const char * name, const ModuleLibrary &, const char * cppName, das_interop_function * FN )
            : BuiltInFunction(name,cppName) {
            this->callBased = true;
            this->interopFn = true;
            fn = FN;
        }
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            const char * fnName = context.code->allocateName(this->name);
            return context.code->makeNode<SimNode_CFuncCall>(BuiltInFunction::at,fnName,fn);
        }
        virtual void * getBuiltinAddress() const override { return (void *) fn; }
        das_interop_function * fn;
    };

    struct SimNode_CFuncCall_Unaligned : SimNode_ExtFuncCallBase {
        SimNode_CFuncCall_Unaligned ( const LineInfo & at, const char * fnName, das_interop_function_unaligned * FN )
            : SimNode_ExtFuncCallBase(at,fnName) { fn = FN; }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            vec4f * args = (vec4f *)(alloca(nArguments * sizeof(vec4f)));
            evalArgs(context, args);
            vec4f result;
            (*fn)((das_context *)&context,(das_node *)this,(vec4f_unaligned *)args,(vec4f_unaligned *)&result);
            return result;
        }
        das_interop_function_unaligned * fn;
    };

    class CFunction_Unaligned : public BuiltInFunction {
    public:
        __forceinline CFunction_Unaligned(const char * name, const ModuleLibrary &, const char * cppName, das_interop_function_unaligned * FN )
            : BuiltInFunction(name,cppName) {
            this->callBased = true;
            this->interopFn = true;
            fn = FN;
        }
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            const char * fnName = context.code->allocateName(this->name);
            return context.code->makeNode<SimNode_CFuncCall_Unaligned>(BuiltInFunction::at,fnName,fn);
        }
        das_interop_function_unaligned * fn;
    };

    struct CStructureAnnotation : BasicStructureAnnotation {
        uint32_t    sizeOf = 0;
        uint32_t    alignOf = 0;
        CStructureAnnotation(const string & n, const string & cpn, ModuleLibrary * l)
            : BasicStructureAnnotation(n,cpn,l) {}
        virtual size_t getSizeOf() const override { return sizeOf; }
        virtual size_t getAlignOf() const override { return alignOf; }
        virtual bool isSmart() const override { return false; }
        virtual bool hasNonTrivialCtor() const override { return false; }
        virtual bool hasNonTrivialDtor() const override { return false; }
        virtual bool hasNonTrivialCopy() const override { return false; }
        virtual bool isPod() const override { return true; }
        virtual bool isRawPod() const override { return false; }
        virtual bool canClone() const override { return true; }
        virtual SimNode * simulateClone ( Context & context, const LineInfo & at, SimNode * l, SimNode * r ) const override {
            return context.code->makeNode<SimNode_CopyRefValue>(at, l, r, sizeOf);
        }
    };
}

das::FileAccessPtr get_file_access( char * pak );//link time resolved dependencies

Context * get_context( int stackSize = 0 );

void das_initialize_modules() {
    NEED_ALL_DEFAULT_MODULES;
}

extern "C" {

DAS_API uint32_t SIDEEFFECTS_none = uint32_t(SideEffects::none);
DAS_API uint32_t SIDEEFFECTS_unsafe = uint32_t(SideEffects::unsafe);
DAS_API uint32_t SIDEEFFECTS_userScenario = uint32_t(SideEffects::userScenario);
DAS_API uint32_t SIDEEFFECTS_modifyExternal = uint32_t(SideEffects::modifyExternal);
DAS_API uint32_t SIDEEFFECTS_accessExternal = uint32_t(SideEffects::accessExternal);
DAS_API uint32_t SIDEEFFECTS_modifyArgument = uint32_t(SideEffects::modifyArgument);
DAS_API uint32_t SIDEEFFECTS_modifyArgumentAndExternal = uint32_t(SideEffects::modifyArgumentAndExternal);
DAS_API uint32_t SIDEEFFECTS_worstDefault = uint32_t(SideEffects::worstDefault);
DAS_API uint32_t SIDEEFFECTS_accessGlobal = uint32_t(SideEffects::accessGlobal);
DAS_API uint32_t SIDEEFFECTS_invoke =  uint32_t(SideEffects::invoke);
DAS_API uint32_t SIDEEFFECTS_inferredSideEffects =  uint32_t(SideEffects::inferredSideEffects);

DAS_API uint32_t DAS_CONTEXT_CATEGORY_THREAD_CLONE = uint32_t(ContextCategory::thread_clone);
DAS_API uint32_t DAS_CONTEXT_CATEGORY_JOB_CLONE = uint32_t(ContextCategory::job_clone);

void das_initialize() {
    das_initialize_modules();
    Module::Initialize();
}

void das_shutdown() {
    Module::Shutdown();
}

das_text_writer * das_text_make_printer() {
    return (das_text_writer *) new TextPrinter();
}

das_text_writer * das_text_make_writer() {
    return (das_text_writer *) new TextWriter();
}

void das_text_release ( das_text_writer * output ) {
    if ( output ) delete (TextWriter *) output;
}

void das_text_output ( das_text_writer * output, char * text ) {
    *((TextWriter *)output) << text;
}

das_module_group * das_modulegroup_make () {
    return (das_module_group *) new ModuleGroup();
}

void das_modulegroup_release ( das_module_group * group ) {
    if ( group ) delete (ModuleGroup *) group;
}

int das_register_dynamic_modules ( das_file_access *file_access,
                                    const char *project_root,
                                    das_text_writer *tout ) {
    TextPrinter printer;
    TextWriter *writer = tout != nullptr ? (TextWriter *)tout : &printer;
    bool res = require_dynamic_modules((FileAccess *)file_access, project_root,
                    project_root, *writer);
    return !res;
}

void das_modulegroup_add_module ( das_module_group* lib, das_module* mod ) {
    ((ModuleGroup*)lib)->addModule((Module*)mod);
}

das_file_access * das_fileaccess_make_default (  ) {
    auto access = get_file_access(nullptr);
    return (das_file_access *) access.orphan();
}

das_file_access * das_fileaccess_make_project ( const char * project_file  ) {
    auto access = get_file_access((char *)project_file);
    return (das_file_access *) access.orphan();
}

void das_fileaccess_release ( das_file_access * access ) {
    if ( access ) ((FileAccess *) access)->delRef();
}

void das_fileaccess_introduce_file ( das_file_access * access, const char * file_name, const char * file_content, int owns ) {
    auto len = uint32_t(strlen(file_content));
    if ( owns ) {
        char * content_copy = (char *) das_aligned_alloc16(len ? len : 1);
        memcpy(content_copy, file_content, len);
        auto fileInfo = make_unique<TextFileInfo>(content_copy, len, true);
        ((FileAccess *) access)->setFileInfo(file_name, das::move(fileInfo));
    } else {
        auto fileInfo = make_unique<TextFileInfo>((char *) file_content, len, false);
        ((FileAccess *) access)->setFileInfo(file_name, das::move(fileInfo));
    }
}

void das_fileaccess_introduce_file_from_disk ( das_file_access * access, const char * name, const char * disk_path ) {
    ((FsFileAccess *) access)->introduceFileFromDisk(name, disk_path);
}

void das_fileaccess_introduce_daslib ( das_file_access * access ) {
    ((FsFileAccess *) access)->introduceDaslib();
}

void das_fileaccess_introduce_native_modules ( das_file_access * access ) {
    ((FsFileAccess *) access)->introduceNativeModules();
}

int das_fileaccess_introduce_native_module ( das_file_access * access, const char * req ) {
    return ((FsFileAccess *) access)->introduceNativeModule(req) ? 1 : 0;
}

void das_fileaccess_lock ( das_file_access * access ) {
    ((FileAccess *) access)->lock();
}

void das_fileaccess_unlock ( das_file_access * access ) {
    ((FileAccess *) access)->unlock();
}

int das_fileaccess_is_locked ( das_file_access * access ) {
    return ((FileAccess *) access)->isLocked() ? 1 : 0;
}

void das_get_root ( char * root, int maxbuf ) {
    auto r = getDasRoot();
    strncpy ( root, r.c_str(), maxbuf );
}

das_program * das_program_compile ( char * program_file, das_file_access * access, das_text_writer * tout, das_module_group * libgroup ) {
    auto program = compileDaScript(program_file,
        (FileAccess *) access,
        *((TextWriter *) tout),
        *((ModuleGroup *) libgroup));
    return (das_program *) program.orphan();
}

void das_program_release ( das_program * program ) {
    if ( program ) ((Program *) program)->delRef();
}

int das_program_err_count ( das_program * program ) {
    auto prog = (Program *) program;
    return prog->failed() ? int(prog->errors.size()) : 0;
}

int das_program_context_stack_size ( das_program * program ) {
    return ((Program *)program)->getContextStackSize();
}

int das_program_simulate ( das_program * program, das_context * ctx, das_text_writer * tout ) {
    if ( ((Program *) program)->simulate(*((Context *)ctx), *((TextWriter *)tout)) ) {
        return 1;
    } else {
        return 0;
    }
}

das_error * das_program_get_error ( das_program * program, int index ) {
    auto prog = (Program *) program;
    if ( index<0 || index>=int(prog->errors.size()) ) return nullptr;
    return (das_error *) &(prog->errors[index]);
}

das_context * das_context_make ( int stackSize ) {
    return (das_context *) get_context(stackSize);
}

void das_context_release ( das_context * context ) {
    if ( context ) delete (Context *) context;
}

das_function * das_context_find_function ( das_context * context, char * name ) {
    return (das_function *) ((Context *)context)->findFunction(name);
}

void das_context_eval_with_catch_unaligned ( das_context * context, das_function * fun, vec4f_unaligned * arguments, int narguments, vec4f_unaligned * result ) {
    vec4f * args = nullptr;
    if ( narguments ) {
        if ( intptr_t(arguments) & 0xf ) {
            args = (vec4f *) alloca(narguments * sizeof(vec4f));
            DAS_ASSERT((intptr_t(args) & 0xf) == 0);
            for ( int i=0; i!=narguments; ++i ) {
                args[i] = v_ldu((const float *)(arguments + i));
            }
        } else {
            args = (vec4f *) arguments;
        }
    }
    vec4f res = ((Context *)context)->evalWithCatch((SimFunction *)fun,args);
    v_stu((float *)result, res);
}

void das_context_eval_with_catch_cmres ( das_context * context, das_function * fun, vec4f * arguments, void * cmres ) {
    ((Context *)context)->evalWithCatch((SimFunction *)fun, arguments, cmres);
}

void das_context_eval_with_catch_cmres_unaligned ( das_context * context, das_function * fun, vec4f_unaligned * arguments, int narguments, void * cmres ) {
    vec4f * args = nullptr;
    if ( narguments ) {
        if ( intptr_t(arguments) & 0xf ) {
            args = (vec4f *) alloca(narguments * sizeof(vec4f));
            DAS_ASSERT((intptr_t(args) & 0xf) == 0);
            for ( int i=0; i!=narguments; ++i ) {
                args[i] = v_ldu((const float *)(arguments + i));
            }
        } else {
            args = (vec4f *) arguments;
        }
    }
    ((Context *)context)->evalWithCatch((SimFunction *)fun, args, cmres);
}

vec4f das_context_eval_with_catch ( das_context * context, das_function * fun, vec4f * arguments ) {
    return ((Context *)context)->evalWithCatch((SimFunction *)fun, arguments);
}

char * das_context_get_exception ( das_context * context ) {
    return (char *) ((Context *)context)->getException();
}

vec4f das_context_eval_lambda ( das_context * context, das_lambda * lambda, vec4f * arguments ) {
    SimFunction ** fnpp = (SimFunction **) lambda;
    if (!fnpp) ((Context *)context)->throw_error("invoke null lambda");
    SimFunction * simFunc = *fnpp;
    if (!simFunc) ((Context *)context)->throw_error("invoke null function");
    vec4f result = ((Context *)context)->callOrFastcall(simFunc, arguments, nullptr);
    return result;
}

void das_context_eval_lambda_unaligned ( das_context * context, das_lambda * lambda, vec4f_unaligned * arguments, int narguments, vec4f_unaligned * result ) {
    vec4f * args = nullptr;
    if ( narguments ) {
        if ( intptr_t(arguments) & 0xf ) {
            args = (vec4f *) alloca(narguments * sizeof(vec4f));
            DAS_ASSERT((intptr_t(args) & 0xf) == 0);
            for ( int i=0; i!=narguments; ++i ) {
                args[i] = v_ldu((const float *)(arguments + i));
            }
        } else {
            args = (vec4f *) arguments;
        }
    }
    vec4f res = das_context_eval_lambda(context, lambda, args);
    v_stu((float *)result, res);
}

void das_context_eval_lambda_cmres ( das_context * context, das_lambda * lambda, vec4f * arguments, void * cmres ) {
    SimFunction ** fnpp = (SimFunction **) lambda;
    if (!fnpp) ((Context *)context)->throw_error("invoke null lambda");
    SimFunction * simFunc = *fnpp;
    if (!simFunc) ((Context *)context)->throw_error("invoke null function");
    ((Context *)context)->callWithCopyOnReturn(simFunc, arguments, cmres, nullptr);
}

void das_context_eval_lambda_cmres_unaligned ( das_context * context, das_lambda * lambda, vec4f_unaligned * arguments, int narguments, void * cmres ) {
    vec4f * args = nullptr;
    if ( narguments ) {
        if ( intptr_t(arguments) & 0xf ) {
            args = (vec4f *) alloca(narguments * sizeof(vec4f));
            DAS_ASSERT((intptr_t(args) & 0xf) == 0);
            for ( int i=0; i!=narguments; ++i ) {
                args[i] = v_ldu((const float *)(arguments + i));
            }
        } else {
            args = (vec4f *) arguments;
        }
    }
    das_context_eval_lambda_cmres(context, lambda, args, cmres);
}

vec4f das_context_eval_block ( das_context * context, das_block * block, vec4f * arguments ) {
    if ( !block ) ((Context *)context)->throw_error("invoke null block");   // this never happens
    return ((Context *)context)->invoke(*((Block *)block), arguments, nullptr, nullptr);
}

void das_context_eval_block_unaligned ( das_context * context, das_block * block, vec4f_unaligned * arguments, int narguments, vec4f_unaligned * result ) {
    vec4f * args = nullptr;
    if ( narguments ) {
        if ( intptr_t(arguments) & 0xf ) {
            args = (vec4f *) alloca(narguments * sizeof(vec4f));
            DAS_ASSERT((intptr_t(args) & 0xf) == 0);
            for ( int i=0; i!=narguments; ++i ) {
                args[i] = v_ldu((const float *)(arguments + i));
            }
        } else {
            args = (vec4f *) arguments;
        }
    }
    vec4f res = das_context_eval_block(context, block, args);
    v_stu((float *)result, res);
}

void das_context_eval_block_cmres ( das_context * context, das_block * block, vec4f * arguments, void * cmres ) {
    if ( !block ) ((Context *)context)->throw_error("invoke null block");   // this never happens
    ((Context *)context)->invoke(*((Block *)block), arguments, cmres, nullptr);
}

void das_context_eval_block_cmres_unaligned ( das_context * context, das_block * block, vec4f_unaligned * arguments, int narguments, void * cmres ) {
    vec4f * args = nullptr;
    if ( narguments ) {
        if ( intptr_t(arguments) & 0xf ) {
            args = (vec4f *) alloca(narguments * sizeof(vec4f));
            DAS_ASSERT((intptr_t(args) & 0xf) == 0);
            for ( int i=0; i!=narguments; ++i ) {
                args[i] = v_ldu((const float *)(arguments + i));
            }
        } else {
            args = (vec4f *) arguments;
        }
    }
    das_context_eval_block_cmres(context, block, args, cmres);
}

void das_error_output ( das_error * error, das_text_writer * tout ) {
    auto err = (Error *) error;
    *((TextWriter *)tout) << reportError(err->at, err->what, err->extra, err->fixme, err->cerr );
}

void das_error_report ( das_error * error, char * text, int maxLength ) {
    auto err = (Error *) error;
    auto str = reportError(err->at, err->what, err->extra, err->fixme, err->cerr );
    strncpy(text, str.c_str(), maxLength);
}

das_module * das_module_create ( char * name ) {
    return (das_module *) new Module(name);
}

das_module * das_module_find ( const char * name ) {
    return (das_module *) Module::require(name);
}

void das_module_bind_interop_function ( das_module * mod, das_module_group * lib, das_interop_function * fun, char * name, char * cppName, uint32_t sideffects, char* args ) {
    auto fn = make_smart<CFunction>(name, *(ModuleLibrary *)lib, cppName, fun);
    fn->setSideEffects((das::SideEffects) sideffects);
    vector <TypeDeclPtr> arguments;
    MangledNameParser parser;
    const char * arg = args;
    while ( *arg ) {
        auto tt = parser.parseTypeFromMangledName(arg, *(ModuleLibrary*)lib,(Module *)mod);
        arguments.push_back(tt);
        while (*arg==' ') arg ++;
    }
    fn->constructInterop(arguments);
    ((Module *)mod)->addFunction(fn, false);
}

void das_module_bind_interop_function_unaligned ( das_module * mod, das_module_group * lib, das_interop_function_unaligned * fun, char * name, char * cppName, uint32_t sideffects, char* args ) {
    auto fn = make_smart<CFunction_Unaligned>(name, *(ModuleLibrary *)lib, cppName, fun);
    fn->setSideEffects((das::SideEffects) sideffects);
    vector <TypeDeclPtr> arguments;
    MangledNameParser parser;
    const char * arg = args;
    while ( *arg ) {
        auto tt = parser.parseTypeFromMangledName(arg, *(ModuleLibrary*)lib,(Module *)mod);
        arguments.push_back(tt);
        while (*arg==' ') arg ++;
    }
    fn->constructInterop(arguments);
    ((Module *)mod)->addFunction(fn, false);
}

void das_module_bind_alias ( das_module * mod, das_module_group * lib, char * aname, char * tname ) {
    MangledNameParser parser;
    auto tt = (const char *) tname;
    auto at = parser.parseTypeFromMangledName(tt, *(ModuleLibrary*)lib,(Module *)mod);
    DAS_ASSERT(at->alias.empty() && "already an alias");
    at->alias = aname;
    ((Module *)mod)->addAlias(at);
}

void das_module_bind_structure ( das_module * mod, das_structure * st ) {
    ((Module *)mod)->addAnnotation((Annotation *)st);
}

void das_module_bind_enumeration ( das_module * mod, das_enumeration * en ) {
    ((Module *)mod)->addEnumeration((Enumeration *)en);
}

das_structure * das_structure_make ( das_module_group * lib, const char * name, const char * cppname, int sz, int al ) {
    auto st = make_smart<CStructureAnnotation>(name,cppname,(ModuleLibrary *)lib);
    st->sizeOf = sz;
    st->alignOf = al;
    return (das_structure *) st.orphan();
}

void das_structure_add_field ( das_structure * st, das_module * mod, das_module_group * lib,  const char * name, const char * cppname, int offset, const char * tname ) {
    MangledNameParser parser;
    auto tt = (const char *) tname;
    auto at = parser.parseTypeFromMangledName(tt, *(ModuleLibrary*)lib,(Module *)mod);
    ((CStructureAnnotation *)st)->addFieldEx(name,cppname,offset,at);
}

das_enumeration * das_enumeration_make ( const char * name, const char * cppname, int ext ) {
    auto pEnum = make_smart<Enumeration>(name);
    pEnum->cppName = cppname;
    pEnum->external = ext;
    return (das_enumeration *) pEnum.orphan();
}

void das_enumeration_add_value ( das_enumeration * enu, const char * name, const char * cppName, int value ) {
    ((Enumeration *)enu)->addIEx(name, cppName, value, LineInfo());
}

char * das_allocate_string ( das_context * context, char * str ) {
    if ( !str ) return nullptr;
    return ((Context *)context)->allocateString(str, uint32_t(strlen(str)), nullptr, false);
}

int    das_argument_int ( vec4f arg ) { return cast<int>::to(arg); }
unsigned int   das_argument_uint ( vec4f arg ) { return cast<unsigned int>::to(arg); }
int    das_argument_bool ( vec4f arg ) { return cast<bool>::to(arg) ? 1 : 0; }
float  das_argument_float ( vec4f arg ) { return cast<float>::to(arg); }
double  das_argument_double ( vec4f arg ) { return cast<double>::to(arg); }
char * das_argument_string ( vec4f arg ) { char * a = cast<char *>::to(arg); return a ? a : ((char *)""); }
void * das_argument_ptr ( vec4f arg ) { return cast<void *>::to(arg); }
das_function * das_argument_function ( vec4f arg ) { return (das_function *)(cast<Func>::to(arg)).PTR; }
das_lambda * das_argument_lambda ( vec4f arg ) { return (das_lambda *)(cast<Lambda>::to(arg)).capture; }
das_block * das_argument_block ( vec4f arg ) { return (das_block *)(cast<void *>::to(arg)); }

vec4f das_result_void () { return v_zero(); }
vec4f das_result_int ( int r ) { return cast<int>::from(r); }
vec4f das_result_uint ( unsigned int r ) { return cast<unsigned int>::from(r); }
vec4f das_result_int64 ( long long r ) { return cast<long long>::from(r); }
vec4f das_result_uint64 ( unsigned long long r ) { return cast<unsigned long long>::from(r); }
vec4f das_result_bool ( int r ) { return cast<bool>::from(r != 0); }
vec4f das_result_float ( float r ) { return cast<float>::from(r); }
vec4f das_result_double ( double r ) { return cast<double>::from(r); }
vec4f das_result_string ( char * r ) { return cast<char *>::from(r); }
vec4f das_result_ptr ( void * r ) { return cast<void *>::from(r); }
vec4f das_result_function ( das_function * r ) { return cast<Func>::from((Func)r); }
vec4f das_result_lambda ( das_lambda * r ) { return cast<Lambda>::from((Lambda)r); }
vec4f das_result_block ( das_block * r ) { return cast<void *>::from(r); }

int das_argument_int_unaligned ( vec4f_unaligned * arg ) { return cast<int>::to(v_ldu((const float *)arg)); }
unsigned int das_argument_uint_unaligned ( vec4f_unaligned * arg ) { return cast<unsigned int>::to(v_ldu((const float *)arg)); }
long long das_argument_int64_unaligned ( vec4f_unaligned * arg ) { return cast<long long>::to(v_ldu((const float *)arg)); }
unsigned long long das_argument_uint64_unaligned ( vec4f_unaligned * arg ) { return cast<unsigned long long>::to(v_ldu((const float *)arg)); }
int das_argument_bool_unaligned ( vec4f_unaligned * arg ) { return cast<bool>::to(v_ldu((const float *)arg)) ? 1 : 0; }
float das_argument_float_unaligned ( vec4f_unaligned * arg ) { return cast<float>::to(v_ldu((const float *)arg)); }
double das_argument_double_unaligned ( vec4f_unaligned * arg ) { return cast<double>::to(v_ldu((const float *)arg)); }
char * das_argument_string_unaligned ( vec4f_unaligned * arg ) { char * a = cast<char *>::to(v_ldu((const float *)arg)); return a ? a : ((char *)""); }
void * das_argument_ptr_unaligned ( vec4f_unaligned * arg ) { return cast<void *>::to(v_ldu((const float *)arg)); }
das_function * das_argument_function_unaligned ( vec4f_unaligned * arg ) { return (das_function *)(cast<Func>::to(v_ldu((const float *)arg))).PTR; }
das_lambda * das_argument_lambda_unaligned ( vec4f_unaligned * arg ) { return (das_lambda *)(cast<Lambda>::to(v_ldu((const float *)arg))).capture; }
das_block * das_argument_block_unaligned ( vec4f_unaligned * arg ) { return (das_block *)(cast<void *>::to(v_ldu((const float *)arg))); }

void das_result_void_unaligned ( vec4f_unaligned * result ) { v_stu((float *)result, v_zero()); }
void das_result_int_unaligned ( vec4f_unaligned * result, int r ) { v_stu((float *)result, cast<int>::from(r)); }
void das_result_uint_unaligned ( vec4f_unaligned * result, unsigned int r ) { v_stu((float *)result, cast<unsigned int>::from(r)); }
void das_result_int64_unaligned ( vec4f_unaligned * result, long long r ) { v_stu((float *)result, cast<long long>::from(r)); }
void das_result_uint64_unaligned ( vec4f_unaligned * result, unsigned long long r ) { v_stu((float *)result, cast<unsigned long long>::from(r)); }
void das_result_bool_unaligned ( vec4f_unaligned * result, int r ) { v_stu((float *)result, cast<bool>::from(r != 0)); }
void das_result_float_unaligned ( vec4f_unaligned * result, float r ) { v_stu((float *)result, cast<float>::from(r)); }
void das_result_double_unaligned ( vec4f_unaligned * result, double r ) { v_stu((float *)result, cast<double>::from(r)); }
void das_result_string_unaligned ( vec4f_unaligned * result, char * r ) { v_stu((float *)result, cast<char *>::from(r)); }
void das_result_ptr_unaligned ( vec4f_unaligned * result, void * r ) { v_stu((float *)result, cast<void *>::from(r)); }
void das_result_function_unaligned ( vec4f_unaligned * result, das_function * r ) { v_stu((float *)result, cast<Func>::from((Func)r)); }
void das_result_lambda_unaligned ( vec4f_unaligned * result, das_lambda * r ) { v_stu((float *)result, cast<Lambda>::from((Lambda)r)); }
void das_result_block_unaligned ( vec4f_unaligned * result, das_block * r ) { v_stu((float *)result, cast<void *>::from(r)); }

// --- Compilation policies ---

das_policies * das_policies_make() {
    return (das_policies *) new CodeOfPolicies();
}

void das_policies_release ( das_policies * policies ) {
    if ( policies ) delete (CodeOfPolicies *) policies;
}

int das_policies_set_bool ( das_policies * policies, das_bool_policy flag, int value ) {
    auto * p = (CodeOfPolicies *) policies;
    bool v = value != 0;
    switch ( flag ) {
        case DAS_POLICY_AOT:                    p->aot = v; break;
        case DAS_POLICY_NO_UNSAFE:              p->no_unsafe = v; break;
        case DAS_POLICY_NO_GLOBAL_VARIABLES:    p->no_global_variables = v; break;
        case DAS_POLICY_NO_GLOBAL_HEAP:         p->no_global_heap = v; break;
        case DAS_POLICY_NO_INIT:                p->no_init = v; break;
        case DAS_POLICY_FAIL_ON_NO_AOT:         p->fail_on_no_aot = v; break;
        case DAS_POLICY_THREADLOCK_CONTEXT:     p->threadlock_context = v; break;
        case DAS_POLICY_INTERN_STRINGS:         p->intern_strings = v; break;
        case DAS_POLICY_PERSISTENT_HEAP:        p->persistent_heap = v; break;
        case DAS_POLICY_MULTIPLE_CONTEXTS:      p->multiple_contexts = v; break;
        case DAS_POLICY_STRICT_SMART_POINTERS:  p->strict_smart_pointers = v; break;
        case DAS_POLICY_RTTI:                   p->rtti = v; break;
        case DAS_POLICY_NO_OPTIMIZATIONS:       p->no_optimizations = v; break;
        case DAS_POLICY_NO_INFER_TIME_FOLDING:  p->no_infer_time_folding = v; break;
        default: return 0;
    }
    return 1;
}

int das_policies_set_int ( das_policies * policies, das_int_policy field, int64_t value ) {
    auto * p = (CodeOfPolicies *) policies;
    switch ( field ) {
        case DAS_POLICY_STACK:                      p->stack = (uint32_t) value; break;
        case DAS_POLICY_MAX_HEAP_ALLOCATED:         p->max_heap_allocated = (uint64_t) value; break;
        case DAS_POLICY_MAX_STRING_HEAP_ALLOCATED:  p->max_string_heap_allocated = (uint64_t) value; break;
        case DAS_POLICY_HEAP_SIZE_HINT:             p->heap_size_hint = (uint32_t) value; break;
        case DAS_POLICY_STRING_HEAP_SIZE_HINT:      p->string_heap_size_hint = (uint32_t) value; break;
        default: return 0;
    }
    return 1;
}

das_program * das_program_compile_policies ( char * program_file, das_file_access * access, das_text_writer * tout, das_module_group * libgroup, das_policies * policies ) {
    auto program = compileDaScript(program_file,
        (FileAccess *) access,
        *((TextWriter *) tout),
        *((ModuleGroup *) libgroup),
        *((CodeOfPolicies *) policies));
    return (das_program *) program.orphan();
}

// --- Context variables ---

int das_context_find_variable ( das_context * context, const char * name ) {
    return ((Context *)context)->findVariable(name);
}

void * das_context_get_variable ( das_context * context, int idx ) {
    return ((Context *)context)->getVariable(idx);
}

int das_context_get_total_variables ( das_context * context ) {
    return ((Context *)context)->getTotalVariables();
}

const char * das_context_get_variable_name ( das_context * context, int idx ) {
    auto * info = ((Context *)context)->getVariableInfo(idx);
    return info ? info->name : nullptr;
}

int das_context_get_variable_size ( das_context * context, int idx ) {
    auto * info = ((Context *)context)->getVariableInfo(idx);
    return info ? (int)info->size : 0;
}

// --- Serialization ---

das_serialized_data * das_program_serialize ( das_program * program, const void ** out_data, int64_t * out_size ) {
    auto * storage = new SerializationStorageVector();
    {
        AstSerializer ser(storage, true);       // writing = true
        ((Program *)program)->serialize(ser);
        ser.moduleLibrary = nullptr;
    }
    *out_data = storage->buffer.data();
    *out_size = (int64_t) storage->buffer.size();
    return (das_serialized_data *) storage;
}

das_program * das_program_deserialize ( const void * data, int64_t size ) {
    auto * storage = new SerializationStorageVector();
    storage->buffer.assign((const uint8_t *)data, (const uint8_t *)data + size);
    ProgramPtr program;
    {
        AstSerializer deser(storage, false);    // writing = false
        program = make_smart<Program>();
        program->serialize(deser);
        deser.moduleLibrary = nullptr;
    }
    delete storage;
    return (das_program *) program.orphan();
}

void das_serialized_data_release ( das_serialized_data * blob ) {
    if ( blob ) delete (SerializationStorageVector *) blob;
}

// --- Threading ---

das_environment * das_environment_get_bound () {
    return (das_environment *) daScriptEnvironment::getBound();
}

void das_environment_set_bound ( das_environment * env ) {
    daScriptEnvironment::setBound((daScriptEnvironment *) env);
}

das_reuse_cache_guard * das_reuse_cache_guard_create () {
    return (das_reuse_cache_guard *) new ReuseCacheGuard();
}

void das_reuse_cache_guard_release ( das_reuse_cache_guard * guard ) {
    if ( guard ) delete (ReuseCacheGuard *) guard;
}

das_context * das_context_clone ( das_context * source, uint32_t category ) {
    return (das_context *) new Context(*(Context *)source, category);
}

// --- Function info ---

int das_function_is_aot ( das_function * func ) {
    return ((SimFunction *)func)->aot ? 1 : 0;
}

// --- Type introspection: flag constants ---

uint32_t DAS_TYPEINFO_FLAG_REF          = TypeInfo::flag_ref;
uint32_t DAS_TYPEINFO_FLAG_REF_TYPE     = TypeInfo::flag_refType;
uint32_t DAS_TYPEINFO_FLAG_CAN_COPY     = TypeInfo::flag_canCopy;
uint32_t DAS_TYPEINFO_FLAG_IS_POD       = TypeInfo::flag_isPod;
uint32_t DAS_TYPEINFO_FLAG_IS_RAW_POD   = TypeInfo::flag_isRawPod;
uint32_t DAS_TYPEINFO_FLAG_IS_CONST     = TypeInfo::flag_isConst;
uint32_t DAS_TYPEINFO_FLAG_IS_TEMP      = TypeInfo::flag_isTemp;
uint32_t DAS_TYPEINFO_FLAG_IS_SMART_PTR = TypeInfo::flag_isSmartPtr;
uint32_t DAS_TYPEINFO_FLAG_IS_HANDLED   = TypeInfo::flag_isHandled;

uint32_t DAS_STRUCTINFO_FLAG_CLASS          = StructInfo::flag_class;
uint32_t DAS_STRUCTINFO_FLAG_LAMBDA         = StructInfo::flag_lambda;
uint32_t DAS_STRUCTINFO_FLAG_HEAP_GC        = StructInfo::flag_heapGC;
uint32_t DAS_STRUCTINFO_FLAG_STRING_HEAP_GC = StructInfo::flag_stringHeapGC;

uint32_t DAS_FUNCINFO_FLAG_INIT     = FuncInfo::flag_init;
uint32_t DAS_FUNCINFO_FLAG_BUILTIN  = FuncInfo::flag_builtin;
uint32_t DAS_FUNCINFO_FLAG_PRIVATE  = FuncInfo::flag_private;
uint32_t DAS_FUNCINFO_FLAG_SHUTDOWN = FuncInfo::flag_shutdown;

// --- Static assertions: C constants must match C++ values ---

static_assert(DAS_TYPE_VOID == (int)Type::tVoid, "DAS_TYPE_VOID mismatch");
static_assert(DAS_TYPE_BOOL == (int)Type::tBool, "DAS_TYPE_BOOL mismatch");
static_assert(DAS_TYPE_INT8 == (int)Type::tInt8, "DAS_TYPE_INT8 mismatch");
static_assert(DAS_TYPE_UINT8 == (int)Type::tUInt8, "DAS_TYPE_UINT8 mismatch");
static_assert(DAS_TYPE_INT16 == (int)Type::tInt16, "DAS_TYPE_INT16 mismatch");
static_assert(DAS_TYPE_UINT16 == (int)Type::tUInt16, "DAS_TYPE_UINT16 mismatch");
static_assert(DAS_TYPE_INT64 == (int)Type::tInt64, "DAS_TYPE_INT64 mismatch");
static_assert(DAS_TYPE_UINT64 == (int)Type::tUInt64, "DAS_TYPE_UINT64 mismatch");
static_assert(DAS_TYPE_INT == (int)Type::tInt, "DAS_TYPE_INT mismatch");
static_assert(DAS_TYPE_INT2 == (int)Type::tInt2, "DAS_TYPE_INT2 mismatch");
static_assert(DAS_TYPE_INT3 == (int)Type::tInt3, "DAS_TYPE_INT3 mismatch");
static_assert(DAS_TYPE_INT4 == (int)Type::tInt4, "DAS_TYPE_INT4 mismatch");
static_assert(DAS_TYPE_UINT == (int)Type::tUInt, "DAS_TYPE_UINT mismatch");
static_assert(DAS_TYPE_UINT2 == (int)Type::tUInt2, "DAS_TYPE_UINT2 mismatch");
static_assert(DAS_TYPE_UINT3 == (int)Type::tUInt3, "DAS_TYPE_UINT3 mismatch");
static_assert(DAS_TYPE_UINT4 == (int)Type::tUInt4, "DAS_TYPE_UINT4 mismatch");
static_assert(DAS_TYPE_FLOAT == (int)Type::tFloat, "DAS_TYPE_FLOAT mismatch");
static_assert(DAS_TYPE_FLOAT2 == (int)Type::tFloat2, "DAS_TYPE_FLOAT2 mismatch");
static_assert(DAS_TYPE_FLOAT3 == (int)Type::tFloat3, "DAS_TYPE_FLOAT3 mismatch");
static_assert(DAS_TYPE_FLOAT4 == (int)Type::tFloat4, "DAS_TYPE_FLOAT4 mismatch");
static_assert(DAS_TYPE_DOUBLE == (int)Type::tDouble, "DAS_TYPE_DOUBLE mismatch");
static_assert(DAS_TYPE_RANGE == (int)Type::tRange, "DAS_TYPE_RANGE mismatch");
static_assert(DAS_TYPE_URANGE == (int)Type::tURange, "DAS_TYPE_URANGE mismatch");
static_assert(DAS_TYPE_RANGE64 == (int)Type::tRange64, "DAS_TYPE_RANGE64 mismatch");
static_assert(DAS_TYPE_URANGE64 == (int)Type::tURange64, "DAS_TYPE_URANGE64 mismatch");
static_assert(DAS_TYPE_STRING == (int)Type::tString, "DAS_TYPE_STRING mismatch");
static_assert(DAS_TYPE_STRUCTURE == (int)Type::tStructure, "DAS_TYPE_STRUCTURE mismatch");
static_assert(DAS_TYPE_HANDLE == (int)Type::tHandle, "DAS_TYPE_HANDLE mismatch");
static_assert(DAS_TYPE_ENUMERATION == (int)Type::tEnumeration, "DAS_TYPE_ENUMERATION mismatch");
static_assert(DAS_TYPE_ENUMERATION8 == (int)Type::tEnumeration8, "DAS_TYPE_ENUMERATION8 mismatch");
static_assert(DAS_TYPE_ENUMERATION16 == (int)Type::tEnumeration16, "DAS_TYPE_ENUMERATION16 mismatch");
static_assert(DAS_TYPE_ENUMERATION64 == (int)Type::tEnumeration64, "DAS_TYPE_ENUMERATION64 mismatch");
static_assert(DAS_TYPE_BITFIELD == (int)Type::tBitfield, "DAS_TYPE_BITFIELD mismatch");
static_assert(DAS_TYPE_BITFIELD8 == (int)Type::tBitfield8, "DAS_TYPE_BITFIELD8 mismatch");
static_assert(DAS_TYPE_BITFIELD16 == (int)Type::tBitfield16, "DAS_TYPE_BITFIELD16 mismatch");
static_assert(DAS_TYPE_BITFIELD64 == (int)Type::tBitfield64, "DAS_TYPE_BITFIELD64 mismatch");
static_assert(DAS_TYPE_POINTER == (int)Type::tPointer, "DAS_TYPE_POINTER mismatch");
static_assert(DAS_TYPE_FUNCTION == (int)Type::tFunction, "DAS_TYPE_FUNCTION mismatch");
static_assert(DAS_TYPE_LAMBDA == (int)Type::tLambda, "DAS_TYPE_LAMBDA mismatch");
static_assert(DAS_TYPE_ITERATOR == (int)Type::tIterator, "DAS_TYPE_ITERATOR mismatch");
static_assert(DAS_TYPE_ARRAY == (int)Type::tArray, "DAS_TYPE_ARRAY mismatch");
static_assert(DAS_TYPE_TABLE == (int)Type::tTable, "DAS_TYPE_TABLE mismatch");
static_assert(DAS_TYPE_BLOCK == (int)Type::tBlock, "DAS_TYPE_BLOCK mismatch");
static_assert(DAS_TYPE_TUPLE == (int)Type::tTuple, "DAS_TYPE_TUPLE mismatch");
static_assert(DAS_TYPE_VARIANT == (int)Type::tVariant, "DAS_TYPE_VARIANT mismatch");

// --- Type introspection: entry points ---

das_type_info * das_context_get_variable_type ( das_context * context, int idx ) {
    return (das_type_info *) ((Context *)context)->getVariableInfo(idx);
}

int das_context_get_total_functions ( das_context * context ) {
    return ((Context *)context)->getTotalFunctions();
}

das_function * das_context_get_function ( das_context * context, int idx ) {
    return (das_function *) ((Context *)context)->getFunction(idx);
}

das_func_info * das_function_get_info ( das_function * func ) {
    return (das_func_info *) ((SimFunction *)func)->debugInfo;
}

// --- Type introspection: TypeInfo accessors ---

das_base_type das_type_info_get_type ( das_type_info * info ) {
    return (das_base_type) ((TypeInfo *)info)->type;
}

int das_type_info_get_size ( das_type_info * info ) {
    return getTypeSize((TypeInfo *)info);
}

int das_type_info_get_align ( das_type_info * info ) {
    return getTypeAlign((TypeInfo *)info);
}

uint32_t das_type_info_get_flags ( das_type_info * info ) {
    return ((TypeInfo *)info)->flags;
}

uint64_t das_type_info_get_hash ( das_type_info * info ) {
    return ((TypeInfo *)info)->hash;
}

const char * das_type_info_get_description ( das_type_info * info ) {
    static thread_local string buf;
    buf = debug_type((TypeInfo *)info);
    return buf.c_str();
}

const char * das_type_info_get_mangled_name ( das_type_info * info ) {
    static thread_local string buf;
    buf = getTypeInfoMangledName((TypeInfo *)info);
    return buf.c_str();
}

// --- Type introspection: composite type navigation ---

das_type_info * das_type_info_get_first_type ( das_type_info * info ) {
    return (das_type_info *) ((TypeInfo *)info)->firstType;
}

das_type_info * das_type_info_get_second_type ( das_type_info * info ) {
    return (das_type_info *) ((TypeInfo *)info)->secondType;
}

int das_type_info_get_arg_count ( das_type_info * info ) {
    return (int) ((TypeInfo *)info)->argCount;
}

das_type_info * das_type_info_get_arg_type ( das_type_info * info, int idx ) {
    auto * ti = (TypeInfo *)info;
    if ( idx < 0 || idx >= (int)ti->argCount ) return nullptr;
    return (das_type_info *) ti->argTypes[idx];
}

const char * das_type_info_get_arg_name ( das_type_info * info, int idx ) {
    auto * ti = (TypeInfo *)info;
    if ( idx < 0 || idx >= (int)ti->argCount || !ti->argNames ) return nullptr;
    return ti->argNames[idx];
}

int das_type_info_get_tuple_field_offset ( das_type_info * info, int idx ) {
    return getTupleFieldOffset((TypeInfo *)info, idx);
}

int das_type_info_get_variant_field_offset ( das_type_info * info, int idx ) {
    return getVariantFieldOffset((TypeInfo *)info, idx);
}

int das_type_info_get_dim_count ( das_type_info * info ) {
    return (int) ((TypeInfo *)info)->dimSize;
}

int das_type_info_get_dim ( das_type_info * info, int idx ) {
    auto * ti = (TypeInfo *)info;
    if ( idx < 0 || idx >= (int)ti->dimSize ) return 0;
    return (int) ti->dim[idx];
}

// --- Type introspection: structures ---

das_struct_info * das_type_info_get_struct ( das_type_info * info ) {
    auto * ti = (TypeInfo *)info;
    if ( ti->type != Type::tStructure ) return nullptr;
    return (das_struct_info *) ti->structType;
}

const char * das_struct_info_get_name ( das_struct_info * info ) {
    return ((StructInfo *)info)->name;
}

const char * das_struct_info_get_module ( das_struct_info * info ) {
    return ((StructInfo *)info)->module_name;
}

int das_struct_info_get_field_count ( das_struct_info * info ) {
    return (int) ((StructInfo *)info)->count;
}

int das_struct_info_get_size ( das_struct_info * info ) {
    return (int) ((StructInfo *)info)->size;
}

uint32_t das_struct_info_get_flags ( das_struct_info * info ) {
    return ((StructInfo *)info)->flags;
}

uint64_t das_struct_info_get_hash ( das_struct_info * info ) {
    return ((StructInfo *)info)->hash;
}

const char * das_struct_info_get_field_name ( das_struct_info * info, int idx ) {
    auto * si = (StructInfo *)info;
    if ( idx < 0 || idx >= (int)si->count ) return nullptr;
    return si->fields[idx]->name;
}

int das_struct_info_get_field_offset ( das_struct_info * info, int idx ) {
    auto * si = (StructInfo *)info;
    if ( idx < 0 || idx >= (int)si->count ) return -1;
    return (int) si->fields[idx]->offset;
}

das_type_info * das_struct_info_get_field_type ( das_struct_info * info, int idx ) {
    auto * si = (StructInfo *)info;
    if ( idx < 0 || idx >= (int)si->count ) return nullptr;
    return (das_type_info *) si->fields[idx];
}

// --- Type introspection: enumerations ---

das_enum_info * das_type_info_get_enum ( das_type_info * info ) {
    auto * ti = (TypeInfo *)info;
    switch ( ti->type ) {
        case Type::tEnumeration:
        case Type::tEnumeration8:
        case Type::tEnumeration16:
        case Type::tEnumeration64:
            return (das_enum_info *) ti->enumType;
        default:
            return nullptr;
    }
}

const char * das_enum_info_get_name ( das_enum_info * info ) {
    return ((EnumInfo *)info)->name;
}

const char * das_enum_info_get_module ( das_enum_info * info ) {
    return ((EnumInfo *)info)->module_name;
}

int das_enum_info_get_count ( das_enum_info * info ) {
    return (int) ((EnumInfo *)info)->count;
}

const char * das_enum_info_get_value_name ( das_enum_info * info, int idx ) {
    auto * ei = (EnumInfo *)info;
    if ( idx < 0 || idx >= (int)ei->count ) return nullptr;
    return ei->fields[idx]->name;
}

int64_t das_enum_info_get_value ( das_enum_info * info, int idx ) {
    auto * ei = (EnumInfo *)info;
    if ( idx < 0 || idx >= (int)ei->count ) return 0;
    return ei->fields[idx]->value;
}

// --- Type introspection: function info ---

const char * das_func_info_get_name ( das_func_info * info ) {
    return ((FuncInfo *)info)->name;
}

const char * das_func_info_get_cpp_name ( das_func_info * info ) {
    return ((FuncInfo *)info)->cppName;
}

int das_func_info_get_arg_count ( das_func_info * info ) {
    return (int) ((FuncInfo *)info)->count;
}

das_type_info * das_func_info_get_arg_type ( das_func_info * info, int idx ) {
    auto * fi = (FuncInfo *)info;
    if ( idx < 0 || idx >= (int)fi->count ) return nullptr;
    return (das_type_info *) fi->fields[idx];
}

const char * das_func_info_get_arg_name ( das_func_info * info, int idx ) {
    auto * fi = (FuncInfo *)info;
    if ( idx < 0 || idx >= (int)fi->count ) return nullptr;
    return fi->fields[idx]->name;
}

das_type_info * das_func_info_get_result ( das_func_info * info ) {
    return (das_type_info *) ((FuncInfo *)info)->result;
}

uint64_t das_func_info_get_hash ( das_func_info * info ) {
    return ((FuncInfo *)info)->hash;
}

uint32_t das_func_info_get_flags ( das_func_info * info ) {
    return ((FuncInfo *)info)->flags;
}

int das_func_info_get_stack_size ( das_func_info * info ) {
    return (int) ((FuncInfo *)info)->stackSize;
}

}
