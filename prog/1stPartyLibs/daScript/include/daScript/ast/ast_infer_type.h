#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // todo: check for eastl and look for better container
    typedef vector<Function *> MatchingFunctions;

    class CaptureLambda : public Visitor {
    public:
        CaptureLambda() = delete;
        CaptureLambda(bool cm);
        virtual void preVisit(ExprVar *expr) override;
        virtual void preVisit(ExprCall *expr) override;
        das_hash_set<VariablePtr, smart_ptr_hash> scope;
        safe_var_set capt;
        bool fail = false;
        bool inClassMethod = false;
    };

    // type inference

    class InferTypes : public FoldingVisitor {
    public:
        InferTypes(const ProgramPtr &prog, TextWriter *logs_);
        bool finished() const;
        bool verbose = true;

    protected:
        Structure *currentStructure = nullptr;
        FunctionPtr func;
        VariablePtr globalVar;
        vector<VariablePtr> local;
        vector<ExpressionPtr> loop;
        vector<ExprBlock *> blocks;
        vector<ExprBlock *> scopes;
        vector<ExprWith *> with;
        struct AssumeEntry {
            smart_ptr<ExprAssume>   expr;
            das_hash_set<string>    vars;   // ExprVar names referenced in subexpr
        };
        vector<AssumeEntry> assume;
        vector<smart_ptr<ExprAssume>> assumeType;
        vector<size_t> varStack;
        vector<size_t> assumeStack;
        vector<size_t> assumeTypeStack;
        vector<bool> inFinally;
        vector<Function *> inInfer;
        smart_ptr<ExprReturn> oneReturn;
        int32_t returnCount = 0;
        bool canFoldResult = true;
        das_hash_set<int32_t> labels;
        size_t fieldOffset = 0;
        int32_t fieldIndex = 0;
        bool cppLayout = false;
        bool cppLayoutPod = false;
        const Structure *cppLayoutParent = nullptr;
        bool needRestart = false;
        bool enableInferTimeFolding = true;
        bool disableAot = false;
        bool multiContext = false;
        bool standaloneContext = false;
        Expression *lastEnuValue = nullptr;
        int32_t unsafeDepth = 0;
        bool checkNoGlobalVariablesAtAll = false;
        bool strictSmartPointers = false;
        bool disableInit = false;
        bool strictUnsafeDelete = false;
        bool reportInvisibleFunctions = false;
        bool reportPrivateFunctions = false;
        bool noUnsafeUninitializedStructs = false;
        bool strictProperties = false;
        bool alwaysExportInitializer = false;
        bool relaxedAssign = false;
        bool relaxedPointerConst = false;
        bool unsafeTableLookup = false;
        bool debugInferFlag = false;
        bool forceInscopePod = false;
        bool logInscopePod = false;
        Module *thisModule = nullptr;
        size_t beforeFunctionErrors = 0;
        TextWriter *logs = nullptr;
        int32_t consumeDepth = 0;
        bool   fatalAliasLoop = false;
        bool   inArgumentInit = false;
        int32_t callDepth = 0;
        int32_t inferDepth = 0;

    public:
        vector<FunctionPtr> extraFunctions;

    protected:
        string generateNewLambdaName(const LineInfo &at);
        string generateNewLocalFunctionName(const LineInfo &at);
        void pushVarStack();
        void popVarStack();
        void error(const string &err, const string &extra, const string &fixme, const LineInfo &at, CompilationError cerr = CompilationError::unspecified) const;
        void reportAstChanged();
        virtual void reportFolding() override;
        string describeType(const TypeDeclPtr &decl) const;
        string describeType(const TypeDecl *decl) const;
        string describeFunction(const FunctionPtr &fun) const;
        string describeFunction(const Function *fun) const;

    protected:
        void verifyType(const TypeDeclPtr &decl, bool allowExplicit = false, bool classMethod = false) const;

        bool jitEnabled() const;

        void propagateTempType(const TypeDeclPtr &parentType, TypeDeclPtr &subexprType);

        // find type alias name, and resolve it to type
        // without one generic function
        TypeDeclPtr findFuncAlias(const FunctionPtr &fptr, const string &name) const;

        // find type alias name, and resolve it to type
        // within current context
        TypeDeclPtr findAlias(const string &name) const;

        // infer alias type
        TypeDeclPtr inferAlias(const TypeDeclPtr &decl, const FunctionPtr &fptr = nullptr, AliasMap *aliases = nullptr, OptionsMap *options = nullptr, bool autoToAlias = false) const;

        // get loop in type system
        bool isLoop(vector<string> & visited, const TypeDeclPtr &decl) const;

        string reportInferAliasErrors(const TypeDeclPtr &decl) const;

        void reportInferAliasErrors(const TypeDeclPtr &decl, TextWriter &tw, bool autoToAlias = false) const;

        // infer alias type
        TypeDeclPtr inferPartialAliases(const TypeDeclPtr &decl, const TypeDeclPtr &passType, const FunctionPtr &fptr = nullptr, AliasMap *aliases = nullptr) const;

        // "_" is "*" with thisModule
        // "__" is thisModule->name with this module
        Module *getSearchModule(string &moduleName) const;

        Module *getFunctionVisModule(Function *fn) const;

        bool canCallPrivate(Function *pFn, Module *mod, Module *thisMod) const;

        bool canAccessGlobal(const VariablePtr &pVar, Module *mod, Module *thisMod) const;

        // b <- a <- this
        //  a(x)    b
        //  this`a(x)   __::b
        //      inWhichModule = this
        //      objModule = _b
        bool isVisibleFunc(Module *inWhichModule, Module *objModule) const;

        MatchingFunctions findFuncAddr(const string &name) const;

        MatchingFunctions findTypedFuncAddr(string &name, const vector<TypeDeclPtr> &arguments);

        // MISSING CANDIDATES

        bool isOperator(const string &s) const;

        bool isCloseEnoughName(const string &s, const string &t, bool identical) const;

        MatchingFunctions findMissingCandidates(const string &name, bool identicalName) const;

        MatchingFunctions findMissingGenericCandidates(const string &name, bool identicalName) const;

        bool isMatchingArgument(const FunctionPtr &pFn, TypeDeclPtr argType, TypeDeclPtr passType, bool inferAuto, bool inferBlock, AliasMap *aliases = nullptr, OptionsMap *options = nullptr) const;

        bool isFunctionCompatible(Function *pFn, const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlock, bool checkLastArgumentsInit = true) const;

        bool isFunctionCompatible(Function *pFn, const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlock) const;

        string reportAliasError(const TypeDeclPtr &type) const;

        string describeMismatchingArgument(const string &argName, const TypeDeclPtr &passType, const TypeDeclPtr &argType, int argIndex) const;

        int getMismatchingFunctionRank(const FunctionPtr &pFn, const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlock) const;

        int getMismatchingFunctionRank(const FunctionPtr &pFn, const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlock) const;

        string describeMismatchingFunction(const FunctionPtr &pFn, const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlock) const;

        string describeMismatchingFunction(const FunctionPtr &pFn, const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlock) const;

        string reportMismatchingMemberCall(Structure *st, const string &name, const vector<MakeFieldDeclPtr> &arguments, const vector<TypeDeclPtr> &nonNamedArguments, bool methodCall) const;

        Function *findMethodFunction(Structure *st, const string &name) const;

        bool hasMatchingMemberCall(Structure *st, const string &name, const vector<MakeFieldDeclPtr> &arguments, const vector<TypeDeclPtr> &nonNamedArguments, bool methodCall) const;

        MatchingFunctions findMatchingFunctions(const string &name, const vector<TypeDeclPtr> &types, const vector<MakeFieldDeclPtr> &arguments, bool inferBlock = false) const;

        MatchingFunctions findMatchingFunctions(const string &moduleName, Module *inWhichModule, const string &funcName, const vector<TypeDeclPtr> &types, const vector<MakeFieldDeclPtr> &arguments, bool inferBlock = false) const;

        uint64_t getLookupHash(const vector<TypeDeclPtr> &types) const;

        MatchingFunctions findMatchingFunctions(const string &name, const vector<TypeDeclPtr> &types, bool inferBlock = false, bool visCheck = true) const;

        MatchingFunctions findMatchingFunctions(const string &moduleName, Module *inWhichModule, const string &funcName, const vector<TypeDeclPtr> &types, bool inferBlock = false, bool visCheck = true) const;

        void findMatchingFunctionsAndGenerics(MatchingFunctions &resultFunctions, MatchingFunctions &resultGenerics, const string &name, const vector<TypeDeclPtr> &types, const vector<MakeFieldDeclPtr> &arguments, bool inferBlock = false) const;

        void findMatchingFunctionsAndGenerics(MatchingFunctions &resultFunctions, MatchingFunctions &resultGenerics, const string &name, const vector<TypeDeclPtr> &types, bool inferBlock = false, bool visCheck = true) const;

        void reportDualFunctionNotFound(const string &name, const string &extra,
                                        const LineInfo &at, const MatchingFunctions &candidateFunctions,
                                        const vector<TypeDeclPtr> &types, const vector<TypeDeclPtr> &types2, bool inferAuto, bool inferBlocks, bool reportDetails,
                                        CompilationError cerror, int nExtra, const string &moreError);

        void reportFunctionNotFound(const string &name, const string &extra,
                                    const LineInfo &at, const MatchingFunctions &candidateFunctions,
                                    const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlocks, bool reportDetails,
                                    CompilationError cerror, int nExtra, const string &moreError);

        void reportFunctionNotFound(const string &name, const string &extra,
                                    const LineInfo &at, const MatchingFunctions &candidateFunctions,
                                    const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments,
                                    bool inferAuto, bool inferBlocks, bool reportDetails,
                                    CompilationError cerror, int nExtra, const string &moreError);

        void reportCantClone(const string &message, const TypeDeclPtr &type, const LineInfo &at) const;

        void reportCantCloneFromConst(const string &errorText, CompilationError errorCode, const TypeDeclPtr &type, const LineInfo &at) const;

        MatchingFunctions findDefaultConstructor(const string &sna) const;

        bool hasDefaultUserConstructor(const string &sna) const;

        bool verifyAnyFunc(const MatchingFunctions &fnList, const LineInfo &at) const;

        MatchingFunctions getCloneFunc(const TypeDeclPtr &left, const TypeDeclPtr &right) const;

        bool verifyCloneFunc(const MatchingFunctions &fnList, const LineInfo &at) const;

        MatchingFunctions getFinalizeFunc(const TypeDeclPtr &subexpr) const;

        bool verifyFinalizeFunc(const MatchingFunctions &fnList, const LineInfo &at) const;

        ExprWith *hasMatchingWith(const string &fieldName) const;

        ExpressionPtr promoteToProperty(ExprVar *expr, const ExpressionPtr &right);

        vector<TypeMacro *> findTypeMacro(const string &name) const;

        bool inferTypeExpr(TypeDeclPtr &type);

    protected:
        // type
        virtual TypeDeclPtr visit(TypeDecl *type) override;

        string saveAliasName;

        virtual void preVisitAlias(TypeDecl *, const string &name) override;

        virtual TypeDeclPtr visitAlias(TypeDecl *td, const string &) override;

        // enumeration

        ExpressionPtr makeEnumConstValue(Enumeration *enu, int64_t nextInt) const;

        virtual ExpressionPtr visitEnumerationValue(Enumeration *enu, const string &name, Expression *value, bool last) override;

        virtual void preVisit(Enumeration *enu) override;

        virtual EnumerationPtr visit(Enumeration *enu) override;

        virtual bool canVisitGlobalVariable ( Variable * fun ) override;
        virtual bool canVisitEnumeration ( Enumeration * en ) override;

        // strcuture
        virtual bool canVisitStructure(Structure *st) override;
        virtual void preVisit(Structure *that) override;
        virtual void preVisitStructureField(Structure *that, Structure::FieldDeclaration &decl, bool last) override;
        bool hasSafeWhenUninitialized(const AnnotationArgumentList &args) const;
        virtual void visitStructureField(Structure *st, Structure::FieldDeclaration &decl, bool) override;
        FunctionPtr getOrCreateDummy(Module *mod);
        bool tryMakeStructureCtor(Structure *var, bool isPrivate, bool fromGeneric);
        virtual StructurePtr visit(Structure *var) override;
        // globals
        virtual void preVisitGlobalLet(const VariablePtr &var) override;
        virtual void preVisitGlobalLetInit(const VariablePtr &var, Expression *init) override;
        virtual ExpressionPtr visitGlobalLetInit(const VariablePtr &var, Expression *init) override;
        virtual VariablePtr visitGlobalLet(const VariablePtr &var) override;
        // function
        bool safeExpression(Expression *expr) const;
        virtual bool canVisitFunction(Function *fun) override;
        virtual void preVisit(Function *f) override;
        virtual void preVisitArgument(Function *fn, const VariablePtr &var, bool lastArg) override;
        virtual void preVisitArgumentInit(Function *f, const VariablePtr &arg, Expression *that) override;
        virtual ExpressionPtr visitArgumentInit(Function *f, const VariablePtr &arg, Expression *that) override;
        virtual VariablePtr visitArgument(Function *fn, const VariablePtr &var, bool lastArg) override;
        virtual FunctionPtr visit(Function *that) override;
        // any expression
        virtual void preVisitExpression(Expression *expr) override;
        // const
        vec4f getEnumerationValue(ExprConstEnumeration *expr, bool &inferred) const;
        bool isConstantType(ExprConst *c) const;
        virtual ExpressionPtr visit(ExprConst *c) override;
        // ExprUnsafe
        virtual void preVisit(ExprUnsafe *expr) override;
        virtual ExpressionPtr visit(ExprUnsafe *expr) override;
        // ExprGoto
        Expression *findLabel(int32_t label) const;
        virtual ExpressionPtr visit(ExprGoto *expr) override;
        // ExprReader
        virtual ExpressionPtr visit(ExprReader *expr) override;
        // ExprLabel
        virtual void preVisit(ExprLabel *expr) override;
        // ExprRef2Value
        virtual ExpressionPtr visit(ExprRef2Value *expr) override;
        // ExprAddr
        void reportFunctionNotFound(ExprAddr *expr);

        virtual ExpressionPtr visit(ExprAddr *expr) override;
        // ExprPtr2Ref
        virtual ExpressionPtr visit(ExprPtr2Ref *expr) override;
        // ExprRef2Ptr
        virtual ExpressionPtr visit(ExprRef2Ptr *expr) override;
        // ExprNullCoalescing
        void propagateAlwaysSafe(const ExpressionPtr &expr);
        virtual ExpressionPtr visit(ExprNullCoalescing *expr) override;
        // ExprStaticAssert
        virtual void preVisit(ExprStaticAssert *expr) override;
        virtual ExpressionPtr visit(ExprStaticAssert *expr) override;
        // ExprAssert
        virtual void preVisit(ExprAssert *expr) override;
        virtual ExpressionPtr visit(ExprAssert *expr) override;
        // ExprQuote
        virtual ExpressionPtr visit(ExprQuote *expr) override;
        // ExprDebug
        virtual ExpressionPtr visit(ExprDebug *expr) override;
        // ExprMemZero
        virtual ExpressionPtr visit(ExprMemZero *expr) override;
        // ExprMakeGenerator
        virtual void preVisit(ExprMakeGenerator *expr) override;
        virtual ExpressionPtr visit(ExprMakeGenerator *expr) override;
        // ExprMakeBlock
        bool verifyCapture(const vector<CaptureEntry> &capture, const CaptureLambda &cl, bool isUnsafe, const LineInfo &at);
        ExpressionPtr convertBlockToLambda(ExprMakeBlock *expr);
        ExpressionPtr convertBlockToLocalFunction(ExprMakeBlock *expr);
        virtual ExpressionPtr visit(ExprMakeBlock *expr) override;
        // find call macro
        ExprLooksLikeCall *makeCallMacro(const LineInfo &at, const string &funcName);
        // ExprInvoke
        virtual ExpressionPtr visit(ExprInvoke *expr) override;

        // ExprSetInsert
        virtual ExpressionPtr visit(ExprSetInsert *expr) override;
        // ExprErase
        virtual ExpressionPtr visit(ExprErase *expr) override;
        // ExprFind
        virtual ExpressionPtr visit(ExprFind *expr) override;
        // ExprKeyExists
        virtual ExpressionPtr visit(ExprKeyExists *expr) override;
        // ExprIs
        virtual ExpressionPtr visit(ExprIs *expr) override;

        // ExprTypeDecl
        virtual ExpressionPtr visit(ExprTypeDecl *expr) override;

        // ExprTypeInfo
        virtual ExpressionPtr visit(ExprTypeInfo *expr) override;
        // ExprDelete
        void reportMissingFinalizer(const string &message, const LineInfo &at, const TypeDeclPtr &ftype);
        virtual ExpressionPtr visit(ExprDelete *expr) override;
        // ExprCast
        TypeDeclPtr castStruct(const LineInfo &at, const TypeDeclPtr &subexprType, const TypeDeclPtr &castType, bool upcast) const;
        TypeDeclPtr castFunc(const LineInfo &at, const TypeDeclPtr &subexprType, const TypeDeclPtr &castType, bool upcast) const;

        virtual ExpressionPtr visit(ExprCast *expr) override;
        // ExprAscend
        void updateNewFlags(ExprAscend *expr);
        virtual void preVisit(ExprAscend *expr) override;
        virtual ExpressionPtr visit(ExprAscend *expr) override;
        // ExprNew
        virtual void preVisit(ExprNew *call) override;
        virtual void preVisitNewArg(ExprNew *call, Expression *arg, bool last) override;
        void checkEmptyBlock(Expression *arg);

        virtual ExpressionPtr visitNewArg(ExprNew *call, Expression *arg, bool last) override;
        virtual ExpressionPtr visit(ExprNew *expr) override;
        // ExprAt
        virtual ExpressionPtr visit(ExprAt *expr) override;
        // ExprSafeAt
        virtual ExpressionPtr visit(ExprSafeAt *expr) override;
        // ExprBlock
        void setBlockCopyMoveFlags(ExprBlock *block);
        virtual void preVisit(ExprBlock *block) override;
        virtual void preVisitBlockFinal(ExprBlock *block) override;
        virtual void preVisitBlockArgument(ExprBlock *block, const VariablePtr &var, bool lastArg) override;
        virtual ExpressionPtr visitBlockArgumentInit(ExprBlock *block, const VariablePtr &arg, Expression *that) override;

        virtual ExpressionPtr visitBlockExpression(ExprBlock *block, Expression *that) override;

        virtual ExpressionPtr visit(ExprBlock *block) override;
        // Swizzle
        virtual ExpressionPtr visit(ExprSwizzle *expr) override;
        // ExprAsVariant
        virtual ExpressionPtr visit(ExprAsVariant *expr) override;
        // ExprSafeAsVariant
        virtual ExpressionPtr visit(ExprSafeAsVariant *expr) override;
        // ExprIsVariant
        virtual ExpressionPtr visit(ExprIsVariant *expr) override;
        // ExprField
        bool verifyPrivateFieldLookup(ExprField *expr);
        ExpressionPtr promoteToProperty(ExprField *expr, const ExpressionPtr &right, const string &opName = "clone");

        LineInfo makeConstAt(ExprField *expr) const;

        virtual ExpressionPtr visit(ExprField *expr) override;
        void collectMissingOperators(const string &opN, MatchingFunctions &mf, bool identicalName);
        virtual ExpressionPtr visit(ExprSafeField *expr) override;
        // tag
        virtual void preVisit(ExprTag *expr) override;
        // ExprVar
        vector<VariablePtr> findMatchingVar(const string &name, bool seePrivate) const;
        virtual void preVisit(ExprVar *expr) override;
        virtual ExpressionPtr visit(ExprVar *expr) override;
        // ExprOp1
        bool isBitfieldOp(const Function *fnc) const;
        virtual ExpressionPtr visit(ExprOp1 *expr) override;
        // ExprOp2
        bool isAssignmentOperator(const string &op);

        bool isLogicOperator(const string &op);

        bool isCommutativeOperator(const string &op);

        string flipCommutativeOperatorSide(const string &op);

        bool canOperateOnPointers(const TypeDeclPtr &leftType, const TypeDeclPtr &rightType, TemporaryMatters tmatter) const;

        bool isSameSmartPtrType(const TypeDeclPtr &lt, const TypeDeclPtr &rt, bool leftOnly = false);

        void preVisit(ExprOp2 *expr) override;
        void removeR2v(ExpressionPtr &expr);
        ExpressionPtr promoteOp2ToProperty(ExprOp2 *expr);
        virtual ExpressionPtr visit(ExprOp2 *expr) override;
        // ExprOp3
        virtual ExpressionPtr visit(ExprOp3 *expr) override;
        // ExprMove
        bool isVoidOrNothing(const TypeDeclPtr &ptr) const;
        bool canCopyOrMoveType(const TypeDeclPtr &leftType, const TypeDeclPtr &rightType, TemporaryMatters tmatter, Expression *leftExpr,
                               const string &errorText, CompilationError errorCode, const LineInfo &at) const;
        string moveErrorInfo(ExprMove *expr) const;
        virtual void preVisit(ExprMove *expr) override;
        virtual ExpressionPtr visit(ExprMove *expr) override;
        // ExprCopy
        string copyErrorInfo(ExprCopy *expr) const;
        void preVisit(ExprCopy *expr) override;
        virtual ExpressionPtr visit(ExprCopy *expr) override;
        // ExprClone
        virtual void preVisit(ExprClone *expr) override;
        ExpressionPtr promoteAssignmentToProperty(ExprOp2 *expr);
        virtual ExpressionPtr visit(ExprClone *expr) override;
        // ExprTryCatch
        void preVisit(ExprTryCatch *expr) override;
        ExpressionPtr visit(ExprTryCatch *expr) override;
        // ExprReturn
        bool inferReturnType(TypeDeclPtr &resType, ExprReturn *expr);
        virtual void preVisit(ExprReturn *expr) override;
        void getDetailsAndSuggests(ExprReturn *expr, string &details, string &suggestions) const;
        virtual ExpressionPtr visit(ExprReturn *expr) override;
        // ExprYield
        virtual ExpressionPtr visit(ExprYield *expr) override;

        // ExprBreak
        virtual ExpressionPtr visit(ExprBreak *expr) override;
        // ExprContinue
        virtual ExpressionPtr visit(ExprContinue *expr) override;
        // ExprIfThenElse
        bool isConstExprFunc(Function *fun) const;
        ExpressionPtr getConstExpr(Expression *expr);
        virtual bool canVisitIfSubexpr(ExprIfThenElse *expr) override;
        virtual void preVisit(ExprIfThenElse *expr) override;
        virtual ExpressionPtr visit(ExprIfThenElse *expr) override;
        // ExprAssume
        virtual void preVisit(ExprAssume *expr) override;

        virtual ExpressionPtr visit(ExprAssume *expr) override;
        // ExprWith
        virtual void preVisitWithBody(ExprWith *expr, Expression *body) override;
        virtual ExpressionPtr visit(ExprWith *expr) override;
        // ExprWhile
        virtual void preVisit(ExprWhile *expr) override;
        virtual ExpressionPtr visit(ExprWhile *expr) override;
        // ExprFor
        virtual void preVisit(ExprFor *expr) override;
        virtual void preVisitForStack(ExprFor *expr) override;
        virtual void preVisitForSource(ExprFor *expr, Expression *that, bool last) override;
        virtual ExpressionPtr visitForSource(ExprFor *expr, Expression *that, bool last) override;
        virtual ExpressionPtr visit(ExprFor *expr) override;
        // ExprLet

        bool isPodDelete(TypeDecl *typ);

        bool isPodDelete(TypeDecl *typ, das_set<Structure *> &dep, bool &hasHeap);

        virtual void preVisit(ExprLet *expr) override;
        virtual void preVisitLet(ExprLet *expr, const VariablePtr &var, bool last) override;
        bool isEmptyInit(const VariablePtr &var) const;
        virtual VariablePtr visitLet(ExprLet *expr, const VariablePtr &var, bool last) override;
        ExpressionPtr promoteToCloneToMove(const VariablePtr &var);
        bool canRelaxAssign(Expression *init) const;
        virtual ExpressionPtr visitLetInit(ExprLet *expr, const VariablePtr &var, Expression *init) override;

        void expandTupleName(const string &name, const LineInfo &varAt);

        virtual ExpressionPtr visit(ExprLet *expr) override;
        // ExprCallMacro
        virtual void preVisit(ExprCallMacro *expr) override;
        virtual ExpressionPtr visit(ExprCallMacro *expr) override;
        // ExprLooksLikeCall
        virtual bool canVisitLooksLikeCall(ExprLooksLikeCall *call) override;
        virtual void preVisit(ExprLooksLikeCall *call) override;
        virtual ExpressionPtr visit(ExprLooksLikeCall *call) override;
        virtual ExpressionPtr visitLooksLikeCallArg(ExprLooksLikeCall *call, Expression *arg, bool last) override;
        // ExprNamedCall
        virtual bool canVisitNamedCall(ExprNamedCall *call) override;
        vector<ExpressionPtr> demoteCallArguments(ExprNamedCall *expr, const FunctionPtr &pFn);
        ExpressionPtr demoteCall(ExprNamedCall *expr, const FunctionPtr &pFn);
        virtual void preVisit(ExprNamedCall *call) override;
        virtual MakeFieldDeclPtr visitNamedCallArg(ExprNamedCall *call, MakeFieldDecl *arg, bool last) override;

        typedef vector<pair<Function *, int>> RankedMatchingFunctions;

        int sortCandidates(RankedMatchingFunctions &ranked, MatchingFunctions &candidates, int nArgs);

        int prepareCandidates(MatchingFunctions &candidates, const vector<TypeDeclPtr> &nonNamedArguments, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlocks);

        void reportMissing(ExprNamedCall *expr, const vector<TypeDeclPtr> &nonNamedArguments, const string &msg, bool reportDetails,
                           CompilationError cerror = CompilationError::function_not_found, const string &moreError = "");
        void reportExcess(ExprNamedCall *expr, const vector<TypeDeclPtr> &nonNamedArguments, const string &msg, MatchingFunctions can1, const MatchingFunctions &can2,
                          CompilationError cerror = CompilationError::function_not_found);

        int prepareCandidates(MatchingFunctions &candidates, const vector<TypeDeclPtr> &nonNamedArguments, bool inferAuto, bool inferBlocks);

        string reportMethodVsCall(ExprLooksLikeCall *expr);

        void reportMissing(ExprLooksLikeCall *expr, const vector<TypeDeclPtr> &types,
                           const string &msg, bool reportDetails,
                           CompilationError cerror = CompilationError::function_not_found);
        void reportExcess(ExprLooksLikeCall *expr, const vector<TypeDeclPtr> &types,
                          const string &msg, MatchingFunctions can1, const MatchingFunctions &can2,
                          CompilationError cerror = CompilationError::function_not_found);
        virtual ExpressionPtr visit(ExprNamedCall *expr) override;
        // ExprCall
        virtual bool canVisitCall(ExprCall *call) override;
        void markNoDiscard(Expression *expr);
        virtual void preVisit(ExprCall *call) override;
        bool isConsumeArgumentFunc(Function *fn);
        bool isConsumeArgumentCall(Expression *arg);
        virtual void preVisitCallArg(ExprCall *call, Expression *arg, bool last) override;
        virtual ExpressionPtr visitCallArg(ExprCall *call, Expression *arg, bool last) override;
        string getGenericInstanceName(const Function *fn) const;
        string callCloneName(const string &name);

        // however many casts is where its at
        static int computeSubstituteDistance(const vector<TypeDeclPtr> &arguments, const FunctionPtr &fn);

        // -1 - less specialized, +1 - more specialized, 0 - we don't know
        int moreSpecialized(const TypeDeclPtr &t1, const TypeDeclPtr &t2, TypeDeclPtr &passType);

        // compares two generics
        //  one generic is more specialized than the other, if ALL arguments are more specialized or the
        bool copmareFunctionSpecialization(const FunctionPtr &f1, const FunctionPtr &f2, ExprLooksLikeCall *expr);

        static void applyLSP(const vector<TypeDeclPtr> &arguments, MatchingFunctions &functions);

        bool reportOp2Errors(ExprLooksLikeCall *expr);
        enum class InferCallError {
            functionOrGeneric,
            operatorOp2,
            tryOperator
        };

        bool inferArguments(vector<TypeDeclPtr> &types, const vector<ExpressionPtr> &arguments);

        FunctionPtr inferFunctionCall(ExprLooksLikeCall *expr, InferCallError cerr = InferCallError::functionOrGeneric, Function *lookupFunction = nullptr, bool failOnMissingCtor = true, bool visCheck = true);
        ExpressionPtr inferGenericOperator3(const string &opN, const LineInfo &expr_at, const ExpressionPtr &arg0, const ExpressionPtr &arg1, const ExpressionPtr &arg2, InferCallError err = InferCallError::tryOperator);
        ExpressionPtr inferGenericOperator(const string &opN, const LineInfo &expr_at, const ExpressionPtr &arg0, const ExpressionPtr &arg1, InferCallError err = InferCallError::tryOperator);
        ExpressionPtr inferGenericOperatorWithName(const string &opN, const LineInfo &expr_at, const ExpressionPtr &arg0, const string &arg1, InferCallError err = InferCallError::tryOperator);

        Variable *findMatchingBlockOrLambdaVariable(const string &name);

        bool isCloneArrayExpression(ExprCall *expr) const;

        virtual ExpressionPtr visit(ExprCall *expr) override;
        // StringBuilder
        virtual ExpressionPtr visitStringBuilderElement(ExprStringBuilder *, Expression *expr, bool) override;
        virtual ExpressionPtr visit(ExprStringBuilder *expr) override;
        // make variant
        virtual void preVisit(ExprMakeVariant *expr) override;
        virtual MakeFieldDeclPtr visitMakeVariantField(ExprMakeVariant *expr, int index, MakeFieldDecl *decl, bool last) override;
        virtual ExpressionPtr visit(ExprMakeVariant *expr) override;
        // make structure
        void describeLocalType(vector<string> &results, TypeDecl *tp, const string &prefix, das_set<Structure *> &dep) const;
        string describeLocalType(TypeDecl *tp) const;
        virtual bool canVisitMakeStructure ( ExprMakeStruct * expr );
        virtual void preVisit(ExprMakeStruct *expr) override;
        bool convertCloneSemanticsToExpression(ExprMakeStruct *expr, int index, MakeFieldDecl *decl);
        virtual MakeFieldDeclPtr visitMakeStructureField(ExprMakeStruct *expr, int index, MakeFieldDecl *decl, bool last) override;
        virtual ExpressionPtr structToTuple(const TypeDeclPtr &makeType, const MakeStructPtr &st, const LineInfo &at);
        virtual ExpressionPtr visit(ExprMakeStruct *expr) override;
        // make tuple
        virtual void preVisit(ExprMakeTuple *expr) override;
        virtual ExpressionPtr visitMakeTupleIndex(ExprMakeTuple *expr, int index, Expression *init, bool lastField) override;
        virtual ExpressionPtr visit(ExprMakeTuple *expr) override;
        // make array
        virtual void preVisit(ExprMakeArray *expr) override;
        virtual ExpressionPtr visitMakeArrayIndex(ExprMakeArray *expr, int index, Expression *init, bool last) override;
        virtual ExpressionPtr visit(ExprMakeArray *expr) override;
        // array comprehension
        virtual void preVisitArrayComprehensionSubexpr(ExprArrayComprehension *expr, Expression *subexpr) override;
        virtual void preVisitArrayComprehensionWhere(ExprArrayComprehension *expr, Expression *where) override;
        virtual ExpressionPtr visit(ExprArrayComprehension *expr) override;
    };
}
