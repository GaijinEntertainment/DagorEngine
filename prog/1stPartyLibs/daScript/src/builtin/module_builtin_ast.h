#pragma once

MAKE_EXTERNAL_TYPE_FACTORY(TypeDecl,das::TypeDecl)
MAKE_EXTERNAL_TYPE_FACTORY(FieldDeclaration,das::Structure::FieldDeclaration)
MAKE_EXTERNAL_TYPE_FACTORY(Structure,das::Structure)
MAKE_EXTERNAL_TYPE_FACTORY(EnumEntry,das::Enumeration::EnumEntry)
MAKE_EXTERNAL_TYPE_FACTORY(Enumeration,das::Enumeration)
MAKE_EXTERNAL_TYPE_FACTORY(Expression,das::Expression)
MAKE_EXTERNAL_TYPE_FACTORY(Function,das::Function)
MAKE_EXTERNAL_TYPE_FACTORY(InferHistory,das::InferHistory)
MAKE_EXTERNAL_TYPE_FACTORY(Variable,das::Variable)
MAKE_EXTERNAL_TYPE_FACTORY(VisitorAdapter,das::VisitorAdapter)
MAKE_EXTERNAL_TYPE_FACTORY(FunctionAnnotation,das::FunctionAnnotation)
MAKE_EXTERNAL_TYPE_FACTORY(StructureAnnotation,das::StructureAnnotation)
MAKE_EXTERNAL_TYPE_FACTORY(EnumerationAnnotation,das::EnumerationAnnotation)
MAKE_EXTERNAL_TYPE_FACTORY(PassMacro,das::PassMacro)
MAKE_EXTERNAL_TYPE_FACTORY(VariantMacro,das::VariantMacro)
MAKE_EXTERNAL_TYPE_FACTORY(ForLoopMacro,das::ForLoopMacro)
MAKE_EXTERNAL_TYPE_FACTORY(CaptureMacro,das::CaptureMacro)
MAKE_EXTERNAL_TYPE_FACTORY(TypeMacro,das::TypeMacro)
MAKE_EXTERNAL_TYPE_FACTORY(SimulateMacro,das::SimulateMacro)
MAKE_EXTERNAL_TYPE_FACTORY(ReaderMacro,das::ReaderMacro)
MAKE_EXTERNAL_TYPE_FACTORY(CommentReader,das::CommentReader)
MAKE_EXTERNAL_TYPE_FACTORY(CallMacro,das::CallMacro)
MAKE_EXTERNAL_TYPE_FACTORY(ModuleLibrary,das::ModuleLibrary)
MAKE_EXTERNAL_TYPE_FACTORY(AstContext,das::AstContext)

MAKE_EXTERNAL_TYPE_FACTORY(ExprBlock,das::ExprBlock)
MAKE_EXTERNAL_TYPE_FACTORY(ExprLet,das::ExprLet)
MAKE_EXTERNAL_TYPE_FACTORY(ExprStringBuilder,das::ExprStringBuilder)
MAKE_EXTERNAL_TYPE_FACTORY(ExprNew,das::ExprNew)
MAKE_EXTERNAL_TYPE_FACTORY(ExprNamedCall,das::ExprNamedCall)
MAKE_EXTERNAL_TYPE_FACTORY(MakeFieldDecl,das::MakeFieldDecl)
MAKE_EXTERNAL_TYPE_FACTORY(MakeStruct,das::MakeStruct)
MAKE_EXTERNAL_TYPE_FACTORY(ExprCall,das::ExprCall)
MAKE_EXTERNAL_TYPE_FACTORY(ExprCallFunc,das::ExprCallFunc)
MAKE_EXTERNAL_TYPE_FACTORY(ExprLooksLikeCall,das::ExprLooksLikeCall)
MAKE_EXTERNAL_TYPE_FACTORY(ExprNullCoalescing,das::ExprNullCoalescing)
MAKE_EXTERNAL_TYPE_FACTORY(ExprAt,das::ExprAt)
MAKE_EXTERNAL_TYPE_FACTORY(ExprSafeAt,das::ExprSafeAt)
MAKE_EXTERNAL_TYPE_FACTORY(ExprPtr2Ref,das::ExprPtr2Ref)
MAKE_EXTERNAL_TYPE_FACTORY(ExprIs,das::ExprIs)
MAKE_EXTERNAL_TYPE_FACTORY(ExprOp,das::ExprOp)
MAKE_EXTERNAL_TYPE_FACTORY(ExprOp2,das::ExprOp2)
MAKE_EXTERNAL_TYPE_FACTORY(ExprOp3,das::ExprOp3)
MAKE_EXTERNAL_TYPE_FACTORY(ExprCopy,das::ExprCopy)
MAKE_EXTERNAL_TYPE_FACTORY(ExprMove,das::ExprMove)
MAKE_EXTERNAL_TYPE_FACTORY(ExprClone,das::ExprClone)
MAKE_EXTERNAL_TYPE_FACTORY(ExprWith,das::ExprWith)
MAKE_EXTERNAL_TYPE_FACTORY(ExprAssume,das::ExprAssume)
MAKE_EXTERNAL_TYPE_FACTORY(ExprWhile,das::ExprWhile)
MAKE_EXTERNAL_TYPE_FACTORY(ExprTryCatch,das::ExprTryCatch)
MAKE_EXTERNAL_TYPE_FACTORY(ExprIfThenElse,das::ExprIfThenElse)
MAKE_EXTERNAL_TYPE_FACTORY(ExprFor,das::ExprFor)
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeLocal,das::ExprMakeLocal)
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeVariant,das::ExprMakeVariant)
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeStruct,das::ExprMakeStruct)
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeArray,das::ExprMakeArray)
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeTuple,das::ExprMakeTuple)
MAKE_EXTERNAL_TYPE_FACTORY(ExprArrayComprehension,das::ExprArrayComprehension)
MAKE_EXTERNAL_TYPE_FACTORY(TypeInfoMacro,das::TypeInfoMacro);
MAKE_EXTERNAL_TYPE_FACTORY(ExprTypeInfo,das::ExprTypeInfo)
MAKE_EXTERNAL_TYPE_FACTORY(ExprTypeDecl,das::ExprTypeDecl)
MAKE_EXTERNAL_TYPE_FACTORY(ExprLabel,das::ExprLabel);
MAKE_EXTERNAL_TYPE_FACTORY(ExprGoto,das::ExprGoto);
MAKE_EXTERNAL_TYPE_FACTORY(ExprRef2Value,das::ExprRef2Value);
MAKE_EXTERNAL_TYPE_FACTORY(ExprRef2Ptr,das::ExprRef2Ptr);
MAKE_EXTERNAL_TYPE_FACTORY(ExprAddr,das::ExprAddr);
MAKE_EXTERNAL_TYPE_FACTORY(ExprAssert,das::ExprAssert);
MAKE_EXTERNAL_TYPE_FACTORY(ExprStaticAssert,das::ExprStaticAssert);
MAKE_EXTERNAL_TYPE_FACTORY(ExprQuote,das::ExprQuote);
MAKE_EXTERNAL_TYPE_FACTORY(ExprDebug,das::ExprDebug);
MAKE_EXTERNAL_TYPE_FACTORY(ExprInvoke,das::ExprInvoke);
MAKE_EXTERNAL_TYPE_FACTORY(ExprErase,das::ExprErase);
MAKE_EXTERNAL_TYPE_FACTORY(ExprSetInsert,das::ExprSetInsert);
MAKE_EXTERNAL_TYPE_FACTORY(ExprFind,das::ExprFind);
MAKE_EXTERNAL_TYPE_FACTORY(ExprKeyExists,das::ExprKeyExists);
MAKE_EXTERNAL_TYPE_FACTORY(ExprAscend,das::ExprAscend);
MAKE_EXTERNAL_TYPE_FACTORY(ExprCast,das::ExprCast);
MAKE_EXTERNAL_TYPE_FACTORY(ExprDelete,das::ExprDelete);
MAKE_EXTERNAL_TYPE_FACTORY(ExprVar,das::ExprVar);
MAKE_EXTERNAL_TYPE_FACTORY(ExprTag,das::ExprTag);
MAKE_EXTERNAL_TYPE_FACTORY(ExprSwizzle,das::ExprSwizzle);
MAKE_EXTERNAL_TYPE_FACTORY(ExprField,das::ExprField);
MAKE_EXTERNAL_TYPE_FACTORY(ExprSafeField,das::ExprSafeField);
MAKE_EXTERNAL_TYPE_FACTORY(ExprIsVariant,das::ExprIsVariant);
MAKE_EXTERNAL_TYPE_FACTORY(ExprAsVariant,das::ExprAsVariant);
MAKE_EXTERNAL_TYPE_FACTORY(ExprSafeAsVariant,das::ExprSafeAsVariant);
MAKE_EXTERNAL_TYPE_FACTORY(ExprOp1,das::ExprOp1);
MAKE_EXTERNAL_TYPE_FACTORY(ExprReturn,das::ExprReturn);
MAKE_EXTERNAL_TYPE_FACTORY(ExprYield,das::ExprYield);
MAKE_EXTERNAL_TYPE_FACTORY(ExprBreak,das::ExprBreak);
MAKE_EXTERNAL_TYPE_FACTORY(ExprContinue,das::ExprContinue);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConst,das::ExprConst);
MAKE_EXTERNAL_TYPE_FACTORY(ExprFakeContext,das::ExprFakeContext);
MAKE_EXTERNAL_TYPE_FACTORY(ExprFakeLineInfo,das::ExprFakeLineInfo);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstPtr,das::ExprConstPtr);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstEnumeration,das::ExprConstEnumeration);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstBitfield,das::ExprConstBitfield);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt8,das::ExprConstInt8);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt16,das::ExprConstInt16);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt64,das::ExprConstInt64);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt,das::ExprConstInt);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt2,das::ExprConstInt2);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt3,das::ExprConstInt3);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstInt4,das::ExprConstInt4);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt8,das::ExprConstUInt8);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt16,das::ExprConstUInt16);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt64,das::ExprConstUInt64);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt,das::ExprConstUInt);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt2,das::ExprConstUInt2);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt3,das::ExprConstUInt3);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstUInt4,das::ExprConstUInt4);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstRange,das::ExprConstRange);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstURange,das::ExprConstURange);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstRange64,das::ExprConstRange64);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstURange64,das::ExprConstURange64);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstBool,das::ExprConstBool);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstFloat,das::ExprConstFloat);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstFloat2,das::ExprConstFloat2);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstFloat3,das::ExprConstFloat3);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstFloat4,das::ExprConstFloat4);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstString,das::ExprConstString);
MAKE_EXTERNAL_TYPE_FACTORY(ExprConstDouble,das::ExprConstDouble);
MAKE_EXTERNAL_TYPE_FACTORY(CaptureEntry,das::CaptureEntry);
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeBlock,das::ExprMakeBlock);
MAKE_EXTERNAL_TYPE_FACTORY(ExprMakeGenerator,das::ExprMakeGenerator);
MAKE_EXTERNAL_TYPE_FACTORY(ExprMemZero,das::ExprMemZero);
MAKE_EXTERNAL_TYPE_FACTORY(ExprReader,das::ExprReader);
MAKE_EXTERNAL_TYPE_FACTORY(ExprCallMacro,das::ExprCallMacro);
MAKE_EXTERNAL_TYPE_FACTORY(ExprUnsafe,das::ExprUnsafe);

DAS_BASE_BIND_ENUM(das::SideEffects, SideEffects,
    none, unsafe, userScenario, modifyExternal, accessExternal, modifyArgument,
    modifyArgumentAndExternal, worstDefault, accessGlobal, invoke, inferredSideEffects)

DAS_BASE_BIND_ENUM(das::CaptureMode, CaptureMode,
    capture_any, capture_by_copy, capture_by_reference, capture_by_clone, capture_by_move)

namespace das {

    class Module_Ast : public Module {
    public:
        template <typename TT>
        smart_ptr<TT> addExpressionAnnotation ( const smart_ptr<TT> & ann );
        Module_Ast();
        void registerFlags(ModuleLibrary & lib);
        void registerAdapterAnnotations(ModuleLibrary & lib);
        void registerAnnotations(ModuleLibrary & lib);
        void registerAnnotations1(ModuleLibrary & lib);
        void registerAnnotations2(ModuleLibrary & lib);
        void registerAnnotations3(ModuleLibrary & lib);
        void registerFunctions(ModuleLibrary & lib);
        void registerMacroExpressions(ModuleLibrary & lib);
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override;
   };

    TypeDeclPtr makeExprLetFlagsFlags();
    TypeDeclPtr makeExprGenFlagsFlags();
    TypeDeclPtr makeExprFlagsFlags();
    TypeDeclPtr makeExprPrintFlagsFlags();
    TypeDeclPtr makeExprBlockFlags();
    TypeDeclPtr makeMakeFieldDeclFlags();
    TypeDeclPtr makeExprAtFlags();
    TypeDeclPtr makeExprMakeLocalFlags();
    TypeDeclPtr makeExprMakeStructFlags();
    TypeDeclPtr makeExprAscendFlags();
    TypeDeclPtr makeExprCastFlags();
    TypeDeclPtr makeExprVarFlags();
    TypeDeclPtr makeExprFieldDerefFlags();
    TypeDeclPtr makeExprFieldFieldFlags();
    TypeDeclPtr makeExprSwizzleFieldFlags();
    TypeDeclPtr makeExprYieldFlags();
    TypeDeclPtr makeExprReturnFlags();
    TypeDeclPtr makeExprMakeBlockFlags();
    TypeDeclPtr makeTypeDeclFlags();
    TypeDeclPtr makeFieldDeclarationFlags();
    TypeDeclPtr makeStructureFlags();
    TypeDeclPtr makeFunctionFlags();
    TypeDeclPtr makeMoreFunctionFlags();
    TypeDeclPtr makeFunctionSideEffectFlags();
    TypeDeclPtr makeVariableFlags();
    TypeDeclPtr makeVariableAccessFlags();
    TypeDeclPtr makeExprCopyFlags();
    TypeDeclPtr makeExprMoveFlags();
    TypeDeclPtr makeExprIfFlags();
    TypeDeclPtr makeExprStringBuilderFlags();

    void init_expr ( BasicStructureAnnotation & ann );
    bool canSubstituteExpr ( const TypeAnnotation* thisAnn, TypeAnnotation* ann );
    void init_expr_looks_like_call ( BasicStructureAnnotation & ann );

    template <typename EXPR>
    struct AstExprAnnotation : ManagedStructureAnnotation <EXPR> {
        const char * parentExpression = nullptr;
        AstExprAnnotation(const string & en, ModuleLibrary & ml)
            : ManagedStructureAnnotation<EXPR> (en, ml) {
        }
        __forceinline void init() {
            init_expr(*this);
        }
        virtual bool canSubstitute ( TypeAnnotation * ann ) const override {
            return canSubstituteExpr(this, ann);
        }
        virtual void * factory () const override {
            return new EXPR();
        }
    };

    template <typename EXPR>
    struct AstExpressionAnnotation : AstExprAnnotation<EXPR> {
        AstExpressionAnnotation(const string & en, ModuleLibrary & ml)
            :  AstExprAnnotation<EXPR> (en, ml) {
            this->init();
        }
    };

    template <typename EXPR>
    struct AstExprLooksLikeCallAnnotation : AstExpressionAnnotation<EXPR> {
        AstExprLooksLikeCallAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExpressionAnnotation<EXPR> (na, ml) {
            init_expr_looks_like_call(*this);
        }
    };

    template <typename EXPR>
    struct AstExprCallFuncAnnotation : AstExprLooksLikeCallAnnotation<EXPR> {
        AstExprCallFuncAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExprLooksLikeCallAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(func)>("func");
            this->template addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
        }
    };

    template <typename EXPR>
    struct AstExprPtr2RefAnnotation : AstExpressionAnnotation<EXPR> {
        AstExprPtr2RefAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExpressionAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            this->template addField<DAS_BIND_MANAGED_FIELD(unsafeDeref)>("unsafeDeref");
            this->template addField<DAS_BIND_MANAGED_FIELD(assumeNoAlias)>("assumeNoAlias");
        }
    };

    template <typename EXPR>
    struct AstExprAtAnnotation : AstExpressionAnnotation<EXPR> {
        AstExprAtAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExpressionAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(subexpr)>("subexpr");
            this->template addField<DAS_BIND_MANAGED_FIELD(index)>("index");
            this->addFieldEx ( "atFlags", "atFlags", offsetof(ExprAt, atFlags), makeExprAtFlags() );
        }
    };

    template <typename EXPR>
    struct AstExprOpAnnotation : AstExprCallFuncAnnotation<EXPR> {
        AstExprOpAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExprCallFuncAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(op)>("op");
        }
    };

    template <typename EXPR>
    struct AstExprOp2Annotation : AstExprOpAnnotation<EXPR> {
        AstExprOp2Annotation(const string & na, ModuleLibrary & ml)
            :  AstExprOpAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(left)>("left");
            this->template addField<DAS_BIND_MANAGED_FIELD(right)>("right");
        }
    };

    template <typename EXPR>
    struct AstExprMakeLocalAnnotation : AstExpressionAnnotation<EXPR> {
        AstExprMakeLocalAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExpressionAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(makeType)>("makeType");
            this->template addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            this->template addField<DAS_BIND_MANAGED_FIELD(extraOffset)>("extraOffset");
            this->addFieldEx ( "makeFlags", "makeFlags", offsetof(ExprMakeLocal, makeFlags), makeExprMakeLocalFlags() );
        }
    };

    template <typename EXPR>
    struct AstExprMakeArrayAnnotation : AstExprMakeLocalAnnotation<EXPR> {
        AstExprMakeArrayAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExprMakeLocalAnnotation<EXPR> (na, ml) {
                using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(recordType)>("recordType");
            this->template addField<DAS_BIND_MANAGED_FIELD(values)>("values");
        }
    };

    template <typename EXPR>
    struct AstExprLikeCallAnnotation : AstExprLooksLikeCallAnnotation<EXPR> {
        AstExprLikeCallAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExprLooksLikeCallAnnotation<EXPR> (na, ml) {
        }
    };

    template <typename EXPR>
    struct AstExprFieldAnnotation : AstExpressionAnnotation<EXPR> {
        AstExprFieldAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExpressionAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            this->template addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            this->template addField<DAS_BIND_MANAGED_FIELD(atField)>("atField");
            this->template addField<DAS_BIND_MANAGED_FIELD(field)>("field");
            this->template addField<DAS_BIND_MANAGED_FIELD(fieldIndex)>("fieldIndex");
            this->template addField<DAS_BIND_MANAGED_FIELD(annotation)>("annotation");
            this->addFieldEx ( "derefFlags", "derefFlags", offsetof(ExprField, derefFlags), makeExprFieldDerefFlags() );
            this->addFieldEx ( "fieldFlags", "fieldFlags", offsetof(ExprField, fieldFlags), makeExprFieldFieldFlags() );
        }
    };

    template <typename EXPR>
    struct AstExprConstAnnotation : AstExpressionAnnotation<EXPR> {
        AstExprConstAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExpressionAnnotation<EXPR> (na, ml) {
            using ManagedType = EXPR;
            this->template addField<DAS_BIND_MANAGED_FIELD(baseType)>("baseType");
        }
        template <typename TT>
        void init( ModuleLibrary & ml ) {
            auto cpptype = makeType<TT>(ml);
            string cppname = "cvalue<" + describeCppType(cpptype) + ">()";
            this->addFieldEx ( "value", cppname, offsetof(ExprConst, value), cpptype );
        }
    };

    template <typename EXPR, typename TT>
    struct AstExprConstTAnnotation : AstExprConstAnnotation<EXPR> {
        AstExprConstTAnnotation(const string & na, ModuleLibrary & ml)
            :  AstExprConstAnnotation<EXPR> (na, ml) {
            this->template init<TT>(ml);
        }
    };

    template <typename TT>
    smart_ptr<TT> Module_Ast::addExpressionAnnotation ( const smart_ptr<TT> & ann ) {
        addAnnotation(ann);
        return ann;
    }
}
