#include "daScript/misc/platform.h"

#include "module_builtin_rtti.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/simulate/simulate_visit_op.h"

#include "daScript/misc/performance_time.h"

using namespace das;

IMPLEMENT_EXTERNAL_TYPE_FACTORY(FileInfo,FileInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(LineInfo,LineInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Annotation,Annotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(TypeAnnotation,TypeAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(BasicStructureAnnotation,BasicStructureAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(StructInfo,StructInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(EnumInfo,EnumInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(EnumValueInfo,EnumValueInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(TypeInfo,TypeInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(VarInfo,VarInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(LocalVariableInfo,LocalVariableInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(FuncInfo,FuncInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationArgument,AnnotationArgument)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationArguments,AnnotationArguments)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationArgumentList,AnnotationArgumentList)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationDeclaration,AnnotationDeclaration)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationList,AnnotationList)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Program,Program)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Module,Module)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Error,Error)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(FileAccess,FileAccess)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Context,Context)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(SimFunction,SimFunction)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CodeOfPolicies,CodeOfPolicies)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(recursive_mutex,das::recursive_mutex)

DAS_BASE_BIND_ENUM(das::CompilationError, CompilationError,
        unspecified
// lexer errors
    ,   mismatching_parentheses
    ,   mismatching_curly_bracers
    ,   string_constant_exceeds_file
    ,   string_constant_exceeds_line
    ,   unexpected_close_comment
    ,   integer_constant_out_of_range
    ,   comment_contains_eof
    ,   invalid_escape_sequence
    ,   invalid_line_directive
// parser errors
    ,   syntax_error
    ,   malformed_ast
// semantic erros
    ,   invalid_type
    ,   invalid_return_type
    ,   invalid_argument_type
    ,   invalid_structure_field_type
    ,   invalid_array_type
    ,   invalid_table_type
    ,   invalid_argument_count
    ,   invalid_variable_type
    ,   invalid_new_type
    ,   invalid_index_type
    ,   invalid_annotation
    ,   invalid_swizzle_mask
    ,   invalid_initialization_type
    ,   invalid_with_type
    ,   invalid_override
    ,   invalid_name
    ,   invalid_array_dimension
    ,   invalid_iteration_source
    ,   invalid_loop
    ,   invalid_label
    ,   invalid_enumeration
    ,   invalid_option
    ,   invalid_member_function

    ,   function_already_declared
    ,   argument_already_declared
    ,   local_variable_already_declared
    ,   global_variable_already_declared
    ,   structure_field_already_declared
    ,   structure_already_declared
    ,   structure_already_has_initializer
    ,   enumeration_already_declared
    ,   enumeration_value_already_declared
    ,   type_alias_already_declared
    ,   field_already_initialized

    ,   type_not_found
    ,   structure_not_found
    ,   operator_not_found
    ,   function_not_found
    ,   variable_not_found
    ,   handle_not_found
    ,   annotation_not_found
    ,   enumeration_not_found
    ,   enumeration_value_not_found
    ,   type_alias_not_found
    ,   bitfield_not_found

    ,   cant_initialize

    ,   cant_dereference
    ,   cant_index
    ,   cant_get_field
    ,   cant_write_to_const
    ,   cant_move_to_const
    ,   cant_write_to_non_reference
    ,   cant_copy
    ,   cant_move
    ,   cant_pass_temporary

    ,   condition_must_be_bool
    ,   condition_must_be_static

    ,   cant_pipe

    ,   invalid_block
    ,   return_or_break_in_finally

    ,   module_not_found
    ,   module_already_has_a_name

    ,   cant_new_handle
    ,   bad_delete

    ,   cant_infer_generic
    ,   cant_infer_missing_initializer
    ,   cant_infer_mismatching_restrictions

    ,   invalid_cast
    ,   incompatible_cast
    ,   unsafe

    ,   index_out_of_range

    ,   expecting_return_value
    ,   not_expecting_return_value
    ,   invalid_return_semantics
    ,   invalid_yield

    ,   typeinfo_reference
    ,   typeinfo_auto
    ,   typeinfo_undefined
    ,   typeinfo_dim
    ,   typeinfo_macro_error
// logic errors
    ,   static_assert_failed
    ,   run_failed
    ,   annotation_failed
    ,   concept_failed

    ,   not_all_paths_return_value
    ,   assert_with_side_effects
    ,   only_fast_aot_no_cpp_name
    ,   aot_side_effects
    ,   no_global_heap
    ,   no_global_variables
    ,   unused_function_argument
    ,   unsafe_function

    ,   too_many_infer_passes

// integration errors

    ,   missing_node
    )

das::FileAccessPtr get_file_access( char * pak );//link time resolved dependencies
das::Context * get_context ( int stackSize=0 );//link time resolved dependencies

namespace das {
    template <>
    struct das_default_vector_size<AnnotationArgumentList> {
        static __forceinline uint32_t size( const AnnotationArgumentList & value ) {
            return uint32_t(value.size());
        }
    };

    template <>
    struct typeFactory<RttiValue> {
        static TypeDeclPtr make(const ModuleLibrary & library ) {
            auto vtype = make_smart<TypeDecl>(Type::tVariant);
            vtype->alias = "RttiValue";
            vtype->aotAlias = true;
            vtype->addVariant("tBool",   typeFactory<decltype(RttiValue::bValue  )>::make(library));
            vtype->addVariant("tInt",    typeFactory<decltype(RttiValue::iValue  )>::make(library));
            vtype->addVariant("tUInt",   typeFactory<decltype(RttiValue::uValue  )>::make(library));
            vtype->addVariant("tInt64",  typeFactory<decltype(RttiValue::i64Value)>::make(library));
            vtype->addVariant("tUInt64", typeFactory<decltype(RttiValue::u64Value)>::make(library));
            vtype->addVariant("tFloat",  typeFactory<decltype(RttiValue::fValue  )>::make(library));
            vtype->addVariant("tDouble", typeFactory<decltype(RttiValue::dfValue )>::make(library));
            vtype->addVariant("tString", typeFactory<decltype(RttiValue::sValue  )>::make(library));
            vtype->addVariant("nothing", typeFactory<decltype(RttiValue::nothing )>::make(library));
            // optional validation
            DAS_ASSERT(sizeof(RttiValue) == vtype->getSizeOf());
            DAS_ASSERT(alignof(RttiValue) == vtype->getAlignOf());
            DAS_ASSERT(offsetof(RttiValue, bValue  ) == vtype->getVariantFieldOffset(0));
            DAS_ASSERT(offsetof(RttiValue, iValue  ) == vtype->getVariantFieldOffset(1));
            DAS_ASSERT(offsetof(RttiValue, uValue  ) == vtype->getVariantFieldOffset(2));
            DAS_ASSERT(offsetof(RttiValue, i64Value) == vtype->getVariantFieldOffset(3));
            DAS_ASSERT(offsetof(RttiValue, u64Value) == vtype->getVariantFieldOffset(4));
            DAS_ASSERT(offsetof(RttiValue, fValue  ) == vtype->getVariantFieldOffset(5));
            DAS_ASSERT(offsetof(RttiValue, dfValue ) == vtype->getVariantFieldOffset(6));
            DAS_ASSERT(offsetof(RttiValue, sValue  ) == vtype->getVariantFieldOffset(7));
            DAS_ASSERT(offsetof(RttiValue, nothing ) == vtype->getVariantFieldOffset(8));
            return vtype;
        }
    };

    TypeDeclPtr makeModuleFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ModuleFlags";
        ft->argNames = {
            "builtIn", "promoted", "isPublic", "isModule", "isSolidContext", "doNotAllowUnsafe"
        };
        return ft;
    }

    struct ModuleAnnotation : ManagedStructureAnnotation<Module,false> {
        ModuleAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Module", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addFieldEx ( "moduleFlags", "moduleFlags", offsetof(Module, moduleFlags), makeModuleFlags() );
        }
    };

    struct AstModuleGroupAnnotation : ManagedStructureAnnotation<ModuleGroup, true, true> {
        AstModuleGroupAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("ModuleGroup", ml) {
        }
    };


    struct FileAccessAnnotation : ManagedStructureAnnotation<FileAccess,false,true> {
        FileAccessAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("FileAccess", ml) {
        }
    };

    struct ErrorAnnotation : ManagedStructureAnnotation<Error> {
        ErrorAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Error", ml) {
            addField<DAS_BIND_MANAGED_FIELD(what)>("what");
            addField<DAS_BIND_MANAGED_FIELD(extra)>("extra");
            addField<DAS_BIND_MANAGED_FIELD(fixme)>("fixme");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(cerr)>("cerr");
        }
    };

    struct FileInfoAnnotation : ManagedStructureAnnotation<FileInfo,false> {
        FileInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("FileInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            // addProperty<DAS_BIND_MANAGED_PROP(getSource)>("source");
            // addField<DAS_BIND_MANAGED_FIELD(sourceLength)>("sourceLength");
            addField<DAS_BIND_MANAGED_FIELD(tabSize)>("tabSize");
        }
    };

    TypeDeclPtr makeContextCategoryFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "context_category_flags";
        ft->argNames = { "dead", "debug_context", "thread_clone", "job_clone", "opengl",
            "debugger_tick", "debugger_attached", "macro_context", "folding_context", "audio" };
        return ft;
    }

    template <>
    struct typeFactory<ContextCategory> {
        static TypeDeclPtr make(const ModuleLibrary &) {
            return makeContextCategoryFlags();
        }
    };

    struct ContextAnnotation : ManagedStructureAnnotation<Context,false> {
        ContextAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Context", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addFieldEx("category", "category", offsetof(Context, category), makeContextCategoryFlags());
            addField<DAS_BIND_MANAGED_FIELD(breakOnException)>("breakOnException");
            addField<DAS_BIND_MANAGED_FIELD(alwaysStackWalkOnException)>("alwaysStackWalkOnException");
            addField<DAS_BIND_MANAGED_FIELD(exception)>("exception");
            addField<DAS_BIND_MANAGED_FIELD(last_exception)>("last_exception");
            addField<DAS_BIND_MANAGED_FIELD(exceptionAt)>("exceptionAt");
            addField<DAS_BIND_MANAGED_FIELD(contextMutex)>("contextMutex");
            addProperty<DAS_BIND_MANAGED_PROP(getTotalFunctions)>("totalFunctions",
                "getTotalFunctions");
            addProperty<DAS_BIND_MANAGED_PROP(getTotalVariables)>("totalVariables",
                "getTotalVariables");
            addProperty<DAS_BIND_MANAGED_PROP(getCodeAllocatorId)>("getCodeAllocatorId",
                "getCodeAllocatorId");
        }
        virtual void walk ( das::DataWalker & walker, void * data ) override {
            if ( !walker.reading ) {
                if ( sizeof(intptr_t)==4 ) {
                    uint32_t T = uint32_t(intptr_t(data));
                    walker.UInt(T);
                } else {
                    uint64_t T = uint64_t(intptr_t(data));
                    walker.UInt64(T);
                }
            }
        }
    };

    TypeDeclPtr makeSimFunctionFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "SimFunctionFlags";
        ft->argNames = { "aot", "fastcall", "builtin", "jit", "unsafe", "cmres", "pinvoke" };
        return ft;
    }

    struct SimFunctionAnnotation : ManagedStructureAnnotation<SimFunction,false> {
        SimFunctionAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("SimFunction", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(mangledName)>("mangledName");
            addField<DAS_BIND_MANAGED_FIELD(debugInfo)>("debugInfo");
            addField<DAS_BIND_MANAGED_FIELD(stackSize)>("stackSize");
            addField<DAS_BIND_MANAGED_FIELD(mangledNameHash)>("mangledNameHash");
            addProperty<DAS_BIND_MANAGED_PROP(getLineInfo)>("lineInfo","getLineInfo");
            addFieldEx ( "flags", "flags", offsetof(SimFunction, flags), makeSimFunctionFlags() );
        }
    };

    struct LineInfoAnnotation : ManagedStructureAnnotation<LineInfo,false> {
        LineInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LineInfo", ml) {
            this->addField<DAS_BIND_MANAGED_FIELD(fileInfo)>("fileInfo");
            this->addField<DAS_BIND_MANAGED_FIELD(column)>("column");
            this->addField<DAS_BIND_MANAGED_FIELD(line)>("line");
            this->addField<DAS_BIND_MANAGED_FIELD(last_column)>("last_column");
            this->addField<DAS_BIND_MANAGED_FIELD(last_line)>("last_line");
        }
        virtual bool isLocal() const override { return true; }
    };

    TypeDeclPtr makeProgramFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "ProgramFlags";
        ft->argNames = { "failToCompile", "_unsafe", "isCompiling",
            "isSimulating", "isCompilingMacros", "needMacroModule"
        };
        return ft;
    }

    struct ProgramAnnotation : ManagedStructureAnnotation <Program,false,true> {
        ProgramAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Program", ml) {
            addField<DAS_BIND_MANAGED_FIELD(thisModuleName)>("thisModuleName");
            addFieldEx ( "flags", "flags", offsetof(Program, flags), makeProgramFlags() );
            addField<DAS_BIND_MANAGED_FIELD(errors)>("errors");
            addField<DAS_BIND_MANAGED_FIELD(options)>("_options","options");
        }
    };

    struct AnnotationArgumentAnnotation : ManagedStructureAnnotation <AnnotationArgument,false> {
        AnnotationArgumentAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("AnnotationArgument", ml) {
            addFieldEx ( "basicType", "type", offsetof(AnnotationArgument, type), makeType<Type>(ml) );
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(sValue)>("sValue");
            addField<DAS_BIND_MANAGED_FIELD(bValue)>("bValue");
            addField<DAS_BIND_MANAGED_FIELD(iValue)>("iValue");
            addField<DAS_BIND_MANAGED_FIELD(fValue)>("fValue");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
        }
    };

    TypeDeclPtr makeAnnotationDeclarationFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "AnnotationDeclarationFlags";
        ft->argNames = { "inherited" };
        return ft;
    }

    struct AnnotationDeclarationAnnotation : ManagedStructureAnnotation <AnnotationDeclaration,true> {
        AnnotationDeclarationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("AnnotationDeclaration", ml) {
                addField<DAS_BIND_MANAGED_FIELD(annotation)>("annotation");
                addField<DAS_BIND_MANAGED_FIELD(arguments)>("arguments");
                addField<DAS_BIND_MANAGED_FIELD(at)>("at");
                addFieldEx ( "flags", "flags", offsetof(AnnotationDeclaration, flags), makeAnnotationDeclarationFlags() );
                // TODO: function?
                // addProperty<DAS_BIND_MANAGED_PROP(getMangledName)>("getMangledName","getMangledName");
        }
    };

    struct TypeAnnotationAnnotation : ManagedStructureAnnotation <TypeAnnotation,false> {
        TypeAnnotationAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TypeAnnotation", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module","module");
            addProperty<DAS_BIND_MANAGED_PROP(isYetAnotherVectorTemplate)>("is_any_vector","isYetAnotherVectorTemplate");
            addProperty<DAS_BIND_MANAGED_PROP(canMove)>("canMove");
            addProperty<DAS_BIND_MANAGED_PROP(canCopy)>("canCopy");
            addProperty<DAS_BIND_MANAGED_PROP(canClone)>("canClone");
            addProperty<DAS_BIND_MANAGED_PROP(isPod)>("isPod");
            addProperty<DAS_BIND_MANAGED_PROP(isRawPod)>("isRawPod");
            addProperty<DAS_BIND_MANAGED_PROP(isRefType)>("isRefType");
            addProperty<DAS_BIND_MANAGED_PROP(hasNonTrivialCtor)>("hasNonTrivialCtor");
            addProperty<DAS_BIND_MANAGED_PROP(hasNonTrivialDtor)>("hasNonTrivialDtor");
            addProperty<DAS_BIND_MANAGED_PROP(hasNonTrivialCopy)>("hasNonTrivialCopy");
            addProperty<DAS_BIND_MANAGED_PROP(canBePlacedInContainer)>("canBePlacedInContainer");
            addProperty<DAS_BIND_MANAGED_PROP(isLocal)>("isLocal");
            addProperty<DAS_BIND_MANAGED_PROP(canNew)>("canNew");
            addProperty<DAS_BIND_MANAGED_PROP(canDelete)>("canDelete");
            addProperty<DAS_BIND_MANAGED_PROP(needDelete)>("needDelete");
            addProperty<DAS_BIND_MANAGED_PROP(canDeletePtr)>("canDeletePtr");
            addProperty<DAS_BIND_MANAGED_PROP(isIterable)>("isIterable");
            addProperty<DAS_BIND_MANAGED_PROP(isShareable)>("isShareable");
            addProperty<DAS_BIND_MANAGED_PROP(isSmart)>("isSmart");
            addProperty<DAS_BIND_MANAGED_PROP(avoidNullPtr)>("avoidNullPtr");
            addProperty<DAS_BIND_MANAGED_PROP(getSizeOf)>("sizeOf", "getSizeOf");
            addProperty<DAS_BIND_MANAGED_PROP(getAlignOf)>("alignOf", "getAlignOf");
        }
    };

    struct BasicStructureAnnotationAnnotation : ManagedStructureAnnotation <BasicStructureAnnotation,false> {
        BasicStructureAnnotationAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("BasicStructureAnnotation", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addProperty<DAS_BIND_MANAGED_PROP(fieldCount)>("fieldCount");
        }
    };


    struct AnnotationAnnotation : ManagedStructureAnnotation <Annotation,false> {
        AnnotationAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Annotation", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isHandledTypeAnnotation)>("isTypeAnnotation",
                "rtti_isHandledTypeAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isBasicStructureAnnotation)>("isBasicStructureAnnotation",
                "rtti_isBasicStructureAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isStructureAnnotation)>("isStructureAnnotation",
                "rtti_isStructureAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isStructureTypeAnnotation)>("isStructureTypeAnnotation",
                "rtti_isStructureTypeAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isFunctionAnnotation)>("isFunctionAnnotation",
                "rtti_isFunctionAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isEnumerationAnnotation)>("isEnumerationAnnotation",
                "rtti_isEnumerationAnnotation");
        }
    };

    struct EnumValueInfoAnnotation : ManagedStructureAnnotation <EnumValueInfo,false> {
        EnumValueInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("EnumValueInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
        }
    };

    template <typename ST>
    struct SimNode_DebugInfoAtField : SimNode_At {
        using TT = ST;
        DAS_PTR_NODE;
        SimNode_DebugInfoAtField ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t ofs )
            : SimNode_At(at, rv, idx, 0, ofs, 0) {}
        __forceinline char * compute ( Context & context ) {
            DAS_PROFILE_NODE
            auto pValue = (ST *) value->evalPtr(context);
            uint32_t idx = cast<uint32_t>::to(index->eval(context));
            if ( idx >= pValue->count ) {
                context.throw_error_at(debugInfo,"field index out of range, %u of %u", idx, pValue->count);
                return nullptr;
            } else {
                return ((char *)(pValue->fields[idx])) + offset;
            }
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP_TT(DebugInfoAtField);
            V_SUB(value);
            V_SUB(index);
            V_ARG(stride);
            V_ARG(offset);
            V_ARG(range);
            V_END();
        }
    };

    template <typename VT, typename ST>
    struct DebugInfoIterator : PointerDimIterator {
        DebugInfoIterator  ( ST * ar )
            : PointerDimIterator((char **)ar->fields, ar->count, sizeof(DebugInfoIterator<VT,ST>)) {}
    };

    template <typename VT, typename ST>
    struct DebugInfoAnnotation : ManagedStructureAnnotation <ST,false> {
        DebugInfoAnnotation(const string & st, ModuleLibrary & ml)
            : ManagedStructureAnnotation<ST,false> (st,ml) {
            using ManagedType = ST;
            this->template addField<DAS_BIND_MANAGED_FIELD(count)>("count");
        }
        virtual bool isIndexable ( const TypeDeclPtr & indexType ) const override {
            return indexType->isIndex();
        }
        virtual TypeDeclPtr makeIndexType ( const ExpressionPtr &, const ExpressionPtr & ) const override {
            return make_smart<TypeDecl>(*fieldType);
        }
        virtual SimNode * simulateGetAt ( Context & context, const LineInfo & at, const TypeDeclPtr &,
                                         const ExpressionPtr & rv, const ExpressionPtr & idx, uint32_t ofs ) const override {
            return context.code->makeNode<SimNode_DebugInfoAtField<ST>>(at,
                                                                                rv->simulate(context),
                                                                                idx->simulate(context),
                                                                                ofs);
        }
        virtual bool isIterable ( ) const override {
            return true;
        }
        virtual TypeDeclPtr makeIteratorType ( const ExpressionPtr & ) const override {
            return make_smart<TypeDecl>(*fieldType);
        }
        virtual SimNode * simulateGetIterator ( Context & context, const LineInfo & at, const ExpressionPtr & src ) const override {
            auto rv = src->simulate(context);
            return context.code->makeNode<SimNode_AnyIterator<ST,DebugInfoIterator<VT,ST>>>(at, rv);
        }
        TypeDeclPtr fieldType;
    };

    template <typename ST, typename VT>
    Sequence debugInfoIterator ( ST * st, Context * context ) {
        using StructIterator = DebugInfoIterator<VT,ST>;
        char * iter = context->heap->allocate(sizeof(StructIterator));
        context->heap->mark_comment(iter, "debug info iterator");
        new (iter) StructIterator(st);
        return { (Iterator *) iter };
    }

    struct EnumInfoAnnotation : DebugInfoAnnotation<EnumValueInfo,EnumInfo> {
        EnumInfoAnnotation(ModuleLibrary & ml) : DebugInfoAnnotation ("EnumInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(module_name)>("module_name");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            fieldType = makeType<EnumValueInfo>(*mlib);
            fieldType->ref = true;
        }
    };

    TSequence<EnumValueInfo> each_EnumInfo ( EnumInfo & st, Context * context ) {
        return debugInfoIterator<EnumInfo,EnumValueInfo>(&st, context);
    }

    TSequence<const EnumValueInfo> each_const_EnumInfo ( const EnumInfo & st, Context * context ) {
        return debugInfoIterator<EnumInfo,EnumValueInfo>((EnumInfo *)&st, context);
    }


    TypeDeclPtr makeStructInfoFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "StructInfoFlags";
        ft->argNames = { "_class", "_lambda", "heapGC", "stringHeapGC", "lockCheck" };
        return ft;
    }

    struct StructInfoAnnotation : DebugInfoAnnotation<VarInfo,StructInfo> {
        StructInfoAnnotation(ModuleLibrary & ml) : DebugInfoAnnotation ("StructInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(module_name)>("module_name");
            addFieldEx ( "flags", "flags", offsetof(StructInfo, flags), makeStructInfoFlags());
            addField<DAS_BIND_MANAGED_FIELD(size)>("size");
            addField<DAS_BIND_MANAGED_FIELD(init_mnh)>("init_mnh");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
        }
        void init () {
            fieldType = makeType<VarInfo>(*mlib);
            fieldType->ref = true;
        }
    };

    TSequence<VarInfo> each_StructInfo ( StructInfo & st, Context * context ) {
        return debugInfoIterator<StructInfo,VarInfo>(&st, context);
    }

    TSequence<const VarInfo> each_const_StructInfo ( const StructInfo & st, Context * context ) {
        return debugInfoIterator<StructInfo,VarInfo>((StructInfo *)&st, context);
    }

    TypeDeclPtr makeTypeInfoFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "TypeInfoFlags";
        ft->argNames = { "ref", "refType", "canCopy", "isPod", "isRawPod", "isConst", "isTemp", "isImplicit",
            "refValue", "hasInitValue", "isSmartPtr", "isSmartPtrNative", "isHandled",
            "heapGC", "stringHeapGC", "lockCheck" };
        return ft;
    }

    template <typename TT>
    struct ManagedTypeInfoAnnotation : ManagedStructureAnnotation <TT,false> {
        ManagedTypeInfoAnnotation ( const string & st, ModuleLibrary & ml )
            : ManagedStructureAnnotation<TT,false> (st, ml) {
            using ManagedType = TT;
            this->addFieldEx ( "basicType", "type", offsetof(TypeInfo, type), makeType<Type>(ml) );
            this->template addProperty<DAS_BIND_MANAGED_PROP(getEnumType)>("enumType", "getEnumType");
            this->template addField<DAS_BIND_MANAGED_FIELD(dimSize)>("dimSize");
            this->template addField<DAS_BIND_MANAGED_FIELD(argCount)>("argCount");
            this->addFieldEx ( "flags", "flags", offsetof(TT, flags), makeTypeInfoFlags());
            this->template addField<DAS_BIND_MANAGED_FIELD(size)>("size");
            this->template addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRef)>("isRef");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRefType)>("isRefType");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRefValue)>("isRefValue");
            this->template addProperty<DAS_BIND_MANAGED_PROP(canCopy)>("canCopy");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isPod)>("isPod");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRawPod)>("isRawPod");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isConst)>("isConst");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isTemp)>("isTemp");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isImplicit)>("isImplicit");
            this->template addProperty<DAS_BIND_MANAGED_PROP(getAnnotation)>("annotation","getAnnotation");
            this->template addField<DAS_BIND_MANAGED_FIELD(argNames)>("argNames");
        }
        void init() {
            // this needs to be initialized separately
            // reason is recursive type
            using ManagedType = TT;
            this->template addProperty<DAS_BIND_MANAGED_PROP(getStructType)>("structType", "getStructType");
            this->template addField<DAS_BIND_MANAGED_FIELD(firstType)>("firstType");
            this->template addField<DAS_BIND_MANAGED_FIELD(secondType)>("secondType");
            this->template addField<DAS_BIND_MANAGED_FIELD(argTypes)>("argTypes");
        }
    };

    struct TypeInfoAnnotation : ManagedTypeInfoAnnotation <TypeInfo> {
        TypeInfoAnnotation(ModuleLibrary & ml) : ManagedTypeInfoAnnotation ("TypeInfo", ml) {
        }
    };

    struct VarInfoAnnotation : ManagedTypeInfoAnnotation <VarInfo> {
        VarInfoAnnotation(ModuleLibrary & ml) : ManagedTypeInfoAnnotation ("VarInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
            addFieldEx ( "annotation_arguments", "annotation_arguments",
                        offsetof(VarInfo, annotation_arguments), makeType<const AnnotationArguments *>(ml) );
            // default values
            addField<DAS_BIND_MANAGED_FIELD(sValue)>("sValue");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            // from
            from("TypeInfo");
        }
    };

    TypeDeclPtr makeLocalVariableInfoFlagsFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "LocalVariableInfoFlags";
        ft->argNames = { "cmres" };
        return ft;
    }


    struct LocalVariableInfoAnnotation : ManagedTypeInfoAnnotation<LocalVariableInfo> {
        LocalVariableInfoAnnotation(ModuleLibrary & ml) : ManagedTypeInfoAnnotation ("LocalVariableInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(visibility)>("visibility");
            addFieldEx ( "localFlags", "localFlags", offsetof(LocalVariableInfo, localFlags), makeLocalVariableInfoFlagsFlags());
            from("TypeInfo");
        }
    };

    struct FuncInfoAnnotation : DebugInfoAnnotation<VarInfo,FuncInfo> {
        FuncInfoAnnotation(ModuleLibrary & ml) : DebugInfoAnnotation ("FuncInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(stackSize)>("stackSize");
            addField<DAS_BIND_MANAGED_FIELD(result)>("result");
            addField<DAS_BIND_MANAGED_FIELD(locals)>("locals");
            addField<DAS_BIND_MANAGED_FIELD(localCount)>("localCount");
            addField<DAS_BIND_MANAGED_FIELD(globals)>("globals");
            addField<DAS_BIND_MANAGED_FIELD(globalCount)>("globalCount");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
            fieldType = makeType<VarInfo>(*mlib);
            fieldType->ref = true;
        }
    };

    TSequence<VarInfo> each_FuncInfo ( FuncInfo & st, Context * context ) {
        return debugInfoIterator<FuncInfo,VarInfo>(&st, context);
    }

    TSequence<const VarInfo> each_const_FuncInfo ( const FuncInfo & st, Context * context ) {
        return debugInfoIterator<FuncInfo,VarInfo>((FuncInfo *)&st, context);
    }

    struct CodeOfPoliciesAnnotation : ManagedStructureAnnotation<CodeOfPolicies,false,false> {
        CodeOfPoliciesAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CodeOfPolicies", ml) {
        // aot
            addField<DAS_BIND_MANAGED_FIELD(aot)>("aot");
            addField<DAS_BIND_MANAGED_FIELD(aot_module)>("aot_module");
            addField<DAS_BIND_MANAGED_FIELD(completion)>("completion");
            addField<DAS_BIND_MANAGED_FIELD(export_all)>("export_all");
        // reporting
            addField<DAS_BIND_MANAGED_FIELD(always_report_candidates_threshold)>("always_report_candidates_threshold");
        // memory
            addField<DAS_BIND_MANAGED_FIELD(stack)>("stack");
            addField<DAS_BIND_MANAGED_FIELD(intern_strings)>("intern_strings");
            addField<DAS_BIND_MANAGED_FIELD(persistent_heap)>("persistent_heap");
            addField<DAS_BIND_MANAGED_FIELD(multiple_contexts)>("multiple_contexts");
            addField<DAS_BIND_MANAGED_FIELD(heap_size_hint)>("heap_size_hint");
            addField<DAS_BIND_MANAGED_FIELD(string_heap_size_hint)>("string_heap_size_hint");
            addField<DAS_BIND_MANAGED_FIELD(solid_context)>("solid_context");
            addField<DAS_BIND_MANAGED_FIELD(macro_context_persistent_heap)>("macro_context_persistent_heap");
            addField<DAS_BIND_MANAGED_FIELD(macro_context_collect)>("macro_context_collect");
        // rtti
            addField<DAS_BIND_MANAGED_FIELD(rtti)>("rtti");
        // language
            addField<DAS_BIND_MANAGED_FIELD(no_unsafe)>("no_unsafe");
            addField<DAS_BIND_MANAGED_FIELD(local_ref_is_unsafe)>("local_ref_is_unsafe");
            addField<DAS_BIND_MANAGED_FIELD(no_global_variables)>("no_global_variables");
            addField<DAS_BIND_MANAGED_FIELD(no_global_variables_at_all)>("no_global_variables_at_all");
            addField<DAS_BIND_MANAGED_FIELD(no_global_heap)>("no_global_heap");
            addField<DAS_BIND_MANAGED_FIELD(only_fast_aot)>("only_fast_aot");
            addField<DAS_BIND_MANAGED_FIELD(aot_order_side_effects)>("aot_order_side_effects");
            addField<DAS_BIND_MANAGED_FIELD(no_unused_function_arguments)>("no_unused_function_arguments");
            addField<DAS_BIND_MANAGED_FIELD(no_unused_block_arguments)>("no_unused_block_arguments");
            addField<DAS_BIND_MANAGED_FIELD(smart_pointer_by_value_unsafe)>("smart_pointer_by_value_unsafe");
            addField<DAS_BIND_MANAGED_FIELD(allow_block_variable_shadowing)>("allow_block_variable_shadowing");
            addField<DAS_BIND_MANAGED_FIELD(allow_local_variable_shadowing)>("allow_local_variable_shadowing");
            addField<DAS_BIND_MANAGED_FIELD(allow_shared_lambda)>("allow_shared_lambda");
            addField<DAS_BIND_MANAGED_FIELD(ignore_shared_modules)>("ignore_shared_modules");
            addField<DAS_BIND_MANAGED_FIELD(default_module_public)>("default_module_public");
            addField<DAS_BIND_MANAGED_FIELD(no_deprecated)>("no_deprecated");
            addField<DAS_BIND_MANAGED_FIELD(no_aliasing)>("no_aliasing");
            addField<DAS_BIND_MANAGED_FIELD(strict_smart_pointers)>("strict_smart_pointers");
            addField<DAS_BIND_MANAGED_FIELD(no_init)>("no_init");
            addField<DAS_BIND_MANAGED_FIELD(strict_unsafe_delete)>("strict_unsafe_delete");
            addField<DAS_BIND_MANAGED_FIELD(no_members_functions_in_struct)>("no_members_functions_in_struct");
            addField<DAS_BIND_MANAGED_FIELD(no_local_class_members)>("no_local_class_members");
        // environment
            addField<DAS_BIND_MANAGED_FIELD(no_optimizations)>("no_optimizations");
            addField<DAS_BIND_MANAGED_FIELD(fail_on_no_aot)>("fail_on_no_aot");
            addField<DAS_BIND_MANAGED_FIELD(fail_on_lack_of_aot_export)>("fail_on_lack_of_aot_export");
        // debugger
            addField<DAS_BIND_MANAGED_FIELD(debugger)>("debugger");
            addField<DAS_BIND_MANAGED_FIELD(debug_module)>("debug_module");
        // profiler
            addField<DAS_BIND_MANAGED_FIELD(profiler)>("profiler");
            addField<DAS_BIND_MANAGED_FIELD(profile_module)>("profile_module");
        // jit
            addField<DAS_BIND_MANAGED_FIELD(jit)>("jit");
            addField<DAS_BIND_MANAGED_FIELD(jit_module)>("jit_module");
        // threadlock context
            addField<DAS_BIND_MANAGED_FIELD(threadlock_context)>("threadlock_context");
        }
        virtual bool isLocal() const override { return true; }
    };

    template <typename TT>
    int32_t rtti_getDim ( const TT & ti, int32_t _index, Context * context, LineInfoArg * at ) {
        uint32_t index = _index;
        if ( ti.dimSize==0 ) {
            context->throw_error_at(at, "type is not an array");
        }
        if ( index>=ti.dimSize ) {
            context->throw_error_at(at, "dim index out of range, %u of %u", index, uint32_t(ti.dimSize));
        }
        return ti.dim[index];
    }

    int32_t rtti_getDimTypeInfo ( const TypeInfo & ti, int32_t index, Context * context, LineInfoArg * at ) {
        return rtti_getDim(ti, index, context, at);
    }

    int32_t rtti_getDimVarInfo ( const VarInfo & ti, int32_t index, Context * context, LineInfoArg * at ) {
        return rtti_getDim(ti, index, context, at);
    }

    int32_t rtti_contextTotalFunctions ( Context & context ) {
        return context.getTotalFunctions();
    }

    int32_t rtti_contextTotalVariables ( Context & context ) {
        return context.getTotalVariables();
    }

    vec4f rtti_contextFunctionInfo ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        Context * ctx = cast<Context *>::to(args[0]);
        int32_t tf = ctx->getTotalFunctions();
        int32_t index = cast<int32_t>::to(args[1]);
        if ( index<0 || index>=tf ) {
            context.throw_error_at(call->debugInfo, "function index out of range, %i of %i", index, tf);
        }
        FuncInfo * fi = ctx->getFunction(index)->debugInfo;
        return cast<FuncInfo *>::from(fi);
    }

    vec4f rtti_contextVariableInfo ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        Context * ctx = cast<Context *>::to(args[0]);
        int32_t tf = ctx->getTotalVariables();
        int32_t index = cast<int32_t>::to(args[1]);
        if ( index<0 || index>=tf ) {
            context.throw_error_at(call->debugInfo, "variable index out of range, %i of %i", index, tf);
        }
        return cast<VarInfo *>::from(ctx->getVariableInfo(index));
    }

    void rtti_builtin_simulate ( const smart_ptr<Program> & program,
            const TBlock<void,bool,smart_ptr<Context>,string> & block, Context * context, LineInfoArg * lineinfo ) {
        TextWriter issues;
        auto ctx = get_context(program->getContextStackSize());
        bool failed = !program->simulate(*ctx, issues);
        if ( failed ) {
            for ( auto & err : program->errors ) {
                issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            string istr = issues.str();
            das_invoke<void>::invoke<bool,smart_ptr<Context>,const string &>(context,lineinfo,block,false,nullptr,istr);
        } else {
            das_invoke<void>::invoke<bool,smart_ptr<Context>,const string &>(context,lineinfo,block,true,ctx,"");
        }
    }

    void rtti_builtin_compile ( char * modName, char * str, const CodeOfPolicies & cop,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at ) {
        return rtti_builtin_compile_ex(modName, str, cop, true, block, context, at);
    }

    void rtti_builtin_compile_ex ( char * modName, char * str, const CodeOfPolicies & cop, bool exportAll,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at ) {
        str = str ? str : ((char *)"");
        TextWriter issues;
        uint32_t str_len = stringLengthSafe(*context, str);
        auto access = make_smart<FileAccess>();
        auto fileInfo = make_unique<TextFileInfo>((char *) str, uint32_t(str_len), false);
        access->setFileInfo(modName, das::move(fileInfo));
        ModuleGroup dummyLibGroup;
        auto program = parseDaScript(modName, access, issues, dummyLibGroup, exportAll, false, cop);
        if ( program ) {
            if (program->failed()) {
                for (auto & err : program->errors) {
                    issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(false),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            } else {
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(true),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            }
        } else {
            context->throw_error_at(at, "rtti_compile internal error, something went wrong");
        }
    }

    Module * rtti_get_this_module ( smart_ptr_raw<Program> program ) {
        return program->thisModule.get();
    }

    Module * rtti_get_builtin_module ( const char * name ) {
        return Module::require(name);
    }

    void rtti_builtin_program_for_each_module ( smart_ptr_raw<Program> program, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at ) {
        program->library.foreach([&](Module * pm) -> bool {
            vec4f args[1] = { cast<Module *>::from(pm) };
            context->invoke(block, args, nullptr, at);
            return true;
        }, "*");
    }

    void rtti_builtin_program_for_each_registered_module ( const TBlock<void,Module *> & block, Context * context, LineInfoArg * at ) {
        Module::foreach([&](Module * pm) -> bool {
            vec4f args[1] = { cast<Module *>::from(pm) };
            context->invoke(block, args, nullptr, at);
            return true;
        });
    }

    void rtti_builtin_module_for_each_enumeration ( Module * module, const TBlock<void,const EnumInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->enumerations.foreach([&](auto penum){
            EnumInfo * info = helper.makeEnumDebugInfo(*penum);
            vec4f args[1] = {
                cast<EnumInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    RttiValue rtti_builtin_argument_value(const AnnotationArgument & info, Context * context ) {
        RttiValue nada;
        nada._variant = 8;  // nothing
        nada.nothing = v_zero();
        switch (info.type) {
        case Type::tBool:
            nada._variant = 0;
            nada.bValue = info.bValue;
            break;
        case Type::tInt:
            nada._variant = 1;
            nada.iValue = info.iValue;
            break;
        case Type::tFloat:
            nada._variant = 5;
            nada.fValue = info.fValue;
            break;
        case Type::tString:
            nada._variant = 7;
            nada.sValue = context->stringHeap->allocateString(info.sValue);
            break;
        default:;
        }
        return nada;
    }


    RttiValue rtti_builtin_variable_value(const VarInfo & info) {
        RttiValue nada;
        nada._variant = 8;  // nothing
        nada.nothing = v_zero();
        if (info.dimSize == 0 && (info.flags & TypeInfo::flag_hasInitValue)!=0 ) {
            switch (info.type) {
            case Type::tBool:   nada._variant = 0; break;
            case Type::tInt:    nada._variant = 1; break;
            case Type::tBitfield:
            case Type::tUInt:   nada._variant = 2; break;
            case Type::tInt64:  nada._variant = 3; break;
            case Type::tUInt64: nada._variant = 4; break;
            case Type::tFloat:  nada._variant = 5; break;
            case Type::tDouble: nada._variant = 6; break;
            case Type::tString: nada._variant = 7; break;
            default:;
            }
            if (nada._variant != 8) {
                if (nada._variant != 7) {
                    nada.nothing = info.value;
                } else {
                    nada.sValue = info.sValue;
                }
            }
        }
        return nada;
    }

    void rtti_builtin_module_for_each_structure ( Module * module, const TBlock<void,const StructInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->structures.foreach([&](auto structPtr){
            StructInfo * info = helper.makeStructureDebugInfo(*structPtr);
            vec4f args[1] = {
                cast<StructInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_structure_for_each_annotation ( const StructInfo & info, const Block & block, Context * context, LineInfoArg * at ) {
        if ( info.annotation_list ) {
            auto al = (const AnnotationList *) info.annotation_list;
            for ( const auto & adp : *al ) {
                vec4f args[2] = {
                    cast<Annotation *>::from(adp->annotation.get()),
                    cast<AnnotationArgumentList *>::from(&adp->arguments)
                };
                context->invoke(block, args, nullptr, at);
            }
        }
    }

    void rtti_builtin_module_for_each_function ( Module * module, const TBlock<void,const FuncInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->functions.foreach([&](auto funcPtr){
            FuncInfo * info = helper.makeFunctionDebugInfo(*funcPtr);
            vec4f args[1] = {
                cast<FuncInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_module_for_each_generic ( Module * module, const TBlock<void,const FuncInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->generics.foreach([&](auto funcPtr){
            FuncInfo * info = helper.makeFunctionDebugInfo(*funcPtr);
            vec4f args[1] = {
                cast<FuncInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_module_for_each_global ( Module * module, const TBlock<void,const VarInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->globals.foreach([&](auto var){
            VarInfo * info = helper.makeVariableDebugInfo(*var);
            vec4f args[1] = {
                cast<VarInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_module_for_each_annotation ( Module * module, const TBlock<void,const Annotation> & block, Context * context, LineInfoArg * at ) {
        module->handleTypes.foreach([&](auto annotationPtr){
            vec4f args[1] = {
                cast<Annotation*>::from(annotationPtr.get())
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_basic_struct_for_each_parent ( const BasicStructureAnnotation & ann, const TBlock<void,Annotation *> & block, Context * context, LineInfoArg * at ) {
        for ( auto & it : ann.parents ) {
            vec4f args[1] = { cast<Annotation *>::from(it) };
            context->invoke(block, args, nullptr, at);
        }
    }

    void rtti_builtin_basic_struct_for_each_field ( const BasicStructureAnnotation & ann,
        const TBlock<void,char *,char*,const TypeInfo,uint32_t> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        for ( auto & it : ann.fields ) {
            const auto & fld = it.second;
            TypeInfo * info = helper.makeTypeInfo(nullptr, fld.decl);
            vec4f args[4] = {
                cast<char *>::from(it.first.c_str()),
                cast<char *>::from(fld.cppName.c_str()),
                cast<TypeInfo *>::from(info),
                cast<uint32_t>::from(fld.offset)
            };
            context->invoke(block, args, nullptr, at);
        }
    }


    char * rtti_get_das_type_name(Type tt, Context * context) {
        string str = das_to_string(tt);
        return context->stringHeap->allocateString(str);
    }

    int rtti_add_annotation_argument(AnnotationArgumentList& list, const char* name) {
        list.emplace_back(name != nullptr ? name : "", 0);
        return (int)list.size() - 1;
    }

    bool rtti_add_file_access_root ( smart_ptr<FileAccess> access, const char * mod, const char * path ) {
        if ( !mod ) return false;
        if ( !path ) return false;
        return access->addFsRoot(mod, path);
    }

    void rtti_builtin_with_program_serialized ( const TBlock<void,ProgramPtr> & block,
            smart_ptr<Program> program, Context * context, LineInfoArg * at ) {
        // serialize
            AstSerializer ser;
            program->serialize(ser);
            program.reset();
        // deserialize
            AstSerializer deser ( ForReading{}, move(ser.buffer) );
            auto new_program = make_smart<Program>();
            new_program->serialize(deser);
            program = new_program;
            vec4f args[1] = { cast<smart_ptr<Program>>::from(program) };
            context->invoke(block, args, nullptr, at);
    }

#if !DAS_NO_FILEIO

    void rtti_builtin_compile_file ( char * modName, smart_ptr<FileAccess> access, ModuleGroup* module_group, const CodeOfPolicies & cop,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at ) {
        TextWriter issues;
        if ( !access ) access = make_smart<FsFileAccess>();
        auto program = compileDaScript(modName, access, issues, *module_group, cop);
        if ( program ) {
            if (program->failed()) {
                for (auto & err : program->errors) {
                    issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(false),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            } else {
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(true),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            }
        } else {
            context->throw_error_at(at, "rtti_compile internal error, something went wrong");
        }
    }

    smart_ptr<FileAccess> makeFileAccess( char * pak, Context *, LineInfoArg * ) {
        return get_file_access(pak);
    }

    bool introduceFile ( smart_ptr_raw<FileAccess> access, char * fname, char * str, Context * context, LineInfoArg * at ) {
        if ( !str ) context->throw_error_at(at, "can't introduce empty file");
        uint32_t str_len = stringLengthSafe(*context, str);
        auto fileInfo = make_unique<TextFileInfo>(str, str_len, false);
        return access->setFileInfo(fname, das::move(fileInfo)) != nullptr;
    }

#else
    smart_ptr<FileAccess> makeFileAccess( char *, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "not supported with DAS_NO_FILEIO");
        return nullptr;
    }

    void rtti_builtin_compile_file(  char *, smart_ptr<FileAccess>, ModuleGroup*, const CodeOfPolicies & cop,
            const TBlock<void, bool, smart_ptr<Program>, const string> &, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "not supported with DAS_NO_FILEIO");
    }

    bool introduceFile ( smart_ptr_raw<FileAccess>, char *, char *, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "not supported with DAS_NO_FILEIO");
        return false;
    }
#endif

    struct SimNode_RttiGetTypeDecl : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_RttiGetTypeDecl ( const LineInfo & at, const ExpressionPtr & d )
            : SimNode_CallBase(at) {
            typeExpr = d->type.get();
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(RttiGetTypeDecl);
            V_ARG(typeExpr->getMangledName().c_str());
            V_END();
        }
        __forceinline char * compute(Context &) {
            DAS_PROFILE_NODE
            return (char *) typeExpr;
        }
        TypeDecl *  typeExpr;   // requires RTTI
    };

    struct RttiTypeInfoMacro : TypeInfoMacro {
        RttiTypeInfoMacro() : TypeInfoMacro("rtti_typeinfo") {}
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, const ExpressionPtr &, string & ) override {
            return typeFactory<const TypeInfo>::make(lib);
        }
        virtual SimNode * simluate ( Context * context, const ExpressionPtr & expr, string & ) override {
            auto exprTypeInfo = static_pointer_cast<ExprTypeInfo>(expr);
            TypeInfo * typeInfo = context->thisHelper->makeTypeInfo(nullptr, exprTypeInfo->typeexpr);
            return context->code->makeNode<SimNode_TypeInfo>(expr->at, typeInfo);
        }
        virtual bool aotNeedTypeInfo ( const ExpressionPtr & ) const override {
            return true;
        }
    };

    LineInfo getCurrentLineInfo( LineInfoArg * lineInfo ) {
        return lineInfo ? *lineInfo : LineInfo();
    }

    char * builtin_print_data ( void * data, const TypeInfo * typeInfo, Bitfield flags, Context * context ) {
        TextWriter ssw;
        ssw << debug_value(data, (TypeInfo *)typeInfo, PrintFlags(uint32_t(flags)));
        return context->stringHeap->allocateString(ssw.str());
    }

    char * builtin_print_data_v ( float4 data, const TypeInfo * typeInfo, Bitfield flags, Context * context ) {
        TextWriter ssw;
        ssw << debug_value(vec4f(data), (TypeInfo *)typeInfo, PrintFlags(uint32_t(flags)));
        return context->stringHeap->allocateString(ssw.str());
    }

    char * builtin_debug_type ( const TypeInfo * typeInfo, Context * context ) {
        if ( !typeInfo ) return nullptr;
        auto dt = debug_type(typeInfo);
        return context->stringHeap->allocateString(dt);
    }

    char * builtin_debug_line ( const LineInfo & at, bool fully, Context * context ) {
        auto dt = at.describe(fully);
        return context->stringHeap->allocateString(dt);
    }

    char * builtin_get_typeinfo_mangled_name ( const TypeInfo * typeInfo, Context * context ) {
        if ( !typeInfo ) return nullptr;
        auto dt = getTypeInfoMangledName((TypeInfo*)typeInfo);
        return context->stringHeap->allocateString(dt);
    }

    const FuncInfo * builtin_get_function_info_by_mnh ( Context &, Func fun ) {
        if ( fun.PTR ) {
            return fun.PTR->debugInfo;
        } else {
            return nullptr;
        }
    }

    void builtin_expected_errors ( ProgramPtr prog, const TBlock<void,CompilationError,int> & block, Context * context, LineInfoArg * lineInfo ) {
        for ( auto er : prog->expectErrors ) {
            das_invoke<void>::invoke<CompilationError,int>(context,lineInfo,block,er.first,er.second);
        }
    }

    void builtin_list_require ( ProgramPtr prog, const TBlock<void,Module *,TTemporary<const char *>,TTemporary<const char *>,bool,const LineInfo &> & block,
            Context * context, LineInfoArg * lineInfo ) {
        for ( auto & allR : prog->allRequireDecl ) {
            das_invoke<void>::invoke<Module *,const char *,const char *,bool,const LineInfo &>(context,lineInfo,block,
                get<0>(allR),get<1>(allR).c_str(),get<2>(allR).c_str(),get<3>(allR),get<4>(allR));
        }
    }


    Func builtin_SimFunction_by_MNH ( Context & context, uint64_t MNH ) {
        Func fn;
        fn.PTR = context.fnByMangledName(MNH);
        return fn;
    }

    LineInfo rtti_get_line_info ( int depth, Context * context, LineInfoArg * at ) {
    #if DAS_ENABLE_STACK_WALK
        char * sp = context->stack.ap();
        const LineInfo * lineAt = at;
        while (  sp < context->stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            // char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    // SP = context->stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
            }
            lineAt = info ? pp->line : pp->functionLine;
            sp += info ? info->stackSize : pp->stackSize;
            depth --;
            if ( depth==0 ) return lineAt ? *lineAt : LineInfo();
        }
        return LineInfo();
    #else
        return LineInfo();
    #endif
    }

    #include "rtti.das.inc"

    Func builtin_getFunctionByMnh ( uint64_t MNH, Context * context ) {
        return Func(context->fnByMangledName(MNH));
    }

    Func builtin_getFunctionByMnh_inContext ( uint64_t MNH, Context & context ) {
        return Func(context.fnByMangledName(MNH));
    }


    uint64_t builtin_getFunctionMnh ( Func func, Context * ) {
        return func.PTR ? func.PTR->mangledNameHash : 0;
    }

    void lockThisContext ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !context->contextMutex ) context->throw_error_at(lineInfo,"threadlock_context is not set");
        context->threadlock_context([&](){
            context->invoke(block, nullptr, nullptr, lineInfo);
        });
    }

    void lockAnyContext ( Context & ctx, const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !context->contextMutex ) context->throw_error_at(lineInfo,"threadlock_context is not set");
        ctx.threadlock_context([&](){
            context->invoke(block, nullptr, nullptr, lineInfo);
        });
    }

    void lockAnyMutex ( recursive_mutex & rm, const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        lock_guard<recursive_mutex> guard(rm);
        context->invoke(block, nullptr, nullptr, lineInfo);
    }

    uint64_t das_get_SimFunction_by_MNH ( uint64_t MNH, Context & context ) {
        return (uint64_t) context.fnByMangledName(MNH);
    }

    template <typename KeyType>
    int32_t tableFindValue ( Table * tab, vec4f _key, int32_t valueTypeSize, Context * context ) {
        auto key = cast<KeyType>::to(_key);
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        return thh.find(*tab, key, hfn);
    }

    int32_t rtti_getTablePtr ( void * _table, vec4f key, Type baseType, int valueTypeSize, Context * context, LineInfoArg * at ) {
        Table * tab = (Table *) _table;
        switch ( baseType ) {
            case Type::tBool:           return tableFindValue<bool>        (tab,key,valueTypeSize,context);
            case Type::tInt8:           return tableFindValue<int8_t>      (tab,key,valueTypeSize,context);
            case Type::tUInt8:          return tableFindValue<uint8_t>     (tab,key,valueTypeSize,context);
            case Type::tInt16:          return tableFindValue<int16_t>     (tab,key,valueTypeSize,context);
            case Type::tUInt16:         return tableFindValue<uint16_t>    (tab,key,valueTypeSize,context);
            case Type::tInt64:          return tableFindValue<int64_t>     (tab,key,valueTypeSize,context);
            case Type::tUInt64:         return tableFindValue<uint64_t>    (tab,key,valueTypeSize,context);
            case Type::tEnumeration:    return tableFindValue<int32_t>     (tab,key,valueTypeSize,context);
            case Type::tEnumeration8:   return tableFindValue<int8_t>      (tab,key,valueTypeSize,context);
            case Type::tEnumeration16:  return tableFindValue<int16_t>     (tab,key,valueTypeSize,context);
            case Type::tBitfield:       return tableFindValue<Bitfield>    (tab,key,valueTypeSize,context);
            case Type::tInt:            return tableFindValue<int32_t>     (tab,key,valueTypeSize,context);
            case Type::tInt2:           return tableFindValue<int2>        (tab,key,valueTypeSize,context);
            case Type::tInt3:           return tableFindValue<int3>        (tab,key,valueTypeSize,context);
            case Type::tInt4:           return tableFindValue<int4>        (tab,key,valueTypeSize,context);
            case Type::tUInt:           return tableFindValue<uint32_t>    (tab,key,valueTypeSize,context);
            case Type::tUInt2:          return tableFindValue<uint2>       (tab,key,valueTypeSize,context);
            case Type::tUInt3:          return tableFindValue<uint3>       (tab,key,valueTypeSize,context);
            case Type::tUInt4:          return tableFindValue<uint4>       (tab,key,valueTypeSize,context);
            case Type::tFloat:          return tableFindValue<float>       (tab,key,valueTypeSize,context);
            case Type::tFloat2:         return tableFindValue<float2>      (tab,key,valueTypeSize,context);
            case Type::tFloat3:         return tableFindValue<float3>      (tab,key,valueTypeSize,context);
            case Type::tFloat4:         return tableFindValue<float4>      (tab,key,valueTypeSize,context);
            case Type::tRange:          return tableFindValue<range>       (tab,key,valueTypeSize,context);
            case Type::tURange:         return tableFindValue<urange>      (tab,key,valueTypeSize,context);
            case Type::tRange64:        return tableFindValue<range64>     (tab,key,valueTypeSize,context);
            case Type::tURange64:       return tableFindValue<urange64>    (tab,key,valueTypeSize,context);
            case Type::tString:         return tableFindValue<char *>      (tab,key,valueTypeSize,context);
            case Type::tPointer:        return tableFindValue<void *>      (tab,key,valueTypeSize,context);
            default:
                context->throw_error_at(at,"rtti.getTablePtr: unsupported type '%s'", das_to_string(baseType).c_str());
        }
    }

    class Module_Rtti : public Module {
    public:
        template <typename RecAnn>
        void addRecAnnotation ( ModuleLibrary & lib ) {
            auto rec = make_smart<RecAnn>(lib);
            addAnnotation(rec);
            initRecAnnotation(rec, lib);
        }
        Module_Rtti() : Module("rtti") {
            DAS_PROFILE_SECTION("Module_Rtti");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // flags
            addAlias(makeProgramFlags());
            addAlias(makeContextCategoryFlags());
            addAlias(makeTypeInfoFlags());
            addAlias(makeStructInfoFlags());
            addAlias(makeModuleFlags());
            addAlias(makeAnnotationDeclarationFlags());
            // enums
            addEnumeration(make_smart<EnumerationCompilationError>());
            // type annotations
            addAnnotation(make_smart<FileInfoAnnotation>(lib));
            addAnnotation(make_smart<LineInfoAnnotation>(lib));
                addCtor<LineInfo>(*this,lib,"LineInfo","LineInfo");
                addCtor<LineInfo,FileInfo *,int,int,int,int>(*this,lib,"LineInfo","LineInfo");
            addAnnotation(make_smart<DummyTypeAnnotation>("recursive_mutex","recursive_mutex",sizeof(recursive_mutex),alignof(recursive_mutex)));
            addUsing<recursive_mutex>(*this, lib, "das::recursive_mutex");
            addAnnotation(make_smart<ContextAnnotation>(lib));
            addAnnotation(make_smart<ErrorAnnotation>(lib));
            addAnnotation(make_smart<FileAccessAnnotation>(lib));
            addAnnotation(make_smart<ModuleAnnotation>(lib));
            addAnnotation(make_smart<AstModuleGroupAnnotation>(lib));
            addEnumeration(make_smart<EnumerationType>());
            addAnnotation(make_smart<AnnotationArgumentAnnotation>(lib));
            addVectorAnnotation<AnnotationArguments>(this,lib,"AnnotationArguments");
            addVectorAnnotation<AnnotationArgumentList>(this,lib,"AnnotationArgumentList");
            addAnnotation(make_smart<ProgramAnnotation>(lib));
            addAnnotation(make_smart<AnnotationAnnotation>(lib));
            addAnnotation(make_smart<AnnotationDeclarationAnnotation>(lib));
            addVectorAnnotation<AnnotationList>(this,lib,"AnnotationList");
            addAnnotation(make_smart<TypeAnnotationAnnotation>(lib));
            addAnnotation(make_smart<BasicStructureAnnotationAnnotation>(lib));
            addAnnotation(make_smart<EnumValueInfoAnnotation>(lib));
            addAnnotation(make_smart<EnumInfoAnnotation>(lib));
            addEnumeration(make_smart<EnumerationRefMatters>());
            addEnumeration(make_smart<EnumerationConstMatters>());
            addEnumeration(make_smart<EnumerationTemporaryMatters>());
            auto sia = make_smart<StructInfoAnnotation>(lib);              // this is type forward decl
            addAnnotation(sia);
            addRecAnnotation<TypeInfoAnnotation>(lib);
            addRecAnnotation<VarInfoAnnotation>(lib);
            addRecAnnotation<LocalVariableInfoAnnotation>(lib);
            initRecAnnotation(sia, lib);
            addAnnotation(make_smart<FuncInfoAnnotation>(lib));
            addAnnotation(make_smart<SimFunctionAnnotation>(lib));
            // CodeOfPolicies
            addAnnotation(make_smart<CodeOfPoliciesAnnotation>(lib));
            addCtorAndUsing<CodeOfPolicies>(*this,lib,"CodeOfPolicies","CodeOfPolicies");
            // RttiValue
            addAlias(typeFactory<RttiValue>::make(lib));
            // func info flags
            addConstant<uint32_t>(*this, "FUNCINFO_INIT", uint32_t(FuncInfo::flag_init));
            addConstant<uint32_t>(*this, "FUNCINFO_BUILTIN", uint32_t(FuncInfo::flag_builtin));
            addConstant<uint32_t>(*this, "FUNCINFO_PRIVATE", uint32_t(FuncInfo::flag_private));
            addConstant<uint32_t>(*this, "FUNCINFO_SHUTDOWN", uint32_t(FuncInfo::flag_shutdown));
            addConstant<uint32_t>(*this, "FUNCINFO_LATE_INIT", uint32_t(FuncInfo::flag_late_init));
            // macros
            addTypeInfoMacro(make_smart<RttiTypeInfoMacro>());
            // ctors
            addUsing<ModuleGroup>(*this, lib, "ModuleGroup");
            // functions
            //      all the stuff is only resolved after debug info is built
            //      hence SideEffects::modifyExternal is essential for it to not be optimized out
            addExtern<DAS_BIND_FUN(rtti_getDimTypeInfo)>(*this, lib, "get_dim",
                SideEffects::modifyExternal, "rtti_getDimTypeInfo")
                    ->args({"typeinfo","index","context","at"});
            addExtern<DAS_BIND_FUN(rtti_getDimVarInfo)>(*this, lib, "get_dim",
                SideEffects::modifyExternal, "rtti_getDimVarInfo")
                    ->args({"typeinfo","index","context","at"});
            addExtern<DAS_BIND_FUN(rtti_contextTotalFunctions)>(*this, lib, "get_total_functions",
                SideEffects::modifyExternal, "rtti_contextTotalFunctions")
                    ->arg("context");
            addExtern<DAS_BIND_FUN(rtti_contextTotalVariables)>(*this, lib, "get_total_variables",
                SideEffects::modifyExternal, "rtti_contextTotalVariables")
                    ->arg("context");
            addInterop<rtti_contextFunctionInfo,const FuncInfo &,vec4f,int32_t>(*this, lib, "get_function_info",
                SideEffects::modifyExternal, "rtti_contextFunctionInfo")
                    ->args({"context","index"});
            addInterop<rtti_contextVariableInfo,const VarInfo &,vec4f,int32_t>(*this, lib, "get_variable_info",
                SideEffects::modifyExternal, "rtti_contextVariableInfo")
                    ->args({"context","index"});
            addExtern<DAS_BIND_FUN(rtti_get_this_module)>(*this, lib, "get_this_module",
                SideEffects::modifyExternal, "rtti_get_this_module")
                    ->arg("program");
            addExtern<DAS_BIND_FUN(rtti_get_builtin_module)>(*this, lib, "get_module",
                SideEffects::modifyExternal, "rtti_get_builtin_module")
                    ->arg("name");
            addExtern<DAS_BIND_FUN(rtti_builtin_compile)>(*this, lib, "compile",
                SideEffects::modifyExternal, "rtti_builtin_compile")
                    ->args({"module_name","codeText","codeOfPolicies","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_compile_ex)>(*this, lib, "compile",
                SideEffects::modifyExternal, "rtti_builtin_compile_ex")
                    ->args({"module_name","codeText","codeOfPolicies","exportAll","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_compile_file)>(*this, lib, "compile_file",
                SideEffects::modifyExternal, "rtti_builtin_compile_file")
                    ->args({"module_name","fileAccess","moduleGroup","codeOfPolicies","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_with_program_serialized)>(*this, lib, "with_program_serialized",
                SideEffects::modifyExternal, "rtti_builtin_with_program_serialized")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_expected_errors)>(*this, lib, "for_each_expected_error",
                SideEffects::modifyExternal, "builtin_expected_errors")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_list_require)>(*this, lib, "for_each_require_declaration",
                SideEffects::modifyExternal, "builtin_list_require")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_simulate)>(*this, lib, "simulate",
                SideEffects::modifyExternal, "rtti_builtin_simulate")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(makeFileAccess)>(*this, lib, "make_file_access",
                SideEffects::modifyExternal, "makeFileAccess")
                    ->args({"project","context","at"});
            addExtern<DAS_BIND_FUN(introduceFile)>(*this, lib, "set_file_source",
                SideEffects::modifyExternal, "introduceFile")
                    ->args({"access","fileName","text","context","line"});
            addExtern<DAS_BIND_FUN(rtti_add_file_access_root)>(*this, lib, "add_file_access_root",
                SideEffects::modifyExternal, "rtti_add_file_access_root")
                    ->args({"access","mod","path"});
            addExtern<DAS_BIND_FUN(rtti_builtin_program_for_each_module)>(*this, lib, "program_for_each_module",
                SideEffects::modifyExternal, "rtti_builtin_program_for_each_module")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_program_for_each_registered_module)>(*this, lib, "program_for_each_registered_module",
                SideEffects::modifyExternal, "rtti_builtin_program_for_each_registered_module")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_structure)>(*this, lib, "module_for_each_structure",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_structure")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_variable_value),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_variable_value",
                SideEffects::modifyExternal, "rtti_builtin_variable_value")
                    ->arg("varInfo");
            addExtern<DAS_BIND_FUN(rtti_builtin_argument_value),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_annotation_argument_value",
                SideEffects::modifyExternal, "rtti_builtin_argument_value")
                    ->args({"info","context"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_enumeration)>(*this, lib, "module_for_each_enumeration",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_enumeration")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_function)>(*this, lib, "module_for_each_function",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_function")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_generic)>(*this, lib, "module_for_each_generic",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_generic")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_global)>(*this, lib, "module_for_each_global",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_global")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_annotation)>(*this, lib, "module_for_each_annotation",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_annotation")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_structure_for_each_annotation)>(*this, lib, "rtti_builtin_structure_for_each_annotation",
                SideEffects::modifyExternal, "rtti_builtin_structure_for_each_annotation")
                    ->args({"struct","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_basic_struct_for_each_field)>(*this, lib, "basic_struct_for_each_field",
                SideEffects::invokeAndAccessExternal, "rtti_builtin_basic_struct_for_each_field")
                    ->args({"annotation","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_basic_struct_for_each_parent)>(*this, lib, "basic_struct_for_each_parent",
                SideEffects::invokeAndAccessExternal, "rtti_builtin_basic_struct_for_each_parent")
                    ->args({"annotation","block","context","line"});
            addExtern<DAS_BIND_FUN(isSameType)>(*this, lib, "builtin_is_same_type",
                SideEffects::modifyExternal, "isSameType")
                    ->args({"a","b","refMatters","cosntMatters","tempMatters","topLevel"});
            addExtern<DAS_BIND_FUN(getTypeSize)>(*this, lib, "get_type_size",
                SideEffects::none, "getTypeSize")
                    ->args({"type"});
            addExtern<DAS_BIND_FUN(getTypeAlign)>(*this, lib, "get_type_align",
                SideEffects::none, "getTypeAlign")
                    ->args({"type"});
            addExtern<DAS_BIND_FUN(getTupleFieldOffset)>(*this, lib, "get_tuple_field_offset",
                SideEffects::none, "getTupleFieldOffset")
                    ->args({"type", "index"});
            addExtern<DAS_BIND_FUN(getVariantFieldOffset)>(*this, lib, "get_variant_field_offset",
                SideEffects::none, "getVariantFieldOffset")
                    ->args({"type", "index"});
            addExtern<DAS_BIND_FUN(isCompatibleCast)>(*this, lib, "is_compatible_cast",
                SideEffects::modifyExternal, "isCompatibleCast")
                    ->args({"from","to"});
            addExtern<DAS_BIND_FUN(rtti_get_das_type_name)>(*this, lib,  "get_das_type_name",
                SideEffects::none, "rtti_get_das_type_name")
                    ->args({"type","context"});
            addExtern<DAS_BIND_FUN(rtti_add_annotation_argument)>(*this, lib,  "add_annotation_argument",
                SideEffects::none, "add_annotation_argument")
                    ->args({"annotation","name"});
            // data printer
            addExtern<DAS_BIND_FUN(builtin_print_data)>(*this, lib, "sprint_data",
                SideEffects::none, "builtin_print_data")
                    ->args({"data","type","flags","context"});
            addExtern<DAS_BIND_FUN(builtin_print_data_v)>(*this, lib, "sprint_data",
                SideEffects::none, "builtin_print_data_v")
                    ->args({"data","type","flags","context"});
            // debug typeinfo
            addExtern<DAS_BIND_FUN(builtin_debug_type)>(*this, lib, "describe",
                SideEffects::none, "builtin_debug_type")
                    ->args({"type","context"});
            auto dl = addExtern<DAS_BIND_FUN(builtin_debug_line)>(*this, lib, "describe",
                SideEffects::none, "builtin_debug_line_info")
                    ->args({"lineinfo","fully","context"});
            dl->arguments[1]->init = make_smart<ExprConstBool>(false);
            addExtern<DAS_BIND_FUN(builtin_get_typeinfo_mangled_name)>(*this, lib, "get_mangled_name",
                SideEffects::none, "getTypeInfoMangledName")
                    ->args({"type","context"});
            // function mnh lookup
            addExtern<DAS_BIND_FUN(builtin_get_function_info_by_mnh)>(*this, lib, "get_function_info",
                SideEffects::none, "builtin_get_function_info_by_mnh")
                    ->args({"context","function"});
            addExtern<DAS_BIND_FUN(builtin_SimFunction_by_MNH)>(*this, lib, "get_function_by_mnh",
                SideEffects::none, "builtin_SimFunction_by_MNH")
                    ->args({"context","MNH"});
            // current line info
            addExtern<DAS_BIND_FUN(getCurrentLineInfo), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
                "get_line_info", SideEffects::none, "getCurrentLineInfo")->arg("line");
            addExtern<DAS_BIND_FUN(rtti_get_line_info), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_line_info",
                SideEffects::modifyExternal, "rtti_get_line_info")
                    ->args({"depth","context","line"});
            // this context
            addExtern<DAS_BIND_FUN(thisContext), SimNode_ExtFuncCallRef>(*this, lib,  "this_context",
                SideEffects::accessExternal, "thisContext")->arg("context");
            // fn-mnh
            addExtern<DAS_BIND_FUN(builtin_getFunctionByMnh)>(*this, lib, "get_function_by_mangled_name_hash",
                SideEffects::none, "builtin_getFunctionByMnh")
                    ->args({"src","context"});
            addExtern<DAS_BIND_FUN(builtin_getFunctionByMnh_inContext)>(*this, lib, "get_function_by_mangled_name_hash",
                SideEffects::none, "builtin_getFunctionByMnh_inContext")
                    ->args({"src","context"});
            addExtern<DAS_BIND_FUN(builtin_getFunctionMnh)>(*this, lib, "get_function_mangled_name_hash",
                SideEffects::none, "builtin_getFunctionMnh")
                    ->args({"src","context"});
            // mutex & lock
            addExtern<DAS_BIND_FUN(lockThisContext)>(*this, lib, "lock_this_context",
                SideEffects::worstDefault, "lockThisContext")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(lockAnyContext)>(*this, lib, "lock_context",
                SideEffects::worstDefault, "lockAnyContext")
                    ->args({"lock_context","block","context","line"});
            addExtern<DAS_BIND_FUN(lockAnyMutex)>(*this, lib,  "lock_mutex",
                SideEffects::worstDefault, "lockAnyMutex")
                    ->args({"mutex","block","context","line"});
            // in context
            addExtern<DAS_BIND_FUN(das_get_SimFunction_by_MNH)>(*this, lib, "get_function_address",
                SideEffects::none, "das_get_SimFunction_by_MNH")
                    ->args({"MNH","at"});
            // table key index
            addExtern<DAS_BIND_FUN(rtti_getTablePtr)>(*this, lib, "get_table_key_index",
                SideEffects::none, "rtti_getTablePtr")
                    ->args({"table","key","baseType","valueTypeSize","context","at"});
            // 'each' iterators for jit
            addExtern<DAS_BIND_FUN(each_FuncInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_FuncInfo")
                    ->args({"info","context"});
            addExtern<DAS_BIND_FUN(each_const_FuncInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_const_FuncInfo")
                    ->args({"info","context"});
            addExtern<DAS_BIND_FUN(each_StructInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_StructInfo")
                    ->args({"info","context"});
            addExtern<DAS_BIND_FUN(each_const_StructInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_const_StructInfo")
                    ->args({"info","context"});
            addExtern<DAS_BIND_FUN(each_EnumInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_EnumInfo")
                    ->args({"info","context"});
            addExtern<DAS_BIND_FUN(each_const_EnumInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_const_EnumInfo")
                    ->args({"info","context"});
            // add builtin module
            compileBuiltinModule("rtti.das",rtti_das, sizeof(rtti_das));
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_rtti.h\"\n";
            tw << "#include \"daScript/ast/ast.h\"\n";
            tw << "#include \"daScript/ast/ast_handle.h\"\n";
            return ModuleAotType::hybrid;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Rtti,das);
