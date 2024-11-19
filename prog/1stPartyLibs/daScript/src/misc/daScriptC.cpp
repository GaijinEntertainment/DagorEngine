#include "daScript/misc/platform.h"

#include "daScript/daScript.h"
#include "daScript/daScriptC.h"

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
        virtual SimNode * simulateCopy ( Context & context, const LineInfo & at, SimNode * l, SimNode * r ) const override {
            return context.code->makeNode<SimNode_CopyRefValue>(at, l, r, sizeOf);
        }
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

uint32_t SIDEEFFECTS_none = uint32_t(SideEffects::none);
uint32_t SIDEEFFECTS_unsafe = uint32_t(SideEffects::unsafe);
uint32_t SIDEEFFECTS_userScenario = uint32_t(SideEffects::userScenario);
uint32_t SIDEEFFECTS_modifyExternal = uint32_t(SideEffects::modifyExternal);
uint32_t SIDEEFFECTS_accessExternal = uint32_t(SideEffects::accessExternal);
uint32_t SIDEEFFECTS_modifyArgument = uint32_t(SideEffects::modifyArgument);
uint32_t SIDEEFFECTS_modifyArgumentAndExternal = uint32_t(SideEffects::modifyArgumentAndExternal);
uint32_t SIDEEFFECTS_worstDefault = uint32_t(SideEffects::worstDefault);
uint32_t SIDEEFFECTS_accessGlobal = uint32_t(SideEffects::accessGlobal);
uint32_t SIDEEFFECTS_invoke =  uint32_t(SideEffects::invoke);
uint32_t SIDEEFFECTS_inferredSideEffects =  uint32_t(SideEffects::inferredSideEffects);

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

void das_fileaccess_introduce_file ( das_file_access * access, const char * file_name, const char * file_content ) {
    auto fileInfo = make_unique<TextFileInfo>((char *) file_content, uint32_t(strlen(file_content)), false);
    ((FileAccess *) access)->setFileInfo(file_name, das::move(fileInfo));
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

vec4f das_context_eval_with_catch ( das_context * context, das_function * fun, vec4f * arguments ) {
    return ((Context *)context)->evalWithCatch((SimFunction *)fun,(vec4f *)arguments);
}

char * das_context_get_exception ( das_context * context ) {
    return (char *) ((Context *)context)->getException();
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

int    das_argument_int ( vec4f arg ) { return cast<int>::to(arg); }
float  das_argument_float ( vec4f arg ) { return cast<float>::to(arg); }
double  das_argument_double ( vec4f arg ) { return cast<double>::to(arg); }
char * das_argument_string ( vec4f arg ) { char * a = cast<char *>::to(arg); return a ? a : ((char *)""); }
void * das_argument_ptr ( vec4f arg ) { return cast<void *>::to(arg); }

vec4f das_result_void () { return v_zero(); }
vec4f das_result_int ( int r ) { return cast<int>::from(r); }
vec4f das_result_float ( float r ) { return cast<float>::from(r); }
vec4f das_result_double ( double r ) { return cast<double>::from(r); }
vec4f das_result_string ( char * r ) { return cast<char *>::from(r); }
vec4f das_result_ptr ( void * r ) { return cast<void *>::from(r); }

int das_argument_int_unaligned ( vec4f_unaligned * arg ) { return cast<int>::to(v_ldu((const float *)arg)); }
float das_argument_float_unaligned ( vec4f_unaligned * arg ) { return cast<float>::to(v_ldu((const float *)arg)); }
double das_argument_double_unaligned ( vec4f_unaligned * arg ) { return cast<double>::to(v_ldu((const float *)arg)); }
char * das_argument_string_unaligned ( vec4f_unaligned * arg ) { char * a = cast<char *>::to(v_ldu((const float *)arg)); return a ? a : ((char *)""); }
void * das_argument_ptr_unaligned ( vec4f_unaligned * arg ) { return cast<void *>::to(v_ldu((const float *)arg)); }

void das_result_void_unaligned ( vec4f_unaligned * result ) { v_stu((float *)result, v_zero()); }
void das_result_int_unaligned ( vec4f_unaligned * result, int r ) { v_stu((float *)result, cast<int>::from(r)); }
void das_result_float_unaligned ( vec4f_unaligned * result, float r ) { v_stu((float *)result, cast<float>::from(r)); }
void das_result_double_unaligned ( vec4f_unaligned * result, double r ) { v_stu((float *)result, cast<double>::from(r)); }
void das_result_string_unaligned ( vec4f_unaligned * result, char * r ) { v_stu((float *)result, cast<char *>::from(r)); }
void das_result_ptr_unaligned ( vec4f_unaligned * result, void * r ) { v_stu((float *)result, cast<void *>::from(r)); }


}
