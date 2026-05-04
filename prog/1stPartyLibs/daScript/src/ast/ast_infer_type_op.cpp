#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_infer_type.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    bool InferTypes::isBitfieldOp(const Function *fnc) const {
        if (fnc->module->name == "$" && fnc->arguments[0]->type->isBitfield()) {
            return true;
        }
        return false;
    }
    ExpressionPtr InferTypes::visit(ExprOp1 *expr) {
        if (!expr->subexpr->type || expr->subexpr->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        // pointer arithmetics
        if (expr->subexpr->type->isPointer()) {
            if (!expr->subexpr->type->firstType) {
                error("operations on 'void' pointers are prohibited; " +
                          describeType(expr->subexpr->type),
                      "", "",
                      expr->at, CompilationError::invalid_type);
            } else {
                string pop;
                if (expr->op == "++" || expr->op == "+++") {
                    pop = "i_das_ptr_inc";
                } else if (expr->op == "--" || expr->op == "---") {
                    pop = "i_das_ptr_dec";
                }
                if (!pop.empty()) {
                    if ( expr->subexpr->type->firstType->isAuto() ) {
                        error("type is not fully inferred, fixed array dimension is unknown", "", "",
                              expr->at, CompilationError::invalid_type);
                        return Visitor::visit(expr);
                    }
                    reportAstChanged();
                    auto popc = make_smart<ExprCall>(expr->at, pop);
                    auto stride = expr->subexpr->type->firstType->getSizeOf();
                    popc->arguments.push_back(expr->subexpr->clone());
                    popc->arguments.push_back(make_smart<ExprConstInt>(expr->at, stride));
                    return popc;
                } else {
                    error("pointer arithmetics only allows +, -, +=, -=, ++, --; not" + expr->op, "", "",
                          expr->at, CompilationError::operator_not_found);
                }
            }
        }
        if (expr->op == "++" || expr->op == "+++" || expr->op == "--" || expr->op == "---") {
            if (expr->subexpr->type->isWorkhorseType()) {
                if (!expr->subexpr->type->ref) {
                    error(expr->op + " can't be applied to non reference " + describeType(expr->subexpr->type), "", "",
                          expr->at, CompilationError::operator_not_found);
                    return Visitor::visit(expr);
                } else if (expr->subexpr->type->constant) {
                    error(expr->op + " can't be applied to constant " + describeType(expr->subexpr->type), "", "",
                          expr->at, CompilationError::operator_not_found);
                    return Visitor::visit(expr);
                }
                if (unsafeTableLookup && expr->subexpr->rtti_isAt()) { // tab[expr]++ is always safe
                    auto pAt = static_cast<ExprAt *>(expr->subexpr.get());
                    if (!pAt->alwaysSafe && pAt->subexpr->type && pAt->subexpr->type->isGoodTableType()) {
                        pAt->alwaysSafe = true;
                        reportAstChanged();
                    }
                }
            }
        }
        auto opName = "_::" + expr->op;
        auto tempCall = make_smart<ExprLooksLikeCall>(expr->at, opName);
        tempCall->arguments.push_back(expr->subexpr);
        expr->func = inferFunctionCall(tempCall.get()).get();
        if (opName != tempCall->name) { // this happens when the operator gets instanced
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr->at, tempCall->name);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        }
        if (expr->func) {
            if (expr->func->firstArgReturnType || isBitfieldOp(expr->func)) {
                TypeDecl::clone(expr->type, expr->subexpr->type);
                expr->type->ref = false;
            } else {
                TypeDecl::clone(expr->type, expr->func->result);
            }
            if (!expr->func->arguments[0]->type->isRef())
                expr->subexpr = Expression::autoDereference(expr->subexpr);
            // lets try to fold it
            if (expr->func && expr->func->unsafeOperation && safeExpression(expr)) {
                error("unsafe operator '" + expr->name + "' must be inside the 'unsafe' block", "", "",
                      expr->at, CompilationError::unsafe);
            } else if (enableInferTimeFolding && isConstExprFunc(expr->func)) {
                if (auto se = getConstExpr(expr->subexpr.get())) {
                    expr->subexpr = se;
                    return evalAndFold(expr);
                }
            }
        }
        return Visitor::visit(expr);
    }
    bool InferTypes::isAssignmentOperator(const string &op) {
        return (op == "+=") || (op == "-=") || (op == "*=") || (op == "/=") || (op == "%=") || (op == "&=") || (op == "|=") || (op == "^=") || (op == "||=") || (op == "&&=") || (op == "^^=") || (op == "<<=") || (op == ">>=") || (op == "<<<=") || (op == ">>>=");
    }
    bool InferTypes::isLogicOperator(const string &op) {
        return (op == "==") || (op == "!=") || (op == ">=") || (op == "<=") || (op == ">") || (op == "<");
    }
    bool InferTypes::isCommutativeOperator(const string &op) {
        return (op == "*") || (op == "+") || isLogicOperator(op);
    }
    string InferTypes::flipCommutativeOperatorSide(const string &op) {
        if (op == ">")
            return "<";
        else if (op == "<")
            return ">";
        else if (op == ">=")
            return "<=";
        else if (op == "<=")
            return ">=";
        else
            return op;
    }
    bool InferTypes::canOperateOnPointers(const TypeDeclPtr &leftType, const TypeDeclPtr &rightType, TemporaryMatters tmatter) const {
        if (leftType->baseType == Type::tPointer) {
            return leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::yes);
        } else {
            return leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::no);
        }
    }
    bool InferTypes::isSameSmartPtrType(const TypeDeclPtr &lt, const TypeDeclPtr &rt, bool leftOnly) {
        auto lt_smart = lt->smartPtr;
        auto rt_smart = rt->smartPtr;
        if (leftOnly) {
            // smart smart  - ok
            // smart ptr    - ok
            // ptr   smart  - not OK
            // ptr   ptr    - ok
            if (!lt_smart && rt_smart)
                return false;
        }
        lt->smartPtr = false;
        rt->smartPtr = false;
        bool res = canOperateOnPointers(lt, rt, TemporaryMatters::no);
        lt->smartPtr = lt_smart;
        rt->smartPtr = rt_smart;
        return res;
    }
    void InferTypes::preVisit(ExprOp2 *expr) {
        Visitor::preVisit(expr);
        if (!strictProperties && isAssignmentOperator(expr->op)) {
            if (expr->left->rtti_isField()) {
                auto field = static_pointer_cast<ExprField>(expr->left);
                field->underClone = true;
            } else if (expr->left->rtti_isVar()) {
                auto var = static_pointer_cast<ExprVar>(expr->left);
                var->underClone = true;
            } else if (expr->left->rtti_isAt()) {
                auto at = static_pointer_cast<ExprAt>(expr->left);
                at->underClone = true;
            }
        }
    }
    void InferTypes::removeR2v(ExpressionPtr &expr) {
        if (expr->rtti_isCallLikeExpr()) {
            auto call = static_pointer_cast<ExprLooksLikeCall>(expr);
            if (call->arguments.size() == 2) {
                if (call->arguments[1]->rtti_isR2V()) {
                    auto arg = static_pointer_cast<ExprRef2Value>(call->arguments[1]);
                    call->arguments[1] = arg->subexpr;
                    reportAstChanged();
                }
            }
        }
    }
    ExpressionPtr InferTypes::promoteOp2ToProperty(ExprOp2 *expr) {
        // a.b += c  -> a`b`set(a`b`get() + c)
        if (expr->right->type && !expr->right->type->isAutoOrAlias()) {
            auto opName = expr->op;
            opName.pop_back();
            if (expr->left->rtti_isVar()) {
                ExprVar *evar = (ExprVar *)(expr->left.get());
                if (auto propGet = promoteToProperty(evar, nullptr)) { // we need both get and set
                    auto opRight = make_smart<ExprOp2>(expr->at, opName, propGet, expr->right);
                    opRight->type = make_smart<TypeDecl>(*expr->right->type);
                    if (auto cloneSet = promoteToProperty(evar, opRight)) {
                        removeR2v(cloneSet);
                        return cloneSet;
                    } else {
                        expr->left = propGet;
                        return expr;
                    }
                }
            } else if (expr->left->rtti_isField()) {
                ExprField *efield = (ExprField *)(expr->left.get());
                if (auto propSetOp = promoteToProperty(efield, expr->right, expr->op)) {
                    removeR2v(propSetOp);
                    return propSetOp;                                           // we need only set
                } else if (auto propGet = promoteToProperty(efield, nullptr)) { // we need both get and set
                    auto opRight = make_smart<ExprOp2>(expr->at, opName, propGet, expr->right);
                    opRight->type = make_smart<TypeDecl>(*expr->right->type);
                    if (auto cloneSet = promoteToProperty(efield, opRight)) {
                        removeR2v(cloneSet);
                        return cloneSet;
                    } else {
                        expr->left = propGet;
                        return expr;
                    }
                }
            } else if (expr->left->rtti_isAt()) {
                ExprAt *eat = (ExprAt *)(expr->left.get());
                auto complexName = "[]" + expr->name;
                if (auto atComplex = inferGenericOperator3(complexName, eat->at, eat->subexpr, eat->index, expr->right)) {
                    atComplex->alwaysSafe = eat->alwaysSafe | expr->alwaysSafe;
                    removeR2v(atComplex);
                    return atComplex;                                                                    // we need only set
                } else if (auto atGet = inferGenericOperator("[]", eat->at, eat->subexpr, eat->index)) { // we need bot get and set
                    atGet->alwaysSafe = eat->alwaysSafe | expr->alwaysSafe;
                    auto opRight = make_smart<ExprOp2>(expr->at, opName, atGet, expr->right);
                    opRight->type = make_smart<TypeDecl>(*expr->right->type);
                    if (auto atSet = inferGenericOperator3("[]=", eat->at, eat->subexpr, eat->index, opRight)) {
                        atSet->alwaysSafe = eat->alwaysSafe | expr->alwaysSafe;
                        removeR2v(atSet);
                        return atSet;
                    } else {
                        reportAstChanged();
                        expr->left = atGet;
                        return nullptr;
                    }
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::visit(ExprOp2 *expr) {
        if (!strictProperties && expr->right->type && isAssignmentOperator(expr->op)) {
            if (auto nExpr = promoteOp2ToProperty(expr)) {
                reportAstChanged();
                return nExpr;
            }
        }
        if (!expr->left->type || expr->left->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        if (!expr->right->type || expr->right->type->isAliasOrExpr())
            return Visitor::visit(expr); // failed to infer
        // flippling commutative operations, if the constant is on the left
        if (expr->left->rtti_isConstant() && !expr->right->rtti_isConstant()) {               // if left is const, but right is not
            if (expr->left->type->isNumericComparable() && isCommutativeOperator(expr->op)) { // if its compareable, and its a logic operator
                auto flip = flipCommutativeOperatorSide(expr->op);                            // we swap left and right, and change the op
                auto nexpr = make_smart<ExprOp2>(expr->at, flip, expr->right, expr->left);
                reportAstChanged();
                return nexpr;
            }
        }
        // pointer arithmetics
        if (expr->left->type->isPointer() && expr->right->type->isIndexExt()) {
            if (!expr->left->type->firstType) {
                error("operations on 'void' pointers are prohibited; " +
                          describeType(expr->left->type),
                      "", "",
                      expr->at, CompilationError::invalid_type);
            } else {
                string pop;
                if (expr->op == "+") {
                    pop = "i_das_ptr_add";
                } else if (expr->op == "-") {
                    pop = "i_das_ptr_sub";
                } else if (expr->op == "+=") {
                    pop = "i_das_ptr_set_add";
                } else if (expr->op == "-=") {
                    pop = "i_das_ptr_set_sub";
                }
                if (!pop.empty()) {
                    reportAstChanged();
                    auto popc = make_smart<ExprCall>(expr->at, pop);
                    popc->alwaysSafe = expr->alwaysSafe;
                    auto stride = expr->left->type->firstType->getSizeOf();
                    popc->arguments.push_back(expr->left->clone());
                    popc->arguments.push_back(expr->right->clone());
                    popc->arguments.push_back(make_smart<ExprConstInt>(expr->at, stride));
                    return popc;
                } else {
                    error("pointer arithmetics only allows +, -, +=, -=, ++, --; not" + expr->op, "", "",
                          expr->at, CompilationError::operator_not_found);
                }
            }
        }
        // infer
        if (expr->left->type->isPointer() && expr->right->type->isPointer()) {
            if (!isSameSmartPtrType(expr->left->type, expr->right->type)) {
                error("operations on incompatible pointers are prohibited; " +
                          describeType(expr->left->type) + " vs " + describeType(expr->right->type),
                      "", "",
                      expr->at, CompilationError::invalid_type);
            }
            if (expr->op == "-") {
                if (!expr->left->type->firstType) {
                    error("operations on 'void' pointers are prohibited; " +
                              describeType(expr->left->type),
                          "", "",
                          expr->at, CompilationError::invalid_type);
                } else {
                    reportAstChanged();
                    auto popc = make_smart<ExprCall>(expr->at, "i_das_ptr_diff");
                    auto stride = expr->left->type->firstType->getSizeOf();
                    popc->arguments.push_back(expr->left->clone());
                    popc->arguments.push_back(expr->right->clone());
                    popc->arguments.push_back(make_smart<ExprConstInt>(expr->at, stride));
                    return popc;
                }
            }
        }
        if (expr->left->type->isEnum() && expr->right->type->isEnum())
            if (!expr->left->type->isSameType(*expr->right->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no))
                error("operations on different enumerations are prohibited", "", "",
                      expr->at, CompilationError::invalid_type);
        auto opName = "_::" + expr->op;
        auto tempCall = make_smart<ExprLooksLikeCall>(expr->at, opName);
        tempCall->arguments.push_back(expr->left);
        tempCall->arguments.push_back(expr->right);
        expr->func = inferFunctionCall(tempCall.get(), InferCallError::operatorOp2).get();
        if (opName != tempCall->name) { // this happens when the operator gets instanced
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr->at, tempCall->name);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        }
        if (expr->func) {
            if (expr->func->firstArgReturnType) {
                TypeDecl::clone(expr->type, expr->left->type);
                expr->type->ref = false;
            } else if (isBitfieldOp(expr->func)) {
                TypeDecl::clone(expr->type, expr->func->result);
                if (!expr->left->type->alias.empty()) {
                    expr->type->alias = expr->left->type->alias;
                } else if (!expr->right->type->alias.empty()) {
                    expr->type->alias = expr->right->type->alias;
                }
            } else {
                TypeDecl::clone(expr->type, expr->func->result);
            }
            if (!expr->func->arguments[0]->type->isRef())
                expr->left = Expression::autoDereference(expr->left);
            if (!expr->func->arguments[1]->type->isRef())
                expr->right = Expression::autoDereference(expr->right);
            // lets try to fold it
            if (expr->func && expr->func->unsafeOperation && !safeExpression(expr)) {
                error("unsafe operator '" + expr->name + "' must be inside the 'unsafe' block", "", "",
                      expr->at, CompilationError::unsafe);
            } else if (enableInferTimeFolding && isConstExprFunc(expr->func)) {
                auto lcc = getConstExpr(expr->left.get());
                auto rcc = getConstExpr(expr->right.get());
                if (lcc && rcc) {
                    expr->left = lcc;
                    expr->right = rcc;
                    return evalAndFold(expr);
                }
            }
        }
        return Visitor::visit(expr);
    }
    ExpressionPtr InferTypes::visit(ExprOp3 *expr) {
        if (!expr->subexpr->type || !expr->left->type || !expr->right->type)
            return Visitor::visit(expr);
        // infer
        if (expr->op != "?") {
            error("Op3 currently only supports 'is'", "", "",
                  expr->at, CompilationError::operator_not_found);
            return Visitor::visit(expr);
        }
        expr->subexpr = Expression::autoDereference(expr->subexpr);
        if (!expr->subexpr->type->isSimpleType(Type::tBool)) {
            error("cond operator condition must be boolean", "", "",
                  expr->at, CompilationError::condition_must_be_bool);
        } else if (!expr->left->type->isSameType(*expr->right->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
            error("cond operator must return the same types on both sides",
                  "\t" + (verbose ? (expr->left->type->describe() + " vs " + expr->right->type->describe()) : ""), "",
                  expr->at, CompilationError::operator_not_found);
        } else if (expr->left->type->isVoid()) {
            error("cond operator must return a value, not void", "", "",
                  expr->at, CompilationError::invalid_type);
        } else {
            if (expr->left->type->ref ^ expr->right->type->ref) { // if either one is not ref
                expr->left = Expression::autoDereference(expr->left);
                expr->right = Expression::autoDereference(expr->right);
            }
            TypeDecl::clone(expr->type, expr->left->type);
            expr->type->constant |= expr->right->type->constant;
            // lets try to fold it
            if (enableInferTimeFolding) {
                auto ccc = getConstExpr(expr->subexpr.get());
                auto lcc = getConstExpr(expr->left.get());
                auto rcc = getConstExpr(expr->right.get());
                if (ccc && lcc && rcc) {
                    expr->subexpr = ccc;
                    expr->left = lcc;
                    expr->right = rcc;
                    return evalAndFold(expr);
                }
            }
        }
        return Visitor::visit(expr);
    }
    bool InferTypes::isVoidOrNothing(const TypeDeclPtr &ptr) const {
        return !ptr || ptr->isVoid();
    }
    bool InferTypes::canCopyOrMoveType(const TypeDeclPtr &leftType, const TypeDeclPtr &rightType, TemporaryMatters tmatter, Expression *leftExpr,
                                       const string &errorText, CompilationError errorCode, const LineInfo &at) const {
        if (leftType->baseType == Type::tPointer) {
            if (!leftType->isVoidPointer() && rightType->isVoidPointer()) {
                if (leftExpr->rtti_isNullPtr()) { // anyptr = null ok, otherwise no void copy
                    return true;
                } else {
                    error(errorText + "; " + describeType(leftType) + " = " + describeType(rightType),
                          "void pointer can't be copied to a non-void pointer", "", at, errorCode);
                    return false;
                }
            }
            if (!relaxedPointerConst) {
                if (!leftType->constant && rightType->constant && !(leftType->firstType && leftType->firstType->constant) && !isVoidOrNothing(leftType->firstType)) {
                    error(errorText + "; " + describeType(leftType) + " = " + describeType(rightType),
                          "can't copy constant to non-constant pointer. needs to be " + describeType(leftType->firstType) + " const?", "", at, errorCode);
                    return false;
                }
            }
            if (!leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::yes, true, true)) {
                error(errorText + "; " + describeType(leftType) + " = " + describeType(rightType),
                      "not the same type", "", at, errorCode);
                return false;
            } else {
                return true;
            }
        } else {
            if (leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::no, true, true)) {
                if (!relaxedPointerConst) {
                    if (!leftType->constant && rightType->constant && !rightType->canCloneFromConst()) {
                        reportCantCloneFromConst(errorText, errorCode, rightType, at);
                        return false;
                    }
                }
                return true;
            } else {
                error(errorText + "; " + describeType(leftType) + " = " + describeType(rightType),
                      "not the same type", "", at, errorCode);
                return false;
            }
        }
    }
    string InferTypes::moveErrorInfo(ExprMove *expr) const {
        if (verbose) {
            return ", " + describeType(expr->left->type) + " <- " + describeType(expr->right->type);
        } else {
            return "";
        }
    }
    void InferTypes::preVisit(ExprMove *expr) {
        Visitor::preVisit(expr);
        markNoDiscard(expr->right.get());
    }
    ExpressionPtr InferTypes::visit(ExprMove *expr) {
        if (!expr->left->type || !expr->right->type)
            return Visitor::visit(expr);
        // infer
        if (!canCopyOrMoveType(expr->left->type, expr->right->type, TemporaryMatters::no, expr->right.get(),
                               "can only move compatible type", CompilationError::cant_move, expr->at)) {
        } else if (!expr->left->type->isRef()) {
            error("can only move to a reference" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_write_to_non_reference);
        } else if (!expr->right->type->isRef() && !expr->right->type->isMoveableValue()) {
            error("can only move to from a reference" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_write_to_non_reference);
        } else if (!expr->allowConstantLValue && expr->left->type->constant) {
            error("can't move to a constant value" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_move_to_const);
        } else if (!expr->left->type->canMove()) {
            error("this type can't be moved, use clone (:=) instead" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_move);
        } else if (expr->right->type->constant) {
            error("can't move from a constant value" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_move);
        } else if (expr->right->type->isTemp(true, false)) {
            error("can't move temporary value" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_pass_temporary);
        } else if (strictSmartPointers && !safeExpression(expr) && expr->right->type->needInScope()) {
            error("moving values which contain smart pointers is unsafe",
                  "try `move(smart_ptr&) <| smart_ptr&` or `move_new(smart_ptr&) <| new [[YouTypeHere ...]]` instead", "",
                  expr->at, CompilationError::unsafe);
        } else if (expr->right->type->isPointer() && expr->right->type->smartPtr) {
            if (!expr->right->type->ref && !safeExpression(expr) && !expr->right->rtti_isAscend()) {
                error("moving from the smart pointer value requires unsafe", "",
                      "try moving from reference instead",
                      expr->at, CompilationError::unsafe);
                return Visitor::visit(expr);
            }
        } else if (expr->left->type->hasClasses() && !safeExpression(expr)) {
            error("moving classes requires unsafe" + moveErrorInfo(expr), "", "",
                  expr->at, CompilationError::unsafe);
        } else if (
            forceInscopePod && !expr->podDelete && func && (func->module->allowPodInscope && (!func->fromGeneric || func->fromGeneric->module->allowPodInscope)) // both modules allow pod inscope
            && !func->hasUnsafe && isPodDelete(expr->left->type.get())                                                                                           // its a pod type
        ) {
            reportAstChanged();
            if (logs && logInscopePod) {
                if (!expr->at.empty() && expr->at.fileInfo) {
                    *logs << expr->at.fileInfo->name << ":" << expr->at.line << ":" << expr->at.column << " ";
                }
                *logs << "In-scope POD applied to '" << *expr << "' in function '" << func->module->name << "::" << func->name << "'\n";
            }
            // we convert left <- right into
            // var left`temp & = left
            // builtin_collect_local_and_zero(left`temp, size_of(left))
            // left`temp <- right // unconvertable
            auto pBlock = make_smart<ExprBlock>();
            pBlock->at = expr->at;
            pBlock->isCollapseable = true;
            scopes.back()->needCollapse = true;
            auto pLet = make_smart<ExprLet>();
            pLet->at = expr->at;
            pLet->alwaysSafe = true;
            pLet->generated = true;
            auto pVar = make_smart<Variable>();
            pVar->at = expr->left->at;
            pVar->type = make_smart<TypeDecl>(Type::autoinfer);
            pVar->type->ref = true;
            pVar->name = "_pod_inscope_temp_" + to_string(pVar->at.line) + "_" + to_string(pVar->at.column);
            pVar->init = expr->left->clone();
            pVar->generated = true;
            pLet->variables.push_back(pVar);
            auto pCall = make_smart<ExprCall>(expr->at, "_::builtin_collect_local_and_zero");
            pCall->alwaysSafe = true;
            pCall->arguments.push_back(make_smart<ExprVar>(expr->at, pVar->name));
            pCall->arguments.push_back(make_smart<ExprConstUInt>(expr->at, expr->left->type->getSizeOf()));
            auto pMove = make_smart<ExprMove>(expr->at, make_smart<ExprVar>(expr->at, pVar->name), expr->right->clone());
            pMove->podDelete = true;
            pBlock->list.push_back(pLet);
            pBlock->list.push_back(pCall);
            pBlock->list.push_back(pMove);
            return pBlock;
        }
        expr->type = make_smart<TypeDecl>(); // we return nothing
        return Visitor::visit(expr);
    }
    string InferTypes::copyErrorInfo(ExprCopy *expr) const {
        if (verbose) {
            return ", " + describeType(expr->left->type) + " = " + describeType(expr->right->type);
        } else {
            return "";
        }
    }
    void InferTypes::preVisit(ExprCopy *expr) {
        Visitor::preVisit(expr);
        if (!strictProperties) {
            if (expr->left->rtti_isField()) {
                auto field = static_pointer_cast<ExprField>(expr->left);
                field->underClone = true;
            } else if (expr->left->rtti_isVar()) {
                auto var = static_pointer_cast<ExprVar>(expr->left);
                var->underClone = true;
            } else if (expr->left->rtti_isAt()) {
                auto at = static_pointer_cast<ExprAt>(expr->left);
                at->underClone = true;
            }
        }
        markNoDiscard(expr->right.get());
    }
    ExpressionPtr InferTypes::visit(ExprCopy *expr) {
        if (auto nExpr = promoteAssignmentToProperty(expr)) {
            reportAstChanged();
            return nExpr;
        }
        if (!expr->left->type || !expr->right->type)
            return Visitor::visit(expr);
        // infer
        if (!canCopyOrMoveType(expr->left->type, expr->right->type, TemporaryMatters::no, expr->right.get(),
                               "can only copy compatible type", CompilationError::cant_copy, expr->at)) {
        } else if (!expr->left->type->isRef()) {
            error("can only copy to a reference" + copyErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_write_to_non_reference);
        } else if (!expr->allowConstantLValue && expr->left->type->constant) {
            error("can't write to a constant value" + copyErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_write_to_const);
        } else if (!expr->allowCopyTemp && expr->right->type->isTemp(true, false)) {
            error("can't copy temporary value" + copyErrorInfo(expr), "", "",
                  expr->at, CompilationError::cant_pass_temporary);
        } else if (expr->left->type->hasClasses() && !safeExpression(expr)) {
            error("copying classes requires unsafe" + copyErrorInfo(expr), "", "",
                  expr->at, CompilationError::unsafe);
        }
        if (!expr->left->type->canCopy()) {
            error("this type can't be copied" + copyErrorInfo(expr),
                  "", "use move (<-) or clone (:=) instead", expr->at, CompilationError::cant_copy);
            if (canRelaxAssign(expr->right.get())) {
                reportAstChanged();
                return make_smart<ExprMove>(expr->at, expr->left->clone(), expr->right->clone());
            }
        }
        expr->type = make_smart<TypeDecl>(); // we return nothing
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprClone *expr) {
        Visitor::preVisit(expr);
        if (expr->left->rtti_isField()) {
            auto field = static_pointer_cast<ExprField>(expr->left);
            field->underClone = true;
        } else if (expr->left->rtti_isVar()) {
            auto var = static_pointer_cast<ExprVar>(expr->left);
            var->underClone = true;
        } else if (expr->left->rtti_isAt()) {
            auto at = static_pointer_cast<ExprAt>(expr->left);
            at->underClone = true;
        }
        markNoDiscard(expr->right.get());
    }
    ExpressionPtr InferTypes::promoteAssignmentToProperty(ExprOp2 *expr) {
        if (expr->right->type && !expr->right->type->isAutoOrAlias()) {
            if (expr->left->rtti_isVar()) {
                ExprVar *evar = (ExprVar *)(expr->left.get());
                if (auto cloneSet = promoteToProperty(evar, expr->right.get())) {
                    return cloneSet;
                }
                if (auto propGet = promoteToProperty(evar, nullptr)) {
                    expr->left = propGet;
                    return expr;
                }
            } else if (expr->left->rtti_isField()) {
                ExprField *efield = (ExprField *)(expr->left.get());
                if (auto cloneSet = promoteToProperty(efield, expr->right.get())) {
                    return cloneSet;
                }
                if (auto propGet = promoteToProperty(efield, nullptr)) {
                    expr->left = propGet;
                    return expr;
                }
                if (expr->right->type->isBool() && efield->value->type && efield->value->type->isBitfield()) { // bitfield.field = bool
                    auto value = efield->value;
                    if (value->rtti_isR2V()) {
                        value = static_pointer_cast<ExprRef2Value>(value)->subexpr;
                    }
                    if (value->type->ref) {
                        // lets find the right field
                        auto fidx = efield->value->type->bitFieldIndex(efield->name);
                        if (fidx != -1) {
                            reportAstChanged();
                            auto mask = make_smart<ExprConstBitfield>(efield->at, 1ul << fidx);
                            mask->bitfieldType = make_smart<TypeDecl>(*efield->value->type);
                            auto call = make_smart<ExprCall>(efield->at, "__bit_set");
                            call->arguments.push_back(value->clone());
                            call->arguments.push_back(mask);
                            call->arguments.push_back(expr->right->clone());
                            return call;
                        }
                    }
                }
            } else if (expr->left->rtti_isAt()) {
                ExprAt *eat = (ExprAt *)(expr->left.get());
                // lets find []= operator
                auto opName = "[]" + expr->name;
                if (auto opAtEq = inferGenericOperator3(opName, expr->at, eat->subexpr, eat->index, expr->right)) {
                    opAtEq->alwaysSafe = eat->alwaysSafe | expr->alwaysSafe;
                    return opAtEq;
                }
                // now, lets see if at itself can be promoted
                if (auto OpAt = inferGenericOperator("[]", expr->at, eat->subexpr, eat->index)) {
                    reportAstChanged();
                    OpAt->alwaysSafe = eat->alwaysSafe;
                    expr->left = OpAt;
                    return nullptr;
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::visit(ExprClone *expr) {
        if (auto nExpr = promoteAssignmentToProperty(expr)) {
            reportAstChanged();
            return nExpr;
        }
        if (!expr->left->type || !expr->right->type) {
            return Visitor::visit(expr);
        }
        if (expr->left->type->isAliasOrExpr() || expr->right->type->isAliasOrExpr()) {
            return Visitor::visit(expr); // failed to infer
        }
        // lets infer clone call (and instance generic if need be)
        auto opName = "_::clone";
        auto tempCall = make_smart<ExprLooksLikeCall>(expr->at, opName);
        tempCall->arguments.push_back(expr->left);
        tempCall->arguments.push_back(expr->right);
        expr->func = inferFunctionCall(tempCall.get(), InferCallError::tryOperator).get();
        if (expr->func || opName != tempCall->name) { // this happens when the clone gets instanced
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr->at, tempCall->name);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        }
        // infer
        if (!isSameSmartPtrType(expr->left->type, expr->right->type, true)) {
            error("can only clone the same type " + describeType(expr->left->type) + " vs " + describeType(expr->right->type), "", "",
                  expr->at, CompilationError::operator_not_found);
        } else if (!expr->left->type->isRef()) {
            error("can only clone to a reference", "", "",
                  expr->at, CompilationError::cant_write_to_non_reference);
        } else if (expr->left->type->constant) {
            error("can't write to a constant value " + expr->left->describe(), "", "",
                  expr->at, CompilationError::cant_write_to_const);
        } else if (!expr->left->type->canClone()) {
            reportCantClone("type " + describeType(expr->left->type) + " can't be cloned from " + describeType(expr->right->type),
                            expr->left->type, expr->at);
        } else if (!relaxedPointerConst && expr->right->type->constant && !expr->right->type->canCloneFromConst()) {
            reportCantCloneFromConst("type can't be cloned from const type", CompilationError::cant_copy,
                                     expr->right->type, expr->at);
        } else {
            auto cloneType = expr->left->type;
            if (cloneType->isHandle()) {
                expr->type = make_smart<TypeDecl>(); // we return nothing
                return Visitor::visit(expr);
            } else if (cloneType->isString() && (expr->right->type->isTemp() || multiContext)) {
                reportAstChanged();
                auto cloneFn = make_smart<ExprCall>(expr->at, "clone_string");
                cloneFn->arguments.push_back(expr->right->clone());
                return make_smart<ExprCopy>(expr->at, expr->left->clone(), cloneFn);
            } else if (cloneType->isPointer() && cloneType->smartPtr) {
                if ( !cloneType->firstType || !cloneType->firstType->annotation ) {
                    error("can only clone smart pointer to handled type", "", "",
                          expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                auto fnClone = makeCloneSmartPtr(expr->at, cloneType, expr->right->type);
                if (program->addFunction(fnClone)) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return cloneFn;
                } else {
                    reportMissingFinalizer("smart pointer clone mismatch ", expr->at, cloneType);
                    return Visitor::visit(expr);
                }
            } else if (cloneType->canCopy(expr->right->type->isTemp() || multiContext)) {
                reportAstChanged();
                auto eCopy = make_smart<ExprCopy>(expr->at, expr->left->clone(), expr->right->clone());
                eCopy->allowCopyTemp = true;
                return eCopy;
            } else if (cloneType->isGoodArrayType() || cloneType->isGoodTableType()) {
                reportAstChanged();
                auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                cloneFn->arguments.push_back(expr->left->clone());
                cloneFn->arguments.push_back(expr->right->clone());
                return cloneFn;
            } else if (cloneType->isStructure()) {
                reportAstChanged();
                auto stt = cloneType->structType;
                auto fnList = getCloneFunc(cloneType, cloneType);
                if (verifyCloneFunc(fnList, expr->at)) {
                    if (fnList.size() == 0) {
                        auto clf = makeClone(stt);
                        if (relaxedPointerConst) {
                            clf->arguments[1]->type->constant = true; // we clone from const, regardless
                        }
                        clf->privateFunction = true;
                        extraFunctions.push_back(clf);
                    }
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return cloneFn;
                } else {
                    return Visitor::visit(expr);
                }
            } else if (cloneType->isTuple()) {
                reportAstChanged();
                auto fnList = getCloneFunc(cloneType, cloneType);
                if (verifyCloneFunc(fnList, expr->at)) {
                    if (fnList.size() == 0) {
                        auto clf = makeCloneTuple(expr->at, cloneType, expr->right->type->constant);
                        clf->privateFunction = true;
                        extraFunctions.push_back(clf);
                    }
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return cloneFn;
                } else {
                    return Visitor::visit(expr);
                }
            } else if (cloneType->isVariant()) {
                reportAstChanged();
                auto fnList = getCloneFunc(cloneType, cloneType);
                if (verifyCloneFunc(fnList, expr->at)) {
                    if (fnList.size() == 0) {
                        auto clf = makeCloneVariant(expr->at, cloneType, expr->right->type->constant);
                        clf->privateFunction = true;
                        extraFunctions.push_back(clf);
                    }
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return cloneFn;
                } else {
                    return Visitor::visit(expr);
                }
            } else if (cloneType->dim.size()) {
                reportAstChanged();
                auto cloneFn = make_smart<ExprCall>(expr->at, "clone_dim");
                cloneFn->arguments.push_back(expr->left->clone());
                cloneFn->arguments.push_back(expr->right->clone());
                return cloneFn;
            } else {
                reportCantClone("this type can't be cloned " + describeType(cloneType),
                                cloneType, expr->at);
            }
        }
        return Visitor::visit(expr);
    }
    void InferTypes::preVisit(ExprTryCatch *expr) {
        Visitor::preVisit(expr);
        if (func)
            func->hasTryRecover = true;
    }
    ExpressionPtr InferTypes::visit(ExprTryCatch *expr) {
        if (jitEnabled()) {
            auto tryBlock = make_smart<ExprMakeBlock>(expr->try_block->at, expr->try_block);
            ((ExprBlock *)tryBlock->block.get())->returnType = make_smart<TypeDecl>(Type::autoinfer);
            auto catchBlock = make_smart<ExprMakeBlock>(expr->catch_block->at, expr->catch_block);
            ((ExprBlock *)catchBlock->block.get())->returnType = make_smart<TypeDecl>(Type::autoinfer);
            auto ccall = make_smart<ExprCall>(expr->at, "builtin_try_recover");
            ccall->arguments.push_back(tryBlock);
            ccall->arguments.push_back(catchBlock);
            return ccall;
        }
        return Visitor::visit(expr);
    }

}
