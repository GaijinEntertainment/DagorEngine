#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"
namespace das {
    char * ast_describe_typedecl ( smart_ptr_raw<TypeDecl> t, bool d_extra, bool d_contracts, bool d_module, Context * context, LineInfoArg * at );
    char * ast_describe_typedecl_cpp ( smart_ptr_raw<TypeDecl> t, bool d_substitureRef, bool d_skipRef, bool d_skipConst, bool d_redundantConst, Context * context, LineInfoArg * at );
    char * ast_describe_expression ( smart_ptr_raw<Expression> t, Context * context, LineInfoArg * at );
    char * ast_describe_function ( smart_ptr_raw<Function> t, Context * context, LineInfoArg * at );
    char * ast_das_to_string ( Type bt, Context * context, LineInfoArg * at );
    char * ast_find_bitfield_name ( smart_ptr_raw<TypeDecl> bft, Bitfield value, Context * context, LineInfoArg * at );
    char * ast_find_enum_name ( Enumeration * enu, int64_t value, Context * context, LineInfoArg * at );
    int64_t ast_find_enum_value ( EnumerationPtr enu, const char * value );
    int64_t ast_find_enum_value_ex ( Enumeration * enu, const char * value );

    int32_t any_array_size ( void * _arr );
    int32_t any_table_size ( void * _tab );
    void any_array_foreach ( void * _arr, int stride, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    void any_table_foreach ( void * _tab, int keyStride, int valueStride, const TBlock<void,void *,void *> & blk, Context * context, LineInfoArg * at );

    int32_t get_variant_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at );
    int32_t get_tuple_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at );

    __forceinline void mks_vector_emplace ( MakeStruct & vec, MakeFieldDeclPtr & value ) {
        vec.emplace_back(das::move(value));
    }
    __forceinline void mks_vector_pop ( MakeStruct & vec ) {
        vec.pop_back();
    }
    __forceinline void mks_vector_clear ( MakeStruct & vec ) {
        vec.clear();
    }
    __forceinline void mks_vector_resize ( MakeStruct & vec, int32_t newSize ) {
        vec.resize(newSize);
    }

    Module * compileModule ( Context * context, LineInfoArg * at );
    smart_ptr_raw<Program> compileProgram ( Context * context, LineInfoArg * at );

    class VisitorAdapter;

    DebugAgentPtr makeDebugAgent ( const void * pClass, const StructInfo * info, Context * context );
    Module * thisModule ( Context * context, LineInfoArg * lineinfo );
    smart_ptr_raw<Program> thisProgram ( Context * context );
    void astVisit ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    void astVisitModulesInOrder ( smart_ptr_raw<Program> program, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    void astVisitFunction ( smart_ptr_raw<Function> func, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    smart_ptr_raw<Expression> astVisitExpression ( smart_ptr_raw<Expression> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info);
    void astVisitBlockFinally ( smart_ptr_raw<ExprBlock> expr, smart_ptr_raw<VisitorAdapter> adapter, Context * context, LineInfoArg * line_info );
    PassMacroPtr makePassMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    smart_ptr<VisitorAdapter> makeVisitor ( const void * pClass, const StructInfo * info, Context * context );
    void addModuleInferMacro ( Module * module, PassMacroPtr & _newM, Context * );
    void addModuleInferDirtyMacro ( Module * module, PassMacroPtr & newM, Context * context );
    void addModuleLintMacro ( Module * module, PassMacroPtr & _newM, Context * );
    void addModuleGlobalLintMacro ( Module * module, PassMacroPtr & _newM, Context * );
    void addModuleOptimizationMacro ( Module * module, PassMacroPtr & _newM, Context * );
    VariantMacroPtr makeVariantMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleVariantMacro ( Module * module, VariantMacroPtr & newM, Context * context );
    ForLoopMacroPtr makeForLoopMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleForLoopMacro ( Module * module, ForLoopMacroPtr & _newM, Context * );
    CaptureMacroPtr makeCaptureMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleCaptureMacro ( Module * module, CaptureMacroPtr & _newM, Context * );
    TypeMacroPtr makeTypeMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleTypeMacro ( Module * module, TypeMacroPtr & _newM, Context *, LineInfoArg * );
    SimulateMacroPtr makeSimulateMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleSimulateMacro ( Module * module, SimulateMacroPtr & _newM, Context * );
    void addModuleFunctionAnnotation ( Module * module, FunctionAnnotationPtr & ann, Context * context, LineInfoArg * at );
    FunctionAnnotationPtr makeFunctionAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    StructureAnnotationPtr makeStructureAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    EnumerationAnnotationPtr makeEnumerationAnnotation ( const char * name, void * pClass, const StructInfo * info, Context * context );
    void addModuleStructureAnnotation ( Module * module, StructureAnnotationPtr & ann, Context * context, LineInfoArg * at );
    void addStructureStructureAnnotation ( smart_ptr_raw<Structure> st, StructureAnnotationPtr & _ann, Context * context, LineInfoArg * at );
    void addModuleEnumerationAnnotation ( Module * module, EnumerationAnnotationPtr & ann, Context * context, LineInfoArg * at );
    int addEnumerationEntry ( smart_ptr<Enumeration> enu, const char* name );
    void forEachFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo );
    void forEachGenericFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo );
    bool addModuleFunction ( Module * module, FunctionPtr & func, Context * context, LineInfoArg * lineInfo );
    bool addModuleGeneric ( Module * module, FunctionPtr & func, Context * context, LineInfoArg * lineInfo );
    bool addModuleVariable ( Module * module, VariablePtr & var, Context * context, LineInfoArg * lineInfo );
    bool addModuleKeyword ( Module * module, char * kwd, bool needOxfordComma, Context * context, LineInfoArg * lineInfo );
    bool addModuleTypeFunction ( Module * module, char * kwd, Context * context, LineInfoArg * lineInfo );
    VariablePtr findModuleVariable ( Module * module, const char * name );
    bool removeModuleStructure ( Module * module, StructurePtr & _stru );
    bool addModuleStructure ( Module * module, StructurePtr & stru );
    bool addModuleAlias ( Module * module, TypeDeclPtr & _ptr );
    void ast_error ( ProgramPtr prog, const LineInfo & at, const char * message, Context * context, LineInfoArg * lineInfo );
    void addModuleReaderMacro ( Module * module, ReaderMacroPtr & newM, Context * context, LineInfoArg * lineInfo );
    ReaderMacroPtr makeReaderMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    CommentReaderPtr makeCommentReader ( const void * pClass, const StructInfo * info, Context * context );
    void addModuleCommentReader ( Module * module, CommentReaderPtr & _newM, Context * context, LineInfoArg * lineInfo );
    void addModuleCallMacro ( Module * module, CallMacroPtr & newM, Context * context, LineInfoArg * lineInfo );
    CallMacroPtr makeCallMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    TypeInfoMacroPtr makeTypeInfoMacro ( const char * name, const void * pClass, const StructInfo * info, Context * context );
    void addModuleTypeInfoMacro ( Module * module, TypeInfoMacroPtr & _newM, Context * context, LineInfoArg * at );
    void addFunctionFunctionAnnotation(smart_ptr_raw<Function> func, FunctionAnnotationPtr & ann, Context* context, LineInfoArg* at);
    void addAndApplyFunctionAnnotation ( smart_ptr_raw<Function> func, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    void addBlockBlockAnnotation ( smart_ptr_raw<ExprBlock> block, FunctionAnnotationPtr & _ann, Context * context, LineInfoArg * at );
    void addAndApplyBlockAnnotation ( smart_ptr_raw<ExprBlock> blk, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    void addAndApplyStructAnnotation ( smart_ptr_raw<Structure> st, smart_ptr_raw<AnnotationDeclaration> & ann, Context * context, LineInfoArg * at );
    __forceinline ExpressionPtr clone_expression ( ExpressionPtr value ) { return value ?value->clone() : nullptr; }
    __forceinline FunctionPtr clone_function ( FunctionPtr value ) { return value ? value->clone() : nullptr; }
    __forceinline TypeDeclPtr clone_type ( TypeDeclPtr value ) { return value ? make_smart<TypeDecl>(*value) : nullptr; }
    __forceinline StructurePtr clone_structure ( const Structure * value ) { return value ? value->clone() : nullptr; }
    __forceinline VariablePtr clone_variable ( VariablePtr value ) { return value ? value->clone() : nullptr; }
    void forceAtRaw ( const smart_ptr_raw<Expression> & expr, const LineInfo & at );
    void getAstContext ( smart_ptr_raw<Program> prog, smart_ptr_raw<Expression> expr, const TBlock<void,bool,AstContext> & block, Context * context, LineInfoArg * at );
    char * get_mangled_name ( smart_ptr_raw<Function> func, Context * context, LineInfoArg * at );
    char * get_mangled_name_t ( smart_ptr_raw<TypeDecl> typ, Context * context, LineInfoArg * at );
    char * get_mangled_name_v ( smart_ptr_raw<Variable> var, Context * context, LineInfoArg * at );
    char * get_mangled_name_b ( smart_ptr_raw<ExprBlock> expr, Context * context, LineInfoArg * at );
    TypeDeclPtr parseMangledNameFn ( const char * txt, ModuleGroup & lib, Module * thisModule, Context * context, LineInfoArg * at );
    void collectDependencies ( FunctionPtr fun, const TBlock<void,TArray<Function *>,TArray<Variable *>> & block, Context * context, LineInfoArg * line );
    bool isExprLikeCall ( const ExpressionPtr & expr );
    bool isExprConst ( const ExpressionPtr & expr );
    bool isTempType ( TypeDeclPtr ptr, bool refMatters );
    float4 evalSingleExpression ( const ExpressionPtr & expr, bool & ok );
    ExpressionPtr makeCall ( const LineInfo & at, const char * name );
    bool builtin_isVisibleDirectly ( Module * from, Module * too );
    bool builtin_hasField ( TypeDeclPtr ptr, const char * field, bool constant );
    TypeDeclPtr builtin_fieldType ( TypeDeclPtr ptr, const char * field, bool constant );
    Module * findRttiModule ( smart_ptr<Program> THAT_PROGRAM, const char * name, Context *, LineInfoArg *);
    smart_ptr<Function> findRttiFunction ( Module * mod, Func func, Context * context, LineInfoArg * line_info );
    void for_each_module ( Program * prog, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at );
    void for_each_typedef ( Module * mod, const TBlock<void,TTemporary<char *>,TypeDeclPtr> & block, Context * context, LineInfoArg * at );
    void for_each_enumeration ( Module * mod, const TBlock<void,EnumerationPtr> & block, Context * context, LineInfoArg * at );
    void for_each_structure ( Module * mod, const TBlock<void,StructurePtr> & block, Context * context, LineInfoArg * at );
    void for_each_generic ( Module * mod, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at );
    void for_each_global ( Module * mod, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at );
    void for_each_call_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at );
    void for_each_reader_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at );
    void for_each_variant_macro ( Module * mod, const TBlock<void,VariantMacroPtr> & block, Context * context, LineInfoArg * at );
    void for_each_typeinfo_macro ( Module * mod, const TBlock<void,TypeInfoMacroPtr> & block, Context * context, LineInfoArg * at );
    void for_each_for_loop_macro ( Module * mod, const TBlock<void,ForLoopMacroPtr> & block, Context * context, LineInfoArg * at );
    Annotation * get_expression_annotation ( Expression * expr, Context * context, LineInfoArg * at );
    Structure * find_unique_structure ( smart_ptr_raw<Program> prog, const char * name, Context * context, LineInfoArg * at );
    void get_use_global_variables ( smart_ptr_raw<Function> func, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at );
    void get_use_functions ( smart_ptr_raw<Function> func, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at );
    Structure::FieldDeclaration * ast_findStructureField ( Structure * structType, const char * field, Context * context, LineInfoArg * at );
    int32_t ast_getTupleFieldOffset ( smart_ptr_raw<TypeDecl> ttype, int32_t field, Context * context, LineInfoArg * at );
    void das_comp_log ( const char * text, Context * context, LineInfoArg * at );
    TypeInfo * das_make_type_info_structure ( Context & ctx, TypeDeclPtr ptr, Context * context, LineInfoArg * at );
    bool isSameAstType ( TypeDeclPtr THIS, TypeDeclPtr decl, RefMatters refMatters, ConstMatters constMatters, TemporaryMatters temporaryMatters, Context * context, LineInfoArg * at );
    void addModuleOption ( Module * mod, char * option, Type type, Context * context, LineInfoArg * at );
    TypeDeclPtr getUnderlyingValueType ( smart_ptr_raw<TypeDecl> type, Context * context, LineInfoArg * at );
    uint32_t getHandledTypeFieldOffset ( smart_ptr_raw<TypeAnnotation> type, char * name, Context * context, LineInfoArg * at );
    TypeInfo * getHandledTypeFieldType ( smart_ptr_raw<TypeAnnotation> annotation, char * name, Context * context, LineInfoArg * at );
    TypeDeclPtr getHandledTypeFieldTypeDecl ( smart_ptr_raw<TypeAnnotation> annotation, char * name, bool isConst, Context * context, LineInfoArg * at );
    bool addModuleRequire ( Module * module, Module * reqModule, bool publ );
    void findMatchingVariable ( Program * program, Function * func, const char * _name, bool seePrivate,
        const TBlock<void,TTemporary<TArray<VariablePtr>>> & block, Context * context, LineInfoArg * arg );
    Module * getCurrentSearchModule(Program * program, Function * func, const char * _moduleName);
    bool canAccessGlobalVariable ( const VariablePtr & pVar, Module * mod, Module * thisMod );
    TypeDeclPtr inferGenericTypeEx ( smart_ptr_raw<TypeDecl> type, smart_ptr_raw<TypeDecl> passType, bool topLevel, bool isPassType );
    void updateAliasMapEx ( smart_ptr_raw<Program> program, smart_ptr_raw<TypeDecl> argType, smart_ptr_raw<TypeDecl> passType, Context * context, LineInfoArg * at );

    template <>
    struct das_iterator <AnnotationArgumentList> : das_iterator<vector<AnnotationArgument>> {
        __forceinline das_iterator(AnnotationArgumentList & r)
            : das_iterator<vector<AnnotationArgument>>(r) {
        }
    };

    template <>
    struct das_iterator <const AnnotationArgumentList> : das_iterator<const vector<AnnotationArgument>> {
        __forceinline das_iterator(const AnnotationArgumentList & r)
            : das_iterator<const vector<AnnotationArgument>>(r) {
        }
    };
}

