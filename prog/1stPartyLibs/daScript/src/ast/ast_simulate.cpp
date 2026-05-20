#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_match.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_simulate.h"
#include "daScript/simulate/simulate_fusion.h"

#include "daScript/simulate/runtime_array.h"
#include "daScript/simulate/runtime_table_nodes.h"
#include "daScript/simulate/runtime_range.h"
#include "daScript/simulate/runtime_string_delete.h"
#include "daScript/simulate/hash.h"

#include "daScript/simulate/simulate_nodes.h"

#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/misc/gc_node.h"
#include "daScript/simulate/standalone_ctx_utils.h"

das::Context * get_context ( int stackSize=0 );//link time resolved dependencies

namespace das
{
    // fusion function pointers (defined here in main lib, set by fusion lib)
    void (*g_fusionContextFn) ( Context & context, TextWriter & logs, bool enableFusion ) = nullptr;
    void (*g_resetFusionEngineFn) () = nullptr;
    // topological sort for the [init] nodes

    struct InitSort {
        struct Entry {
            uint64_t    id;
            vector<string>  pass;
            vector<string>  before;
            vector<string>  after;
        };
        vector<Entry> entires;

        struct Node {
            uint64_t            id;
            vector<uint64_t>    before;
        };

        vector<Node> build_unsorted () {
            map<uint64_t, Node> nodes;
            map<string, set<uint64_t>> tags;
            for ( auto & e : entires ) {
                Node & n = nodes[e.id];
                n.id = e.id;
                for ( auto & p : e.pass ) {
                    tags[p].insert(e.id);
                }
            }
            for ( auto & e : entires ) {
                Node & n = nodes[e.id];
                for ( auto & b : e.before ) {
                    for ( auto & t : tags[b] ) {
                        n.before.push_back(t);
                    }
                }
                for ( auto & a : e.after ) {
                    for ( auto & t : tags[a] ) {
                        nodes[t].before.push_back(e.id);
                    }
                }
            }
            vector<Node> unsorted;
            for ( auto & n : nodes ) {
                unsorted.emplace_back(n.second);
            }
            return unsorted;
        }

        vector<uint64_t> sort () {
            auto unsorted = build_unsorted();
            vector<uint64_t> result;
            auto lnodes = unsorted.size();
            if ( lnodes != 0 ) {
                vector<Node> sorted;
                sorted.reserve(lnodes);
                while ( unsorted.size() ) {
                    auto node = das::move(unsorted[0]);
                    unsorted.erase(unsorted.begin());
                    if ( node.before.size()==0 ) {
                        for ( auto & other : unsorted ) {
                            for ( int i=int(other.before.size())-1; i>=0; --i ) {
                                if ( other.before[i] == node.id ) {
                                    other.before.erase(other.before.begin()+i);
                                }
                            }
                        }
                        sorted.emplace_back(node);
                    } else {
                        unsorted.emplace_back(node);
                    }
                }
                DAS_ASSERTF(sorted.size()==lnodes,"cyclic dependency in [init] nodes");
                result.reserve(lnodes);
                for ( auto & n : sorted ) {
                    result.push_back(n.id);
                }
            }
            return result;
        }

        void addNode ( uint64_t mnh, AnnotationArgumentList & args ) {
            Entry e;
            e.id = mnh;
            for ( auto & arg : args ) {
                if ( arg.name=="tag" && arg.type==Type::tString ) {
                    e.pass.push_back(arg.sValue);
                } else if ( arg.name=="before" && arg.type==Type::tString ) {
                    e.before.push_back(arg.sValue);
                } else if ( arg.name=="after" && arg.type==Type::tString ) {
                    e.after.push_back(arg.sValue);
                }
            }
            entires.push_back(e);
        }
    };

    struct SimulateVisitor : Visitor {
        Context & context;
        das_hash_map<const Expression*, SimNode*> e2v;
        das_hash_map<const Expression*, vector<SimNode*>> makeLocalInits;
        das_hash_map<const ExprLet*, vector<SimNode*>> letInitVec;

        SimulateVisitor ( Context & ctx ) : context(ctx) {}

        virtual bool canVisitLabel ( ExprLabel * ) override { return false; }
        virtual bool canVisitReader ( ExprReader * ) override { return false; }

        void setE ( const Expression * e, SimNode * n ) {
            e2v[e] = n;
        }

        SimNode * getE ( const Expression * e ) {
            auto it = e2v.find(e);
            DAS_ASSERTF(it != e2v.end(), "SimulateVisitor::getE - expression not found in e2v");
            return it->second;
        }

        // All expression types must be handled by explicit visit() overrides.
        virtual ExpressionPtr visitExpression ( Expression * expr ) override {
            DAS_ASSERTF(0, "SimulateVisitor: unhandled expression type %s", expr->__rtti);
            setE(expr, nullptr);
            return expr;
        }

        // Simulate an expression subtree via the visitor walk
        SimNode * simulateExpression ( Expression * expr ) {
            expr->visit(*this);
            return getE(expr);
        }

        // --- Helper methods ---
        vector<SimNode *> simulateExprMakeVariant(const ExprMakeVariant *mkv);
        vector<SimNode *> simulateExprMakeStruct(const ExprMakeStruct *mks);
        vector<SimNode *> simulateExprMakeArray(const ExprMakeArray *mka);
        vector<SimNode *> simulateExprMakeTuple(const ExprMakeTuple *mkt);

        SimNode * sv_makeLocalCMResMove(const LineInfo & at, uint32_t offset, ExpressionPtr rE);
        SimNode * sv_makeLocalCMResCopy(const LineInfo & at, uint32_t offset, ExpressionPtr rE);
        SimNode * sv_makeLocalRefMove(const LineInfo & at, uint32_t stackTop, uint32_t offset, ExpressionPtr rE);
        SimNode * sv_makeLocalRefCopy(const LineInfo & at, uint32_t stackTop, uint32_t offset, ExpressionPtr rE);
        SimNode * sv_makeLocalMove(const LineInfo & at, uint32_t stackTop, ExpressionPtr rE);
        SimNode * sv_makeLocalCopy(const LineInfo & at, uint32_t stackTop, ExpressionPtr rE);
        SimNode * sv_makeCopy(const LineInfo & at, ExpressionPtr lE, ExpressionPtr rE);
        SimNode * sv_makeMove(const LineInfo & at, ExpressionPtr lE, ExpressionPtr rE);
        SimNode_CallBase * sv_simulateCall(const FunctionPtr & func, const ExprLooksLikeCall * expr, SimNode_CallBase * pCall);
        SimNode * sv_trySimulate(const Expression * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType);
        SimNode * sv_trySimulate_Var(const ExprVar * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType);
        SimNode * sv_trySimulate_Cast(const ExprCast * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType);
        SimNode * sv_trySimulate_At(const ExprAt * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType);
        SimNode * sv_trySimulate_Swizzle(const ExprSwizzle * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType);
        SimNode * sv_trySimulate_Field(const ExprField * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType);
        vector<SimNode*> sv_simulateLocal(const ExprMakeLocal * expr);
        SimNode * sv_simulateLetInit(const VariablePtr & var, bool local);
        vector<SimNode *> sv_simulateLetInits(const ExprLet * pLet);
        void sv_simulateLabels(const ExprBlock * expr, SimNode_Block * block, const das_map<int32_t,uint32_t> & ofsmap);
        void sv_simulateFinal(const ExprBlock * expr, SimNode_Final * block);
        void sv_simulateBlock(const ExprBlock * expr, SimNode_Block * block);
        void sv_whileSimulateFinal(ExpressionPtr bod, SimNode_Block * blk);
        vector<SimNode *> sv_collectExpressions(const vector<ExpressionPtr> & lis, das_map<int32_t,uint32_t> * ofsmap = nullptr);
        SimNode * sv_keepAlive(const LineInfo & at, SimNode * result);

        // All ExprConstXxx types simulate identically: ConstValue(at, value)
#define SV_CONST_VISIT(ExprType) \
        ExpressionPtr visit(ExprType * expr) override { \
            setE(expr, context.code->makeNode<SimNode_ConstValue>(expr->at, expr->value)); \
            return expr; \
        }
        // ExprConst is called as final dispatch after specific ExprConstXxx visitors.
        // Those already set e2v, so this is a no-op passthrough.
        ExpressionPtr visit(ExprConst * expr) override { return expr; }
        SV_CONST_VISIT(ExprFakeContext)
        SV_CONST_VISIT(ExprFakeLineInfo)
        SV_CONST_VISIT(ExprConstPtr)
        SV_CONST_VISIT(ExprConstEnumeration)
        SV_CONST_VISIT(ExprConstBitfield)
        SV_CONST_VISIT(ExprConstInt8)
        SV_CONST_VISIT(ExprConstInt16)
        SV_CONST_VISIT(ExprConstInt64)
        SV_CONST_VISIT(ExprConstInt)
        SV_CONST_VISIT(ExprConstInt2)
        SV_CONST_VISIT(ExprConstInt3)
        SV_CONST_VISIT(ExprConstInt4)
        SV_CONST_VISIT(ExprConstUInt8)
        SV_CONST_VISIT(ExprConstUInt16)
        SV_CONST_VISIT(ExprConstUInt64)
        SV_CONST_VISIT(ExprConstUInt)
        SV_CONST_VISIT(ExprConstUInt2)
        SV_CONST_VISIT(ExprConstUInt3)
        SV_CONST_VISIT(ExprConstUInt4)
        SV_CONST_VISIT(ExprConstRange)
        SV_CONST_VISIT(ExprConstURange)
        SV_CONST_VISIT(ExprConstRange64)
        SV_CONST_VISIT(ExprConstURange64)
        SV_CONST_VISIT(ExprConstBool)
        SV_CONST_VISIT(ExprConstFloat)
        SV_CONST_VISIT(ExprConstFloat2)
        SV_CONST_VISIT(ExprConstFloat3)
        SV_CONST_VISIT(ExprConstFloat4)
        SV_CONST_VISIT(ExprConstDouble)
#undef SV_CONST_VISIT

        ExpressionPtr visit(ExprConstString * expr) override;
        ExpressionPtr visit(ExprLooksLikeCall * expr) override { return expr; }
        ExpressionPtr visit(ExprVar * expr) override;
        ExpressionPtr visit(ExprAddr * expr) override;
        ExpressionPtr visit(ExprTypeDecl * expr) override;
        ExpressionPtr visit(ExprBreak * expr) override;
        ExpressionPtr visit(ExprContinue * expr) override;
        ExpressionPtr visit(ExprAssume * expr) override;
        ExpressionPtr visit(ExprStaticAssert * expr) override;

        ExpressionPtr visit(ExprReader * expr) override;
        ExpressionPtr visit(ExprLabel * expr) override;
        ExpressionPtr visit(ExprTag * expr) override;
        ExpressionPtr visit(ExprIs * expr) override;
        ExpressionPtr visit(ExprMakeGenerator * expr) override;
        ExpressionPtr visit(ExprYield * expr) override;
        ExpressionPtr visit(ExprArrayComprehension * expr) override;

        ExpressionPtr visit(ExprRef2Value * expr) override;
        ExpressionPtr visit(ExprRef2Ptr * expr) override;
        ExpressionPtr visit(ExprPtr2Ref * expr) override;
        ExpressionPtr visit(ExprCast * expr) override;
        ExpressionPtr visit(ExprAscend * expr) override;
        ExpressionPtr visit(ExprDelete * expr) override;
        ExpressionPtr visit(ExprMemZero * expr) override;
        ExpressionPtr visit(ExprUnsafe * expr) override;
        ExpressionPtr visit(ExprWith * expr) override;

        ExpressionPtr visit(ExprNullCoalescing * expr) override;
        ExpressionPtr visit(ExprGoto * expr) override;
        ExpressionPtr visit(ExprOp1 * expr) override;
        ExpressionPtr visit(ExprOp2 * expr) override;
        ExpressionPtr visit(ExprOp3 * expr) override;
        ExpressionPtr visit(ExprCopy * expr) override;
        ExpressionPtr visit(ExprMove * expr) override;
        ExpressionPtr visit(ExprClone * expr) override;
        ExpressionPtr visit(ExprTryCatch * expr) override;

        ExpressionPtr visit(ExprAssert * expr) override;
        ExpressionPtr visit(ExprQuote * expr) override;
        ExpressionPtr visit(ExprDebug * expr) override;
        ExpressionPtr visit(ExprInvoke * expr) override;
        ExpressionPtr visit(ExprErase * expr) override;
        ExpressionPtr visit(ExprSetInsert * expr) override;
        ExpressionPtr visit(ExprFind * expr) override;
        ExpressionPtr visit(ExprKeyExists * expr) override;
        ExpressionPtr visit(ExprTypeInfo * expr) override;
        ExpressionPtr visit(ExprCall * expr) override;
        ExpressionPtr visit(ExprNamedCall * expr) override;
        ExpressionPtr visit(ExprNew * expr) override;

        ExpressionPtr visit(ExprAt * expr) override;
        ExpressionPtr visit(ExprSafeAt * expr) override;
        ExpressionPtr visit(ExprSwizzle * expr) override;
        ExpressionPtr visit(ExprField * expr) override;
        ExpressionPtr visit(ExprSafeField * expr) override;
        ExpressionPtr visit(ExprIsVariant * expr) override;
        ExpressionPtr visit(ExprAsVariant * expr) override;
        ExpressionPtr visit(ExprSafeAsVariant * expr) override;
        ExpressionPtr visit(ExprStringBuilder * expr) override;

        ExpressionPtr visit(ExprReturn * expr) override;
        ExpressionPtr visit(ExprIfThenElse * expr) override;
        ExpressionPtr visit(ExprWhile * expr) override;
        ExpressionPtr visit(ExprFor * expr) override;

        ExpressionPtr visit(ExprBlock * expr) override;
        ExpressionPtr visit(ExprLet * expr) override;
        ExpressionPtr visit(ExprMakeBlock * expr) override;
        ExpressionPtr visit(ExprMakeVariant * expr) override;
        ExpressionPtr visit(ExprMakeStruct * expr) override;
        ExpressionPtr visit(ExprMakeArray * expr) override;
        ExpressionPtr visit(ExprMakeTuple * expr) override;
    };

    // Free function wrapper for SimulateVisitor — usable from other translation units.
    SimNode * simulateExpression ( Context & context, Expression * expr ) {
        SimulateVisitor sv(context);
        return sv.simulateExpression(expr);
    }

    SimNode * trySimulateExpression ( Context & context, const Expression * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType ) {
        SimulateVisitor sv(context);
        return sv.sv_trySimulate(expr, extraOffset, r2vType);
    }

    SimNode * simulateLetInit ( Context & context, const VariablePtr & var, bool local ) {
        SimulateVisitor sv(context);
        if ( var->init ) sv.simulateExpression(var->init);
        return sv.sv_simulateLetInit(var, local);
    }

    vector<SimNode *> simulateLetInits ( Context & context, const ExprLet * pLet ) {
        SimulateVisitor sv(context);
        for ( auto & var : pLet->variables ) {
            if ( var->init ) sv.simulateExpression(var->init);
        }
        return sv.sv_simulateLetInits(pLet);
    }
    // common for move and copy

    SimNode_CallBase * getCallBase ( SimNode * node ) {
#if DAS_ENABLE_KEEPALIVE
        if ( node->rtti_node_isKeepAlive() ) {
            SimNode_KeepAlive * ka = static_cast<SimNode_KeepAlive *>(node);
            node = ka->value;
        }
#endif
        DAS_ASSERTF(node->rtti_node_isCallBase(),"we are calling getCallBase on a node, which is not a call base node."
                    "we should not be here, script compiler should have caught this during compilation.");
        return static_cast<SimNode_CallBase *>(node);
    }

    SimNode * SimulateVisitor::sv_makeLocalCMResMove (const LineInfo & at, uint32_t offset, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto * right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return right;
            }
        }
        // now, to the regular move
        auto left = context.code->makeNode<SimNode_GetCMResOfs>(at, offset);
        auto right = getE(rE);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeLocalCMResCopy(const LineInfo & at, uint32_t offset, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( rightType.canCopy() &&
                "we are calling makeLocalCMResCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        auto right = getE(rE);
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return right;
            }
        }
        // wo standard path
        auto left = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeLocalRefMove (const LineInfo & at, uint32_t stackTop, uint32_t offset, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return right;
            }
        }
        // now, to the regular move
        auto left = context.code->makeNode<SimNode_GetLocalRefOff>(at, stackTop, offset);
        auto right = getE(rE);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeLocalRefCopy(const LineInfo & at, uint32_t stackTop, uint32_t offset, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( rightType.canCopy() &&
                "we are calling makeLocalRefCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        auto right = getE(rE);
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return right;
            }
        }
        // wo standard path
        auto left = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeLocalMove (const LineInfo & at, uint32_t stackTop, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, to the regular move
        auto left = context.code->makeNode<SimNode_GetLocal>(at, stackTop);
        auto right = getE(rE);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else  {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeLocalCopy(const LineInfo & at, uint32_t stackTop, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( rightType.canCopy() &&
                "we are calling makeLocalCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, to the regular copy
        auto left = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
        auto right = getE(rE);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeCopy(const LineInfo & at, ExpressionPtr lE, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( (rightType.canCopy() || rightType.isGoodBlockType()) &&
                "we are calling makeCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = getE(lE);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = getE(lE);
                return right;
            }
        }
        // now, to the regular copy
        auto left = getE(lE);
        auto right = getE(rE);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * SimulateVisitor::sv_makeMove (const LineInfo & at, ExpressionPtr lE, ExpressionPtr rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_cast<ExprCall*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = getE(lE);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_cast<ExprInvoke*>(rE);
            if ( cll->allowCmresSkip() ) {
                auto right = getE(rE);
                getCallBase(right)->cmresEval = getE(lE);
                return right;
            }
        }
        // now to the regular one
        if ( rightType.isRef() ) {
            auto left = getE(lE);
            auto right = getE(rE);
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            // this here might happen during initialization, by moving value types
            // like var t <- 5
            // its ok to generate simplified set here
            auto left = getE(lE);
            auto right = getE(rE);
            return context.code->makeValueNode<SimNode_Set>(rightType.getR2VType(), at, left, right);
        }
    }

    SimNode * Function::makeSimNode ( Context & context, const vector<ExpressionPtr> & ) {
        if ( copyOnReturn || moveOnReturn ) {
            return context.code->makeNodeUnrollAny<SimNode_CallAndCopyOrMove>(int(arguments.size()), at);
        } else if ( fastCall ) {
            return context.code->makeNodeUnrollAny<SimNode_FastCall>(int(arguments.size()), at);
        } else {
            return context.code->makeNodeUnrollAny<SimNode_Call>(int(arguments.size()), at);
        }
    }

    SimNode * Function::simulate (Context & context) const {
        if ( builtIn ) {
            DAS_ASSERTF(0, "can only simulate non built-in function");
            return nullptr;
        }
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto fann = (FunctionAnnotation *)(ann->annotation);
                string err;
                auto node = fann->simulate(&context, (Function*)this, ann->arguments, err);
                if ( !node ) {
                    if ( !err.empty() ) {
                        context.thisProgram->error("integration error, function failed to simulate", err, "",
                            at, CompilationError::missing_node );
                        return nullptr;
                    }
                } else {
                    return node;
                }
            }
        }
        SimulateVisitor sv(context);
        sv.simulateExpression(body);
        if ( fastCall ) {
            DAS_ASSERT(totalStackSize == sizeof(Prologue) && "function can't allocate stack");
            DAS_ASSERT((result->isWorkhorseType() || result->isVoid()) && "fastcall can only return a workhorse type");
            DAS_ASSERT(body->rtti_isBlock() && "function must contain a block");
            auto block = static_cast<ExprBlock*>(body);
            if ( block->list.size()==0 ) {
                DAS_ASSERT(block->inFunction && block->inFunction->result->isVoid() && "only void function produces fastcall NOP");
                return context.code->makeNode<SimNode_NOP>(block->at);
            }
            if ( block->list.back()->rtti_isReturn() ) {
                DAS_ASSERT(block->list.back()->rtti_isReturn() && "fastcall body expr is return");
                auto retE = static_cast<ExprReturn*>(block->list.back());
                if ( retE->subexpr ) {
                    return sv.getE(retE->subexpr);
                } else {
                    return context.code->makeNode<SimNode_NOP>(retE->at);
                }
            } else {
                return sv.getE(block->list.back());
            }
        } else {
#if DAS_DEBUGGER
            if ( context.debugger ) {
                auto sbody = sv.getE(body);
                if ( !sbody->rtti_node_isBlock() ) {
                    auto block = context.code->makeNode<SimNodeDebug_BlockNF>(sbody->debugInfo);
                    block->total = 1;
                    block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*1);
                    block->list[0] = sbody;
                    return block;
                } else {
                    return sbody;
                }
            } else {
                return sv.getE(body);
            }
#else
            return sv.getE(body);
#endif
        }
    }

    void ExprMakeLocal::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        useStackRef = ref;
        useCMRES = cmres;
        doesNotNeedSp = true;
        doesNotNeedInit = true;
        stackTop = sp;
        extraOffset = off;
    }

    // variant

    void ExprMakeVariant::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        ExprMakeLocal::setRefSp(ref, cmres, sp, off);
        int stride = makeType->getStride();
        // we go through all fields, and if its [[ ]] field
        // we tell it to piggy-back on our current sp, with appropriate offset
        int index = 0;
        for ( const auto & decl : variants ) {
            auto fieldVariant = makeType->findArgumentIndex(decl->name);
            DAS_ASSERT(fieldVariant!=-1 && "should have failed in type infer otherwise");
            if ( decl->value->rtti_isMakeLocal() ) {
                auto fieldOffset = makeType->getVariantFieldOffset(fieldVariant);
                uint32_t offset =  extraOffset + index*stride + fieldOffset;
                auto mkl = static_cast<ExprMakeLocal*>(decl->value);
                mkl->setRefSp(ref, cmres, sp, offset);
                mkl->doesNotNeedInit = false;
            } else if ( decl->value->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(decl->value);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( decl->value->rtti_isInvoke() ) {
                auto cll = static_cast<ExprInvoke*>(decl->value);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
            index++;
        }
    }

    vector<SimNode *> SimulateVisitor::simulateExprMakeVariant(const ExprMakeVariant *mkv) {
        gc_guard gc_scope;
        vector<SimNode *> simlist;
        int index = 0;
        int stride = mkv->makeType->getStride();
        // init with 0 it its 'default' initialization
        if ( stride && mkv->variants.empty() ) {
            int bytes = stride;
            SimNode * init0;
            if ( mkv->useCMRES ) {
                if ( bytes==0 ) {
                    init0 = nullptr;
                } else if ( bytes <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, mkv->at, mkv->extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(mkv->at, mkv->extraOffset, bytes);
                }
            } else if ( mkv->useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(mkv->at, mkv->stackTop, mkv->extraOffset, bytes);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(mkv->at, mkv->stackTop + mkv->extraOffset, bytes);
            }
            if (init0) simlist.push_back(init0);
        }
        // now fields
        for ( const auto & decl : mkv->variants ) {
            auto fieldVariant = mkv->makeType->findArgumentIndex(decl->name);
            DAS_ASSERT(fieldVariant!=-1 && "should have failed in type infer otherwise");
            // lets set variant index
            uint32_t voffset = mkv->extraOffset + index*stride;
            auto vconst = new ExprConstInt(mkv->at, int32_t(fieldVariant));
            vconst->type = new TypeDecl(Type::tInt);
            setE(vconst, context.code->makeNode<SimNode_ConstValue>(vconst->at, vconst->value));
            SimNode * svi;
            if ( mkv->useCMRES ) {
                svi = sv_makeLocalCMResCopy(mkv->at, voffset, vconst);
            } else if (mkv->useStackRef) {
                svi = sv_makeLocalRefCopy(mkv->at, mkv->stackTop, voffset, vconst);
            } else {
                svi = sv_makeLocalCopy(mkv->at, mkv->stackTop+voffset, vconst);
            }
            simlist.push_back(svi);
            // field itself
            auto fieldOffset = mkv->makeType->getVariantFieldOffset(fieldVariant);
            uint32_t offset =  voffset + fieldOffset;
            SimNode * cpy = nullptr;
            if ( decl->value->rtti_isMakeLocal() ) {
                // so what happens here, is we ask it for the generated commands and append it to this list only
                auto mkl = static_cast<ExprMakeLocal*>(decl->value);
                auto lsim = sv_simulateLocal(mkl);
                simlist.insert(simlist.end(), lsim.begin(), lsim.end());
            } else if ( mkv->useCMRES ) {
                if ( decl->moveSemantics ){
                    cpy = sv_makeLocalCMResMove(mkv->at, offset, decl->value);
                } else {
                    cpy = sv_makeLocalCMResCopy(mkv->at, offset, decl->value);
                }
            } else if ( mkv->useStackRef ) {
                if ( decl->moveSemantics ){
                    cpy = sv_makeLocalRefMove(mkv->at, mkv->stackTop, offset, decl->value);
                } else {
                    cpy = sv_makeLocalRefCopy(mkv->at, mkv->stackTop, offset, decl->value);
                }
            } else {
                if ( decl->moveSemantics ){
                    cpy = sv_makeLocalMove(mkv->at, mkv->stackTop+offset, decl->value);
                } else {
                    cpy = sv_makeLocalCopy(mkv->at, mkv->stackTop+offset, decl->value);
                }
            }
            if ( cpy ) simlist.push_back(cpy);
            index++;
        }
        return simlist;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMakeVariant * expr) {
        const auto &at = expr->at;
        SimNode_Block * block;
        if ( expr->useCMRES ) {
            block = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            block = context.code->makeNode<SimNode_MakeLocal>(at, expr->stackTop);
        }
        auto simlist = sv_simulateLocal(expr);
        block->total = int(simlist.size());
        block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
        for ( uint32_t i=0, is=block->total; i!=is; ++i )
            block->list[i] = simlist[i];
        setE(expr, block);
        return expr;
    }

    // structure

    void ExprMakeStruct::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        ExprMakeLocal::setRefSp(ref, cmres, sp, off);
        // if it's a handle type, we can't reuse the make-local chain
        if ( makeType->baseType == Type::tHandle ) return;
        // we go through all fields, and if its [[ ]] field
        // we tell it to piggy-back on our current sp, with appropriate offset
        int total = int(structs.size());
        int stride = makeType->getStride();
        for ( int index=0; index != total; ++index ) {
            auto & fields = structs[index];
            for ( const auto & decl : *fields ) {
                auto field = makeType->structType->findField(decl->name);
                DAS_ASSERT(field && "should have failed in type infer otherwise");
                if ( decl->value->rtti_isMakeLocal() ) {
                    uint32_t offset =  extraOffset + index*stride + field->offset;
                    auto mkl = static_cast<ExprMakeLocal*>(decl->value);
                    mkl->setRefSp(ref, cmres, sp, offset);
                } else if ( decl->value->rtti_isCall() ) {
                    auto cll = static_cast<ExprCall*>(decl->value);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    }
                } else if ( decl->value->rtti_isInvoke() ) {
                    auto cll = static_cast<ExprInvoke*>(decl->value);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    }
                }
            }
        }
    }

    vector<SimNode *> SimulateVisitor::simulateExprMakeStruct(const ExprMakeStruct *mks) {
        gc_guard gc_scope;
        vector<SimNode *> simlist;
        // init with 0
        int total = int(mks->structs.size());
        int stride = mks->makeType->getStride();
        // note: if its an empty tuple init, like [[tuple<int;float>]] and its embedded - we need to zero it out
        bool emptyEmbeddedTuple = ( mks->makeType->baseType==Type::tTuple && total==0);
        bool partialyInitStruct = !mks->doesNotNeedInit && !mks->initAllFields;
        if ( (emptyEmbeddedTuple || partialyInitStruct) && stride ) {
            int bytes = das::max(total,1) * stride;
            SimNode * init0;
            if ( mks->useCMRES ) {
                if ( bytes==0 ) {
                    init0 = nullptr;
                } else if ( bytes <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, mks->at, mks->extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(mks->at, mks->extraOffset, bytes);
                }
            } else if ( mks->useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(mks->at, mks->stackTop, mks->extraOffset, bytes);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(mks->at, mks->stackTop + mks->extraOffset, bytes);
            }
            if (init0) simlist.push_back(init0);
        }
        if ( mks->makeType->baseType == Type::tStructure ) {
            for ( int index=0; index != total; ++index ) {
                if ( mks->constructor ) {
                    uint32_t offset = mks->extraOffset + index*stride;
                    SimNode_CallBase * pCall = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_CallAndCopyOrMove>(0, mks->at);
                    DAS_ASSERT(mks->constructor->index!=-1 && "should have failed in type infer otherwise");
                    pCall->fnPtr = context.getFunction(mks->constructor->index);
                    if ( mks->useCMRES ) {
                        pCall->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(mks->at, offset);
                    } else if ( mks->useStackRef ) {
                        pCall->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(mks->at, mks->stackTop, offset);
                    } else {
                        pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(mks->at, mks->stackTop + offset);
                    }
                    simlist.push_back(pCall);
                }
                auto & fields = mks->structs[index];
                for ( const auto & decl : *fields ) {
                    auto field = mks->makeType->structType->findField(decl->name);
                    DAS_ASSERT(field && "should have failed in type infer otherwise");
                    uint32_t offset = mks->extraOffset + index*stride + field->offset;
                    SimNode * cpy;
                    if ( decl->value->rtti_isMakeLocal() ) {
                        // so what happens here, is we ask it for the generated commands and append it to this list only
                        auto mkl = static_cast<ExprMakeLocal*>(decl->value);
                        auto lsim = sv_simulateLocal(mkl);
                        simlist.insert(simlist.end(), lsim.begin(), lsim.end());
                        continue;
                    } else if ( mks->useCMRES ) {
                        if ( decl->moveSemantics ){
                            cpy = sv_makeLocalCMResMove(mks->at, offset, decl->value);
                        } else {
                            cpy = sv_makeLocalCMResCopy(mks->at, offset, decl->value);
                        }
                    } else if ( mks->useStackRef ) {
                        if ( decl->moveSemantics ){
                            cpy = sv_makeLocalRefMove(mks->at, mks->stackTop, offset, decl->value);
                        } else {
                            cpy = sv_makeLocalRefCopy(mks->at, mks->stackTop, offset, decl->value);
                        }
                    } else {
                        if ( decl->moveSemantics ){
                            cpy = sv_makeLocalMove(mks->at, mks->stackTop+offset, decl->value);
                        } else {
                            cpy = sv_makeLocalCopy(mks->at, mks->stackTop+offset, decl->value);
                        }
                    }
                    if ( !cpy ) {
                        context.thisProgram->error("internal compilation error, can't generate structure initialization", "", "", mks->at);
                    }
                    simlist.push_back(cpy);
                }
            }
        } else {
            auto ann = mks->makeType->annotation;
            // making fake variable, which points to out field
            string fakeName = "__makelocal";
            auto fakeVariable = new Variable();
            fakeVariable->name = fakeName;
            fakeVariable->type = new TypeDecl(Type::tHandle);
            fakeVariable->type->annotation = ann;
            fakeVariable->at = mks->at;
            if ( mks->useCMRES ) {
                fakeVariable->aliasCMRES = true;
            } else if ( mks->useStackRef ) {
                fakeVariable->stackTop = mks->stackTop;
                fakeVariable->extraLocalOffset = mks->extraOffset;
                fakeVariable->type->ref = true;
                if ( total != 1 ) {
                    fakeVariable->type->dim.push_back(total);
                }
            }
            fakeVariable->generated = true;
            // make fake ExprVar which is that field
            auto fakeVar = new ExprVar(mks->at, fakeName);
            fakeVar->type = fakeVariable->type;
            fakeVar->variable = fakeVariable;
            fakeVar->local = true;
            // make fake expression
            ExpressionPtr fakeExpr = fakeVar;
            ExprConstInt * indexExpr = nullptr;
            if ( mks->useStackRef && total > 1 ) {
                // if its stackRef with multiple indices, its actually var[total], and lookup is var[index]
                indexExpr = new ExprConstInt(mks->at, 0);
                indexExpr->type = new TypeDecl(Type::tInt);
                fakeExpr = new ExprAt(mks->at, fakeExpr, indexExpr);
                fakeExpr->type = new TypeDecl(Type::tHandle);
                fakeExpr->type->annotation = ann;
                fakeExpr->type->ref = true;
            }
            for ( int index=0; index != total; ++index ) {
                auto & fields = mks->structs[index];
                // adjust var for index
                if ( mks->useCMRES ) {
                    fakeVariable->stackTop = mks->extraOffset + index*stride;
                } else if ( mks->useStackRef ) {
                    if ( total > 1 ) {
                        indexExpr->value = cast<int32_t>::from(index);
                    }
                } else {
                    fakeVariable->stackTop = mks->stackTop + mks->extraOffset + index*stride;
                }
                // now, setup fields
                for ( const auto & decl : *fields ) {
                    auto fieldType = ann->makeFieldType(decl->name, false);
                    DAS_ASSERT(fieldType && "how did this infer?");
                    uint32_t fieldSize = fieldType->getSizeOf();
                    SimNode * cpy = nullptr;
                    uint32_t fieldOffset = ann->getFieldOffset(decl->name);
                    SimNode * simV = simulateExpression(fakeExpr);
                    auto left = context.code->makeNode<SimNode_FieldDeref>(mks->at, simV, fieldOffset);
                    auto right = getE(decl->value);
                    if ( !decl->value->type->isRef() ) {
                        cpy = context.code->makeValueNode<SimNode_Set>(decl->value->type->getR2VType(), decl->at, left, right);
                    } else if ( decl->moveSemantics ) {
                        cpy = context.code->makeNode<SimNode_MoveRefValue>(decl->at, left, right, fieldSize);
                    } else {
                        cpy = context.code->makeNode<SimNode_CopyRefValue>(decl->at, left, right, fieldSize);
                    }
                    simlist.push_back(cpy);
                }
            }
        }
        if ( mks->block ) {
            /*
                TODO: optimize
                    there is no point in making fake invoke expression, we can replace 'self' with fake variable we've made
                    however this needs to happen during infer, and this needs to have different visitor,
                    so that stack is allocated properly in subexpressions etc
            */
            // making fake variable, which points to entire structure
            string fakeName = "__makelocal";
            auto fakeVariable = new Variable();
            fakeVariable->name = fakeName;
            fakeVariable->type = new TypeDecl(*mks->type);
            if ( mks->useCMRES ) {
                fakeVariable->aliasCMRES = true;
            } else if ( mks->useStackRef ) {
                fakeVariable->stackTop = mks->stackTop + mks->extraOffset;
                fakeVariable->type->ref = true;
            } else {
                fakeVariable->stackTop = mks->stackTop + mks->extraOffset;
            }
            fakeVariable->generated = true;
            // make fake ExprVar which is that field
            auto fakeVar = new ExprVar(mks->at, fakeName);
            fakeVar->type = fakeVariable->type;
            fakeVar->variable = fakeVariable;
            fakeVar->local = true;
            // make fake invoke expression
            auto fakeInvoke = new ExprInvoke(mks->at, "invoke");
            fakeInvoke->arguments.push_back(mks->block);
            fakeInvoke->arguments.push_back(fakeVar);
            // simulate it
            auto simI = simulateExpression(fakeInvoke);
            simlist.push_back(simI);
        }
        return simlist;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMakeStruct * expr) {
        const auto &at = expr->at;
        SimNode_Block * blk;
        if ( expr->useCMRES ) {
            blk = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            blk = context.code->makeNode<SimNode_MakeLocal>(at, expr->stackTop);
        }
        auto simlist = sv_simulateLocal(expr);
        blk->total = int(simlist.size());
        blk->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*blk->total);
        for ( uint32_t i=0, is=blk->total; i!=is; ++i )
            blk->list[i] = simlist[i];
        setE(expr, blk);
        return expr;
    }

    // make array

    void ExprMakeArray::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        ExprMakeLocal::setRefSp(ref, cmres, sp, off);
        int total = int(values.size());
        uint32_t stride = recordType->getSizeOf();
        for ( int index=0; index != total; ++index ) {
            auto & val = values[index];
            if ( val->rtti_isMakeLocal() ) {
                uint32_t offset =  extraOffset + index*stride;
                auto mkl = static_cast<ExprMakeLocal*>(val);
                mkl->setRefSp(ref, cmres, sp, offset);
            } else if ( val->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( val->rtti_isInvoke() ) {
                auto cll = static_cast<ExprInvoke*>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
        }
    }

    vector<SimNode *> SimulateVisitor::simulateExprMakeArray(const ExprMakeArray *mka) {
        vector<SimNode *> simlist;
        // init with 0
        int total = int(mka->values.size());
        uint32_t stride = mka->recordType->getSizeOf();
        if ( !mka->doesNotNeedInit && !mka->initAllFields ) {
            int bytes = total * stride;
            SimNode * init0;
            if ( mka->useCMRES ) {
                if ( bytes==0 ) {
                    init0 = nullptr;
                } else if ( bytes <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, mka->at, mka->extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(mka->at, mka->extraOffset, bytes);
                }
            } else if ( mka->useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(mka->at, mka->stackTop, mka->extraOffset, stride * total);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(mka->at, mka->stackTop + mka->extraOffset, stride * total);
            }
            if (init0) simlist.push_back(init0);
        }
        for ( int index=0; index != total; ++index ) {
            auto & val = mka->values[index];
            uint32_t offset = mka->extraOffset + index*stride;
            SimNode * cpy;
            if ( val->rtti_isMakeLocal() ) {
                // so what happens here, is we ask it for the generated commands and append it to this list only
                auto mkl = static_cast<ExprMakeLocal*>(val);
                auto lsim = sv_simulateLocal(mkl);
                simlist.insert(simlist.end(), lsim.begin(), lsim.end());
                continue;
            } else if ( mka->useCMRES ) {
                if (val->type->canCopy()) {
                    cpy = sv_makeLocalCMResCopy(mka->at, offset, val);
                } else {
                    cpy = sv_makeLocalCMResMove(mka->at, offset, val);
                }
            } else if ( mka->useStackRef ) {
                if (val->type->canCopy()) {
                    cpy = sv_makeLocalRefCopy(mka->at, mka->stackTop, offset, val);
                } else {
                    cpy = sv_makeLocalRefMove(mka->at, mka->stackTop, offset, val);
                }
            } else {
                if (val->type->canCopy()) {
                    cpy = sv_makeLocalCopy(mka->at, mka->stackTop + offset, val);
                } else {
                    cpy = sv_makeLocalMove(mka->at, mka->stackTop + offset, val);
                }
            }
            if ( !cpy ) {
                context.thisProgram->error("internal compilation error, can't generate array initialization", "", "", mka->at);
            }
            simlist.push_back(cpy);
        }
        return simlist;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMakeArray * expr) {
        const auto &at = expr->at;
        SimNode_Block * block;
        if ( expr->useCMRES ) {
            block = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            block = context.code->makeNode<SimNode_MakeLocal>(at, expr->stackTop);
        }
        auto simlist = sv_simulateLocal(expr);
        block->total = int(simlist.size());
        block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
        for ( uint32_t i=0, is=block->total; i!=is; ++i )
            block->list[i] = simlist[i];
        setE(expr, block);
        return expr;
    }

    // make tuple

    void ExprMakeTuple::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        ExprMakeLocal::setRefSp(ref, cmres, sp, off);
        int total = int(values.size());
        for ( int index=0; index != total; ++index ) {
            auto & val = values[index];
            if ( val->rtti_isMakeLocal() ) {
                uint32_t offset =  extraOffset + makeType->getTupleFieldOffset(index);
                auto mkl = static_cast<ExprMakeLocal*>(val);
                mkl->setRefSp(ref, cmres, sp, offset);
            } else if ( val->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( val->rtti_isInvoke() ) {
                auto cll = static_cast<ExprInvoke*>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
        }
    }

    vector<SimNode *> SimulateVisitor::simulateExprMakeTuple(const ExprMakeTuple *mkt) {
        vector<SimNode *> simlist;
        // init with 0
        int total = int(mkt->values.size());
        if ( !mkt->doesNotNeedInit && !mkt->initAllFields ) {
            uint32_t sizeOf = mkt->makeType->getSizeOf();
            SimNode * init0;
            if ( mkt->useCMRES ) {
                if ( sizeOf==0 ) {
                    init0 = nullptr;
                } else if ( sizeOf <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(sizeOf, mkt->at, mkt->extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(mkt->at, mkt->extraOffset, sizeOf);
                }
            } else if ( mkt->useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(mkt->at, mkt->stackTop, mkt->extraOffset, sizeOf);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(mkt->at, mkt->stackTop + mkt->extraOffset, sizeOf);
            }
            if (init0) simlist.push_back(init0);
        }
        for ( int index=0; index != total; ++index ) {
            auto & val = mkt->values[index];
            uint32_t offset = mkt->extraOffset + mkt->makeType->getTupleFieldOffset(index);
            SimNode * cpy;
            if ( val->rtti_isMakeLocal() ) {
                // so what happens here, is we ask it for the generated commands and append it to this list only
                auto mkl = static_cast<ExprMakeLocal*>(val);
                auto lsim = sv_simulateLocal(mkl);
                simlist.insert(simlist.end(), lsim.begin(), lsim.end());
                continue;
            } else if ( mkt->useCMRES ) {
                if (val->type->canCopy()) {
                    cpy = sv_makeLocalCMResCopy(mkt->at, offset, val);
                } else {
                    cpy = sv_makeLocalCMResMove(mkt->at, offset, val);
                }
            } else if ( mkt->useStackRef ) {
                if (val->type->canCopy()) {
                    cpy = sv_makeLocalRefCopy(mkt->at, mkt->stackTop, offset, val);
                } else {
                    cpy = sv_makeLocalRefMove(mkt->at, mkt->stackTop, offset, val);
                }
            } else {
                if (val->type->canCopy()) {
                    cpy = sv_makeLocalCopy(mkt->at, mkt->stackTop + offset, val);
                } else {
                    cpy = sv_makeLocalMove(mkt->at, mkt->stackTop + offset, val);
                }
            }
            if ( !cpy ) {
                context.thisProgram->error("internal compilation error, can't generate array initialization", "", "", mkt->at);
            }
            simlist.push_back(cpy);
        }
        return simlist;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMakeTuple * expr) {
        const auto &at = expr->at;
        SimNode_Block * block;
        if ( expr->useCMRES ) {
            block = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            block = context.code->makeNode<SimNode_MakeLocal>(at, expr->stackTop);
        }
        auto simlist = sv_simulateLocal(expr);
        block->total = int(simlist.size());
        block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
        for ( uint32_t i=0, is=block->total; i!=is; ++i )
            block->list[i] = simlist[i];
        setE(expr, block);
        return expr;
    }

    // reader

    ExpressionPtr SimulateVisitor::visit(ExprReader * expr) {
        context.thisProgram->error("internal compilation error, calling 'simulate' on reader", "", "", expr->at);
        setE(expr, nullptr);
        return expr;
    }

    // label

    ExpressionPtr SimulateVisitor::visit(ExprLabel * expr) {
        context.thisProgram->error("internal compilation error, calling 'simulate' on label", "", "", expr->at);
        setE(expr, nullptr);
        return expr;
    }

    // goto

    ExpressionPtr SimulateVisitor::visit(ExprGoto * expr) {
        const auto &at = expr->at;
        SimNode * result = nullptr;
        if ( expr->subexpr ) {
            result = context.code->makeNode<SimNode_Goto>(at, getE(expr->subexpr));
        } else {
            result = context.code->makeNode<SimNode_GotoLabel>(at, expr->label);
        }
#if DAS_ENABLE_KEEPALIVE
        if ( context.thisProgram->policies.keep_alive ) {
            result = context.code->makeNode<SimNode_KeepAlive>(at, result);
        }
#endif
        setE(expr, result);
        return expr;
    }

    // r2v

    SimNode * GetR2V ( Context & context, const LineInfo & at, const TypeDeclPtr & type, SimNode * expr ) {
        if ( type->isRefType() ) {
            return expr;
        } else {
            return context.code->makeValueNode<SimNode_Ref2Value>(type->getR2VType(), at, expr);
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprRef2Value * expr) {
        const auto &at = expr->at;
        setE(expr, GetR2V(context, at, expr->type, getE(expr->subexpr)));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprAddr * expr) {
        const auto &at = expr->at;
        if ( !expr->func ) {
            context.thisProgram->error("internal compilation error, ExprAddr func is null", "", "", at);
            setE(expr, nullptr);
            return expr;
        } else if ( expr->func->index<0 ) {
            context.thisProgram->error("internal compilation error, ExprAddr func->index is unused", "", "", at);
            setE(expr, nullptr);
            return expr;

        }
        union {
            uint64_t    mnh;
            vec4f       cval;
        } temp;
        temp.cval = v_zero();
        if ( expr->func->module->isSolidContext ) {
            DAS_ASSERT(expr->func->index>=0 && "address of unsued function? how?");
            temp.mnh = expr->func->index;
            setE(expr, context.code->makeNode<SimNode_FuncConstValue>(at,temp.cval));
        } else {
            temp.mnh = expr->func->getMangledNameHash();
            setE(expr, context.code->makeNode<SimNode_FuncConstValueMnh>(at,temp.cval));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprPtr2Ref * expr) {
        const auto &at = expr->at;
        if ( expr->unsafeDeref ) {
            setE(expr, getE(expr->subexpr));
        } else {
            auto errorMessage = context.code->allocateName(", "+expr->subexpr->describe()+" is null");
            setE(expr, context.code->makeNode<SimNode_Ptr2Ref>(at, getE(expr->subexpr), errorMessage));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprRef2Ptr * expr) {
        setE(expr, getE(expr->subexpr));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprNullCoalescing * expr) {
        const auto &at = expr->at;
        if ( expr->type->isRef() ) {
            setE(expr, context.code->makeNode<SimNode_NullCoalescingRef>(at, getE(expr->subexpr), getE(expr->defaultValue)));
        } else if ( expr->type->isHandle() ) {
            if ( auto resN = expr->type->annotation->simulateNullCoalescing(context, at, getE(expr->subexpr), getE(expr->defaultValue)) ) {
                setE(expr, resN);
            } else {
                context.thisProgram->error("internal compilation error, simluateNullCoalescing returned null", "", "", at);
            }
        } else {
            setE(expr, context.code->makeValueNode<SimNode_NullCoalescing>(expr->type->getR2VType(), at, getE(expr->subexpr), getE(expr->defaultValue)));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprConstString * expr) {
        const auto &at = expr->at;
        if ( !expr->text.empty() ) {
            char* str = context.constStringHeap->impl_allocateString(expr->text);
            setE(expr, context.code->makeNode<SimNode_ConstString>(at, str));
        } else {
            setE(expr, context.code->makeNode<SimNode_ConstString>(at, nullptr));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprStaticAssert * expr) {
        setE(expr, nullptr);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprAssert * expr) {
        const auto &at = expr->at;
        string message;
        if ( expr->arguments.size()==2 && expr->arguments[1]->rtti_isStringConstant() )
            message = static_cast<ExprConstString*>(expr->arguments[1])->getValue();
        setE(expr, context.code->makeNode<SimNode_Assert>(at, getE(expr->arguments[0]),
            context.constStringHeap->impl_allocateString(message)));
        return expr;
    }

    struct SimNode_AstGetExpression : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_AstGetExpression ( const LineInfo & at, ExpressionPtr e, char * d )
            : SimNode_CallBase(at,"") {
            expr = e;
            descr = d;
        }
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override {
            auto that = (SimNode_AstGetExpression *) SimNode::copyNode(context, code);
            that->descr = code->allocateName(descr);
            return that;
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(AstGetExpression);
            V_ARG(descr);
            V_END();
        }
        __forceinline char * compute(Context &) {
            DAS_PROFILE_NODE
            return (char *) expr->clone();
        }
        Expression *  expr;   // requires RTTI
        char *        descr;
    };

    ExpressionPtr SimulateVisitor::visit(ExprQuote * expr) {
        const auto &at = expr->at;
        DAS_ASSERTF(expr->arguments.size()==1,"Quote expects to return only one ExpressionPtr."
        "We should not be here, since typeinfer should catch the mismatch.");
        TextWriter ss;
        ss << *expr->arguments[0];
        char * descr = context.code->allocateName(ss.str());
        setE(expr, context.code->makeNode<SimNode_AstGetExpression>(at, expr->arguments[0], descr));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprDebug * expr) {
        const auto &at = expr->at;
        TypeInfo * pTypeInfo = context.thisHelper->makeTypeInfo(nullptr, expr->arguments[0]->type);
        string message;
        if ( expr->arguments.size()==2 && expr->arguments[1]->rtti_isStringConstant() )
            message = static_cast<ExprConstString*>(expr->arguments[1])->getValue();
        setE(expr, context.code->makeNode<SimNode_Debug>(at, getE(expr->arguments[0]),
            pTypeInfo, context.constStringHeap->impl_allocateString(message)));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMemZero * expr) {
        const auto &at = expr->at;
        const auto & subexpr = expr->arguments[0];
        uint32_t dataSize = subexpr->type->getSizeOf();
        setE(expr, context.code->makeNode<SimNode_MemZero>(at, getE(subexpr), dataSize));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMakeGenerator * expr) {
        const auto &at = expr->at;
        DAS_ASSERTF(0, "we should not be here ever, ExprMakeGenerator should completly fold during type inference.");
        context.thisProgram->error("internal compilation error, generating node for ExprMakeGenerator", "", "", at);
        setE(expr, nullptr);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprYield * expr) {
        const auto &at = expr->at;
        DAS_ASSERTF(0, "we should not be here ever, ExprYield should completly fold during type inference.");
        context.thisProgram->error("internal compilation error, generating node for ExprYield", "", "", at);
        setE(expr, nullptr);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprArrayComprehension * expr) {
        const auto &at = expr->at;
        DAS_ASSERTF(0, "we should not be here ever, ExprArrayComprehension should completly fold during type inference.");
        context.thisProgram->error("internal compilation error, generating node for ExprArrayComprehension", "", "", at);
        setE(expr, nullptr);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMakeBlock * expr) {
        const auto &at = expr->at;
        gc_guard gc_scope;
        auto blk = static_cast<ExprBlock*>(expr->block);
        uint32_t argSp = blk->stackTop;
        auto bt = blk->makeBlockType();
        auto info = context.thisHelper->makeInvokeableTypeDebugInfo(bt, blk->at);
        if ( context.gcEnabled || context.debugger  ) {
            context.thisHelper->appendLocalVariables(info, (Expression *)expr);
        }
        setE(expr, context.code->makeNode<SimNode_MakeBlock>(at, getE(expr->block), argSp, expr->stackTop, info));
        return expr;
    }

    bool ExprInvoke::isCopyOrMove() const {
        auto blockT = arguments[0]->type;
        return blockT->firstType && blockT->firstType->isRefType() && !blockT->firstType->ref;
    }

    ExpressionPtr SimulateVisitor::visit(ExprInvoke * expr) {
        const auto &at = expr->at;
        auto blockT = expr->arguments[0]->type;
        SimNode_CallBase * pInvoke;
        uint32_t methodOffset = 0;
        if ( expr->isInvokeMethod ) {
            bool foundOffset = false;
            if ( expr->arguments[0]->rtti_isField() ) {
                auto field = static_cast<ExprField*>(expr->arguments[0]);
                methodOffset = field->fieldRef->offset;
                foundOffset = true;
            } else if ( expr->arguments[0]->rtti_isR2V() ) {
                auto eR2V = static_cast<ExprRef2Value*>(expr->arguments[0]);
                if ( eR2V->subexpr->rtti_isField() ) {
                    auto field = static_cast<ExprField*>(eR2V->subexpr);
                    methodOffset = field->fieldRef->offset;
                    foundOffset = true;
                }
            }
            if (!foundOffset) {
                context.thisProgram->error("internal compilation error, invoke method expects field", "", "", at);
                setE(expr, nullptr);
                return expr;
            }
        }
        {
            if ( expr->isCopyOrMove() ) {
                DAS_ASSERTF ( blockT->baseType!=Type::tString, "its never CMRES for named function" );
                auto getSp = context.code->makeNode<SimNode_GetLocal>(at, expr->stackTop);
                if ( blockT->baseType==Type::tBlock ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMove>(
                                                        int(expr->arguments.size()), at, getSp);
                } else if ( blockT->baseType==Type::tFunction && expr->isInvokeMethod ) {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMoveMethod>(
                                                        int(expr->arguments.size()-1), at, getSp, errorMessage);
                    ((SimNode_InvokeAndCopyOrMoveMethodAny *) pInvoke)->methodOffset = methodOffset;
                } else if ( blockT->baseType==Type::tFunction ) {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMoveFn>(
                                                        int(expr->arguments.size()), at, getSp, errorMessage);
                } else {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMoveLambda>(
                                                        int(expr->arguments.size()), at, getSp, errorMessage);
                }
            } else {
                if ( blockT->baseType==Type::tString ) {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeFnByName>(int(expr->arguments.size()), at, errorMessage);
                } else if ( blockT->baseType==Type::tBlock ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_Invoke>(int(expr->arguments.size()), at);
                } else if ( blockT->baseType==Type::tFunction && expr->isInvokeMethod ) {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeMethod>(int(expr->arguments.size()-1), at, errorMessage);
                    ((SimNode_InvokeMethodAny *) pInvoke)->methodOffset = methodOffset;
                } else if ( blockT->baseType==Type::tFunction ) {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeFn>(int(expr->arguments.size()), at, errorMessage);
                } else {
                    auto errorMessage = context.code->allocateName(", "+expr->arguments[0]->describe());
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeLambda>(int(expr->arguments.size()), at, errorMessage);
                }
            }
        }
        pInvoke->debugInfo = at;
        int nArg = (int) expr->arguments.size();
        if ( expr->isInvokeMethod ) nArg --;
        if ( nArg ) {
            pInvoke->arguments = (SimNode **) context.code->allocate(nArg * sizeof(SimNode *));
            pInvoke->nArguments = nArg;
            if ( !expr->isInvokeMethod ) {
                for ( int a=0; a!=nArg; ++a ) {
                    pInvoke->arguments[a] = getE(expr->arguments[a]);
                }
            } else {
                for ( int a=0; a!=nArg; ++a ) {
                    pInvoke->arguments[a] = getE(expr->arguments[a+1]);
                }
            }
        } else {
            pInvoke->arguments = nullptr;
            pInvoke->nArguments = 0;
        }
        setE(expr, sv_keepAlive(at, pInvoke));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprErase * expr) {
        const auto &at = expr->at;
        auto cont = getE(expr->arguments[0]);
        auto val = getE(expr->arguments[1]);
        if ( expr->arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = expr->arguments[0]->type->secondType->getSizeOf();
            Type valueType;
            if ( expr->arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = expr->arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = expr->arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            setE(expr, context.code->makeValueNode<SimNode_TableErase>(valueType, at, cont, val, valueTypeSize));
        } else {
            DAS_ASSERTF(0, "we should not even be here. erase can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating erase for non-table type", "", "", at);
            setE(expr, nullptr);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprSetInsert * expr) {
        const auto &at = expr->at;
        auto cont = getE(expr->arguments[0]);
        auto val = getE(expr->arguments[1]);
        if ( expr->arguments[0]->type->isGoodTableType() ) {
            DAS_ASSERTF(expr->arguments[0]->type->secondType->getSizeOf()==0,"Expecting value type size to be 0 for set insert");
            Type valueType;
            if ( expr->arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = expr->arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = expr->arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            setE(expr, context.code->makeValueNode<SimNode_TableSetInsert>(valueType, at, cont, val));
        } else {
            DAS_ASSERTF(0, "we should not even be here. erase can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating set insert for non-table type", "", "", at);
            setE(expr, nullptr);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprFind * expr) {
        const auto &at = expr->at;
        auto cont = getE(expr->arguments[0]);
        auto val = getE(expr->arguments[1]);
        if ( expr->arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = expr->arguments[0]->type->secondType->getSizeOf();
            Type valueType;
            if ( expr->arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = expr->arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = expr->arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            setE(expr, context.code->makeValueNode<SimNode_TableFind>(valueType, at, cont, val, valueTypeSize));
        } else {
            DAS_ASSERTF(0, "we should not even be here. find can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating find for non-table type", "", "", at);
            setE(expr, nullptr);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprKeyExists * expr) {
        const auto &at = expr->at;
        auto cont = getE(expr->arguments[0]);
        auto val = getE(expr->arguments[1]);
        if ( expr->arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = expr->arguments[0]->type->secondType->getSizeOf();
            Type valueType;
            if ( expr->arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = expr->arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = expr->arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            setE(expr, context.code->makeValueNode<SimNode_KeyExists>(valueType, at, cont, val, valueTypeSize));
        } else {
            DAS_ASSERTF(0, "we should not even be here. find can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating find for non-table type", "", "", at);
            setE(expr, nullptr);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprIs * expr) {
        const auto &at = expr->at;
        DAS_ASSERTF(0, "we should not even be here. 'is' should resolve to const during infer pass.");
        context.thisProgram->error("internal compilation error, generating 'is'", "", "", at);
        setE(expr, nullptr);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprTypeDecl * expr) {
        const auto &at = expr->at;
        setE(expr, context.code->makeNode<SimNode_ConstValue>(at, v_zero()));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprTypeInfo * expr) {
        const auto &at = expr->at;
        if ( !expr->macro ) {
            DAS_ASSERTF(0, "we should not even be here. typeinfo should resolve to const during infer pass.");
            context.thisProgram->error("internal compilation error, generating typeinfo(...)", "", "", at);
            setE(expr, nullptr);
        } else {
            string errors;
            auto node = expr->macro->simluate(&context, (Expression*)expr, errors);
            if ( !node || !errors.empty() ) {
                context.thisProgram->error("typeinfo(" + expr->trait + "...) macro generated no node; " + errors,
                    "", "", at, CompilationError::typeinfo_macro_error);
            }
            setE(expr, node);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprDelete * expr) {
        const auto &at = expr->at;
        uint32_t total = uint32_t(expr->subexpr->type->getCountOf());
        DAS_ASSERTF(total==1,"we should not be deleting more than one at a time");
        auto sube = getE(expr->subexpr);
        if ( expr->subexpr->type->baseType==Type::tArray ) {
            auto stride = expr->subexpr->type->firstType->getSizeOf();
            auto errorMessage = context.code->allocateName(", "+expr->describe());
            setE(expr, context.code->makeNode<SimNode_DeleteArray>(at, sube, total, stride, errorMessage));
        } else if ( expr->subexpr->type->baseType==Type::tTable ) {
            auto vts_add_kts = expr->subexpr->type->firstType->getSizeOf() +
                expr->subexpr->type->secondType->getSizeOf();
            auto errorMessage = context.code->allocateName(", "+expr->describe());
            setE(expr, context.code->makeNode<SimNode_DeleteTable>(at, sube, total, vts_add_kts, errorMessage));
        } else if ( expr->subexpr->type->baseType==Type::tPointer ) {
            if ( expr->subexpr->type->firstType->baseType==Type::tStructure ) {
                bool persistent = expr->subexpr->type->firstType->structType->persistent;
                if ( expr->subexpr->type->firstType->structType->isClass ) {
                    if ( expr->sizeexpr ) {
                        auto sze = getE(expr->sizeexpr);
                        setE(expr, context.code->makeNode<SimNode_DeleteClassPtr>(at, sube, total, sze, persistent));
                    } else {
                        context.thisProgram->error("internal compiler error: SimNode_DeleteClassPtr needs size expression", "", "",
                                                at, CompilationError::missing_node );
                        setE(expr, nullptr);
                    }
                } else {
                    auto structSize = expr->subexpr->type->firstType->getSizeOf();
                    bool isLambda = expr->subexpr->type->firstType->structType->isLambda;
                    setE(expr, context.code->makeNode<SimNode_DeleteStructPtr>(at, sube, total, structSize, persistent, isLambda));
                }
            } else if ( expr->subexpr->type->firstType->baseType==Type::tTuple ) {
                auto structSize = expr->subexpr->type->firstType->getSizeOf();
                setE(expr, context.code->makeNode<SimNode_DeleteStructPtr>(at, sube, total, structSize, false, false));
            } else if ( expr->subexpr->type->firstType->baseType==Type::tVariant ) {
                auto structSize = expr->subexpr->type->firstType->getSizeOf();
                setE(expr, context.code->makeNode<SimNode_DeleteStructPtr>(at, sube, total, structSize, false, false));
            } else {
                auto ann = expr->subexpr->type->firstType->annotation;
                DAS_ASSERT(ann->canDeletePtr() && "has to be able to delete ptr");
                auto resN = ann->simulateDeletePtr(context, at, sube, total);
                if ( !resN ) {
                    context.thisProgram->error("integration error, simulateDelete returned null", "", "",
                                               at, CompilationError::missing_node );
                }
                setE(expr, resN);
            }
        } else if ( expr->subexpr->type->baseType==Type::tHandle ) {
            auto ann = expr->subexpr->type->annotation;
            DAS_ASSERT(ann->canDelete() && "has to be able to delete");
            auto resN = ann->simulateDelete(context, at, sube, total);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateDelete returned null", "", "",
                                           at, CompilationError::missing_node );
            }
            setE(expr, resN);
        } else if ( expr->subexpr->type->baseType==Type::tLambda ) {
            auto errorMessage = context.code->allocateName(", "+expr->describe());
            setE(expr, context.code->makeNode<SimNode_DeleteLambda>(at, sube, total, errorMessage));
        } else {
            DAS_ASSERTF(0, "we should not be here. this is delete for unsupported type. infer types should have failed.");
            context.thisProgram->error("internal compiler error: generating node for unsupported ExprDelete", "", "", at);
            setE(expr, nullptr);
        }
        return expr;
    }

    SimNode * SimulateVisitor::sv_trySimulate_Cast(const ExprCast * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType) {
        return sv_trySimulate(expr->subexpr, extraOffset, r2vType);
    }

    ExpressionPtr SimulateVisitor::visit(ExprCast * expr) {
        setE(expr, getE(expr->subexpr));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprAscend * expr) {
        const auto &at = expr->at;
        auto se = getE(expr->subexpr);
        auto bytes = expr->subexpr->type->getSizeOf();
        TypeInfo * typeInfo = nullptr;
        if ( expr->needTypeInfo ) {
            typeInfo = context.thisHelper->makeTypeInfo(nullptr, expr->subexpr->type);
        }
        SimNode * result;
        if ( expr->subexpr->type->baseType==Type::tHandle ) {
            DAS_ASSERTF(expr->useStackRef,"new of handled type should always be over stackref");
            auto ne = expr->subexpr->type->annotation->simulateGetNew(context, at);
            result = context.code->makeNode<SimNode_AscendNewHandleAndRef>(at, se, ne, bytes, expr->stackTop);
        } else {
            bool persistent = false;
            if ( expr->subexpr->type->baseType==Type::tStructure ) {
                persistent = expr->subexpr->type->structType->persistent;
            }
            if ( expr->useStackRef ) {
                result = context.code->makeNode<SimNode_AscendAndRef>(at, se, bytes, expr->stackTop, typeInfo, persistent);
            } else {
                result = context.code->makeNode<SimNode_Ascend<false>>(at, se, bytes, typeInfo, persistent);
            }
        }
        setE(expr, result);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprNew * expr) {
        const auto &at = expr->at;
        SimNode * newNode;
        if ( expr->typeexpr->baseType == Type::tHandle ) {
            DAS_ASSERT(expr->typeexpr->annotation->canNew() && "how???");
            if ( expr->initializer ) {
                auto pCall = static_cast<SimNode_CallBase *>(expr->func->makeSimNode(context, expr->arguments));
                sv_simulateCall(expr->func, expr, pCall);
                pCall->cmresEval = expr->typeexpr->annotation->simulateGetNew(context, at);
                if ( !pCall->cmresEval ) {
                    context.thisProgram->error("integration error, simulateGetNew returned null", "", "",
                                            at, CompilationError::missing_node );
                }
                setE(expr, pCall);
                return expr;
            } else {
                newNode = expr->typeexpr->annotation->simulateGetNew(context, at);
                if ( !newNode ) {
                    context.thisProgram->error("integration error, simulateGetNew returned null", "", "",
                                            at, CompilationError::missing_node );
                }
            }
        } else {
            bool persistent = false;
            if ( expr->typeexpr->baseType == Type::tStructure ) {
                persistent = expr->typeexpr->structType->persistent;
            }
            int32_t bytes = expr->type->firstType->getBaseSizeOf();
            if ( expr->initializer ) {
                auto pCall = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_NewWithInitializer>(
                    int(expr->arguments.size()), at, bytes, persistent);
                pCall->cmresEval = nullptr;
                sv_simulateCall(expr->func, expr, pCall);
                newNode = pCall;
            } else {
                newNode = context.code->makeNode<SimNode_New>(at, bytes, persistent);
            }
        }
        if ( expr->type->dim.size() ) {
            uint32_t count = expr->type->getCountOf();
            setE(expr, context.code->makeNode<SimNode_NewArray>(at, newNode, expr->stackTop, count));
        } else {
            setE(expr, newNode);
        }
        return expr;
    }

    SimNode * SimulateVisitor::sv_trySimulate_At(const ExprAt * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType) {
        const auto &at = expr->at;
        if ( expr->subexpr->type->isVectorType() ) {
            return nullptr;
        } else if ( expr->subexpr->type->isGoodTableType() ) {
            return nullptr;
        } else if ( expr->subexpr->type->isHandle() ) {
            SimNode * result;
            if ( r2vType->baseType!=Type::none ) {
                result = expr->subexpr->type->annotation->simulateGetAtR2V(context, at, r2vType, expr->subexpr, expr->index, extraOffset);
                if ( !result ) {
                    context.thisProgram->error("integration error, simulateGetAtR2V returned null", "", "",
                                               at, CompilationError::missing_node );
                }
            } else {
                result = expr->subexpr->type->annotation->simulateGetAt(context, at, r2vType, expr->subexpr, expr->index, extraOffset);
                if ( !result ) {
                    context.thisProgram->error("integration error, simulateGetAt returned null", "", "",
                                               at, CompilationError::missing_node );
                }
            }
            return result;
        } else if ( expr->subexpr->type->isGoodArrayType() ) {
            auto prv = simulateExpression(expr->subexpr);
            auto pidx = simulateExpression(expr->index);
            uint32_t stride = expr->subexpr->type->firstType->getSizeOf();
            if ( r2vType->baseType!=Type::none ) {
                return context.code->makeValueNode<SimNode_ArrayAtR2V>(r2vType->getR2VType(), at, prv, pidx, stride, extraOffset);
            } else {
                return context.code->makeNode<SimNode_ArrayAt>(at, prv, pidx, stride, extraOffset);
            }
        } else if ( expr->subexpr->type->isPointer() ) {
            uint32_t stride = expr->subexpr->type->firstType->getSizeOf();
            auto prv = simulateExpression(expr->subexpr);
            auto pidx = simulateExpression(expr->index);
            if ( r2vType->baseType!=Type::none ) {
                switch ( expr->index->type->baseType ) {
                case Type::tInt:    return context.code->makeValueNode<SimNode_PtrAtR2V_Int>(r2vType->getR2VType(), at, prv, pidx, stride, extraOffset);
                case Type::tUInt:   return context.code->makeValueNode<SimNode_PtrAtR2V_UInt>(r2vType->getR2VType(), at, prv, pidx, stride, extraOffset);
                case Type::tInt64:  return context.code->makeValueNode<SimNode_PtrAtR2V_Int64>(r2vType->getR2VType(), at, prv, pidx, stride, extraOffset);
                case Type::tUInt64: return context.code->makeValueNode<SimNode_PtrAtR2V_UInt64>(r2vType->getR2VType(), at, prv, pidx, stride, extraOffset);
                default:
                    context.thisProgram->error("internal compilation error, generating ptr at for unsupported index type " + expr->index->type->describe(), "", "", at);
                    return nullptr;
                };
            } else {
                switch ( expr->index->type->baseType ) {
                case Type::tInt:    return context.code->makeNode<SimNode_PtrAt<int32_t>>(at, prv, pidx, stride, extraOffset);
                case Type::tUInt:   return context.code->makeNode<SimNode_PtrAt<uint32_t>>(at, prv, pidx, stride, extraOffset);
                case Type::tInt64:  return context.code->makeNode<SimNode_PtrAt<int64_t>>(at, prv, pidx, stride, extraOffset);
                case Type::tUInt64: return context.code->makeNode<SimNode_PtrAt<uint64_t>>(at, prv, pidx, stride, extraOffset);
                default:
                    context.thisProgram->error("internal compilation error, generating ptr at for unsupported index type " + expr->index->type->describe(), "", "", at);
                    return nullptr;
                };
            }
        } else {
            uint32_t range = expr->subexpr->type->dim[0];
            uint32_t stride = expr->subexpr->type->getStride();
            if ( expr->index->rtti_isConstant() ) {
            // if its constant index, like a[3]..., we try to let node bellow simulate
                auto idxCE = static_cast<ExprConst*>(expr->index);
                uint32_t idxC = cast<uint32_t>::to(idxCE->value);
                if ( idxC >= range ) {
                    context.thisProgram->error("index out of range " + to_string(idxC) + " of " + to_string(range) + ", " + expr->describe(), "", "",
                        at, CompilationError::index_out_of_range);
                    return nullptr;
                }
                auto tnode = sv_trySimulate(expr->subexpr, extraOffset + idxC*stride, r2vType);
                if ( tnode ) {
                    return tnode;
                }
            }
            // regular scenario
            auto prv = simulateExpression(expr->subexpr);
            auto pidx = simulateExpression(expr->index);
            auto errorMessage = context.code->allocateName(", "+expr->describe());
            if ( r2vType->baseType!=Type::none ) {
                return context.code->makeValueNode<SimNode_AtR2V>(r2vType->getR2VType(), at, prv, pidx, stride, extraOffset, range, errorMessage);
            } else {
                return context.code->makeNode<SimNode_At>(at, prv, pidx, stride, extraOffset, range, errorMessage);
            }
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprAt * expr) {
        const auto &at = expr->at;
        if ( expr->subexpr->type->isVectorType() ) {
            auto prv = getE(expr->subexpr);
            auto pidx = getE(expr->index);
            uint32_t range = expr->subexpr->type->getVectorDim();
            uint32_t stride = expr->type->getSizeOf();
            auto errorMessage = context.code->allocateName(", "+expr->describe());
            if ( expr->subexpr->type->ref ) {
                auto res = context.code->makeNode<SimNode_At>(at, prv, pidx, stride, 0, range, errorMessage);
                if ( expr->r2v ) {
                    setE(expr, GetR2V(context, at, expr->type, res));
                } else {
                    setE(expr, res);
                }
            } else {
                switch ( expr->type->baseType ) {
                    case tInt:      setE(expr, (SimNode *) context.code->makeNode<SimNode_AtVector<int32_t>>(at, prv, pidx, range, errorMessage)); break;
                    case tInt64:    setE(expr, (SimNode *) context.code->makeNode<SimNode_AtVector<int64_t>>(at, prv, pidx, range, errorMessage)); break;
                    case tUInt:
                    case tBitfield:
                                    setE(expr, (SimNode *) context.code->makeNode<SimNode_AtVector<uint32_t>>(at, prv, pidx, range, errorMessage)); break;
                    case tUInt64:
                    case tBitfield64:
                                    setE(expr, (SimNode *) context.code->makeNode<SimNode_AtVector<uint64_t>>(at, prv, pidx, range, errorMessage)); break;
                    case tFloat:    setE(expr, (SimNode *) context.code->makeNode<SimNode_AtVector<float>>(at, prv, pidx, range, errorMessage)); break;
                    default:
                        DAS_ASSERTF(0, "we should not even be here. infer type should have failed on unsupported_vector[blah]");
                        context.thisProgram->error("internal compilation error, generating vector at for unsupported vector type.", "", "", at);
                        setE(expr, nullptr);
                }
            }
        } else if ( expr->subexpr->type->isGoodTableType() ) {
            auto prv = getE(expr->subexpr);
            auto pidx = getE(expr->index);
            uint32_t valueTypeSize = expr->subexpr->type->secondType->getSizeOf();
            Type keyType;
            if ( expr->subexpr->type->firstType->isWorkhorseType() ) {
                keyType = expr->subexpr->type->firstType->baseType;
            } else {
                auto keyValueType = expr->subexpr->type->firstType->annotation->makeValueType();
                keyType = keyValueType->baseType;
                pidx = context.code->makeNode<SimNode_CastToWorkhorse>(at, pidx);
            }
            auto res = context.code->makeValueNode<SimNode_TableIndex>(keyType, at, prv, pidx, valueTypeSize, 0);
            if ( expr->r2v ) {
                setE(expr, GetR2V(context, at, expr->type, res));
            } else {
                setE(expr, res);
            }
        } else {
            if ( expr->r2v ) {
                setE(expr, sv_trySimulate(expr, 0, expr->type));
            } else {
                gc_local<TypeDecl> noneType = new TypeDecl(Type::none);
                setE(expr, sv_trySimulate(expr, 0, noneType));
            }
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprSafeAt * expr) {
        const auto &at = expr->at;
        if ( expr->subexpr->type->isPointer() ) {
            const auto & seT = expr->subexpr->type->firstType;
            if ( seT->isGoodArrayType() ) {
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                uint32_t stride = seT->firstType->getSizeOf();
                setE(expr, context.code->makeNode<SimNode_SafeArrayAt>(at, prv, pidx, stride, 0));
            } else if ( seT->isGoodTableType() ) {
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                uint32_t valueTypeSize = seT->secondType->getSizeOf();
                Type valueType;
                if ( seT->firstType->isWorkhorseType() ) {
                    valueType = seT->firstType->baseType;
                } else {
                    auto valueT = seT->firstType->annotation->makeValueType();
                    valueType = valueT->baseType;
                    pidx = context.code->makeNode<SimNode_CastToWorkhorse>(at, pidx);
                }
                setE(expr, context.code->makeValueNode<SimNode_SafeTableIndex>(valueType, at, prv, pidx, valueTypeSize, 0));
            } else if ( seT->dim.size() ) {
                uint32_t range = seT->dim[0];
                uint32_t stride = seT->getStride();
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                setE(expr, context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range));
            } else if ( seT->isVectorType() ) {
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                uint32_t range = seT->getVectorDim();
                uint32_t stride = expr->type->firstType->getSizeOf();
                setE(expr, context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range));
            } else {
                // pointer safe-at: null check + pointer arithmetic, no bounds check
                uint32_t stride = seT->getSizeOf();
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                switch ( expr->index->type->baseType ) {
                case Type::tInt:    setE(expr, context.code->makeNode<SimNode_PtrSafeAt<int32_t>>(at, prv, pidx, stride, 0)); break;
                case Type::tUInt:   setE(expr, context.code->makeNode<SimNode_PtrSafeAt<uint32_t>>(at, prv, pidx, stride, 0)); break;
                case Type::tInt64:  setE(expr, context.code->makeNode<SimNode_PtrSafeAt<int64_t>>(at, prv, pidx, stride, 0)); break;
                case Type::tUInt64: setE(expr, context.code->makeNode<SimNode_PtrSafeAt<uint64_t>>(at, prv, pidx, stride, 0)); break;
                default:
                    DAS_VERIFY(0 && "unsupported index type for pointer safe-at");
                    setE(expr, nullptr);
                }
            }
        } else {
            const auto & seT = expr->subexpr->type;
            if ( seT->isGoodArrayType() ) {
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                uint32_t stride = seT->firstType->getSizeOf();
                setE(expr, context.code->makeNode<SimNode_SafeArrayAt>(at, prv, pidx, stride, 0));
            } else if ( expr->subexpr->type->isGoodTableType() ) {
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                uint32_t valueTypeSize = seT->secondType->getSizeOf();
                Type valueType;
                if ( seT->firstType->isWorkhorseType() ) {
                    valueType = seT->firstType->baseType;
                } else {
                    auto valueT = seT->firstType->annotation->makeValueType();
                    valueType = valueT->baseType;
                    pidx = context.code->makeNode<SimNode_CastToWorkhorse>(at, pidx);
                }
                setE(expr, context.code->makeValueNode<SimNode_SafeTableIndex>(valueType, at, prv, pidx, valueTypeSize, 0));
            } else if ( seT->dim.size() ) {
                uint32_t range = seT->dim[0];
                uint32_t stride = seT->getStride();
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                setE(expr, context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range));
            } else if ( seT->isVectorType() && seT->ref ) {
                auto prv = getE(expr->subexpr);
                auto pidx = getE(expr->index);
                uint32_t range = seT->getVectorDim();
                uint32_t stride = getTypeBaseSize(seT->getVectorBaseType());
                setE(expr, context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range));
            } else {
                DAS_VERIFY(0 && "TODO: safe-at not implemented");
                setE(expr, nullptr);
            }
        }
        return expr;
    }

    vector<SimNode *> SimulateVisitor::sv_collectExpressions ( const vector<ExpressionPtr> & lis,
                                                               das_map<int32_t,uint32_t> * ofsmap ) {
        vector<SimNode *> simlist;
        for ( auto & node : lis ) {
            if ( node->rtti_isLet()) {
                auto pLet = static_cast<ExprLet*>(node);
                auto it = letInitVec.find(pLet);
                DAS_ASSERTF(it != letInitVec.end(), "collectExpressions: let init nodes not found");
                simlist.insert(simlist.end(), it->second.begin(), it->second.end());
                continue;
            }
            if ( node->rtti_isLabel() ) {
                if ( ofsmap ) {
                    auto lnode = static_cast<ExprLabel*>(node);
                    (*ofsmap)[lnode->label] = uint32_t(simlist.size());
                }
                continue;
            }
            auto it = e2v.find(node);
            if ( it != e2v.end() && it->second ) {
                simlist.push_back(it->second);
            }
        }
        return simlist;
    }

    void SimulateVisitor::sv_simulateFinal ( const ExprBlock * expr, SimNode_Final * block ) {
        vector<SimNode *> simFList = sv_collectExpressions(expr->finalList);
        block->totalFinal = int(simFList.size());
        if ( block->totalFinal ) {
            block->finalList = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->totalFinal);
            for ( uint32_t i=0, is=block->totalFinal; i!=is; ++i )
                block->finalList[i] = simFList[i];
        }
    }

    void SimulateVisitor::sv_simulateBlock ( const ExprBlock * expr, SimNode_Block * block ) {
        das_map<int32_t,uint32_t> ofsmap;
        vector<SimNode *> simlist = sv_collectExpressions(expr->list, &ofsmap);
        block->total = int(simlist.size());
        if ( block->total ) {
            block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
            for ( uint32_t i=0, is=block->total; i!=is; ++i )
                block->list[i] = simlist[i];
        }
        sv_simulateLabels(expr, block, ofsmap);
    }

    void SimulateVisitor::sv_simulateLabels ( const ExprBlock * expr, SimNode_Block * block, const das_map<int32_t,uint32_t> & ofsmap ) {
        if ( expr->maxLabelIndex!=-1 ) {
            block->totalLabels = expr->maxLabelIndex + 1;
            block->labels = (uint32_t *) context.code->allocate(block->totalLabels * sizeof(uint32_t));
            for ( uint32_t i=0, is=block->totalLabels; i!=is; ++i ) {
                block->labels[i] = -1U;
            }
            for ( auto & it : ofsmap ) {
                block->labels[it.first] = it.second;
            }
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprBlock * expr) {
        das_map<int32_t,uint32_t> ofsmap;
        const auto &at = expr->at;
        vector<SimNode *> simlist = sv_collectExpressions(expr->list, &ofsmap);
        // wow, such empty
        if ( expr->finalList.size()==0 && simlist.size()==0 && !expr->annotationDataSid ) {
            setE(expr, context.code->makeNode<SimNode_NOP>(at));
            return expr;
        }
        // we memzero block's stack memory, if there is a finally section
        // bad scenario we fight is ( in scope X ; return ; in scope Y )
        if ( expr->finalList.size() ) {
            uint32_t blockDataSize = expr->stackVarBottom - expr->stackVarTop;
            if ( blockDataSize ) {
                for ( const auto & svr : expr->stackCleanVars ) {
                    SimNode * fakeVar = context.code->makeNode<SimNode_GetLocal>(at, svr.first);
                    SimNode * memZ = context.code->makeNode<SimNode_MemZero>(at, fakeVar, svr.second );
                    simlist.insert( simlist.begin(), memZ );
                }
            }
        }
        // TODO: what if list size is 0?
        if ( simlist.size()!=1 || expr->isClosure || expr->finalList.size() || (expr->maxLabelIndex!=-1) ) {
            SimNode_Block * block;
            if ( expr->isClosure ) {
                bool needResult = expr->type!=nullptr && expr->type->baseType!=Type::tVoid;
                bool C0 = !needResult && simlist.size()==1 && expr->finalList.size()==0;
#if DAS_DEBUGGER
                if ( context.debugger ) {
                    block = context.code->makeNode<SimNodeDebug_ClosureBlock>(at, needResult, C0, expr->annotationData);
                } else
#endif
                {
                    block = context.code->makeNode<SimNode_ClosureBlock>(at, needResult, C0, expr->annotationData);
                }
            } else {
                if ( expr->maxLabelIndex!=-1 ) {
#if DAS_DEBUGGER
                    if ( context.debugger ) {
                        block = context.code->makeNode<SimNodeDebug_BlockWithLabels>(at);
                    } else
#endif
                    {
                        block = context.code->makeNode<SimNode_BlockWithLabels>(at);
                    }
                    sv_simulateLabels(expr, block, ofsmap);
                } else {
                    if ( expr->finalList.size()==0 ) {
#if DAS_DEBUGGER
                        if ( context.debugger ) {
                            block = context.code->makeNode<SimNodeDebug_BlockNF>(at);
                        } else
#endif
                        {
                            auto lsize = int(simlist.size());
                            block = (SimNode_Block *) context.code->makeNodeUnrollAny<SimNode_BlockNFT>(lsize, at);
                        }
                    } else {
#if DAS_DEBUGGER
                        if ( context.debugger ) {
                            block = context.code->makeNode<SimNodeDebug_Block>(at);
                        } else
#endif
                        {
                            block = context.code->makeNode<SimNode_Block>(at);
                        }
                    }
                }
            }
            block->annotationDataSid = expr->annotationDataSid;
            block->total = int(simlist.size());
            if ( block->total ) {
                block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
                for ( uint32_t i=0, is=block->total; i!=is; ++i )
                    block->list[i] = simlist[i];
            }
            if ( !expr->inTheLoop ) {
                sv_simulateFinal(expr, block);
            }
            setE(expr, block);
        } else {
            setE(expr, simlist[0]);
        }
        return expr;
    }

    SimNode * SimulateVisitor::sv_trySimulate_Swizzle(const ExprSwizzle * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType) {
        if ( !expr->value->type->ref ) {
            return nullptr;
        }
        const auto &at = expr->at;
        int offset = expr->value->type->getVectorFieldOffset(expr->fields[0]);
        if  (offset==-1 ) {
            context.thisProgram->error("internal compilation error, swizzle field offset of unsupported type", "", "", at);
            return nullptr;
        }
        if ( auto chain = sv_trySimulate(expr->value, uint32_t(offset) + extraOffset, r2vType) ) {
            return chain;
        }
        auto simV = getE(expr->value);
        if ( r2vType->baseType!=Type::none ) {
            return context.code->makeValueNode<SimNode_FieldDerefR2V>(r2vType->getR2VType(), at, simV, uint32_t(offset) + extraOffset);
        } else {
            return context.code->makeNode<SimNode_FieldDeref>(at, simV, uint32_t(offset) + extraOffset);
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprSwizzle * expr) {
        const auto &at = expr->at;
        if ( !expr->type->ref ) {
            bool seq = TypeDecl::isSequencialMask(expr->fields);
            if (seq && expr->value->type->ref) {
                setE(expr, sv_trySimulate(expr, 0, expr->type));
            } else {
                auto fsz = expr->fields.size();
                uint8_t fs[4];
                fs[0] = expr->fields[0];
                fs[1] = fsz >= 2 ? expr->fields[1] : expr->fields[0];
                fs[2] = fsz >= 3 ? expr->fields[2] : expr->fields[0];
                fs[3] = fsz >= 4 ? expr->fields[3] : expr->fields[0];
                auto simV = getE(expr->value);
                if ( expr->type->baseType==Type::tInt64 || expr->type->baseType==Type::tUInt64
                    || expr->type->baseType==Type::tRange64 || expr->type->baseType==Type::tURange64 ) {
                    setE(expr, context.code->makeNode<SimNode_Swizzle64>(at, simV, fs));
                } else {
                    setE(expr, context.code->makeNode<SimNode_Swizzle>(at, simV, fs));
                }
            }
        } else {
            if ( expr->r2v ) {
                setE(expr, sv_trySimulate(expr, 0, expr->type));
            } else {
                gc_local<TypeDecl> noneType = new TypeDecl(Type::none);
                setE(expr, sv_trySimulate(expr, 0, noneType));
            }
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprField * expr) {
        const auto &at = expr->at;
        if ( expr->value->type->isBitfield() ) {
            auto simV = getE(expr->value);
            uint32_t mask = 1u << expr->fieldIndex;
            setE(expr, context.code->makeNode<SimNode_GetBitField>(at, simV, mask));
        } else {
            if ( expr->r2v ) {
                setE(expr, sv_trySimulate(expr, 0, expr->type));
            } else {
                gc_local<TypeDecl> noneType = new TypeDecl(Type::none);
                setE(expr, sv_trySimulate(expr, 0, noneType));
            }
        }
        return expr;
    }

    SimNode * SimulateVisitor::sv_trySimulate_Field(const ExprField * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType) {
        const auto &at = expr->at;
        if ( expr->value->type->isBitfield() ) {
            return nullptr;
        }
        int fieldOffset = -1;
        if ( !expr->fieldRef && expr->fieldIndex==-1 ) {
            fieldOffset = (int) expr->annotation->getFieldOffset(expr->name);
        } else if ( expr->fieldIndex != - 1 ) {
            if ( expr->value->type->isPointer() ) {
                if ( expr->value->type->firstType->isVariant() ) {
                    fieldOffset = expr->value->type->firstType->getVariantFieldOffset(expr->fieldIndex);
                } else {
                    fieldOffset = expr->value->type->firstType->getTupleFieldOffset(expr->fieldIndex);
                }
            } else {
                if ( expr->value->type->isVariant() ) {
                    fieldOffset = expr->value->type->getVariantFieldOffset(expr->fieldIndex);
                } else {
                    fieldOffset = expr->value->type->getTupleFieldOffset(expr->fieldIndex);
                }
            }
        } else {
            DAS_ASSERTF(expr->fieldRef, "field can't be null");
            if (!expr->fieldRef) return nullptr;
            fieldOffset = expr->fieldRef->offset;
        }
        DAS_ASSERTF(fieldOffset>=0,"field offset is somehow not there");
        if (expr->value->type->isPointer()) {
            if ( expr->unsafeDeref ) {
                auto simV = simulateExpression(expr->value);
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_FieldDerefR2V>(r2vType->getR2VType(), at, simV, fieldOffset + extraOffset);
                } else {
                    return context.code->makeNode<SimNode_FieldDeref>(at, simV, fieldOffset + extraOffset);
                }
            } else {
                auto simV = simulateExpression(expr->value);
                auto errorMessage = context.code->allocateName(", "+expr->value->describe()+" is null");
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_PtrFieldDerefR2V>(r2vType->getR2VType(), at, simV, fieldOffset + extraOffset, errorMessage);
                } else {
                    return context.code->makeNode<SimNode_PtrFieldDeref>(at, simV, fieldOffset + extraOffset, errorMessage);
                }
            }
        } else {
            if ( auto chain = sv_trySimulate(expr->value, extraOffset + fieldOffset, r2vType) ) {
                return chain;
            }
            auto simV = simulateExpression(expr->value);
            if ( r2vType->baseType!=Type::none ) {
                return context.code->makeValueNode<SimNode_FieldDerefR2V>(r2vType->getR2VType(), at, simV, extraOffset + fieldOffset);
            } else {
                return context.code->makeNode<SimNode_FieldDeref>(at, simV, extraOffset + fieldOffset);
            }
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprIsVariant * expr) {
        const auto &at = expr->at;
        DAS_ASSERT(expr->fieldIndex != -1);
        setE(expr, context.code->makeNode<SimNode_IsVariant>(at, getE(expr->value), expr->fieldIndex));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprAsVariant * expr) {
        const auto &at = expr->at;
        int fieldOffset = expr->value->type->getVariantFieldOffset(expr->fieldIndex);
        auto simV = getE(expr->value);
        auto errorMessage = context.code->allocateName(", "+expr->value->describe()+" is not '"+expr->name+"'");
        if ( expr->r2v ) {
            setE(expr, context.code->makeValueNode<SimNode_VariantFieldDerefR2V>(expr->type->getR2VType(), at, simV, fieldOffset, expr->fieldIndex, errorMessage));
        } else {
            setE(expr, context.code->makeNode<SimNode_VariantFieldDeref>(at, simV, fieldOffset, expr->fieldIndex, errorMessage));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprSafeAsVariant * expr) {
        const auto &at = expr->at;
        int fieldOffset = expr->value->type->isPointer() ?
            expr->value->type->firstType->getVariantFieldOffset(expr->fieldIndex) :
            expr->value->type->getVariantFieldOffset(expr->fieldIndex);
        auto simV = getE(expr->value);
        if ( expr->skipQQ ) {
            setE(expr, context.code->makeNode<SimNode_SafeVariantFieldDerefPtr>(at, simV, fieldOffset, expr->fieldIndex));
        } else {
            setE(expr, context.code->makeNode<SimNode_SafeVariantFieldDeref>(at, simV, fieldOffset, expr->fieldIndex));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprSafeField * expr) {
        const auto &at = expr->at;
        int fieldOffset = -1;
        if ( !expr->annotation ) {
            if ( expr->fieldIndex != - 1 ) {
                if ( expr->value->type->firstType->isVariant() ) {
                    fieldOffset = expr->value->type->firstType->getVariantFieldOffset(expr->fieldIndex);
                } else {
                    fieldOffset = expr->value->type->firstType->getTupleFieldOffset(expr->fieldIndex);
                }
            } else {
                fieldOffset = expr->fieldRef->offset;
            }
            DAS_ASSERTF(fieldOffset>=0,"field offset is somehow not there");
        }
        if ( expr->annotation ) fieldOffset = (int) expr->annotation->getFieldOffset(expr->name);
        if ( expr->skipQQ ) {
            setE(expr, context.code->makeNode<SimNode_SafeFieldDerefPtr>(at, getE(expr->value), fieldOffset));
        } else {
            setE(expr, context.code->makeNode<SimNode_SafeFieldDeref>(at, getE(expr->value), fieldOffset));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprStringBuilder * expr) {
        const auto &at = expr->at;
        SimNode_StringBuilder * pSB = context.code->makeNode<SimNode_StringBuilder>(expr->isTempString, at);
        if ( int nArg = (int) expr->elements.size() ) {
            pSB->arguments = (SimNode **) context.code->allocate(nArg * sizeof(SimNode *));
            pSB->types = (TypeInfo **) context.code->allocate(nArg * sizeof(TypeInfo *));
            pSB->nArguments = nArg;
            for ( int a=0; a!=nArg; ++a ) {
                pSB->arguments[a] = getE(expr->elements[a]);
                pSB->types[a] = context.thisHelper->makeTypeInfo(nullptr, expr->elements[a]->type);
            }
        } else {
            pSB->arguments = nullptr;
            pSB->types = nullptr;
            pSB->nArguments = 0;
        }
        setE(expr, pSB);
        return expr;
    }


    SimNode * SimulateVisitor::sv_trySimulate_Var(const ExprVar * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType) {
        const auto &at = expr->at;
        if ( expr->block ) {
        } else if ( expr->local ) {
            if ( expr->variable->type->ref ) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetLocalRefOffR2V>(r2vType->getR2VType(), at,
                                                    expr->variable->stackTop, extraOffset + expr->variable->extraLocalOffset);
                } else {
                    return context.code->makeNode<SimNode_GetLocalRefOff>(at,
                                                    expr->variable->stackTop, extraOffset + expr->variable->extraLocalOffset);
                }
            } else if ( expr->variable->aliasCMRES ) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetCMResOfsR2V>(r2vType->getR2VType(), at, extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetCMResOfs>(at, extraOffset);
                }
            } else {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetLocalR2V>(r2vType->getR2VType(), at,
                                                                            expr->variable->stackTop + extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetLocal>(at, expr->variable->stackTop + extraOffset);
                }
            }
        } else if ( expr->argument ) {
            if ( expr->variable->type->isPointer() && expr->variable->type->isRef() ) {
                return nullptr;
            } else if ( expr->variable->type->isPointer() ) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetArgumentRefOffR2V>(r2vType->getR2VType(), at, expr->argumentIndex, extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetArgumentRefOff>(at, expr->argumentIndex, extraOffset);
                }
            } else if (expr->variable->type->isRef()) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetArgumentRefOffR2V>(r2vType->getR2VType(), at, expr->argumentIndex, extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetArgumentRefOff>(at, expr->argumentIndex, extraOffset);
                }
            }
        } else { // global
        }
        return nullptr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprVar * expr) {
        const auto &at = expr->at;
        if ( expr->block ) {
            auto blk = expr->pBlock;
            if (expr->variable->type->isRef()) {
                if (expr->r2v && !expr->type->isRefType()) {
                    if ( expr->thisBlock ) {
                        setE(expr, context.code->makeValueNode<SimNode_GetThisBlockArgumentR2V>(expr->type->getR2VType(), at, expr->argumentIndex));
                    } else {
                        setE(expr, context.code->makeValueNode<SimNode_GetBlockArgumentR2V>(expr->type->getR2VType(), at, expr->argumentIndex, blk->stackTop));
                    }
                } else {
                    if ( expr->thisBlock ) {
                        setE(expr, context.code->makeNode<SimNode_GetThisBlockArgument>(at, expr->argumentIndex));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetBlockArgument>(at, expr->argumentIndex, blk->stackTop));
                    }
                }
            } else {
                if (expr->r2v && !expr->type->isRefType()) {
                    if ( expr->thisBlock ) {
                        setE(expr, context.code->makeNode<SimNode_GetThisBlockArgument>(at, expr->argumentIndex));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetBlockArgument>(at, expr->argumentIndex, blk->stackTop));
                    }
                }
                else {
                    if ( expr->thisBlock ) {
                        setE(expr, context.code->makeNode<SimNode_GetThisBlockArgumentRef>(at, expr->argumentIndex));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetBlockArgumentRef>(at, expr->argumentIndex, blk->stackTop));
                    }
                }
            }
        } else if ( expr->local ) {
            if ( expr->r2v ) {
                setE(expr, sv_trySimulate(expr, expr->variable->extraLocalOffset, expr->type));
            } else {
                gc_local<TypeDecl> noneType = new TypeDecl(Type::none);
                setE(expr, sv_trySimulate(expr, expr->variable->extraLocalOffset, noneType));
            }
        } else if ( expr->argument) {
            if (expr->variable->type->isRef()) {
                if (expr->r2v && !expr->type->isRefType()) {
                    setE(expr, context.code->makeValueNode<SimNode_GetArgumentR2V>(expr->type->getR2VType(), at, expr->argumentIndex));
                } else {
                    setE(expr, context.code->makeNode<SimNode_GetArgument>(at, expr->argumentIndex));
                }
            } else {
                if (expr->r2v && !expr->type->isRefType()) {
                    setE(expr, context.code->makeNode<SimNode_GetArgument>(at, expr->argumentIndex));
                }
                else {
                    setE(expr, context.code->makeNode<SimNode_GetArgumentRef>(at, expr->argumentIndex));
                }
            }
        } else {
            DAS_ASSERT(expr->variable->index >= 0 && "using variable which is not used. how?");
            uint64_t mnh = expr->variable->getMangledNameHash();
            if ( !expr->variable->module->isSolidContext ) {
                if ( expr->variable->global_shared ) {
                    if ( expr->r2v ) {
                        setE(expr, context.code->makeValueNode<SimNode_GetSharedMnhR2V>(expr->type->getR2VType(), at, expr->variable->stackTop, mnh));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetSharedMnh>(at, expr->variable->stackTop, mnh));
                    }
                } else {
                    if ( expr->r2v ) {
                        setE(expr, context.code->makeValueNode<SimNode_GetGlobalMnhR2V>(expr->type->getR2VType(), at, expr->variable->stackTop, mnh));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetGlobalMnh>(at, expr->variable->stackTop, mnh));
                    }
                }
            } else {
                if ( expr->variable->global_shared ) {
                    if ( expr->r2v ) {
                        setE(expr, context.code->makeValueNode<SimNode_GetSharedR2V>(expr->type->getR2VType(), at, expr->variable->stackTop, mnh));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetShared>(at, expr->variable->stackTop, mnh));
                    }
                } else {
                    if ( expr->r2v ) {
                        setE(expr, context.code->makeValueNode<SimNode_GetGlobalR2V>(expr->type->getR2VType(), at, expr->variable->stackTop, mnh));
                    } else {
                        setE(expr, context.code->makeNode<SimNode_GetGlobal>(at, expr->variable->stackTop, mnh));
                    }
                }
            }
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprOp1 * expr) {
        const auto &at = expr->at;
        vector<ExpressionPtr> sarguments = { expr->subexpr };
        if ( expr->func->builtIn && !expr->func->callBased ) {
            auto pSimOp1 = static_cast<SimNode_Op1 *>(expr->func->makeSimNode(context, sarguments));
            pSimOp1->debugInfo = at;
            pSimOp1->x = getE(expr->subexpr);
            setE(expr, pSimOp1);
        } else {
            auto pCall = static_cast<SimNode_CallBase *>(expr->func->makeSimNode(context, sarguments));
            pCall->debugInfo = at;
            pCall->fnPtr = context.getFunction(expr->func->index);
            pCall->arguments = (SimNode **) context.code->allocate(1 * sizeof(SimNode *));
            pCall->nArguments = 1;
            pCall->arguments[0] = getE(expr->subexpr);
            pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at, expr->stackTop);
            setE(expr, pCall);
        }
        return expr;
    }

    // we flatten or(or(or(a,b),c),d) into a single vector {a,b,c,d}
    void flattenOrSequence ( const ExprOp2 * op, vector<ExpressionPtr> & ret ) {
        if ( op->left->rtti_isOp2() && static_cast<ExprOp2*>(op->left)->op==op->op ) {
            flattenOrSequence(static_cast<ExprOp2*>(op->left), ret);
        } else {
            ret.push_back(op->left);
        }
        if ( op->right->rtti_isOp2() && static_cast<ExprOp2*>(op->right)->op==op->op ) {
            flattenOrSequence(static_cast<ExprOp2*>(op->right), ret);
        } else {
            ret.push_back(op->right);
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprOp2 * expr) {
        const auto &at = expr->at;
        // special case for logical operator sequence
        if ( expr->func->builtIn && expr->func->module->name=="$" && expr->func->arguments[0]->type->isBool() ) {
            if ( expr->func->name=="||" || expr->func->name=="&&" ) {
                vector<ExpressionPtr> flat;
                flattenOrSequence(expr, flat);
                if ( flat.size()>2 && flat.size()<=7 ) {
                    SimNode_CallBase * pSimNode;
                    if ( expr->func->name=="||" ) {
                        pSimNode = (SimNode_OrAny *) context.code->makeNodeUnrollAny<SimNode_OrT>((int32_t)flat.size(), at);
                    } else if ( expr->func->name=="&&" ) {
                        pSimNode = (SimNode_AndAny *) context.code->makeNodeUnrollAny<SimNode_AndT>((int32_t)flat.size(), at);
                    } else {
                        DAS_ASSERTF(0, "unknown logical operator");
                        setE(expr, nullptr);
                        return expr;
                    }
                    pSimNode->nArguments = (uint32_t) flat.size();
                    pSimNode->arguments = (SimNode **) context.code->allocate((uint32_t)(flat.size() * sizeof(SimNode *)));
                    for ( uint32_t i=0; i!=uint32_t(flat.size()); ++i ) {
                        pSimNode->arguments[i] = getE(flat[i]);
                    }
                    setE(expr, pSimNode);
                    return expr;
                }
            }
        }
        vector<ExpressionPtr> sarguments = { expr->left, expr->right };
        if ( expr->func->builtIn && !expr->func->callBased ) {
            auto pSimOp2 = static_cast<SimNode_Op2 *>(expr->func->makeSimNode(context, sarguments));
            pSimOp2->debugInfo = at;
            pSimOp2->l = getE(expr->left);
            pSimOp2->r = getE(expr->right);
            setE(expr, pSimOp2);
        } else {
            auto pCall = static_cast<SimNode_CallBase *>(expr->func->makeSimNode(context, sarguments));
            pCall->debugInfo = at;
            pCall->fnPtr = context.getFunction(expr->func->index);
            pCall->arguments = (SimNode **) context.code->allocate(2 * sizeof(SimNode *));
            pCall->nArguments = 2;
            pCall->arguments[0] = getE(expr->left);
            pCall->arguments[1] = getE(expr->right);
            pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at, expr->stackTop);
            setE(expr, pCall);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprOp3 * expr) {
        const auto &at = expr->at;
        setE(expr, context.code->makeNode<SimNode_IfThenElse>(at,
            getE(expr->subexpr), getE(expr->left), getE(expr->right)));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprTag * expr) {
        const auto &at = expr->at;
        context.thisProgram->error("internal compilation error, trying to simulate a tag", "", "", at);
        setE(expr, nullptr);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprMove * expr) {
        const auto &at = expr->at;
        if ( expr->takeOverRightStack ) {
            auto sl = getE(expr->left);
            auto sr = getE(expr->right);
            setE(expr, context.code->makeNode<SimNode_SetLocalRefAndEval>(at, sl, sr, expr->stackTop));
        } else {
            auto retN = sv_makeMove(at, expr->left, expr->right);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate move", "", "", at);
            }
            setE(expr, retN);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprClone * expr) {
        const auto &at = expr->at;
        SimNode * retN = nullptr;
        if ( expr->left->type->isHandle() ) {
            auto lN = getE(expr->left);
            auto rN = getE(expr->right);
            auto ta = ((TypeAnnotation *)(expr->right->type->annotation));
            if ( !ta->isRefType() && expr->right->type->isRef() ) {
                rN = context.code->makeValueNode<SimNode_Ref2Value>(expr->right->type->getR2VType(), at, rN);
            }
            retN = expr->left->type->annotation->simulateClone(context, at, lN, rN);
        } else if ( expr->left->type->canCopy() ) {
            retN = sv_makeCopy(at, expr->left, expr->right);
        }
        if ( !retN ) {
            context.thisProgram->error("internal compilation error, can't generate clone", "", "", at);
        }
        setE(expr, retN);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprCopy * expr) {
        const auto &at = expr->at;
        if ( expr->takeOverRightStack ) {
            auto sl = getE(expr->left);
            auto sr = getE(expr->right);
            setE(expr, context.code->makeNode<SimNode_SetLocalRefAndEval>(at, sl, sr, expr->stackTop));
        } else {
            auto retN = sv_makeCopy(at, expr->left, expr->right);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate copy", "", "", at);
            }
            setE(expr, retN);
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprTryCatch * expr) {
        const auto &at = expr->at;
#if DAS_DEBUGGER
        if ( context.debugger ) {
            setE(expr, context.code->makeNode<SimNodeDebug_TryCatch>(at, getE(expr->try_block), getE(expr->catch_block)));
        } else
#endif
        {
            setE(expr, context.code->makeNode<SimNode_TryCatch>(at, getE(expr->try_block), getE(expr->catch_block)));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprReturn * expr) {
        const auto &at = expr->at;
        // return string is its own thing
        if (expr->subexpr && expr->subexpr->type && expr->subexpr->rtti_isConstant()) {
            if (expr->subexpr->type->isSimpleType(Type::tString)) {
                auto cVal = static_cast<ExprConstString*>(expr->subexpr);
                char * str = context.constStringHeap->impl_allocateString(cVal->text);
                setE(expr, context.code->makeNode<SimNode_ReturnConstString>(at, str));
                return expr;
            }
        }
        // now, lets do the standard everything
        bool skipIt = false;
        if ( expr->subexpr && expr->subexpr->rtti_isMakeLocal() ) {
            if ( static_cast<ExprMakeLocal*>(expr->subexpr)->useCMRES ) {
                skipIt = true;
            }
        }
        SimNode * simSubE = (expr->subexpr && !skipIt) ? getE(expr->subexpr) : nullptr;
        if (!expr->subexpr) {
            setE(expr, context.code->makeNode<SimNode_ReturnNothing>(at));
            return expr;
        } else if ( expr->subexpr->rtti_isConstant() ) {
            auto cVal = static_cast<ExprConst*>(expr->subexpr);
            setE(expr, context.code->makeNode<SimNode_ReturnConst>(at, cVal->value));
            return expr;
        }
        if ( expr->returnReference ) {
            if ( expr->returnInBlock ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnReferenceFromBlock>(at, simSubE));
            } else {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnReference>(at, simSubE));
            }
        } else if ( expr->returnInBlock ) {
            if ( expr->returnCallCMRES ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                getCallBase(simSubE)->cmresEval = context.code->makeNode<SimNode_GetBlockCMResOfs>(at, 0, expr->stackTop);
                setE(expr, context.code->makeNode<SimNode_Return>(at, simSubE));
            } else if ( expr->takeOverRightStack ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnRefAndEvalFromBlock>(at,
                            simSubE, expr->refStackTop, expr->stackTop));
            } else if ( expr->block->copyOnReturn  ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnAndCopyFromBlock>(at,
                            simSubE, expr->subexpr->type->getSizeOf(), expr->stackTop));
            } else if ( expr->block->moveOnReturn ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnAndMoveFromBlock>(at,
                    simSubE, expr->subexpr->type->getSizeOf(), expr->stackTop));
            }
        } else if ( expr->subexpr ) {
            if ( expr->returnCallCMRES ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                getCallBase(simSubE)->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(at, 0);
                setE(expr, context.code->makeNode<SimNode_Return>(at, simSubE));
            } else if ( expr->returnCMRES ) {
                // ReturnLocalCMRes
                if ( expr->subexpr->rtti_isMakeLocal() ) {
                    auto mkl = static_cast<ExprMakeLocal*>(expr->subexpr);
                    if ( mkl->useCMRES ) {
                        SimNode_Block * blockT = context.code->makeNode<SimNode_ReturnLocalCMRes>(at);
                        auto simlist = sv_simulateLocal(mkl);
                        blockT->total = int(simlist.size());
                        blockT->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*blockT->total);
                        for ( uint32_t i=0, is=blockT->total; i!=is; ++i )
                            blockT->list[i] = simlist[i];
                        setE(expr, blockT);
                        return expr;
                    }
                }
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_Return>(at, simSubE));
            } else if ( expr->takeOverRightStack ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnRefAndEval>(at, simSubE, expr->refStackTop));
            } else if ( expr->returnFunc && expr->returnFunc->copyOnReturn ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnAndCopy>(at, simSubE, expr->subexpr->type->getSizeOf()));
            } else if ( expr->returnFunc && expr->returnFunc->moveOnReturn ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                setE(expr, context.code->makeNode<SimNode_ReturnAndMove>(at, simSubE, expr->subexpr->type->getSizeOf()));
            }
        }
        if ( e2v.find(expr) == e2v.end() ) {
            if ( !simSubE ) {
                // this is when subexpr produces no expression worth simulating (meaning its internal reported error or internal error)
                setE(expr, nullptr);
                return expr;
            }
            if ( expr->moveSemantics ) {
                if ( expr->subexpr->type->isRef() ) {
                    setE(expr, context.code->makeValueNode<SimNode_ReturnAndMoveR2V>(expr->subexpr->type->getR2VType(), at, simSubE));
                } else {
                    setE(expr, context.code->makeNode<SimNode_Return>(at, simSubE));
                }
            } else {
                setE(expr, context.code->makeNode<SimNode_Return>(at, simSubE));
            }
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprBreak * expr) {
        const auto &at = expr->at;
        setE(expr, context.code->makeNode<SimNode_Break>(at));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprContinue * expr) {
        const auto &at = expr->at;
        setE(expr, context.code->makeNode<SimNode_Continue>(at));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprIfThenElse * expr) {
        const auto &at = expr->at;
        ExpressionPtr zeroCond = nullptr;
        bool condIfZero = false;
        bool match0 = matchEquNequZero(expr->cond, zeroCond, condIfZero);
#if DAS_DEBUGGER
        if ( context.debugger ) {
            if ( match0 && zeroCond->type->isWorkhorseType() ) {
                if ( condIfZero ) {
                    if ( expr->if_false ) {
                        setE(expr, context.code->makeNumericValueNode<SimNodeDebug_IfZeroThenElse>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true), getE(expr->if_false)));
                    } else {
                        setE(expr, context.code->makeNumericValueNode<SimNodeDebug_IfZeroThen>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true)));
                    }
                } else {
                    if ( expr->if_false ) {
                        setE(expr, context.code->makeNumericValueNode<SimNodeDebug_IfNotZeroThenElse>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true), getE(expr->if_false)));
                    } else {
                        setE(expr, context.code->makeNumericValueNode<SimNodeDebug_IfNotZeroThen>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true)));
                    }
                }
            } else {
                // good old if
                if ( expr->if_false ) {
                    setE(expr, context.code->makeNode<SimNodeDebug_IfThenElse>(at, getE(expr->cond),
                                        getE(expr->if_true), getE(expr->if_false)));
                } else {
                    setE(expr, context.code->makeNode<SimNodeDebug_IfThen>(at, getE(expr->cond),
                                        getE(expr->if_true)));
                }
            }
        } else
#endif
        {
            if ( match0 && zeroCond->type->isWorkhorseType() ) {
                if ( condIfZero ) {
                    if ( expr->if_false ) {
                        setE(expr, context.code->makeNumericValueNode<SimNode_IfZeroThenElse>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true), getE(expr->if_false)));
                    } else {
                        setE(expr, context.code->makeNumericValueNode<SimNode_IfZeroThen>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true)));
                    }
                } else {
                    if ( expr->if_false ) {
                        setE(expr, context.code->makeNumericValueNode<SimNode_IfNotZeroThenElse>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true), getE(expr->if_false)));
                    } else {
                        setE(expr, context.code->makeNumericValueNode<SimNode_IfNotZeroThen>(zeroCond->type->baseType,
                                        at, getE(zeroCond), getE(expr->if_true)));
                    }
                }
            } else {
                // good old if
                if ( expr->if_false ) {
                    setE(expr, context.code->makeNode<SimNode_IfThenElse>(at, getE(expr->cond),
                                        getE(expr->if_true), getE(expr->if_false)));
                } else {
                    setE(expr, context.code->makeNode<SimNode_IfThen>(at, getE(expr->cond),
                                        getE(expr->if_true)));
                }
            }
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprWith * expr) {
        setE(expr, getE(expr->body));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprAssume * expr) {
        setE(expr, nullptr);
        return expr;
    }

    void SimulateVisitor::sv_whileSimulateFinal ( ExpressionPtr bod, SimNode_Block * blk ) {
        if ( bod->rtti_isBlock() ) {
            auto pBlock = static_cast<ExprBlock*>(bod);
            sv_simulateBlock(pBlock, blk);
            sv_simulateFinal(pBlock, blk);
        } else {
            context.thisProgram->error("internal error, expecting block", "", "", bod->at);
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprWhile * expr) {
        const auto &at = expr->at;
        SimNode_Block * whileNode = nullptr;
#if DAS_DEBUGGER
        if ( context.debugger ) {
            whileNode = context.code->makeNode<SimNodeDebug_While>(at, getE(expr->cond));
        } else
#endif
#if DAS_ENABLE_KEEPALIVE
        if ( context.thisProgram->policies.keep_alive ) {
            whileNode = context.code->makeNode<SimNodeKeepAlive_While>(at, getE(expr->cond));
        } else
#endif
        {
            whileNode = context.code->makeNode<SimNode_While>(at, getE(expr->cond));
        }
        sv_whileSimulateFinal(expr->body, whileNode);
        setE(expr, whileNode);
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprUnsafe * expr) {
        setE(expr, getE(expr->body));
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprFor * expr) {
        const auto &at = expr->at;
        // determine iteration types
        bool nativeIterators = false;
        bool fixedArrays = false;
        bool dynamicArrays = false;
        bool stringChars = false;
        bool rangeBase = false;
        int32_t fixedSize = INT32_MAX;
        for ( auto & src : expr->sources ) {
            if ( !src->type ) continue;
            if ( src->type->isArray() ) {
                fixedSize = das::min(fixedSize, src->type->dim[0]);
                fixedArrays = true;
            } else if ( src->type->isGoodArrayType() ) {
                dynamicArrays = true;
            } else if ( src->type->isGoodIteratorType() ) {
                nativeIterators = true;
            } else if ( src->type->isHandle() ) {
                nativeIterators = true;
            } else if ( src->type->isRange() ) {
                rangeBase = true;
            } else if ( src->type->isString() ) {
                stringChars = true;
            }
        }
        // create loops based on
        int  total = int(expr->sources.size());
        int  sourceTypes = int(dynamicArrays) + int(fixedArrays) + int(rangeBase) + int(stringChars);
        bool hybridRange = rangeBase && (total>1);
        if ( (sourceTypes>1) || hybridRange || nativeIterators || stringChars || total>MAX_FOR_UNROLL ) {
            SimNode_ForWithIteratorBase * result;
#if DAS_DEBUGGER
            if ( context.debugger ) {
                if ( total>MAX_FOR_UNROLL ) {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNode<SimNodeDebug_ForWithIteratorBase>(at);
                } else {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeDebug_ForWithIterator>(total, at);
                }
            } else
#endif
#if DAS_ENABLE_KEEPALIVE
            if ( context.thisProgram->policies.keep_alive ) {
                if ( total>MAX_FOR_UNROLL ) {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNode<SimNodeKeepAlive_ForWithIteratorBase>(at);
                } else {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeKeepAlive_ForWithIterator>(total, at);
                }
            } else
#endif
            {
                if ( total>MAX_FOR_UNROLL ) {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNode<SimNode_ForWithIteratorBase>(at);
                } else {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNodeUnrollNZ_FOR<SimNode_ForWithIterator>(total, at);
                }
            }
            result->allocateFor(context.code.get(), total);
            for ( int t=0; t!=total; ++t ) {
                if ( expr->sources[t]->type->isGoodIteratorType() ) {
                    auto errorMessage = context.code->allocateName(", "+expr->sources[t]->describe());
                    result->source_iterators[t] = context.code->makeNode<SimNode_Seq2Iter>(
                        expr->sources[t]->at,
                        getE(expr->sources[t]),
                        errorMessage);
                } else if ( expr->sources[t]->type->isGoodArrayType() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_GoodArrayIterator>(
                        expr->sources[t]->at,
                        getE(expr->sources[t]),
                        expr->sources[t]->type->firstType->getSizeOf());
                } else if ( expr->sources[t]->type->isRange() ) {
                    result->source_iterators[t] = context.code->makeRangeNode<SimNode_RangeIterator>(
                        expr->sources[t]->type->baseType, expr->sources[t]->at, getE(expr->sources[t]));
                } else if ( expr->sources[t]->type->isString() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_StringIterator>(
                        expr->sources[t]->at,
                        getE(expr->sources[t]));
                } else if ( expr->sources[t]->type->isHandle() ) {
                    if ( !result ) {
                        context.thisProgram->error("integration error, simulateGetIterator returned null", "", "",
                                                   at, CompilationError::missing_node );
                        setE(expr, nullptr);
                        return expr;
                    } else {
                        result->source_iterators[t] = expr->sources[t]->type->annotation->simulateGetIterator(
                            context,
                            expr->sources[t]->at,
                            expr->sources[t]
                        );
                    }
                } else if ( expr->sources[t]->type->dim.size() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_FixedArrayIterator>(
                        expr->sources[t]->at,
                        getE(expr->sources[t]),
                        expr->sources[t]->type->dim[0],
                        expr->sources[t]->type->getStride());
                } else {
                    DAS_ASSERTF(0, "we should not be here. we are doing iterator for on an unsupported type.");
                    context.thisProgram->error("internal compilation error, generating for-with-iterator", "", "", at);
                    setE(expr, nullptr);
                    return expr;
                }
                result->stackTop[t] = expr->iteratorVariables[t]->stackTop;
            }
            sv_whileSimulateFinal(expr->body, result);
            setE(expr, result);
        } else {
            auto flagsE = expr->body->getEvalFlags();
            bool NF = flagsE == 0;
            SimNode_ForBase * result;
            DAS_ASSERT(expr->body->rtti_isBlock() && "there would be internal error otherwise");
            auto subB = static_cast<ExprBlock*>(expr->body);
            bool loop1 = (subB->list.size() == 1);
#if DAS_DEBUGGER
            if ( context.debugger ) {
                if ( dynamicArrays ) {
                    if (loop1) {
                        result = (SimNode_ForBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeDebug_ForGoodArray1>(total, at);
                    } else {
                        result = (SimNode_ForBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeDebug_ForGoodArray>(total, at);
                    }
                } else if ( fixedArrays ) {
                    if (loop1) {
                        result = (SimNode_ForBase *)context.code->makeNodeUnrollNZ_FOR<SimNodeDebug_ForFixedArray1>(total, at);
                    } else {
                        result = (SimNode_ForBase *)context.code->makeNodeUnrollNZ_FOR<SimNodeDebug_ForFixedArray>(total, at);
                    }
                } else if ( rangeBase ) {
                    DAS_ASSERT(total==1 && "simple range on 1 loop only");
                    if ( NF ) {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRangeNF1>(expr->sources[0]->type->baseType, at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRangeNF>(expr->sources[0]->type->baseType, at);
                        }
                    } else {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRange1>(expr->sources[0]->type->baseType, at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRange>(expr->sources[0]->type->baseType, at);
                        }
                    }
                } else {
                    DAS_ASSERTF(0, "we should not be here yet. logic above assumes optimized for path of some kind.");
                    context.thisProgram->error("internal compilation error, generating for", "", "", at);
                    setE(expr, nullptr);
                    return expr;
                }
            } else
#endif
#if DAS_ENABLE_KEEPALIVE
            if ( context.thisProgram->policies.keep_alive ) {
                if ( dynamicArrays ) {
                    if (loop1) {
                        result = (SimNode_ForBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeKeepAlive_ForGoodArray1>(total, at);
                    } else {
                        result = (SimNode_ForBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeKeepAlive_ForGoodArray>(total, at);
                    }
                } else if ( fixedArrays ) {
                    if (loop1) {
                        result = (SimNode_ForBase *)context.code->makeNodeUnrollNZ_FOR<SimNodeKeepAlive_ForFixedArray1>(total, at);
                    } else {
                        result = (SimNode_ForBase *)context.code->makeNodeUnrollNZ_FOR<SimNodeKeepAlive_ForFixedArray>(total, at);
                    }
                } else if ( rangeBase ) {
                    DAS_ASSERT(total==1 && "simple range on 1 loop only");
                    if ( NF ) {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeKeepAlive_ForRangeNF1>(expr->sources[0]->type->baseType, at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeKeepAlive_ForRangeNF>(expr->sources[0]->type->baseType, at);
                        }
                    } else {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeKeepAlive_ForRange1>(expr->sources[0]->type->baseType, at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeKeepAlive_ForRange>(expr->sources[0]->type->baseType, at);
                        }
                    }
                } else {
                    DAS_ASSERTF(0, "we should not be here yet. logic above assumes optimized for path of some kind.");
                    context.thisProgram->error("internal compilation error, generating for", "", "", at);
                    setE(expr, nullptr);
                    return expr;
                }
            } else
#endif
            {
                if ( dynamicArrays ) {
                    if (loop1) {
                        result = (SimNode_ForBase *) context.code->makeNodeUnrollNZ_FOR<SimNode_ForGoodArray1>(total, at);
                    } else {
                        result = (SimNode_ForBase *) context.code->makeNodeUnrollNZ_FOR<SimNode_ForGoodArray>(total, at);
                    }
                } else if ( fixedArrays ) {
                    if (loop1) {
                        result = (SimNode_ForBase *)context.code->makeNodeUnrollNZ_FOR<SimNode_ForFixedArray1>(total, at);
                    } else {
                        result = (SimNode_ForBase *)context.code->makeNodeUnrollNZ_FOR<SimNode_ForFixedArray>(total, at);
                    }
                } else if ( rangeBase ) {
                    DAS_ASSERT(total==1 && "simple range on 1 loop only");
                    if ( NF ) {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRangeNF1>(expr->sources[0]->type->baseType, at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRangeNF>(expr->sources[0]->type->baseType, at);
                        }
                    } else {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRange1>(expr->sources[0]->type->baseType, at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRange>(expr->sources[0]->type->baseType, at);
                        }
                    }
                } else {
                    DAS_ASSERTF(0, "we should not be here yet. logic above assumes optimized for path of some kind.");
                    context.thisProgram->error("internal compilation error, generating for", "", "", at);
                    setE(expr, nullptr);
                    return expr;
                }
            }
            result->allocateFor(context.code.get(), total);
            for ( int t=0; t!=total; ++t ) {
                result->sources[t] = getE(expr->sources[t]);
                if ( expr->sources[t]->type->isGoodArrayType() ) {
                    result->strides[t] = expr->sources[t]->type->firstType->getSizeOf();
                } else {
                    result->strides[t] = expr->sources[t]->type->getStride();
                }
                result->stackTop[t] = expr->iteratorVariables[t]->stackTop;
            }
            result->size = fixedSize;
            sv_whileSimulateFinal(expr->body, result);
            setE(expr, result);
        }
        return expr;
    }

    vector<SimNode *> SimulateVisitor::sv_simulateLetInits(const ExprLet * pLet) {
        vector<SimNode *> simlist;
        simlist.reserve(pLet->variables.size());
        for (auto & var : pLet->variables) {
            SimNode * init;
            if (var->init) {
                init = sv_simulateLetInit(var, true);
            } else if (var->aliasCMRES ) {
                int bytes = var->type->getSizeOf();
                if ( bytes==0 ) {
                    init = nullptr;
                } else if ( bytes <= 32 ) {
                    init = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, pLet->at,0);
                } else {
                    init = context.code->makeNode<SimNode_InitLocalCMRes>(pLet->at,0,bytes);
                }
            } else {
                init = context.code->makeNode<SimNode_InitLocal>(pLet->at, var->stackTop, var->type->getSizeOf());
            }
            if (init) simlist.push_back(init);
        }
        return simlist;
    }

    SimNode * SimulateVisitor::sv_simulateLetInit(const VariablePtr & var, bool local) {
        gc_guard _guard;
        SimNode * get;
        if ( local ) {
            if ( var->init && var->init->rtti_isMakeLocal() ) {
                return getE(var->init);
            } else {
                get = context.code->makeNode<SimNode_GetLocal>(var->init->at, var->stackTop);
            }
        } else {
            if ( var->init && var->init->rtti_isMakeLocal() ) {
                return getE(var->init);
            } else {
                if ( !var->module->isSolidContext ) {
                    if ( var->global_shared ) {
                        get = context.code->makeNode<SimNode_GetSharedMnh>(var->init->at, var->index, var->getMangledNameHash());
                    } else {
                        get = context.code->makeNode<SimNode_GetGlobalMnh>(var->init->at, var->index, var->getMangledNameHash());
                    }
                } else {
                    if ( var->global_shared ) {
                        get = context.code->makeNode<SimNode_GetShared>(var->init->at, var->index, var->getMangledNameHash());
                    } else {
                        get = context.code->makeNode<SimNode_GetGlobal>(var->init->at, var->index, var->getMangledNameHash());
                    }
                }
            }
        }
        if ( var->type->ref ) {
            return context.code->makeNode<SimNode_CopyReference>(var->init->at, get,
                                                                 getE(var->init));
        } else if ( var->init_via_move && (var->type->canMove() || var->type->isGoodBlockType()) ) {
            auto varExpr = new ExprVar(var->at, var->name);
            varExpr->variable = var;
            varExpr->local = local;
            varExpr->type = new TypeDecl(*var->type);
            simulateExpression(varExpr);
            auto retN = sv_makeMove(var->init->at, varExpr, var->init);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate move", "", "", var->at);
            }
            return retN;
        } else if ( !var->init_via_move && (var->type->canCopy() || var->type->isGoodBlockType()) ) {
            auto varExpr = new ExprVar(var->at, var->name);
            varExpr->variable = var;
            varExpr->local = local;
            varExpr->type = new TypeDecl(*var->type);
            simulateExpression(varExpr);
            auto retN = sv_makeCopy(var->init->at, varExpr, var->init);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate copy", "", "", var->at);
            }
            return retN;
        } else if ( var->isCtorInitialized() ) {
            auto varExpr = new ExprVar(var->at, var->name);
            varExpr->variable = var;
            varExpr->local = local;
            varExpr->type = new TypeDecl(*var->type);
            simulateExpression(varExpr);
            SimNode * retN = nullptr; // it has to be CALL with CMRES
            const auto & rE = var->init;
            if ( rE->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(rE);
                if ( cll->allowCmresSkip() ) {
                    retN = getE(rE);
                    getCallBase(retN)->cmresEval = getE(varExpr);
                }
            }
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate class constructor", "", "", var->at);
            }
            return retN;
        } else {
            context.thisProgram->error("internal compilation error, initializing variable which can't be copied or moved", "", "", var->at);
            return nullptr;
        }
    }

    ExpressionPtr SimulateVisitor::visit(ExprLet * expr) {
        const auto &at = expr->at;
        auto let = context.code->makeNode<SimNode_Let>(at);
        let->total = (uint32_t) expr->variables.size();
        let->list = (SimNode **) context.code->allocate(let->total * sizeof(SimNode*));
        auto simList = sv_simulateLetInits(expr);
        letInitVec[expr] = simList;
        copy(simList.data(), simList.data() + simList.size(), let->list);
        setE(expr, let);
        return expr;
    }

    SimNode * SimulateVisitor::sv_keepAlive ( const LineInfo & at, SimNode * result ) {
#if DAS_ENABLE_KEEPALIVE
        if ( context.thisProgram->policies.keep_alive ) {
            return context.code->makeNode<SimNode_KeepAlive>(at,result);
        } else
#endif
        {
            return result;
        }
    }

    SimNode_CallBase * SimulateVisitor::sv_simulateCall(const FunctionPtr & func, const ExprLooksLikeCall * expr, SimNode_CallBase * pCall) {
        bool needTypeInfo = false;
        for ( auto & arg : func->arguments ) {
            if ( arg->type->baseType==Type::anyArgument )
                needTypeInfo = true;
        }
        pCall->debugInfo = expr->at;
        if ( func->builtIn) {
            pCall->fnPtr = nullptr;
        } else if ( func->index>=0 ) {
            pCall->fnPtr = context.getFunction(func->index);
            DAS_ASSERTF(pCall->fnPtr, "calling function which null. how?");
        } else {
            DAS_ASSERTF(0, "calling function which is not used. how?");
        }
        if ( int nArg = (int) expr->arguments.size() ) {
            pCall->arguments = (SimNode **) context.code->allocate(nArg * sizeof(SimNode *));
            if ( needTypeInfo ) {
                pCall->types = (TypeInfo **) context.code->allocate(nArg * sizeof(TypeInfo *));
            } else {
                pCall->types = nullptr;
            }
            pCall->nArguments = nArg;
            for ( int a=0; a!=nArg; ++a ) {
                pCall->arguments[a] = getE(expr->arguments[a]);
                if ( pCall->types ) {
                    if ( func->arguments[a]->type->baseType==Type::anyArgument ) {
                        pCall->types[a] = context.thisHelper->makeTypeInfo(nullptr, expr->arguments[a]->type);
                    } else {
                        pCall->types[a] = nullptr;
                    }
                }
            }
        } else {
            pCall->arguments = nullptr;
            pCall->nArguments = 0;
        }
        return pCall;
    }

    SimNode * SimulateVisitor::sv_trySimulate(const Expression * expr, uint32_t extraOffset, const TypeDeclPtr & r2vType) {
        if ( expr->rtti_isCast() ) {
            return sv_trySimulate_Cast(static_cast<const ExprCast*>(expr), extraOffset, r2vType);
        }
        if ( expr->rtti_isVar() ) {
            return sv_trySimulate_Var(static_cast<const ExprVar*>(expr), extraOffset, r2vType);
        }
        if ( expr->rtti_isAt() ) {
            return sv_trySimulate_At(static_cast<const ExprAt*>(expr), extraOffset, r2vType);
        }
        if ( expr->rtti_isSwizzle() ) {
            return sv_trySimulate_Swizzle(static_cast<const ExprSwizzle*>(expr), extraOffset, r2vType);
        }
        if ( expr->rtti_isField() ) {
            return sv_trySimulate_Field(static_cast<const ExprField*>(expr), extraOffset, r2vType);
        }
        return nullptr;
    }

    vector<SimNode*> SimulateVisitor::sv_simulateLocal(const ExprMakeLocal * expr) {
        if ( expr->rtti_isMakeVariant() ) {
            makeLocalInits[expr] = simulateExprMakeVariant(static_cast<const ExprMakeVariant*>(expr));
            return makeLocalInits[expr];
        }
        if ( expr->rtti_isMakeStruct() ) {
            makeLocalInits[expr] = simulateExprMakeStruct(static_cast<const ExprMakeStruct*>(expr));
            return makeLocalInits[expr];
        }
        if ( expr->rtti_isMakeArray() && !expr->rtti_isMakeTuple() ) {
            makeLocalInits[expr] = simulateExprMakeArray(static_cast<const ExprMakeArray*>(expr));
            return makeLocalInits[expr];
        }
        if ( expr->rtti_isMakeTuple() ) {
            makeLocalInits[expr] = simulateExprMakeTuple(static_cast<const ExprMakeTuple*>(expr));
            return makeLocalInits[expr];
        }
        makeLocalInits[expr] = vector<SimNode*>();
        return vector<SimNode*>();
    }

    ExpressionPtr SimulateVisitor::visit(ExprCall * expr) {
        const auto &at = expr->at;
        auto pCall = static_cast<SimNode_CallBase *>(expr->func->makeSimNode(context, expr->arguments));
        sv_simulateCall(expr->func, expr, pCall);
        if ( !expr->doesNotNeedSp && expr->stackTop ) {
            pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at, expr->stackTop);
        }
        if ( expr->func->builtIn || !expr->func->recursive ) {
            // TODO: we need to determine, if we need keep-alive for the func
            // basic function which is not recursive in any shape or form may not need one
            setE(expr, pCall);   // we don't need keep-alive for the built-in functions
        } else {
            setE(expr, sv_keepAlive(at, pCall));
        }
        return expr;
    }

    ExpressionPtr SimulateVisitor::visit(ExprNamedCall * expr) {
        DAS_ASSERTF(false, "we should not be here. named call should be promoted to regular call");
        setE(expr, nullptr);
        return expr;
    }

    void Program::buildGMNLookup ( Context & context, TextWriter & logs ) {
        context.tabGMnLookup = make_shared<das_hash_map<uint64_t,uint32_t>>();
        context.tabGMnLookup->clear();
        for ( int i=0, is=context.totalVariables; i!=is; ++i ) {
            auto & gvar = context.globalVariables[i];
            auto mnh = gvar.mangledNameHash;
            auto it = context.tabGMnLookup->find(mnh);
            if ( it != context.tabGMnLookup->end() ) {
                GlobalVariable * collision = context.globalVariables + it->second;
                LineInfo * errorAt = nullptr;
                TextWriter message;
                message << "internal compiler error: global variable mangled name hash collision '"
                    << gvar.name << ": " << debug_type(gvar.debugInfo) << "'"
                    << " hash=" << HEX << gvar.mangledNameHash << DEC
                    << " offset=" << gvar.offset;
                if ( collision ) {
                    message << " and '" << collision->name << ": " << debug_type(collision->debugInfo) << "'"
                        << " hash=" << HEX << collision->mangledNameHash << DEC
                        << " offset=" << collision->offset;
                }
                if ( gvar.init ) {
                    errorAt = &gvar.init->debugInfo;
                } else if ( collision && collision->init ) {
                    errorAt = &collision->init->debugInfo;
                }
                error(message.str(), "", "", errorAt ? *errorAt : LineInfo());
                return;
            }
            context.tabGMnLookup->insert({mnh, context.globalVariables[i].offset});
        }
        if ( options.getBoolOption("log_gmn_hash",false) ) {
            logs
                << "totalGlobals: " << context.totalVariables << "\n"
                << "tabGMnLookup:" << context.tabGMnLookup->size() << "\n";
        }
    }

    void Program::buildMNLookup ( Context & context, const vector<FunctionPtr> & lookupFunctions, TextWriter & logs ) {
        context.tabMnLookup = make_shared<das_hash_map<uint64_t,SimFunction *>>();
        context.tabMnLookup->clear();
        for ( const auto & fn : lookupFunctions ) {
            auto mnh = fn->getMangledNameHash();
            auto it = context.tabMnLookup->find(mnh);
            if ( it != context.tabMnLookup->end() ) {
                error("internal compiler error: function mangled name hash collision '" + fn->name + "'",
                    "", "", LineInfo());
                return;
            }
            context.tabMnLookup->insert({mnh, context.functions + fn->index});
        }
        if ( options.getBoolOption("log_mn_hash",false) ) {
            logs
                << "totalFunctions: " << context.totalFunctions << "\n"
                << "tabMnLookup:" << context.tabMnLookup->size() << "\n";
        }
    }

    void Program::buildADLookup ( Context & context, TextWriter & logs ) {
        context.tabAdLookup = make_shared<das_hash_map<uint64_t,uint64_t>>();
        for (auto & pm : library.modules ) {
            for(auto s2d : pm->annotationData ) {
                auto it = context.tabAdLookup->find(s2d.first);
                if ( it != context.tabAdLookup->end() ) {
                    error("internal compiler error: annotation data hash collision " + to_string(s2d.second),
                        "", "", LineInfo());
                    return;
                }
                context.tabAdLookup->insert(s2d);
            }
        }
        if ( options.getBoolOption("log_ad_hash",false) ) {
            logs<< "tabAdLookup:" << context.tabAdLookup->size() << "\n";
        }
    }

    void Program::makeMacroModule ( TextWriter & logs ) {
        isCompilingMacros = true;
        thisModule->macroContext = get_context(getContextStackSize());
        thisModule->macroContext->category = uint32_t(das::ContextCategory::macro_context);
        auto oldAot = policies.aot;
        auto oldHeap = policies.persistent_heap;
        policies.aot = false;
        policies.persistent_heap = policies.macro_context_persistent_heap;
        simulate(*thisModule->macroContext, logs);
        policies.aot = oldAot;
        policies.persistent_heap = oldHeap;
        isCompilingMacros = false;
    }

    extern "C" int64_t ref_time_ticks ();
    extern "C" int get_time_usec (int64_t reft);

    bool isRecursive ( Function * fun, das_hash_set<Function *> & visited ) {
        if ( visited.find(fun) != visited.end() ) {
            return true;
        }
        visited.insert(fun);
        for ( auto call : fun->useFunctions ) {
            if ( isRecursive(call, visited) ) {
                return true;
            }
        }
        return false;
    }

    void Program::updateKeepAliveFlags() {
        if ( !policies.keep_alive ) return;
        for ( auto mod : library.modules ) {
            mod->functions.foreach([&](FunctionPtr & fn){
                if ( fn->builtIn ) return;
                if ( !fn->used ) return;
                if ( fn->isTemplate ) return;
                das_hash_set<Function *> visited;
                fn->recursive = isRecursive(fn, visited);
            });
        }
    }

    void Program::updateSemanticHash() {
        thisModule->structures.foreach([&](StructurePtr & ps){
            HashBuilder hb;
            das_set<Structure *> dep;
            das_set<Annotation *> adep;
            ps->ownSemanticHash = ps->getOwnSemanticHash(hb,dep,adep);
        });
    }

    bool Program::simulate ( Context & context, TextWriter & logs, StackAllocator * sharedStack ) {
        CompilationCallbackGuard callbackGuard("", thisModule->fileName, "simulation");
        auto time0 = ref_time_ticks();
    #if !(DAS_ENABLE_KEEPALIVE)
        if ( policies.keep_alive ) {
            error("keep_alive is not enabled in this build. Modify DAS_ENABLE_KEEPALIVE first", "", "", LineInfo());
            return false;
        }
    #endif
        if ( policies.keep_alive ) {
            updateKeepAliveFlags();
        }
        isSimulating = true;
        context.failed = true;
        context.verySafeContext = options.getBoolOption("very_safe_context",policies.very_safe_context);
        astTypeInfo.clear();    // this is to be filled via typeinfo(ast_typedecl and such)
        auto disableInit = options.getBoolOption("no_init", policies.no_init);
        context.thisProgram = this;
        context.breakOnException |= policies.debugger;
        context.persistent = options.getBoolOption("persistent_heap", policies.persistent_heap);
        context.gcEnabled = options.getBoolOption("gc", false);
        context.debugger = getDebugger();
        if ( context.persistent ) {
            context.heap = make_unique<PersistentHeapAllocator>();
            context.stringHeap = make_unique<PersistentStringAllocator>();
        } else {
            context.heap = make_unique<LinearHeapAllocator>();
            context.stringHeap = make_unique<LinearStringAllocator>();
        }
        context.heap->setInitialSize ( options.getIntOption("heap_size_hint", policies.heap_size_hint) );
        context.heap->setLimit ( options.getUInt64OptionEx("heap_size_limit", "max_heap_allocated", policies.max_heap_allocated) );
        context.stringHeap->setInitialSize ( options.getIntOption("string_heap_size_hint", policies.string_heap_size_hint) );
        context.stringHeap->setLimit ( options.getUInt64OptionEx("string_heap_size_limit", "max_string_heap_allocated", policies.max_string_heap_allocated) );
        bool trackAlloc = options.getBoolOption("track_allocations", policies.track_allocations);
        context.heap->setTrackAllocations(trackAlloc);
        context.stringHeap->setTrackAllocations(trackAlloc);
        context.constStringHeap = make_shared<ConstStringAllocator>();
        if ( globalStringHeapSize ) {
            context.constStringHeap->setInitialSize(globalStringHeapSize);
        }
        DebugInfoHelper helper(context.debugInfo);
        helper.rtti = options.getBoolOption("rtti",policies.rtti);
        context.thisHelper = &helper;
        context.globalVariables = (GlobalVariable *) context.code->allocate( totalVariables*sizeof(GlobalVariable) );
        context.globalsSize = 0;
        context.sharedSize = 0;
        if ( totalVariables ) {
            for (auto & pm : library.modules ) {
                pm->globals.foreach([&](auto pvar){
                    if (!pvar->used)
                        return;
                    if ( pvar->index<0 ) {
                        error("Internal compiler errors. Simulating variable which is not used" + pvar->name,
                            "", "", LineInfo());
                        return;
                    }
                    auto & gvar = context.globalVariables[pvar->index];
                    gvar.name = context.code->allocateName(pvar->name);
                    gvar.size = pvar->type->getSizeOf();
                    gvar.debugInfo = helper.makeVariableDebugInfo(*pvar);
                    gvar.flags = 0;
                    if ( pvar->global_shared ) {
                        gvar.offset = pvar->stackTop = (uint32_t) context.sharedSize;
                        gvar.shared = true;
                        context.sharedSize = (context.sharedSize + uint64_t(gvar.size) + 0xful) & ~0xfull;
                    } else {
                        gvar.offset = pvar->stackTop = (uint32_t) context.globalsSize;
                        context.globalsSize = (context.globalsSize + uint64_t(gvar.size) + 0xful) & ~0xfull;
                    }
                    gvar.mangledNameHash = pvar->getMangledNameHash();
                    gvar.init = nullptr;
                });
            }
        }
        bool canAllocateVariables = true;
        if ( context.globalsSize >= policies.max_static_variables_size || context.globalsSize >= 0x100000000ul ) {
            error("Global variables size exceeds " + to_string(policies.max_static_variables_size), "Global variables size is " + to_string(context.globalsSize) + " bytes", "", LineInfo());
            canAllocateVariables = false;
        }
        if ( context.sharedSize >= policies.max_static_variables_size || context.sharedSize >= 0x100000000ul ) {
            error("Shared variables size exceeds " + to_string(policies.max_static_variables_size), "Shared variables size is " + to_string(context.sharedSize) + " bytes", "", LineInfo());
            canAllocateVariables = false;
        }
        if ( canAllocateVariables ) {
            context.allocateGlobalsAndShared();
            if ( context.globalsSize && !context.globals ) {
                error("Failed to allocate memory for global variables", "Global variables size is " + to_string(context.globalsSize) + " bytes", "", LineInfo());
                canAllocateVariables = false;
            }
            if ( context.sharedSize && !context.shared ) {
                error("Failed to allocate memory for shared variables", "Shared variables size is " + to_string(context.sharedSize) + " bytes", "", LineInfo());
                canAllocateVariables = false;
            }
            context.totalVariables = totalVariables;
        }
        if ( !canAllocateVariables ) {
            context.freeGlobalsAndShared();
            context.globalsSize = 0;
            context.sharedSize = 0;
            context.totalVariables = 0;
        }
        context.functions = (SimFunction *) context.code->allocate( totalFunctions*sizeof(SimFunction) );
        context.totalFunctions = totalFunctions;
        auto debuggerOrGC = context.debugger || context.gcEnabled;
        vector<FunctionPtr> lookupFunctionTable;
        das_hash_map<uint64_t,Function *> fnByMnh;
        bool anyPInvoke = false;
        if ( totalFunctions ) {
            for (auto & pm : library.modules) {
                pm->functions.foreach([&](auto pfun){
                    if (pfun->index < 0 || !pfun->used || pfun->isTemplate)
                        return;
                    if ( (pfun->init || pfun->shutdown) && disableInit ) {
                        error("[init] is disabled in the options or CodeOfPolicies",
                            "internal compiler error: [init] function made it all the way to simulate somehow", "",
                                pfun->at, CompilationError::no_init);
                    }
                    auto mangledName = pfun->getMangledName();
                    auto MNH = hash_blockz64((uint8_t *)mangledName.c_str());
                    fnByMnh[MNH] = pfun;
                    auto & gfun = context.functions[pfun->index];
                    gfun.name = context.code->allocateName(pfun->name);
                    gfun.mangledName = context.code->allocateName(mangledName);
                    gfun.debugInfo = helper.makeFunctionDebugInfo(*pfun);
                    if ( folding ) {
                        gfun.debugInfo->flags &= ~ (FuncInfo::flag_init | FuncInfo::flag_shutdown);
                    }
                    if ( debuggerOrGC ) {
                        helper.appendLocalVariables(gfun.debugInfo, pfun->body);
                        helper.appendGlobalVariables(gfun.debugInfo, pfun);
                    }
                    gfun.stackSize = pfun->totalStackSize;
                    gfun.mangledNameHash = MNH;
                    gfun.aotFunction = nullptr;
                    gfun.flags = 0;
                    gfun.fastcall = pfun->fastCall;
                    gfun.unsafe = pfun->unsafeOperation;
                    if ( pfun->result->isRefType() && !pfun->result->ref ) {
                        gfun.cmres = true;
                    }
                    if ( pfun->module->builtIn && !pfun->module->promoted ) {
                        gfun.builtin = true;
                    }
                    gfun.code = pfun->simulate(context);
                    if ( pfun->pinvoke ) {
                        anyPInvoke = true;
                        gfun.pinvoke = true;
                    }
                    lookupFunctionTable.push_back(pfun);
                });
            }
        }
        if ( totalVariables ) {
            for (auto & pm : library.modules ) {
                pm->globals.foreach([&](auto pvar){
                    if (!pvar->used)
                        return;
                    auto & gvar = context.globalVariables[pvar->index];
                    if ( !folding && pvar->init ) {
                        if ( disableInit && !pvar->init->rtti_isConstant() ) {
                            error("[init] is disabled in the options or CodeOfPolicies",
                                "internal compiler error: [init] function made it all the way to simulate somehow", "",
                                    pvar->at, CompilationError::no_init);
                        }
                        if ( pvar->init->rtti_isMakeLocal() ) {
                            if ( pvar->global_shared ) {
                                auto sl = context.code->makeNode<SimNode_GetSharedMnh>(pvar->init->at, pvar->stackTop, pvar->getMangledNameHash());
                                auto sr = simulateLetInit(context, pvar, false);
                                auto gvari = context.code->makeNode<SimNode_SetLocalRefAndEval>(pvar->init->at, sl, sr, uint32_t(sizeof(Prologue)));
                                auto cndb = context.code->makeNode<SimNode_GetArgument>(pvar->init->at, 1); // arg 1 of init script is "init_globals"
                                gvar.init = context.code->makeNode<SimNode_IfThen>(pvar->init->at, cndb, gvari);
                            } else {
                                auto sl = context.code->makeNode<SimNode_GetGlobalMnh>(pvar->init->at, pvar->stackTop, pvar->getMangledNameHash());
                                auto sr = simulateLetInit(context, pvar, false);
                                gvar.init = context.code->makeNode<SimNode_SetLocalRefAndEval>(pvar->init->at, sl, sr, uint32_t(sizeof(Prologue)));
                            }
                        } else {
                            gvar.init = simulateLetInit(context, pvar, false);
                        }
                    } else {
                        gvar.init = nullptr;
                    }
                });
            }
        }
        // if there is anything pinvoke or the option is set
        if ( !context.contextMutex && (anyPInvoke || policies.threadlock_context || options.getBoolOption("threadlock_context",false) || policies.debugger) ) {
            context.contextMutex = new recursive_mutex;
        }
        //
        context.globalInitStackSize = globalInitStackSize;
        buildMNLookup(context, lookupFunctionTable, logs);
        buildGMNLookup(context, logs);
        buildADLookup(context, logs);
        context.simEnd();
        // if RTTI is enabled
        if (errors.size()) {
            isSimulating = false;
            return false;
        }
        context.failed = false;
        bool aot_hint = policies.aot && !folding && !thisModule->isModule;
#if DAS_FUSION
        if ( !folding ) {               // note: only run fusion when not folding
            DAS_ASSERTF(g_fusionContextFn, "fusion library not loaded, add call to NEED_FUSION macro.");
            g_fusionContextFn(context, logs, options.getBoolOption("fusion",true));
            context.relocateCode(true); // this to get better estimate on relocated size. its fust enough
        }
#else
        if ( !folding ) {
            context.relocateCode(true);
        }
#endif
        if ( !folding ) {
            if ( !aot_hint ) {
                context.relocateCode();
            }
        }
        context.restart();
        // now call annotation simulate
        das_hash_map<int,Function *> indexToFunction;
        for (auto & pm : library.modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used || pfun->isTemplate)
                    return;
                auto & gfun = context.functions[pfun->index];
                for ( const auto & an : pfun->annotations ) {
                    auto fna = static_cast<FunctionAnnotation*>(an->annotation);
                    if (!fna->simulate(&context, &gfun)) {
                        error("function " + pfun->describe() + " annotation " + fna->name + " simulation failed", "", "",
                            LineInfo(), CompilationError::cant_initialize);
                    }
                }
                indexToFunction[pfun->index] = pfun;
            });
        }
        // verify code and string heaps
#if DAS_FUSION
        if ( !folding ) {
            // note: this only matters if code has significant jumping around
            // which is always introduced by fusion
            DAS_ASSERTF(context.code->depth()<=1, "code must come in one page");
        }
#endif
        DAS_ASSERTF(context.constStringHeap->depth()<=1, "strings must come in one page");
        context.stringHeap->setIntern(options.getBoolOption("intern_strings", policies.intern_strings));
        // log all functions
        if ( options.getBoolOption("log_nodes",false) ) {
            bool displayHash = options.getBoolOption("log_nodes_aot_hash",false);
            for ( int i=0, is=context.totalVariables; i!=is; ++i ) {
                auto & pv = context.globalVariables[i];
                if ( pv.init ) {
                    logs << "// init " << pv.name << "\n";
                    printSimNode(logs, &context, pv.init, displayHash);
                    logs << "\n\n";
                }
            }
            for ( int i=0, is=context.totalFunctions; i!=is; ++i ) {
                if (SimFunction * fn = context.getFunction(i)) {
                    logs << "// " << fn->name << " " << fn->mangledName << "\n";
                    printSimFunction(logs, &context, indexToFunction[i], fn->code, displayHash);
                    logs << "\n\n";
                }
            }
        }
        // now relocate before we run that init script
        if ( aot_hint ) {
            linkCppAot(context, getGlobalAotLibrary(), logs);
            context.relocateCode(true);
            context.relocateCode();
        }
        // build init functions
        vector<SimFunction *> allInitFunctions;
        for ( int fni=0; fni!=context.totalFunctions; fni++ ) {
            auto & fn = context.functions[fni];
            if ( fn.debugInfo->flags & FuncInfo::flag_init ) {
                allInitFunctions.push_back(&fn);
            }
        }
        stable_sort(allInitFunctions.begin(), allInitFunctions.end(), [](auto a, auto b) { // sort them, so that late init is last
            int lateA = a->debugInfo->flags & FuncInfo::flag_late_init;
            int lateB = b->debugInfo->flags & FuncInfo::flag_late_init;
            return lateA < lateB;
        });
        if ( options.getBoolOption("log_init") ) {
            logs << "INIT FUNCTIONS:\n";
            for ( auto & inf : allInitFunctions ) {
                logs << "\t" << inf->mangledName << "\n";
            }
            logs << "\n";
        }
        // find first late init
        InitSort initSort;
        int firstLateInit = -1;
        for ( int i=0, is=int(allInitFunctions.size()); i!=is; ++i ) {
            auto initFn = allInitFunctions[i];
            if ( initFn->debugInfo->flags & FuncInfo::flag_late_init ) {
                if ( firstLateInit==-1 ) firstLateInit = i;
                auto pfn = fnByMnh[initFn->mangledNameHash];
                DAS_ASSERT(pfn);
                for ( auto & ann : pfn->annotations ) {
                    if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                        auto fna = static_cast<FunctionAnnotation*>(ann->annotation);
                        if ( fna->name=="init" ) {
                            initSort.addNode(initFn->mangledNameHash, ann->arguments);
                            break;
                        }
                    }
                }
            }
        }
        if ( firstLateInit!=-1 ) {
            auto sorted = initSort.sort();
            for ( int i=0, is=(int)sorted.size(); i!=is; ++i ) {
                allInitFunctions[firstLateInit+i] = context.fnByMangledName(sorted[i]);
            }
        }
        context.totalInitFunctions = (uint32_t) allInitFunctions.size();
        if ( context.totalInitFunctions!=0 && allInitFunctions.data()!=nullptr ) {
            context.initFunctions = (SimFunction **) context.code->allocate(uint32_t(allInitFunctions.size()*sizeof(SimFunction *)));
            memcpy ( context.initFunctions, allInitFunctions.data(), allInitFunctions.size()*sizeof(SimFunction *) );
        } else {
            context.initFunctions = nullptr;
        }
        // run init script and restart
        if ( !folding ) {
            auto time1 = ref_time_ticks();
            bool initScriptSuccess;
            if ( context.stack.size() && context.stack.size()>globalInitStackSize ) {
                initScriptSuccess = context.runWithCatch([&]() {
                    context.runInitScript();
                });
            } else if ( sharedStack && sharedStack->size()>globalInitStackSize ) {
                SharedStackGuard guard(context, *sharedStack);
                initScriptSuccess = context.runWithCatch([&]() {
                    context.runInitScript();
                });
            } else {
                auto ssz = max ( getContextStackSize(), 16384 ) + globalInitStackSize;
                StackAllocator init_stack(ssz);
                SharedStackGuard guard(context, init_stack);
                initScriptSuccess = context.runWithCatch([&]() {
                    context.runInitScript();
                });
            }
            if (!initScriptSuccess) {
                error("exception during init script", context.getException(), "",
                    context.exceptionAt, CompilationError::cant_initialize);
                context.clearException();
            }
            if ( options.getBoolOption("log_total_compile_time",policies.log_total_compile_time) ) {
                auto dt = get_time_usec(time1) / 1000000.;
                logs << "init script took " << dt << "\n";
            }
        }
        context.restart();
        if (options.getBoolOption("log_mem",false) ) {
            context.logMemInfo(logs);
            logs << "shared        " << context.getSharedMemorySize() << "\n";
            logs << "unique        " << context.getUniqueMemorySize() << "\n";
        }

        isSimulating = false;
        context.thisHelper = &helper;   // note - we may need helper for the 'complete'
        auto bound_env = daScriptEnvironment::getBound();
        auto boundProgram = bound_env->g_Program;
        bound_env->g_Program = this;   // node - we are calling macros
        library.foreach_in_order([&](Module * pm) -> bool {
            for ( auto & sm : pm->simulateMacros ) {
                if ( !sm->preSimulate(this, &context) ) {
                    error("simulate macro " + pm->name + "::" + sm->name + " failed to preSimulate", "", "", LineInfo());
                    bound_env->g_Program = boundProgram;
                    return false;
                }
            }
            return true;
        }, thisModule.get());
        for ( int i=0, is=context.totalFunctions; i!=is; ++i ) {
            Function *func = indexToFunction[i];
            for (auto &ann : func->annotations) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto fann = static_cast<FunctionAnnotation*>(ann->annotation);
                    fann->complete(&context, func);
                }
            }
        }
        for ( int i=0, is=context.totalFunctions; i!=is; ++i ) {
            Function * func = indexToFunction[i];
            SimFunction & fn = context.functions[i];
            func->hash = getFunctionHash(func, fn.code, &context);
        }
        for (auto pm : library.modules) {
            pm->structures.foreach([&](auto st){
                for ( auto & ann : st->annotations ) {
                    if ( ann->annotation->rtti_isStructureAnnotation() ) {
                        auto sann = static_cast<StructureAnnotation*>(ann->annotation);
                        sann->complete(&context, st);
                    }
                }
            });
        }
        library.foreach_in_order([&](Module * pm) -> bool {
            for ( auto & sm : pm->simulateMacros ) {
                if ( !sm->simulate(this, &context) ) {
                    error("simulate macro " + pm->name + "::" + sm->name + " failed to simulate", "", "", LineInfo());
                    bound_env->g_Program = boundProgram;
                    return false;
                }
            }
            return true;
        }, thisModule.get());
        context.thisHelper = nullptr;
        bound_env->g_Program = boundProgram;
        // dispatch about new inited context
        context.announceCreation();
        if ( options.getBoolOption("log_debug_mem",false) ) {
            helper.logMemInfo(logs);
        }
        if ( !options.getBoolOption("rtti",policies.rtti) ) {
            context.thisProgram = nullptr;
        }
        if ( options.getBoolOption("log_total_compile_time",policies.log_total_compile_time) ) {
            auto dt = get_time_usec(time0) / 1000000.;
            logs << "simulate (including init script) took " << dt << "\n";
        }
        dapiSimulateContext(context);
        return errors.size() == 0;
    }

    // Unused! Consider removing it.
    uint64_t Program::getInitSemanticHashWithDep( uint64_t initHash ) {
        vector<const Variable *> globs;
        globs.reserve(totalVariables);
        for (auto & pm : library.modules) {
            pm->globals.foreach([&](auto var){
                if (var->used) {
                    globs.push_back(var);
                }
            });
        }
        uint64_t res = getVariableListAotHash(globs, initHash);
        // add init functions to dependencies
        const uint64_t fnv_prime = 1099511628211ul;
        for (auto& pm : library.modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used || !pfun->init)
                    return;
                res = (res ^ pfun->aotHash) * fnv_prime;
            });
        }
        initSemanticHashWithDep = res; // pass it to the standalone context registration
        return initSemanticHashWithDep;
    }

    void Program::linkCppAot ( Context & context, AotLibrary & aotLib, TextWriter & logs ) {
        bool logIt = options.getBoolOption("log_aot",false);
        // make list of functions
        vector<Function *> fnn; fnn.reserve(totalFunctions);
        das_hash_map<int,Function *> indexToFunction;
        for (auto & pm : library.modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used || pfun->isTemplate)
                    return;
                fnn.push_back(pfun);
                indexToFunction[pfun->index] = pfun;
            });
        }
        for ( int fni=0, fnis=context.totalFunctions; fni!=fnis; ++fni ) {
            if ( !fnn[fni]->noAot ) {
                SimFunction & fn = context.functions[fni];
                fnn[fni]->hash = getFunctionHash(fnn[fni], fn.code, &context);
            }
        }
        for ( int fni=0, fnis=context.totalFunctions; fni!=fnis; ++fni ) {
            if ( !fnn[fni]->noAot ) {
                SimFunction & fn = context.functions[fni];
                uint64_t semHash = getFunctionAotHash(fnn[fni]);
                auto it = aotLib.find(semHash);
                if ( it != aotLib.end() ) {
                    fn.code = (it->second)(context);
                    fn.aot = true;
                    if ( logIt ) logs << fn.mangledName << " AOT=0x" << HEX << semHash << DEC << "\n";
                    auto fcb = (SimNode_CallBase *) fn.code;
                    fn.aotFunction = fcb->aotFunction;
                } else {
                    if ( logIt ) logs << "NOT FOUND " << fn.mangledName << " AOT=0x" << HEX << semHash << DEC << "\n";
                    TextWriter tp;
                    tp << "semantic hash is " << HEX << semHash << DEC << "\n";
                    tp << "// " << getAotHashComment(fnn[fni]) << "\n";
                    printSimFunction(tp, &context, indexToFunction[fni], fn.code, true);
                    linkError(string(fn.mangledName), tp.str() );
                }
            }
        }
    }
}
