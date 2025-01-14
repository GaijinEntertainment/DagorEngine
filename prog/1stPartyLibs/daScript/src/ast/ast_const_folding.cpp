#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/simulate/debug_print.h"

/*
TODO:
    Fold vector constructors
*/

namespace das {

    void PassVisitor::preVisitProgram ( Program * prog ) {
        Visitor::preVisitProgram(prog);
        program = prog;
        anyFolding = false;
    }

    void PassVisitor::visitProgram ( Program * prog ) {
        program = nullptr;
        Visitor::visitProgram(prog);
    }

    void PassVisitor::reportFolding() {
        anyFolding = true;
    }

    class SetSideEffectVisitor : public Visitor {
        // any expression
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            expr->noSideEffects = false;
            expr->noNativeSideEffects = false;
        }
    };

    class NoSideEffectVisitor : public Visitor {
    protected:
        // virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        // virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        // virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
    // make block
        virtual void preVisit ( ExprMakeBlock * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
        }
    // make generator
        virtual void preVisit ( ExprMakeGenerator * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
        }
    // cast
        virtual void preVisit ( ExprCast * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = expr->subexpr->noSideEffects;
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects;
        }
    // const
        virtual void preVisit ( ExprConst * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
        }
    // find
        virtual void preVisit ( ExprFind * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
        }
    // key_exists
        virtual void preVisit ( ExprKeyExists * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
        }
    // variable
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // swizzle
        virtual void preVisit ( ExprSwizzle * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // field
        virtual void preVisit ( ExprField * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // safe-field
        virtual void preVisit ( ExprSafeField * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // is variant
        virtual void preVisit ( ExprIsVariant * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // as variant
        virtual void preVisit ( ExprAsVariant * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // safe as variant
        virtual void preVisit ( ExprSafeAsVariant * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // null-coalescing
        virtual ExpressionPtr visit ( ExprNullCoalescing * expr ) override {
            expr->noSideEffects = expr->subexpr->noSideEffects && expr->defaultValue->noSideEffects;
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects && expr->defaultValue->noNativeSideEffects;
            return Visitor::visit(expr);
        }
    // at
        virtual void preVisit ( ExprAt * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->subexpr->type->isHandle() && expr->subexpr->type->annotation->isIndexMutable(expr->index->type.get()) ) {
                expr->noSideEffects = false;
            } else if ( expr->subexpr->type->isGoodTableType() ) {
                expr->noSideEffects = false;
            } else {
                expr->noSideEffects = true;
            }
            // expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // at
        virtual void preVisit ( ExprSafeAt * expr ) override {
            Visitor::preVisit(expr);
            expr->noSideEffects = true; // !expr->write;
            expr->noNativeSideEffects = true;
        }
    // op1
        virtual ExpressionPtr visit ( ExprOp1 * expr ) override {
            expr->noSideEffects = expr->subexpr->noSideEffects && (expr->func->sideEffectFlags==0);
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects
                && ((expr->func->sideEffectFlags & uint32_t(SideEffects::inferredSideEffects))==0);
            return Visitor::visit(expr);
        }
    // op2
        virtual ExpressionPtr visit ( ExprOp2 * expr ) override {
            expr->noSideEffects = expr->left->noSideEffects && expr->right->noSideEffects && (expr->func->sideEffectFlags==0);
            expr->noNativeSideEffects = expr->left->noNativeSideEffects && expr->right->noNativeSideEffects
                && ((expr->func->sideEffectFlags & uint32_t(SideEffects::inferredSideEffects))==0);
            return Visitor::visit(expr);
        }
    // op3
        virtual ExpressionPtr visit ( ExprOp3 * expr ) override {
            expr->noSideEffects = expr->subexpr->noSideEffects && expr->left->noSideEffects && expr->right->noSideEffects;
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects
                && expr->left->noNativeSideEffects && expr->right->noNativeSideEffects;
            return Visitor::visit(expr);
        }
    // call
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            expr->noSideEffects = (expr->func->sideEffectFlags==0);
            expr->noNativeSideEffects = (expr->func->sideEffectFlags & uint32_t(SideEffects::inferredSideEffects))==0;
            if ( expr->noSideEffects ) {
                for ( auto & arg : expr->arguments ) {
                    expr->noSideEffects &= arg->noSideEffects;
                    expr->noNativeSideEffects &= arg->noNativeSideEffects;
                }
            }
            return Visitor::visit(expr);
        }
    // make tuple
        virtual ExpressionPtr visit ( ExprMakeTuple * expr ) override {
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
            for ( auto & val : expr->values ) {
                expr->noSideEffects &= val->noSideEffects;
                expr->noNativeSideEffects &= val->noNativeSideEffects;
            }
            return Visitor::visit(expr);
        }
    // make array
        virtual ExpressionPtr visit ( ExprMakeArray * expr ) override {
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
            for ( auto & val : expr->values ) {
                expr->noSideEffects &= val->noSideEffects;
                expr->noNativeSideEffects &= val->noNativeSideEffects;
            }
            return Visitor::visit(expr);
        }
    // make structure
        virtual ExpressionPtr visit ( ExprMakeStruct * expr ) override {
            if ( expr->block ) {
                // TODO: determine real side effects of the block
                expr->noSideEffects = false;
                expr->noNativeSideEffects = false;
            } else {
                expr->noSideEffects = true;
                expr->noNativeSideEffects = true;
                for ( const auto & mksp : expr->structs ) {
                    for ( const auto & fld : *mksp ) {
                        expr->noSideEffects &= fld->value->noSideEffects;
                        expr->noNativeSideEffects &= fld->value->noNativeSideEffects;
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // make variant
        virtual ExpressionPtr visit ( ExprMakeVariant * expr ) override {
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
            for ( const auto & fld : expr->variants ) {
                expr->noSideEffects &= fld->value->noSideEffects;
                expr->noNativeSideEffects &= fld->value->noNativeSideEffects;
            }
            return Visitor::visit(expr);
        }
    // looks like call
        /*
        virtual ExpressionPtr visit ( ExprLooksLikeCall * expr ) override {
            if ( expr->noSideEffects ) {
                for ( auto & arg : expr->arguments ) {
                    expr->noSideEffects &= arg->noSideEffects;
                }
            }
            return Visitor::visit(expr);
        }
        */
    // string-builder
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) override {
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
            for ( auto & arg : expr->elements ) {
                expr->noSideEffects &= arg->noSideEffects;
                expr->noNativeSideEffects &= arg->noNativeSideEffects;
            }
            return Visitor::visit(expr);
        }
    // addr
        virtual ExpressionPtr visit ( ExprAddr * expr ) override {
            expr->noSideEffects = true;
            expr->noNativeSideEffects = true;
            return Visitor::visit(expr);
        }
    // ref2value
        virtual ExpressionPtr visit ( ExprRef2Value * expr ) override {
            expr->noSideEffects = expr->subexpr->noSideEffects;
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects;
            return Visitor::visit(expr);
        }
    // ptr2ref
        virtual ExpressionPtr visit ( ExprPtr2Ref * expr ) override {
            expr->noSideEffects = expr->subexpr->noSideEffects;
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects;
            return Visitor::visit(expr);
        }
    // ref2ptr
        virtual ExpressionPtr visit ( ExprRef2Ptr * expr ) override {
            expr->noSideEffects = expr->subexpr->noSideEffects;
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects;
            return Visitor::visit(expr);
        }
    };

    vec4f FoldingVisitor::eval ( Expression * expr, bool & failed ) {
        ctx.restart();
        ctx.thisProgram = program;
        auto node = expr->simulate(ctx);
        ctx.restart();
        auto fb = program->folding;
        vec4f result = ctx.evalWithCatch(node);
        program->folding = fb;
        if ( ctx.getException() ) {
            failed = true;
            return v_zero();
        } else {
            failed = false;
            return result;
        }
    }

    ExpressionPtr FoldingVisitor::evalAndFold ( Expression * expr ) {
        if ( expr->type->baseType == Type::tString ) return evalAndFoldString(expr);
        if ( expr->rtti_isConstant() ) return expr;
        bool failed;
        vec4f value = eval(expr, failed);
        if ( !failed ) {
            if ( expr->type->isEnumT() ) {
                int64_t ival = 0;
                switch ( expr->type->enumType->baseType ) {
                case Type::tInt8:       ival = cast<int8_t>::to(value); break;
                case Type::tUInt8:      ival = cast<uint8_t>::to(value); break;
                case Type::tInt16:      ival = cast<int16_t>::to(value); break;
                case Type::tUInt16:     ival = cast<uint16_t>::to(value); break;
                case Type::tInt:        ival = cast<int32_t>::to(value); break;
                case Type::tUInt:       ival = cast<uint32_t>::to(value); break;
                case Type::tBitfield:   ival = cast<uint32_t>::to(value); break;
                case Type::tInt64:      ival = cast<int64_t>::to(value); break;
                case Type::tUInt64:     ival = cast<uint64_t>::to(value); break;
                default: DAS_ASSERTF(0,"we should not be here. unsupported enum type");
                }
                auto cef = expr->type->enumType->find(ival, "");
                if ( cef.empty() ) return expr; // it folded to unsupported value
                auto sim = make_smart<ExprConstEnumeration>(expr->at, cef, expr->type);
                sim->type = expr->type->enumType->makeEnumType();
                sim->constexpression = true;
                sim->at = encloseAt(expr);
                sim->value = value;
                sim->foldedNonConst = !expr->type->constant;
                reportFolding();
                return sim;
            } else {
                auto wasRef = expr->type->ref;
                expr->type->ref = false;
                auto sim = Program::makeConst(expr->at, expr->type, value);
                expr->type->ref = wasRef;
                sim->type = make_smart<TypeDecl>(*expr->type);
                sim->constexpression = true;
                ((ExprConst *)sim.get())->foldedNonConst = !expr->type->constant;
                sim->at = encloseAt(expr);
                reportFolding();
                return sim;
            }
        } else {
            return expr;
        }
    }

    ExpressionPtr FoldingVisitor::evalAndFoldString ( Expression * expr ) {
        if ( expr->rtti_isStringConstant() ) return expr;
        bool failed;
        vec4f value = eval(expr, failed);
        if ( !failed ) {
            auto pTypeInfo = helper.makeTypeInfo(nullptr,expr->type);
            auto res = debug_value(value, pTypeInfo, PrintFlags::string_builder);
            auto sim = make_smart<ExprConstString>(expr->at, res);
            sim->type = make_smart<TypeDecl>(Type::tString);
            sim->constexpression = true;
            sim->foldedNonConst = !expr->type->constant;
            sim->at = encloseAt(expr);
            reportFolding();
            return sim;
        } else {
            return expr;
        }
    }

    ExpressionPtr FoldingVisitor::cloneWithType ( const ExpressionPtr & expr ) {
        auto rexpr = expr->clone();
        if ( expr->type ) rexpr->type = make_smart<TypeDecl>(*expr->type);
        return rexpr;
    }

    ExpressionPtr FoldingVisitor::evalAndFoldStringBuilder ( ExprStringBuilder * expr )  {
        // concatinate all constant strings, which are close together
        smart_ptr<ExprConstString> str;
        for ( auto it=expr->elements.begin(); it != expr->elements.end(); ) { // note - loop has erase, don't store 'end'
            auto & elem = *it;
            if ( elem->rtti_isStringConstant() ) {
                auto selem = static_pointer_cast<ExprConstString>(elem);
                if ( str ) {
                    str->text += selem->text;
                    it = expr->elements.erase(it);
                    reportFolding();
                } else {
                    str = static_pointer_cast<ExprConstString>(cloneWithType(selem));
                    elem = str;
                    ++ it;
                }
            } else {
                str.reset();
                ++ it;
            }
        }
        // check if we are no longer a builder
        if ( expr->elements.size()==0 ) {
            // empty string builder is "" string
            auto estr = make_smart<ExprConstString>(expr->at,"");
            estr->type = make_smart<TypeDecl>(Type::tString);
            estr->constexpression = true;
            estr->foldedNonConst = !expr->type->constant;
            reportFolding();
            return estr;
        } else if ( expr->elements.size()==1 && expr->elements[0]->rtti_isStringConstant() ) {
            // string builder with one string constant is that constant
            reportFolding();
            ((ExprConstString *)expr->elements[0].get())->foldedNonConst = !expr->type->constant;
            return expr->elements[0];
        } else {
            return expr;
        }
    }

    /*
        op1 constexpr = op1(constexpr)
        op2 constexpr,constexpr = op2(constexpr,constexpr)
        op3 constexpr,a,b = constexpr ? a or b
        op3 cond,a,a = a
        if ( constexpr ) a; else b;    =   constexpr ? a : b
        constexpr; = nop
        assert(true,...) = nop
        hash(x) = ...
        no_side_effec_builtin(const...) -> const
        stringbuilder(const1,const2,...) -> stringbuilder(const12,...)
        stringbuilder(const) -> const
        def FN return const -> const
        invoke(@fn,...) -> fn(...)
        @fn==@fn, @fn!=@fn    -> const bool
        @fn==ptr, @fn!=ptr    -> const bool
     */
    class ConstFolding : public FoldingVisitor {
    public:
        ConstFolding( const ProgramPtr & prog ) : FoldingVisitor(prog) {}
    public:
        vector<Function *> needRun;
    protected:
        // function which is fully a nop
        bool isNop ( const FunctionPtr & func ) {
            if ( func->builtIn ) return false;
            if ( func->body->rtti_isBlock() ) {
                auto block = static_pointer_cast<ExprBlock>(func->body);
                if ( block->list.size()==0 && block->finalList.size() ) {
                    return true;
                }
            }
            return false;
        }
        // function which is 'return const'
        ExpressionPtr getSimpleConst ( const FunctionPtr & func ) {
            if ( func->builtIn ) return nullptr;
            if ( func->userScenario ) return nullptr;
            if ( func->body->rtti_isBlock() ) {
                auto block = static_pointer_cast<ExprBlock>(func->body);
                if ( block->list.size()==1 && block->finalList.size()==0 ) {
                    if ( block->list.back()->rtti_isReturn() ) {
                        auto ret = static_pointer_cast<ExprReturn>(block->list.back());
                        if ( ret->subexpr && ret->subexpr->rtti_isConstant() ) {
                            return cloneWithType(ret->subexpr);
                        }
                    }
                }
            }
            return nullptr;
        }
    protected:
        // virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        // virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        // virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
    // swizzle
        virtual ExpressionPtr visit ( ExprSwizzle * expr ) override {
            if ( expr->type->baseType == expr->value->type->baseType ) {
                if ( expr->fields[0]==0 && TypeDecl::isSequencialMask(expr->fields) ) {
                    if ( !expr->r2v ) {
                        return expr->value;
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // variable
        virtual ExpressionPtr visit ( ExprVar * var ) override {
            if ( var->r2v ) {
                auto variable = var->variable;
                if ( variable && variable->init && variable->type->isConst() && variable->type->isFoldable() ) {
                    if ( !var->local && !var->argument && !var->block ) {
                        if ( variable->init->rtti_isConstant() ) {
                            reportFolding();
                            return cloneWithType(variable->init);
                        }
                    }
                }
            }
            return Visitor::visit(var);
        }
        virtual ExpressionPtr visitLetInit ( ExprLet * expr, const VariablePtr & var, Expression * init ) override {
            if ( init->rtti_isVar() ) {
                auto evar = static_cast<ExprVar *>(init);
                auto variable = evar->variable;
                if ( variable && variable->init && variable->type->isConst() && variable->type->isFoldable() ) {
                    if ( !evar->local && !evar->argument && !evar->block ) {
                        if ( variable->init->rtti_isConstant() ) {
                            reportFolding();
                            return cloneWithType(variable->init);
                        }
                    }
                }
            }
            return Visitor::visitLetInit(expr,var,init);
        }

    // op1
        virtual ExpressionPtr visit ( ExprOp1 * expr ) override {
            if (expr->func->sideEffectFlags) {
                return Visitor::visit(expr);
            }
            if ( expr->subexpr->constexpression && expr->func->builtIn ) {
                if ( expr->type->isFoldable() && expr->subexpr->type->isFoldable() ) {
                    return evalAndFold(expr);
                }
            }
            return Visitor::visit(expr);
        }
    // op2
        virtual ExpressionPtr visit ( ExprOp2 * expr ) override {
            if (expr->func->sideEffectFlags) {
                return Visitor::visit(expr);
            }
            if ( expr->left->constexpression && expr->right->constexpression && expr->func->builtIn ) {
                if ( expr->type->isFoldable() && expr->left->type->isFoldable() && expr->right->type->isFoldable() ) {
                    return evalAndFold(expr);
                }
                if ( expr->type->isSimpleType(Type::tBool) ) {
                    // we are covering
                    //  func == func
                    //  func != func
                    //  func == ptr
                    //  func != ptr
                    if ( expr->left->type->isGoodFunctionType() ) {
                        bool res;
                        auto lfn = static_pointer_cast<ExprAddr>(expr->left);
                        if ( expr->right->type->isGoodFunctionType() ) {
                            auto rfn = static_pointer_cast<ExprAddr>(expr->right);
                            if ( expr->op=="==" ) {
                                res = lfn->func == rfn->func;
                            } else {
                                res = lfn->func != rfn->func;
                            }
                        } else if ( expr->right->type->isPointer() ) {
                            auto rpt = static_pointer_cast<ExprConstPtr>(expr->right);
                            if ( !rpt->getValue() ) {
                                if ( expr->op=="==" ) {
                                    res = false;
                                } else {
                                    res = true;
                                }
                            } else {
                                res = false;
                            }
                        } else {
                            DAS_ASSERTF(0, "how did we even end up here?");
                            return Visitor::visit(expr);
                        }
                        auto sim = Program::makeConst(expr->at, expr->type, cast<bool>::from(res));
                        sim->type = make_smart<TypeDecl>(*expr->type);
                        sim->constexpression = true;
                        reportFolding();
                        return sim;
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // op3
        virtual ExpressionPtr visit ( ExprOp3 * expr ) override {
            if ( expr->type->isFoldable() && expr->subexpr->constexpression && expr->left->constexpression && expr->right->constexpression &&
                (expr->func && expr->func->builtIn) ) {
                return evalAndFold(expr);
            } else if ( expr->type->isFoldable() && expr->subexpr->noSideEffects && expr->left->constexpression && expr->right->constexpression ) {
                bool failed;
                vec4f left = eval(expr->left.get(), failed);
                if ( failed ) return Visitor::visit(expr);
                vec4f right = eval(expr->right.get(), failed);
                if ( failed ) return Visitor::visit(expr);
                if ( isSameFoldValue(expr->type, left, right) ) {
                    reportFolding();
                    return cloneWithType(expr->left);
                }
            } else if ( expr->subexpr->constexpression ) {
                bool failed;
                vec4f resB = eval(expr->subexpr.get(), failed);
                if ( failed ) return Visitor::visit(expr);
                bool res = cast<bool>::to(resB);
                reportFolding();
                return res ? expr->left : expr->right;
            }
            return Visitor::visit(expr);
        }
    // ExprIfThenElse
        virtual ExpressionPtr visit ( ExprIfThenElse * expr ) override {
            if ( expr->cond->constexpression ) {
                bool failed;
                vec4f resB = eval(expr->cond.get(), failed);
                if ( failed ) return Visitor::visit(expr);
                bool res = cast<bool>::to(resB);
                reportFolding();
                return res ? expr->if_true : expr->if_false;
            }
            if ( expr->if_false ) {
                if ( expr->if_false->rtti_isBlock() ) {
                    auto ifeb = static_pointer_cast<ExprBlock>(expr->if_false);
                    if ( !ifeb->list.size() && !ifeb->finalList.size() ) {
                        expr->if_false = nullptr;
                        reportFolding();
                    }
                }
            }
            if ( !expr->if_false ) {
                if ( expr->if_true->rtti_isBlock() ) {
                    auto ifb = static_pointer_cast<ExprBlock>(expr->if_true);
                    if ( !ifb->list.size() && !ifb->finalList.size() ) {
                        if ( expr->cond->noSideEffects ) {
                            reportFolding();
                            return nullptr;
                        } else {
                            reportFolding();
                            return expr->cond;
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // block
        virtual ExpressionPtr visitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            if ( expr->constexpression ) {  // top level constant expresson like 4;
                reportFolding();
                return nullptr;
            }
            if ( expr->noSideEffects ) {    // top level expressions like a + 5;
                reportFolding();
                return nullptr;
            }
            return Visitor::visitBlockExpression(block, expr);
        }
        virtual ExpressionPtr visit ( ExprFor * expr ) override {
            if ( expr->body->rtti_isBlock()) {
                auto block = static_pointer_cast<ExprBlock>(expr->body);
                if ( !block->list.size() && !block->finalList.size() ) {
                    bool noSideEffects = true;
                    for ( auto & src : expr->sources ) {
                        noSideEffects &= src->noSideEffects;
                        if ( !noSideEffects ) break;
                        if ( src->type->isIterator() ) {
                            noSideEffects = false;
                            break;
                        }
                    }
                    if ( noSideEffects ) {
                        reportFolding();
                        return nullptr;
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // const
        virtual ExpressionPtr visit ( ExprConst * c ) override {
            c->constexpression = true;
            return Visitor::visit(c);
        }
    // assert
        virtual ExpressionPtr visit ( ExprAssert * expr ) override {
            if ( expr->arguments[0]->constexpression ) {
                bool failed;
                vec4f resB = eval(expr->arguments[0].get(), failed);
                if ( failed ) return Visitor::visit(expr);
                bool res = cast<bool>::to(resB);
                if ( res ) {
                    reportFolding();
                    return nullptr;
                }
            }
            return Visitor::visit(expr);
        }
    // ExprInvoke
        virtual ExpressionPtr visit ( ExprInvoke * expr ) override {
            auto what = expr->arguments[0];
            if ( what->type->baseType==Type::tFunction && what->rtti_isAddr() ) {
                auto pAddr = static_pointer_cast<ExprAddr>(what);
                auto funcC = pAddr->func;
                auto pCall = make_smart<ExprCall>(expr->at, funcC->name);
                pCall->func = funcC;
                uint32_t numArgs = uint32_t(expr->arguments.size());
                pCall->arguments.reserve(numArgs-1);
                for ( uint32_t i=1; i!=numArgs; ++i ) {
                    pCall->arguments.push_back( cloneWithType(expr->arguments[i]) );
                }
                pCall->type = make_smart<TypeDecl>(*funcC->result);
                reportFolding();
                return pCall;
            }
            return Visitor::visit(expr);
        }
    // ExprCall
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            bool allNoSideEffects = true;
            for ( auto & arg : expr->arguments ) {
                if ( arg->type->baseType!=Type::fakeContext && arg->type->baseType!=Type::fakeLineInfo ) {
                    allNoSideEffects &= arg->noSideEffects;
                }
            }
            if ( allNoSideEffects ) {
                if ( isNop(expr->func) ) {
                    reportFolding();
                    return nullptr;
                }
                if ( auto sc = getSimpleConst(expr->func) ) {
                    reportFolding();
                    return sc;
                }
            }
            if ( expr->func->result->isFoldable() && (expr->func->sideEffectFlags==0) ) {
                auto allConst = true;
                bool canFold = true;
                for ( auto & arg : expr->arguments ) {
                    if ( arg->type->baseType!=Type::fakeContext && arg->type->baseType!=Type::fakeLineInfo ) {
                        allConst &= arg->constexpression;
                        canFold &= arg->type->isFoldable();
                    }
                }
                if ( allConst && canFold ) {
                    if ( expr->func->builtIn ) {
                        return evalAndFold(expr);
                    } else {
                        needRun.push_back(expr->func);
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprStringBuilder
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) override {
            if ( expr->constexpression ) {
                return evalAndFoldString(expr);
            }
            return Visitor::visitStringBuilderElement(sb, expr, last);
        }
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) override {
            return evalAndFoldStringBuilder(expr);
        }
    };

    //  turn static-assert into nop
    //  fail if condition is not there
    class ContractFolding : public FoldingVisitor {
    public:
        ContractFolding( const ProgramPtr & prog ) : FoldingVisitor(prog) {}
    protected:
        FunctionPtr             func;
    protected:
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
        }
        virtual FunctionPtr visit ( Function * that ) override {
            func.reset();
            return Visitor::visit(that);
        }
    // turn static assert into nada or report error
        virtual ExpressionPtr visit(ExprStaticAssert * expr) override {
            auto cond = expr->arguments[0];
            if (!cond->constexpression && !cond->rtti_isConstant()) {
                program->error("static assert condition is not constexpr or const", "","",expr->at);
                return nullptr;
            }
            bool result = false;
            if (cond->constexpression) {
                bool failed;
                vec4f resB = eval(cond.get(), failed);
                if ( failed ) {
                    program->error("exception while computing static assert condition", "","",expr->at);
                }
                result = cast<bool>::to(resB);
            } else {
                result = ((ExprConstBool *)cond.get())->getValue();
            }
            if ( !result ) {
                bool iscf = expr->name=="concept_assert";
                string message;
                if ( expr->arguments.size()==2 ) {
                    bool failed;
                    vec4f resM = eval(expr->arguments[1].get(), failed);
                    if ( failed ) {
                        program->error("exception while computing static assert message","","", expr->at);
                        message = "";
                    } else {
                        message = cast<char *>::to(resM);
                    }
                } else {
                    message = iscf ? "static assert failed" : "concept failed";
                }
                if ( iscf ) {
                    LineInfo atC = expr->at;
                    string extra;
                    if ( func ) {
                        extra = "\nconcept_assert at " + expr->at.describe();
                        extra += func->getLocationExtra();
                        atC = func->getConceptLocation(atC);
                    }
                    program->error(message, extra,"",atC, CompilationError::concept_failed);
                } else {
                    string extra;
                    if ( func ) {
                        extra = func->getLocationExtra();
                    }
                    program->error(message, extra,"",expr->at, CompilationError::static_assert_failed);
                }
            }
            return cond->constexpression ? nullptr : Visitor::visit(expr);
        }
    // verify 'run'
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->func && expr->func->hasToRunAtCompileTime ) {
                if ( expr->func->sideEffectFlags ) {
                    program->error("function did not run at compilation time because it has side-effects","","",
                                   expr->at, CompilationError::run_failed);
                } else {
                    program->error("function did not run at compilation time","","",
                                   expr->at, CompilationError::run_failed);
                }
            }
        }
    };

    class RunFolding : public FoldingVisitor {
    public:
        RunFolding( const ProgramPtr & prog, const vector<Function *> & _needRun ) : FoldingVisitor(prog),
            runProgram(prog.get()), needRun(_needRun) {
        }
    protected:
        Program * runProgram = nullptr;
        const vector<Function *> & needRun;
        bool anySimulated = false;
    protected:
        // ExprCall
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            if ( expr->func->result->isFoldable() && (expr->func->sideEffectFlags==0) && !expr->func->builtIn ) {
                auto allConst = true;
                for ( auto & arg : expr->arguments ) {
                    if ( arg->type->baseType!=Type::fakeContext && arg->type->baseType!=Type::fakeLineInfo ) {
                        allConst &= arg->constexpression;
                    }
                }
                if ( allConst ) {
                    if ( !anySimulated ) {          // the reason for lazy simulation is that function can be optimized out during the same pass as it was marked for folding
                        anySimulated = true;
                        TextWriter dummy;
                        runProgram->folding = true;
                        runProgram->markFoldingSymbolUse(needRun);
                        DAS_ASSERTF ( !runProgram->failed(), "internal error while folding (remove unused)?" );
                        runProgram->deriveAliases(dummy,false,false);
                        DAS_ASSERTF ( !runProgram->failed(), "internal error while folding (derive aliases)?" );
                        runProgram->allocateStack(dummy,false,false);
                        DAS_ASSERTF ( !runProgram->failed(), "internal error while folding (allocate stack)?" );
                        runProgram->updateSemanticHash();
                        runProgram->simulate(ctx, dummy);
                        DAS_ASSERTF ( !runProgram->failed(), "internal error while folding (simulate)?" );
                        runProgram->folding = false;
                    }
                    if ( expr->func->index==-1 ) {
                        runProgram->error("internal compilation error, folding symbol was not marked as used","","",
                            expr->at, CompilationError::run_failed);
                        return Visitor::visit(expr);
                    }
                    return evalAndFold(expr);
                }
            }
            return Visitor::visit(expr);
        }
    };

    // program

    void Program::checkSideEffects() {
        SetSideEffectVisitor sse;
        visit(sse);
        NoSideEffectVisitor nse;
        visit(nse);
    }

    bool Program::optimizationConstFolding() {
        checkSideEffects();
        ConstFolding cfe(this);
        visit(cfe);
        bool any = cfe.didAnything();
        if ( !cfe.needRun.empty() ) {
            if ( !options.getBoolOption("disable_run",false) ) {
                RunFolding rfe(this,cfe.needRun);
                visit(rfe);
                any |= rfe.didAnything();
            }
        }
        return any;
    }

    bool Program::verifyAndFoldContracts() {
        ContractFolding context(this);
        visit(context);
        return context.didAnything();
    }
}

