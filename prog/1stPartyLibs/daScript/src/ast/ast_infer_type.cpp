#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_infer_type.h"
#include "daScript/ast/ast_visitor.h"

#define DAS_XSTR(s) #s
#define DAS_STR(s) DAS_XSTR(s)

namespace das {

    // in ast_handle of all places, due to reporting fields
    void reportTrait(const TypeDeclPtr &type, const string &prefix, const callable<void(const TypeDeclPtr &, const string &)> &report);

    CaptureLambda::CaptureLambda(bool cm) : inClassMethod(cm) {}
    void CaptureLambda::preVisit(ExprVar *expr) {
        if (!expr->type) { // trying to capture non-inferred section
            fail = true;
            return;
        }
        auto var = expr->variable;
        if (expr->local || expr->argument || expr->block) {
            if (expr->argument || (scope.find(var) != scope.end())) {
                auto varT = var->type;
                if (!varT || varT->isAutoOrAlias()) {
                    fail = true;
                    return;
                }
                capt.insert(var);
            }
        }
    }
    void CaptureLambda::preVisit(ExprCall *expr) {
        if (inClassMethod && !expr->type) { // the idea is if call is not resolved and its a class method
            fail = true;                    // it could be self.call, so we need to wait til its resolved
            return;
        }
    }

    InferTypes::InferTypes(const ProgramPtr &prog, TextWriter *logs_) : FoldingVisitor(prog), logs(logs_) {
        debugInferFlag = prog->options.getBoolOption("debug_infer_flag", prog->policies.debug_infer_flag);
        enableInferTimeFolding = prog->options.getBoolOption("infer_time_folding", !prog->policies.no_infer_time_folding);
        disableAot = prog->options.getBoolOption("no_aot", false);
        multiContext = prog->options.getBoolOption("multiple_contexts", prog->policies.multiple_contexts);
        standaloneContext = prog->options.getBoolOption("standalone_context", prog->policies.standalone_context);
        checkNoGlobalVariablesAtAll = prog->options.getBoolOption("no_global_variables_at_all", prog->policies.no_global_variables_at_all);
        strictSmartPointers = prog->options.getBoolOption("strict_smart_pointers", prog->policies.strict_smart_pointers);
        disableInit = prog->options.getBoolOption("no_init", prog->policies.no_init);
        strictUnsafeDelete = prog->options.getBoolOption("strict_unsafe_delete", prog->policies.strict_unsafe_delete);
        reportInvisibleFunctions = prog->options.getBoolOption("report_invisible_functions", prog->policies.report_invisible_functions);
        reportPrivateFunctions = prog->options.getBoolOption("report_private_functions", prog->policies.report_private_functions);
        noUnsafeUninitializedStructs = prog->options.getBoolOption("no_unsafe_uninitialized_structures", prog->policies.no_unsafe_uninitialized_structures);
        strictProperties = prog->options.getBoolOption("strict_properties", prog->policies.strict_properties);
        alwaysExportInitializer = prog->options.getBoolOption("always_export_initializer", false);
        relaxedAssign = prog->options.getBoolOption("relaxed_assign", prog->policies.relaxed_assign);
        relaxedPointerConst = prog->options.getBoolOption("relaxed_pointer_const", prog->policies.relaxed_pointer_const);
        unsafeTableLookup = prog->options.getBoolOption("unsafe_table_lookup", prog->policies.unsafe_table_lookup);
        forceInscopePod = prog->options.getBoolOption("force_inscope_pod", prog->policies.force_inscope_pod);
        logInscopePod = prog->options.getBoolOption("log_inscope_pod", prog->policies.log_inscope_pod);
        thisModule = prog->thisModule.get();
    }
    TypeDeclPtr InferTypes::visit(TypeDecl *type) {
        TypeDeclPtr newType = type;
        if (inferTypeExpr(newType)) {
            reportAstChanged();
        }
        return newType;
    }
    void InferTypes::preVisitAlias(TypeDecl *, const string &name) {
        saveAliasName = name;
    }
    TypeDeclPtr InferTypes::visitAlias(TypeDecl *td, const string &) {
        if (td->isAlias()) {
            vector<string> visited;
            visited.push_back(saveAliasName);
            if ( isLoop(visited, td) ) {
                fatalAliasLoop = true;
                error("alias loop detected: '" + describeType(td) + "'", "", "",
                      td->at, CompilationError::invalid_type);
                return td;
            }
            if (auto ta = inferAlias(td)) {
                if (ta->isAutoOrAlias()) {
                    error("internal compiler error: can't be inferred: '" + describeType(td) + "'", "", "",
                          td->at, CompilationError::invalid_type);
                    return td;
                }
                return ta;
            } else {
                error("can't be inferred: '" + describeType(td) + "'", reportInferAliasErrors(td), "",
                      td->at, CompilationError::invalid_type);
            }
        }
        td->alias = saveAliasName;
        saveAliasName.clear();
        return td;
    }
    ExpressionPtr InferTypes::makeEnumConstValue(Enumeration *enu, int64_t nextInt) const {
        vec4f nextV = v_zero();
        switch (enu->baseType) {
        case Type::tInt8:
            nextV = cast<int8_t>::from(int8_t(nextInt));
            break;
        case Type::tUInt8:
            nextV = cast<uint8_t>::from(uint8_t(nextInt));
            break;
        case Type::tInt16:
            nextV = cast<int16_t>::from(int16_t(nextInt));
            break;
        case Type::tUInt16:
            nextV = cast<uint16_t>::from(uint16_t(nextInt));
            break;
        case Type::tInt:
            nextV = cast<int32_t>::from(int32_t(nextInt));
            break;
        case Type::tUInt:
            nextV = cast<uint32_t>::from(uint32_t(nextInt));
            break;
        case Type::tBitfield:
            nextV = cast<uint32_t>::from(uint32_t(nextInt));
            break;
        case Type::tBitfield8:
            nextV = cast<uint8_t>::from(uint8_t(nextInt));
            break;
        case Type::tBitfield16:
            nextV = cast<uint16_t>::from(uint16_t(nextInt));
            break;
        case Type::tBitfield64:
            nextV = cast<uint64_t>::from(uint64_t(nextInt));
            break;
        case Type::tInt64:
            nextV = cast<int64_t>::from(int64_t(nextInt));
            break;
        case Type::tUInt64:
            nextV = cast<uint64_t>::from(uint64_t(nextInt));
            break;
        default:
            DAS_ASSERTF(0, "we should not be here. unsupported enum type");
        }
        auto nextValue = Program::makeConst(enu->at, enu->makeBaseType(), nextV);
        nextValue->type = enu->makeBaseType();
        return nextValue;
    }
    ExpressionPtr InferTypes::visitEnumerationValue(Enumeration *enu, const string &name, Expression *value, bool last) {
        if (!value) {
            if (lastEnuValue) {
                if (lastEnuValue->rtti_isConstant() && lastEnuValue->type && lastEnuValue->type->isInteger()) {
                    reportAstChanged();
                    int64_t nextInt = getConstExprIntOrUInt(lastEnuValue) + 1;
                    auto nextValue = makeEnumConstValue(enu, nextInt);
                    lastEnuValue = nextValue.get();
                    return nextValue;
                } else {
                    error("enumeration value '" + name + "' can't be inferred yet", "", "",
                          enu->at);
                }
            } else {
                reportAstChanged();
                auto zeroValue = Program::makeConst(enu->at, enu->makeBaseType(), v_zero());
                zeroValue->type = enu->makeBaseType();
                lastEnuValue = zeroValue.get();
                return zeroValue;
            }
        } else {
            if (!value->type) {
                error("enumeration value '" + name + "' can't be inferred yet", "", "",
                      value->at);
            } else if (!value->type || !value->type->isInteger()) {
                error("enumeration value '" + name + "' has to be signed or unsigned integer of any size, and not '" + value->type->describe() + "'", "", "",
                      value->at, CompilationError::invalid_enumeration);
            } else if (!value->rtti_isConstant()) {
                if (auto fenum = getConstExpr(value)) {
                    reportAstChanged();
                    return fenum;
                } else {
                    error("enumeration value '" + name + "' must be integer constant, and not an expression", "", "",
                          value->at, CompilationError::invalid_enumeration);
                }
            } else if (value->type->baseType != enu->baseType) {
                reportAstChanged();
                int64_t thisInt = getConstExprIntOrUInt(value);
                auto thisValue = makeEnumConstValue(enu, thisInt);
                lastEnuValue = thisValue.get();
                return thisValue;
            }
        }
        lastEnuValue = value;
        return Visitor::visitEnumerationValue(enu, name, value, last);
    }
    void InferTypes::preVisit(Enumeration *enu) {
        Visitor::preVisit(enu);
        lastEnuValue = nullptr;
    }
    EnumerationPtr InferTypes::visit(Enumeration *enu) {
        lastEnuValue = nullptr;
        return Visitor::visit(enu);
    }
    bool InferTypes::canVisitStructure(Structure *st) {
        if ( fatalAliasLoop ) return false;
        return !st->isTemplate; // we don't do a thing with templates
    }
    void InferTypes::preVisit(Structure *that) {
        Visitor::preVisit(that);
        currentStructure = that;
        fieldOffset = 0;
        cppLayout = that->cppLayout;
        cppLayoutPod = !that->cppLayoutNotPod;
        cppLayoutParent = nullptr;
        that->fieldLookup.clear();
        that->hasInitFields = false;
        fieldIndex = 0;
    }
    void InferTypes::preVisitStructureField(Structure *that, Structure::FieldDeclaration &decl, bool last) {
        Visitor::preVisitStructureField(that, decl, last);
        that->fieldLookup[decl.name] = fieldIndex++;
        if (decl.type->isAuto() && !decl.init) {
            error("structure field type can't be inferred, it needs an initializer", "", "",
                  decl.at, CompilationError::cant_infer_missing_initializer);
        }
    }
    bool InferTypes::hasSafeWhenUninitialized(const AnnotationArgumentList &args) const {
        for (auto &ann : args) {
            if (ann.name == "safe_when_uninitialized") {
                return true;
            }
        }
        return false;
    }
    void InferTypes::visitStructureField(Structure *st, Structure::FieldDeclaration &decl, bool) {
        if (decl.init)
            st->hasInitFields = true;
        if (decl.type && decl.type->isExprType()) {
            return;
        }
        if (!st->parent && decl.classMethod && decl.type && decl.type->baseType == Type::autoinfer) {
            // if its field:auto = cast<auto>(@@fun) - we demote to @@fun; this is only possible when its sealed in the base class
            if (decl.init && decl.init->rtti_isCast()) {
                auto castExpr = static_pointer_cast<ExprCast>(decl.init);
                if (castExpr->castType && castExpr->castType->baseType == Type::autoinfer) {
                    decl.init = castExpr->subexpr;
                    reportAstChanged();
                    return;
                }
            }
        }
        if (decl.parentType && st->parent) {
            auto pf = st->parent->findField(decl.name);
            if (!pf->type->isAutoOrAlias()) {
                TypeDecl::clone(decl.type, pf->type);
                decl.parentType = false;
                decl.type->sanitize();
                reportAstChanged();
            } else {
                error("not fully resolved yet", "", "", decl.at);
                return;
            }
        }
        if (decl.type->isAlias()) {
            if (auto aT = inferAlias(decl.type)) {
                decl.type = aT;
                decl.type->sanitize();
                reportAstChanged();
            } else {
                error("undefined structure field type '" + describeType(decl.type) + "'",
                      reportInferAliasErrors(decl.type), "", decl.at, CompilationError::invalid_structure_field_type);
            }
        }
        if (decl.type->isAuto() && decl.init && decl.init->type) {
            auto varT = TypeDecl::inferGenericType(decl.type, decl.init->type, false, false, nullptr);
            if (!varT) {
                error("structure field initialization type can't be inferred, " + describeType(decl.type) + " = " + describeType(decl.init->type), "", "",
                      decl.at, CompilationError::invalid_structure_field_type);
            } else {
                TypeDecl::applyAutoContracts(varT, decl.type);
                decl.type = varT;
                decl.type->ref = false;
                decl.type->sanitize();
                reportAstChanged();
            }
        }
        if (isCircularType(decl.type)) {
            return;
        }
        if (decl.type->isVoid()) {
            error("structure field type can't be declared void", "", "",
                  decl.at, CompilationError::invalid_structure_field_type);
        } else if (decl.type->ref) {
            error("structure field type can't be declared a reference", "", "",
                  decl.at, CompilationError::invalid_structure_field_type);
        }

        if (noUnsafeUninitializedStructs && !st->isLambda && !decl.init && decl.type->unsafeInit()) {
            if (!hasSafeWhenUninitialized(decl.annotation)) {
                error("Uninitialized field " + decl.name + " is unsafe. Use initializer syntax or @safe_when_uninitialized when intended.", "", "",
                      decl.at, CompilationError::unsafe);
            }
        }

        if (decl.init) {
            if (decl.init->type) {
                if (!canCopyOrMoveType(decl.type, decl.init->type, TemporaryMatters::yes, decl.init.get(),
                                       "structure field " + decl.name + " initialization type mismatch", CompilationError::invalid_initialization_type, decl.init->at)) {
                } else if (!decl.type->canCopy() && !decl.moveSemantics) {
                    error("field " + decl.name + " can't be copied, use <- instead; " + describeType(decl.type), "", "",
                          decl.init->at, CompilationError::invalid_initialization_type);
                    if (canRelaxAssign(decl.init.get())) {
                        reportAstChanged();
                        decl.moveSemantics = true;
                    }
                } else if (!decl.init->type->canCopy() && !decl.init->type->canMove()) {
                    error("field " + decl.name + "can't be initialized at all; " + describeType(decl.init->type), "", "",
                          decl.at, CompilationError::invalid_initialization_type);
                } else if (decl.moveSemantics && decl.init->type->isConst()) {
                    error("can't move from a constant value " + describeType(decl.init->type), "", "",
                          decl.init->at, CompilationError::cant_move);
                }
            } else if (!decl.type->isAuto()) {
                if (decl.init->rtti_isCast()) {
                    auto castExpr = static_pointer_cast<ExprCast>(decl.init);
                    if (castExpr->castType->isAuto()) {
                        reportAstChanged();
                        TypeDecl::clone(castExpr->castType, decl.type);
                    }
                }
            }
        }
        if (decl.type->isFullyInferred()) {
            int fieldAlignemnt = decl.type->getAlignOf();
            int fa = fieldAlignemnt - 1;
            if (cppLayout) {
                auto fp = st->findFieldParent(decl.name);
                if (fp != cppLayoutParent) {
                    if (DAS_NON_POD_PADDING || cppLayoutPod) {
                        fieldOffset = cppLayoutParent ? cppLayoutParent->getSizeOf64() : 0;
                    }
                    cppLayoutParent = fp;
                }
            }
            fieldOffset = (fieldOffset + fa) & ~fa;
            decl.offset = int(fieldOffset);
            fieldOffset += decl.type->getSizeOf64();
        }
        verifyType(decl.type, false, decl.classMethod);
    }
    StructurePtr InferTypes::visit(Structure *var) {
        if (!var->genCtor) {
            if (var->hasAnyInitializers()) {
                if (tryMakeStructureCtor(var, var->privateStructure, false)) {
                    var->genCtor = true;
                }
            } else {
                getOrCreateDummy(thisModule);
            }
        }
        auto tt = make_smart<TypeDecl>(Type::tStructure);
        tt->structType = var;
        if (isCircularType(tt)) {
            var->circular = true;
            error("type creates circular dependency", "", "",
                  var->at, CompilationError::invalid_type);
        }
        currentStructure = nullptr;
        return Visitor::visit(var);
    }
    void InferTypes::preVisitGlobalLet(const VariablePtr &var) {
        Visitor::preVisitGlobalLet(var);
        if (noUnsafeUninitializedStructs && !var->init && var->type->unsafeInit()) {
            if (!hasSafeWhenUninitialized(var->annotation)) {
                error("Uninitialized variable " + var->name + " is unsafe. Use initializer syntax or @safe_when_uninitialized when intended.", "", "",
                      var->at, CompilationError::unsafe);
            }
        }
        if (checkNoGlobalVariablesAtAll && !var->generated) {
            error("global variables are disabled by option no_global_variables_at_all", "", "",
                  var->at, CompilationError::no_global_variables);
        }
        if (var->type->isAuto() && !var->init) {
            error("global variable type can't be inferred, it needs an initializer", "", "",
                  var->at, CompilationError::cant_infer_missing_initializer);
        }
        if (var->type->isAlias()) {
            if (auto aT = inferAlias(var->type)) {
                var->type = aT;
                reportAstChanged();
            } else {
                error("undefined global variable type " + describeType(var->type),
                      reportInferAliasErrors(var->type), "", var->at, CompilationError::invalid_type);
            }
        }
    }
    void InferTypes::preVisitGlobalLetInit(const VariablePtr &var, Expression *init) {
        Visitor::preVisitGlobalLetInit(var, init);
        globalVar = var;
    }
    ExpressionPtr InferTypes::visitGlobalLetInit(const VariablePtr &var, Expression *init) {
        globalVar = nullptr;
        if (!var->init->type)
            return Visitor::visitGlobalLetInit(var, init);
        if (var->type->isAuto()) {
            auto varT = TypeDecl::inferGenericInitType(var->type, var->init->type);
            if (!varT || varT->isAlias()) {
                error("global variable '" + var->name + "' initialization type can't be inferred, " + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                      var->at, CompilationError::cant_infer_mismatching_restrictions);
            } else {
                varT->ref = false;
                TypeDecl::applyAutoContracts(varT, var->type);
                if (!relaxedPointerConst) { // var a = Foo? const -> var a : Foo const? = Foo? const
                    if (varT->isPointer() && !varT->constant && var->init->type->constant && varT->firstType) {
                        varT->firstType->constant = true;
                    }
                }
                var->type = varT;
                var->type->sanitize();
                reportAstChanged();
            }
        } else if (!canCopyOrMoveType(var->type, var->init->type, TemporaryMatters::no, var->init.get(),
                                      "global variable '" + var->name + "' initialization type mismatch", CompilationError::invalid_initialization_type, var->init->at)) {
        } else if (var->type->ref && !var->type->isConst() && var->init->type->isConst()) {
            error("global variable '" + var->name + "' initialization type mismatch, const matters " + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (!var->init_via_clone && (!var->init->type->canCopy() && !var->init->type->canMove())) {
            error("global variable '" + var->name + "' can't be initialized at all. " + describeType(var->type), "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (var->init_via_move && var->init->type->isConst()) {
            error("global variable '" + var->name + "' can't init (move) from a constant value. " + describeType(var->init->type), "", "",
                  var->at, CompilationError::cant_move);
        } else if (!(var->init_via_move || var->init_via_clone) && !var->init->type->canCopy()) {
            error("global variable '" + var->name + "' can't be copied", "", "",
                  var->at, CompilationError::cant_copy);
            if (canRelaxAssign(var->init.get())) {
                reportAstChanged();
                var->init_via_move = true;
            }
        } else if (var->init_via_move && !var->init->type->canMove()) {
            error("global variable '" + var->name + "' can't be moved", "", "",
                  var->at, CompilationError::cant_move);
        } else if (var->init_via_clone && !var->init->type->canClone()) {
            auto varType = make_smart<TypeDecl>(*var->type);
            varType->ref = true;
            auto fnList = getCloneFunc(varType, var->init->type);
            if (fnList.size() && verifyCloneFunc(fnList, var->at)) {
                return promoteToCloneToMove(var);
            } else {
                reportCantClone("global variable '" + var->name + "' can't be cloned",
                                var->init->type, var->at);
            }
        } else {
            if (var->init_via_clone) {
                if (var->init->type->isWorkhorseType()) {
                    var->init_via_clone = false;
                    var->init_via_move = false;
                    reportAstChanged();
                } else {
                    return promoteToCloneToMove(var);
                }
            }
        }
        if (var->init->rtti_isVar()) { // this folds specifically global a = b, where b is const
            auto ivar = static_pointer_cast<ExprVar>(var->init);
            if (ivar->isGlobalVariable() && ivar->variable->init && ivar->variable->init->rtti_isConstant()) {
                reportAstChanged();
                return ivar->variable->init;
            }
        }
        if (disableInit && !var->init->rtti_isConstant()) {
            program->error("[init] is disabled in the options or CodeOfPolicies", "", "",
                           var->at, CompilationError::no_init);
        }
        return Visitor::visitGlobalLetInit(var, init);
    }
    VariablePtr InferTypes::visitGlobalLet(const VariablePtr &var) {
        if (var->type && var->type->isExprType()) {
            return Visitor::visitGlobalLet(var);
        }
        if (isCircularType(var->type)) {
            return Visitor::visitGlobalLet(var);
        }
        if (var->type->ref)
            error("global variable can't be declared a reference", "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (var->type->isVoid())
            error("global variable can't be declared void", "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (!var->type->isLocal())
            error("can't have a global variable of type " + describeType(var->type), "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (!var->type->constant && var->global_shared)
            error("shared global variable must be constant", "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (var->global_shared && !var->init)
            error("shared global variable must be initialized", "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (var->global_shared && !var->type->isShareable()) {
            if (!(var->type->isSimpleType(Type::tLambda) && program->policies.allow_shared_lambda)) {
                error("this variable type can't be shared, " + describeType(var->type), "", "",
                      var->at, CompilationError::invalid_variable_type);
            }
        }
        if (!var->type->ref && var->type->hasClasses()) {
            error("can only have global pointers to classes " + describeType(var->type), "", "",
                  var->at, CompilationError::unsafe);
        }
        if (!var->init && var->type->hasNonTrivialCtor()) {
            error("global variable of type " + describeType(var->type) + " needs to be initialized", "", "",
                  var->at, CompilationError::invalid_variable_type);
        }
        // we are looking into initialization with empty table or array to replace with nada
        if (isEmptyInit(var)) {
            var->init.reset();
            reportAstChanged();
            return Visitor::visitGlobalLet(var);
        }
        verifyType(var->type);
        return Visitor::visitGlobalLet(var);
    }
    bool InferTypes::canVisitFunction(Function *fun) {
        if ( fatalAliasLoop ) return false;
        if (fun->stub)
            return false;
        if (verbose || debugInferFlag) { // it can be fully inferred, and fail concept assert
            return !fun->isTemplate;     // we don't do a thing with templates
        } else {
            return !fun->isTemplate            // we don't do a thing with templates
                   && !(fun->isFullyInferred); // and if its fully inferred - we do nada as well
        }
    }
    void InferTypes::preVisit(Function *f) {
        Visitor::preVisit(f);
        oneReturn.reset();
        returnCount = 0;
        canFoldResult = true;
        unsafeDepth = 0;
        consumeDepth = 0;
        func = f;
        func->hasReturn = false;
        func->isFullyInferred = true;
        beforeFunctionErrors = program->errors.size();
        if (!standaloneContext) {
            func->noAot |= disableAot;
        }
        if (f->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) {
            error("function has too many arguments, max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS), "", "",
                  f->at, CompilationError::too_many_arguments);
        }
        if ((f->init | f->shutdown) && disableInit) {
            error("[init] is disabled in the options or CodeOfPolicies", "", "",
                  f->at, CompilationError::no_init);
        }
    }
    void InferTypes::preVisitArgument(Function *fn, const VariablePtr &var, bool lastArg) {
        Visitor::preVisitArgument(fn, var, lastArg);
        if (var->type->isAlias()) {
            if (auto aT = inferAlias(var->type)) {
                var->type = aT;
                reportAstChanged();
            } else {
                error("undefined function argument type " + describeType(var->type),
                      reportInferAliasErrors(var->type), "", var->at, CompilationError::type_not_found);
            }
        }
        if (var->type->isVoid()) {
            error("function argument type can't be declared void", "", "",
                  var->at, CompilationError::invalid_type);
        }
        if (var->type->ref && var->type->isRefType()) { // silently fix a : Foo& into a : Foo
            var->type->ref = false;
            auto mname = fn->getMangledName();
            if (fn->module->functions.find(mname)) {
                error("function already exists in non-ref form", "\t" + fn->describe(Function::DescribeModule::yes), "",
                      var->at, CompilationError::cant_infer_generic);
                var->type->ref = true;
            }
        }
    }
    void InferTypes::preVisitArgumentInit(Function *f, const VariablePtr &arg, Expression *that) {
        Visitor::preVisitArgumentInit(f, arg, that);
        inArgumentInit = true;
    }
    ExpressionPtr InferTypes::visitArgumentInit(Function *f, const VariablePtr &arg, Expression *that) {
        inArgumentInit = false;
        if (arg->type->isAuto() && arg->init->type) {
            auto varT = TypeDecl::inferGenericType(arg->type, arg->init->type, false, false, nullptr);
            if (!varT) {
                error("generic argument type can't be inferred, " + describeType(arg->type) + " = " + describeType(arg->init->type), "", "",
                      arg->at, CompilationError::cant_infer_generic);
            } else {
                TypeDecl::applyAutoContracts(varT, arg->type);
                arg->type = varT;
                arg->type->ref = false; // so that def ( a = VAR ) infers as def ( a : var_type ), not as def ( a : var_type & )
                reportAstChanged();
                return Visitor::visitArgumentInit(f, arg, that);
            }
        }
        if (!arg->init->type || !arg->type->isSameType(*arg->init->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
            error("function argument default value type mismatch '" + describeType(arg->type) + "' vs '" + (arg->init->type ? describeType(arg->init->type) : "???") + "'", "", "",
                  arg->init->at, CompilationError::invalid_type);
        }
        if (arg->init->type && arg->type->ref && !arg->init->type->ref) {
            error("function argument default value type mismatch '" + describeType(arg->type) + "' vs '" + describeType(arg->init->type) + "', reference matters", "", "",
                  arg->init->at, CompilationError::invalid_type);
        }
        return Visitor::visitArgumentInit(f, arg, that);
    }
    VariablePtr InferTypes::visitArgument(Function *fn, const VariablePtr &var, bool lastArg) {
        if (var->type->isAuto()) {
            error("unresolved generics are not supported", "", "",
                  var->at, CompilationError::cant_infer_generic);
        }
        verifyType(var->type, true);
        return Visitor::visitArgument(fn, var, lastArg);
    }
    FunctionPtr InferTypes::visit(Function *that) {
        // if function got no 'result', function is a void function
        if (!func->hasReturn && canFoldResult) {
            if (func->result->isAuto()) {
                func->result = make_smart<TypeDecl>(Type::tVoid);
                reportAstChanged();
            } else if (!func->result->isVoid()) {
                error("function does not return a value", "", "",
                      func->at, CompilationError::expecting_return_value);
            }
        }
        if (func->result->isAlias()) {
            if (auto aT = inferAlias(func->result)) {
                func->result = aT;
                func->result->sanitize();
                if (!func->result->ref && func->result->isWorkhorseType() && !func->result->isPointer()) {
                    func->result->constant = true;
                }
                reportAstChanged();
            } else {
                error("undefined function result type '" + describeType(func->result) + "'",
                      reportInferAliasErrors(func->result), "", func->at, CompilationError::type_not_found);
            }
        }
        verifyType(func->result);
        if (!func->result->isReturnType()) {
            error("not a valid function return type '" + describeType(func->result) + "'", "", "",
                  func->result->at, CompilationError::invalid_return_type);
        }
        if (func->result->isRefType() && !func->result->ref) {
            if (func->result->canCopy()) {
                if (func->copyOnReturn != true || func->moveOnReturn != false) {
                    reportAstChanged();
                    func->copyOnReturn = true;
                    func->moveOnReturn = false;
                }
            } else if (func->result->canMove()) {
                if (func->copyOnReturn != false || func->moveOnReturn != true) {
                    reportAstChanged();
                    func->copyOnReturn = false;
                    func->moveOnReturn = true;
                }
            } else {
                // the error will be reported in the inferReturnType
                /*
                error("this type can't be copied or moved. not a valid function return type "
                      + describeType(func->result),func->result->at,CompilationError::invalid_return_type);
                */
            }
        } else {
            func->copyOnReturn = false;
            func->moveOnReturn = false;
        }
        // if there were errors, we are not fully inferred
        if (beforeFunctionErrors != program->errors.size()) {
            func->notInferred();
        }
        // now, for some debugging
        if (debugInferFlag) {
            if (func->isFullyInferred) {
                TextWriter srcCode;
                srcCode << *func;
                if (!func->inferredSource.empty()) {
                    if (func->inferredSource != srcCode.c_str()) {
                        program->error("fully inferred function has changed\nbefore:\n" + func->inferredSource + "\nafter:\n" + srcCode.c_str(),
                                       "", "", func->at, CompilationError::unspecified);
                    }
                } else {
                    func->inferredSource = srcCode.c_str();
                }
            }
        }
        // now, lets mark the single return via move thing
        if (forceInscopePod && oneReturn && returnCount == 1) {
            if (oneReturn->moveSemantics) {
                if (oneReturn->subexpr && oneReturn->subexpr->type) {
                    ExprVar *exprVar = nullptr;
                    if (oneReturn->subexpr->rtti_isVar()) {
                        exprVar = (ExprVar *)oneReturn->subexpr.get();
                    }
                    if (exprVar && exprVar->variable) {
                        exprVar->variable->single_return_via_move = true;
                    }
                }
            }
        }
        // if any of this asserts failed, we have a logic error in how we pop
        DAS_ASSERT(loop.size() == 0);
        DAS_ASSERT(scopes.size() == 0);
        DAS_ASSERT(blocks.size() == 0);
        DAS_ASSERT(local.size() == 0);
        DAS_ASSERT(with.size() == 0);
        labels.clear();
        oneReturn.reset();
        returnCount = 0;
        func.reset();
        return Visitor::visit(that);
    }
    void InferTypes::preVisitExpression(Expression *expr) {
        Visitor::preVisitExpression(expr);
        if (func && (expr->userSaidItsSafe && !expr->generated))
            func->hasUnsafe = true;
        // WARNING - this is potentially dangerous. In theory type should be set to nada, and then re-inferred
        // the reason not to reset it is that usually once inferred it should not change. but in some cases it can.
        // even more rare is that it changed, and then no longer can be inferred. all those cases are pathological
        // and should be avoided. but if you see a bug, this is the first place to look.
        // expr->type.reset();
    }
    vec4f InferTypes::getEnumerationValue(ExprConstEnumeration *expr, bool &inferred) const {
        inferred = false;
        auto cfa = expr->enumType->find(expr->text);
        if (!cfa.second) {
            return v_zero();
        }
        if (!cfa.first || !cfa.first->rtti_isConstant()) {
            return v_zero();
        }
        vec4f envalue = v_zero();
        auto eva = tryGetConstExprIntOrUInt(cfa.first);
        if ( !eva.second ) {
            return v_zero();
        }
        int64_t iou = eva.first;
        switch (expr->enumType->baseType) {
        case Type::tInt8:
        case Type::tUInt8:
        case Type::tBitfield8: {
            int8_t tv = int8_t(iou);
            memcpy(&envalue, &tv, sizeof(int8_t));
            break;
        }
        case Type::tInt16:
        case Type::tUInt16:
        case Type::tBitfield16: {
            int16_t tv = int16_t(iou);
            memcpy(&envalue, &tv, sizeof(int16_t));
            break;
        }
        case Type::tInt:
        case Type::tUInt:
        case Type::tBitfield: {
            int32_t tv = int32_t(iou);
            memcpy(&envalue, &tv, sizeof(int32_t));
            break;
        }
        case Type::tInt64:
        case Type::tUInt64:
        case Type::tBitfield64:{
            memcpy(&envalue, &iou, sizeof(int64_t));
            break;
        }
        default:
            DAS_ASSERTF(0, "we should not even be here. unsupported enumeration type.");
        }
        inferred = true;
        return envalue;
    }
    bool InferTypes::isConstantType(ExprConst *c) const {
        return !c->foldedNonConst;
    }
    ExpressionPtr InferTypes::visit(ExprConst *c) {
        if (c->baseType == Type::tEnumeration || c->baseType == Type::tEnumeration8 ||
            c->baseType == Type::tEnumeration16 || c->baseType == Type::tEnumeration64) {
            auto cE = static_cast<ExprConstEnumeration *>(c);
            bool infE = false;
            c->value = getEnumerationValue(cE, infE);
            if (infE) {
                c->type = cE->enumType->makeEnumType();
                c->type->constant = isConstantType(c);
            } else {
                error("enumeration value not inferred yet " + cE->text, "", "",
                      c->at, CompilationError::invalid_enumeration);
                c->type.reset();
            }
        } else if (c->baseType == Type::tBitfield || c->baseType == Type::tBitfield8 ||
                   c->baseType == Type::tBitfield16 || c->baseType == Type::tBitfield64) {
            auto cB = static_cast<ExprConstBitfield *>(c);
            if (cB->bitfieldType) {
                TypeDecl::clone(c->type, cB->bitfieldType);
                c->type->ref = false;
            } else {
                c->type = make_smart<TypeDecl>(c->baseType);
            }
            c->type->constant = isConstantType(c);
        } else if (c->baseType == Type::tPointer) {
            c->type = make_smart<TypeDecl>(c->baseType);
            auto cptr = static_cast<ExprConstPtr *>(c);
            c->type->smartPtr = cptr->isSmartPtr;
            if (cptr->ptrType) {
                c->type->firstType = make_smart<TypeDecl>(*cptr->ptrType);
                c->type->constant = c->type->firstType->constant;
            } else {
                c->type->constant = false; // true;
            }
        } else {
            c->type = make_smart<TypeDecl>(c->baseType);
            c->type->constant = isConstantType(c);
        }
        return Visitor::visit(c);
    }
    void InferTypes::preVisit(ExprUnsafe *expr) {
        Visitor::preVisit(expr);
        unsafeDepth++;
        if (func)
            func->hasUnsafe = true;
    }
    ExpressionPtr InferTypes::visit(ExprUnsafe *expr) {
        unsafeDepth--;
        return Visitor::visit(expr);
    }
    Expression *InferTypes::findLabel(int32_t label) const {
        for (auto it = scopes.rbegin(), its = scopes.rend(); it != its; ++it) {
            auto blk = *it;
            for (const auto &ex : blk->list) {
                if (ex->rtti_isLabel()) {
                    auto lab = static_pointer_cast<ExprLabel>(ex);
                    if (lab->label == label) {
                        return lab.get();
                    }
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::visit(ExprGoto *expr) {
        if (expr->subexpr) {
            if (!expr->subexpr->type)
                return Visitor::visit(expr);
            expr->subexpr = Expression::autoDereference(expr->subexpr);
            if (!expr->subexpr->type) {
                error("goto label type can't be inferred", "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            if (!expr->subexpr->type->isSimpleType(Type::tInt)) {
                error("label type has to be int, not " + describeType(expr->subexpr->type), "", "",
                      expr->at, CompilationError::invalid_label);
            } else {
                if (enableInferTimeFolding) {
                    if (auto se = getConstExpr(expr->subexpr.get())) {
                        auto le = static_pointer_cast<ExprConstInt>(se);
                        expr->label = le->getValue();
                        expr->subexpr = nullptr;
                    }
                }
            }
        }
        scopes.back()->hasExitByLabel = true;
        for (const auto &scp : scopes.back()->list) {
            if (scp->rtti_isLabel()) {
                auto lab = static_pointer_cast<ExprLabel>(scp);
                if (lab->label == expr->label) {
                    scopes.back()->hasExitByLabel = false;
                    break;
                }
            }
        }
        if (!expr->subexpr && !findLabel(expr->label)) {
            error("can't find label " + to_string(expr->label), "", "",
                  expr->at, CompilationError::invalid_label);
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprReader *expr) {
        // implement reader macros
        auto errc = program->errors.size();
        auto substitute = expr->macro->visit(program, thisModule, expr);
        if (substitute) {
            reportAstChanged();
            return substitute;
        }
        if (errc == program->errors.size()) {
            error("unsupported read macro " + expr->macro->name, "", "",
                  expr->at, CompilationError::unsupported_read_macro);
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprLabel *expr) {
        Visitor::preVisit(expr);
        if (!labels.insert(expr->label).second) {
            error("duplicate label " + to_string(expr->label), "", "",
                  expr->at, CompilationError::invalid_label);
        }
    }
    ExpressionPtr InferTypes::visit(ExprRef2Value *expr) {
        if (!expr->subexpr->type)
            return Visitor::visit(expr);
        // infer r2v(type<Foo...>)
        if (expr->subexpr->rtti_isTypeDecl()) {
            reportAstChanged();
            if (expr->subexpr->type->isEnum()) {
                auto ewsType = make_smart<TypeDecl>(*(expr->subexpr->type));
                ewsType->ref = false;
                auto anyEnumValue = expr->subexpr->type->enumType->list.size() ? expr->subexpr->type->enumType->list[0].name : "";
                auto ews = make_smart<ExprConstEnumeration>(expr->at, anyEnumValue, ewsType);
                ews->type = ewsType;
                return ews;
            } else if (expr->subexpr->type->isWorkhorseType()) {
                auto ewsType = make_smart<TypeDecl>(*(expr->subexpr->type));
                ewsType->ref = false;
                auto ews = Program::makeConst(expr->at, ewsType, v_zero());
                ews->type = ewsType;
                return ews;
            } else {
                auto mks = make_smart<ExprMakeStruct>(expr->at);
                mks->makeType = expr->subexpr->type;
                mks->useInitializer = false;
                return mks;
            }
        }
        // mark ExprAt to be under_deref
        if (expr->subexpr->rtti_isAt()) {
            auto atExpr = static_cast<ExprAt*>(expr->subexpr.get());
            atExpr->underDeref = true;
        }
        // infer
        if (!expr->subexpr->type->isRef()) {
            if (expr->subexpr->rtti_isConstant()) {
                reportAstChanged();
                return expr->subexpr;
            } else {
                TextWriter tw;
                tw << *expr->subexpr;
                error("can only dereference a reference", tw.str(), "",
                      expr->at, CompilationError::invalid_type);
            }
        } else if (!expr->subexpr->type->isSimpleType()) {
            error("can only dereference value types, not a " + describeType(expr->subexpr->type), "", "",
                  expr->at, CompilationError::invalid_type);
        } else {
            TypeDecl::clone(expr->type, expr->subexpr->type);
            expr->type->ref = false;
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprAddr *expr) {
        if (expr->funcType) {
            // when we infer function type, we really don't care for the result.
            // however having auto or alias in the result may cause problems, so we swap it to void
            // and then swap it back to whatever it was
            auto retT = expr->funcType->firstType;
            expr->funcType->firstType = make_smart<TypeDecl>(Type::tVoid);
            if (expr->funcType->isAlias()) {
                auto aT = inferAlias(expr->funcType);
                if (aT) {
                    expr->funcType = aT;
                    reportAstChanged();
                } else {
                    error("undefined address expression type " + describeType(expr->funcType),
                          reportInferAliasErrors(expr->funcType), "", expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
            }
            if (expr->funcType->isAuto()) {
                error("function of undefined type " + describeType(expr->funcType), "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            expr->funcType->firstType = retT;
        }
        expr->func = nullptr;
        MatchingFunctions fns;
        if (expr->funcType) {
            if (!expr->funcType->isFunction()) {
                error("function of non-function type " + describeType(expr->funcType), "", "",
                      expr->at, CompilationError::type_not_found);
            }
            fns = findTypedFuncAddr(expr->target, expr->funcType->argTypes);
        } else {
            fns = findFuncAddr(expr->target);
        }
        if (fns.size() == 1) {
            expr->func = fns.back();
            expr->func->addr = true;
            expr->func->fastCall = false;
            expr->type = make_smart<TypeDecl>(Type::tFunction);
            expr->type->firstType = make_smart<TypeDecl>(*expr->func->result);
            expr->type->argTypes.reserve(expr->func->arguments.size());
            for (auto &arg : expr->func->arguments) {
                auto at = make_smart<TypeDecl>(*arg->type);
                expr->type->argTypes.push_back(at);
                expr->type->argNames.push_back(arg->name);
            }
            if (expr->func->isTemplate) {
                error("can't take address of a template function " + describeFunction(expr->func), "", "",
                      expr->at, CompilationError::function_not_found);
                expr->type.reset();
            } else {
                verifyType(expr->type);
            }
        } else if (fns.size() == 0) {
            if (!expr->funcType) {
                reportFunctionNotFound(expr);
            } else {
                error("function not found " + expr->target, "", "",
                      expr->at, CompilationError::function_not_found);
            }
        } else {
            string candidates = verbose ? program->describeCandidates(fns) : "";
            error("function not found " + expr->target, candidates, "",
                  expr->at, CompilationError::function_not_found);
        }
        if (expr->func && expr->func->builtIn) {
            TextWriter tw;
            if (verbose) {
                tw << "@@ ( ";
                for (size_t i = 0; i < expr->type->argTypes.size(); ++i) {
                    if (i)
                        tw << ", ";
                    tw << "arg" << i << " : " << describeType(expr->type->argTypes[i]);
                }
                tw << " ) : " << describeType(expr->type->firstType) << " { ";
                if (!expr->func->result->isVoid()) {
                    tw << "return ";
                }
                tw << expr->func->name << "(";
                for (size_t i = 0; i < expr->func->arguments.size(); ++i) {
                    if (i)
                        tw << ",";
                    tw << "arg" << i;
                }
                tw << "); }";
            }
            error("can't get address of builtin function " + describeFunction(expr->func), "wrap local function instead", tw.str(),
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprPtr2Ref *expr) {
        if (!expr->subexpr->type)
            return Visitor::visit(expr);
        // safe deref of non-pointer is it
        if (expr->alwaysSafe && !expr->subexpr->type->isPointer() && !(expr->subexpr->type->baseType == Type::autoinfer || expr->subexpr->type->baseType == Type::alias)) {
            reportAstChanged();
            return expr->subexpr;
        }
        expr->unsafeDeref = func ? func->unsafeDeref : false;
        // infer
        expr->subexpr = Expression::autoDereference(expr->subexpr);
        if (!expr->subexpr->type) {
            error("dereference type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (!expr->subexpr->type->isPointer()) {
            error("can only dereference a pointer", "", "",
                  expr->at, CompilationError::cant_dereference);
        } else if (!expr->subexpr->type->firstType || expr->subexpr->type->firstType->isVoid()) {
            error("can only dereference a pointer to something", "", "",
                  expr->at, CompilationError::cant_dereference);
        } else {
            TypeDecl::clone(expr->type, expr->subexpr->type->firstType);
            expr->type->ref = true;
            expr->type->constant |= expr->subexpr->type->constant;
            propagateTempType(expr->subexpr->type, expr->type); // deref(Foo#?) is Foo#
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprRef2Ptr *expr) {
        if (!expr->subexpr->type)
            return Visitor::visit(expr);
        // infer
        if (!expr->subexpr->type->isRef()) {
            error("can only make a pointer of a reference", "", "",
                  expr->at, CompilationError::cant_dereference);
        } else {
            if (!safeExpression(expr)) {
                if (!expr->subexpr->type->temporary) { // address of temp type is temp, but its safe
                    bool isSelf = false;
                    if (func && func->isClassMethod) {
                        ExprVar *mbs = nullptr;
                        if (expr->subexpr->rtti_isVar()) {
                            mbs = static_cast<ExprVar *>(expr->subexpr.get());
                        } else if (expr->subexpr->rtti_isR2V()) {
                            auto r2v = static_cast<ExprRef2Value *>(expr->subexpr.get());
                            if (r2v->subexpr->rtti_isVar()) {
                                mbs = static_cast<ExprVar *>(r2v->subexpr.get());
                            }
                        }
                        if (mbs && mbs->name == "self" && mbs->argument == true) {
                            isSelf = true;
                        }
                    }
                    if (!isSelf) {
                        error("address of reference requires unsafe", "", "",
                              expr->at, CompilationError::unsafe);
                    }
                }
            }
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*expr->subexpr->type);
            expr->type->firstType->ref = false;
            expr->type->constant |= expr->subexpr->type->constant;
            propagateTempType(expr->subexpr->type, expr->type); // addr(Foo#) is Foo#?#
        }
        return Visitor::visit(expr);
    }
    void InferTypes::propagateAlwaysSafe(const ExpressionPtr &expr) {
        if (expr->alwaysSafe)
            return;
        // make a ?as b ?? c always safe
        if (expr->rtti_isSafeAsVariant()) {
            reportAstChanged();
            auto sav = static_pointer_cast<ExprSafeAsVariant>(expr);
            sav->alwaysSafe = true;
            if (sav->value->type->isPointer()) {
                propagateAlwaysSafe(sav->value);
            }
        }
        // make a ?[b] ?? c always safe
        else if (expr->rtti_isSafeAt()) {
            reportAstChanged();
            auto sat = static_pointer_cast<ExprSafeAt>(expr);
            sat->alwaysSafe = true;
            if (sat->subexpr->type->isPointer()) {
                propagateAlwaysSafe(sat->subexpr);
            }
        }
        // make a ? b ?? c always safe (we need flag only)
        else if (expr->rtti_isSafeField()) {
            reportAstChanged();
            auto saf = static_pointer_cast<ExprSafeField>(expr);
            saf->alwaysSafe = true;
            if (saf->value->type->isPointer()) {
                propagateAlwaysSafe(saf->value);
            }
        }
    }
    ExpressionPtr InferTypes::visit(ExprNullCoalescing *expr) {
        if (!expr->subexpr->type || expr->subexpr->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->defaultValue->type || expr->defaultValue->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (auto opE = inferGenericOperator("??", expr->at, expr->subexpr, expr->defaultValue))
            return opE;
        // infer
        expr->subexpr = Expression::autoDereference(expr->subexpr);
        auto seT = expr->subexpr->type;
        if (!seT) {
            error("null coalescing type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        auto dvT = expr->defaultValue->type;
        if (!seT->isPointer()) {
            error("can only dereference a pointer", "", "",
                  expr->at, CompilationError::cant_dereference);
        } else if (!seT->firstType || seT->firstType->isVoid()) {
            error("can only dereference a pointer to something", "", "",
                  expr->at, CompilationError::cant_dereference);
        } else if (!seT->firstType->isSameType(*dvT, RefMatters::no, ConstMatters::no, TemporaryMatters::yes)) {
            error("default value type mismatch in " + describeType(seT->firstType) + " vs " + describeType(dvT), "", "",
                  expr->at, CompilationError::cant_dereference);
        } else if (seT->isRef() && !seT->isConst() && dvT->isConst()) {
            error("default value type mismatch, constant matters in " + describeType(seT) + " vs " + describeType(dvT), "", "",
                  expr->at, CompilationError::cant_dereference);
        } else {
            TypeDecl::clone(expr->type, seT->firstType);
            expr->type->constant |= expr->subexpr->type->constant || dvT->constant;
            expr->type->ref = dvT->ref;                         // only ref if default value is ref
            propagateTempType(expr->subexpr->type, expr->type); // t?# ?? def = #t
            propagateAlwaysSafe(expr->subexpr);
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprStaticAssert *expr) {
        Visitor::preVisit(expr);
        for (auto &arg : expr->arguments) {
            markNoDiscard(arg.get());
        }
    }
    ExpressionPtr InferTypes::visit(ExprStaticAssert *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() < 1 || expr->arguments.size() > 2) {
            error("static_assert(expr) or static_assert(expr,string)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->autoDereference();
        if (!expr->arguments[0]->type) {
            error("static_assert argument type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (!expr->arguments[0]->type->isSimpleType(Type::tBool))
            error("static assert condition must be boolean", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        if (expr->arguments.size() == 2 && !expr->arguments[1]->rtti_isStringConstant()) {
            error("static assert comment must be string constant", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }

        // check if we can give more info this early (buy only if we are already reporting errors)
        if (verbose && expr->arguments[0]->rtti_isConstant()) {
            bool pass = ((ExprConstBool *)(expr->arguments[0].get()))->getValue();
            if (!pass) {
                bool iscf = expr->name == "concept_assert";
                string message;
                if (expr->arguments.size() == 2 && expr->arguments[1]->rtti_isConstant()) {
                    message = ((ExprConstString *)(expr->arguments[1].get()))->getValue();
                    if (message.empty()) {
                        message = iscf ? "concept assert failed" : "static assert failed";
                    }
                } else {
                    message = iscf ? "static assert failed" : "concept failed";
                }
                if (iscf) {
                    LineInfo atC = expr->at;
                    string extra;
                    if (func) {
                        extra = "\nconcept_assert at " + expr->at.describe();
                        extra += func->getLocationExtra();
                        atC = func->getConceptLocation(atC);
                    }
                    program->error(message, extra, "", atC, CompilationError::concept_failed);
                } else {
                    string extra;
                    if (func) {
                        extra = func->getLocationExtra();
                    }
                    program->error(message, extra, "", expr->at, CompilationError::static_assert_failed);
                }
            }
        }

        expr->type = make_smart<TypeDecl>(Type::tVoid);
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprAssert *expr) {
        Visitor::preVisit(expr);
        for (auto &arg : expr->arguments) {
            markNoDiscard(arg.get());
        }
    }
    ExpressionPtr InferTypes::visit(ExprAssert *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() < 1 || expr->arguments.size() > 2) {
            error("assert(expr) or assert(expr,string)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->autoDereference();
        if (!expr->arguments[0]->type) {
            error("assert argument type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (!expr->arguments[0]->type->isSimpleType(Type::tBool))
            error("assert condition must be boolean", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        if (expr->arguments.size() == 2 && !expr->arguments[1]->rtti_isStringConstant()) {
            error("assert comment must be string constant", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        expr->type = make_smart<TypeDecl>(Type::tVoid);
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprQuote *expr) {
        if (expr->arguments.size() != 1) {
            error("quote(expr) only. can only return one expression tree", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->type = make_smart<TypeDecl>(Type::tPointer);
        expr->type->smartPtr = true;
        expr->type->smartPtrNative = true;
        expr->type->firstType = make_smart<TypeDecl>(Type::tHandle);
        expr->type->firstType->annotation = (TypeAnnotation *)Module::require("ast_core")->findAnnotation("Expression").get();
        // mark quote as noAot
        if (func) {
            if (!program->policies.aot_macros) {
                func->noAot = true;
            }
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprDebug *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() < 1 || expr->arguments.size() > 2) {
            error("debug(expr) or debug(expr,string)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        if (expr->arguments.size() == 2 && !expr->arguments[1]->rtti_isStringConstant()) {
            error("debug comment must be string constant", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        TypeDecl::clone(expr->type, expr->arguments[0]->type);
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprMemZero *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() != 1) {
            error("memzero(ref expr)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        const auto &arg = expr->arguments[0];
        if (!arg->type->isRef()) {
            error("memzero argument must be reference", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        } else if (arg->type->isConst()) {
            error("memzero argument can't be constant", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        expr->type = make_smart<TypeDecl>();
        return Visitor::visit(expr);
    }
    ExprLooksLikeCall *InferTypes::makeCallMacro(const LineInfo &at, const string &funcName) {
        vector<ExprCallFactory *> ptr;
        Module *currentModule = thisModule;
        if (func && func->fromGeneric) {
            currentModule = func->getOrigin()->module;
        }
        program->library.foreach ([&](Module *pm) -> bool {
                if ( currentModule->isVisibleDirectly(pm) ) {
                    if ( auto pp = pm->findCall(funcName) ) {
                        ptr.push_back(pp);
                    }
                }
                return true; }, "*");
        if (ptr.size() == 1) {
            return (*ptr.back())(at);
        } else if (ptr.size() > 1) {
            error("ambiguous call macro " + funcName, "", "",
                  at, CompilationError::function_not_found);
            return nullptr;
        } else {
            return nullptr;
        }
    }

    ExpressionPtr extractCastAutoDeref ( ExpressionPtr & expr ) {
        auto eFoo = expr;
        if ( eFoo->rtti_isCast() ) {
            eFoo = static_pointer_cast<ExprCast>(eFoo)->subexpr;
            if ( eFoo->rtti_isPtr2Ref() ) {
                auto ePtr2Ref = static_pointer_cast<ExprPtr2Ref>(eFoo);
                if ( ePtr2Ref->alwaysSafe ) {
                    eFoo = ePtr2Ref->subexpr;
                }
            }
        }
        return eFoo;
    }

    ExpressionPtr InferTypes::visit(ExprInvoke *expr) {
        if (expr->argumentsFailedToInfer) {
            auto blockT = expr->arguments[0]->type;
            if ( expr->isInvokeMethod && expr->arguments[0]->rtti_isField() && expr->arguments[1]->rtti_isTypeDecl() ) {
                auto eField = static_pointer_cast<ExprField>(expr->arguments[0]);
                auto eFVT = eField->value->type;
                if ( eFVT && !eFVT->isAutoOrAlias() ) {
                    // arg 1 becomes cast<auto> deref(field.value)
                    auto pCast = make_smart<ExprCast>();
                    pCast->at = expr->at;
                    pCast->castType = make_smart<TypeDecl>(Type::autoinfer);
                    pCast->subexpr = make_smart<ExprPtr2Ref>(expr->at, eField->value);
                    pCast->subexpr->alwaysSafe = true;
                    expr->arguments[1] = pCast;
                    // arg 0 becomes cast<filed_type>.fieldName
                    eField->value = make_smart<ExprTypeDecl>(expr->at, make_smart<TypeDecl>(*eFVT));
                    // and we are done
                    reportAstChanged();
                    // we don't need to wait for another pass - code bellow handles exactly this case
                    // return Visitor::visit(expr);
                } else if (func && func->isClassMethod && eField->value->rtti_isVar()) {
                    auto eVar = static_pointer_cast<ExprVar>(eField->value);
                    if (eVar->name == "super") {
                        if (auto baseClass = func->classParent->parent) {
                            reportAstChanged();
                            auto callName = "_::" + baseClass->name + "`" + eField->name;
                            auto newCall = make_smart<ExprCall>(expr->at, callName);
                            newCall->atEnclosure = expr->atEnclosure;
                            newCall->arguments.push_back(make_smart<ExprVar>(expr->at, "self"));
                            for (size_t i = 2; i != expr->arguments.size(); ++i) {
                                newCall->arguments.push_back(expr->arguments[i]);
                            }
                            return newCall;
                        } else {
                            error("call to super in " + func->name + " is not allowed, no base class for " + func->classParent->name, "", "",
                                    expr->at, CompilationError::function_not_found);
                            return Visitor::visit(expr);
                        }
                    }
                } else {
                    error("can't infer method call, field type is not inferred yet", "", "",
                          expr->at, CompilationError::function_not_found);
                    return Visitor::visit(expr);
                }
            }
            if (!blockT) {
                if (expr->isInvokeMethod) {
                    ExpressionPtr value;
                    TypeDeclPtr valueType;
                    string methodName;
                    if (expr->arguments[0]->rtti_isField()) {
                        auto eField = static_pointer_cast<ExprField>(expr->arguments[0]);
                        if (eField->value->type) { // it inferred, but field not found
                            value = extractCastAutoDeref(expr->arguments[1]);
                            valueType = eField->value->type;
                            methodName = eField->name;
                        }
                    } else if (expr->arguments[0]->rtti_isSwizzle()) {
                        auto eSwizzle = static_pointer_cast<ExprSwizzle>(expr->arguments[0]);
                        if (eSwizzle->value->type) { // it inferred, but field not found
                            value = eSwizzle->value;
                            valueType = eSwizzle->value->type;
                            methodName = eSwizzle->mask;
                        }
                    }
                    if (valueType) {
                        bool allOtherInferred = true; // we check, if all other arguments inferred
                        if (!value->type || value->type->isAliasOrExpr()) {
                            allOtherInferred = false;
                        } else {
                            for (size_t i = 2; i != expr->arguments.size(); ++i) {
                                if (!expr->arguments[i]->type) {
                                    allOtherInferred = false;
                                    break;
                                } else if (expr->arguments[i]->type->isAliasOrExpr()) {
                                    allOtherInferred = false;
                                    break;
                                }
                            }
                        }
                        if (allOtherInferred) {
                            // we build _::{field.name} ( field, arg1, arg2, ... )
                            auto callName = "_::" + methodName;
                            auto newCall = make_smart<ExprCall>(expr->at, callName);
                            newCall->atEnclosure = expr->atEnclosure;
                            newCall->alwaysSafe = expr->alwaysSafe;
                            if (value->rtti_isR2V()) {
                                value = static_pointer_cast<ExprRef2Value>(value)->subexpr;
                            }
                            newCall->arguments.push_back(value);
                            for (size_t i = 2; i != expr->arguments.size(); ++i) {
                                newCall->arguments.push_back(expr->arguments[i]);
                            }
                            auto fcall = inferFunctionCall(newCall.get(), InferCallError::tryOperator); // we infer it
                            if (fcall != nullptr || newCall->name != callName) {
                                reportAstChanged();
                                return newCall;
                            }
                            // lets try static class method
                            if (valueType->baseType == Type::tStructure) {
                                callName = "_::" + valueType->structType->name + "`" + methodName;
                                newCall->name = callName;
                                fcall = inferFunctionCall(newCall.get(), InferCallError::tryOperator);
                                if ((fcall != nullptr && fcall->isStaticClassMethod) || newCall->name != callName) {
                                    reportAstChanged();
                                    return newCall;
                                }
                            } else if (valueType->baseType == Type::tPointer && valueType->firstType && valueType->firstType->baseType == Type::tStructure) {
                                callName = "_::" + valueType->firstType->structType->name + "`" + methodName;
                                newCall->name = callName;
                                auto derefValue = make_smart<ExprPtr2Ref>(value->at, value);
                                derefValue->type = make_smart<TypeDecl>(*valueType->firstType);
                                derefValue->type->constant |= valueType->constant;
                                newCall->arguments[0] = derefValue;
                                fcall = inferFunctionCall(newCall.get(), InferCallError::tryOperator);
                                if ((fcall != nullptr && fcall->isStaticClassMethod) || newCall->name != callName) {
                                    reportAstChanged();
                                    return newCall;
                                }
                            }
                        }
                        if (auto mcall = makeCallMacro(expr->at, methodName)) {
                            mcall->arguments.push_back(value);
                            for (size_t i = 2; i != expr->arguments.size(); ++i) {
                                mcall->arguments.push_back(expr->arguments[i]);
                            }
                            reportAstChanged();
                            return mcall;
                        }
                    }
                }
            } else if (!blockT->isGoodBlockType() && !blockT->isGoodFunctionType() && !blockT->isGoodLambdaType()) {
                // no go, not a good block
            } else if (expr->arguments.size() - 1 != blockT->argTypes.size()) {
                // default arguments
                // invoke(type<foo>.GetValue,cast<auto> deref(foo))
                if (expr->isInvokeMethod) {
                    auto classDotMethod = expr->arguments[0];
                    if (classDotMethod->rtti_isR2V()) {
                        classDotMethod = static_pointer_cast<ExprRef2Value>(classDotMethod)->subexpr;
                    }
                    if (classDotMethod->rtti_isField()) {
                        auto eField = static_pointer_cast<ExprField>(classDotMethod);
                        if (eField->value->type && !eField->value->type->isAutoOrAlias()) {
                            Structure *stt = nullptr;
                            if (eField->value->type->baseType == Type::tStructure) {
                                stt = eField->value->type->structType;
                            } else if (eField->value->type->baseType == Type::tPointer && eField->value->type->firstType && eField->value->type->firstType->baseType == Type::tStructure) {
                                stt = eField->value->type->firstType->structType;
                            }
                            if (stt) {
                                auto sttf = stt->findField(eField->name);
                                if (sttf) {
                                    if (sttf->init) {
                                        smart_ptr<ExprAddr> fnAddr;
                                        if (sttf->init->rtti_isAddr()) {
                                            fnAddr = static_pointer_cast<ExprAddr>(sttf->init);
                                        } else if (sttf->init->rtti_isCast()) {
                                            auto cast = static_pointer_cast<ExprCast>(sttf->init);
                                            if (cast->subexpr->rtti_isAddr()) {
                                                fnAddr = static_pointer_cast<ExprAddr>(cast->subexpr);
                                            }
                                        }
                                        if (fnAddr) {
                                            if (fnAddr->func) {
                                                if ( inArgumentInit && fnAddr->func==func ) {
                                                    error("recursive call in argument initializer", "", "",
                                                        expr->at, CompilationError::invalid_argument_count);
                                                } else {
                                                    int fnArgSize = int(fnAddr->func->arguments.size());
                                                    int fromFnArgSize = int(expr->arguments.size() - 1);
                                                    bool allHaveInit = true;
                                                    for (int ai = fromFnArgSize; ai < fnArgSize; ++ai) {
                                                        if (!fnAddr->func->arguments[ai]->init) {
                                                            allHaveInit = false;
                                                            break;
                                                        }
                                                    }
                                                    if (allHaveInit) {
                                                        for (int ai = fromFnArgSize; ai < fnArgSize; ++ai) {
                                                            expr->arguments.emplace_back(fnAddr->func->arguments[ai]->init->clone());
                                                        }
                                                        reportAstChanged();
                                                        return Visitor::visit(expr);
                                                    }
                                                }
                                            } else {
                                                error("'" + fnAddr->target + "' is not fully resolved yet", "", "",
                                                      expr->at, CompilationError::invalid_argument_count);
                                            }
                                        } else {
                                            error("expecting class_ptr or cast<auto> class_ptr", "", "",
                                                  expr->at, CompilationError::invalid_argument_count);
                                        }
                                    } else {
                                        auto stf = sttf->type.get();
                                        if (stf && stf->dim.size() == 0 && (stf->baseType == Type::tBlock || stf->baseType == Type::tFunction || stf->baseType == Type::tLambda)) {
                                            reportAstChanged();
                                            expr->isInvokeMethod = false;
                                            // we replace invoke(foo.GetValue,cast<auto> foo,...) with invoke(foo.GetValue,...)
                                            eField->value = extractCastAutoDeref(expr->arguments[1]);
                                            expr->arguments.erase(expr->arguments.begin() + 1);
                                        } else {
                                            error("'" + stt->name + "->" + eField->name + "' expecting function", "", "",
                                                  expr->at, CompilationError::invalid_argument_count);
                                        }
                                        error("'" + stt->name + "->" + eField->name + "' expecting function", "", "",
                                              expr->at, CompilationError::invalid_argument_count);
                                    }
                                } else {
                                    error("'" + stt->name + "->" + eField->name + "' not found", "", "",
                                          expr->at, CompilationError::invalid_argument_count);
                                }
                            } else {
                                error("expecting class->method", "", "",
                                      expr->at, CompilationError::invalid_argument_count);
                            }
                        } else {
                            error("class type is not inferred yet", "", "",
                                  expr->at, CompilationError::invalid_argument_count);
                        }
                    } else {
                        error("expecting class.method in the method invoke", "", "",
                              expr->at, CompilationError::invalid_argument_count);
                    }
                }
                error("expecting " + to_string(blockT->argTypes.size()) + " arguments, got " + to_string(expr->arguments.size() - 1), "", "",
                      expr->at, CompilationError::invalid_argument_count);
            } else {
                for (size_t i = 0, is = blockT->argTypes.size(); i != is; ++i) {
                    auto &arg = expr->arguments[i + 1];
                    auto &passType = arg->type;
                    auto &argType = blockT->argTypes[i];
                    if (arg->rtti_isCast() && !passType) {
                        auto argCast = static_pointer_cast<ExprCast>(arg);
                        if (argCast->castType->isAuto()) {
                            reportAstChanged();
                            TypeDecl::clone(argCast->castType, argType);
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() < 1) {
            error("expecting invoke(block_or_function_or_lambda) or invoke(block_or_function_or_lambda,...)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->arguments[0] = Expression::autoDereference(expr->arguments[0]);
        auto blockT = expr->arguments[0]->type;
        if (!blockT) {
            error("invoke argument type can't be inferred", "", "",
                  expr->at, CompilationError::invalid_argument_type);
            return Visitor::visit(expr);
        }
        if (blockT->isAutoOrAlias()) {
            error("invoke argument not fully inferred " + describeType(blockT), "", "",
                  expr->at, CompilationError::invalid_argument_type);
            return Visitor::visit(expr);
        }
        // promote invoke(string_name,...) into string_name(...)
        if (expr->arguments[0]->rtti_isStringConstant()) {
            auto cname = static_pointer_cast<ExprConstString>(expr->arguments[0])->text;
            auto call = make_smart<ExprCall>(expr->at, cname);
            for (size_t i = 1, is = expr->arguments.size(); i < is; ++i) {
                call->arguments.push_back(expr->arguments[i]->clone());
            }
            reportAstChanged();
            return call;
        }
        if (!blockT->isGoodBlockType() && !blockT->isGoodFunctionType() && !blockT->isGoodLambdaType() && !blockT->isString()) {
            error("expecting block, or function, or lambda, or string, not a " + describeType(blockT), "", "",
                  expr->at, CompilationError::invalid_argument_type);
            return Visitor::visit(expr);
        }
        if (!blockT->isString() && expr->arguments.size() - 1 != blockT->argTypes.size()) {
            error("invalid number of arguments, expecting " + describeType(blockT), "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        if (blockT->isGoodLambdaType()) {
            expr->arguments[0] = Expression::autoDereference(expr->arguments[0]);
        }
        for (size_t i = 0, is = blockT->argTypes.size(); i != is; ++i) {
            auto &passType = expr->arguments[i + 1]->type;
            if (!passType) {
                error("invoke argument " + to_string(i + 1) + " type can't be inferred", "", "",
                      expr->at, CompilationError::invalid_argument_type);
                return Visitor::visit(expr);
            }
            auto &argType = blockT->argTypes[i];
            if (!isMatchingArgument(nullptr, argType, passType, false, false)) {
                auto extras = verbose ? ("\n" + describeMismatchingArgument(to_string(i + 1), passType, argType, (int)i)) : "";
                error("incompatible argument " + to_string(i + 1),
                      "\t" + describeType(passType) + " vs " + describeType(argType) + extras, "",
                      expr->at, CompilationError::invalid_argument_type);
                return Visitor::visit(expr);
            }
            // TODO: invoke with TEMP types
            if (!argType->isRef())
                expr->arguments[i + 1] = Expression::autoDereference(expr->arguments[i + 1]);
        }
        if (blockT->firstType) {
            TypeDecl::clone(expr->type, blockT->firstType);
        } else {
            expr->type = make_smart<TypeDecl>();
        }
        // we replace invoke/*method*/(cptr.method, cptr, ...) with invoke/*method*/(typedecl(cptr.type).method, cptr, ...)
        if (expr->isInvokeMethod && expr->arguments.size()) {
            ExprField *eField = nullptr;
            if (expr->arguments[0]->rtti_isField()) {
                eField = (ExprField *)expr->arguments[0].get();
            } else if (expr->arguments[0]->rtti_isR2V()) {
                auto eR2V = (ExprRef2Value *)expr->arguments[0].get();
                if (eR2V->subexpr->rtti_isField()) {
                    eField = (ExprField *)eR2V->subexpr.get();
                }
            }
            if (eField && !eField->value->rtti_isTypeDecl() && !eField->value->type->isAutoOrAlias()) {
                auto fType = eField->value->type->isPointer() ? eField->value->type->firstType : eField->value->type;
                auto cType = make_smart<TypeDecl>(*fType);
                auto mkType = make_smart<ExprTypeDecl>(eField->at, cType);
                cType->ref = false;
                eField->value = mkType;
                reportAstChanged();
            }
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprSetInsert *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() != 2) {
            error("insert(table,key)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
        auto containerType = expr->arguments[0]->type;
        auto valueType = expr->arguments[1]->type;
        if (!containerType || !valueType) {
            error("argument type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (containerType->isGoodTableType()) {
            if (!containerType->firstType->isSameType(*valueType, RefMatters::no, ConstMatters::no, TemporaryMatters::no))
                error("key must be of the same type as table<key,...>", "", "",
                      expr->at, CompilationError::invalid_argument_type);
            expr->type = make_smart<TypeDecl>(Type::tBool);
        } else {
            error("first argument must be fully qualified table", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        valueType->constant = true;
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprErase *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() != 2) {
            error("eraseKey(table,key)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
        auto containerType = expr->arguments[0]->type;
        auto valueType = expr->arguments[1]->type;
        if (!containerType || !valueType) {
            error("argument type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (containerType->isGoodTableType()) {
            if (!containerType->firstType->isSameType(*valueType, RefMatters::no, ConstMatters::no, TemporaryMatters::no))
                error("key must be of the same type as table<key,...>", "", "",
                      expr->at, CompilationError::invalid_argument_type);
            expr->type = make_smart<TypeDecl>(Type::tBool);
        } else {
            error("first argument must be fully qualified table", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        valueType->constant = true;
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprFind *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() != 2) {
            error("findKey(table,key)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
        auto containerType = expr->arguments[0]->type;
        auto valueType = expr->arguments[1]->type;
        if (!containerType || !valueType) {
            error("argument type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (containerType->isGoodTableType()) {
            if (!containerType->firstType->isSameType(*valueType, RefMatters::no, ConstMatters::no, TemporaryMatters::no))
                error("key must be of the same type as table<key,...>", "", "",
                      expr->at, CompilationError::invalid_argument_type);
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*containerType->secondType);
        } else {
            error("first argument must be fully qualified table", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        containerType->constant = true;
        valueType->constant = true;
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprKeyExists *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() != 2) {
            error("keyExists(table,key)", "", "",
                  expr->at, CompilationError::invalid_argument_count);
            return Visitor::visit(expr);
        }
        // infer
        expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
        auto containerType = expr->arguments[0]->type;
        auto valueType = expr->arguments[1]->type;
        if (!containerType || !valueType) {
            error("argument type can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (containerType->isGoodTableType()) {
            if (!containerType->firstType->isSameType(*valueType, RefMatters::no, ConstMatters::no, TemporaryMatters::no))
                error("key must be of the same type as table<key,...>", "", "",
                      expr->at, CompilationError::invalid_argument_type);
            expr->type = make_smart<TypeDecl>(Type::tBool);
        } else {
            error("first argument must be fully qualified table", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        }
        containerType->constant = true;
        valueType->constant = true;
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprIs *expr) {
        if (expr->typeexpr->isExprType()) {
            return Visitor::visit(expr);
        }
        if (!expr->subexpr->type || expr->subexpr->type->isAutoOrAlias()) {
            return Visitor::visit(expr);
        }
        // generic operator
        if ( !expr->typeexpr->isAutoOrAlias() ) {
            auto tname = das_to_string(expr->typeexpr->baseType);
            if (auto opE = inferGenericOperator("`is`" + tname, expr->at, expr->subexpr, nullptr))
                return opE;
            if (auto opE = inferGenericOperatorWithName("`is", expr->at, expr->subexpr, tname))
                return opE;
        } else {
            error("is " + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + " can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        auto nErrors = program->errors.size();
        if (expr->typeexpr->isAlias()) {
            if (auto eT = inferAlias(expr->typeexpr)) {
                expr->typeexpr = eT;
                reportAstChanged();
                return Visitor::visit(expr);
            } else {
                error("undefined is expression type " + describeType(expr->typeexpr),
                      reportInferAliasErrors(expr->typeexpr), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        if (expr->typeexpr->isAuto()) {
            error("is ... auto is undefined, " + describeType(expr->typeexpr), "", "",
                  expr->at, CompilationError::typeinfo_auto);
            return Visitor::visit(expr);
        }
        if (expr->typeexpr->isAutoOrAlias()) {
            error("is " + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + " can't be fully inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        verifyType(expr->typeexpr);
        if (nErrors == program->errors.size()) {
            reportAstChanged();
            bool isSame = expr->subexpr->type->isSameType(*expr->typeexpr, RefMatters::no, ConstMatters::no, TemporaryMatters::no);
            return make_smart<ExprConstBool>(expr->at, isSame);
        }
        // infer
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprTypeDecl *expr) {
        if (expr->typeexpr->isExprType()) {
            return Visitor::visit(expr);
        }
        if (expr->typeexpr->isAlias()) {
            if (auto eT = inferAlias(expr->typeexpr)) {
                eT->sanitize();
                expr->typeexpr = eT;
                reportAstChanged();
                return Visitor::visit(expr);
            } else {
                error("undefined type<" + describeType(expr->typeexpr) + ">",
                      reportInferAliasErrors(expr->typeexpr), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        if (expr->typeexpr->isAutoOrAlias()) {
            error("type<" + describeType(expr->typeexpr) + "> can't be fully inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        verifyType(expr->typeexpr, true);
        TypeDecl::clone(expr->type, expr->typeexpr);
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprTypeInfo *expr) {
        expr->macro = nullptr;
        if (expr->typeexpr && expr->typeexpr->isExprType()) {
            return Visitor::visit(expr);
        }
        if (expr->subexpr && expr->subexpr->type) {
            TypeDecl::clone(expr->typeexpr, expr->subexpr->type);
            expr->typeexpr->ref = false;
        }
        // verify
        bool allowMissingTypeExpr = false;
        if (expr->trait == "builtin_function_exists" || expr->trait == "builtin_module_exists") {
            allowMissingTypeExpr = true;
        }
        bool allowMissingType = false;
        if (expr->trait == "builtin_annotation_exists" || expr->trait == "ast_typedecl") {
            allowMissingType = true;
        }
        //
        if (!expr->typeexpr && !allowMissingTypeExpr) {
            error("typeinfo(" + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + ") can't be inferred", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        auto nErrors = program->errors.size();
        if (expr->typeexpr) {
            if (expr->typeexpr->isAlias()) {
                if (auto eT = inferAlias(expr->typeexpr)) {
                    expr->typeexpr = eT;
                    eT->sanitize();
                    reportAstChanged();
                    return Visitor::visit(expr);
                } else if (!allowMissingType) {
                    error("undefined typeinfo type expression type " + describeType(expr->typeexpr),
                          reportInferAliasErrors(expr->typeexpr), "", expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
            }
            if (expr->typeexpr->isAuto()) {
                error("typeinfo(... auto) is undefined, " + describeType(expr->typeexpr), "", "",
                      expr->at, CompilationError::typeinfo_auto);
                return Visitor::visit(expr);
            }
            if (allowMissingType ? expr->typeexpr->isAuto() : !expr->typeexpr->isFullyInferred()) {
                error("typeinfo(" + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + ") can't be fully inferred", "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            verifyType(expr->typeexpr, true);
        }
        if (nErrors == program->errors.size()) {
            if (expr->trait == "sizeof") {
                bool failed = false;
                uint64_t size = expr->typeexpr->getSizeOf64(failed);
                if (failed) {
                    error("typeinfo(sizeof " + describeType(expr->typeexpr) + ") is not fully inferred yet", "", "",
                          expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                if (size > 0x7fffffff) {
                    error("typeinfo(sizeof " + describeType(expr->typeexpr) + ") is too big", "", "",
                          expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                reportAstChanged();
                return make_smart<ExprConstInt>(expr->at, int(size));
            } else if (expr->trait == "alignof") {
                bool failed = false;
                auto align = expr->typeexpr->getAlignOfFailed(failed);
                if (failed) {
                    error("typeinfo(alignof " + describeType(expr->typeexpr) + ") is not fully inferred yet", "", "",
                          expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                reportAstChanged();
                return make_smart<ExprConstInt>(expr->at, align);
            } else if (expr->trait == "is_dim") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->dim.size() != 0);
            } else if (expr->trait == "dim") {
                if (expr->typeexpr->isExprTypeAnywhere()) {
                    error("typeinfo(dim " + describeType(expr->typeexpr) + ") is not fully inferred, expecting resolved dim", "", "",
                          expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                if (expr->typeexpr->dim.size()) {
                    reportAstChanged();
                    return make_smart<ExprConstInt>(expr->at, expr->typeexpr->dim[0]);
                } else {
                    error("typeinfo(dim non_array) is prohibited, " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::typeinfo_dim);
                }
            } else if (expr->trait == "dim_table_value") {
                if (!expr->typeexpr->isGoodTableType()) {
                    error("typeinfo(dim_table_value " + describeType(expr->typeexpr) + ") is not a table type", "", "",
                          expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                if (!expr->typeexpr->secondType) {
                    error("typeinfo(dim_table_value " + describeType(expr->typeexpr) + ") is not a table type with value", "", "",
                          expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                if (expr->typeexpr->secondType->isExprTypeAnywhere()) {
                    error("typeinfo(dim_table_value " + describeType(expr->typeexpr->secondType) + ") is not fully inferred, expecting resolved dim", "", "",
                          expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                if (expr->typeexpr->secondType->dim.size()) {
                    reportAstChanged();
                    return make_smart<ExprConstInt>(expr->at, expr->typeexpr->secondType->dim[0]);
                } else {
                    error("typeinfo(dim_table_value table<...,non_array>) is prohibited, " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::typeinfo_dim);
                }
            } else if (expr->trait == "is_any_vector") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isHandle() &&
                                                               expr->typeexpr->annotation && expr->typeexpr->annotation->isYetAnotherVectorTemplate());
            } else if (expr->trait == "variant_index" || expr->trait == "safe_variant_index") {
                if (!expr->typeexpr->isGoodVariantType()) {
                    if (expr->trait == "variant_index") {
                        error("variant_index only valid for variant, not for " + describeType(expr->typeexpr), "", "",
                              expr->at, CompilationError::invalid_type);
                    } else {
                        reportAstChanged();
                        return make_smart<ExprConstInt>(expr->at, -1);
                    }
                } else {
                    int32_t index = expr->typeexpr->findArgumentIndex(expr->subtrait);
                    if (index != -1 || expr->trait == "safe_variant_index") {
                        reportAstChanged();
                        return make_smart<ExprConstInt>(expr->at, index);
                    } else {
                        error("variant_index variant " + expr->subtrait + " not found in " + describeType(expr->typeexpr), "", "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "mangled_name") {
                if (!expr->subexpr) {
                    error("mangled name requires subexpression", "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else {
                    if (expr->subexpr->rtti_isAddr()) {
                        auto eaddr = static_pointer_cast<ExprAddr>(expr->subexpr);
                        if (!eaddr->func) {
                            error("mangled name of unknown @@function", "", "",
                                  expr->at, CompilationError::typeinfo_undefined);
                        } else {
                            reportAstChanged();
                            return make_smart<ExprConstString>(expr->at, eaddr->func->getMangledName());
                        }
                    } else {
                        error("unsupported mangled name subexpression ", expr->subexpr->__rtti, "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "is_argument") {
                if (!expr->subexpr) {
                    error("is argument requires subexpression", "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else {
                    if (expr->subexpr->rtti_isVar()) {
                        auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                        reportAstChanged();
                        return make_smart<ExprConstBool>(expr->at, func->findArgument(evar->name) != nullptr);
                    } else {
                        reportAstChanged();
                        return make_smart<ExprConstBool>(expr->at, false);
                    }
                }
            } else if (expr->trait == "typename") {
                reportAstChanged();
                return make_smart<ExprConstString>(expr->at, expr->typeexpr->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no));
            } else if (expr->trait == "undecorated_typename") {
                reportAstChanged();
                return make_smart<ExprConstString>(expr->at, expr->typeexpr->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no, TypeDecl::DescribeModule::no));
            } else if (expr->trait == "stripped_typename") {
                reportAstChanged();
                auto ctype = make_smart<TypeDecl>(*expr->typeexpr);
                ctype->constant = false;
                ctype->temporary = false;
                ctype->ref = false;
                ctype->explicitConst = false;
                return make_smart<ExprConstString>(expr->at, ctype->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no, TypeDecl::DescribeModule::yes));
            } else if (expr->trait == "fulltypename") {
                reportAstChanged();
                return make_smart<ExprConstString>(expr->at, expr->typeexpr->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::yes));
            } else if (expr->trait == "modulename") {
                reportAstChanged();
                auto modd = expr->typeexpr->module;
                return make_smart<ExprConstString>(expr->at, modd ? modd->name : "");
            } else if (expr->trait == "struct_name") {
                if (expr->typeexpr->isStructure()) {
                    reportAstChanged();
                    return make_smart<ExprConstString>(expr->at, expr->typeexpr->structType->name);
                } else {
                    error("can't get struct_name of " + expr->typeexpr->describe(), "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                }
            } else if (expr->trait == "struct_modulename") {
                if (expr->typeexpr->isStructure()) {
                    reportAstChanged();
                    auto modd = expr->typeexpr->structType->module;
                    return make_smart<ExprConstString>(expr->at, modd ? modd->name : "");
                } else {
                    error("can't get struct_modulename of " + expr->typeexpr->describe(), "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                }
            } else if (expr->trait == "is_pod") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isPod());
            } else if (expr->trait == "is_raw") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isRawPod());
            } else if (expr->trait == "is_struct") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isStructure());
            } else if (expr->trait == "is_tuple") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isTuple());
            } else if (expr->trait == "is_variant") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVariant());
            } else if (expr->trait == "is_class") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isClass());
            } else if (expr->trait == "is_lambda") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isLambda());
            } else if (expr->trait == "is_enum") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isEnum());
            } else if (expr->trait == "is_bitfield") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isBitfield());
            } else if (expr->trait == "is_string") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isString());
            } else if (expr->trait == "is_handle") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isHandle());
            } else if (expr->trait == "is_ref") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isRef());
            } else if (expr->trait == "is_ref_type") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isRefType());
            } else if (expr->trait == "is_ref_value") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, bool(expr->typeexpr->ref));
            } else if (expr->trait == "is_const") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isConst());
            } else if (expr->trait == "is_temp") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isTemp());
            } else if (expr->trait == "is_temp_type") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isTempType());
            } else if (expr->trait == "is_pointer") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isPointer());
            } else if (expr->trait == "is_smart_ptr") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->smartPtr && expr->typeexpr->isPointer());
            } else if (expr->trait == "is_iterator") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isGoodIteratorType());
            } else if (expr->trait == "is_iterable") {
                reportAstChanged();
                bool iterable = false;
                if (expr->typeexpr->dim.size()) {
                    iterable = true;
                } else if (expr->typeexpr->isGoodIteratorType()) {
                    iterable = true;
                } else if (expr->typeexpr->isGoodArrayType()) {
                    iterable = true;
                } else if (expr->typeexpr->isRange()) {
                    iterable = true;
                } else if (expr->typeexpr->isString()) {
                    iterable = true;
                } else if (expr->typeexpr->isHandle() && expr->typeexpr->annotation->isIterable()) {
                    iterable = true;
                }
                return make_smart<ExprConstBool>(expr->at, iterable);
            } else if (expr->trait == "is_vector") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVectorType());
            } else if (expr->trait == "vector_dim") {
                reportAstChanged();
                return make_smart<ExprConstInt>(expr->at, expr->typeexpr->getVectorDim());
            } else if (expr->trait == "is_array") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isGoodArrayType());
            } else if (expr->trait == "is_table") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isGoodTableType());
            } else if (expr->trait == "is_numeric") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isNumeric());
            } else if (expr->trait == "is_numeric_comparable") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isNumericComparable());
            } else if (expr->trait == "is_local") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isLocal());
            } else if (expr->trait == "is_function") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isFunction());
            } else if (expr->trait == "is_void") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVoid());
            } else if (expr->trait == "is_void_pointer") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVoidPointer());
            } else if (expr->trait == "need_inscope") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->needInScope());
            } else if (expr->trait == "can_be_placed_in_container") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canBePlacedInContainer());
            } else if (expr->trait == "can_copy") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canCopy());
            } else if (expr->trait == "can_move") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canMove());
            } else if (expr->trait == "can_clone") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canClone());
            } else if (expr->trait == "can_clone_from_const") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canCloneFromConst());
            } else if (expr->trait == "can_new") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canNew());
            } else if (expr->trait == "can_delete") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canDelete());
            } else if (expr->trait == "can_delete_ptr") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canDeletePtr());
            } else if (expr->trait == "need_delete") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->needDelete());
            } else if (expr->trait == "is_workhorse") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isWorkhorseType());
            } else if (expr->trait == "is_unsafe_when_uninitialized") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, noUnsafeUninitializedStructs && expr->typeexpr->unsafeInit());
            } else if (expr->trait == "has_nontrivial_ctor") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->hasNonTrivialCtor());
            } else if (expr->trait == "has_nontrivial_dtor") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->hasNonTrivialDtor());
            } else if (expr->trait == "has_nontrivial_copy") {
                reportAstChanged();
                return make_smart<ExprConstBool>(expr->at, expr->typeexpr->hasNonTrivialCopy());
            } else if (expr->trait == "has_field" || expr->trait == "safe_has_field") {
                auto etype = expr->typeexpr;
                if (etype->isPointer() && etype->firstType)
                    etype = etype->firstType;
                if (etype->isStructure()) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, etype->structType->findField(expr->subtrait));
                } else if (etype->isHandle()) {
                    reportAstChanged();
                    auto ft = etype->annotation->makeFieldType(expr->subtrait, false);
                    return make_smart<ExprConstBool>(expr->at, ft != nullptr);
                } else {
                    if (expr->trait == "safe_has_field") {
                        return make_smart<ExprConstBool>(expr->at, false);
                    } else {
                        error("typeinfo(has_field<" + expr->subtrait + "> ...) is only defined for structures and handled types, " + describeType(expr->typeexpr), "", "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "struct_has_annotation" || expr->trait == "struct_safe_has_annotation") {
                if (expr->typeexpr->isStructure()) {
                    reportAstChanged();
                    const auto &ann = expr->typeexpr->structType->annotations;
                    auto it = find_if(ann.begin(), ann.end(), [&](const AnnotationDeclarationPtr &pa) { return pa->annotation->name == expr->subtrait; });
                    return make_smart<ExprConstBool>(expr->at, it != ann.end());
                } else {
                    if (expr->trait == "struct_safe_has_annotation") {
                        return make_smart<ExprConstBool>(expr->at, false);
                    } else {
                        error("typeinfo(struct_has_annotation<" + expr->subtrait + "> ...) is only defined for structures, " + describeType(expr->typeexpr), "", "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "struct_has_annotation_argument" || expr->trait == "struct_safe_has_annotation_argument") {
                if (expr->typeexpr->isStructure()) {
                    const auto &ann = expr->typeexpr->structType->annotations;
                    auto it = find_if(ann.begin(), ann.end(), [&](const AnnotationDeclarationPtr &pa) { return pa->annotation->name == expr->subtrait; });
                    if (it == ann.end()) {
                        if (expr->trait == "struct_safe_has_annotation_argument") {
                            return make_smart<ExprConstBool>(expr->at, false);
                        } else {
                            error("typeinfo(struct_has_annotation_argument<" + expr->subtrait + ";" + expr->extratrait + "> ...) annotation not found ", "", "",
                                  expr->at, CompilationError::typeinfo_undefined);
                        }
                    } else {
                        reportAstChanged();
                        const auto &args = (*it)->arguments;
                        auto ita = find_if(args.begin(), args.end(), [&](const AnnotationArgument &arg) { return arg.name == expr->extratrait; });
                        return make_smart<ExprConstBool>(expr->at, ita != args.end());
                    }
                } else {
                    if (expr->trait == "struct_safe_has_annotation_argument") {
                        return make_smart<ExprConstBool>(expr->at, false);
                    } else {
                        error("typeinfo(struct_has_annotation_argument<" + expr->subtrait + "> ...) is only defined for structures, " + describeType(expr->typeexpr), "", "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "struct_get_annotation_argument") {
                if (expr->typeexpr->isStructure()) {
                    const auto &ann = expr->typeexpr->structType->annotations;
                    auto it = find_if(ann.begin(), ann.end(), [&](const AnnotationDeclarationPtr &pa) { return pa->annotation->name == expr->subtrait; });
                    if (it == ann.end()) {
                        error("typeinfo(struct_get_annotation_argument<" + expr->subtrait + ";" + expr->extratrait + "> ...) annotation not found ", "", "",
                              expr->at, CompilationError::typeinfo_undefined);
                    } else {
                        const auto &args = (*it)->arguments;
                        auto ita = find_if(args.begin(), args.end(), [&](const AnnotationArgument &arg) { return arg.name == expr->extratrait; });
                        if (ita == args.end()) {
                            error("typeinfo(struct_get_annotation_argument<" + expr->subtrait + ";" + expr->extratrait + "> ...) annotation argument not found ", "", "",
                                  expr->at, CompilationError::typeinfo_undefined);
                        } else {
                            switch (ita->type) {
                            case Type::tBool:
                                reportAstChanged();
                                return make_smart<ExprConstBool>(expr->at, ita->bValue);
                            case Type::tInt:
                                reportAstChanged();
                                return make_smart<ExprConstInt>(expr->at, ita->iValue);
                            case Type::tFloat:
                                reportAstChanged();
                                return make_smart<ExprConstFloat>(expr->at, ita->fValue);
                            case Type::tString:
                                reportAstChanged();
                                return make_smart<ExprConstString>(expr->at, ita->sValue);
                            default:
                                error("typeinfo(struct_get_annotation_argument<" + expr->subtrait + ";" + expr->extratrait + "> ...) unsupported annotation argument type ", "", "",
                                      expr->at, CompilationError::typeinfo_undefined);
                            }
                        }
                    }
                } else {
                    error("typeinfo(struct_get_annotation_argument<" + expr->subtrait + "> ...) is only defined for structures, " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                }
            } else if (expr->trait == "offsetof") {
                if (expr->typeexpr->isStructure()) {
                    auto decl = expr->typeexpr->structType->findField(expr->subtrait);
                    // NOTE: we do need to check if its fully sealed here
                    if (expr->typeexpr->isFullySealed()) {
                        reportAstChanged();
                        return make_smart<ExprConstInt>(expr->at, decl->offset);
                    } else {
                        error("typeinfo(offsetof<" + expr->subtrait + "> ...) of undefined type, " + describeType(expr->typeexpr), "", "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                } else {
                    error("typeinfo(offsetof<" + expr->subtrait + "> ...) is only defined for structures, " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                }
            } else if (expr->trait == "builtin_function_exists") {
                if (!expr->subexpr) {
                    error("builtin_function_exists requires subexpression", "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else {
                    if (expr->subexpr->rtti_isAddr()) {
                        auto eaddr = static_pointer_cast<ExprAddr>(expr->subexpr);
                        if (!eaddr->func) {
                            reportAstChanged();
                            return make_smart<ExprConstBool>(false);
                        } else if (!eaddr->func->builtIn) {
                            error("builtin_function_exists of non-builtin function @@" + describeFunction(eaddr->func), "", "",
                                  expr->at, CompilationError::typeinfo_undefined);
                        } else {
                            reportAstChanged();
                            return make_smart<ExprConstBool>(true);
                        }
                    } else {
                        error("unsupported mangled name subexpression ", expr->subexpr->__rtti, "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "builtin_module_exists") {
                if (!expr->subexpr) {
                    error("builtin_module_exists requires subexpression", "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else {
                    if (expr->subexpr->rtti_isVar()) {
                        auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                        auto mod = Module::requireEx(evar->name, false);
                        reportAstChanged();
                        return make_smart<ExprConstBool>(mod != nullptr);
                    } else {
                        error("unsupported module name subexpression ", expr->subexpr->__rtti, "",
                              expr->at, CompilationError::typeinfo_undefined);
                    }
                }
            } else if (expr->trait == "builtin_annotation_exists") {
                if (expr->typeexpr->isAlias()) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(false);
                } else if (!expr->typeexpr->isHandle()) {
                    error("builtin_function_exists requires annotation type", "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(true);
                }
            } else {
                auto mtis = program->findTypeInfoMacro(expr->trait);
                if (mtis.size() > 1) {
                    error("typeinfo(" + expr->trait + " ...) too many macros match the trait", "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else if (mtis.empty()) {
                    error("typeinfo(" + expr->trait + " ...) is undefined, " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::typeinfo_undefined);
                } else {
                    expr->macro = mtis.back().get();
                    string errors;
                    auto cexpr = expr->macro->getAstChange(expr, errors);
                    if (cexpr) {
                        reportAstChanged();
                        return cexpr;
                    } else if (!errors.empty()) {
                        error("typeinfo(" + expr->trait + " ...) macro reported error; " + errors, "", "",
                              expr->at, CompilationError::typeinfo_macro_error);
                    } else {
                        auto ctype = expr->macro->getAstType(program->library, expr, errors);
                        if (ctype) {
                            TypeDecl::clone(expr->type, ctype);
                            if (func && expr->macro->noAot(expr)) {
                                func->noAot = true;
                            }
                            return Visitor::visit(expr);
                        } else if (!errors.empty()) {
                            error("typeinfo(" + expr->trait + " ...) macro reported error; " + errors, "", "",
                                  expr->at, CompilationError::typeinfo_macro_error);
                        } else {
                            error("typeinfo(" + expr->trait + " ...) is macro integration error; no ast change and no type", "", "",
                                  expr->at, CompilationError::typeinfo_macro_error);
                        }
                    }
                }
            }
        }
        // infer
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprDelete *expr) {
        if (!expr->subexpr->type)
            return Visitor::visit(expr);
        if (expr->sizeexpr && !expr->sizeexpr->type)
            return Visitor::visit(expr);
        // lets see if there is clone operator already (a user operator can ignore all the rules bellow)
        if (!expr->native) {
            auto fnList = getFinalizeFunc(expr->subexpr->type);
            if (fnList.size()) {
                if (verifyFinalizeFunc(fnList, expr->at)) {
                    reportAstChanged();
                    // auto fn = fnList[0];
                    // string finalizeName = (fn->module->name.empty() ? "_" : fn->module->name) + "::finalize";
                    string finalizeName = "_::finalize";
                    auto finalizeFn = make_smart<ExprCall>(expr->at, finalizeName);
                    finalizeFn->arguments.push_back(expr->subexpr->clone());
                    return finalizeFn;
                } else {
                    return Visitor::visit(expr);
                }
            }
        }
        // size
        if (expr->sizeexpr) {
            expr->sizeexpr = Expression::autoDereference(expr->sizeexpr);
            if (!expr->sizeexpr->type) {
                error("delete size type can't be inferred", "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            if (!expr->sizeexpr->type->isSimpleType(Type::tInt)) {
                error("can't delete, expecting size to be int and not " + describeType(expr->sizeexpr->type), "", "",
                      expr->at, CompilationError::bad_delete);
            }
        }
        // strict unsafe delete
        if (strictUnsafeDelete && !safeExpression(expr) && !expr->subexpr->type->isSafeToDelete()) {
            error("delete of " + expr->subexpr->type->describe() + " requires unsafe", "", "",
                  expr->at, CompilationError::unsafe);
            return Visitor::visit(expr);
        }
        // infer
        if (!expr->subexpr->type->canDelete()) {
            error("can't delete " + describeType(expr->subexpr->type), "", "",
                  expr->at, CompilationError::bad_delete);
        } else if (!expr->subexpr->type->isRef()) {
            error("can only delete reference " + describeType(expr->subexpr->type), "", "",
                  expr->at, CompilationError::bad_delete);
        } else if (expr->subexpr->type->isConst()) {
            error("can't delete constant expression " + describeType(expr->subexpr->type), "", "",
                  expr->at, CompilationError::bad_delete);
        } else if (expr->subexpr->type->isPointer()) {
            if (!safeExpression(expr) && !(expr->subexpr->type->smartPtr || expr->subexpr->type->smartPtrNative)) {
                error("delete of pointer requires unsafe", "", "",
                      expr->at, CompilationError::unsafe);
            } else if (expr->subexpr->type->firstType && expr->subexpr->type->firstType->isHandle() &&
                       expr->subexpr->type->firstType->annotation->isSmart() && !expr->subexpr->type->smartPtr) {
                error("can't delete smart pointer type via regular pointer delete", "", "",
                      expr->at, CompilationError::bad_delete);
            } else if (!expr->native) {
                if (expr->subexpr->type->firstType && expr->subexpr->type->firstType->needDelete()) {
                    auto ptrf = getFinalizeFunc(expr->subexpr->type);
                    if (ptrf.size() == 0) {
                        auto fnDel = generatePointerFinalizer(expr->subexpr->type, expr->at);
                        if (!strictUnsafeDelete && !expr->alwaysSafe)
                            fnDel->unsafeOperation = true;
                        if (!program->addFunction(fnDel)) {
                            reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                            return Visitor::visit(expr);
                        } else {
                            reportAstChanged();
                        }
                    } else if (ptrf.size() > 1) {
                        string candidates = verbose ? program->describeCandidates(ptrf) : "";
                        error("too many finalizers", candidates, "",
                              expr->at, CompilationError::function_already_declared);
                        return Visitor::visit(expr);
                    }
                    reportAstChanged();
                    expr->native = true;
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return cloneFn;
                } else {
                    reportAstChanged();
                    expr->native = true;
                }
            }
        } else {
            auto finalizeType = expr->subexpr->type;
            if (finalizeType->isGoodIteratorType()) {
                reportAstChanged();
                auto cloneFn = make_smart<ExprCall>(expr->at, "_builtin_iterator_delete");
                cloneFn->arguments.push_back(expr->subexpr->clone());
                return cloneFn;
            } else if (finalizeType->isGoodArrayType() || finalizeType->isGoodTableType()) {
                reportAstChanged();
                auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                cloneFn->arguments.push_back(expr->subexpr->clone());
                return cloneFn;
            } else if (finalizeType->isStructure()) {
                auto fnDel = generateStructureFinalizer(finalizeType->structType);
                if (program->addFunction(fnDel)) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return cloneFn;
                } else {
                    reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                    return Visitor::visit(expr);
                }
            } else if (finalizeType->isTuple()) {
                auto fnDel = generateTupleFinalizer(expr->at, finalizeType);
                if (program->addFunction(fnDel)) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return cloneFn;
                } else {
                    reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                    return Visitor::visit(expr);
                }
            } else if (finalizeType->isVariant()) {
                auto fnDel = generateVariantFinalizer(expr->at, finalizeType);
                if (program->addFunction(fnDel)) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return cloneFn;
                } else {
                    reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                    return Visitor::visit(expr);
                }
            } else if (finalizeType->dim.size()) {
                reportAstChanged();
                auto cloneFn = make_smart<ExprCall>(expr->at, "finalize_dim");
                cloneFn->arguments.push_back(expr->subexpr->clone());
                return cloneFn;
            } else {
                expr->type = make_smart<TypeDecl>();
                return Visitor::visit(expr);
            }
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprCast *expr) {
        if (!expr->subexpr->type)
            return Visitor::visit(expr);
        if (expr->castType && expr->castType->isExprType()) {
            return Visitor::visit(expr);
        }
        if (expr->castType->isAlias()) {
            auto aT = inferAlias(expr->castType);
            if (aT) {
                expr->castType = aT;
                expr->castType->sanitize();
                reportAstChanged();
            } else {
                error("undefined cast type " + describeType(expr->castType),
                      reportInferAliasErrors(expr->castType), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        if (expr->castType->isAuto()) {
            error("casting to undefined type " + describeType(expr->castType), "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (expr->subexpr->type->isSameExactType(*expr->castType)) {
            reportAstChanged();
            return expr->subexpr;
        }
        if (expr->reinterpret) {
            TypeDecl::clone(expr->type, expr->castType);
            expr->type->ref = expr->subexpr->type->ref;
        } else if (expr->castType->isGoodBlockType() || expr->castType->isGoodFunctionType() || expr->castType->isGoodLambdaType()) {
            expr->type = castFunc(expr->at, expr->subexpr->type, expr->castType, expr->upcast);
        } else if (expr->castType->isHandle()) {
            if (expr->castType->isSameType(*expr->subexpr->type, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, AllowSubstitute::yes)) {
                TypeDecl::clone(expr->type, expr->castType);
                expr->type->ref = expr->subexpr->type->ref;
            } else {
                expr->type = nullptr;
            }
        } else {
            expr->type = castStruct(expr->at, expr->subexpr->type, expr->castType, expr->upcast);
        }
        if (expr->upcast || expr->reinterpret) {
            if (!safeExpression(expr)) {
                error("cast requires unsafe", "", "",
                      expr->at, CompilationError::unsafe);
            }
        }
        if (expr->type) {
            verifyType(expr->type);
        } else {
            error("can't cast " + describeType(expr->subexpr->type) + " to " + describeType(expr->castType), "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprAscend *expr) {
        Visitor::preVisit(expr);
        if (!expr->subexpr->type)
            return;
        updateNewFlags(expr);
    }
    ExpressionPtr InferTypes::visit(ExprAscend *expr) {
        if (!expr->subexpr->type)
            return Visitor::visit(expr);
        updateNewFlags(expr);
        if (!expr->subexpr->type->isRef()) {
            error("can't ascend (to heap) non-reference value", "", "",
                  expr->at, CompilationError::invalid_new_type);
        } else if (expr->subexpr->type->baseType == Type::tHandle) {
            const auto &subt = expr->subexpr->type;
            if (!subt->dim.empty()) {
                error("array of handled type cannot be allocated on the heap: '" + describeType(subt) + "'", "", "",
                      expr->at, CompilationError::invalid_new_type);
            }
            if (!subt->annotation->canNew()) {
                error("cannot allocate on the heap this handled type at all: '" + describeType(subt) + "'", "", "",
                      expr->at, CompilationError::invalid_new_type);
            }
        }
        if (expr->ascType) {
            TypeDecl::clone(expr->type, expr->ascType);
        } else {
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*expr->subexpr->type);
            expr->type->firstType->ref = false;
            if (expr->type->firstType->baseType == Type::tHandle) {
                expr->type->smartPtr = expr->type->firstType->annotation->isSmart();
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprNew *call) {
        Visitor::preVisit(call);
        call->argumentsFailedToInfer = false;
        if (call->func && call->func->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) {
            error("too many arguments in 'new', max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS), "", "",
                  call->at, CompilationError::too_many_arguments);
        }
    }
    void InferTypes::preVisitNewArg(ExprNew *call, Expression *arg, bool last) {
        Visitor::preVisitNewArg(call, arg, last);
        arg->isCallArgument = true;
    }
    void InferTypes::checkEmptyBlock(Expression *arg) {
        if (arg->rtti_isBlock()) {
            error("block can't be function argument", "", "",
                  arg->at, CompilationError::invalid_argument_type);
        }
    }
    ExpressionPtr InferTypes::visitNewArg(ExprNew *call, Expression *arg, bool last) {
        if (!arg->type)
            call->argumentsFailedToInfer = true;
        if (arg->type && arg->type->isAliasOrExpr())
            call->argumentsFailedToInfer = true;
        checkEmptyBlock(arg);
        return Visitor::visitNewArg(call, arg, last);
    }
    ExpressionPtr InferTypes::visit(ExprNew *expr) {
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (!expr->typeexpr) {
            error("new type did not infer", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (expr->typeexpr && expr->typeexpr->isExprType()) {
            return Visitor::visit(expr);
        }
        // infer
        if (expr->typeexpr->isAlias()) {
            if (auto aT = findAlias(expr->typeexpr->alias)) {
                TypeDecl::clone(expr->typeexpr, aT);
                expr->typeexpr->ref = false;      // drop a ref
                expr->typeexpr->constant = false; // drop a const
                expr->typeexpr->sanitize();
                reportAstChanged();
            } else {
                error("undefined new expression type '" + describeType(expr->typeexpr) + "'", "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        expr->name.clear();
        expr->func = nullptr;
        if (expr->typeexpr->ref) {
            error("a reference cannot be allocated on the heap", "", "",
                  expr->at, CompilationError::invalid_new_type);
        } else if (expr->typeexpr->baseType == Type::tStructure) {
            if (!expr->initializer && expr->typeexpr->structType->isClass) {
                error("invalid syntax for 'new' of class, expected syntax: 'new " + describeType(expr->typeexpr) + "()'", "", "",
                      expr->at, CompilationError::invalid_new_type);
            }
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
            expr->type->firstType->dim.clear();
            expr->type->dim = expr->typeexpr->dim;
            expr->name = expr->typeexpr->structType->getMangledName();
        } else if (expr->typeexpr->baseType == Type::tHandle) {
            if (expr->typeexpr->annotation->canNew()) {
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
                expr->type->firstType->dim.clear();
                expr->type->dim = expr->typeexpr->dim;
                expr->type->smartPtr = expr->typeexpr->annotation->isSmart();
                expr->name = expr->typeexpr->annotation->module->name + "::" + expr->typeexpr->annotation->name;
            } else {
                error("cannot allocate this type on the heap: '" + describeType(expr->typeexpr) + "'", "", "",
                      expr->at, CompilationError::invalid_new_type);
            }
        } else if (expr->typeexpr->baseType == Type::tTuple) {
            if ( expr->typeexpr->isAutoOrAlias() ) {
                error("new expression cannot be auto or alias type '" + describeType(expr->typeexpr) + "'", "", "",
                      expr->at, CompilationError::invalid_new_type);
                return Visitor::visit(expr);
            }
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
            expr->type->firstType->dim.clear();
            expr->type->dim = expr->typeexpr->dim;
            expr->name = expr->typeexpr->getMangledName();
        } else if (expr->typeexpr->baseType == Type::tVariant) {
            if ( expr->typeexpr->isAutoOrAlias() ) {
                error("new expression cannot be auto or alias type '" + describeType(expr->typeexpr) + "'", "", "",
                      expr->at, CompilationError::invalid_new_type);
                return Visitor::visit(expr);
            }
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
            expr->type->firstType->dim.clear();
            expr->type->dim = expr->typeexpr->dim;
            expr->name = expr->typeexpr->getMangledName();
        } else {
            error("only tuples, variants, structures or handled types can be allocated on the heap, not '" + describeType(expr->typeexpr) + "'", "", "",
                  expr->at, CompilationError::invalid_new_type);
        }
        if (expr->initializer && expr->name.empty()) {
            error("only native structures can have initializers, not '" + describeType(expr->typeexpr) + "'", "", "",
                  expr->at, CompilationError::invalid_new_type);
        }
        if (expr->type && expr->initializer && !expr->name.empty()) {
            auto resultType = make_smart<TypeDecl>(*expr->type);
            expr->func = inferFunctionCall(expr, InferCallError::functionOrGeneric, nullptr, false).get();
            if (!expr->func && expr->typeexpr->baseType == Type::tStructure) {
                auto saveName = expr->name;
                expr->name = "_::" + expr->typeexpr->structType->name;
                expr->func = inferFunctionCall(expr, InferCallError::functionOrGeneric, nullptr, false).get();
                if (!expr->func)
                    expr->name = saveName;
            }
            swap(resultType, expr->type);
            if (expr->func) {
                if (!expr->type->firstType->isSameType(*resultType, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes)) {
                    error("initializer returns '" + describeType(resultType) + "' vs '" + describeType(expr->type->firstType) + "'", "", "",
                          expr->at, CompilationError::invalid_new_type);
                }
            } else {
                if (expr->typeexpr->baseType == Type::tStructure &&
                    !expr->typeexpr->structType->hasAnyInitializers() && expr->arguments.empty()) {
                    expr->initializer = false;
                    reportAstChanged();
                }
                string extraError;
                if (func) {
                    extraError = "while compiling function " + func->describe();
                }
                error("'" + describeType(expr->type->firstType) + "' does not have default initializer", extraError, "",
                      expr->at, CompilationError::invalid_new_type);
            }
        }
        verifyType(expr->typeexpr);
        return Visitor::visit(expr);
    }

    ExpressionPtr InferTypes::visit(ExprAt *expr) {
        expr->underDeref = false;
        if (!expr->subexpr->type || expr->subexpr->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->index->type || expr->index->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->no_promotion && !expr->underClone) {
            if (auto opE = inferGenericOperator("[]", expr->at, expr->subexpr, expr->index)) {
                opE->alwaysSafe = expr->alwaysSafe;
                return opE;
            }
        }
        if (jitEnabled() && expr->subexpr->type->isHandle()) {
            // If `[]` not found try looking for native `.[]`.
            // In JIT we have `.[]` similar to `das_index` in aot.
            auto candidates = findMatchingFunctions("*", thisModule, ".[]", {expr->subexpr->type, expr->index->type});
            if (!candidates.empty()) {
                reportAstChanged();
                auto eachFn = make_smart<ExprCall>(expr->at, ".[]");
                eachFn->arguments.push_back(expr->subexpr->clone());
                eachFn->arguments.push_back(expr->index->clone());
                return eachFn;
            }
        }
        expr->index = Expression::autoDereference(expr->index);
        auto seT = expr->subexpr->type;
        auto ixT = expr->index->type;
        if (!ixT) {
            error("index type can't be inferred", "", "",
                  expr->index->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (seT->isGoodTableType()) {
            if (!seT->firstType->isSameType(*ixT, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                error("table index type mismatch, '" + describeType(seT->firstType) + "' vs '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            if (seT->isConst()) {
                error("cannot access the constant table by index, use 'get' instead", "", "",
                      expr->index->at, CompilationError::invalid_table_type);
                return Visitor::visit(expr);
            }
            if (ixT->temporary && seT->firstType->isTempType()) {
                error("can't index with the temporary key", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            if (seT->secondType->isVoid()) {
                error("table<...; void> cannot be accessed by index", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            if (unsafeTableLookup && !safeExpression(expr)) {
                error("table index requires unsafe", "use 'get_value', 'insert', 'insert_clone' or 'emplace' instead. consider 'get'", "",
                      expr->at, CompilationError::unsafe);
            }
            TypeDecl::clone(expr->type, seT->secondType);
            expr->type->ref = true;
            expr->type->constant |= seT->constant;
        } else if (seT->isHandle()) {
            if (!seT->annotation->isIndexable(ixT)) {
                error("handle '" + seT->annotation->name + "' does not support index type '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            expr->type = seT->annotation->makeIndexType(expr->subexpr, expr->index);
            expr->type->constant |= seT->constant;
        } else if (seT->isPointer()) {
            if (!safeExpression(expr)) {
                error("index of the pointer must be inside the 'unsafe' block", "", "",
                      expr->at, CompilationError::unsafe);
            }
            if (!ixT->isIndexExt()) {
                error("index type must be 'int', 'int64', 'uint', or 'uint64' and not '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            } else if (!seT->firstType || seT->firstType->getSizeOf() == 0) {
                error("can't index 'void' pointer", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            } else {
                expr->subexpr = Expression::autoDereference(expr->subexpr);
                TypeDecl::clone(expr->type, seT->firstType);
                expr->type->ref = true;
                expr->type->constant |= seT->constant;
            }
        } else {
            if (ixT->isRange() && (seT->isGoodArrayType() || seT->dim.size())) { // a[range(b)] into subset(a,range(b))
                auto subset = make_smart<ExprCall>(expr->at, "subarray");
                subset->arguments.push_back(expr->subexpr->clone());
                subset->arguments.push_back(expr->index->clone());
                reportAstChanged();
                return subset;
            }
            if (!ixT->isIndex()) {
                expr->type.reset();
                error("index type must be 'int' or 'uint', not '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            } else if (seT->isVectorType()) {
                expr->type = make_smart<TypeDecl>(seT->getVectorBaseType());
                expr->type->ref = seT->ref;
                expr->type->constant = seT->constant;
            } else if (seT->isGoodArrayType()) {
                TypeDecl::clone(expr->type, seT->firstType);
                expr->type->ref = true;
                expr->type->constant |= seT->constant;
            } else if (!seT->dim.size()) {
                error("type can't be indexed: '" + describeType(seT) + "'", "", "",
                      expr->subexpr->at, CompilationError::cant_index);
                return Visitor::visit(expr);
            } else if (!seT->isAutoArrayResolved()) {
                error("type dimensions are not resolved yet: '" + describeType(seT) + "'", "", "",
                      expr->subexpr->at, CompilationError::cant_index);
                return Visitor::visit(expr);
            } else {
                TypeDecl::clone(expr->type, seT);
                expr->type->ref = true;
                expr->type->dim.erase(expr->type->dim.begin());
                if (!expr->type->dimExpr.empty()) {
                    expr->type->dimExpr.erase(expr->type->dimExpr.begin());
                }
                expr->type->constant |= seT->constant;
            }
        }
        propagateTempType(expr->subexpr->type, expr->type); // foo#[a] = a#
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprSafeAt *expr) {
        if (!expr->subexpr->type || expr->subexpr->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->index->type || expr->index->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->no_promotion) {
            if (auto opE = inferGenericOperator("?[]", expr->at, expr->subexpr, expr->index))
                return opE;
        }
        if (!expr->subexpr->type->isVectorType()) {
            expr->subexpr = Expression::autoDereference(expr->subexpr);
        }
        expr->index = Expression::autoDereference(expr->index);
        auto ixT = expr->index->type;
        if (!ixT) {
            error("safe index type can't be inferred", "", "",
                  expr->index->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (expr->subexpr->type->isPointer()) {
            if (!expr->subexpr->type->firstType) {
                error("can't index 'void' pointer", "", "",
                      expr->index->at, CompilationError::cant_index);
                return Visitor::visit(expr);
            }
            auto seT = expr->subexpr->type->firstType;
            if (seT->isGoodTableType()) {
                if (!safeExpression(expr)) {
                    error("safe-index of table<> must be inside the 'unsafe' block", "", "",
                          expr->at, CompilationError::unsafe);
                }
                if (!seT->firstType->isSameType(*ixT, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                    error("table safe-index type mismatch, '" + describeType(seT->firstType) + "' vs '" + describeType(ixT) + "'", "", "",
                          expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*seT->secondType);
                expr->type->constant |= seT->constant;
            } else if (seT->isHandle()) {
                // TODO: support handle safe index
                // if (!seT->annotation->isIndexable(ixT)) {
                error("handle '" + seT->annotation->name + "' does not support safe index type '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
                // }
                // expr->type = seT->annotation->makeIndexType(expr->subexpr, expr->index);
                // expr->type->constant |= seT->constant;
            } else if (seT->isVectorType() || seT->isGoodArrayType() || seT->dim.size()) {
                // bounded types — int/uint only
                if (!ixT->isIndex()) {
                    expr->type.reset();
                    error("index type must be 'int' or 'uint', not '" + describeType(ixT) + "'", "", "",
                          expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                } else if (seT->isVectorType()) {
                    expr->type = make_smart<TypeDecl>(Type::tPointer);
                    expr->type->firstType = make_smart<TypeDecl>(seT->getVectorBaseType());
                    expr->type->firstType->constant = seT->constant;
                } else if (seT->isGoodArrayType()) {
                    if (!safeExpression(expr)) {
                        error("safe-index of array<> must be inside the 'unsafe' block", "", "",
                              expr->at, CompilationError::unsafe);
                    }
                    expr->type = make_smart<TypeDecl>(Type::tPointer);
                    expr->type->firstType = make_smart<TypeDecl>(*seT->firstType);
                    expr->type->firstType->constant |= seT->constant;
                } else if (seT->dim.size()) {
                    if (!seT->isAutoArrayResolved()) {
                        error("type dimensions are not resolved yet '" + describeType(seT) + "'", "", "",
                              expr->subexpr->at, CompilationError::cant_index);
                        return Visitor::visit(expr);
                    } else {
                        expr->type = make_smart<TypeDecl>(Type::tPointer);
                        expr->type->firstType = make_smart<TypeDecl>(*seT);
                        expr->type->firstType->dim.erase(expr->type->firstType->dim.begin());
                        if (!expr->type->firstType->dimExpr.empty()) {
                            expr->type->firstType->dimExpr.erase(expr->type->firstType->dimExpr.begin());
                        }
                        expr->type->firstType->constant |= seT->constant;
                    }
                }
            } else {
                // pointer safe-at: a?[index] where a : T*
                if (!ixT->isIndexExt()) {
                    expr->type.reset();
                    error("index type must be 'int', 'int64', 'uint', or 'uint64' and not '" + describeType(ixT) + "'", "", "",
                          expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                if (!safeExpression(expr)) {
                    error("safe-index of pointer must be inside the 'unsafe' block", "", "",
                          expr->at, CompilationError::unsafe);
                }
                expr->type = make_smart<TypeDecl>(*expr->subexpr->type);
                expr->type->constant |= seT->constant;
            }
        } else if (expr->subexpr->type->isGoodArrayType()) {
            if (!safeExpression(expr)) {
                error("safe-index of array<> must be inside the 'unsafe' block", "", "",
                      expr->at, CompilationError::unsafe);
            }
            if (!ixT->isIndex()) {
                error("index type must be 'int' or 'uint', not '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            auto seT = expr->subexpr->type;
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*seT->firstType);
            expr->type->firstType->constant |= seT->constant;
        } else if (expr->subexpr->type->isGoodTableType()) {
            if (!safeExpression(expr)) {
                error("safe-index of table<> must be inside the 'unsafe' block", "", "",
                      expr->at, CompilationError::unsafe);
            }
            const auto &seT = expr->subexpr->type;
            if (!seT->firstType->isSameType(*ixT, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                error("table safe-index type mismatch, '" + describeType(seT->firstType) + "' vs '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            if (seT->secondType->isVoid()) {
                error("can't safe-index into table<...; void>", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*seT->secondType);
            expr->type->constant |= seT->constant;
        } else if (expr->subexpr->type->dim.size()) {
            if (!safeExpression(expr)) {
                error("safe-index of fixed_array<> must be inside the 'unsafe' block", "", "",
                      expr->at, CompilationError::unsafe);
            }
            if (!ixT->isIndex()) {
                error("index type must be 'int' or 'uint', not '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            const auto &seT = expr->subexpr->type;
            if (!seT->isAutoArrayResolved()) {
                error("type dimensions are not resolved yet: '" + describeType(seT) + "'", "", "",
                      expr->subexpr->at, CompilationError::cant_index);
            }
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(*seT);
            expr->type->firstType->dim.erase(expr->type->firstType->dim.begin());
            if (!expr->type->firstType->dimExpr.empty()) {
                expr->type->firstType->dimExpr.erase(expr->type->firstType->dimExpr.begin());
            }
            expr->type->firstType->constant |= seT->constant;
        } else if (expr->subexpr->type->isVectorType() && expr->subexpr->type->isRef()) {
            if (!ixT->isIndex()) {
                error("index type must be 'int' or 'uint', not '" + describeType(ixT) + "'", "", "",
                      expr->index->at, CompilationError::invalid_index_type);
                return Visitor::visit(expr);
            }
            const auto &seT = expr->subexpr->type;
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = make_smart<TypeDecl>(seT->getVectorBaseType());
            expr->type->firstType->constant = seT->constant;
        } else {
            error("type can't be safe-indexed: '" + describeType(expr->subexpr->type) + "'", "", "",
                  expr->subexpr->at, CompilationError::cant_index);
            return Visitor::visit(expr);
        }
        propagateTempType(expr->subexpr->type, expr->type); // foo#?[a] = a?#
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprBlock *block) {
        Visitor::preVisit(block);
        block->hasEarlyOut = false;
        block->hasReturn = false;
        block->forLoop = false;
        if (block->isClosure) {
            if (block->returnType) {
                blocks.push_back(block);
                TypeDecl::clone(block->type, block->returnType);
            } else {
                error("malformed AST, closure is missing return type", "", "",
                      block->at, CompilationError::malformed_ast);
            }
        }
        scopes.push_back(block);
        inFinally.push_back(false);
        block->insideErrorCount = program->errors.size();
        pushVarStack();
        block->inFunction = func.get();
    }
    void InferTypes::preVisitBlockFinal(ExprBlock *block) {
        Visitor::preVisitBlockFinal(block);
        inFinally.back() = true;
        if (block->getFinallyEvalFlags()) {
            error("finally section can't have 'break', 'return', or 'goto'", "", "",
                  block->at, CompilationError::return_or_break_in_finally);
        }
    }
    void InferTypes::preVisitBlockArgument(ExprBlock *block, const VariablePtr &var, bool lastArg) {
        Visitor::preVisitBlockArgument(block, var, lastArg);
        if (!var->can_shadow && !program->policies.allow_block_variable_shadowing) {
            if (func) {
                for (auto &fna : func->arguments) {
                    if (fna->name == var->name) {
                        error("block argument '" + var->name + "' is shadowed by function argument '" + fna->name + ": " + describeType(fna->type) + "' at line " + to_string(fna->at.line), "", "",
                              var->at, CompilationError::variable_not_found);
                    }
                }
            }
            for (auto &blk : blocks) {
                if (blk == block)
                    continue;
                for (auto &bna : blk->arguments) {
                    if (bna->name == var->name) {
                        error("block argument '" + var->name + "' is shadowed by another block argument '" + bna->name + ": " + describeType(bna->type) + "' at line " + to_string(bna->at.line), "", "",
                              var->at, CompilationError::variable_not_found);
                    }
                }
            }
            for (auto &lv : local) {
                if (lv->name == var->name) {
                    error("block argument '" + var->name + "' is shadowed by local variable '" + lv->name + ": " + describeType(lv->type) + "' at line " + to_string(lv->at.line), "", "",
                          var->at, CompilationError::variable_not_found);
                    break;
                }
            }
            if (auto eW = hasMatchingWith(var->name)) {
                error("block argument '" + var->name + "' is shadowed by with expression at line " + to_string(eW->at.line), "", "",
                      var->at, CompilationError::variable_not_found);
            }
        }
        if (var->type->isAlias()) {
            auto aT = inferAlias(var->type);
            if (aT) {
                var->type = aT;
                reportAstChanged();
            } else {
                error("undefined block argument type '" + describeType(var->type) + "'",
                      reportInferAliasErrors(var->type), "", var->at, CompilationError::type_not_found);
            }
        }
        if (var->type->isAuto() && !var->init) {
            error("block argument type can't be inferred, it needs an "
                  "initializer or to be passed to the function with the explicit block definition",
                  "", "",
                  var->at, CompilationError::cant_infer_missing_initializer);
        }
        if (var->type->ref && var->type->isRefType()) { // silently fix a : Foo& into a : Foo
            var->type->ref = false;
        }
        if (var->type->isVoid()) {
            error("block argument type can't be declared void", "", "",
                  var->at, CompilationError::invalid_type);
        }
        verifyType(var->type, true);
    }
    ExpressionPtr InferTypes::visitBlockArgumentInit(ExprBlock *block, const VariablePtr &arg, Expression *that) {
        if (!arg->init->type) {
            error("block argument initialization with undefined expression", "", "",
                  arg->at, CompilationError::invalid_type);
            return Visitor::visitBlockArgumentInit(block, arg, that);
        }
        if (arg->type->isAuto()) {
            auto argT = TypeDecl::inferGenericType(arg->type, arg->init->type, false, false, nullptr);
            if (!argT) {
                error("block argument initialization type can't be inferred, " + describeType(arg->type) + " = " + describeType(arg->init->type), "", "",
                      arg->at, CompilationError::cant_infer_mismatching_restrictions);
            } else {
                TypeDecl::applyAutoContracts(argT, arg->type);
                arg->type = argT;
                arg->type->ref = false; // so that def ( a = VAR ) infers as def ( a : var_type ), not as def ( a : var_type & )
                reportAstChanged();
                return Visitor::visitBlockArgumentInit(block, arg, that);
            }
        }
        if (!arg->type->isAuto()) {
            if (!arg->init->type || !arg->type->isSameType(*arg->init->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                error("block argument default value type mismatch '" + describeType(arg->type) + "' vs '" + (arg->init->type ? describeType(arg->init->type) : "???") + "'", "", "",
                      arg->init->at, CompilationError::invalid_argument_type);
            }
            if (arg->init->type && arg->type->ref && !arg->init->type->ref) {
                error("block argument default value type mismatch '" + describeType(arg->type) + "' vs '" + describeType(arg->init->type) + "', reference matters", "", "",
                      arg->init->at, CompilationError::invalid_argument_type);
            }
        }
        return Visitor::visitBlockArgumentInit(block, arg, that);
    }
    ExpressionPtr InferTypes::visitBlockExpression(ExprBlock *block, Expression *that) {
        // lets do strenth reduction on ++ and -- at the top-level expressions
        if (that->type && that->rtti_isOp1() && that->type->isWorkhorseType()) {
            auto op1 = static_cast<ExprOp1 *>(that);
            if (op1->op == "+++") {
                op1->op = "++";
                reportAstChanged();
            } else if (op1->op == "---") {
                op1->op = "--";
                reportAstChanged();
            }
        }
        // lets collapse the following every time
        //  return quote() <|
        //      pass
        if (that->rtti_isMakeBlock()) {
            auto mblk = static_cast<ExprMakeBlock *>(that);
            auto iblk = static_cast<ExprBlock *>(mblk->block.get());
            if (iblk->list.empty() && iblk->finalList.empty()) {
                reportAstChanged();
                return nullptr;
            }
        }
        return Visitor::visitBlockExpression(block, that);
    }
    ExpressionPtr InferTypes::visit(ExprBlock *block) {
        // to the rest of it
        block->insideErrorCount = program->errors.size() - block->insideErrorCount;
        popVarStack();
        scopes.pop_back();
        inFinally.pop_back();
        if (block->isClosure && block->returnType) {
            blocks.pop_back();
            if (block->list.size()) {
                uint32_t flags = block->getEvalFlags();
                if (flags & EvalFlags::stopForBreak) {
                    error("captured block can't 'break' outside of the block", "", "",
                          block->at, CompilationError::invalid_block);
                }
                if (flags & EvalFlags::stopForContinue) {
                    error("captured block can't 'continue' outside of the block", "", "",
                          block->at, CompilationError::invalid_block);
                }
            }
            if (!block->hasReturn && canFoldResult && block->type->isAuto()) {
                block->returnType = make_smart<TypeDecl>(Type::tVoid);
                block->type = make_smart<TypeDecl>(Type::tVoid);
                setBlockCopyMoveFlags(block);
                reportAstChanged();
            }
            if (block->returnType) {
                if (block->returnType->isAlias()) {
                    if (auto aT = inferAlias(block->returnType)) {
                        block->returnType = aT;
                        block->returnType->sanitize();
                        if (!block->returnType->ref && block->returnType->isWorkhorseType() && !block->returnType->isPointer()) {
                            block->returnType->constant = true;
                        }
                        reportAstChanged();
                    } else {
                        error("undefined block result type '" + describeType(block->returnType) + "'",
                            reportInferAliasErrors(block->returnType), "", block->at, CompilationError::type_not_found);
                        return Visitor::visit(block);
                    }
                }
                setBlockCopyMoveFlags(block);
                verifyType(block->returnType);
            }
        }
        if (block->needCollapse) {
            block->needCollapse = false;
            if (block->collapse())
                reportAstChanged();
        }
        return Visitor::visit(block);
    }
    ExpressionPtr InferTypes::visit(ExprSwizzle *expr) {
        if (!expr->value->type)
            return Visitor::visit(expr);
        auto valT = expr->value->type;
        if (!valT->isVectorType()) {
            error("unsupported swizzle type '" + valT->describe() + "'", "", "",
                  expr->at, CompilationError::invalid_swizzle_mask);
            return Visitor::visit(expr);
        }
        int dim = valT->getVectorDim();
        if (!TypeDecl::buildSwizzleMask(expr->mask, dim, expr->fields)) {
            error("invalid swizzle mask " + expr->mask, "", "",
                  expr->at, CompilationError::invalid_swizzle_mask);
        } else {
            auto bt = valT->getVectorBaseType();
            auto rt = valT->isRange() ? TypeDecl::getRangeType(bt, int(expr->fields.size())) : TypeDecl::getVectorType(bt, int(expr->fields.size()));
            expr->type = make_smart<TypeDecl>(rt);
            expr->type->constant = valT->constant;
            expr->type->ref = valT->ref;
            if (expr->type->ref) {
                expr->type->ref &= TypeDecl::isSequencialMask(expr->fields);
            }
            if (!expr->type->ref) {
                expr->value = Expression::autoDereference(expr->value);
            }
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprAsVariant *expr) {
        if (!expr->value->type || expr->value->type->isAliasOrExpr())
            return Visitor::visit(expr);
        // implement variant macros
        ExpressionPtr substitute;
        auto modMacro = [&](Module *mod) -> bool {
            if (thisModule->isVisibleDirectly(mod) && mod != thisModule) {
                for (const auto &pm : mod->variantMacros) {
                    if ((substitute = pm->visitAs(program, thisModule, expr))) {
                        return false;
                    }
                }
            }
            return true;
        };
        Module::foreach (modMacro);
        if (!substitute)
            program->library.foreach (modMacro, "*");
        if (substitute) {
            reportAstChanged();
            return substitute;
        }
        // generic operator
        if (auto opE = inferGenericOperator("`as`" + expr->name, expr->at, expr->value, nullptr))
            return opE;
        if (auto opE = inferGenericOperatorWithName("`as", expr->at, expr->value, expr->name))
            return opE;
        // regular infer
        auto valT = expr->value->type;
        if (!valT->isGoodVariantType()) {
            error(" as '" + expr->name + "' only allowed for variants", "", "",
                  expr->at, CompilationError::invalid_type);
            return Visitor::visit(expr);
        }
        int index = valT->findArgumentIndex(expr->name);
        if (index == -1 || index >= int(valT->argTypes.size())) {
            error("can't get variant field '" + expr->name + "'", "", "",
                  expr->at, CompilationError::cant_get_field);
            return Visitor::visit(expr);
        }
        expr->fieldIndex = index;
        TypeDecl::clone(expr->type, valT->argTypes[expr->fieldIndex]);
        expr->type->ref = true;
        expr->type->constant |= valT->constant;
        propagateTempType(expr->value->type, expr->type); // a# as foo = foo#
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprSafeAsVariant *expr) {
        if (!expr->value->type || expr->value->type->isAliasOrExpr())
            return Visitor::visit(expr);
        // implement variant macros
        ExpressionPtr substitute;
        auto modMacro = [&](Module *mod) -> bool {
            if (thisModule->isVisibleDirectly(mod) && mod != thisModule) {
                for (const auto &pm : mod->variantMacros) {
                    if ((substitute = pm->visitSafeAs(program, thisModule, expr))) {
                        return false;
                    }
                }
            }
            return true;
        };
        Module::foreach (modMacro);
        if (!substitute)
            program->library.foreach (modMacro, "*");
        if (substitute) {
            reportAstChanged();
            return substitute;
        }
        // generic operator
        if (auto opE = inferGenericOperator("?as`" + expr->name, expr->at, expr->value, nullptr)) {
            reportAstChanged();
            return opE;
        }
        if (auto opE = inferGenericOperatorWithName("?as", expr->at, expr->value, expr->name)) {
            reportAstChanged();
            return opE;
        }
        // regular infer
        if (!expr->value->type->isPointer() && !safeExpression(expr)) {
            error("variant ?as on non-pointer requires unsafe", "", "",
                  expr->at, CompilationError::unsafe);
        }
        auto valT = expr->value->type->isPointer() ? expr->value->type->firstType : expr->value->type;
        if (!valT || !valT->isGoodVariantType()) {
            error(" ?as " + expr->name + " only allowed for variants or pointers to variants and not " + expr->value->type->describe(), "", "",
                  expr->at, CompilationError::invalid_type);
            return Visitor::visit(expr);
        }
        if (expr->value->type->isPointer()) {
            expr->value = Expression::autoDereference(expr->value);
        }
        int index = valT->findArgumentIndex(expr->name);
        if (index == -1 || index >= int(valT->argTypes.size())) {
            error("can't get variant field '" + expr->name + "'", "", "",
                  expr->at, CompilationError::cant_get_field);
            return Visitor::visit(expr);
        }
        expr->fieldIndex = index;
        TypeDecl::clone(expr->type, valT->argTypes[expr->fieldIndex]);
        expr->skipQQ = expr->type->isPointer();
        if (!expr->skipQQ) {
            auto fieldType = expr->type;
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = fieldType;
        }
        expr->type->constant |= valT->constant || expr->value->type->constant;
        propagateTempType(expr->value->type, expr->type); // a# ?as foo = foo?#
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprIsVariant *expr) {
        if (!expr->value->type || expr->value->type->isAliasOrExpr())
            return Visitor::visit(expr);
        // implement variant macros
        ExpressionPtr substitute;
        auto modMacro = [&](Module *mod) -> bool {
            if (thisModule->isVisibleDirectly(mod) && mod != thisModule) {
                for (const auto &pm : mod->variantMacros) {
                    if ((substitute = pm->visitIs(program, thisModule, expr))) {
                        return false;
                    }
                }
            }
            return true;
        };
        Module::foreach (modMacro);
        if (!substitute)
            program->library.foreach (modMacro, "*");
        if (substitute) {
            reportAstChanged();
            return substitute;
        }
        // generic operator
        if (auto opE = inferGenericOperator("`is`" + expr->name, expr->at, expr->value, nullptr))
            return opE;
        if (auto opE = inferGenericOperatorWithName("`is", expr->at, expr->value, expr->name))
            return opE;
        // regular infer
        auto valT = expr->value->type;
        if (!valT->isGoodVariantType()) {
            error(" is " + expr->name + " only allowed for variants", "", "",
                  expr->at, CompilationError::invalid_type);
            return Visitor::visit(expr);
        }
        int index = valT->findArgumentIndex(expr->name);
        if (index == -1 || index >= int(valT->argTypes.size())) {
            error("can't get variant field '" + expr->name + "'", "", "",
                  expr->at, CompilationError::cant_get_field);
            return Visitor::visit(expr);
        }
        expr->fieldIndex = index;
        expr->type = make_smart<TypeDecl>(Type::tBool);
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprField *expr) {
        if (expr->value->rtti_isVar() && !expr->value->type) { // if its a var expression, but it did not infer
            auto var = static_cast<ExprVar *>(expr->value.get());
            string moduleName, enumName;
            splitTypeName(var->name, moduleName, enumName);
            auto inWhichModule = getSearchModule(moduleName);
            vector<Enumeration *> possibleEnums;
            vector<TypeDecl *> possibleBitfields;
            program->library.foreach ([&](Module *mod) -> bool {
                    if ( inWhichModule->isVisibleDirectly(mod) ) {
                        if ( auto possibleEnum = mod->findEnum(enumName) ) {
                            possibleEnums.push_back(possibleEnum.get());
                        }
                        if ( auto possibleBitfield = mod->findAlias(enumName) ) {
                            if ( possibleBitfield->isBitfield() ) {
                                possibleBitfields.push_back(possibleBitfield.get());
                            }
                        }
                    }
                    return true; }, moduleName);
            if (possibleBitfields.size() && possibleEnums.size()) {
                error("ambiguous field lookup, '" + var->name + "' is both an enum and a bitfield", "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            if (possibleEnums.size() == 1) {
                reportAstChanged();
                auto td = make_smart<TypeDecl>(possibleEnums.back());
                td->constant = true;
                auto res = make_smart<ExprConstEnumeration>(makeConstAt(expr), expr->name, td);
                bool infE = false;
                res->value = getEnumerationValue(res.get(), infE);
                if (infE)
                    res->type = td;
                return res;
            } else if (possibleBitfields.size() == 1) {
                auto alias = possibleBitfields.back();
                int bit = alias->findArgumentIndex(expr->name);
                if (bit != -1) {
                    reportAstChanged();
                    auto td = make_smart<TypeDecl>(*alias);
                    td->ref = false;
                    auto bitConst = new ExprConstBitfield(makeConstAt(expr), 1ull << uint64_t(bit));
                    bitConst->bitfieldType = make_smart<TypeDecl>(*alias);
                    bitConst->type = td;
                    return bitConst;
                } else {
                    auto varName = "`" + enumName + "`" + expr->name;
                    auto vars = findMatchingVar(varName, false);
                    if (vars.size()) {
                        Variable *found = nullptr;
                        int foundCount = 0;
                        for (auto v : vars) {
                            if (v->bitfield_constant) {
                                found = v.get();
                                foundCount++;
                            }
                        }
                        if (foundCount == 1) {
                            if (!found->init) {
                                error("bitfield constant '" + expr->name + "' of type " + describeType(alias) + " is not initialized", "", "",
                                      expr->at, CompilationError::cant_get_field);
                            } else if (!found->init->type || !found->init->type->constant || !found->init->type->isBitfield()) {
                                error("not a valid bitfield constant " + expr->name + " of type " + describeType(found->type), "", "",
                                      expr->at, CompilationError::cant_get_field);
                                return Visitor::visit(expr);
                            }
                            reportAstChanged();
                            auto res = found->init->clone();
                            res->at = makeConstAt(expr);
                            return res;
                        } else {
                            TextWriter tw;
                            if (verbose) {
                                tw << "possible bitfield constants:\n";
                                for (auto v : vars) {
                                    if (v->bitfield_constant) {
                                        tw << "\t" << (v->module->name.empty() ? "_" : v->module->name) << "::" << v->name << "\n";
                                    }
                                }
                            }
                            error("bitfield constant '" + expr->name + "' of type " + describeType(alias) + " is ambiguous", tw.str(), "",
                                  expr->at, CompilationError::cant_get_field);
                            return Visitor::visit(expr);
                        }
                    } else {
                        error("bitfield '" + expr->name + "' not found in " + describeType(alias), "", "",
                              expr->at, CompilationError::cant_get_field);
                        return Visitor::visit(expr);
                    }
                }
            } else {
                if (verbose) {
                    // note - we only report this error if verbose, i.e. if we are reporting FINAL error
                    // this is an error only if things failed to compile
                    TextWriter tw;
                    if (possibleEnums.size() || possibleBitfields.size()) {
                        tw << "possible candidates:\n";
                        for (auto en : possibleEnums) {
                            tw << "\tenum " << en->name << " in " << (en->module->name.empty() ? "this module" : en->module->name) << "\n";
                        }
                        for (auto bf : possibleBitfields) {
                            tw << "\tbitfield " << bf->alias << " in " << (bf->alias.empty() ? "this module" : bf->alias) << "\n";
                        }
                        error("'" + var->name + "' is ambiguous", tw.str(), "",
                              expr->at, CompilationError::cant_get_field);
                    } else {
                        error("Don't know what '" + var->name + "' is", "", "",
                              expr->at, CompilationError::cant_get_field);
                    }
                }
            }
        }
        if (!expr->value->type || expr->value->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (expr->name.empty()) {
            error("syntax error, expecting field after '.'", "", "",
                  expr->at, CompilationError::cant_get_field);
            return Visitor::visit(expr);
        }
        if (!expr->underClone) {
            if (auto getProp = promoteToProperty(expr, nullptr)) {
                reportAstChanged();
                return getProp;
            }
        }
        auto valT = expr->value->type;
        if (valT->isPointer()) {
            expr->value = Expression::autoDereference(expr->value);
            expr->unsafeDeref = func ? func->unsafeDeref : false;
            if (!valT->firstType) {
                error("can't get field '" + expr->name + "' of 'void' pointer", "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            } else if (valT->firstType->isStructure()) {
                expr->fieldRef = valT->firstType->structType->findFieldRef(expr->name);
                if (!expr->fieldRef && valT->firstType->structType->hasStaticMembers) {
                    auto fname = valT->firstType->structType->name + "`" + expr->name;
                    if (auto pVar = valT->firstType->structType->module->findVariable(fname)) {
                        if (pVar->static_class_member) {
                            reportAstChanged();
                            return make_smart<ExprVar>(expr->at, fname);
                        }
                    }
                }
            } else if (valT->firstType->isHandle()) {
                expr->annotation = valT->firstType->annotation;
                expr->type = expr->annotation->makeFieldType(expr->name, valT->constant || valT->firstType->constant);
                if (expr->type)
                    expr->type->constant |= valT->constant || valT->firstType->constant;
            } else if (valT->firstType->isGoodTupleType()) {
                int index = valT->tupleFieldIndex(expr->name);
                if (index == -1 || index >= int(valT->firstType->argTypes.size())) {
                    error("can't get tuple field '" + expr->name + "'", "", "",
                          expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
            } else if (valT->firstType->isGoodVariantType()) {
                if (!safeExpression(expr)) {
                    error("'variant.field' must be inside the 'unsafe' block", "", "",
                          expr->at, CompilationError::unsafe);
                    return Visitor::visit(expr);
                }
                int index = valT->variantFieldIndex(expr->name);
                if (index == -1 || index >= int(valT->firstType->argTypes.size())) {
                    error("can't get variant field '" + expr->name + "'", "", "",
                          expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
            }
        } else {
            if (valT->isVectorType()) {
                reportAstChanged();
                return make_smart<ExprSwizzle>(expr->at, expr->value, expr->name);
            } else if (valT->isBitfield()) {
                expr->value = Expression::autoDereference(expr->value);
                valT = expr->value->type;
                if (!valT) {
                    error("bitfield type can't be inferred", "", "",
                          expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                int index = valT->bitFieldIndex(expr->name);
                if (index == -1 || index >= int(valT->argNames.size())) {
                    error("can't get bit field '" + expr->name + "' in " + describeType(valT), "", "",
                          expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
            } else if (valT->isHandle()) {
                expr->annotation = valT->annotation;
                expr->type = expr->annotation->makeFieldType(expr->name, valT->constant);
            } else if (valT->isStructure()) {
                expr->fieldRef = valT->structType->findFieldRef(expr->name);
                if (!expr->fieldRef && valT->structType->hasStaticMembers) {
                    auto fname = valT->structType->name + "`" + expr->name;
                    if (auto pVar = valT->structType->module->findVariable(fname)) {
                        if (pVar->static_class_member) {
                            reportAstChanged();
                            return make_smart<ExprVar>(expr->at, fname);
                        }
                    }
                }
            } else if (valT->isGoodTupleType()) {
                int index = valT->tupleFieldIndex(expr->name);
                if (index == -1 || index >= int(valT->argTypes.size())) {
                    error("can't get tuple field '" + expr->name + "'", "", "",
                          expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
            } else if (valT->isGoodVariantType()) {
                if (!safeExpression(expr)) {
                    error("variant.field requires unsafe", "", "",
                          expr->at, CompilationError::unsafe);
                    return Visitor::visit(expr);
                }
                int index = valT->variantFieldIndex(expr->name);
                if (index == -1 || index >= int(valT->argTypes.size())) {
                    error("can't get variant field '" + expr->name + "'", "", "",
                          expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
            } else {
                error("can't get field '" + expr->name + "' of " + describeType(expr->value->type), "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
        }
        // lets verify private field lookup
        if (!verifyPrivateFieldLookup(expr)) {
            return Visitor::visit(expr);
        }
        // handle
        if (expr->fieldRef) {
            if (expr->fieldRef->type->isAliasOrExpr()) {
                error("undefined field type '" + describeType(expr->fieldRef->type) + "'",
                      reportInferAliasErrors(expr->fieldRef->type), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            TypeDecl::clone(expr->type, expr->fieldRef->type);
            expr->type->ref = true;
            expr->type->constant |= valT->constant;
            if (valT->isPointer() && valT->firstType) {
                expr->type->constant |= valT->firstType->constant;
            }
            if (!expr->ignoreCaptureConst) {
                expr->type->constant |= expr->fieldRef->capturedConstant;
            }
        } else if (expr->fieldIndex != -1) {
            if (valT->isBitfield()) {
                expr->type = make_smart<TypeDecl>(Type::tBool);
            } else {
                auto tupleT = valT->isPointer() ? valT->firstType : valT;
                auto & tt = tupleT->argTypes[expr->fieldIndex];
                if ( !tt->isRefType() && tt->ref ) {
                    error("can't get field '" + expr->name + "', invalid field type '" + describeType(tt), "", "",
                          expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                TypeDecl::clone(expr->type, tt);
                expr->type->ref = true;
                expr->type->constant |= tupleT->constant;
            }
        } else if (!expr->type) {
            if (verbose && valT) {
                MatchingFunctions mf;
                collectMissingOperators(".`" + expr->name, mf, false);
                collectMissingOperators(".", mf, true);
                if (!mf.empty()) {
                    reportDualFunctionNotFound(".`" + expr->name, "field '" + expr->name + "' not found in " + describeType(valT),
                                               expr->at, mf, {valT}, {valT, make_smart<TypeDecl>(Type::tString)}, true, false, true,
                                               CompilationError::cant_get_field, 0, "");
                } else {
                    error("field '" + expr->name + "' not found in " + describeType(valT), "", "",
                          expr->at, CompilationError::cant_get_field);
                }
            } else {
                error("field '" + expr->name + "' not found in " + describeType(valT), "", "",
                      expr->at, CompilationError::cant_get_field);
            }
            return Visitor::visit(expr);
        } else {
            expr->type->constant |= valT->constant;
        }
        propagateTempType(expr->value->type, expr->type); // a#.foo = foo#
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprSafeField *expr) {
        if (!expr->value->type || expr->value->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->no_promotion) {
            if (auto opE = inferGenericOperator("?.`" + expr->name, expr->at, expr->value, nullptr))
                return opE;
            if (auto opE = inferGenericOperatorWithName("?.", expr->at, expr->value, expr->name))
                return opE;
        }
        auto valT = expr->value->type;
        if (!(valT->isPointer() && valT->firstType) && !valT->isVariant()) {
            if (verbose && !expr->no_promotion) {
                MatchingFunctions mf;
                collectMissingOperators("?.`" + expr->name, mf, false);
                collectMissingOperators("?.", mf, true);
                if (!mf.empty()) {
                    reportDualFunctionNotFound("?.`" + expr->name, "can only safe dereference a variant or a pointer to a tuple, a structure or a handle " + describeType(valT),
                                               expr->at, mf, {expr->value->type}, {expr->value->type, make_smart<TypeDecl>(Type::tString)}, true, false, true,
                                               CompilationError::cant_get_field, 0, "");
                } else {
                    error("can only safe dereference a variant or a pointer to a tuple, a structure or a handle " + describeType(valT), "", "",
                          expr->at, CompilationError::cant_get_field);
                }
            } else {
                error("can only safe dereference a variant or a pointer to a tuple, a structure or a handle " + describeType(valT), "", "",
                      expr->at, CompilationError::cant_get_field);
            }
            return Visitor::visit(expr);
        }
        expr->value = Expression::autoDereference(expr->value);
        if (valT->isGoodVariantType() || valT->firstType->isGoodVariantType()) {
            int index = valT->variantFieldIndex(expr->name);
            auto argSize = valT->isGoodVariantType() ? valT->argTypes.size() : valT->firstType->argTypes.size();
            if (index == -1 || index >= static_cast<int>(argSize)) {
                error("can't get variant field '" + expr->name + "'", "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            reportAstChanged();
            auto safeAs = make_smart<ExprSafeAsVariant>(expr->at, expr->value, expr->name);
            return safeAs;
        } else if (valT->firstType->structType) {
            expr->fieldRef = valT->firstType->structType->findFieldRef(expr->name);
            if (!expr->fieldRef) {
                error("can't safe get field '" + expr->name + "'", "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            } else if (expr->fieldRef->type->isAliasOrExpr()) {
                error("undefined safe field type '" + describeType(expr->fieldRef->type) + "'",
                      reportInferAliasErrors(expr->fieldRef->type), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            TypeDecl::clone(expr->type, expr->fieldRef->type);
        } else if (valT->firstType->isHandle()) {
            expr->annotation = valT->firstType->annotation;
            expr->type = expr->annotation->makeSafeFieldType(expr->name, valT->constant);
            if (!expr->type) {
                error("can't safe get field '" + expr->name + "'", "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
        } else if (valT->firstType->isGoodTupleType()) {
            int index = valT->tupleFieldIndex(expr->name);
            if (index == -1 || index >= int(valT->firstType->argTypes.size())) {
                error("can't get tuple field '" + expr->name + "'", "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            expr->fieldIndex = index;
            auto & tt = valT->firstType->argTypes[expr->fieldIndex];
            if ( !tt->isRefType() && tt->ref ) {
                error("can't safe get field '" + expr->name + "', invalid field type '" + describeType(tt), "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            TypeDecl::clone(expr->type, tt);
        } else {
            error("can only safe dereference a pointer to a tuple, a structure or a handle " + describeType(valT), "", "",
                  expr->at, CompilationError::cant_get_field);
            return Visitor::visit(expr);
        }
        expr->skipQQ = expr->type->isPointer();
        if (!expr->skipQQ) {
            auto fieldType = expr->type;
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->firstType = fieldType;
        }
        expr->type->constant |= valT->constant;
        propagateTempType(expr->value->type, expr->type); // a#?.foo = foo?#
        // lets verify private field lookup
        verifyPrivateFieldLookup(expr);
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprTag *expr) {
        Visitor::preVisit(expr);
        error("macro tags can only appear in macro blocks", "", "", expr->at, CompilationError::unbound_macro_tag);
    }
    void InferTypes::preVisit(ExprVar *expr) {
        Visitor::preVisit(expr);
        expr->variable = nullptr;
        expr->local = false;
        expr->block = false;
        expr->pBlock = nullptr;
        expr->argument = false;
        expr->argumentIndex = -1;
    }
    ExpressionPtr InferTypes::visit(ExprVar *expr) {
        // assume (that on the stack)
        for (auto it = assume.rbegin(), its = assume.rend(); it != its; ++it) {
            auto & ita = *it;
            if (ita.expr->alias == expr->name) {
                reportAstChanged();
                auto csub = ita.expr->subexpr->clone();
                // forceAt(csub, ita.expr->at);
                return csub;
            }
        }
        // local (that on the stack)
        for (auto it = local.rbegin(); it != local.rend(); ++it) {
            auto var = *it;
            if (var->name == expr->name || var->aka == expr->name) {
                expr->variable = var;
                expr->local = true;
                TypeDecl::clone(expr->type, var->type);
                expr->type->ref = true;
                var->used_in_finally = inFinally.empty() ? false : inFinally.back();
                if (consumeDepth) {
                    var->consumed = true;
                }
                return Visitor::visit(expr);
            }
        }
        // block arguments
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            ExprBlock *block = *it;
            int argumentIndex = 0;
            for (auto &arg : block->arguments) {
                if (arg->name == expr->name || arg->aka == expr->name) {
                    expr->variable = arg;
                    expr->argumentIndex = argumentIndex;
                    expr->block = true;
                    if (blocks.rbegin() == it) {
                        expr->thisBlock = true;
                    }
                    TypeDecl::clone(expr->type, arg->type);
                    if (!expr->type->isRefType())
                        expr->type->ref = true;
                    expr->type->sanitize();
                    expr->pBlock = static_cast<ExprBlock *>(block);
                    return Visitor::visit(expr);
                }
                argumentIndex++;
            }
        }
        // function argument
        if (func) {
            int argumentIndex = 0;
            for (auto &arg : func->arguments) {
                if (arg->name == expr->name || arg->aka == expr->name) {
                    expr->variable = arg;
                    expr->argumentIndex = argumentIndex;
                    expr->argument = true;
                    TypeDecl::clone(expr->type, arg->type);
                    if (!expr->type->isRefType())
                        expr->type->ref = true;
                    expr->type->sanitize();
                    return Visitor::visit(expr);
                }
                argumentIndex++;
            }
        }
        // with
        if (auto eW = hasMatchingWith(expr->name)) {
            reportAstChanged();
            return make_smart<ExprField>(expr->at, forceAt(eW->with->clone(), expr->at), expr->name);
        }
        // with prop
        if (!expr->underClone) {
            if (auto getProp = promoteToProperty(expr, nullptr)) {
                reportAstChanged();
                return getProp;
            }
        }
        // static class method accessing static variables
        if (func && func->isStaticClassMethod && func->classParent->hasStaticMembers) {
            auto staticVarName = func->classParent->name + "`" + expr->name;
            if (func->classParent->module->findVariable(staticVarName)) {
                reportAstChanged();
                return make_smart<ExprVar>(expr->at, staticVarName);
            }
        }
        // global
        auto vars = findMatchingVar(expr->name, false);
        if (vars.size() == 1) {
            auto var = vars.back();
            if (var == globalVar) {
                error("global variable '" + expr->name + "' cant't be initialized with itself",
                      "", "", expr->at, CompilationError::variable_not_found);
                return Visitor::visit(expr);
            }
            expr->variable = var;
            TypeDecl::clone(expr->type, var->type);
            expr->type->ref = true;
            return Visitor::visit(expr);
        } else if (vars.size() == 0) {
            if (verbose) {
                vars = findMatchingVar(expr->name, true);
                if (vars.size()) {
                    TextWriter errs;
                    for (auto &var : vars) {
                        errs << "\t" << var->module->name << "::" << var->name << ": " << describeType(var->type);
                        if (var->at.line) {
                            errs << " at " << var->at.describe();
                        }
                        errs << "\n";
                    }
                    error("can't access private variable '" + expr->name + "'", "not visible due to privacy:\n" + errs.str(), "",
                          expr->at, CompilationError::variable_not_found);
                } else {
                    error("can't locate variable '" + expr->name + "'", "", "",
                          expr->at, CompilationError::variable_not_found);
                }
            } else {
                error("can't locate variable '" + expr->name + "'", "", "",
                      expr->at, CompilationError::variable_not_found);
            }
        } else {
            if (verbose) {
                TextWriter errs;
                for (auto &var : vars) {
                    errs << "\t" << var->module->name << "::" << var->name << ": " << describeType(var->type);
                    if (var->at.line) {
                        errs << " at " << var->at.describe();
                    }
                    errs << "\n";
                }
                error("too many matching variables '" + expr->name + "'", "candidates are:\n" + errs.str(), "",
                      expr->at, CompilationError::variable_not_found);
            } else {
                error("too many matching variables '" + expr->name + "'", "", "",
                      expr->at, CompilationError::variable_not_found);
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprReturn *expr) {
        Visitor::preVisit(expr);
        expr->block = nullptr;
        expr->returnType.reset();
        // ok, now lets mark early outs for the block chain
        auto i = scopes.size();
        while (i > 0) {
            i--;
            scopes[i]->hasEarlyOut = true;
            if (scopes[i]->isClosure)
                break;
        }
        if (expr->subexpr)
            markNoDiscard(expr->subexpr.get());
    }
    ExpressionPtr InferTypes::visit(ExprReturn *expr) {
        if (blocks.size()) {
            ExprBlock *block = blocks.back();
            expr->block = block;
            block->hasReturn = true;
            if (expr->subexpr) {
                if (!expr->subexpr->type)
                    return Visitor::visit(expr);
                if (!block->returnType->ref) {
                    expr->subexpr = Expression::autoDereference(expr->subexpr);
                } else {
                    expr->returnReference = true;
                }
                expr->returnInBlock = true;
            }
            if (inferReturnType(block->type, expr)) {
                TypeDecl::clone(block->returnType, block->type);
                setBlockCopyMoveFlags(block);
            }
            if (block->moveOnReturn && !expr->moveSemantics && expr->subexpr) {
                string details, suggestions;
                getDetailsAndSuggests(expr, details, suggestions);
                error(details + ": " + describeType(block->type), "", suggestions,
                      expr->at, CompilationError::invalid_return_semantics);
                if (canRelaxAssign(expr->subexpr.get())) {
                    reportAstChanged();
                    expr->moveSemantics = true;
                }
            }
            if (block->returnType && block->returnType->ref && !safeExpression(expr)) {
                error("returning reference requires unsafe", "", "",
                      expr->at, CompilationError::invalid_return_type);
            }
            if (block->returnType && block->returnType->isTemp() && !safeExpression(expr)) {
                error("returning temporary value from block requires unsafe", "", "",
                      expr->at, CompilationError::invalid_return_type);
            }
            if (strictSmartPointers && block->returnType && !expr->moveSemantics && !safeExpression(expr) && block->returnType->needInScope()) {
                error("returning smart pointers without move semantics is unsafe", "use return <- instead", "",
                      expr->at, CompilationError::unsafe);
            }
            if (block->returnType) {
                TypeDecl::clone(expr->returnType, block->returnType);
            }
        } else {
            // infer
            func->hasReturn = true;
            if (expr->subexpr) {
                if (!expr->subexpr->type)
                    return Visitor::visit(expr);
                if (!func->result->ref) {
                    if (!expr->moveSemantics) {
                        expr->subexpr = Expression::autoDereference(expr->subexpr);
                    }
                } else {
                    expr->returnReference = true;
                }
            }
            inferReturnType(func->result, expr);
            if (func->moveOnReturn && !expr->moveSemantics && expr->subexpr) {
                string details, suggestions;
                getDetailsAndSuggests(expr, details, suggestions);
                error(details + ": " + describeType(func->result), "", suggestions,
                      expr->at, CompilationError::invalid_return_semantics);
                if (canRelaxAssign(expr->subexpr.get())) {
                    reportAstChanged();
                    expr->moveSemantics = true;
                }
            }
            if (func->result->ref && !safeExpression(expr)) {
                error("returning reference requires unsafe", "", "",
                      func->result->at, CompilationError::invalid_return_type);
            }
            if (func->result->isTemp() && !safeExpression(expr)) {
                error("returning temporary value from function requires unsafe", "", "",
                      func->result->at, CompilationError::invalid_return_type);
            }
            if (strictSmartPointers && !expr->moveSemantics && !safeExpression(expr) && func->result->needInScope()) {
                error("returning smart pointers without move semantics is unsafe", "use return <- instead", "",
                      expr->at, CompilationError::unsafe);
            }
            if (func->result) {
                TypeDecl::clone(expr->returnType, func->result);
            }
        }
        expr->type = make_smart<TypeDecl>();
        if (forceInscopePod && func) {
            if (returnCount == 0)
                oneReturn = expr;
            returnCount++;
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprYield *expr) {
        if (blocks.size()) {
            error("can't yield from inside the block", "", "",
                  expr->at, CompilationError::invalid_yield);
        } else {
            if (!func->generator) {
                error("can't yield from non-generator", "", "",
                      expr->at, CompilationError::invalid_yield);
                return Visitor::visit(expr);
            }
            if (!expr->subexpr->type)
                return Visitor::visit(expr);
            // const auto & yarg = func->arguments[1];
            // TODO: verify yield type so that error is 'yield' error, not copy or move error
            auto blk = generateYield(expr, func);
            scopes.back()->needCollapse = true;
            reportAstChanged();
            return blk;
        }
        expr->type = make_smart<TypeDecl>();
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprBreak *expr) {
        if (!loop.size())
            error("'break' without a loop", "", "",
                  expr->at, CompilationError::invalid_loop);
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprContinue *expr) {
        if (!loop.size())
            error("'continue' without a loop", "", "",
                  expr->at, CompilationError::invalid_loop);
        return Visitor::visit(expr);
    }
    bool InferTypes::canVisitIfSubexpr(ExprIfThenElse *expr) {
        if (expr->isStatic) {
            // static_if prevents normal resolve flow
            //  so we say 'hasReturn' for current block or function
            //  to prevent it from becoming void
            //  until static_if resolves that is
            if (blocks.size()) {
                ExprBlock *block = blocks.back();
                block->hasReturn = true;
            } else {
                func->hasReturn = true;
            }
        }
        return !expr->isStatic;
    }
    void InferTypes::preVisit(ExprIfThenElse *expr) {
        Visitor::preVisit(expr);
        if (expr->cond)
            markNoDiscard(expr->cond.get());
    }
    ExpressionPtr InferTypes::visit(ExprIfThenElse *expr) {
        if (!expr->cond->type) {
            return Visitor::visit(expr);
        }
        // infer
        if (!expr->cond->type->isSimpleType(Type::tBool)) {
            error("if-then-else condition must be boolean, and not " + expr->cond->type->describe(), "", "",
                  expr->at, CompilationError::condition_must_be_bool);
            return Visitor::visit(expr);
        }
        if (expr->cond->type->isRef()) {
            expr->cond = Expression::autoDereference(expr->cond);
        }
        // now, for the static if
        if ((enableInferTimeFolding && !expr->doNotFold) || expr->isStatic) {
            if (auto constCond = getConstExpr(expr->cond.get())) {
                reportAstChanged();
                auto condR = static_pointer_cast<ExprConstBool>(constCond)->getValue();
                if (condR) {
                    return expr->if_true;
                } else {
                    return expr->if_false;
                }
            } else if (expr->isStatic) {
                error("static_if must resolve to constant", "", "",
                      expr->at, CompilationError::condition_must_be_static);
                return Visitor::visit(expr);
            }
        }
        // now, to unwrap the generator
        if (func && func->generator) {
            // only topmost
            //  which in case of generator is 2, due to
            //  def GENERATOR { with LAMBDA { ...collapse here... } }
            if (!blocks.empty() /* || scopes.size()!=2 */) {
                return Visitor::visit(expr);
            }
            uint32_t tf = expr->if_true->getEvalFlags();
            uint32_t ff = expr->if_false ? expr->if_false->getEvalFlags() : 0;
            if ((tf | ff) & EvalFlags::yield) { // only unwrap if it has "yield"
                // verify if it can be cloned at all
                if (expr->if_true->rtti_isBlock()) {
                    auto iftb = static_pointer_cast<ExprBlock>(expr->if_true);
                    if (!iftb->finalList.empty()) {
                        error("can't have final section in the if-then-else inside generator yet", "", "",
                              expr->at, CompilationError::invalid_yield);
                        return Visitor::visit(expr);
                    }
                }
                if (expr->if_false && expr->if_false->rtti_isBlock()) {
                    auto iffb = static_pointer_cast<ExprBlock>(expr->if_false);
                    if (!iffb->finalList.empty()) {
                        error("can't have final section in the if-then-else inside generator yet", "", "",
                              expr->at, CompilationError::invalid_yield);
                        return Visitor::visit(expr);
                    }
                }
                auto blk = replaceGeneratorIfThenElse(expr, func);
                scopes.back()->needCollapse = true;
                reportAstChanged();
                return blk;
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprAssume *expr) {
        Visitor::preVisit(expr);
        const auto &name = expr->alias;
        // assume
        if (expr->subexpr) {
            for (const auto &aa : assume) {
                if (aa.expr->alias == name) {
                    error("can't assume " + name + ", alias already taken by another assume expression at " + aa.expr->at.describe(), "", "",
                          expr->at, CompilationError::invalid_assume);
                    return;
                }
            }
            // local variable
            for (const auto &lv : local) {
                if (lv->name == name || lv->aka == name) {
                    error("can't assume " + name + ", alias already taken by local variable at " + lv->at.describe(), "", "",
                          expr->at, CompilationError::invalid_assume);
                    return;
                }
            }
            // with
            if (auto mW = hasMatchingWith(name)) {
                error("can't assume " + name + ", alias already taken by `with` at " + mW->at.describe(), "", "",
                      expr->at, CompilationError::invalid_assume);
                return;
            }
            // block arguments
            for (const auto &block : blocks) {
                for (const auto &arg : block->arguments) {
                    if (arg->name == name || arg->aka == name) {
                        error("can't assume " + name + ", alias already taken by block argument at " + arg->at.describe(), "", "",
                              expr->at, CompilationError::invalid_assume);
                        return;
                    }
                }
            }
            // function argument
            if (func) {
                for (auto &arg : func->arguments) {
                    if (arg->name == name || arg->aka == name) {
                        error("can't assume " + name + ", alias already taken by block argument at " + arg->at.describe(), "", "",
                              expr->at, CompilationError::invalid_assume);
                        return;
                    }
                }
            }
            // global
            auto globals = findMatchingVar(name, false);
            if (globals.size()) {
                if (globals.size() == 1) {
                    error("can't assume " + name + ", alias already taken by global variable at " + globals[0]->at.describe(), "", "",
                          expr->at, CompilationError::invalid_assume);
                } else {
                    error("can't assume " + name + ", alias already taken by multiple global variables", "", "",
                          expr->at, CompilationError::invalid_assume);
                }
                return;
            }
        } else {
            auto clashAlias = findAlias(name);
            if (clashAlias) {
                string extra;
                if (verbose) {
                    auto atClash = clashAlias->getDeclarationLocation();
                    if (!atClash.empty()) {
                        extra = "previously declarated at " + atClash.describe();
                    }
                }
                error("can't assume " + name + ", type or alias name is already used", extra, "",
                      expr->at, CompilationError::invalid_assume);
                return;
            }
            if (!expr->assumeType) {
                error("assume without subexpression must have type", "", "",
                      expr->at, CompilationError::invalid_assume);
                return;
            }
        }
    }
    struct CollectAssumeVars : Visitor {
        das_hash_set<string> vars;
        virtual void preVisit(ExprVar * expr) override {
            Visitor::preVisit(expr);
            vars.insert(expr->name);
        }
    };
    ExpressionPtr InferTypes::visit(ExprAssume *expr) {
        if (expr->subexpr) {
            // collect variables referenced in subexpr
            CollectAssumeVars collector;
            expr->subexpr->visit(collector);
            // check for circular dependency: can we reach expr->alias transitively?
            das_hash_map<string,string> cameFrom;   // node -> predecessor in BFS
            vector<string> worklist;
            for (const auto & v : collector.vars) {
                if (cameFrom.find(v) == cameFrom.end()) {
                    worklist.push_back(v);
                    cameFrom[v] = expr->alias;
                }
            }
            bool hasCycle = false;
            while (!worklist.empty()) {
                auto cur = move(worklist.back());
                worklist.pop_back();
                if (cur == expr->alias) { hasCycle = true; break; }
                for (const auto & ae : assume) {
                    if (ae.expr->alias == cur) {
                        for (const auto & v : ae.vars) {
                            if (cameFrom.find(v) == cameFrom.end()) {
                                cameFrom[v] = cur;
                                worklist.push_back(v);
                            }
                        }
                    }
                }
            }
            if (hasCycle) {
                string extra;
                if (verbose) {
                    // reconstruct: walk cameFrom backwards from the node that reached expr->alias
                    // cameFrom[expr->alias] was set during seed, so find the node whose assume deps include expr->alias
                    vector<string> chain;
                    string cur = expr->alias;
                    do {
                        chain.push_back(cur);
                        auto it = cameFrom.find(cur);
                        if (it == cameFrom.end()) break;
                        cur = it->second;
                    } while (cur != expr->alias);
                    chain.push_back(expr->alias);
                    // chain is reversed (e.g. [y, x, y]), reverse to get [y, x, y] -> "y -> x -> y"
                    // actually it's already in reverse BFS order, so reverse it
                    std::reverse(chain.begin(), chain.end());
                    extra = chain[0];
                    for (size_t i = 1; i < chain.size(); i++) {
                        extra += " -> " + chain[i];
                    }
                }
                error("assume '" + expr->alias + "' creates a circular dependency", extra, "",
                      expr->at, CompilationError::invalid_assume);
                return expr;
            }
            assume.push_back(AssumeEntry{expr, move(collector.vars)});
        } else {
            assumeType.emplace_back(expr);
        }
        return expr;
    }
    void InferTypes::preVisitWithBody ( ExprWith * expr, Expression * body) {
        Visitor::preVisitWithBody(expr, body);
        with.push_back(expr);
    }
    ExpressionPtr InferTypes::visit(ExprWith *expr) {
        if (auto wT = expr->with->type) {
            StructurePtr pSt;
            if (wT->dim.size()) {
                error("with array in undefined, " + describeType(wT), "", "",
                      expr->at, CompilationError::invalid_with_type);
            } else if (wT->isStructure()) {
                pSt = wT->structType;
            } else if (wT->isPointer() && wT->firstType && wT->firstType->isStructure()) {
                pSt = wT->firstType->structType;
            } else {
                error("unexpected with type " + describeType(wT), "", "",
                      expr->at, CompilationError::invalid_with_type);
            }
            if (pSt) {
                for (auto fi : pSt->fields) {
                    for (auto &lv : local) {
                        if (lv->name == fi.name) {
                            error("with expression shadows local variable " +
                                      lv->name + " at line " + to_string(lv->at.line),
                                  "", "",
                                  expr->at, CompilationError::variable_not_found);
                        }
                    }
                }
            }
        }
        with.pop_back();
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprWhile *expr) {
        Visitor::preVisit(expr);
        loop.push_back(expr);
        markNoDiscard(expr->cond.get());
    }
    ExpressionPtr InferTypes::visit(ExprWhile *expr) {
        loop.pop_back();
        if (!expr->cond->type)
            return Visitor::visit(expr);
        // infer
        if (!expr->cond->type->isSimpleType(Type::tBool)) {
            error("while loop condition must be boolean", "", "",
                  expr->at, CompilationError::invalid_loop);
        }
        if (expr->cond->type->isRef()) {
            expr->cond = Expression::autoDereference(expr->cond);
        }
        // now, to unwrap the generator
        if (func && func->generator) {
            // only topmost
            //  which in case of generator is 2, due to
            //  def GENERATOR { with LAMBDA { ...collapse here... } }
            if (!blocks.empty() /* || scopes.size()!=2 */) {
                return Visitor::visit(expr);
            }
            uint32_t tf = expr->body->getEvalFlags();
            if (tf & EvalFlags::yield) { // only unwrap if it has "yield"
                auto blk = replaceGeneratorWhile(expr, func);
                scopes.back()->needCollapse = true;
                reportAstChanged();
                return blk;
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprFor *expr) {
        Visitor::preVisit(expr);
        // macro generated invisible variables
        // DAS_ASSERT(expr->visibility.line);
        loop.push_back(expr);
        pushVarStack();
    }
    void InferTypes::preVisitForStack(ExprFor *expr) {
        Visitor::preVisitForStack(expr);
        if (!expr->iterators.size()) {
            error("for loop needs at least one iterator", "", "",
                  expr->at, CompilationError::invalid_loop);
            return;
        } else if (expr->iterators.size() != expr->sources.size()) {
            error("for loop needs as many iterators as there are sources", "", "",
                  expr->at, CompilationError::invalid_loop);
            return;
        }
        // iterator variables
        int idx = 0;
        expr->iteratorVariables.clear();
        for (auto &src : expr->sources) {
            if (!src->type) {
                idx++;
                continue;
            }
            auto pVar = make_smart<Variable>();
            pVar->name = expr->iterators[idx];
            pVar->aka = expr->iteratorsAka[idx];
            pVar->at = expr->iteratorsAt[idx];
            if (src->type->dim.size()) {
                pVar->type = make_smart<TypeDecl>(*src->type);
                pVar->type->ref = true;
                pVar->type->dim.erase(pVar->type->dim.begin());
                if (!pVar->type->dimExpr.empty()) {
                    pVar->type->dimExpr.erase(pVar->type->dimExpr.begin());
                }
            } else if (src->type->isGoodIteratorType()) {
                if (src->type->isConst()) {
                    error("can't iterate over const iterator", "", "",
                          expr->at, CompilationError::invalid_iteration_source);
                }
                pVar->type = make_smart<TypeDecl>(*src->type->firstType);
            } else if (src->type->isGoodArrayType()) {
                pVar->type = make_smart<TypeDecl>(*src->type->firstType);
                pVar->type->ref = true;
            } else if (src->type->isRange()) {
                pVar->type = make_smart<TypeDecl>(src->type->getRangeBaseType());
                pVar->type->ref = false;
                pVar->type->constant = true;
            } else if (src->type->isString()) {
                pVar->type = make_smart<TypeDecl>(Type::tInt);
                pVar->type->ref = false;
                pVar->type->constant = true;
            } else if (src->type->isHandle() && src->type->annotation->isIterable()) {
                pVar->type = make_smart<TypeDecl>(*src->type->annotation->makeIteratorType(src));
            } else {
                error("unsupported iteration type " + src->type->describe() + " for the loop variable " + pVar->name + ", iterating over " + describeType(src->type), "", "",
                      expr->at, CompilationError::invalid_iteration_source);
                return;
            }
            pVar->type->constant |= src->type->isConst();
            pVar->type->temporary |= src->type->isTemp();
            pVar->source = src;
            pVar->can_shadow = expr->canShadow;
            for (auto &al : assume) {
                if (al.expr->alias == pVar->name) {
                    error("can't make loop variable `" + pVar->name + "`, name already taken by alias", "", "",
                          pVar->at, CompilationError::variable_not_found);
                }
            }
            if (!pVar->can_shadow && !program->policies.allow_local_variable_shadowing) {
                if (func) {
                    for (auto &fna : func->arguments) {
                        if (fna->name == pVar->name) {
                            error("for loop iterator variable " + pVar->name + " is shadowed by function argument " + fna->name + ": " + describeType(fna->type) + " at line " + to_string(fna->at.line), "", "",
                                  pVar->at, CompilationError::variable_not_found);
                        }
                    }
                }
                for (auto &blk : blocks) {
                    for (auto &bna : blk->arguments) {
                        if (bna->name == pVar->name) {
                            error("for loop iterator variable " + pVar->name + " is shadowed by block argument " + bna->name + ": " + describeType(bna->type) + " at line " + to_string(bna->at.line), "", "",
                                  pVar->at, CompilationError::variable_not_found);
                        }
                    }
                }
                for (auto &lv : local) {
                    if (lv->name == pVar->name) {
                        error("for loop iterator variable " + pVar->name + " is shadowed by another local variable " + lv->name + ": " + describeType(lv->type) + " at line " + to_string(lv->at.line), "", "",
                              pVar->at, CompilationError::variable_not_found);
                        break;
                    }
                }
                if (auto eW = hasMatchingWith(pVar->name)) {
                    error("for loop iterator variable " + pVar->name + " is shadowed by with expression at line " + to_string(eW->at.line), "", "",
                          pVar->at, CompilationError::variable_not_found);
                }
            }
            local.push_back(pVar);
            expr->iteratorVariables.push_back(pVar);
            if (expr->iteratorsTupleExpansion.size() > idx && expr->iteratorsTupleExpansion[idx]) {
                if (pVar->type && !pVar->type->isTuple()) {
                    error("for loop iterator variable " + pVar->name + " is not a tuple", "", "",
                          expr->at, CompilationError::invalid_iteration_source);
                } else {
                    expandTupleName(pVar->name, pVar->at);
                }
            }
            ++idx;
        }
    }
    void InferTypes::preVisitForSource(ExprFor *expr, Expression *that, bool last) {
        Visitor::preVisitForSource(expr, that, last);
        that->isForLoopSource = true;
        markNoDiscard(that);
    }
    ExpressionPtr InferTypes::visitForSource(ExprFor *expr, Expression *that, bool last) {
        if (jitEnabled() && that->type && ((that->type->isHandle() && that->type->annotation->isIterable()) || (that->type->isString()))) {
            auto fnc = findMatchingFunctions("*", thisModule, "each", {that->type});
            // If there's any `each` for handle type use it, otherwise
            // stay in interpreter.
            if (!fnc.empty()) {
                reportAstChanged();
                auto eachFn = make_smart<ExprCall>(expr->at, "each");
                eachFn->arguments.push_back(that->clone());
                return eachFn;
            }
        }
        // now, for the one where we did not find anything
        if (that->type) {
            if (!that->type->dim.size() &&
                !that->type->isGoodIteratorType() &&
                !that->type->isGoodArrayType() &&
                !that->type->isRange() &&
                !that->type->isString() &&
                !(that->type->isHandle() && that->type->annotation->isIterable())) {
                auto eachCall = make_smart<ExprCall>(that->at, "each");
                eachCall->arguments.push_back(that->clone());
                if (auto mkCall = inferFunctionCall(eachCall.get(), InferCallError::tryOperator)) {
                    if (mkCall->result->isGoodIteratorType()) {
                        reportAstChanged();
                        return eachCall;
                    }
                } else if (eachCall->name != "each") {
                    reportAstChanged();
                    return eachCall;
                }
            }
        }
        if (that->type && that->type->isRef()) {
            return Expression::autoDereference(that);
        }
        return Visitor::visitForSource(expr, that, last);
    }
    ExpressionPtr InferTypes::visit(ExprFor *expr) {
        popVarStack();
        loop.pop_back();
        // forLoop flag
        if (expr->body && expr->body->rtti_isBlock()) {
            static_pointer_cast<ExprBlock>(expr->body)->forLoop = true;
        }
        // now, to unwrap the generator
        if (func && func->generator) {
            // only fully resolved loop
            if (expr->sources.empty()) {
                return Visitor::visit(expr);
            } else if (expr->iteratorVariables.size() != expr->sources.size()) {
                return Visitor::visit(expr);
            }
            // only topmost
            //  which in case of generator is 2, due to
            //  def GENERATOR { with LAMBDA { ...collapse here... } }
            if (!blocks.empty() /* || scopes.size()!=2 */) {
                return Visitor::visit(expr);
            }
            uint32_t tf = expr->body->getEvalFlags();
            if (tf & EvalFlags::yield) { // only unwrap if it has "yield"
                auto blk = replaceGeneratorFor(expr, func);
                scopes.back()->needCollapse = true;
                reportAstChanged();
                return blk;
            }
        }
        // implement for loop macro
        ExpressionPtr substitute;
        auto modMacro = [&](Module *mod) -> bool {
            if (thisModule->isVisibleDirectly(mod) && mod != thisModule) {
                for (const auto &pm : mod->forLoopMacros) {
                    if ((substitute = pm->visit(program, thisModule, expr))) {
                        return false;
                    }
                }
            }
            return true;
        };
        Module::foreach (modMacro);
        if (!substitute)
            program->library.foreach (modMacro, "*");
        if (substitute) {
            reportAstChanged();
            return substitute;
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprLet *expr) {
        Visitor::preVisit(expr);
        DAS_ASSERT(!scopes.empty());
        auto scope = scopes.back();
        expr->visibility.fileInfo = expr->at.fileInfo;
        expr->visibility.column = expr->atInit.last_column;
        expr->visibility.line = expr->atInit.last_line;
        expr->visibility.last_column = scope->at.last_column;
        expr->visibility.last_line = scope->at.last_line;
        // macro generated invisible variable
        // DAS_ASSERTF(expr->visibility.line,"visiblity failed at %s",expr->at.describe().c_str());
        if (expr->inScope && scopes.back()->inTheLoop) {
            error("in scope let is not allowed in the loop",
                  "you can always create scope with 'if true'", "", expr->at, CompilationError::in_scope_in_the_loop);
        }
    }
    void InferTypes::preVisitLet(ExprLet *expr, const VariablePtr &var, bool last) {
        Visitor::preVisitLet(expr, var, last);
        var->single_return_via_move = false;
        var->consumed = false;
        if (var->type && var->type->isExprType()) {
            return;
        }
        if (var->type->isAlias()) {
            auto aT = inferAlias(var->type);
            if (aT) {
                var->type = aT;
                var->type->sanitize();
                reportAstChanged();
            } else {
                error("undefined let type " + describeType(var->type),
                      reportInferAliasErrors(var->type), "", var->at, CompilationError::type_not_found);
            }
        }
        if (var->type->isAuto() && !var->init) {
            error("local variable type can't be inferred, it needs an initializer", "", "",
                  var->at, CompilationError::cant_infer_missing_initializer);
        }
        for (auto &al : assume) {
            if (al.expr->alias == var->name) {
                error("can't make local variable `" + var->name + "`, name already taken by alias", "", "",
                      var->at, CompilationError::variable_not_found);
            }
        }
        if (!var->can_shadow && !program->policies.allow_local_variable_shadowing) {
            if (func) {
                for (auto &fna : func->arguments) {
                    if (fna->name == var->name) {
                        error("local variable " + var->name + " is shadowed by function argument " + fna->name + ": " + describeType(fna->type) + " at line " + to_string(fna->at.line), "", "",
                              var->at, CompilationError::variable_not_found);
                    }
                }
            }
            for (auto &blk : blocks) {
                for (auto &bna : blk->arguments) {
                    if (bna->name == var->name) {
                        error("local variable " + var->name + " is shadowed by block argument " + bna->name + ": " + describeType(bna->type) + " at line " + to_string(bna->at.line), "", "",
                              var->at, CompilationError::variable_not_found);
                    }
                }
            }
            for (auto &lv : local) {
                if (lv->name == var->name) {
                    error("local variable " + var->name + " is shadowed by another local variable " + lv->name + ": " + describeType(lv->type) + " at line " + to_string(lv->at.line), "", "",
                          var->at, CompilationError::variable_not_found);
                    break;
                }
            }
            if (auto eW = hasMatchingWith(var->name)) {
                error("local variable " + var->name + " is shadowed by with expression at line " + to_string(eW->at.line), "", "",
                      var->at, CompilationError::variable_not_found);
            }
        }
        if (!var->init) {
            local.push_back(var);
        }
    }
    VariablePtr InferTypes::visitLet(ExprLet *expr, const VariablePtr &var, bool last) {
        if (var->type && var->type->isExprType()) {
            return Visitor::visitLet(expr, var, last);
        }
        if (isCircularType(var->type)) {
            return Visitor::visitLet(expr, var, last);
        }
        if (var->type->ref && !var->init)
            error("local reference has to be initialized", "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (var->type->ref && var->init && !(var->init->alwaysSafe || isLocalOrGlobal(var->init)) && !safeExpression(expr) && !(var->init->type && var->init->type->temporary)) {
            if (program->policies.local_ref_is_unsafe) {
                error("local reference to non-local expression is unsafe", "", "",
                      var->at, CompilationError::unsafe);
            }
        }
        if (var->type->isVoid())
            error("local variable can't be declared void", "", "",
                  var->at, CompilationError::invalid_variable_type);
        if (!var->type->isLocal() && !var->type->ref && !safeExpression(expr)) {
            if (var->type->isGoodBlockType()) {
                if (!var->init) {
                    error("local block variable needs to be initialized", "", "",
                          var->at, CompilationError::invalid_variable_type);
                }
            } else {
                error("local variable of type " + describeType(var->type) + " requires unsafe", "", "",
                      var->at, CompilationError::invalid_variable_type);
            }
        }
        if (!var->type->ref && var->type->hasClasses() && !safeExpression(expr)) {
            error("local class requires unsafe " + describeType(var->type), "", "",
                  var->at, CompilationError::unsafe);
        }
        if (!var->type->isAutoOrAlias()) {
            if (!var->init && var->type->isLocal()) { // we already report error for non-local
                if (var->type->hasNonTrivialCtor()) {
                    error("local variable of type " + describeType(var->type) + " needs to be initialized", "", "",
                          var->at, CompilationError::invalid_variable_type);
                }
            } else if (var->init && var->init->rtti_isCast()) {
                auto castExpr = static_pointer_cast<ExprCast>(var->init);
                if (castExpr->castType->isAuto()) {
                    reportAstChanged();
                    TypeDecl::clone(castExpr->castType, var->type);
                }
            }
            if (forceInscopePod && !expr->inScope && !var->pod_delete && !var->type->ref) { // no constant for now
                if ((expr->variables.size() == 1)                                           // only one variable
                                                                                            // very restrictive on functions
                    && (func && !func->generated && !func->generator && !func->lambda)
                    // not in the generator block
                    && (blocks.empty() || !blocks.back()->isGeneratorBlock)) {
                    if (isPodDelete(var->type.get())) {
                        var->pod_delete = true;
                        reportAstChanged();
                    }
                }
            }
            if (expr->inScope) {
                if (!var->inScope) {
                    if ( inFinally.back() ) {
                        error("in-scope variable " + var->name + " can't be declared in the finally block", "", "",
                              var->at, CompilationError::invalid_variable_type);
                        return Visitor::visitLet(expr, var, last);
                    }
                    if (var->type->canDelete()) {
                        if (var->type->constant) {
                            error("variable " + var->name + " of type " + describeType(var->type) + " can't be in-scope const",
                                  "const variable with complex finalizer can't be deleted automatically", "",
                                  var->at, CompilationError::invalid_variable_type);
                            return Visitor::visitLet(expr, var, last);
                        }
                        var->inScope = true;
                        auto eVar = make_smart<ExprVar>(var->at, var->name);
                        auto exprDel = make_smart<ExprDelete>(var->at, eVar);
                        if (func && func->generator) {
                            auto finFuncs = getFinalizeFunc(func->arguments[0]->type);
                            if (finFuncs.size() == 1) {
                                auto finBody = finFuncs[0]->body.get();
                                if (finBody->rtti_isBlock()) {
                                    auto finBlk = static_cast<ExprBlock *>(finBody);
                                    finBlk->list.insert(finBlk->list.begin(), exprDel);
                                } else {
                                    error("internal error. generator function " + func->name + " has finalize function which is not a block for type " + describeType(func->arguments[0]->type), "", "",
                                          var->at, CompilationError::invalid_generator_finalizer);
                                    return Visitor::visitLet(expr, var, last);
                                }
                            } else {
                                error("internal error. generator function " + func->name + " has multiple finalize functions for type " + describeType(func->arguments[0]->type), "", "",
                                      var->at, CompilationError::invalid_generator_finalizer);
                                return Visitor::visitLet(expr, var, last);
                            }
                        } else if (scopes.back()->isLambdaBlock) {
                            var->inScope = false;
                            error("internal error. in-scope variable in lambda gets converted as part of the lambda function", "", "",
                                  var->at, CompilationError::invalid_variable_type);
                            return Visitor::visitLet(expr, var, last);
                        } else {
                            scopes.back()->finalList.insert(scopes.back()->finalList.begin(), exprDel);
                        }
                        reportAstChanged();
                    } else {
                        error("can't delete " + describeType(var->type), "", "",
                              var->at, CompilationError::bad_delete);
                    }
                }
            } else {
                if (strictSmartPointers && !var->generated && !safeExpression(expr) && var->type->needInScope()) {
                    error("variable " + var->name + " of type " + describeType(var->type) + " requires var inscope to be safe", "", "",
                          var->at, CompilationError::invalid_variable_type);
                }
            }
            if (noUnsafeUninitializedStructs && !var->init && var->type->unsafeInit()) {
                if (!safeExpression(expr)) {
                    error("Uninitialized variable " + var->name + " is unsafe. Use initializer syntax or [safe_when_uninitialized] when intended.", "", "",
                          expr->at, CompilationError::unsafe);
                }
            }
        }
        if (unsafeTableLookup && var->init && var->type && !var->type->ref) { // we are looking for tab[at] to make it safe
            auto pInit = var->init.get();
            if (pInit->rtti_isR2V()) {
                pInit = static_cast<ExprRef2Value *>(pInit)->subexpr.get();
            }
            if (pInit->rtti_isAt()) {
                auto pAt = static_cast<ExprAt *>(pInit);
                if (pAt->subexpr->type && pAt->subexpr->type->isGoodTableType()) {
                    if (!pAt->alwaysSafe) {
                        pAt->alwaysSafe = true;
                        reportAstChanged();
                    }
                }
            }
        }
        // we are looking into initialization with empty table or array to replace with nada
        if (isEmptyInit(var)) {
            var->init.reset();
            reportAstChanged();
            return Visitor::visitLet(expr, var, last);
        }
        verifyType(var->type);
        return Visitor::visitLet(expr, var, last);
    }
    ExpressionPtr InferTypes::visitLetInit(ExprLet *expr, const VariablePtr &var, Expression *init) {
        local.push_back(var);
        if (!var->init->type) {
            return Visitor::visitLetInit(expr, var, init);
        }
        markNoDiscard(var->init.get());
        if (var->type->isAuto()) {
            auto varT = TypeDecl::inferGenericInitType(var->type, var->init->type);
            if (!varT || varT->isAlias()) {
                error("local variable " + var->name + " initialization type can't be inferred, " + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                      var->at, CompilationError::cant_infer_mismatching_restrictions);
            } else if (varT->isVoid()) {
                error("local variable " + var->name + " initialization can't be void, " + describeType(var->type), "", "",
                      var->at, CompilationError::cant_infer_mismatching_restrictions);
            } else {
                varT->ref = false;
                TypeDecl::applyAutoContracts(varT, var->type);
                if (!relaxedPointerConst) { // var a = Foo? const -> var a : Foo const? = Foo? const
                    if (varT->isPointer() && varT->firstType && !varT->constant && var->init->type->constant) {
                        varT->firstType->constant = true;
                    }
                }
                var->type = varT;
                var->type->sanitize();
                reportAstChanged();
            }
        } else if (!canCopyOrMoveType(var->type, var->init->type, TemporaryMatters::no, var->init.get(),
                                      "local variable " + var->name + " initialization type mismatch", CompilationError::invalid_initialization_type, var->at)) {
        } else if (var->type->ref && !var->init->type->isRef()) {
            error("local variable " + var->name + " initialization type mismatch. reference can't be initialized via value, " + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (var->type->ref && !var->type->isConst() && var->init->type->isConst()) {
            error("local variable " + var->name + " initialization type mismatch. const matters, " + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (!var->type->ref && var->type->isGoodBlockType()) {
            if (!var->init->rtti_isMakeBlock()) {
                error("local variable " + var->name + " can only be initialized with make block expression", "", "",
                      var->at, CompilationError::invalid_initialization_type);
            }
        } else if (!var->type->ref && !var->init->type->canCopy() && !var->init->type->canMove() && var->type->hasNonTrivialCtor() && !var->isCtorInitialized()) {
            error("local variable " + var->name + " can only be initialized with type constructor", "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (!var->type->ref && !var->init->type->canCopy() && !var->init->type->canMove() && var->type->hasNonTrivialCtor() && var->isCtorInitialized() && var->init_via_move) {
            error("local variable " + var->name + " can only be initialized with copy", "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (!var->type->ref && !var->init->type->canCopy() && !var->init->type->canMove() && !var->type->hasNonTrivialCtor()) {
            error("local variable " + var->name + " can't be initialized at all", "", "",
                  var->at, CompilationError::invalid_initialization_type);
        } else if (!var->type->ref && !var->init->type->canCopy() && var->init->type->canMove() && !(var->init_via_move || var->init_via_clone)) {
            error("local variable " + var->name + " can only be move-initialized", "", "use <- for that",
                  var->at, CompilationError::invalid_initialization_type);
            if (canRelaxAssign(var->init.get())) {
                reportAstChanged();
                var->init_via_move = true;
            }
        } else if (var->init_via_move && var->init->type->isConst()) {
            error("local variable " + var->name + " can't init (move) from a constant value. " + describeType(var->init->type), "", "",
                  var->at, CompilationError::cant_move);
        } else if (var->init_via_clone && !var->init->type->canClone()) {
            auto varType = make_smart<TypeDecl>(*var->type);
            varType->ref = true;
            auto fnList = getCloneFunc(varType, var->init->type);
            if (fnList.size() && verifyCloneFunc(fnList, expr->at)) {
                return promoteToCloneToMove(var);
            } else {
                reportCantClone("local variable " + var->name + " of type " + describeType(var->type) + " can't be cloned from " + describeType(var->init->type), var->init->type, var->at);
            }
        } else {
            if (var->init_via_clone) {
                if (var->init->type->isWorkhorseType()) {
                    var->init_via_clone = false;
                    var->init_via_move = false;
                    reportAstChanged();
                } else {
                    return promoteToCloneToMove(var);
                }
            }
        }
        return Visitor::visitLetInit(expr, var, init);
    }
    ExpressionPtr InferTypes::visit(ExprLet *expr) {
        if (func && func->generator) {
            // only topmost
            //  which in case of generator is 2, due to
            //  def GENERATOR { with LAMBDA { ...collapse here... } }
            if (!blocks.empty() || scopes.size() != 2) {
                return Visitor::visit(expr);
            }
            for (auto &var : expr->variables) {
                if (var->type->isAutoOrAlias()) {
                    error("type not ready yet", "", "", var->at);
                    return Visitor::visit(expr);
                }
            }
            auto blk = replaceGeneratorLet(expr, func, scopes.back());
            scopes.back()->needCollapse = true;
            // need to update finalizer
            auto finFuncs = getFinalizeFunc(func->arguments[0]->type);
            if (finFuncs.size() == 1) {
                auto finFunc = finFuncs.back();
                auto stype = func->arguments[0]->type->structType;
                auto lname = stype->name;
                auto newFinalizer = generateStructureFinalizer(stype);
                finFunc->body = newFinalizer->body;
                finFunc->notInferred();
            }
            // ---
            reportAstChanged();
            return blk;
        }
        if (scopes.size()) {
            auto hasEarlyOut = scopes.back()->hasEarlyOut;
            expr->hasEarlyOut = hasEarlyOut;
            for (auto &var : expr->variables) {
                var->early_out = hasEarlyOut;
            }
        }
        if (expr->isTupleExpansion) {
            for (auto &var : expr->variables) {
                if (!var->type->isTuple()) {
                    error("expansion of " + var->name + " should be tuple", "", "",
                          var->at, CompilationError::invalid_type);
                }
                expandTupleName(var->name, var->at);
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprCallMacro *expr) {
        expr->inFunction = func.get();
        canFoldResult = expr->macro->canFoldReturnResult(expr) && canFoldResult;
        expr->macro->preVisit(program, thisModule, expr); // pre-visit is allowed to do nothing and not report errors.
        return Visitor::preVisit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprCallMacro *expr) {
        auto errc = program->errors.size();
        auto substitute = expr->macro->visit(program, thisModule, expr);
        if (substitute) {
            reportAstChanged();
            return substitute;
        }
        if (errc == program->errors.size()) { // this fail safe adds error if macro failed, but did not report any errors
            error("call macro '" + expr->macro->name + "' failed to compile", "possibly missing require", "",
                  expr->at, CompilationError::unsupported_call_macro);
        }
        return Visitor::visit(expr);
    }
    bool InferTypes::canVisitLooksLikeCall(ExprLooksLikeCall *call) {
        if ( callDepth >= program->policies.max_call_depth ) {
            error("call expression depth exceeded maximum allowed (" + to_string(program->policies.max_call_depth) + ")", "", "",
                  call->at, CompilationError::too_many_infer_passes);
            return false;
        }
        return true;
    }
    void InferTypes::preVisit(ExprLooksLikeCall *call) {
        callDepth++;
        Visitor::preVisit(call);
        call->argumentsFailedToInfer = false;
        if (call->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) {
            error("too many arguments in " + call->name + ", max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS), "", "",
                  call->at, CompilationError::too_many_arguments);
        }
    }
    ExpressionPtr InferTypes::visit(ExprLooksLikeCall *call) {
        callDepth--;
        return Visitor::visit(call);
    }
    ExpressionPtr InferTypes::visitLooksLikeCallArg(ExprLooksLikeCall *call, Expression *arg, bool last) {
        if (!arg->type)
            call->argumentsFailedToInfer = true;
        if (arg->type && arg->type->isAliasOrExpr())
            call->argumentsFailedToInfer = true;
        checkEmptyBlock(arg);
        return Visitor::visitLooksLikeCallArg(call, arg, last);
    }
    bool InferTypes::canVisitNamedCall(ExprNamedCall *call) {
        if ( callDepth >= program->policies.max_call_depth ) {
            error("call expression depth exceeded maximum allowed (" + to_string(program->policies.max_call_depth) + ")", "", "",
                  call->at, CompilationError::too_many_infer_passes);
            return false;
        }
        return true;
    }
    void InferTypes::preVisit(ExprNamedCall *call) {
        callDepth++;
        Visitor::preVisit(call);
        call->argumentsFailedToInfer = false;
        if ((call->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) || (call->nonNamedArguments.size() > DAS_MAX_FUNCTION_ARGUMENTS)) {
            error("too many arguments in " + call->name + ", max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS), "", "",
                  call->at, CompilationError::too_many_arguments);
        }
    }
    MakeFieldDeclPtr InferTypes::visitNamedCallArg(ExprNamedCall *call, MakeFieldDecl *arg, bool last) {
        if (!arg->value->type) {
            call->argumentsFailedToInfer = true;
        } else if (arg->value->type && arg->value->type->isAliasOrExpr()) {
            call->argumentsFailedToInfer = true;
        }
        return Visitor::visitNamedCallArg(call, arg, last);
    }
    ExpressionPtr InferTypes::visit(ExprNamedCall *expr) {
        callDepth--;
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }

        vector<TypeDeclPtr> nonNamedTypes;
        if (!inferArguments(nonNamedTypes, expr->nonNamedArguments)) {
            error("can't infer type of non-named argument in call to " + expr->name, "", "",
                  expr->at, CompilationError::invalid_argument_type);
            return Visitor::visit(expr);
        }
        MatchingFunctions functions, generics;
        findMatchingFunctionsAndGenerics(functions, generics, expr->name, nonNamedTypes, expr->arguments, true);
        if (functions.size() > 1) {
            vector<TypeDeclPtr> types;
            types.reserve(expr->arguments.size());
            for (const auto &arg : expr->arguments) {
                types.push_back(arg->value->type);
            }
            applyLSP(types, functions);
        }
        if (generics.size() > 1 || functions.size() > 1) {
            reportExcess(expr, nonNamedTypes, "too many matching functions or generics: ", functions, generics);
        } else if (functions.size() == 0) {
            if (generics.size() == 1) {
                reportAstChanged();
                return demoteCall(expr, generics.back());
            } else {
                if (expr->methodCall) {
                    if ( expr->nonNamedArguments.empty() ) {
                        reportMissing(expr, nonNamedTypes, "no matching functions or generics: ", true);
                        return Visitor::visit(expr);
                    }
                    auto tp = expr->nonNamedArguments[0]->type.get();
                    auto vSelf = expr->nonNamedArguments[0];
                    if (tp->isPointer() && tp->firstType) {
                        tp = tp->firstType.get();
                        vSelf = make_smart<ExprPtr2Ref>(vSelf->at, vSelf);
                        vSelf->type = make_smart<TypeDecl>(*tp);
                    }
                    if (tp->isStructure()) {
                        auto callStruct = tp->structType;
                        vector<TypeDeclPtr> nonNamedArgumentTypes;
                        nonNamedArgumentTypes.push_back(vSelf->type);
                        if (hasMatchingMemberCall(callStruct, expr->name, expr->arguments, nonNamedArgumentTypes, true)) {
                            reportAstChanged();
                            auto pInvoke = makeInvokeMethod(expr->at, callStruct, vSelf.get(), expr->name);
                            auto methodFunc = findMethodFunction(callStruct, expr->name);
                            auto newArguments = demoteCallArguments(expr, methodFunc);
                            for (size_t i = 1, n = newArguments.size(); i != n; ++i) {
                                pInvoke->arguments.push_back(newArguments[i]);
                            }
                            return pInvoke;
                        }
                        string moreError = reportMismatchingMemberCall(callStruct, expr->name, expr->arguments, nonNamedArgumentTypes, true);
                        reportMissing(expr, nonNamedTypes, "no matching functions or generics: ", true, CompilationError::function_not_found, moreError);
                        return Visitor::visit(expr);
                    }
                } else if (func && func->isClassMethod && !func->isStaticClassMethod) { // if its a class method with 'self'
                    auto selfStruct = func->arguments.size() > 0 ? func->arguments[0]->type->structType : nullptr;
                    if (!selfStruct) {
                        reportMissing(expr, nonNamedTypes, "no matching functions or generics: ", true);
                        return Visitor::visit(expr);
                    }
                    vector<TypeDeclPtr> nonNamedArgumentTypes;
                    for (auto &arg : expr->nonNamedArguments) {
                        nonNamedArgumentTypes.push_back(arg->type);
                    }
                    if (hasMatchingMemberCall(selfStruct, expr->name, expr->arguments, nonNamedArgumentTypes, false)) {
                        reportAstChanged();
                        auto self = new ExprVar(expr->at, "self");
                        auto pInvoke = makeInvokeMethod(expr->at, selfStruct, self, expr->name);
                        auto methodFunc = findMethodFunction(selfStruct, expr->name);
                        expr->nonNamedArguments.insert(expr->nonNamedArguments.begin(), self);
                        auto newArguments = demoteCallArguments(expr, methodFunc);
                        for (size_t i = 1, n = newArguments.size(); i != n; ++i) {
                            pInvoke->arguments.push_back(newArguments[i]);
                        }
                        return pInvoke;
                    }
                    string moreError = reportMismatchingMemberCall(selfStruct, expr->name, expr->arguments, nonNamedArgumentTypes, false);
                    reportMissing(expr, nonNamedTypes, "no matching functions or generics: ", true, CompilationError::function_not_found, moreError);
                    return Visitor::visit(expr);
                }
                reportMissing(expr, nonNamedTypes, "no matching functions or generics: ", true);
            }
        } else {
            auto fun = functions.back();
            if (generics.size() == 1) {
                auto gen = generics.back();
                if (fun->fromGeneric != gen) { // TODO: getOrigin??
                    reportExcess(expr, nonNamedTypes, "too many matching functions or generics: ", functions, generics);
                    return Visitor::visit(expr);
                }
            }
            if ( inArgumentInit && func==fun ) {
                error("recursive call to " + func->name + " in argument initializer is not allowed", "", "",expr->at);
                return Visitor::visit(expr);
            }
            reportAstChanged();
            return demoteCall(expr, fun);
        }
        return Visitor::visit(expr);
    }
    bool InferTypes::canVisitCall(ExprCall *call) {
        if ( callDepth >= program->policies.max_call_depth ) {
            error("call expression depth exceeded maximum allowed (" + to_string(program->policies.max_call_depth) + ")", "", "",
                  call->at, CompilationError::too_many_infer_passes);
            return false;
        }
        return true;
    }
    void InferTypes::preVisit(ExprCall *call) {
        callDepth++;
        Visitor::preVisit(call);
        call->argumentsFailedToInfer = false;
        if (call->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) {
            error("too many arguments in " + call->name + ", max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS), "", "",
                  call->at, CompilationError::too_many_arguments);
        }
        if (call->func && call->func->nodiscard && !call->notDiscarded && !safeExpression(call)) {
            error("call to " + call->name + " result is discarded, which is unsafe",
                  "use let _ = " + call->name + "(...)", "",
                  call->at, CompilationError::result_discarded);
        }
        if (func && func->isClassMethod && func->classParent && call->name == "super") {
            if (auto baseClass = func->classParent->parent) {
                call->name = baseClass->name + "`" + baseClass->name;
                call->arguments.insert(call->arguments.begin(), make_smart<ExprVar>(call->at, "self"));
                reportAstChanged();
            } else {
                error("call to super in " + func->name + " is not allowed, no base class for " + func->classParent->name, "", "",
                      call->at, CompilationError::function_not_found);
            }
        }
    }
    void InferTypes::preVisitCallArg(ExprCall *call, Expression *arg, bool last) {
        Visitor::preVisitCallArg(call, arg, last);
        arg->isCallArgument = true;
        markNoDiscard(arg);
        if (forceInscopePod && isConsumeArgumentCall(arg)) {
            consumeDepth++;
        }
    }
    ExpressionPtr InferTypes::visitCallArg(ExprCall *call, Expression *arg, bool last) {
        if (!arg->type) {
            call->argumentsFailedToInfer = true;
        } else if (arg->type && arg->type->isAliasOrExpr()) {
            call->argumentsFailedToInfer = true;
        }
        if (forceInscopePod && isConsumeArgumentCall(arg)) {
            consumeDepth--;
        }
        checkEmptyBlock(arg);
        return Visitor::visitCallArg(call, arg, last);
    }
    ExpressionPtr InferTypes::visit(ExprCall *expr) {
        callDepth--;
        if (expr->argumentsFailedToInfer) {
            if (func)
                func->notInferred();
            return Visitor::visit(expr);
        }
        if (forceInscopePod) {
            bool resolvedBefore = expr->genericFunction && expr->func;
            expr->func = inferFunctionCall(expr, InferCallError::functionOrGeneric, expr->genericFunction ? expr->func : nullptr).get();
            if (expr->func && expr->func->fromGeneric) {
                expr->genericFunction = true;
                if (!resolvedBefore && isConsumeArgumentFunc(expr->func))
                    func->notInferred();
            }
        } else {
            auto b4 = expr->func;
            expr->func = inferFunctionCall(expr, InferCallError::functionOrGeneric, expr->genericFunction ? expr->func : nullptr).get();
            if (expr->func && expr->func->fromGeneric)
                expr->genericFunction = true;
            if (b4 != expr->func) reportAstChanged();
        }
        if (expr->aliasSubstitution) {
            if (expr->arguments.size() != 1) {
                error("casting to bitfield requires one argument", "", "",
                      expr->at, CompilationError::invalid_cast);
                return Visitor::visit(expr);
            }
            auto ecast = make_smart<ExprCast>(expr->at, expr->clone(), expr->aliasSubstitution);
            ecast->reinterpret = true;
            ecast->alwaysSafe = true;
            expr->aliasSubstitution.reset();
            reportAstChanged();
            return ecast;
        }
        if (!expr->func) {
            if (auto aliasT = findAlias(expr->name)) {
                if (aliasT->isTuple()) {
                    reportAstChanged();
                    if (expr->arguments.size()) {
                        auto mkt = make_smart<ExprMakeTuple>(expr->at);
                        mkt->recordType = make_smart<TypeDecl>(*aliasT);
                        for (auto &arg : expr->arguments) {
                            mkt->values.push_back(arg->clone());
                        }
                        return mkt;
                    } else {
                        auto mks = make_smart<ExprMakeStruct>(expr->at);
                        mks->makeType = make_smart<TypeDecl>(*aliasT);
                        return mks;
                    }
                } else if (expr->arguments.empty()) {
                    // this is Blah() - so we promote to default<Blah>
                    reportAstChanged();
                    auto mks = make_smart<ExprMakeStruct>(expr->at);
                    mks->makeType = make_smart<TypeDecl>(*aliasT);
                    mks->useInitializer = true;
                    mks->alwaysUseInitializer = true;
                    return mks;
                } else if (aliasT->isStructure() && aliasT->structType) {
                    // this is Blah(args...) where Blah is a typedef for a struct — promote to StructName(args...)
                    auto structName = aliasT->structType->name;
                    if (structName != expr->name) {
                        reportAstChanged();
                        auto newCall = expr->clone();
                        static_cast<ExprCall *>(newCall.get())->name = structName;
                        return newCall;
                    }
                }
            }
        }
        if (!expr->func) {
            auto var = findMatchingBlockOrLambdaVariable(expr->name); // if this is lambda_var(args...) or such
            if (var && var->type && var->type->dim.size() == 0) {     // we promote to vname(args...) to invoke(vname,args...)
                auto bt = var->type->baseType;
                if (bt == Type::tBlock || bt == Type::tLambda || bt == Type::tFunction) {
                    reportAstChanged();
                    auto varExpr = make_smart<ExprVar>(expr->at, var->name);
                    auto invokeExpr = make_smart<ExprInvoke>(expr->at, expr->name);
                    invokeExpr->arguments.push_back(varExpr);
                    for (auto &arg : expr->arguments) {
                        invokeExpr->arguments.push_back(arg->clone());
                    }
                    return invokeExpr;
                }
            }
        }
        if (func && !expr->func && func->isClassMethod && func->arguments.size() >= 1) {
            auto bt = func->arguments[0]->type;
            if (bt && bt->isStructure()) {
                if (expr->name.find("::") == string::npos) { // we only promote to Struct`call() or self->call() if its not blah::call, _::call, or __::call
                    auto memFn = bt->structType->findField(expr->name);
                    if (memFn && memFn->type) {
                        if (memFn->type->dim.size() == 0 && (memFn->type->baseType == Type::tBlock || memFn->type->baseType == Type::tLambda || memFn->type->baseType == Type::tFunction)) {
                            reportAstChanged();
                            if (memFn->classMethod) {
                                auto self = new ExprVar(expr->at, "self");
                                auto pInvoke = makeInvokeMethod(expr->at, bt->structType, self, expr->name);
                                for (auto &arg : expr->arguments) {
                                    pInvoke->arguments.push_back(arg->clone());
                                }
                                pInvoke->alwaysSafe = expr->alwaysSafe;
                                return pInvoke;
                            } else {
                                auto invokeExpr = make_smart<ExprInvoke>(expr->at, expr->name);
                                auto self = make_smart<ExprVar>(expr->at, "self");
                                auto that = make_smart<ExprField>(expr->at, self, expr->name);
                                invokeExpr->arguments.push_back(that);
                                for (auto &arg : expr->arguments) {
                                    invokeExpr->arguments.push_back(arg->clone());
                                }
                                return invokeExpr;
                            }
                        }
                    } else {
                        auto staticName = bt->structType->name + "`" + expr->name;
                        vector<TypeDeclPtr> types;
                        if (inferArguments(types, expr->arguments)) {
                            auto functions = findMatchingFunctions(staticName, types, true);
                            if (functions.size() == 1) {
                                auto staticFunc = functions.back();
                                if (staticFunc->isStaticClassMethod) {
                                    reportAstChanged();
                                    expr->name = staticName;
                                    return Visitor::visit(expr);
                                }
                            }
                        }
                    }
                }
            }
        }
        /*
        // NOTE: this should not be necessary, since infer function call suppose to report every time
        if ( !expr->func ) {
            error("unknwon function '" + expr->name + "'", "", "",
                expr->at, CompilationError::function_not_found);
        }
        */
        if (expr->func && expr->func->unsafeOperation && !safeExpression(expr)) {
            error("unsafe call '" + expr->name + "' must be inside the 'unsafe' block", "", "",
                  expr->at, CompilationError::unsafe);
        } else if (expr->func && expr->func->unsafeOutsideOfFor && !(expr->isForLoopSource || safeExpression(expr))) {
            error("'" + expr->name + "' is unsafe, when not source of the for-loop; must be inside the 'unsafe' block", "", "",
                  expr->at, CompilationError::unsafe);
        } else if (expr->func && expr->func->unsafeWhenNotCloneArray && !(safeExpression(expr) || isCloneArrayExpression(expr))) {
            error("'" + expr->name + "' is unsafe, when not clone array; must be inside the 'unsafe' block", "", "",
                  expr->at, CompilationError::unsafe);
        } else if (enableInferTimeFolding && expr->func && isConstExprFunc(expr->func)) {
            vector<ExpressionPtr> cargs;
            cargs.reserve(expr->arguments.size());
            bool failed = false;
            for (auto &arg : expr->arguments) {
                if (arg->type->ref) {
                    failed = true;
                    break;
                }
                if (auto carg = getConstExpr(arg.get())) {
                    cargs.push_back(carg);
                } else if (arg->type->baseType == Type::fakeContext || arg->type->baseType == Type::fakeLineInfo) {
                    cargs.push_back(cloneWithType(arg));
                } else {
                    failed = true;
                    break;
                }
            }
            if (!failed) {
                swap(cargs, expr->arguments);
                return evalAndFold(expr);
            }
        }
        if (expr->func) {
            for (const auto &ann : expr->func->annotations) {
                if (ann->annotation->rtti_isFunctionAnnotation()) {
                    auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    string err;
                    auto fexpr = fnAnn->transformCall(expr, err);
                    if (!err.empty()) {
                        error("call annotated by '" + fnAnn->name + "' failed to transform, " + err, "", "",
                              expr->at, CompilationError::annotation_failed);
                    } else if (fexpr) {
                        reportAstChanged();
                        return fexpr;
                    }
                }
            }
        }
        if (expr->func && expr->func->isClassMethod && expr->func->result && expr->func->result->isClass()) {
            if (expr->func->name == expr->func->result->structType->name) {
                if (!safeExpression(expr)) {
                    error("Constructing class on stack is unsafe. Allocate it on the heap via 'new [[...]]' or 'new " + expr->func->name + "()' instead.", "", "",
                          expr->at, CompilationError::unsafe);
                }
            }
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visitStringBuilderElement(ExprStringBuilder *, Expression *expr, bool) {
        auto res = Expression::autoDereference(expr);
        if (expr->type) {
            if (expr->type->isVoid()) {
                error("argument of format string should not be `void`", "", "",
                      expr->at, CompilationError::expecting_return_value);
            } else if (expr->type->isAutoOrAlias()) {
                error("argument of format string can't be `auto` or alias", "", "",
                      expr->at, CompilationError::invalid_type);
            }
        }
        if (expr->constexpression) {
            return evalAndFoldString(res.get());
        } else {
            return res;
        }
    }
    ExpressionPtr InferTypes::visit(ExprStringBuilder *expr) {
        expr->type = make_smart<TypeDecl>(Type::tString);
        return evalAndFoldStringBuilder(expr);
    }

    // try infer, if failed - no macros
    // run macros til any of them does work, then reinfer and restart (i.e. infer after each macro)
    void Program::inferTypes(TextWriter &logs, ModuleGroup &libGroup) {
        newLambdaIndex = 1;
        inferTypesDirty(logs, false);
        bool anyMacrosDidWork = false;
        bool anyMacrosFailedToInfer = false;
        int pass = 0;
        int32_t maxInferPasses = options.getIntOption("max_infer_passes", policies.max_infer_passes);
        if (failed())
            goto failed_to_infer;
        do {
            if (pass++ >= maxInferPasses)
                goto failed_to_infer;
            anyMacrosDidWork = false;
            anyMacrosFailedToInfer = false;
            auto modMacro = [&](Module *mod) -> bool { // we run all macros for each module
                if (thisModule->isVisibleDirectly(mod) && mod != thisModule.get()) {
                    for (const auto &pm : mod->macros) {
                        bool anyWork = pm->apply(this, thisModule.get());
                        if (failed()) { // if macro failed, we report it, and we are done
                            error("macro '" + mod->name + "::" + pm->name + "' failed", "", "", LineInfo());
                            return false;
                        }
                        if (anyWork) { // if macro did anything, we done
                            reportingInferErrors = true;
                            inferTypesDirty(logs, true);
                            reportingInferErrors = false;
                            if (failed()) { // if it failed to infer types after, we report it
                                error("macro '" + mod->name + "::" + pm->name + "' failed to infer", "", "", LineInfo());
                                anyMacrosFailedToInfer = true;
                                return false;
                            }
                            anyMacrosDidWork = true; // if any work been done, we start over
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach (modMacro);
            if (failed())
                break;
            if (anyMacrosDidWork)
                continue;
            if (relocatePotentiallyUninitialized(logs)) {
                anyMacrosDidWork = true;
                reportingInferErrors = true;
                inferTypesDirty(logs, true);
                reportingInferErrors = false;
                if (failed()) {
                    error("internal compiler error: variable relocation infer to fail", "", "", LineInfo());
                }
                continue;
            }
            libGroup.foreach (modMacro, "*");
            if (inScopePodAnalysis(logs)) {
                anyMacrosDidWork = true;
                reportingInferErrors = true;
                inferTypesDirty(logs, true);
                reportingInferErrors = false;
                if (failed()) {
                    error("internal compiler error: pod analysis infer to fail", "", "", LineInfo());
                }
                continue;
            }
        } while (!failed() && anyMacrosDidWork);
    failed_to_infer:;
        if (failed() && !anyMacrosFailedToInfer && !macroException) {
            reportingInferErrors = true;
            inferTypesDirty(logs, true);
            reportingInferErrors = false;
        }
        if (pass >= maxInferPasses) {
            error("type inference exceeded maximum allowed number of passes (" + to_string(maxInferPasses) + ")\n"
                                                                                                             "this is likely due to a macro continuously being applied",
                  "", "",
                  LineInfo(), CompilationError::too_many_infer_passes);
        }
    }

    void Program::inferTypesDirty(TextWriter &logs, bool verbose) {
        int pass = 0;
        int32_t maxInferPasses = options.getIntOption("max_infer_passes", policies.max_infer_passes);
        bool logInferPasses = options.getBoolOption("log_infer_passes", false);
        if (logInferPasses) {
            logs << "INITIAL CODE:\n"
                 << *this;
        }
        for (pass = 0; pass < maxInferPasses; ++pass) {
            if (macroException)
                break;
            failToCompile = false;
            errors.clear();
            InferTypes context(this, &logs);
            context.verbose = verbose || logInferPasses;
            visit(context);
            for (auto efn : context.extraFunctions) {
                addFunction(efn);
            }
            vector<tuple<Function *, uint64_t, uint64_t>> refreshFunctions;
            thisModule->functions.foreach_with_hash([&](auto fn, uint64_t hash) {
                auto mnh = fn->getMangledNameHash();
                if ( hash != mnh ) {
                    refreshFunctions.emplace_back(make_tuple(fn.get(), hash, mnh));
                    fn->lookup.clear();
                    fn->notInferred();
                } });
            for (auto rfn : refreshFunctions) {
                if (!thisModule->functions.refresh_key(get<1>(rfn), get<2>(rfn))) {
                    error("internal compiler error: failed to refresh '" + get<0>(rfn)->getMangledName() + "'", "", "", get<0>(rfn)->at);
                    goto failedIt;
                }
            }
            bool anyMacrosDidWork = false;
            auto modMacro = [&](Module *mod) -> bool {
                if (thisModule->isVisibleDirectly(mod) && mod != thisModule.get()) {
                    for (const auto &pm : mod->inferMacros) {
                        anyMacrosDidWork |= pm->apply(this, thisModule.get());
                    }
                }
                return true;
            };
            Module::foreach (modMacro);
            library.foreach (modMacro, "*");
            inferLint(logs);
            if (logInferPasses) {
                logs << "PASS " << pass << ":\n"
                     << *this;
                sort(errors.begin(), errors.end());
                for (auto &err : errors) {
                    logs << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
            }
            if (anyMacrosDidWork)
                continue;
            if (context.finished())
                break;
        }
    failedIt:;
        if (pass == maxInferPasses) {
            error("type inference exceeded maximum allowed number of passes (" + to_string(maxInferPasses) + ")\n"
                                                                                                             "this is likely due to a loop in the type system",
                  "", "",
                  LineInfo(), CompilationError::too_many_infer_passes);
        }
    }
}
