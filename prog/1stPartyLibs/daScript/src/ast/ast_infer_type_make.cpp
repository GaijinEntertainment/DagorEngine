#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_infer_type.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    void InferTypes::preVisit(ExprMakeGenerator *expr) {
        Visitor::preVisit(expr);
        if (expr->arguments.size()) {
            auto mkBlock = expr->arguments[0];
            if (mkBlock->rtti_isMakeBlock()) {
                auto mb = (ExprMakeBlock *)(mkBlock.get());
                if (mb->block->rtti_isBlock()) {
                    auto blk = (ExprBlock *)(mb->block.get());
                    blk->isGeneratorBlock = true;
                }
            }
        }
    }
    ExpressionPtr InferTypes::visit(ExprMakeGenerator *expr) {
        if (expr->iterType && expr->iterType->isExprType()) {
            return Visitor::visit(expr);
        }
        if (expr->iterType->isAlias()) {
            auto aT = inferAlias(expr->iterType);
            if (aT) {
                expr->iterType = aT;
                reportAstChanged();
            } else {
                error("undefined generator type " + describeType(expr->iterType),
                      reportInferAliasErrors(expr->iterType), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        if (expr->iterType->isAuto()) {
            error("generator of undefined type " + describeType(expr->iterType), "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        } else if (expr->iterType->isVoid()) {
            error("generator can't be void (yet)", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        if (expr->arguments.size() != 1) {
            error("generator can only have one argument", "", "",
                  expr->at, CompilationError::invalid_argument_count);
        } else if (!expr->arguments[0]->rtti_isMakeBlock()) {
            error("expecting generator(closure), got " + string(expr->arguments[0]->__rtti) + " instead", "", "",
                  expr->at, CompilationError::invalid_argument_type);
        } else {
            auto mkBlock = static_pointer_cast<ExprMakeBlock>(expr->arguments[0]);
            auto block = static_pointer_cast<ExprBlock>(mkBlock->block);
            if (auto bT = block->makeBlockType()) {
                if (bT->isAutoOrAlias()) {
                    error("can't infer generator block type", "", "",
                          expr->at, CompilationError::invalid_block);
                } else if (!bT->firstType->isSimpleType(Type::tBool)) {
                    error("generator must return boolean", "", "",
                          expr->at, CompilationError::invalid_argument_type);
                } else if (!bT->argTypes.empty()) {
                    error("generator must have no arguments", "", "",
                          expr->at, CompilationError::invalid_argument_type);
                } else {
                    // TODO: check validity of the generator type
                    CaptureLambda cl(func && func->isClassMethod);
                    // we can only capture in-scope variables
                    // i.e stuff BEFORE the scope
                    for (auto &lv : local)
                        cl.scope.insert(lv);
                    for (auto &bls : blocks) {
                        for (auto &blv : bls->arguments) {
                            cl.scope.insert(blv);
                        }
                    }
                    block->visit(cl);
                    if (!cl.fail) {
                        for (auto ba : block->arguments) {
                            cl.capt.erase(ba);
                        }
                        // add "yield" argument
                        bool makeRef = false;
                        if (!expr->iterType->isVoid()) {
                            auto yva = make_smart<Variable>();
                            if (expr->iterType->ref) {
                                yva->type = make_smart<TypeDecl>(Type::tPointer);
                                yva->type->firstType = make_smart<TypeDecl>(*expr->iterType);
                                yva->type->firstType->ref = false;
                                yva->type->constant = false;
                                yva->type->ref = true;
                                makeRef = true;
                            } else {
                                yva->type = make_smart<TypeDecl>(*expr->iterType);
                                yva->type->constant = false;
                                yva->type->ref = !expr->iterType->isRefType();
                            }
                            yva->name = (makeRef ? "_ryield" : "_yield_") + to_string(block->at.line);
                            yva->at = block->at;
                            yva->generated = true;
                            block->arguments.push_back(yva);
                        }
                        // collapse it from the very top, so that all the macro goo which adds empty block goo collapses
                        block->collapse();
                        // make it all
                        bool isUnsafe = !safeExpression(expr);
                        if (verifyCapture(expr->capture, cl, isUnsafe, expr->at)) {
                            string lname = generateNewLambdaName(block->at);
                            auto ls = generateLambdaStruct(lname, block.get(), cl.capt, expr->capture, true);
                            ls->generator = true;
                            if (program->addStructure(ls)) {
                                auto jitFlags = (func && func->requestJit) ? generator_jit : 0;
                                if (func && func->requestNoJit)
                                    jitFlags |= generator_nojit;
                                auto pFn = generateLambdaFunction(lname, block.get(), ls, cl.capt, expr->capture, generator_needYield | jitFlags, program);
                                if (program->addFunction(pFn)) {
                                    auto pFnFin = generateLambdaFinalizer(lname, block.get(), ls, program);
                                    if (program->addFunction(pFnFin)) {
                                        if (func && func->isClassMethod) {
                                            // lambda, captured in the class is a method of that class - for the purposes of 'private'
                                            pFn->isClassMethod = true;
                                            pFn->classParent = func->classParent;
                                            DAS_ASSERT(pFn->classParent);
                                            pFnFin->isClassMethod = true;
                                            pFnFin->classParent = func->classParent;
                                            DAS_ASSERT(pFnFin->classParent);
                                        }
                                        reportAstChanged();
                                        auto ms = generateLambdaMakeStruct(ls, pFn, pFnFin, cl.capt, expr->capture, expr->at, expr->captureAt, program);
                                        // each ( [[ ]]] )
                                        auto cEach = make_smart<ExprCall>(block->at, makeRef ? "each_ref" : "each");
                                        cEach->generated = true;
                                        cEach->arguments.push_back(ms);
                                        return cEach;
                                    } else {
                                        error("generator finalizer name mismatch", "", "",
                                              expr->at, CompilationError::invalid_block);
                                    }
                                } else {
                                    error("generator function name mismatch", "", "",
                                          expr->at, CompilationError::invalid_block);
                                }
                            } else {
                                error("generator struct name mismatch " + ls->name, "", "",
                                      expr->at, CompilationError::invalid_block);
                            }
                        }
                        // in case of error
                        if (!expr->iterType->isVoid()) {
                            block->arguments.pop_back();
                        }
                    }
                }
            }
        }
        return Visitor::visit(expr);
    }
    bool InferTypes::verifyCapture(const vector<CaptureEntry> &capture, const CaptureLambda &cl, bool isUnsafe, const LineInfo &at) {
        for (auto &cV : cl.capt) {
            CaptureMode mode = CaptureMode::capture_any;
            auto it = find_if(capture.begin(), capture.end(), [&](const auto &entry) { return entry.name == cV->name; });
            if (it != capture.end()) {
                mode = it->mode;
            }
            if (mode == CaptureMode::capture_any) {
                if (cV->capture_as_ref) {
                    // this is ok by default
                } else if (!cV->type->canCopy() && !cV->type->canMove()) {
                    error("can't captured variable " + cV->name, "it can't be copied or moved", "",
                          at, CompilationError::invalid_capture);
                    return false;
                } else if (!cV->type->canCopy() && isUnsafe) {
                    error("implicit capture by move requires unsafe, while capturing " + cV->name, "", "",
                          at, CompilationError::invalid_capture);
                    return false;
                } else if (!cV->type->canCopy() && cV->type->isConst()) {
                    error("can't implicitly capture constant variable " + cV->name + " by move", "", "",
                          at, CompilationError::invalid_capture);
                    return false;
                }
            } else if (mode == CaptureMode::capture_by_reference) {
                if (!cV->capture_as_ref && isUnsafe) {
                    error("capture by reference requires unsafe, while capturing " + cV->name, "", "",
                          at, CompilationError::invalid_capture);
                    return false;
                }
            } else if (mode == CaptureMode::capture_by_move) {
                if (!cV->type->canMove()) {
                    error("can't move captured variable " + cV->name, "", "",
                          at, CompilationError::invalid_capture);
                    return false;
                } else if (cV->type->isConst()) {
                    error("can't capture constant variable " + cV->name + " by move", "", "",
                          at, CompilationError::invalid_capture);
                    return false;
                }
            } else if (mode == CaptureMode::capture_by_copy) {
                if (!cV->type->canCopy()) {
                    error("can't copy captured variable " + cV->name, "", "",
                          at, CompilationError::invalid_capture);
                    return false;
                }
            }
            if (cV->no_capture) {
                error("can't capture variable " + cV->name,
                      cV->name == "self" ? "can't capture `self` in the class initializer" : "it is marked as no_capture",
                      "", at, CompilationError::invalid_capture);
                return false;
            }
        }
        return true;
    }
    ExpressionPtr InferTypes::convertBlockToLambda(ExprMakeBlock *expr) {
        auto block = static_pointer_cast<ExprBlock>(expr->block);
        if (auto bT = block->makeBlockType()) {
            if (bT->isAutoOrAlias()) {
                error("can't infer lambda block type", "", "",
                      expr->at, CompilationError::invalid_block);
            } else {
                CaptureLambda cl(func && func->isClassMethod);
                // we can only capture in-scope variables
                // i.e stuff BEFORE the scope
                for (auto &lv : local)
                    cl.scope.insert(lv);
                for (auto &bls : blocks) {
                    for (auto &blv : bls->arguments) {
                        cl.scope.insert(blv);
                    }
                }
                block->visit(cl);
                if (!cl.fail) {
                    for (auto ba : block->arguments) {
                        cl.capt.erase(ba);
                    }
                    bool isUnsafe = !safeExpression(expr);
                    if (verifyCapture(expr->capture, cl, isUnsafe, expr->at)) {
                        string lname = generateNewLambdaName(block->at);
                        auto ls = generateLambdaStruct(lname, block.get(), cl.capt, expr->capture, false);
                        if (program->addStructure(ls)) {
                            auto jitFlags = (func && func->requestJit) ? generator_jit : 0;
                            if (func && func->requestNoJit)
                                jitFlags |= generator_nojit;
                            auto pFn = generateLambdaFunction(lname, block.get(), ls, cl.capt, expr->capture, jitFlags, program);
                            if (program->addFunction(pFn)) {
                                auto pFnFin = generateLambdaFinalizer(lname, block.get(), ls, program);
                                if (program->addFunction(pFnFin)) {
                                    // lambda, captured in the class is a method of that class - for the purposes of 'private'
                                    if (func && func->isClassMethod) {
                                        pFn->isClassMethod = true;
                                        pFn->classParent = func->classParent;
                                        DAS_ASSERT(pFn->classParent);
                                        pFnFin->isClassMethod = true;
                                        pFnFin->classParent = func->classParent;
                                        DAS_ASSERT(pFnFin->classParent);
                                    }
                                    reportAstChanged();
                                    auto ms = generateLambdaMakeStruct(ls, pFn, pFnFin, cl.capt, expr->capture, expr->at, expr->captureAt, program);
                                    return ms;
                                } else {
                                    error("lambda finalizer name mismatch", "", "",
                                          expr->at, CompilationError::invalid_block);
                                }
                            } else {
                                error("lambda function name mismatch", "", "",
                                      expr->at, CompilationError::invalid_block);
                            }
                        } else {
                            error("lambda struct name mismatch", "", "",
                                  expr->at, CompilationError::invalid_block);
                        }
                    }
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::convertBlockToLocalFunction(ExprMakeBlock *expr) {
        auto block = static_pointer_cast<ExprBlock>(expr->block);
        if (auto bT = block->makeBlockType()) {
            if (bT->isAutoOrAlias()) {
                error("can't infer local function block type", "", "",
                      expr->at, CompilationError::invalid_block);
            } else {
                string lname = generateNewLocalFunctionName(block->at);
                auto pFn = generateLocalFunction(lname, block.get());
                if (func) {
                    if (auto origin = func->getOriginPtr()) {
                        pFn->fromGeneric = getOrCreateDummy(origin->module);
                    }
                }
                if (program->addFunction(pFn)) {
                    reportAstChanged();
                    return make_smart<ExprAddr>(expr->at, "_::" + lname + "`function");
                } else {
                    error("local function name mismatch", "", "",
                          expr->at, CompilationError::invalid_block);
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::visit(ExprMakeBlock *expr) {
        auto block = static_pointer_cast<ExprBlock>(expr->block);
        // can only infer block type, if all argument types are inferred
        for (auto &arg : block->arguments) {
            if (arg->type->isAlias()) {
                error(reportAliasError(arg->type), "", "",
                      arg->at, CompilationError::invalid_argument_type);
                return Visitor::visit(expr);
            }
        }
        expr->type = block->makeBlockType();
        if (expr->isLambda) {
            expr->type->baseType = Type::tLambda;
            if (!expr->type->isAutoOrAlias()) {
                if (auto unInferred = isFullyInferredBlock(block.get())) {
                    // only report block inference error if there is no error inside the block, to avoid reporting multiple errors caused by the same issue
                    if ( ((ExprBlock *)expr->block.get())->insideErrorCount==0 ) {
                        TextWriter tt;
                        if (verbose)
                            tt << unInferred->at.describe() << ": " << unInferred->describe() << " is not fully inferred yet";
                        error("block is not fully inferred yet", tt.str(), "",
                            expr->at, CompilationError::invalid_block);
                    }
                } else {
                    if (auto btl = convertBlockToLambda(expr)) {
                        return btl;
                    }
                }
            }
        } else if (expr->isLocalFunction) {
            expr->type->baseType = Type::tFunction;
            if (!expr->type->isAutoOrAlias()) {
                if (auto unInferred = isFullyInferredBlock(block.get())) {
                    // only report block inference error if there is no error inside the block, to avoid reporting multiple errors caused by the same issue
                    if ( ((ExprBlock *)expr->block.get())->insideErrorCount==0 ) {
                        TextWriter tt;
                        if (verbose)
                            tt << unInferred->at.describe() << ": " << unInferred->describe() << " is not fully inferred yet";
                        error("block is not fully inferred yet", tt.str(), "",
                            expr->at, CompilationError::invalid_block);
                    }
                } else {
                    if (auto btl = convertBlockToLocalFunction(expr)) {
                        return btl;
                    }
                }
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprMakeVariant *expr) {
        Visitor::preVisit(expr);
        if (expr->makeType && expr->makeType->isExprType()) {
            return;
        }
        verifyType(expr->makeType);
        if (expr->makeType->baseType != Type::tVariant) {
            error("[[variant" + describeType(expr->makeType) + "]] with non-variant type", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        if (expr->makeType->dim.size() > 1) {
            error("[[" + describeType(expr->makeType) + "]] variant can only initialize single dimension arrays", "", "",
                  expr->at, CompilationError::invalid_type);
        } else if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] != int32_t(expr->variants.size())) {
            error("[[" + describeType(expr->makeType) + "]] variant dimension mismatch, provided " +
                      to_string(expr->variants.size()) + " elements, expecting " + to_string(expr->makeType->dim[0]),
                  "", "",
                  expr->at, CompilationError::invalid_type);
        } else if (expr->makeType->ref) {
            error("[[" + describeType(expr->makeType) + "]] variant can't be reference", "", "",
                  expr->at, CompilationError::invalid_type);
        }
    }
    MakeFieldDeclPtr InferTypes::visitMakeVariantField(ExprMakeVariant *expr, int index, MakeFieldDecl *decl, bool last) {
        if (!decl->value->type) {
            return Visitor::visitMakeVariantField(expr, index, decl, last);
        }
        auto fieldVariant = expr->makeType->findArgumentIndex(decl->name);
        if (fieldVariant != -1) {
            auto fieldType = expr->makeType->argTypes[fieldVariant];
            if (!canCopyOrMoveType(fieldType, decl->value->type, TemporaryMatters::yes, decl->value.get(),
                                   "can't initialize field " + decl->name, CompilationError::cant_copy, decl->value->at)) {
            } else if (decl->value->type->isTemp(true, false)) {
                error("can't initialize variant field " + decl->name + " with temporary value", "", "",
                      decl->value->at, CompilationError::cant_pass_temporary);
            }
            if (!fieldType->canCopy() && !decl->moveSemantics) {
                error("field " + decl->name + " can't be copied; " + describeType(fieldType), "", "use <- instead",
                      decl->at, CompilationError::invalid_type);
                if (canRelaxAssign(decl->value.get())) {
                    reportAstChanged();
                    decl->moveSemantics = true;
                }
            } else if (decl->moveSemantics && decl->value->type->isConst()) {
                error("can't move from a constant value " + describeType(decl->value->type), "", "",
                      decl->value->at, CompilationError::cant_move);
            }
        } else {
            error("field not found: '" + decl->name + "'", "", "",
                  decl->at, CompilationError::cant_get_field);
        }
        return Visitor::visitMakeVariantField(expr, index, decl, last);
    }
    ExpressionPtr InferTypes::visit(ExprMakeVariant *expr) {
        if (expr->makeType && expr->makeType->isExprType()) {
            return Visitor::visit(expr);
        }
        // result type
        auto resT = make_smart<TypeDecl>(*expr->makeType);
        if ( resT->isAlias() ) {
            auto aT = inferAlias(resT);
            if (aT) {
                resT = aT;
                reportAstChanged();
            } else {
                error("undefined variant type " + describeType(expr->makeType),
                      reportInferAliasErrors(expr->makeType), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        if ( resT->isAuto() ) {
            error("variant of undefined type " + describeType(expr->makeType), "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        } else if ( resT->isVoid() ) {
            error("variant can't be void", "", "",
                  expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        uint32_t resDim = uint32_t(expr->variants.size());
        if (resDim == 0) {
            resT->dim.clear();
        } else if (resDim != 1) {
            resT->dim.resize(1);
            resT->dim[0] = resDim;
        } else {
            resT->dim.clear();
        }
        expr->type = resT;
        verifyType(expr->type);
        return Visitor::visit(expr);
    }
    void InferTypes::describeLocalType(vector<string> &results, TypeDecl *tp, const string &prefix, das_set<Structure *> &dep) const {
        if (tp->baseType == Type::tHandle) {
            if (!tp->annotation->isLocal()) {
                results.push_back(prefix + " : " + tp->describe());
            }
        } else if (tp->baseType == Type::tStructure) {
            if (tp->structType) {
                if (dep.find(tp->structType) != dep.end())
                    return;
                dep.insert(tp->structType);
                for (auto &field : tp->structType->fields) {
                    describeLocalType(results, field.type.get(), prefix + "." + field.name, dep);
                }
            }
        } else if (tp->baseType == Type::tTuple || tp->baseType == Type::tVariant || tp->baseType == Type::option) {
            int argIndex = 0;
            for (const auto &arg : tp->argTypes) {
                if (tp->argNames.size()) {
                    describeLocalType(results, arg.get(), prefix + "." + tp->argNames[argIndex], dep);
                } else {
                    describeLocalType(results, arg.get(), prefix + "." + to_string(argIndex), dep);
                }
                argIndex++;
            }
        } else if (tp->baseType == Type::tArray || tp->baseType == Type::tTable) {
            if (tp->firstType) {
                describeLocalType(results, tp->firstType.get(), prefix + "[]", dep);
            }
            if (tp->secondType) {
                describeLocalType(results, tp->secondType.get(), prefix + "[]", dep);
            }
        }
    }
    string InferTypes::describeLocalType(TypeDecl *tp) const {
        if (verbose) {
            das_set<Structure *> dep;
            vector<string> results;
            describeLocalType(results, tp, "type<" + tp->describe() + ">", dep);
            TextWriter tw;
            for (auto &res : results) {
                tw << "\t" << res << " is not a local type\n";
            }
            return tw.str();
        } else {
            return "";
        }
    }
    bool InferTypes::canVisitMakeStructure ( ExprMakeStruct * expr ) {
        if ( callDepth >= program->policies.max_call_depth ) {
            error("call expression depth exceeded maximum allowed (" + to_string(program->policies.max_call_depth) + ")", "", "",
                  expr->at, CompilationError::too_many_infer_passes);
            return false;
        }
        return true;
    }
    void InferTypes::preVisit(ExprMakeStruct *expr) {
        callDepth ++;
        Visitor::preVisit(expr);
        if (expr->makeType && expr->makeType->isExprType()) {
            return;
        }
        expr->constructor = nullptr;
        verifyType(expr->makeType);
        if (expr->makeType->baseType != Type::tStructure && expr->makeType->baseType != Type::tHandle) {
            if (expr->structs.size()) {
                error("[[" + describeType(expr->makeType) + "]] with non-structure type", "", "",
                      expr->at, CompilationError::invalid_type);
            }
        }
        if (expr->makeType->dim.size() > 1) {
            error("[[" + describeType(expr->makeType) + "]] struct can only initialize single dimension arrays", "", "",
                  expr->at, CompilationError::invalid_type);
        } else if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] != int32_t(expr->structs.size())) {
            error("[[" + describeType(expr->makeType) + "]] struct dimension mismatch, provided " +
                      to_string(expr->structs.size()) + " elements, expecting " + to_string(expr->makeType->dim[0]),
                  "", "",
                  expr->at, CompilationError::invalid_type);
        } else if (expr->makeType->ref) {
            error("[[" + describeType(expr->makeType) + "]] struct can't be reference", "", "",
                  expr->at, CompilationError::invalid_type);
        } else if (!expr->makeType->isLocal() && !expr->isNewHandle) {
            if (expr->makeType->isClass()) {
                error("Class '" + expr->makeType->structType->name + "' has fields, which can't be allocated locally, which is not allowed. "
                                                                     "It contains Handled type, where isLocal() returned false.",
                      describeLocalType(expr->makeType.get()), "",
                      expr->at, CompilationError::invalid_type);
            } else {
                error(describeType(expr->makeType) + "() can`t be allocated locally (on the stack or as part of other data structure), which is not allowed. "
                                                     "It contains Handled type, where isLocal() returned false. "
                                                     "Allocate it on the heap (new [[...]]) or modify your C++ bindings.",
                      describeLocalType(expr->makeType.get()), "",
                      expr->at, CompilationError::invalid_type);
            }
        } else if (expr->makeType->baseType == Type::tHandle && expr->isNewHandle && !expr->useInitializer) {
            error("'new [[" + describeType(expr->makeType) + "]]' struct requires initializer syntax", "",
                  "use 'new [[" + describeType(expr->makeType) + "()]]' instead",
                  expr->at, CompilationError::invalid_type);
        } else if (!expr->isNewClass && expr->makeType->isClass()) {
            if (!safeExpression(expr)) {
                error("Constructing class on stack is unsafe. Allocate it on the heap via new [[...]] or new " + expr->makeType->structType->name + "() instead.", "", "",
                      expr->at, CompilationError::unsafe);
            }
        } else if (noUnsafeUninitializedStructs && !(expr->useInitializer || expr->usedInitializer) && expr->makeType->structType && !expr->makeType->structType->safeWhenUninitialized && !expr->makeType->structType->isLambda && expr->makeType->structType->hasInitFields) {
            if (!safeExpression(expr)) {
                error("Uninitialized structure " + expr->makeType->structType->name + " is unsafe. Use initializer syntax or [safe_when_uninitialized] when intended.", "", "",
                      expr->at, CompilationError::unsafe);
            }
        }
    }
    bool InferTypes::convertCloneSemanticsToExpression(ExprMakeStruct *expr, int index, MakeFieldDecl *decl) {
        if (!expr->block)
            expr->block = makeStructWhereBlock(expr);
        if ( !expr->block->rtti_isMakeBlock() ) {
            error("Expected make block for struct construction, got " + expr->block->describe(), "", "",
                  expr->at, CompilationError::invalid_type);
            return false;
        }
        auto mkb = static_pointer_cast<ExprMakeBlock>(expr->block);
        if (!mkb->block->rtti_isBlock()) {
            error("Expected block for make block, got " + mkb->block->describe(), "", "",
                  expr->at, CompilationError::invalid_type);
            return false;
        }
        auto blk = static_pointer_cast<ExprBlock>(mkb->block);
        bool ignoreCapturedConstant = false;
        if (expr->makeType->baseType == Type::tStructure) {
            if (auto field = expr->makeType->structType->findField(decl->name)) {
                if (field->capturedConstant) {
                    ignoreCapturedConstant = true;
                }
            }
        }
        auto cle = convertToCloneExpr(expr, index, decl, ignoreCapturedConstant);
        blk->list.insert(blk->list.begin(), cle); // TODO: fix order. we are making them backwards now
        reportAstChanged();
        return true;
    }
    MakeFieldDeclPtr InferTypes::visitMakeStructureField(ExprMakeStruct *expr, int index, MakeFieldDecl *decl, bool last) {
        if (!decl->value->type) {
            return Visitor::visitMakeStructureField(expr, index, decl, last);
        }
        if (decl->cloneSemantics) {
            if ( convertCloneSemanticsToExpression(expr, index, decl) ) {
                return nullptr;
            } else {
                return Visitor::visitMakeStructureField(expr, index, decl, last);
            }
        }
        if (expr->makeType->baseType == Type::tStructure) {
            if (auto field = expr->makeType->structType->findField(decl->name)) {
                auto copyFieldType = field->type;
                if (field->capturedConstant) {
                    copyFieldType = make_smart<TypeDecl>(*field->type);
                    copyFieldType->constant = true;
                }
                if (!canCopyOrMoveType(copyFieldType, decl->value->type, TemporaryMatters::yes, decl->value.get(),
                                       "can't initialize field " + decl->name, CompilationError::cant_copy, decl->value->at)) {
                } else if (decl->value->type->isTemp(true, false)) {
                    if (expr->makeType->structType->isLambda) {
                        error("can't capture temporary lambda variable " + decl->name, "", "",
                              decl->value->at, CompilationError::cant_pass_temporary);
                    } else {
                        error("can't initialize structure field " + decl->name + " with temporary value", "", "",
                              decl->value->at, CompilationError::cant_pass_temporary);
                    }
                }
                if (!field->type->canCopy() && !decl->moveSemantics) {
                    error("field " + decl->name + " can't be copied; " + describeType(field->type), "", "use <- instead",
                          decl->at, CompilationError::invalid_type);
                    if (canRelaxAssign(decl->value.get())) {
                        reportAstChanged();
                        decl->moveSemantics = true;
                    }
                } else if (decl->moveSemantics && decl->value->type->isConst()) {
                    error("can't move from a constant value " + describeType(decl->value->type), "", "",
                          decl->value->at, CompilationError::cant_move);
                }
                if (field->privateField && !expr->nativeClassInitializer) {
                    error("field " + decl->name + " is private, can't be initialized", "", "",
                          decl->at, CompilationError::cant_get_field);
                }
                if (!decl->moveSemantics && !field->type->ref) {
                    decl->value = Expression::autoDereference(decl->value);
                }
            } else {
                TextWriter extra;
                vector<TypeDeclPtr> args;
                args.push_back(expr->makeType);
                args.push_back(decl->value->type);
                auto compareName = ".`" + decl->name + "`clone";
                auto opName = "_::" + compareName;
                MatchingFunctions funs, gens;
                findMatchingFunctionsAndGenerics(funs, gens, opName, args);
                bool brokenStrictProperty = false;
                if (funs.size() == 1 || gens.size() == 1) {
                    if (strictProperties) {
                        brokenStrictProperty = true;
                        if (verbose) {
                            extra
                                << "since there is operator ." << decl->name << " := ("
                                << expr->makeType->structType->name << "," << decl->value->type->describe() << ") , try "
                                << decl->name << " := " << *(decl->value);
                        }
                    } else {
                        convertCloneSemanticsToExpression(expr, index, decl);
                        return nullptr;
                    }
                }
                if (!brokenStrictProperty && verbose) {
                    auto can1 = findMissingCandidates(opName, false);
                    auto can2 = findMissingGenericCandidates(opName, false);
                    can1.reserve(can1.size() + can2.size());
                    can1.insert(can1.end(), can2.begin(), can2.end());
                    for (auto &fn : can1) {
                        if (fn->isClassMethod && fn->arguments.size() == 2) {
                            if (fn->name != compareName) {
                                // .`name`clone
                                auto realName = fn->name.substr(2, fn->name.size() - 8);
                                extra << "property name " << realName << " is similar, typo?\n";
                                if (!fn->arguments[1]->type->isSameType(*args[1], RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes)) {
                                    extra << "\t" << describeType(fn->arguments[1]->type) << " can't be initialized with " << decl->value->type->describe() << "\n";
                                }
                            } else if (!fn->arguments[1]->type->isSameType(*args[1], RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes)) {
                                extra << "property " << decl->name << " : " << describeType(fn->arguments[1]->type) << " can't be initialized with " << decl->value->type->describe() << "\n";
                            }
                        }
                    }
                }
                error("field not found, " + decl->name, extra.str(), "",
                      decl->at, CompilationError::cant_get_field);
            }
        } else if (expr->makeType->baseType == Type::tHandle) {
            if (auto fldt = expr->makeType->annotation->makeFieldType(decl->name, false)) {
                if (!fldt->isRef()) {
                    error("field is a property, not a value; " + decl->name, "", "",
                          decl->at, CompilationError::cant_get_field);
                }
                if (!canCopyOrMoveType(fldt, decl->value->type, TemporaryMatters::no, decl->value.get(),
                                       "can't initialize field " + decl->name, CompilationError::cant_copy, decl->value->at)) {
                }
                if (!fldt->canCopy() && !decl->moveSemantics) {
                    error("field " + decl->name + " can't be copied; " + describeType(fldt), "", "use <- instead",
                          decl->at, CompilationError::invalid_type);
                    if (canRelaxAssign(decl->value.get())) {
                        reportAstChanged();
                        decl->moveSemantics = true;
                    }
                } else if (decl->moveSemantics && decl->value->type->isConst()) {
                    error("can't move from a constant value " + describeType(decl->value->type), "", "",
                          decl->value->at, CompilationError::cant_move);
                }
                if (!decl->moveSemantics && !fldt->ref) {
                    decl->value = Expression::autoDereference(decl->value);
                }
            } else {
                error("annotation field not found, " + decl->name, "", "",
                      decl->at, CompilationError::cant_get_field);
            }
        }
        return Visitor::visitMakeStructureField(expr, index, decl, last);
    }
    ExpressionPtr InferTypes::structToTuple(const TypeDeclPtr &makeType, const MakeStructPtr &st, const LineInfo &at) {
        if (makeType->isAutoOrAlias()) { // not fully inferred?
            error("can't infer tuple type " + describeType(makeType), "", "",
                  at, CompilationError::invalid_type);
            return nullptr;
        }
        auto mkt = make_smart<ExprMakeTuple>(at);
        mkt->recordType = make_smart<TypeDecl>(*makeType);
        mkt->values.resize(makeType->argTypes.size());
        for (auto &fld : *st) {
            auto idx = makeType->findArgumentIndex(fld->name);
            if (idx == -1) {
                error("tuple field not found, " + fld->name, "", "",
                      fld->at, CompilationError::cant_get_field);
                return nullptr;
            } else if (mkt->values[idx]) {
                error("tuple field already initialized, " + fld->name, "", "",
                      fld->at, CompilationError::field_already_initialized);
                return nullptr;
            } else {
                mkt->values[idx] = fld->value->clone();
            }
        }
        for (size_t i = 0, s = mkt->values.size(); i != s; ++i) {
            if (!mkt->values[i]) {
                auto mks = make_smart<ExprMakeStruct>(at);
                mks->makeType = make_smart<TypeDecl>(*makeType->argTypes[i]);
                mkt->values[i] = mks;
            }
        }
        return mkt;
    }
    ExpressionPtr InferTypes::visit(ExprMakeStruct *expr) {
        callDepth --;
        if (expr->makeType && expr->makeType->isExprType()) {
            return Visitor::visit(expr);
        }
        if (expr->ignoreVisCheck && !safeExpression(expr)) {
            error("ignoring visibility check on structure initialization requires unsafe", "", "",
                  expr->at, CompilationError::unsafe);
        }
        if (expr->makeType && expr->makeType->isAlias()) {
            if (auto aT = inferAlias(expr->makeType)) {
                expr->makeType = aT;
                reportAstChanged();
            } else {
                error("undefined [[ ]] expression type " + describeType(expr->makeType),
                      reportInferAliasErrors(expr->makeType), "", expr->makeType->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        auto isClassCtor = !expr->nativeClassInitializer &&
                           (expr->useInitializer || expr->usedInitializer) &&
                           expr->makeType && (expr->makeType->isClass() || (expr->alwaysUseInitializer && expr->makeType->isStructure() && !expr->makeType->structType->noGenCtor));
        if (isClassCtor) {
            auto st = expr->makeType->structType;
            // auto ctorName = st->module->name  + "::" + st->name;
            auto ctorName = st->module->name + "::" + st->name;
            auto tempCall = make_smart<ExprLooksLikeCall>(expr->at, ctorName);
            expr->constructor = inferFunctionCall(tempCall.get(), InferCallError::functionOrGeneric, nullptr, false, !expr->ignoreVisCheck).get();
            if (!expr->constructor) {
                tempCall->name = "_::" + st->name;
                expr->constructor = inferFunctionCall(tempCall.get(), InferCallError::functionOrGeneric, nullptr, false, !expr->ignoreVisCheck).get();
            }
            if (!expr->constructor) {
                if (!expr->makeType->structType->hasAnyInitializers()) {
                    expr->alwaysUseInitializer = false;
                    reportAstChanged();
                }
                error("constructor can't be inferred " + describeType(expr->makeType),
                      reportInferAliasErrors(expr->makeType), "", expr->makeType->at, CompilationError::function_not_found);
            } else if (expr->constructor->arguments.size() && expr->structs.empty()) {
                // this one with default arguments, we demote back to call
                reportAstChanged();
                auto callName = expr->constructor->module->name + "::" + expr->constructor->name;
                auto callMks = make_smart<ExprCall>(expr->at, callName);
                return callMks;
            }
        }

        if (expr->block) {
            if (!expr->block->rtti_isMakeBlock()) {
                string btype = expr->block->type ? describeType(expr->block->type) : "unknown";
                error("can only pipe block into structure declaration. expecting <| $ ( var decl ), got " + btype,
                      "", "", expr->block->at, CompilationError::invalid_block);
                return Visitor::visit(expr);
            }
            auto mkb = static_pointer_cast<ExprMakeBlock>(expr->block);
            DAS_ASSERT(mkb->block->rtti_isBlock());
            auto blk = static_pointer_cast<ExprBlock>(mkb->block);
            if (blk->arguments.size() != 1) {
                error("where closure should only have one argument", "", "",
                      expr->block->at, CompilationError::invalid_block);
            } else {
                auto arg = blk->arguments[0];
                if (arg->type) {
                    int32_t rec = 0;
                    if (expr->structs.size() > 1)
                        rec = int32_t(expr->structs.size());
                    auto passT = make_smart<TypeDecl>(*expr->makeType);
                    passT->dim.clear();
                    if (rec)
                        passT->dim.push_back(rec);
                    if (arg->type->isAuto()) {
                        auto nargT = TypeDecl::inferGenericType(passT, arg->type, false, false, nullptr);
                        if (nargT) {
                            TypeDecl::applyAutoContracts(nargT, arg->type);
                            arg->type = nargT;
                        } else {
                            error("can't infer where closure block argument", "", "",
                                  arg->at, CompilationError::invalid_block);
                        }
                    }
                    if (!arg->type->isSameType(*passT, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                        error("where closure block argument type mismatch, " +
                                  describeType(arg->type) + " vs " + describeType(expr->makeType),
                              "", "",
                              arg->at, CompilationError::invalid_block);
                    } else if (arg->type->constant) {
                        error("where closure block argument can't be constant, " +
                                  describeType(arg->type) + " vs " + describeType(expr->makeType),
                              "", "",
                              arg->at, CompilationError::invalid_block);
                    }
                }
            }
        }
        // promote to make variant
        if (expr->makeType->baseType == Type::tVariant) {
            if (expr->forceClass) {
                error(expr->makeType->describe() + " is not a class, but a variant", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            } else if (expr->forceStruct) {
                error(expr->makeType->describe() + " is not a struct, but a variant", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            } else if (expr->forceTuple) {
                error(expr->makeType->describe() + " is not a tuple, but a variant", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
            if (expr->block) {
                error("[[variant]] can't have where closure", "", "",
                      expr->block->at, CompilationError::invalid_block);
                return Visitor::visit(expr);
            }
            auto mkv = make_smart<ExprMakeVariant>(expr->at);
            mkv->makeType = make_smart<TypeDecl>(*expr->makeType);
            auto allGood = true;
            for (auto &st : expr->structs) {
                if (st->size() != 1) {
                    error("variant only supports one initializer", "", "",
                          st->front()->at, CompilationError::field_already_initialized);
                    allGood = false;
                } else {
                    mkv->variants.push_back(st->front()->clone());
                }
            }
            if (allGood) {
                reportAstChanged();
                return mkv;
            }
        }
        // promote to make tuple
        if (expr->makeType->baseType == Type::tTuple && expr->structs.size()) {
            if (expr->forceClass) {
                error(expr->makeType->describe() + " is not a class, but a tuple", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            } else if (expr->forceStruct) {
                error(expr->makeType->describe() + " is not a struct, but a tuple", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            } else if (expr->forceVariant) {
                error(expr->makeType->describe() + " is not a variant, but a tuple", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
            if (expr->block) {
                error("[[tuple]] can't have where closure", "", "",
                      expr->block->at, CompilationError::invalid_block);
                return Visitor::visit(expr);
            }
            if (expr->structs.size() == 1) {
                auto mkt = structToTuple(expr->makeType, expr->structs.front(), expr->at);
                if (mkt) {
                    reportAstChanged();
                    return mkt;
                }
            } else {
                auto mka = make_smart<ExprMakeArray>(expr->at);
                mka->makeType = make_smart<TypeDecl>(*expr->makeType);
                mka->values.resize(expr->structs.size());
                for (size_t i = 0; i != expr->structs.size(); ++i) {
                    mka->values[i] = structToTuple(expr->makeType, expr->structs[i], expr->at);
                    if (!mka->values[i]) {
                        return Visitor::visit(expr);
                    }
                }
                reportAstChanged();
                return mka;
            }
        }

        // see if there are any duplicate fields
        if (expr->makeType->baseType == Type::tStructure) {
            if (expr->makeType->structType->isTemplate) {
                string extraError;
                if (func) {
                    extraError = "while compiling function " + func->describe();
                }
                error("can't initialize template structure " + expr->makeType->structType->name, extraError, "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
            bool anyDuplicates = false;
            for (auto &st : expr->structs) {
                das_set<string> fld;
                for (auto &fi : *st) {
                    if (fld.find(fi->name) != fld.end()) {
                        error("field " + fi->name + " is already initialized", "", "",
                              fi->at, CompilationError::field_already_initialized);
                        anyDuplicates = true;
                    } else {
                        fld.insert(fi->name);
                    }
                }
            }
            if (anyDuplicates)
                return Visitor::visit(expr);
            // see if we need to fill in missing fields
            if (expr->useInitializer && expr->makeType->structType) {
                for (auto &stf : expr->makeType->structType->fields) {
                    if (stf.init) {
                        if (!stf.init->type || stf.init->type->isAuto()) {
                            error("structure '" + expr->makeType->structType->name + "' is not fully resolved yet", "", "", expr->at);
                            return Visitor::visit(expr);
                        }
                    }
                }
                if (!expr->structs.size()) {
                    reportAstChanged();
                    expr->structs.emplace_back(make_smart<MakeStruct>());
                }
                if (!isClassCtor) {
                    for (auto &st : expr->structs) {
                        for (auto &fi : expr->makeType->structType->fields) {
                            if (fi.init) {
                                auto it = find_if(st->begin(), st->end(), [&](const MakeFieldDeclPtr &fd) { return fd->name == fi.name; });
                                if (it == st->end()) {
                                    auto msf = make_smart<MakeFieldDecl>(fi.at, fi.name, fi.init->clone(), !fi.init->type->canCopy(), false);
                                    st->push_back(msf);
                                    reportAstChanged();
                                }
                            }
                        }
                    }
                }
                expr->useInitializer = false;
                expr->usedInitializer = true;
            }
            // see if we need to init fields
            if (expr->makeType->structType) {
                expr->initAllFields = !expr->structs.empty();
                for (auto &st : expr->structs) {
                    if (st->size() == expr->makeType->structType->fields.size()) {
                        for (auto &va : *st) {
                            if (va->value->rtti_isMakeLocal()) {
                                auto mkl = static_pointer_cast<ExprMakeLocal>(va->value);
                                expr->initAllFields &= mkl->initAllFields;
                            }
                        }
                    } else {
                        expr->initAllFields = false;
                        break;
                    }
                }
            } else {
                expr->initAllFields = false;
            }
        } else {
            if (expr->makeType->baseType == Type::tTuple && expr->structs.size() == 0) {
                expr->initAllFields = true;
            }
        }
        // drop the ref
        if (expr->makeType->ref) {
            expr->makeType->ref = false;
            reportAstChanged();
        }
        // if unresolved - but we still return.  cause sometimes we pass [], and then we want it resolved
        bool isAutoOrAlias = expr->makeType->isAutoOrAlias();
        // result type
        auto resT = make_smart<TypeDecl>(*expr->makeType);
        uint32_t resDim = uint32_t(expr->structs.size());
        if (resDim == 0) {
            // resT->dim.clear();
        } else if (resDim != 1) {
            resT->dim.resize(1);
            resT->dim[0] = resDim;
        } else {
            if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] == 1) {
                // do nothing
            } else if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] == TypeDecl::dimAuto) {
                resT->dim.resize(1);
                resT->dim[0] = 1;
                expr->makeType->dim[0] = 1;
                reportAstChanged();
            } else {
                resT->dim.clear();
            }
        }
        expr->type = resT;
        if (expr->type->isString()) {
            reportAstChanged();
            auto ecs = make_smart<ExprConstString>(expr->at);
            ecs->type = make_smart<TypeDecl>(Type::tString);
            return ecs;
        } else if (expr->type->isEnumT()) {
            auto f0 = expr->type->enumType->find(0, "");
            if (!f0.empty()) {
                reportAstChanged();
                auto et = make_smart<TypeDecl>(*expr->type);
                et->ref = false;
                auto ens = make_smart<ExprConstEnumeration>(expr->at, f0, et);
                ens->type = make_smart<TypeDecl>(*et);
                ens->type->constant = true;
                return ens;
            } else {
                error("enumeration " + describeType(expr->type) + " is missing 0 value", "", "",
                      expr->at, CompilationError::invalid_type);
            }
        } else if (expr->type->isPointer()) {
            if ( !isAutoOrAlias ) {
                expr->type->ref = false;
                reportAstChanged();
                auto ews = make_smart<ExprConstPtr>(expr->at);
                ews->type = make_smart<TypeDecl>(*expr->type);
                ews->isSmartPtr = expr->type->smartPtr;
                if (expr->type->firstType) {
                    ews->ptrType = make_smart<TypeDecl>(*expr->type->firstType);
                }
                return ews;
            }
        } else if (expr->type->isWorkhorseType()) {
            expr->type->ref = false;
            reportAstChanged();
            auto ews = Program::makeConst(expr->at, expr->type, v_zero());
            ews->type = make_smart<TypeDecl>(*expr->type);
            return ews;
        } else if (!expr->type->isRefType()) {
            expr->type->ref = true;
        }
        if (isAutoOrAlias) {
            error("undefined structure type " + describeType(expr->type), "", "",
                  expr->at, CompilationError::invalid_type);
            return Visitor::visit(expr);
        } else if (expr->type->isClass() && !expr->usedInitializer && !safeExpression(expr)) {
            error("skipping initializer for class initialization requires unsafe", "", "",
                  expr->at, CompilationError::unsafe);
        }
        if (expr->forceClass && !(expr->makeType->baseType == Type::tStructure && expr->makeType->structType && expr->makeType->structType->isClass)) {
            error(expr->type->describe() + " is not a class", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        if (expr->forceStruct && !(expr->makeType->baseType == Type::tStructure && expr->makeType->structType && !expr->makeType->structType->isClass)) {
            error(expr->type->describe() + " is not a struct", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        if (expr->forceVariant && !(expr->makeType->baseType == Type::tVariant)) {
            error(expr->type->describe() + " is not a variant", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        if (expr->forceTuple && !(expr->makeType->baseType == Type::tTuple)) {
            error(expr->type->describe() + " is not a tuple", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        verifyType(expr->type);
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprMakeTuple *expr) {
        Visitor::preVisit(expr);
        expr->makeType.reset();
        expr->initAllFields = true;
    }
    ExpressionPtr InferTypes::visitMakeTupleIndex(ExprMakeTuple *expr, int index, Expression *init, bool lastField) {
        if (!init->type) {
            return Visitor::visitMakeTupleIndex(expr, index, init, lastField);
        }
        if (expr->recordType && expr->recordType->baseType == Type::tTuple) {
            if (expr->recordType->argTypes.size() <= index) {
                error("tuple element _" + to_string(index) + " out of element range", "", "",
                      init->at, CompilationError::invalid_type);
                return Visitor::visitMakeTupleIndex(expr, index, init, lastField);
            }
            if (!canCopyOrMoveType(expr->recordType->argTypes[index], init->type, TemporaryMatters::no, init,
                                   "can't initialize tuple element " + to_string(index), CompilationError::cant_copy, init->at)) {
            }
        }
        if (!init->type->canCopy() && init->type->canMove() && init->type->isConst()) {
            error("can't move from a constant value " + describeType(init->type), "", "",
                  init->at, CompilationError::cant_move);
        }
        if (init->rtti_isMakeLocal()) {
            auto initl = static_cast<ExprMakeLocal *>(init);
            expr->initAllFields &= initl->initAllFields;
        }

        return Expression::autoDereference(Visitor::visitMakeTupleIndex(expr, index, init, lastField));
    }
    ExpressionPtr InferTypes::visit(ExprMakeTuple *expr) {
        for (auto &val : expr->values) {
            if (!val->type || val->type->isAutoOrAlias()) {
                error("not fully defined tuple element type", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
        }
        if (expr->recordType) {
            if (!expr->recordType->isTuple()) {
                error("internal error. ExprMakeTuple with non-tuple record type", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
            size_t argCount = expr->values.size();
            if (expr->recordType->argTypes.size() != argCount) {
                error("declaring " + to_string(argCount) + " arguments in " + describeType(expr->recordType),
                    "but it only has " + to_string(expr->recordType->argTypes.size()) + " elements", "",
                        expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
            auto mkt = make_smart<TypeDecl>(Type::tTuple);
            for (size_t ai = 0; ai != argCount; ++ai) {
                const auto &val = expr->values[ai];
                const auto &argT = expr->recordType->argTypes[ai];
                if (!argT->isSameType(*val->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                    error("invalid argument _" + to_string(ai) + ", expecting " +
                              describeType(argT) + ", passing " + describeType(val->type),
                          "", "",
                          expr->at, CompilationError::invalid_type);
                }
                auto valT = make_smart<TypeDecl>(*argT);
                valT->ref = false;
                valT->constant = false;
                mkt->argTypes.push_back(valT);
                if (expr->recordType->argNames.size() > ai) {
                    mkt->argNames.push_back(expr->recordType->argNames[ai]);
                }
            }
            expr->makeType = mkt;
        } else {
            auto mkt = make_smart<TypeDecl>(Type::tTuple);
            mkt->at = expr->at;
            for (auto &val : expr->values) {
                auto valT = make_smart<TypeDecl>(*val->type);
                if (valT->isVoid()) {
                    error("tuple element type can't be void", "", "",
                          val->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                valT->ref = false;
                valT->constant = false;
                mkt->argTypes.push_back(valT);
            }
            if (expr->recordNames.size()) {
                if (expr->recordNames.size() != expr->values.size()) {
                    error("tuple field names mismatch", "", "",
                          expr->at, CompilationError::invalid_type);
                } else {
                    for (size_t ri = 0, rsize = expr->recordNames.size(); ri != rsize; ++ri) {
                        mkt->argNames.push_back(expr->recordNames[ri]);
                    }
                }
            }
            expr->makeType = mkt;
        }
        TypeDecl::clone(expr->type, expr->makeType);
        verifyType(expr->type);
        if (expr->isKeyValue) {
            auto keyType = expr->makeType->argTypes[0];
            if (keyType->ref) {
                error("a => b tuple key can't be declared as a reference", "", "",
                      keyType->at, CompilationError::invalid_table_type);
            }
            if (!keyType->isWorkhorseType()) {
                error("a => b tuple key has to be declare as a basic 'hashable' type", "", "",
                      keyType->at, CompilationError::invalid_table_type);
            }
        }
        for (auto &argType : expr->makeType->argTypes) {
            if (!argType->canCopy() && !argType->canMove()) {
                error("tuple element has to be copyable or moveable", "", "",
                      expr->at, CompilationError::invalid_type);
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprMakeArray *expr) {
        Visitor::preVisit(expr);
        if (expr->makeType && expr->makeType->isExprType()) {
            return;
        }
        if (expr->makeType && expr->makeType->isAuto()) {
            return;
        }
        verifyType(expr->makeType);
        if (expr->gen2) {
            if (expr->makeType->ref) {
                error("fixed_array<" + describeType(expr->makeType) + "> array type can't be reference", "", "",
                      expr->at, CompilationError::invalid_type);
            }
            TypeDecl::clone(expr->recordType, expr->makeType);
        } else {
            if (expr->makeType->dim.size() > 1) {
                error("[[" + describeType(expr->makeType) + "]] array can only initialize single dimension arrays", "", "",
                      expr->at, CompilationError::invalid_type);
            } else if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] != int32_t(expr->values.size())) {
                error("[[" + describeType(expr->makeType) + "]] array dimension mismatch, provided " +
                          to_string(expr->values.size()) + " elements, expecting " + to_string(expr->makeType->dim[0]),
                      "", "",
                      expr->at, CompilationError::invalid_type);
            } else if (expr->makeType->ref) {
                error("[[" + describeType(expr->makeType) + "]] array can't be reference", "", "",
                      expr->at, CompilationError::invalid_type);
            }
            TypeDecl::clone(expr->recordType, expr->makeType);
            expr->recordType->dim.clear();
        }
        expr->initAllFields = true;
    }
    ExpressionPtr InferTypes::visitMakeArrayIndex(ExprMakeArray *expr, int index, Expression *init, bool last) {
        if (expr->makeType && expr->makeType->isExprType()) {
            return Visitor::visitMakeArrayIndex(expr, index, init, last);
        }
        if (!expr->recordType) {
            if (index == 0) {
                if (init->type && !init->type->isAutoOrAlias()) {
                    // blah[] vs blah
                    TypeDeclPtr mkt;
                    if (!expr->gen2 && expr->makeType->dim.size() && !init->type->dim.size()) {
                        if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] == TypeDecl::dimAuto) {
                            auto infT = make_smart<TypeDecl>(*expr->makeType);
                            infT->dim.clear();
                            mkt = TypeDecl::inferGenericType(infT, init->type, false, false, nullptr);
                            if (mkt) {
                                mkt->dim.resize(1);
                                mkt->dim[0] = int32_t(expr->values.size());
                            }
                        }
                    } else {
                        mkt = TypeDecl::inferGenericType(expr->makeType, init->type, false, false, nullptr);
                    }
                    if (!mkt) {
                        error("array type can't be inferred, " + describeType(expr->makeType) + " = " + describeType(init->type), "", "",
                              init->at, CompilationError::invalid_array_type);
                    } else {
                        mkt->ref = false;
                        mkt->constant = false;
                        TypeDecl::applyAutoContracts(mkt, init->type);
                        if (mkt->isVoid()) {
                            error("array element type can't be void", "", "",
                                  init->at, CompilationError::invalid_array_type);
                            return Visitor::visitMakeArrayIndex(expr, index, init, last);
                        }
                        expr->makeType = mkt;
                        reportAstChanged();
                        return Visitor::visitMakeArrayIndex(expr, index, init, last);
                    }
                } else {
                    error("can't infer array auto type, first element type is undefined", "", "",
                          init->at, CompilationError::invalid_array_type);
                }
            }
        }
        if (!init->type || !expr->recordType) {
            return Visitor::visitMakeArrayIndex(expr, index, init, last);
        }
        if (!canCopyOrMoveType(expr->recordType, init->type, TemporaryMatters::no, init,
                               "can't initialize array element", CompilationError::cant_copy, init->at)) {
            if (expr->recordType->isVariant()) {
                int uidx = expr->recordType->getVariantUniqueFieldIndex(init->type);
                if (uidx == -1) {
                    vector<pair<string, TypeDeclPtr>> options;
                    for (size_t vi = 0, vis = expr->recordType->argTypes.size(); vi != vis; ++vi) {
                        const auto &argT = expr->recordType->argTypes[vi];
                        if (argT->isSameType(*init->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::no)) {
                            options.emplace_back(expr->recordType->argNames[vi], argT);
                        }
                    }
                    if (options.size() == 0) {
                        error("can't recognize unique variant '" + describeType(init->type) + "' in '" + describeType(expr->recordType) + "'", "", "",
                              init->at, CompilationError::invalid_type);
                    } else {
                        if (verbose) {
                            TextWriter tw;
                            for (auto &opt : options) {
                                tw << "\t\t" << opt.first << ":" << describeType(opt.second);
                                if (opt != options.back())
                                    tw << "\n";
                            }
                            error("can't recognize unique variant '" + describeType(init->type) + "' in '" + describeType(expr->recordType) + "'",
                                  "\tcandidates are:\n" + tw.str(), "",
                                  init->at, CompilationError::invalid_type);
                        } else {
                            error("can't recognize unique variant", "", "",
                                  init->at, CompilationError::invalid_type);
                        }
                    }
                } else {
                    auto mkv = make_smart<ExprMakeVariant>(expr->at);
                    mkv->variants.push_back(make_smart<MakeFieldDecl>(
                        init->at,
                        expr->recordType->argNames[uidx],
                        init->clone(),
                        false, // move
                        false  // clone
                        ));
                    mkv->makeType = make_smart<TypeDecl>(*expr->recordType);
                    reportAstChanged();
                    return mkv;
                }
            } else {
                // nada, we already reported error
            }
        } else if (!expr->recordType->canCopy() && expr->recordType->canMove() && init->type->isConst()) {
            error("can't move from a constant value\n\t" + describeType(init->type), "", "",
                  init->at, CompilationError::cant_move);
        } else if (init->type->isTemp(true, false)) {
            error("can't initialize array element with temporary value", "", "",
                  init->at, CompilationError::cant_pass_temporary);
        }
        if (init->rtti_isMakeLocal()) {
            auto initl = static_cast<ExprMakeLocal *>(init);
            expr->initAllFields &= initl->initAllFields;
        }
        if (init->type && !init->type->isSmartPointer()) // in reality only need for foldable
            return Expression::autoDereference(Visitor::visitMakeArrayIndex(expr, index, init, last));
        else
            return Visitor::visitMakeArrayIndex(expr, index, init, last);
    }
    ExpressionPtr InferTypes::visit(ExprMakeArray *expr) {
        if (expr->makeType && expr->makeType->isExprType()) {
            return Visitor::visit(expr);
        }
        if (!expr->recordType) {
            return Visitor::visit(expr);
        }
        if (expr->recordType->isVariant()) {
            bool canPromoteToMakeVariant = true;
            for (auto &eval : expr->values) {
                if (!eval->rtti_isMakeVariant()) {
                    canPromoteToMakeVariant = false;
                    break;
                } else {
                    auto emkv = static_pointer_cast<ExprMakeVariant>(eval);
                    if (emkv->variants.size() != 1) {
                        canPromoteToMakeVariant = false;
                        break;
                    }
                }
            }
            if (canPromoteToMakeVariant) {
                auto mkv = make_smart<ExprMakeVariant>(expr->at);
                for (const auto &eval : expr->values) {
                    auto emkv = static_pointer_cast<ExprMakeVariant>(eval);
                    DAS_ASSERT(emkv->variants.size() == 1);
                    const auto &fmkv = emkv->variants[0];
                    mkv->variants.push_back(make_smart<MakeFieldDecl>(
                        fmkv->at,
                        fmkv->name,
                        fmkv->value->clone(),
                        bool(fmkv->moveSemantics),
                        bool(fmkv->cloneSemantics)));
                }
                mkv->makeType = make_smart<TypeDecl>(*expr->makeType);
                reportAstChanged();
                return mkv;
            }
        }
        if (!expr->recordType->canCopy() && !expr->recordType->canMove()) {
            error("array element has to be copyable or moveable", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        auto resT = make_smart<TypeDecl>(*expr->makeType);
        uint32_t resDim = uint32_t(expr->values.size());
        if (expr->gen2) {
            resT->dim.push_back(resDim);
        } else if (resDim != 1 || expr->makeType->dim.size()) {
            resT->dim.resize(1);
            resT->dim[0] = resDim;
        } else {
            DAS_ASSERT(expr->values.size() == 1);
            auto eval = expr->values[0];
            if (!eval->type) {
                error("unknown value type", "", "",
                      expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            } else if (!expr->recordType->isSameType(*(eval->type), RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::yes)) {
                error("incompatible value type. expecting " + describeType(expr->recordType) + " vs " + describeType(eval->type), "", "",
                      eval->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            } else {
                reportAstChanged();
                auto resExpr = expr->values[0];
                if (resExpr->rtti_isMakeTuple()) {
                    auto mkt = static_pointer_cast<ExprMakeTuple>(resExpr);
                    mkt->recordType = make_smart<TypeDecl>(*expr->recordType);
                    mkt->makeType.reset();
                }
                if (!expr->recordType->isSameType(*(eval->type), RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::no)) { // disable substitue
                    // we need a cast
                    auto cast = make_smart<ExprCast>(expr->at, resExpr, make_smart<TypeDecl>(*expr->recordType));
                    resExpr = cast;
                }
                return resExpr;
            }
        }
        expr->type = resT;
        verifyType(expr->type);
        if ( resT->isAutoOrAlias() ) {
            error("array element type is not resolved", "", "",
                  expr->at, CompilationError::invalid_type);
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisitArrayComprehensionSubexpr(ExprArrayComprehension *expr, Expression *subexpr) {
        Visitor::preVisitArrayComprehensionSubexpr(expr, subexpr);
        pushVarStack();
        auto eFor = static_cast<ExprFor *>(expr->exprFor.get());
        preVisitForStack(eFor);
    }
    void InferTypes::preVisitArrayComprehensionWhere(ExprArrayComprehension *expr, Expression *where) {
        Visitor::preVisitArrayComprehensionWhere(expr, where);
    }
    ExpressionPtr InferTypes::visit(ExprArrayComprehension *expr) {
        popVarStack();
        if (expr->subexpr->type) {
            if (!expr->subexpr->type->canCopy() && !expr->subexpr->type->canMove()) {
                error("comprehension element has to be copyable or moveable", "", "",
                      expr->at, CompilationError::invalid_type);
            } else if (expr->subexpr->type->isAutoOrAlias()) {
                error("comprehension element type is not resolved", "", "",
                      expr->at, CompilationError::invalid_type);
            } else {
                auto pAC = expr->generatorSyntax ? generateComprehensionIterator(expr) : generateComprehension(expr, expr->tableSyntax);
                reportAstChanged();
                return pAC;
            }
        }
        return Visitor::visit(expr);
    }

}
