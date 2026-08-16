#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_simulate.h"
#include "daScript/simulate/debug_print.h"

namespace das {
    // A C++-bound function must not be folded: see BuiltInFunction::noFolding.
    static bool foldableBuiltin ( const Function * fn ) {
        if ( !fn || !fn->builtIn ) return false;
        return !static_cast<const BuiltInFunction *>(fn)->noFolding;
    }



    void PassVisitor::preVisitProgram ( Program * prog ) {
        Visitor::preVisitProgram(prog);
        program = prog;
        anyFolding = false;
    }

    void PassVisitor::visitProgram ( Program * prog ) {
        program = nullptr;
        Visitor::visitProgram(prog);
    }

    void PassVisitor::preVisit ( Function * f ) {
        Visitor::preVisit(f);
        func = f;
    }

    FunctionPtr PassVisitor::visit ( Function * f ) {
        func = nullptr;
        return Visitor::visit(f);
    }

    void PassVisitor::reportFolding() {
        anyFolding = true;
        if (func) {
            // Mark the function as dirty for the current and
            // upcoming optimization pass.
            // max() so a pass constructed with a lower round (or a
            // non-optimizer pass with round 0) can never downgrade a
            // dirty marker another pass already stamped this round.
            func->optimizationRound = das::max(func->optimizationRound, round);
        }
    }

    class SetSideEffectVisitor : public Visitor {
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        // any expression
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            expr->noSideEffects = false;
            expr->noNativeSideEffects = false;
        }
    };

    class NoSideEffectVisitor : public Visitor {
    public:
        explicit NoSideEffectVisitor ( Program * prog ) : program(prog) {}
    protected:
        Program * program = nullptr;
        // report an internal compiler error. a resolved ExprCall / ExprOpN
        // must have a non-null func by this stage; reaching here means the
        // call was produced after type inference and infer was never re-run
        // on it. common causes:
        //  - a structure or function annotation patch() mutated the AST but did
        //    not set astChanged=true, so the parse loop did not re-infer;
        //  - a pass / optimization macro modified the AST and returned false from
        //    apply(), suppressing the re-infer cycle;
        //  - a substitution macro (call / reader / typeinfo / variant / type / etc.)
        //    produced a node, but the call site forgot to forward it as a substitute
        //    so type inference never sees it.
        // we report the diagnostic (which sets failToCompile) and then let the
        // caller fall through to Visitor::visit so child traversal continues.
        // skipping the func-deref code path keeps us safe within this pass; the
        // noSideEffects/noNativeSideEffects flags retain their conservative
        // default of false set by SetSideEffectVisitor::preVisitExpression, and
        // ast_parse.cpp now gates downstream pipeline stages (foldUnsafe /
        // optimize / buildAccessFlags / verifyAndFoldContracts) on !failed()
        // so the broken AST is not walked again.
        void reportNullFunc ( Expression * expr, const char * exprKind ) {
            program->error("internal compilation error, " + string(exprKind) +
                " reached side-effect analysis with unresolved func",
                "this typically means the AST was modified after type inference "
                "without signalling that infer needs to run again - check the "
                "annotation patch() / pass apply() / substitution macro that produced this node",
                "", expr->at, CompilationError::internal_function_not_resolved_yet);
        }
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructure ( Structure * st ) override {
            return !st->isTemplate;    // we don't do a thing with templates
        }
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
            if ( expr->subexpr->type->isHandle() && expr->subexpr->type->annotation->isIndexMutable(expr->index->type) ) {
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
            if ( !expr->func ) { reportNullFunc(expr, "op1"); return Visitor::visit(expr); }
            expr->noSideEffects = expr->subexpr->noSideEffects && (expr->func->sideEffectFlags==0);
            expr->noNativeSideEffects = expr->subexpr->noNativeSideEffects
                && ((expr->func->sideEffectFlags & uint32_t(SideEffects::inferredSideEffects))==0);
            return Visitor::visit(expr);
        }
    // op2
        virtual ExpressionPtr visit ( ExprOp2 * expr ) override {
            if ( !expr->func ) { reportNullFunc(expr, "op2"); return Visitor::visit(expr); }
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
            if ( !expr->func ) { reportNullFunc(expr, "call"); return Visitor::visit(expr); }
            uint32_t sef = expr->func->sideEffectFlags;
            expr->noSideEffects = (sef==0);
            // A call whose ONLY effect is modifyArgument, and whose every argument is a freshly-built
            // make-local rvalue, is side-effect-free at this site: the writes land on temporaries the
            // expression itself constructs and nothing else can observe. The function keeps its
            // modifyArgument flag for callers that pass real lvalues. This is what makes move-through
            // wrappers over array/table literals (to_array_move / to_table_move) pure.
            if ( !expr->noSideEffects && !expr->arguments.empty()
                    && (sef & ~uint32_t(SideEffects::modifyArgument))==0 ) {
                bool allTemp = true;
                for ( auto & a : expr->arguments ) {
                    if ( !a->rtti_isMakeLocal() ) { allTemp = false; break; }
                }
                expr->noSideEffects = allTemp;
            }
            expr->noNativeSideEffects = (sef & uint32_t(SideEffects::inferredSideEffects))==0;
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
        auto node = simulateExpression(ctx, expr);
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
        if ( expr->rtti_isConstant() ) return expr->clone();
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
                case Type::tBitfield8:  ival = cast<uint8_t>::to(value); break;
                case Type::tBitfield16: ival = cast<uint16_t>::to(value); break;
                case Type::tBitfield64: ival = cast<uint64_t>::to(value); break;
                case Type::tInt64:      ival = cast<int64_t>::to(value); break;
                case Type::tUInt64:     ival = cast<uint64_t>::to(value); break;
                default: DAS_ASSERTF(0,"we should not be here. unsupported enum type");
                }
                auto cef = expr->type->enumType->find(ival, "");
                if ( cef.empty() ) return expr; // it folded to unsupported value
                auto sim = new ExprConstEnumeration(expr->at, cef, expr->type);
                sim->type = expr->type->enumType->makeEnumType();
                sim->constexpression = true;
                sim->at = encloseAt(expr);
                sim->value = value;
                sim->foldedNonConst = !expr->type->constant;
                reportFolding();
                return sim;
            } else if ( expr->type->isBitfield() ) {
                uint64_t ival = 0;
                switch ( expr->type->baseType ) {
                case Type::tBitfield8:  ival = cast<uint8_t>::to(value); break;
                case Type::tBitfield16: ival = cast<uint16_t>::to(value); break;
                case Type::tBitfield:   ival = cast<uint32_t>::to(value); break;
                case Type::tBitfield64: ival = cast<uint64_t>::to(value); break;
                default: DAS_ASSERTF(0,"we should not be here. unsupported bitfield type");
                }
                auto sim = new ExprConstBitfield(expr->at, ival);
                sim->type = new TypeDecl(*expr->type);
                sim->constexpression = true;
                sim->at = encloseAt(expr);
                sim->foldedNonConst = !expr->type->constant;
                sim->baseType = expr->type->baseType;
                sim->bitfieldType = new TypeDecl(*expr->type);
                reportFolding();
                return sim;
            } else {
                auto wasRef = expr->type->ref;
                expr->type->ref = false;
                auto sim = Program::makeConst(expr->at, expr->type, value);
                expr->type->ref = wasRef;
                if ( !sim ) return expr;    // no const node for this type (lattice vectors) — don't fold
                sim->type = new TypeDecl(*expr->type);
                sim->constexpression = true;
                ((ExprConst *)sim)->foldedNonConst = !expr->type->constant;
                sim->at = encloseAt(expr);
                reportFolding();
                return sim;
            }
        } else {
            return expr;
        }
    }

    ExpressionPtr FoldingVisitor::evalAndFoldString ( Expression * expr ) {
        if ( expr->rtti_isStringConstant() ) return expr->clone();
        bool failed;
        vec4f value = eval(expr, failed);
        if ( !failed ) {
            auto b4ref = expr->type->ref;
            expr->type->ref = false;
            TypeInfo * pTypeInfo = helper.makeTypeInfo(nullptr,expr->type);
            expr->type->ref = b4ref;
            auto res = debug_value(value, pTypeInfo, PrintFlags::string_builder);
            auto sim = new ExprConstString(expr->at, res);
            sim->type = new TypeDecl(Type::tString);
            sim->constexpression = true;
            sim->foldedNonConst = !expr->type->constant;
            sim->at = encloseAt(expr);
            reportFolding();
            return sim;
        } else {
            return expr;
        }
    }

    ExpressionPtr FoldingVisitor::cloneWithType ( ExpressionPtr expr ) {
        auto rexpr = expr->clone();
        if ( expr->type ) rexpr->type = new TypeDecl(*expr->type);
        return rexpr;
    }

    ExpressionPtr FoldingVisitor::evalAndFoldStringBuilder ( ExprStringBuilder * expr )  {
        // concatinate all constant strings, which are close together
        ExprConstString * str = nullptr;
        for ( auto it=expr->elements.begin(); it != expr->elements.end(); ) { // note - loop has erase, don't store 'end'
            auto & elem = *it;
            if ( elem->rtti_isStringConstant() ) {
                auto selem = static_cast<ExprConstString*>(elem);
                if ( str ) {
                    str->text += selem->text;
                    it = expr->elements.erase(it);
                    reportFolding();
                } else {
                    str = static_cast<ExprConstString*>(cloneWithType(selem));
                    elem = str;
                    ++ it;
                }
            } else {
                str = nullptr;
                ++ it;
            }
        }
        // check if we are no longer a builder
        if ( expr->elements.size()==0 ) {
            // empty string builder is "" string
            auto estr = new ExprConstString(expr->at,"");
            estr->type = new TypeDecl(Type::tString);
            estr->constexpression = true;
            estr->foldedNonConst = !expr->type->constant;
            reportFolding();
            return estr;
        } else if ( expr->elements.size()==1 && expr->elements[0]->rtti_isStringConstant() ) {
            // string builder with one string constant is that constant
            reportFolding();
            ((ExprConstString *)expr->elements[0])->foldedNonConst = !expr->type->constant;
            return expr->elements[0];
        } else {
            return expr;
        }
    }

    // ===== algebraic identity folding helpers =====

    static bool isFloat32Family ( Type bt ) {
        return bt==Type::tFloat || bt==Type::tFloat2 || bt==Type::tFloat3 || bt==Type::tFloat4;
    }

    static bool isFPFamily ( Type bt ) {
        return isFloat32Family(bt) || bt==Type::tDouble;
    }

    static bool isSignedNumericFamily ( Type bt ) {
        switch ( bt ) {
        case Type::tInt: case Type::tInt8: case Type::tInt16: case Type::tInt64:
        case Type::tInt2: case Type::tInt3: case Type::tInt4:
        case Type::tFloat: case Type::tFloat2: case Type::tFloat3: case Type::tFloat4:
        case Type::tDouble:
            return true;
        default:
            return false;
        }
    }

    static bool isIntegerFamily ( Type bt ) {
        switch ( bt ) {
        case Type::tInt: case Type::tInt8: case Type::tInt16: case Type::tInt64:
        case Type::tUInt: case Type::tUInt8: case Type::tUInt16: case Type::tUInt64:
        case Type::tInt2: case Type::tInt3: case Type::tInt4:
        case Type::tUInt2: case Type::tUInt3: case Type::tUInt4:
            return true;
        default:
            return false;
        }
    }

    static bool isNumericFamily ( Type bt ) {
        return isIntegerFamily(bt) || isFPFamily(bt);
    }

    // every lane of a numeric constant equals `v` (v is small and exactly representable in every type tested)
    static bool constEquals ( Expression * e, double v ) {
        if ( !e || !e->rtti_isConstant() || !e->type || e->type->ref ) return false;
        auto c = static_cast<ExprConst *>(e);
        switch ( e->type->baseType ) {
        case Type::tInt:     return cast<int32_t>::to(c->value)==int32_t(v);
        case Type::tInt8:    return cast<int8_t>::to(c->value)==int8_t(v);
        case Type::tInt16:   return cast<int16_t>::to(c->value)==int16_t(v);
        case Type::tInt64:   return cast<int64_t>::to(c->value)==int64_t(v);
        case Type::tUInt:    return v>=0 && cast<uint32_t>::to(c->value)==uint32_t(v);
        case Type::tUInt8:   return v>=0 && cast<uint8_t>::to(c->value)==uint8_t(v);
        case Type::tUInt16:  return v>=0 && cast<uint16_t>::to(c->value)==uint16_t(v);
        case Type::tUInt64:  return v>=0 && cast<uint64_t>::to(c->value)==uint64_t(v);
        case Type::tFloat:   return cast<float>::to(c->value)==float(v);
        case Type::tDouble:  return cast<double>::to(c->value)==double(v);
        case Type::tInt2:    { auto t = cast<int2>::to(c->value);   return t.x==int32_t(v) && t.y==int32_t(v); }
        case Type::tInt3:    { auto t = cast<int3>::to(c->value);   return t.x==int32_t(v) && t.y==int32_t(v) && t.z==int32_t(v); }
        case Type::tInt4:    { auto t = cast<int4>::to(c->value);   return t.x==int32_t(v) && t.y==int32_t(v) && t.z==int32_t(v) && t.w==int32_t(v); }
        case Type::tUInt2:   { if ( v<0 ) return false; auto t = cast<uint2>::to(c->value);  return t.x==uint32_t(v) && t.y==uint32_t(v); }
        case Type::tUInt3:   { if ( v<0 ) return false; auto t = cast<uint3>::to(c->value);  return t.x==uint32_t(v) && t.y==uint32_t(v) && t.z==uint32_t(v); }
        case Type::tUInt4:   { if ( v<0 ) return false; auto t = cast<uint4>::to(c->value);  return t.x==uint32_t(v) && t.y==uint32_t(v) && t.z==uint32_t(v) && t.w==uint32_t(v); }
        case Type::tFloat2:  { auto t = cast<float2>::to(c->value); return t.x==float(v) && t.y==float(v); }
        case Type::tFloat3:  { auto t = cast<float3>::to(c->value); return t.x==float(v) && t.y==float(v) && t.z==float(v); }
        case Type::tFloat4:  { auto t = cast<float4>::to(c->value); return t.x==float(v) && t.y==float(v) && t.z==float(v) && t.w==float(v); }
        default: return false;
        }
    }

    static bool isPosZeroBits ( float f )  { uint32_t u; memcpy(&u,&f,sizeof(u)); return u==0u; }
    static bool isPosZeroBits ( double d ) { uint64_t u; memcpy(&u,&d,sizeof(u)); return u==0ull; }
    static bool isNegZeroBits ( float f )  { uint32_t u; memcpy(&u,&f,sizeof(u)); return u==0x80000000u; }
    static bool isNegZeroBits ( double d ) { uint64_t u; memcpy(&u,&d,sizeof(u)); return u==0x8000000000000000ull; }

    // every lane is +0.0 exactly (integer zero also qualifies)
    static bool constAllPosZero ( Expression * e ) {
        if ( !constEquals(e, 0) ) return false;
        auto c = static_cast<ExprConst *>(e);
        switch ( e->type->baseType ) {
        case Type::tFloat:   return isPosZeroBits(cast<float>::to(c->value));
        case Type::tDouble:  return isPosZeroBits(cast<double>::to(c->value));
        case Type::tFloat2:  { auto t = cast<float2>::to(c->value); return isPosZeroBits(t.x) && isPosZeroBits(t.y); }
        case Type::tFloat3:  { auto t = cast<float3>::to(c->value); return isPosZeroBits(t.x) && isPosZeroBits(t.y) && isPosZeroBits(t.z); }
        case Type::tFloat4:  { auto t = cast<float4>::to(c->value); return isPosZeroBits(t.x) && isPosZeroBits(t.y) && isPosZeroBits(t.z) && isPosZeroBits(t.w); }
        default: return true;
        }
    }

    // every lane is -0.0 exactly (never true for integers)
    static bool constAllNegZero ( Expression * e ) {
        if ( !constEquals(e, 0) ) return false;
        auto c = static_cast<ExprConst *>(e);
        switch ( e->type->baseType ) {
        case Type::tFloat:   return isNegZeroBits(cast<float>::to(c->value));
        case Type::tDouble:  return isNegZeroBits(cast<double>::to(c->value));
        case Type::tFloat2:  { auto t = cast<float2>::to(c->value); return isNegZeroBits(t.x) && isNegZeroBits(t.y); }
        case Type::tFloat3:  { auto t = cast<float3>::to(c->value); return isNegZeroBits(t.x) && isNegZeroBits(t.y) && isNegZeroBits(t.z); }
        case Type::tFloat4:  { auto t = cast<float4>::to(c->value); return isNegZeroBits(t.x) && isNegZeroBits(t.y) && isNegZeroBits(t.z) && isNegZeroBits(t.w); }
        default: return false;
        }
    }

    static bool isConstBoolValue ( Expression * e, bool v ) {
        if ( !e || !e->rtti_isConstant() || !e->type || e->type->baseType!=Type::tBool ) return false;
        return static_cast<ExprConstBool *>(e)->getValue()==v;
    }

    // both subtrees are the same pure computation: structural equality
    // (Expression::sameAs) plus side-effect freedom, so dropping or deduplicating an
    // evaluation cannot lose observable work. `E op E` folds route through here.
    static bool samePureValue ( Expression * a, Expression * b ) {
        return a && b && a->noSideEffects && a->sameAs(b);
    }

    // inf-safe finiteness check without <cmath>: inf-inf and NaN-NaN are NaN
    static bool finiteFP ( float f )  { return f-f==0.0f; }
    static bool finiteFP ( double d ) { return d-d==0.0; }

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
        using PassVisitor::visit;
        ConstFolding( const ProgramPtr & prog, int32_t round ) : FoldingVisitor(prog, round) {
            fastMath = prog->options.getBoolOption("fast_math", prog->policies.fast_math);
        }
    public:
        vector<Function *> needRun;
    protected:
        bool fastMath = false;
        // sign-of-zero-only bit differences (x+0, 0-x): float32 is not bit-exact across das
        // tiers so they are always fair game; double IS bit-exact and needs fast_math; int is exact
        bool zeroSignLaxOk ( Type bt ) const {
            return bt==Type::tDouble ? fastMath : true;
        }
        // folds that change values for inf/NaN inputs (x*0, x-x): fast_math only for FP
        bool valueChangeOk ( Type bt ) const {
            return isFPFamily(bt) ? fastMath : true;
        }
        // builtin, side-effect-free operator with these exact argument types (t1 null → unary)
        Function * findBuiltinOperator ( const char * opName, TypeDecl * t0, TypeDecl * t1 = nullptr ) {
            if ( !t0 ) return nullptr;
            Function * result = nullptr;
            uint64_t hName = hash64z(opName);
            size_t nArgs = t1 ? 2 : 1;
            program->library.foreach([&](Module * mod) -> bool {
                if ( auto kv = mod->functionsByName.find(hName) ) {
                    for ( auto fn : kv->second ) {
                        if ( !fn->builtIn || fn->sideEffectFlags || fn->isTemplate ) continue;
                        if ( fn->arguments.size()!=nArgs ) continue;
                        if ( !fn->arguments[0]->type->isSameType(*t0, RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) continue;
                        if ( t1 && !fn->arguments[1]->type->isSameType(*t1, RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) continue;
                        result = fn;
                        return false;
                    }
                }
                return true;
            }, "*");
            return result;
        }
        ExpressionPtr makeZeroConst ( Expression * expr ) {
            auto sim = Program::makeConst(expr->at, expr->type, v_zero());
            if ( !sim ) return nullptr;
            sim->type = new TypeDecl(*expr->type);
            sim->type->ref = false;
            sim->constexpression = true;
            ((ExprConst *)sim)->foldedNonConst = !expr->type->constant;
            return sim;
        }
        ExpressionPtr makeNegate ( Expression * host, Expression * sub ) {
            auto negFn = findBuiltinOperator("-", sub->type);
            if ( !negFn ) return nullptr;
            auto neg = new ExprOp1(host->at, "-", sub);
            neg->func = negFn;
            neg->type = new TypeDecl(*host->type);
            neg->type->ref = false;
            neg->type->constant = false;
            return neg;
        }
        // 1/C for an FP constant; null when any lane is zero or the reciprocal is not finite
        ExpressionPtr makeReciprocalConst ( Expression * c ) {
            auto cc = static_cast<ExprConst *>(c);
            vec4f rv = v_zero();
            switch ( c->type->baseType ) {
            case Type::tFloat: {
                float f = cast<float>::to(cc->value);
                float r = 1.0f / f;
                if ( f==0.0f || !finiteFP(r) ) return nullptr;
                rv = cast<float>::from(r);
                break;
            }
            case Type::tDouble: {
                double d = cast<double>::to(cc->value);
                double r = 1.0 / d;
                if ( d==0.0 || !finiteFP(r) ) return nullptr;
                rv = cast<double>::from(r);
                break;
            }
            case Type::tFloat2: {
                auto t = cast<float2>::to(cc->value);
                if ( t.x==0.0f || t.y==0.0f ) return nullptr;
                float2 r; r.x = 1.0f/t.x; r.y = 1.0f/t.y;
                if ( !finiteFP(r.x) || !finiteFP(r.y) ) return nullptr;
                rv = cast<float2>::from(r);
                break;
            }
            case Type::tFloat3: {
                auto t = cast<float3>::to(cc->value);
                if ( t.x==0.0f || t.y==0.0f || t.z==0.0f ) return nullptr;
                float3 r; r.x = 1.0f/t.x; r.y = 1.0f/t.y; r.z = 1.0f/t.z;
                if ( !finiteFP(r.x) || !finiteFP(r.y) || !finiteFP(r.z) ) return nullptr;
                rv = cast<float3>::from(r);
                break;
            }
            case Type::tFloat4: {
                auto t = cast<float4>::to(cc->value);
                if ( t.x==0.0f || t.y==0.0f || t.z==0.0f || t.w==0.0f ) return nullptr;
                float4 r; r.x = 1.0f/t.x; r.y = 1.0f/t.y; r.z = 1.0f/t.z; r.w = 1.0f/t.w;
                if ( !finiteFP(r.x) || !finiteFP(r.y) || !finiteFP(r.z) || !finiteFP(r.w) ) return nullptr;
                rv = cast<float4>::from(r);
                break;
            }
            default: return nullptr;
            }
            auto sim = Program::makeConst(c->at, c->type, rv);
            if ( !sim ) return nullptr;
            sim->type = new TypeDecl(*c->type);
            sim->type->ref = false;
            sim->constexpression = true;
            return sim;
        }
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate && funcIsDirty(fun);    // we don't do a thing with templates
        }
        virtual bool canVisitStructure ( Structure * st ) override {
            return !st->isTemplate;    // we don't do a thing with templates
        }
        // function which is fully a nop. finalizers count as content — a body
        // that is nothing but finalList (e.g. a lone `defer { ... }`) still runs
        // observable code, so deleting the call would miscompile. declared side
        // effects count too: emission stubs (glsl `_discard()` etc) have empty
        // das bodies on purpose and the call itself is the payload
        bool isNop ( const FunctionPtr & func ) {
            if ( func->builtIn ) return false;
            if ( func->sideEffectFlags ) return false;
            if ( func->body->rtti_isBlock() ) {
                auto block = static_cast<ExprBlock*>(func->body);
                if ( block->list.size()==0 && block->finalList.size()==0 ) {
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
                auto block = static_cast<ExprBlock*>(func->body);
                if ( block->list.size()==1 && block->finalList.size()==0 ) {
                    if ( block->list.back()->rtti_isReturn() ) {
                        auto ret = static_cast<ExprReturn*>(block->list.back());
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
            if ( expr->subexpr->constexpression && foldableBuiltin(expr->func) ) {
                if ( expr->type->isFoldable() && expr->subexpr->type->isFoldable() ) {
                    return evalAndFold(expr);
                }
            }
            if ( expr->func->builtIn ) {
                // !!x → x ; -(-x) → x (bit-exact for FP too: negate is a sign flip)
                if ( (expr->op=="!" || expr->op=="-") && expr->subexpr->rtti_isOp1() ) {
                    auto inner = static_cast<ExprOp1 *>(expr->subexpr);
                    const bool involutive = inner->subexpr->type &&
                        ( (expr->op=="!" && inner->subexpr->type->isSimpleType(Type::tBool))
                       || (expr->op=="-" && inner->subexpr->type->isNumeric()) );
                    if ( inner->op==expr->op && inner->func && inner->func->builtIn && !inner->func->sideEffectFlags
                        && involutive ) {
                        reportFolding();
                        return inner->subexpr;
                    }
                }
                // !(a<b) → a>=b etc. FP compares change NaN behavior → fast_math
                if ( expr->op=="!" && expr->subexpr->rtti_isOp2() ) {
                    auto cmp = static_cast<ExprOp2 *>(expr->subexpr);
                    if ( cmp->func && cmp->func->builtIn && !cmp->func->sideEffectFlags && cmp->type && cmp->type->isSimpleType(Type::tBool) ) {
                        const char * flip = nullptr;
                        if ( cmp->op=="==" ) flip = "!=";
                        else if ( cmp->op=="!=" ) flip = "==";
                        else if ( cmp->op=="<" )  flip = ">=";
                        else if ( cmp->op==">=" ) flip = "<";
                        else if ( cmp->op==">" )  flip = "<=";
                        else if ( cmp->op=="<=" ) flip = ">";
                        if ( flip ) {
                            bool fpCompare = ( cmp->left->type && isFPFamily(cmp->left->type->baseType) )
                                || ( cmp->right->type && isFPFamily(cmp->right->type->baseType) );
                            if ( !fpCompare || fastMath ) {
                                if ( auto flipFn = findBuiltinOperator(flip, cmp->left->type, cmp->right->type) ) {
                                    cmp->op = flip;
                                    cmp->func = flipFn;
                                    reportFolding();
                                    return cmp;
                                }
                            }
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // op2
        virtual ExpressionPtr visit ( ExprOp2 * expr ) override {
            if (expr->func->sideEffectFlags) {
                return Visitor::visit(expr);
            }
            if ( expr->left->constexpression && expr->right->constexpression && foldableBuiltin(expr->func) ) {
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
                        auto lfn = static_cast<ExprAddr*>(expr->left);
                        if ( expr->right->type->isGoodFunctionType() ) {
                            auto rfn = static_cast<ExprAddr*>(expr->right);
                            if ( expr->op=="==" ) {
                                res = lfn->func == rfn->func;
                            } else {
                                res = lfn->func != rfn->func;
                            }
                        } else if ( expr->right->type->isPointer() ) {
                            auto rpt = static_cast<ExprConstPtr*>(expr->right);
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
                        sim->type = new TypeDecl(*expr->type);
                        sim->constexpression = true;
                        reportFolding();
                        return sim;
                    }
                }
            }
            // algebraic identities. only for the builtin operators on basic types; laxity per fold:
            //   exact           - always on (bit-identical for every input)
            //   zeroSignLaxOk   - sign-of-zero-only difference (float32 always, double under fast_math)
            //   valueChangeOk   - inf/NaN results change (FP under fast_math only)
            if ( expr->func->builtIn && expr->type && !expr->type->ref ) {
                const auto bt = expr->type->baseType;
                auto L = expr->left;
                auto R = expr->right;
                const auto lbt = L->type ? L->type->baseType : Type::none;
                const auto rbt = R->type ? R->type->baseType : Type::none;
                if ( expr->op=="*" && isNumericFamily(bt) ) {
                    if ( constEquals(R,1) && lbt==bt ) { reportFolding(); return L; }
                    if ( constEquals(L,1) && rbt==bt ) { reportFolding(); return R; }
                    if ( constEquals(R,-1) && lbt==bt && isSignedNumericFamily(bt) ) {
                        if ( auto neg = makeNegate(expr, L) ) { reportFolding(); return neg; }
                    }
                    if ( constEquals(L,-1) && rbt==bt && isSignedNumericFamily(bt) ) {
                        if ( auto neg = makeNegate(expr, R) ) { reportFolding(); return neg; }
                    }
                    if ( ( (constEquals(R,0) && L->noSideEffects) || (constEquals(L,0) && R->noSideEffects) ) && valueChangeOk(bt) ) {
                        if ( auto z = makeZeroConst(expr) ) { reportFolding(); return z; }
                    }
                } else if ( expr->op=="+" && isNumericFamily(bt) ) {
                    // x + (-0.0) and integer x + 0 are exact; x + (+0.0) flips a -0.0 input's sign
                    if ( lbt==bt && constEquals(R,0) && ( constAllNegZero(R) || zeroSignLaxOk(bt) ) ) { reportFolding(); return L; }
                    if ( rbt==bt && constEquals(L,0) && ( constAllNegZero(L) || zeroSignLaxOk(bt) ) ) { reportFolding(); return R; }
                } else if ( expr->op=="-" && isNumericFamily(bt) ) {
                    // x - (+0.0) and integer x - 0 are exact; x - (-0.0) flips a -0.0 input's sign
                    if ( lbt==bt && constEquals(R,0) && ( constAllPosZero(R) || zeroSignLaxOk(bt) ) ) { reportFolding(); return L; }
                    // 0 - x → -x: exact when the zero is -0.0, sign-of-zero lax when +0.0
                    if ( rbt==bt && constEquals(L,0) && isSignedNumericFamily(bt) && ( constAllNegZero(L) || zeroSignLaxOk(bt) ) ) {
                        if ( auto neg = makeNegate(expr, R) ) { reportFolding(); return neg; }
                    }
                    // x - x → 0: changes inf/NaN results → fast_math for FP
                    if ( samePureValue(L, R) && valueChangeOk(bt) ) {
                        if ( auto z = makeZeroConst(expr) ) { reportFolding(); return z; }
                    }
                } else if ( expr->op=="/" && isNumericFamily(bt) ) {
                    if ( lbt==bt && constEquals(R,1) ) { reportFolding(); return L; }
                    // x / C → x * (1/C): the reciprocal rounds → fast_math, FP only
                    if ( fastMath && isFPFamily(bt) && R->rtti_isConstant() ) {
                        if ( auto rec = makeReciprocalConst(R) ) {
                            if ( auto mulFn = findBuiltinOperator("*", L->type, rec->type) ) {
                                expr->op = "*";
                                expr->func = mulFn;
                                expr->right = rec;
                                reportFolding();
                                return Visitor::visit(expr);
                            }
                        }
                    }
                } else if ( ( expr->op=="&" || expr->op=="|" || expr->op=="^" ) && isIntegerFamily(bt) ) {
                    const bool zr = constEquals(R,0);
                    const bool zl = constEquals(L,0);
                    if ( expr->op=="&" ) {
                        if ( (zr && L->noSideEffects) || (zl && R->noSideEffects) ) {
                            if ( auto z = makeZeroConst(expr) ) { reportFolding(); return z; }
                        }
                        if ( samePureValue(L,R) ) { reportFolding(); return L; }
                    } else if ( expr->op=="|" ) {
                        if ( zr && lbt==bt ) { reportFolding(); return L; }
                        if ( zl && rbt==bt ) { reportFolding(); return R; }
                        if ( samePureValue(L,R) ) { reportFolding(); return L; }
                    } else {
                        if ( zr && lbt==bt ) { reportFolding(); return L; }
                        if ( zl && rbt==bt ) { reportFolding(); return R; }
                        if ( samePureValue(L,R) ) {
                            if ( auto z = makeZeroConst(expr) ) { reportFolding(); return z; }
                        }
                    }
                } else if ( ( expr->op=="<<" || expr->op==">>" || expr->op=="<<<" || expr->op==">>>" ) && isIntegerFamily(bt) ) {
                    if ( constEquals(R,0) && lbt==bt ) { reportFolding(); return L; }
                } else if ( expr->op=="%" && isIntegerFamily(bt) ) {
                    if ( constEquals(R,1) && L->noSideEffects ) {
                        if ( auto z = makeZeroConst(expr) ) { reportFolding(); return z; }
                    }
                } else if ( expr->op=="&&" && bt==Type::tBool ) {
                    // short-circuit: a const-false LEFT never evaluates the right, so those drops
                    // are unconditional; dropping an evaluated left needs purity
                    if ( isConstBoolValue(L,true) ) { reportFolding(); return R; }
                    if ( isConstBoolValue(L,false) ) { reportFolding(); return L; }
                    if ( isConstBoolValue(R,true) ) { reportFolding(); return L; }
                    if ( isConstBoolValue(R,false) && L->noSideEffects ) { reportFolding(); return R; }
                } else if ( expr->op=="||" && bt==Type::tBool ) {
                    if ( isConstBoolValue(L,false) ) { reportFolding(); return R; }
                    if ( isConstBoolValue(L,true) ) { reportFolding(); return L; }
                    if ( isConstBoolValue(R,false) ) { reportFolding(); return L; }
                    if ( isConstBoolValue(R,true) && L->noSideEffects ) { reportFolding(); return R; }
                }
                // v * floatN(s) → v * s ; v / floatN(s) → v / s (splat collapse, per-lane bit-identical)
                if ( ( expr->op=="*" || expr->op=="/" ) && isFloat32Family(bt) && bt!=Type::tFloat ) {
                    auto trySplat = [&]( Expression * e ) -> Expression * {
                        if ( !e || !e->rtti_isCall() ) return nullptr;
                        auto call = static_cast<ExprCall *>(e);
                        if ( !call->func || !call->func->builtIn || call->func->sideEffectFlags ) return nullptr;
                        if ( call->arguments.size()!=1 ) return nullptr;
                        auto & a0 = call->arguments[0];
                        if ( !a0->type || a0->type->baseType!=Type::tFloat || a0->type->ref ) return nullptr;
                        if ( call->func->name!=das_to_string(bt) ) return nullptr;
                        return a0;
                    };
                    if ( auto s = trySplat(expr->right) ) {
                        if ( auto fn = findBuiltinOperator(expr->op.c_str(), L->type, s->type) ) {
                            expr->right = s;
                            expr->func = fn;
                            reportFolding();
                            return Visitor::visit(expr);
                        }
                    }
                    if ( expr->op=="*" ) {
                        if ( auto s = trySplat(expr->left) ) {
                            if ( auto fn = findBuiltinOperator("*", R->type, s->type) ) {
                                expr->left = R;
                                expr->right = s;
                                expr->func = fn;
                                reportFolding();
                                return Visitor::visit(expr);
                            }
                        }
                    }
                }
                // (x ∘ C1) ∘ C2 → x ∘ (C1 ∘ C2) — reassociation rounds differently → fast_math only,
                // uniform-baseType chains only (mixed vec/scalar chains would retype the operator)
                if ( fastMath && ( expr->op=="+" || expr->op=="*" ) && isNumericFamily(bt) ) {
                    ExprOp2 * inner = nullptr;
                    Expression * c2 = nullptr;
                    if ( expr->right->rtti_isConstant() && expr->left->rtti_isOp2() ) {
                        inner = static_cast<ExprOp2 *>(expr->left); c2 = expr->right;
                    } else if ( expr->left->rtti_isConstant() && expr->right->rtti_isOp2() ) {
                        inner = static_cast<ExprOp2 *>(expr->right); c2 = expr->left;
                    }
                    if ( inner && inner->op==expr->op && inner->func && inner->func->builtIn && !inner->func->sideEffectFlags
                        && inner->type && inner->type->baseType==bt && c2->type && c2->type->baseType==bt
                        && inner->left->type && inner->left->type->baseType==bt
                        && inner->right->type && inner->right->type->baseType==bt ) {
                        Expression * x = nullptr;
                        Expression * c1 = nullptr;
                        bool constOnRight = false;
                        if ( inner->right->rtti_isConstant() && !inner->left->rtti_isConstant() ) {
                            x = inner->left; c1 = inner->right; constOnRight = true;
                        } else if ( inner->left->rtti_isConstant() && !inner->right->rtti_isConstant() ) {
                            x = inner->right; c1 = inner->left;
                        }
                        if ( x ) {
                            inner->left = c1;
                            inner->right = c2;
                            auto k = evalAndFold(inner);
                            if ( k->rtti_isConstant() ) {
                                expr->left = x;
                                expr->right = k;
                                reportFolding();
                                return Visitor::visit(expr);
                            } else {
                                // restore the original tree exactly
                                if ( constOnRight ) { inner->left = x; inner->right = c1; }
                                else { inner->left = c1; inner->right = x; }
                            }
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // op3
        virtual ExpressionPtr visit ( ExprOp3 * expr ) override {
            if ( expr->type->isFoldable() && expr->subexpr->constexpression && expr->left->constexpression && expr->right->constexpression &&
                foldableBuiltin(expr->func) ) {
                return evalAndFold(expr);
            } else if ( expr->type->isFoldable() && expr->subexpr->noSideEffects && expr->left->constexpression && expr->right->constexpression ) {
                bool failed;
                vec4f left = eval(expr->left, failed);
                if ( failed ) return Visitor::visit(expr);
                vec4f right = eval(expr->right, failed);
                if ( failed ) return Visitor::visit(expr);
                if ( isSameFoldValue(expr->type, left, right) ) {
                    reportFolding();
                    return cloneWithType(expr->left);
                }
            } else if ( expr->subexpr->constexpression ) {
                bool failed;
                vec4f resB = eval(expr->subexpr, failed);
                if ( failed ) return Visitor::visit(expr);
                bool res = cast<bool>::to(resB);
                reportFolding();
                return res ? expr->left : expr->right;
            }
            // c ? true : false → c ; c ? false : true → !c
            if ( expr->type && expr->type->isSimpleType(Type::tBool) ) {
                if ( isConstBoolValue(expr->left,true) && isConstBoolValue(expr->right,false) ) {
                    reportFolding();
                    return expr->subexpr;
                }
                if ( isConstBoolValue(expr->left,false) && isConstBoolValue(expr->right,true) ) {
                    if ( auto notFn = findBuiltinOperator("!", expr->subexpr->type) ) {
                        auto notE = new ExprOp1(expr->at, "!", expr->subexpr);
                        notE->func = notFn;
                        notE->type = new TypeDecl(Type::tBool);
                        reportFolding();
                        return notE;
                    }
                }
            }
            // c ? a : a → a for same-shape arms. Arm purity is NOT required — exactly one
            // arm evaluates before and after the fold; only the dropped condition must be pure
            if ( expr->subexpr->noSideEffects && expr->left->sameAs(expr->right) ) {
                reportFolding();
                return expr->left;
            }
            return Visitor::visit(expr);
        }
    // ExprIfThenElse
        virtual ExpressionPtr visit ( ExprIfThenElse * expr ) override {
            if ( expr->cond->constexpression ) {
                bool failed;
                vec4f resB = eval(expr->cond, failed);
                if ( failed ) return Visitor::visit(expr);
                bool res = cast<bool>::to(resB);
                reportFolding();
                return res ? expr->if_true : expr->if_false;
            }
            if ( expr->if_false ) {
                if ( expr->if_false->rtti_isBlock() ) {
                    auto ifeb = static_cast<ExprBlock*>(expr->if_false);
                    if ( !ifeb->list.size() && !ifeb->finalList.size() ) {
                        expr->if_false = nullptr;
                        reportFolding();
                    }
                }
            }
            if ( !expr->if_false ) {
                if ( expr->if_true->rtti_isBlock() ) {
                    auto ifb = static_cast<ExprBlock*>(expr->if_true);
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
        virtual ExpressionPtr visit ( ExprWhile * expr ) override {
            // while(false) never runs and a constexpr condition is pure — drop the loop
            if ( expr->cond->constexpression ) {
                bool failed;
                vec4f resB = eval(expr->cond, failed);
                if ( !failed && !cast<bool>::to(resB) ) {
                    reportFolding();
                    return nullptr;
                }
            }
            return Visitor::visit(expr);
        }
        virtual ExpressionPtr visit ( ExprFor * expr ) override {
            // a loop over a constant empty range never runs the body
            if ( expr->sources.size()==1 && expr->sources[0]->rtti_isConstant() && expr->sources[0]->type ) {
                auto src = static_cast<ExprConst *>(expr->sources[0]);
                bool emptyRange = false;
                switch ( src->type->baseType ) {
                case Type::tRange:    { auto rv = cast<range>::to(src->value);    emptyRange = rv.from >= rv.to; break; }
                case Type::tURange:   { auto rv = cast<urange>::to(src->value);   emptyRange = rv.from >= rv.to; break; }
                case Type::tRange64:  { auto rv = cast<range64>::to(src->value);  emptyRange = rv.from >= rv.to; break; }
                case Type::tURange64: { auto rv = cast<urange64>::to(src->value); emptyRange = rv.from >= rv.to; break; }
                default: break;
                }
                if ( emptyRange ) {
                    reportFolding();
                    return nullptr;
                }
            }
            if ( expr->body->rtti_isBlock()) {
                auto block = static_cast<ExprBlock*>(expr->body);
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
                vec4f resB = eval(expr->arguments[0], failed);
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
            if ( what->type && what->type->baseType==Type::tFunction && what->rtti_isAddr() ) {
                auto pAddr = static_cast<ExprAddr*>(what);
                auto funcC = pAddr->func;
                auto pCall = new ExprCall(expr->at, funcC->name);
                pCall->func = funcC;
                uint32_t numArgs = uint32_t(expr->arguments.size());
                pCall->arguments.reserve(numArgs-1);
                for ( uint32_t i=1; i!=numArgs; ++i ) {
                    pCall->arguments.push_back( cloneWithType(expr->arguments[i]) );
                }
                TypeDecl::clone(pCall->type,funcC->result);
                reportFolding();
                return pCall;
            }
            return Visitor::visit(expr);
        }
    // ExprCall
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            // same-type workhorse cast is a no-op: int(x) where x is already int, float(x:float), ...
            // numeric families only — a string "cast" can be lifetime-relevant (temporary-ness),
            // and qualifiers don't matter for numeric value types
            if ( expr->func->builtIn && expr->arguments.size()==1 ) {
                auto arg = expr->arguments[0];
                if ( expr->type && arg->type && !expr->type->ref && !arg->type->ref
                    && expr->type->baseType==arg->type->baseType
                    && isNumericFamily(expr->type->baseType)
                    && expr->func->name==das_to_string(expr->type->baseType) ) {
                    reportFolding();
                    return arg;
                }
            }
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
                    if ( foldableBuiltin(expr->func) ) {
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
        FunctionPtr             func = nullptr;
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
        }
        virtual FunctionPtr visit ( Function * that ) override {
            func = nullptr;
            return Visitor::visit(that);
        }
    // turn static assert into nada or report error
        virtual ExpressionPtr visit(ExprStaticAssert * expr) override {
            auto cond = expr->arguments[0];
            if (!cond->constexpression && !cond->rtti_isConstant()) {
                program->error("static assert condition is not constexpr or const", "","",expr->at, CompilationError::invalid_expression);
                return nullptr;
            }
            bool result = false;
            if (cond->constexpression) {
                bool failed;
                vec4f resB = eval(cond, failed);
                if ( failed ) {
                    program->error("exception while computing static assert condition", "","",expr->at, CompilationError::runtime_expression);
                }
                result = cast<bool>::to(resB);
            } else {
                result = ((ExprConstBool *)cond)->getValue();
            }
            if ( !result ) {
                bool iscf = expr->name=="concept_assert";
                string message;
                if ( expr->arguments.size()==2 ) {
                    bool failed;
                    vec4f resM = eval(expr->arguments[1], failed);
                    if ( failed ) {
                        program->error("exception while computing static assert message","","", expr->at, CompilationError::runtime_expression);
                        message = "";
                    } else {
                        message = cast<char *>::to(resM);
                    }
                } else {
                    message = iscf ? "concept assert failed" : "static assert failed";
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
                                   expr->at, CompilationError::runtime_function);
                } else {
                    program->error("function did not run at compilation time","","",
                                   expr->at, CompilationError::runtime_function);
                }
            }
        }
    };

    class RunFolding : public FoldingVisitor {
    public:
        using PassVisitor::visit;
        RunFolding( const ProgramPtr & prog, vector<Function *> & _needRun, int32_t round ) : FoldingVisitor(prog, round),
            runProgram(prog.get()), needRun(_needRun) {
        }
    protected:
        Program * runProgram = nullptr;
        vector<Function *> & needRun;
        bool anySimulated = false;
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructure ( Structure * str ) override {
            return !str->isTemplate;    // we don't do a thing with template structures
        }
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
                    auto it = find_if(needRun.begin(), needRun.end(), [&](Function * f) { return f==expr->func; });
                    if ( it==needRun.end() ) {
                        needRun.push_back(expr->func);
                        anySimulated = false;
                    }
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
                        runProgram->folding = false;
                        if ( runProgram->failed() ) {
                            // its ok to not reset anySimulated here, because if it failed here - it will fail on final simulate
                            // not resetting will make it fail faster, hence cutting time of the failed compilation anyway
                            return Visitor::visit(expr);
                        }
                    }
                    if ( expr->func->index==-1 ) {
                        runProgram->error("internal compilation error, folding symbol was not marked as used","","",
                            expr->at, CompilationError::internal_function);
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
        NoSideEffectVisitor nse(this);
        visit(nse);
    }

    bool Program::optimizationConstFolding(int32_t round) {
        checkSideEffects();
        ConstFolding cfe(this, round);
        visit(cfe);
        bool any = cfe.didAnything();
        if ( !cfe.needRun.empty() ) {
            if ( !options.getBoolOption("disable_run", policies.disable_run) ) {
                RunFolding rfe(this,cfe.needRun,round);
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

