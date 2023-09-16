#pragma once

#include "daScript/simulate/bind_enum.h"
#include "daScript/misc/arraytype.h"

DAS_BIND_ENUM_CAST(RefMatters);
DAS_BIND_ENUM_CAST(ConstMatters);
DAS_BIND_ENUM_CAST(TemporaryMatters);

DAS_BIND_ENUM_CAST(Type);

namespace das {
    class ModuleGroup;
    struct AnnotationArgumentList;
}

MAKE_EXTERNAL_TYPE_FACTORY(ModuleGroup, das::ModuleGroup)

namespace das {

    class Program;
    typedef smart_ptr<Program> ProgramPtr;

    class Module;
    struct Annotation;

    #pragma pack(16)
    struct RttiValue {
        int32_t         _variant;
        union {
            bool        bValue;         // 0
            int32_t     iValue;         // 1
            uint32_t    uValue;         // 2
            int64_t     i64Value;       // 3
            uint64_t    u64Value;       // 4
            float       fValue;         // 5
            double      dfValue;        // 6
            char *      sValue;         // 7
            vec4f       nothing;        // 8
        };
    };
    #pragma pack()
    static_assert(sizeof(RttiValue)==32,"sizeof RttiValue must be 32");

    template <> struct das_alias<RttiValue>
        : das_alias_ref<RttiValue,TVariant<sizeof(RttiValue),alignof(RttiValue),bool,int32_t,uint32_t,int64_t,uint64_t,float,double,char *,vec4f>> {};

    template <typename TT, typename PD, typename TTA = const TT>
    struct das_rtti_iterator {
        __forceinline das_rtti_iterator(const PD & r) {
            array_start = r.fields;
            array_end = array_start + r.count;
        }
        __forceinline bool first ( Context *, TTA * & i ) {
            at = array_start;
            if ( at != array_end ) {
                i = *at;
                return true;
            } else {
                return false;
            }
        }
        __forceinline bool next  ( Context *, TTA * & i ) {
            at++;
            if ( at != array_end ) {
                i = *at;
                return true;
            } else {
                return false;
            }
        }
        __forceinline void close ( Context *, TTA * & i ) {
            i = nullptr;
        }
        TT ** at;
        TT ** array_start;
        TT ** array_end;
    };

    template <>
    struct das_iterator<EnumInfo> :
        das_rtti_iterator<EnumValueInfo, EnumInfo, EnumValueInfo> {
        das_iterator(const EnumInfo & info) : das_rtti_iterator<EnumValueInfo, EnumInfo, EnumValueInfo>(info) {}
    };

    template <>
    struct das_iterator<EnumInfo const> :
        das_rtti_iterator<EnumValueInfo, EnumInfo> {
        das_iterator(EnumInfo const & info) : das_rtti_iterator<EnumValueInfo, EnumInfo>(info) {}
    };

    template <>
    struct das_iterator<FuncInfo const> :
        das_rtti_iterator<VarInfo, FuncInfo> {
        das_iterator(FuncInfo const & info) : das_rtti_iterator<VarInfo, FuncInfo>(info) {}
    };

    template <>
    struct das_iterator<StructInfo const> :
        das_rtti_iterator<VarInfo, StructInfo> {
        das_iterator(StructInfo const & info) : das_rtti_iterator<VarInfo, StructInfo>(info) {}
    };

    char * rtti_get_das_type_name(Type tt, Context * context);
    int rtti_add_annotation_argument(AnnotationArgumentList& list, const char* name);

    vec4f rtti_contextFunctionInfo ( Context & context, SimNode_CallBase *, vec4f * );
    vec4f rtti_contextVariableInfo ( Context & context, SimNode_CallBase *, vec4f * );
    int32_t rtti_contextTotalFunctions(Context & context);
    int32_t rtti_contextTotalVariables(Context & context);

    __forceinline Context  & thisContext ( Context * context ) { return *context; }

    RttiValue rtti_builtin_variable_value(const VarInfo & info);

    struct AnnotationArgument;
    RttiValue rtti_builtin_argument_value(const AnnotationArgument & info, Context * context);

    int32_t rtti_getDimTypeInfo(const TypeInfo & ti, int32_t index, Context * context, LineInfoArg * at);
    int32_t rtti_getDimVarInfo(const VarInfo & ti, int32_t index, Context * context, LineInfoArg * at);

    char * builtin_das_root ( Context * context );
    smart_ptr<FileAccess> makeFileAccess( char * pak, Context * context, LineInfoArg * at );
    bool introduceFile ( smart_ptr_raw<FileAccess> access, char * fname, char * str, Context * context, LineInfoArg * );
    bool rtti_add_file_access_root ( smart_ptr<FileAccess> access, const char * mod, const char * path );

    struct CodeOfPolicies;
    void rtti_builtin_compile(char * modName, char * str, const CodeOfPolicies & cop,
        const TBlock<void, bool, smart_ptr<Program>, const string> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_compile_ex ( char * modName, char * str, const CodeOfPolicies & cop, bool exportAll,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at );
    void rtti_builtin_compile_file(char * modName, smart_ptr<FileAccess> access, ModuleGroup* module_group, const CodeOfPolicies & cop,
        const TBlock<void, bool, smart_ptr<Program>, const string> & block, Context * context, LineInfoArg * lineinfo);

    void rtti_builtin_simulate ( const smart_ptr<Program> & program,
        const TBlock<void,bool,smart_ptr<Context>,string> & block, Context * context, LineInfoArg * lineinfo );

    void rtti_builtin_program_for_each_module(smart_ptr_raw<Program> prog, const TBlock<void, Module *> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_program_for_each_registered_module(const TBlock<void, Module *> & block, Context * context, LineInfoArg * lineinfo);

    Module * rtti_get_this_module(smart_ptr_raw<Program> prog);
    Module * rtti_get_builtin_module(const char * name);

    void rtti_builtin_module_for_each_enumeration(Module * module, const TBlock<void, const EnumInfo> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_module_for_each_structure(Module * module, const TBlock<void, const StructInfo> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_module_for_each_function(Module * module, const TBlock<void, const FuncInfo> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_module_for_each_generic(Module * module, const TBlock<void, const FuncInfo> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_module_for_each_global(Module * module, const TBlock<void, const VarInfo> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_module_for_each_annotation(Module * module, const TBlock<void, const Annotation> & block, Context * context, LineInfoArg * lineinfo);

    // note: this one is not TBlock, so that we don't have to include ast.h
    void rtti_builtin_structure_for_each_annotation(const StructInfo & info, const Block & block, Context * context, LineInfoArg * lineinfo);

    // if we are in the module, compiling macros
    bool is_compiling_macros_in_module ( char * name );

    struct BasicStructureAnnotation;
    void rtti_builtin_basic_struct_for_each_field(const BasicStructureAnnotation & ann,
        const TBlock<void, char *, char*, const TypeInfo, uint32_t> & block, Context * context, LineInfoArg * lineinfo);
    void rtti_builtin_basic_struct_for_each_parent ( const BasicStructureAnnotation & ann,
        const TBlock<void,Annotation *> & block, Context * context, LineInfoArg * at );

    LineInfo getCurrentLineInfo( LineInfoArg * lineInfo );
    void builtin_expected_errors ( ProgramPtr prog, const TBlock<void,CompilationError,int> & block, Context * context, LineInfoArg * lineInfo );
    void builtin_list_require ( ProgramPtr prog, const TBlock<void,Module *,TTemporary<const char *>,TTemporary<const char *>,bool,const LineInfo &> & block,
        Context * context, LineInfoArg * lineInfo );

    Func builtin_getFunctionByMnh ( uint64_t MNH, Context * context );
    Func builtin_getFunctionByMnh_inContext ( uint64_t MNH, Context & context );
    uint64_t builtin_getFunctionMnh ( Func func, Context * context );
    uint64_t das_get_SimFunction_by_MNH ( uint64_t MNH, Context & context );
    int32_t rtti_getTablePtr ( void * _table, vec4f key, Type baseType, int valueTypeSize, Context * context, LineInfoArg * at );

    void lockThisContext ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo );
    void lockAnyContext ( Context & ctx, const TBlock<void> & block, Context * context, LineInfoArg * lineInfo );
    void lockAnyMutex ( recursive_mutex & rm, const TBlock<void> & block, Context * context, LineInfoArg * lineInfo );

    TSequence<VarInfo> each_FuncInfo ( FuncInfo & st, Context * context );
    TSequence<const VarInfo> each_const_FuncInfo ( const FuncInfo & st, Context * context );
    TSequence<VarInfo> each_StructInfo ( StructInfo & st, Context * context );
    TSequence<const VarInfo> each_const_StructInfo ( const StructInfo & st, Context * context );
    TSequence<EnumValueInfo> each_EnumInfo ( EnumInfo & st, Context * context );
    TSequence<const EnumValueInfo> each_const_EnumInfo ( const EnumInfo & st, Context * context );
}

