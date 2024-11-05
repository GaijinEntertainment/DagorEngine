#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_match.h"
#include "daScript/ast/ast_expressions.h"

#include "daScript/simulate/runtime_array.h"
#include "daScript/simulate/runtime_table_nodes.h"
#include "daScript/simulate/runtime_range.h"
#include "daScript/simulate/runtime_string_delete.h"
#include "daScript/simulate/hash.h"

#include "daScript/simulate/simulate_nodes.h"

#include "daScript/simulate/simulate_visit_op.h"

das::Context * get_context ( int stackSize=0 );//link time resolved dependencies

namespace das
{
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

    // common for move and copy

    SimNode * makeLocalCMResMove (const LineInfo & at, Context & context, uint32_t offset, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return right;
            }
        }
        // now, to the regular move
        auto left = context.code->makeNode<SimNode_GetCMResOfs>(at, offset);
        auto right = rE->simulate(context);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeLocalCMResCopy(const LineInfo & at, Context & context, uint32_t offset, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( rightType.canCopy() &&
                "we are calling makeLocalCMResCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        auto right = rE->simulate(context);
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * rightC = (SimNode_CallBase *) right;
                rightC->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return rightC;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * rightC = (SimNode_CallBase *) right;
                rightC->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
                return rightC;
            }
        }
        // wo standard path
        auto left = context.code->makeNode<SimNode_GetCMResOfs>(rE->at, offset);
        if ( rightType.isHandle() ) {
            auto resN = rightType.annotation->simulateCopy(context, at, left,
                (!rightType.isRefType() && rightType.ref) ? rightType.annotation->simulateRef2Value(context, at, right) : right);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateCopy returned null", "", "",
                                           at, CompilationError::missing_node );
            }
            return resN;
        } else if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeLocalRefMove (const LineInfo & at, Context & context, uint32_t stackTop, uint32_t offset, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return right;
            }
        }
        // now, to the regular move
        auto left = context.code->makeNode<SimNode_GetLocalRefOff>(at, stackTop, offset);
        auto right = rE->simulate(context);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeLocalRefCopy(const LineInfo & at, Context & context, uint32_t stackTop, uint32_t offset, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( rightType.canCopy() &&
                "we are calling makeLocalRefCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        auto right = rE->simulate(context);
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * rightC = (SimNode_CallBase *) right;
                rightC->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return rightC;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * rightC = (SimNode_CallBase *) right;
                rightC->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
                return rightC;
            }
        }
        // wo standard path
        auto left = context.code->makeNode<SimNode_GetLocalRefOff>(rE->at, stackTop, offset);
        if ( rightType.isHandle() ) {
            auto resN = rightType.annotation->simulateCopy(context, at, left,
                (!rightType.isRefType() && rightType.ref) ? rightType.annotation->simulateRef2Value(context, at, right) : right);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateCopy returned null", "", "",
                                           at, CompilationError::missing_node );
            }
            return resN;
        } else if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeLocalMove (const LineInfo & at, Context & context, uint32_t stackTop, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, to the regular move
        auto left = context.code->makeNode<SimNode_GetLocal>(at, stackTop);
        auto right = rE->simulate(context);
        if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else  {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeLocalCopy(const LineInfo & at, Context & context, uint32_t stackTop, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( rightType.canCopy() &&
                "we are calling makeLocalCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
                return right;
            }
        }
        // now, to the regular copy
        auto left = context.code->makeNode<SimNode_GetLocal>(rE->at, stackTop);
        auto right = rE->simulate(context);
        if ( rightType.isHandle() ) {
            auto resN = rightType.annotation->simulateCopy(context, at, left,
                (!rightType.isRefType() && rightType.ref) ? rightType.annotation->simulateRef2Value(context, at, right) : right);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateCopy returned null", "", "",
                                           at, CompilationError::missing_node );
            }
            return resN;
        } else if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeCopy(const LineInfo & at, Context & context, const ExpressionPtr & lE, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        DAS_ASSERT ( (rightType.canCopy() || rightType.isGoodBlockType()) &&
                "we are calling makeCopy on a type, which can't be copied."
                "we should not be here, script compiler should have caught this during compilation."
                "compiler later will likely report internal compilation error.");
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = lE->simulate(context);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = lE->simulate(context);
                return right;
            }
        }
        // now, to the regular copy
        auto left = lE->simulate(context);
        auto right = rE->simulate(context);
        if ( rightType.isHandle() ) {
            auto resN = rightType.annotation->simulateCopy(context, at, left,
                (!rightType.isRefType() && rightType.ref) ? rightType.annotation->simulateRef2Value(context, at, right) : right);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateCopy returned null", "", "",
                    at, CompilationError::missing_node );
            }
            return resN;
        } else if ( rightType.isRef() ) {
            return context.code->makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
        }
    }

    SimNode * makeMove (const LineInfo & at, Context & context, const ExpressionPtr & lE, const ExpressionPtr & rE ) {
        const auto & rightType = *rE->type;
        // now, call with CMRES
        if ( rE->rtti_isCall() ) {
            auto cll = static_pointer_cast<ExprCall>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = lE->simulate(context);
                return right;
            }
        }
        // now, invoke with CMRES
        if ( rE->rtti_isInvoke() ) {
            auto cll = static_pointer_cast<ExprInvoke>(rE);
            if ( cll->allowCmresSkip() ) {
                SimNode_CallBase * right = (SimNode_CallBase *) rE->simulate(context);
                right->cmresEval = lE->simulate(context);
                return right;
            }
        }
        // now to the regular one
        if ( rightType.isRef() ) {
            auto left = lE->simulate(context);
            auto right = rE->simulate(context);
            return context.code->makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else {
            // this here might happen during initialization, by moving value types
            // like var t <- 5
            // its ok to generate simplified set here
            auto left = lE->simulate(context);
            auto right = rE->simulate(context);
            if ( rightType.baseType == Type::tHandle ) {
                // this is a value type, we need to copy it
                return ((TypeAnnotation*)(rightType.annotation))->simulateCopy(context, at, left, right);
            } else {
                return context.code->makeValueNode<SimNode_Set>(rightType.baseType, at, left, right);
            }
        }
    }

    SimNode * Function::makeSimNode ( Context & context, const vector<ExpressionPtr> & ) {
        {
            if ( copyOnReturn || moveOnReturn ) {
                return context.code->makeNodeUnrollAny<SimNode_CallAndCopyOrMove>(int(arguments.size()), at);
            } else if ( fastCall ) {
                return context.code->makeNodeUnrollAny<SimNode_FastCall>(int(arguments.size()), at);
            } else {
                return context.code->makeNodeUnrollAny<SimNode_Call>(int(arguments.size()), at);
            }
        }
    }

    SimNode * Function::simulate (Context & context) const {
        if ( builtIn ) {
            DAS_ASSERTF(0, "can only simulate non built-in function");
            return nullptr;
        }
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto fann = (FunctionAnnotation *)(ann->annotation.get());
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
        if ( fastCall ) {
            DAS_ASSERT(totalStackSize == sizeof(Prologue) && "function can't allocate stack");
            DAS_ASSERT((result->isWorkhorseType() || result->isVoid()) && "fastcall can only return a workhorse type");
            DAS_ASSERT(body->rtti_isBlock() && "function must contain a block");
            auto block = static_pointer_cast<ExprBlock>(body);
            if ( block->list.size()==0 ) {
                DAS_ASSERT(block->inFunction && block->inFunction->result->isVoid() && "only void function produces fastcall NOP");
                return context.code->makeNode<SimNode_NOP>(block->at);
            }
            if ( block->list.back()->rtti_isReturn() ) {
                DAS_ASSERT(block->list.back()->rtti_isReturn() && "fastcall body expr is return");
                auto retE = static_pointer_cast<ExprReturn>(block->list.back());
                if ( retE->subexpr ) {
                    return retE->subexpr->simulate(context);
                } else {
                    return context.code->makeNode<SimNode_NOP>(retE->at);
                }
            } else {
                return block->list.back()->simulate(context);
            }
        } else {
#if DAS_DEBUGGER
            if ( context.thisProgram->getDebugger() ) {
                auto sbody = body->simulate(context);
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
                return body->simulate(context);
            }
#else
            return body->simulate(context);
#endif
        }
    }

    SimNode * Expression::trySimulate (Context &, uint32_t, const TypeDeclPtr &) const {
        return nullptr;
    }

    void ExprMakeLocal::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        useStackRef = ref;
        useCMRES = cmres;
        doesNotNeedSp = true;
        doesNotNeedInit = true;
        stackTop = sp;
        extraOffset = off;
    }

    vector<SimNode *> ExprMakeLocal::simulateLocal ( Context & /*context*/ ) const {
        return vector<SimNode *>();
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
            auto fieldType = makeType->argTypes[fieldVariant];
            if ( decl->value->rtti_isMakeLocal() ) {
                auto fieldOffset = makeType->getVariantFieldOffset(fieldVariant);
                uint32_t offset =  extraOffset + index*stride + fieldOffset;
                auto mkl = static_pointer_cast<ExprMakeLocal>(decl->value);
                mkl->setRefSp(ref, cmres, sp, offset);
                mkl->doesNotNeedInit = false;
            } else if ( decl->value->rtti_isCall() ) {
                auto cll = static_pointer_cast<ExprCall>(decl->value);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( decl->value->rtti_isInvoke() ) {
                auto cll = static_pointer_cast<ExprInvoke>(decl->value);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
            index++;
        }
    }

    vector<SimNode *> ExprMakeVariant::simulateLocal (Context & context) const {
        vector<SimNode *> simlist;
        int index = 0;
        int stride = makeType->getStride();
        // init with 0 it its 'default' initialization
        if ( stride && variants.empty() ) {
            int bytes = stride;
            SimNode * init0;
            if ( useCMRES ) {
                if ( bytes <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, at,extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(at,extraOffset,bytes);
                }
            } else if ( useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(at,stackTop,extraOffset,bytes);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(at,stackTop + extraOffset,bytes);
            }
            simlist.push_back(init0);
        }
        // now fields
        for ( const auto & decl : variants ) {
            auto fieldVariant = makeType->findArgumentIndex(decl->name);
            DAS_ASSERT(fieldVariant!=-1 && "should have failed in type infer otherwise");
            // lets set variant index
            uint32_t voffset = extraOffset + index*stride;
            auto vconst = make_smart<ExprConstInt>(at, int32_t(fieldVariant));
            vconst->type = make_smart<TypeDecl>(Type::tInt);
            SimNode * svi;
            if ( useCMRES ) {
                svi = makeLocalCMResCopy(at,context,voffset,vconst);
            } else if (useStackRef) {
                svi = makeLocalRefCopy(at,context,stackTop,voffset,vconst);
            } else {
                svi = makeLocalCopy(at,context,stackTop+voffset,vconst);
            }
            simlist.push_back(svi);
            // field itself
            auto fieldOffset = makeType->getVariantFieldOffset(fieldVariant);
            uint32_t offset =  voffset + fieldOffset;
            SimNode * cpy = nullptr;
            if ( decl->value->rtti_isMakeLocal() ) {
                // so what happens here, is we ask it for the generated commands and append it to this list only
                auto mkl = static_pointer_cast<ExprMakeLocal>(decl->value);
                auto lsim = mkl->simulateLocal(context);
                simlist.insert(simlist.end(), lsim.begin(), lsim.end());
            } else if ( useCMRES ) {
                if ( decl->moveSemantics ){
                    cpy = makeLocalCMResMove(at,context,offset,decl->value);
                } else {
                    cpy = makeLocalCMResCopy(at,context,offset,decl->value);
                }
            } else if ( useStackRef ) {
                if ( decl->moveSemantics ){
                    cpy = makeLocalRefMove(at,context,stackTop,offset,decl->value);
                } else {
                    cpy = makeLocalRefCopy(at,context,stackTop,offset,decl->value);
                }
            } else {
                if ( decl->moveSemantics ){
                    cpy = makeLocalMove(at,context,stackTop+offset,decl->value);
                } else {
                    cpy = makeLocalCopy(at,context,stackTop+offset,decl->value);
                }
            }
            if ( cpy ) simlist.push_back(cpy);
            index++;
        }
        return simlist;
    }

    SimNode * ExprMakeVariant::simulate (Context & context) const {
        SimNode_Block * block;
        if ( useCMRES ) {
            block = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            block = context.code->makeNode<SimNode_MakeLocal>(at, stackTop);
        }
        auto simlist = simulateLocal(context);
        block->total = int(simlist.size());
        block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
        for ( uint32_t i=0, is=block->total; i!=is; ++i )
            block->list[i] = simlist[i];
        return block;
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
                    auto mkl = static_pointer_cast<ExprMakeLocal>(decl->value);
                    mkl->setRefSp(ref, cmres, sp, offset);
                } else if ( decl->value->rtti_isCall() ) {
                    auto cll = static_pointer_cast<ExprCall>(decl->value);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    }
                } else if ( decl->value->rtti_isInvoke() ) {
                    auto cll = static_pointer_cast<ExprInvoke>(decl->value);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    }
                }
            }
        }
    }

    vector<SimNode *> ExprMakeStruct::simulateLocal (Context & context) const {
        vector<SimNode *> simlist;
        // init with 0
        int total = int(structs.size());
        int stride = makeType->getStride();
        // note: if its an empty tuple init, like [[tuple<int;float>]] and its embedded - we need to zero it out
        bool emptyEmbeddedTuple = ( makeType->baseType==Type::tTuple && total==0);
        bool partialyInitStruct = !doesNotNeedInit && !initAllFields;
        if ( (emptyEmbeddedTuple || partialyInitStruct) && stride ) {
            int bytes = das::max(total,1) * stride;
            SimNode * init0;
            if ( useCMRES ) {
                if ( bytes <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, at,extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(at,extraOffset,bytes);
                }
            } else if ( useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(at,stackTop,extraOffset,bytes);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(at,stackTop + extraOffset,bytes);
            }
            simlist.push_back(init0);
        }
        if ( makeType->baseType == Type::tStructure ) {
            for ( int index=0; index != total; ++index ) {
                if ( constructor ) {
                    uint32_t offset =  extraOffset + index*stride;
                    SimNode_CallBase * pCall = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_CallAndCopyOrMove>(0, at);
                    DAS_ASSERT(constructor->index!=-1 && "should have failed in type infer otherwise");
                    pCall->fnPtr = context.getFunction(constructor->index);
                    if ( useCMRES ) {
                        pCall->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(at, offset);
                    } else if ( useStackRef ) {
                        pCall->cmresEval = context.code->makeNode<SimNode_GetLocalRefOff>(at, stackTop, offset);
                    } else {
                        pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at, stackTop + offset);
                    }
                    simlist.push_back(pCall);
                }
                auto & fields = structs[index];
                for ( const auto & decl : *fields ) {
                    auto field = makeType->structType->findField(decl->name);
                    DAS_ASSERT(field && "should have failed in type infer otherwise");
                    uint32_t offset =  extraOffset + index*stride + field->offset;
                    SimNode * cpy;
                    if ( decl->value->rtti_isMakeLocal() ) {
                        // so what happens here, is we ask it for the generated commands and append it to this list only
                        auto mkl = static_pointer_cast<ExprMakeLocal>(decl->value);
                        auto lsim = mkl->simulateLocal(context);
                        simlist.insert(simlist.end(), lsim.begin(), lsim.end());
                        continue;
                    } else if ( useCMRES ) {
                        if ( decl->moveSemantics ){
                            cpy = makeLocalCMResMove(at,context,offset,decl->value);
                        } else {
                            cpy = makeLocalCMResCopy(at,context,offset,decl->value);
                        }
                    } else if ( useStackRef ) {
                        if ( decl->moveSemantics ){
                            cpy = makeLocalRefMove(at,context,stackTop,offset,decl->value);
                        } else {
                            cpy = makeLocalRefCopy(at,context,stackTop,offset,decl->value);
                        }
                    } else {
                        if ( decl->moveSemantics ){
                            cpy = makeLocalMove(at,context,stackTop+offset,decl->value);
                        } else {
                            cpy = makeLocalCopy(at,context,stackTop+offset,decl->value);
                        }
                    }
                    if ( !cpy ) {
                        context.thisProgram->error("internal compilation error, can't generate structure initialization", "", "", at);
                    }
                    simlist.push_back(cpy);
                }
            }
        } else {
            auto ann = makeType->annotation;
            // making fake variable, which points to out field
            string fakeName = "__makelocal";
            auto fakeVariable = make_smart<Variable>();
            fakeVariable->name = fakeName;
            fakeVariable->type = make_smart<TypeDecl>(Type::tHandle);
            fakeVariable->type->annotation = ann;
            fakeVariable->at = at;
            if ( useCMRES ) {
                fakeVariable->aliasCMRES = true;
            } else if ( useStackRef ) {
                fakeVariable->stackTop = stackTop;
                fakeVariable->extraLocalOffset = extraOffset;
                fakeVariable->type->ref = true;
                if ( total != 1 ) {
                    fakeVariable->type->dim.push_back(total);
                }
            }
            fakeVariable->generated = true;
            // make fake ExprVar which is that field
            auto fakeVar = make_smart<ExprVar>(at, fakeName);
            fakeVar->type = fakeVariable->type;
            fakeVar->variable = fakeVariable;
            fakeVar->local = true;
            // make fake expression
            ExpressionPtr fakeExpr = fakeVar;
            smart_ptr<ExprConstInt> indexExpr;
            if ( useStackRef && total > 1 ) {
                // if its stackRef with multiple indices, its actually var[total], and lookup is var[index]
                indexExpr = make_smart<ExprConstInt>(at, 0);
                indexExpr->type = make_smart<TypeDecl>(Type::tInt);
                fakeExpr = make_smart<ExprAt>(at, fakeExpr, indexExpr);
                fakeExpr->type = make_smart<TypeDecl>(Type::tHandle);
                fakeExpr->type->annotation = ann;
                fakeExpr->type->ref = true;
            }
            for ( int index=0; index != total; ++index ) {
                auto & fields = structs[index];
                // adjust var for index
                if ( useCMRES ) {
                    fakeVariable->stackTop = extraOffset + index*stride;
                } else if ( useStackRef ) {
                    if ( total > 1 ) {
                        indexExpr->value = cast<int32_t>::from(index);
                    }
                } else {
                    fakeVariable->stackTop = stackTop + extraOffset + index*stride;
                }
                // now, setup fields
                for ( const auto & decl : *fields ) {
                    auto fieldType = ann->makeFieldType(decl->name, false);
                    DAS_ASSERT(fieldType && "how did this infer?");
                    uint32_t fieldSize = fieldType->getSizeOf();
                    SimNode * cpy = nullptr;
                    uint32_t fieldOffset = ann->getFieldOffset(decl->name);
                    SimNode * simV = fakeExpr->simulate(context);
                    auto left = context.code->makeNode<SimNode_FieldDeref>(at,simV,fieldOffset);
                    auto right = decl->value->simulate(context);
                    if ( !decl->value->type->isRef() ) {
                        if ( decl->value->type->isHandle() ) {
                            auto rightType = decl->value->type;
                            cpy = rightType->annotation->simulateCopy(context, at, left,
                                (!rightType->isRefType() && rightType->ref) ? rightType->annotation->simulateRef2Value(context, at, right) : right);
                            if ( !cpy ) {
                                context.thisProgram->error("integration error, simulateCopy returned null", "", "",
                                                        at, CompilationError::missing_node );
                            }
                        } else {
                            cpy = context.code->makeValueNode<SimNode_Set>(decl->value->type->baseType, decl->at, left, right);
                        }
                    } else if ( decl->moveSemantics ) {
                        cpy = context.code->makeNode<SimNode_MoveRefValue>(decl->at, left, right, fieldSize);
                    } else {
                        cpy = context.code->makeNode<SimNode_CopyRefValue>(decl->at, left, right, fieldSize);
                    }
                    simlist.push_back(cpy);
                }
            }
        }
        if ( block ) {
            /*
                TODO: optimize
                    there is no point in making fake invoke expression, we can replace 'self' with fake variable we've made
                    however this needs to happen during infer, and this needs to have different visitor,
                    so that stack is allocated properly in subexpressions etc
            */
            // making fake variable, which points to entire structure
            string fakeName = "__makelocal";
            auto fakeVariable = make_smart<Variable>();
            fakeVariable->name = fakeName;
            fakeVariable->type = make_smart<TypeDecl>(*type);
            if ( useCMRES ) {
                fakeVariable->aliasCMRES = true;
            } else if ( useStackRef ) {
                fakeVariable->stackTop = stackTop + extraOffset;
                fakeVariable->type->ref = true;
            } else {
                fakeVariable->stackTop = stackTop + extraOffset;
            }
            fakeVariable->generated = true;
            // make fake ExprVar which is that field
            auto fakeVar = make_smart<ExprVar>(at, fakeName);
            fakeVar->type = fakeVariable->type;
            fakeVar->variable = fakeVariable;
            fakeVar->local = true;
            // make fake invoke expression
            auto fakeInvoke = make_smart<ExprInvoke>(at,"invoke");
            fakeInvoke->arguments.push_back(block);
            fakeInvoke->arguments.push_back(fakeVar);
            // simulate it
            auto simI = fakeInvoke->simulate(context);
            simlist.push_back(simI);
        }
        return simlist;
    }

    SimNode * ExprMakeStruct::simulate (Context & context) const {
        SimNode_Block * blk;
        if ( useCMRES ) {
            blk = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            blk = context.code->makeNode<SimNode_MakeLocal>(at, stackTop);
        }
        auto simlist = simulateLocal(context);
        blk->total = int(simlist.size());
        blk->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*blk->total);
        for ( uint32_t i=0, is=blk->total; i!=is; ++i )
            blk->list[i] = simlist[i];
        return blk;
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
                auto mkl = static_pointer_cast<ExprMakeLocal>(val);
                mkl->setRefSp(ref, cmres, sp, offset);
            } else if ( val->rtti_isCall() ) {
                auto cll = static_pointer_cast<ExprCall>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( val->rtti_isInvoke() ) {
                auto cll = static_pointer_cast<ExprInvoke>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
        }
    }

    vector<SimNode *> ExprMakeArray::simulateLocal (Context & context) const {
        vector<SimNode *> simlist;
        // init with 0
        int total = int(values.size());
        uint32_t stride = recordType->getSizeOf();
        if ( !doesNotNeedInit && !initAllFields ) {
            int bytes = total * stride;
            SimNode * init0;
            if ( useCMRES ) {
                if ( bytes <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, at,extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(at,extraOffset,bytes);
                }
            } else if ( useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(at,stackTop,extraOffset,stride * total);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(at,stackTop + extraOffset,stride * total);
            }
            simlist.push_back(init0);
        }
        for ( int index=0; index != total; ++index ) {
            auto & val = values[index];
            uint32_t offset = extraOffset + index*stride;
            SimNode * cpy;
            if ( val->rtti_isMakeLocal() ) {
                // so what happens here, is we ask it for the generated commands and append it to this list only
                auto mkl = static_pointer_cast<ExprMakeLocal>(val);
                auto lsim = mkl->simulateLocal(context);
                simlist.insert(simlist.end(), lsim.begin(), lsim.end());
                continue;
            } else if ( useCMRES ) {
                if (val->type->canCopy()) {
                    cpy = makeLocalCMResCopy(at, context, offset, val);
                } else {
                    cpy = makeLocalCMResMove(at, context, offset, val);
                }
            } else if ( useStackRef ) {
                if (val->type->canCopy()) {
                    cpy = makeLocalRefCopy(at, context, stackTop, offset, val);
                } else {
                    cpy = makeLocalRefMove(at, context, stackTop, offset, val);
                }
            } else {
                if (val->type->canCopy()) {
                    cpy = makeLocalCopy(at, context, stackTop + offset, val);
                } else {
                    cpy = makeLocalMove(at, context, stackTop + offset, val);
                }
            }
            if ( !cpy ) {
                context.thisProgram->error("internal compilation error, can't generate array initialization", "", "", at);
            }
            simlist.push_back(cpy);
        }
        return simlist;
    }

    SimNode * ExprMakeArray::simulate (Context & context) const {
        SimNode_Block * block;
        if ( useCMRES ) {
            block = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            block = context.code->makeNode<SimNode_MakeLocal>(at, stackTop);
        }
        auto simlist = simulateLocal(context);
        block->total = int(simlist.size());
        block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
        for ( uint32_t i=0, is=block->total; i!=is; ++i )
            block->list[i] = simlist[i];
        return block;
    }

    // make tuple

    void ExprMakeTuple::setRefSp ( bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        ExprMakeLocal::setRefSp(ref, cmres, sp, off);
        int total = int(values.size());
        for ( int index=0; index != total; ++index ) {
            auto & val = values[index];
            if ( val->rtti_isMakeLocal() ) {
                uint32_t offset =  extraOffset + makeType->getTupleFieldOffset(index);
                auto mkl = static_pointer_cast<ExprMakeLocal>(val);
                mkl->setRefSp(ref, cmres, sp, offset);
            } else if ( val->rtti_isCall() ) {
                auto cll = static_pointer_cast<ExprCall>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( val->rtti_isInvoke() ) {
                auto cll = static_pointer_cast<ExprInvoke>(val);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
        }
    }

    vector<SimNode *> ExprMakeTuple::simulateLocal (Context & context) const {
        vector<SimNode *> simlist;
        // init with 0
        int total = int(values.size());
        if ( !doesNotNeedInit && !initAllFields ) {
            uint32_t sizeOf = makeType->getSizeOf();
            SimNode * init0;
            if ( useCMRES ) {
                if ( sizeOf <= 32 ) {
                    init0 = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(sizeOf, at,extraOffset);
                } else {
                    init0 = context.code->makeNode<SimNode_InitLocalCMRes>(at,extraOffset,sizeOf);
                }
            } else if ( useStackRef ) {
                init0 = context.code->makeNode<SimNode_InitLocalRef>(at,stackTop,extraOffset,sizeOf);
            } else {
                init0 = context.code->makeNode<SimNode_InitLocal>(at,stackTop + extraOffset,sizeOf);
            }
            simlist.push_back(init0);
        }
        for ( int index=0; index != total; ++index ) {
            auto & val = values[index];
            uint32_t offset = extraOffset + makeType->getTupleFieldOffset(index);
            SimNode * cpy;
            if ( val->rtti_isMakeLocal() ) {
                // so what happens here, is we ask it for the generated commands and append it to this list only
                auto mkl = static_pointer_cast<ExprMakeLocal>(val);
                auto lsim = mkl->simulateLocal(context);
                simlist.insert(simlist.end(), lsim.begin(), lsim.end());
                continue;
            } else if ( useCMRES ) {
                if (val->type->canCopy()) {
                    cpy = makeLocalCMResCopy(at, context, offset, val);
                } else {
                    cpy = makeLocalCMResMove(at, context, offset, val);
                }
            } else if ( useStackRef ) {
                if (val->type->canCopy()) {
                    cpy = makeLocalRefCopy(at, context, stackTop, offset, val);
                } else {
                    cpy = makeLocalRefMove(at, context, stackTop, offset, val);
                }
            } else {
                if (val->type->canCopy()) {
                    cpy = makeLocalCopy(at, context, stackTop + offset, val);
                } else {
                    cpy = makeLocalMove(at, context, stackTop + offset, val);
                }
            }
            if ( !cpy ) {
                context.thisProgram->error("internal compilation error, can't generate array initialization", "", "", at);
            }
            simlist.push_back(cpy);
        }
        return simlist;
    }


    SimNode * ExprMakeTuple::simulate (Context & context) const {
        SimNode_Block * block;
        if ( useCMRES ) {
            block = context.code->makeNode<SimNode_MakeLocalCMRes>(at);
        } else {
            block = context.code->makeNode<SimNode_MakeLocal>(at, stackTop);
        }
        auto simlist = simulateLocal(context);
        block->total = int(simlist.size());
        block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
        for ( uint32_t i=0, is=block->total; i!=is; ++i )
            block->list[i] = simlist[i];
        return block;
    }

    // reader

    SimNode * ExprReader::simulate (Context & context) const {
        context.thisProgram->error("internal compilation error, calling 'simulate' on reader", "", "", at);
        return nullptr;
    }

    // label

    SimNode * ExprLabel::simulate (Context & context) const {
        context.thisProgram->error("internal compilation error, calling 'simulate' on label", "", "", at);
        return nullptr;
    }

    // goto

    SimNode * ExprGoto::simulate (Context & context) const {
        if ( subexpr ) {
            return context.code->makeNode<SimNode_Goto>(at, subexpr->simulate(context));
        } else {
            return context.code->makeNode<SimNode_GotoLabel>(at,label);
        }
    }

    // r2v

    SimNode * ExprRef2Value::GetR2V ( Context & context, const LineInfo & at, const TypeDeclPtr & type, SimNode * expr ) {
        if ( type->isHandle() ) {
            auto resN = type->annotation->simulateRef2Value(context, at, expr);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateRef2Value returned null", "", "",
                                           at, CompilationError::missing_node );
            }
            return resN;
        } else {
            if ( type->isRefType() ) {
                return expr;
            } else {
                return context.code->makeValueNode<SimNode_Ref2Value>(type->baseType, at, expr);
            }
        }
    }

    SimNode * ExprRef2Value::simulate (Context & context) const {
        return GetR2V(context, at, type, subexpr->simulate(context));
    }

    SimNode * ExprAddr::simulate (Context & context) const {
        if ( !func ) {
            context.thisProgram->error("internal compilation error, ExprAddr func is null", "", "", at);
            return nullptr;
        } else if ( func->index<0 ) {
            context.thisProgram->error("internal compilation error, ExprAddr func->index is unused", "", "", at);
            return nullptr;

        }
        union {
            uint64_t    mnh;
            vec4f       cval;
        } temp;
        temp.cval = v_zero();
        if ( func->module->isSolidContext ) {
            DAS_ASSERT(func->index>=0 && "address of unsued function? how?");
            temp.mnh = func->index;
            return context.code->makeNode<SimNode_FuncConstValue>(at,temp.cval);
        } else {
            temp.mnh = func->getMangledNameHash();
            return context.code->makeNode<SimNode_FuncConstValueMnh>(at,temp.cval);
        }
    }

    SimNode * ExprPtr2Ref::simulate (Context & context) const {
        if ( unsafeDeref ) {
            return subexpr->simulate(context);
        } else {
            return context.code->makeNode<SimNode_Ptr2Ref>(at,subexpr->simulate(context));
        }
    }

    SimNode * ExprRef2Ptr::simulate (Context & context) const {
        return subexpr->simulate(context);
    }

    SimNode * ExprNullCoalescing::simulate (Context & context) const {
        if ( type->isRef() ) {
            return context.code->makeNode<SimNode_NullCoalescingRef>(at,subexpr->simulate(context),defaultValue->simulate(context));
        } else if ( type->isHandle() ) {
            if ( auto resN = type->annotation->simulateNullCoalescing(context, at, subexpr->simulate(context), defaultValue->simulate(context)) ) {
                return resN;
            } else {
                context.thisProgram->error("internal compilation error, simluateNullCoalescing returned null", "", "", at);
                return nullptr;
            }
        } else {
            return context.code->makeValueNode<SimNode_NullCoalescing>(type->baseType,at,subexpr->simulate(context),defaultValue->simulate(context));
        }
    }

    SimNode * ExprConst::simulate (Context & context) const {
        return context.code->makeNode<SimNode_ConstValue>(at,value);
    }

    SimNode * ExprConstEnumeration::simulate (Context & context) const {
        return context.code->makeNode<SimNode_ConstValue>(at, value);
    }

    SimNode * ExprConstString::simulate (Context & context) const {
        if ( !text.empty() ) {
            char* str = context.constStringHeap->impl_allocateString(text);
            return context.code->makeNode<SimNode_ConstString>(at, str);
        } else {
            return context.code->makeNode<SimNode_ConstString>(at, nullptr);
        }
    }

    SimNode * ExprStaticAssert::simulate (Context &) const {
        return nullptr;
    }

    SimNode * ExprAssert::simulate (Context & context) const {
        string message;
        if ( arguments.size()==2 && arguments[1]->rtti_isStringConstant() )
            message = static_pointer_cast<ExprConstString>(arguments[1])->getValue();
        return context.code->makeNode<SimNode_Assert>(at,arguments[0]->simulate(context),context.constStringHeap->impl_allocateString(message));
    }

    struct SimNode_AstGetExpression : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_AstGetExpression ( const LineInfo & at, const ExpressionPtr & e, char * d )
            : SimNode_CallBase(at) {
            expr = e.get();
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
            return (char *) expr->clone().orphan();
        }
        Expression *  expr;   // requires RTTI
        char *        descr;
    };

    SimNode * ExprQuote::simulate (Context & context) const {
        DAS_ASSERTF(arguments.size()==1,"Quote expects to return only one ExpressionPtr."
        "We should not be here, since typeinfer should catch the mismatch.");
        TextWriter ss;
        ss << *arguments[0];
        char * descr = context.code->allocateName(ss.str());
        return context.code->makeNode<SimNode_AstGetExpression>(at, arguments[0], descr);
    }

    SimNode * ExprDebug::simulate (Context & context) const {
        TypeInfo * pTypeInfo = context.thisHelper->makeTypeInfo(nullptr, arguments[0]->type);
        string message;
        if ( arguments.size()==2 && arguments[1]->rtti_isStringConstant() )
            message = static_pointer_cast<ExprConstString>(arguments[1])->getValue();
        return context.code->makeNode<SimNode_Debug>(at,
                                               arguments[0]->simulate(context),
                                               pTypeInfo,
                                               context.constStringHeap->impl_allocateString(message));
    }

    SimNode * ExprMemZero::simulate (Context & context) const {
        const auto & subexpr = arguments[0];
        uint32_t dataSize = subexpr->type->getSizeOf();
        return context.code->makeNode<SimNode_MemZero>(at, subexpr->simulate(context), dataSize);
    }

    SimNode * ExprMakeGenerator::simulate (Context & context) const {
        DAS_ASSERTF(0, "we should not be here ever, ExprMakeGenerator should completly fold during type inference.");
        context.thisProgram->error("internal compilation error, generating node for ExprMakeGenerator", "", "", at);
        return nullptr;
    }

    SimNode * ExprYield::simulate (Context & context) const {
        DAS_ASSERTF(0, "we should not be here ever, ExprYield should completly fold during type inference.");
        context.thisProgram->error("internal compilation error, generating node for ExprYield", "", "", at);
        return nullptr;
    }

    SimNode * ExprArrayComprehension::simulate (Context & context) const {
        DAS_ASSERTF(0, "we should not be here ever, ExprArrayComprehension should completly fold during type inference.");
        context.thisProgram->error("internal compilation error, generating node for ExprArrayComprehension", "", "", at);
        return nullptr;
    }

    SimNode * ExprMakeBlock::simulate (Context & context) const {
        auto blk = static_pointer_cast<ExprBlock>(block);
        uint32_t argSp = blk->stackTop;
        auto info = context.thisHelper->makeInvokeableTypeDebugInfo(blk->makeBlockType(),blk->at);
        if ( context.thisProgram->getDebugger() || context.thisProgram->options.getBoolOption("gc",false) ) {
            context.thisHelper->appendLocalVariables(info, (Expression *)this);
        }
        return context.code->makeNode<SimNode_MakeBlock>(at,block->simulate(context),argSp,stackTop,info);
    }

    bool ExprInvoke::isCopyOrMove() const {
        auto blockT = arguments[0]->type;
        return blockT->firstType && blockT->firstType->isRefType() && !blockT->firstType->ref;
    }

    SimNode * ExprInvoke::simulate (Context & context) const {
        auto blockT = arguments[0]->type;
        SimNode_CallBase * pInvoke;
        uint32_t methodOffset = 0;
        if ( isInvokeMethod ) {
            bool foundOffset = false;
            if ( arguments[0]->rtti_isField() ) {
                auto field = static_pointer_cast<ExprField>(arguments[0]);
                methodOffset = field->field->offset;
                foundOffset = true;
            } else if ( arguments[0]->rtti_isR2V() ) {
                auto eR2V = static_pointer_cast<ExprRef2Value>(arguments[0]);
                if ( eR2V->subexpr->rtti_isField() ) {
                    auto field = static_pointer_cast<ExprField>(eR2V->subexpr);
                    methodOffset = field->field->offset;
                    foundOffset = true;
                }
            }
            if (!foundOffset) {
                context.thisProgram->error("internal compilation error, invoke method expects field", "", "", at);
                return nullptr;
            }
        }
        {
            if ( isCopyOrMove() ) {
                DAS_ASSERTF ( blockT->baseType!=Type::tString, "its never CMRES for named function" );
                auto getSp = context.code->makeNode<SimNode_GetLocal>(at,stackTop);
                if ( blockT->baseType==Type::tBlock ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMove>(
                                                        int(arguments.size()), at, getSp);
                } else if ( blockT->baseType==Type::tFunction && isInvokeMethod ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMoveMethod>(
                                                        int(arguments.size()-1), at, getSp);
                    ((SimNode_InvokeAndCopyOrMoveMethodAny *) pInvoke)->methodOffset = methodOffset;
                } else if ( blockT->baseType==Type::tFunction ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMoveFn>(
                                                        int(arguments.size()), at, getSp);
                } else {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeAndCopyOrMoveLambda>(
                                                        int(arguments.size()), at, getSp);
                }
            } else {
                if ( blockT->baseType==Type::tString ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeFnByName>(int(arguments.size()),at);
                } else if ( blockT->baseType==Type::tBlock ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_Invoke>(int(arguments.size()),at);
                } else if ( blockT->baseType==Type::tFunction && isInvokeMethod ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeMethod>(int(arguments.size()-1),at);
                    ((SimNode_InvokeMethodAny *) pInvoke)->methodOffset = methodOffset;
                } else if ( blockT->baseType==Type::tFunction ) {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeFn>(int(arguments.size()),at);
                } else {
                    pInvoke = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_InvokeLambda>(int(arguments.size()),at);
                }
            }
        }
        pInvoke->debugInfo = at;
        int nArg = (int) arguments.size();
        if ( isInvokeMethod ) nArg --;
        if (  nArg ) {
            pInvoke->arguments = (SimNode **) context.code->allocate(nArg * sizeof(SimNode *));
            pInvoke->nArguments = nArg;
            if ( !isInvokeMethod ) {
                for ( int a=0; a!=nArg; ++a ) {
                    pInvoke->arguments[a] = arguments[a]->simulate(context);
                }
            } else {
                for ( int a=0; a!=nArg; ++a ) {
                    pInvoke->arguments[a] = arguments[a+1]->simulate(context);
                }
            }
        } else {
            pInvoke->arguments = nullptr;
            pInvoke->nArguments = 0;
        }
        return pInvoke;
    }

    SimNode * ExprErase::simulate (Context & context) const {
        auto cont = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        if ( arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = arguments[0]->type->secondType->getSizeOf();
            Type valueType;
            if ( arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            return context.code->makeValueNode<SimNode_TableErase>(valueType, at, cont, val, valueTypeSize);
        } else {
            DAS_ASSERTF(0, "we should not even be here. erase can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating erase for non-table type", "", "", at);
            return nullptr;
        }
    }

    SimNode * ExprSetInsert::simulate (Context & context) const {
        auto cont = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        if ( arguments[0]->type->isGoodTableType() ) {
            DAS_ASSERTF(arguments[0]->type->secondType->getSizeOf()==0,"Expecting value type size to be 0 for set insert");
            Type valueType;
            if ( arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            return context.code->makeValueNode<SimNode_TableSetInsert>(valueType, at, cont, val);
        } else {
            DAS_ASSERTF(0, "we should not even be here. erase can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating erase for non-table type", "", "", at);
            return nullptr;
        }
    }

    SimNode * ExprFind::simulate (Context & context) const {
        auto cont = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        if ( arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = arguments[0]->type->secondType->getSizeOf();
            Type valueType;
            if ( arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            return context.code->makeValueNode<SimNode_TableFind>(valueType, at, cont, val, valueTypeSize);
        } else {
            DAS_ASSERTF(0, "we should not even be here. find can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating find for non-table type", "", "", at);
            return nullptr;
        }
    }

    SimNode * ExprKeyExists::simulate (Context & context) const {
        auto cont = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        if ( arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = arguments[0]->type->secondType->getSizeOf();
            Type valueType;
            if ( arguments[0]->type->firstType->isWorkhorseType() ) {
                valueType = arguments[0]->type->firstType->baseType;
            } else {
                auto valueT = arguments[0]->type->firstType->annotation->makeValueType();
                valueType = valueT->baseType;
                val = context.code->makeNode<SimNode_CastToWorkhorse>(at, val);
            }
            return context.code->makeValueNode<SimNode_KeyExists>(valueType, at, cont, val, valueTypeSize);
        } else {
            DAS_ASSERTF(0, "we should not even be here. find can only accept tables. infer type should have failed.");
            context.thisProgram->error("internal compilation error, generating find for non-table type", "", "", at);
            return nullptr;
        }
    }

    SimNode * ExprIs::simulate (Context & context) const {
        DAS_ASSERTF(0, "we should not even be here. 'is' should resolve to const during infer pass.");
        context.thisProgram->error("internal compilation error, generating 'is'", "", "", at);
        return nullptr;
    }

    SimNode * ExprTypeDecl::simulate (Context & context) const {
        return context.code->makeNode<SimNode_ConstValue>(at,v_zero());
    }

    SimNode * ExprTypeInfo::simulate (Context & context) const {
        if ( !macro ) {
            DAS_ASSERTF(0, "we should not even be here. typeinfo should resolve to const during infer pass.");
            context.thisProgram->error("internal compilation error, generating typeinfo(...)", "", "", at);
            return nullptr;
        } else {
            string errors;
            auto node = macro->simluate(&context, (Expression*)this, errors);
            if ( !node || !errors.empty() ) {
                context.thisProgram->error("typeinfo(" + trait + "...) macro generated no node; " + errors,
                    "", "", at, CompilationError::typeinfo_macro_error);
            }
            return node;
        }
    }

    SimNode * ExprDelete::simulate (Context & context) const {
        uint32_t total = uint32_t(subexpr->type->getCountOf());
        DAS_ASSERTF(total==1,"we should not be deleting more than one at a time");
        auto sube = subexpr->simulate(context);
        if ( subexpr->type->baseType==Type::tArray ) {
            auto stride = subexpr->type->firstType->getSizeOf();
            return context.code->makeNode<SimNode_DeleteArray>(at, sube, total, stride);
        } else if ( subexpr->type->baseType==Type::tTable ) {
            auto vts_add_kts = subexpr->type->firstType->getSizeOf() +
                subexpr->type->secondType->getSizeOf();
            return context.code->makeNode<SimNode_DeleteTable>(at, sube, total, vts_add_kts);
        } else if ( subexpr->type->baseType==Type::tPointer ) {
            if ( subexpr->type->firstType->baseType==Type::tStructure ) {
                bool persistent = subexpr->type->firstType->structType->persistent;
                if ( subexpr->type->firstType->structType->isClass ) {
                    if ( sizeexpr ) {
                        auto sze = sizeexpr->simulate(context);
                        return context.code->makeNode<SimNode_DeleteClassPtr>(at, sube, total, sze, persistent);
                    } else {
                        context.thisProgram->error("internal compiler error: SimNode_DeleteClassPtr needs size expression", "", "",
                                                at, CompilationError::missing_node );
                        return nullptr;
                    }
                } else {
                    auto structSize = subexpr->type->firstType->getSizeOf();
                    bool isLambda = subexpr->type->firstType->structType->isLambda;
                    return context.code->makeNode<SimNode_DeleteStructPtr>(at, sube, total, structSize, persistent, isLambda);
                }
            } else if ( subexpr->type->firstType->baseType==Type::tTuple ) {
                auto structSize = subexpr->type->firstType->getSizeOf();
                return context.code->makeNode<SimNode_DeleteStructPtr>(at, sube, total, structSize, false, false);
            } else if ( subexpr->type->firstType->baseType==Type::tVariant ) {
                auto structSize = subexpr->type->firstType->getSizeOf();
                return context.code->makeNode<SimNode_DeleteStructPtr>(at, sube, total, structSize, false, false);
            } else {
                auto ann = subexpr->type->firstType->annotation;
                DAS_ASSERT(ann->canDeletePtr() && "has to be able to delete ptr");
                auto resN = ann->simulateDeletePtr(context, at, sube, total);
                if ( !resN ) {
                    context.thisProgram->error("integration error, simulateDelete returned null", "", "",
                                               at, CompilationError::missing_node );
                }
                return resN;
            }
        } else if ( subexpr->type->baseType==Type::tHandle ) {
            auto ann = subexpr->type->annotation;
            DAS_ASSERT(ann->canDelete() && "has to be able to delete");
            auto resN =  ann->simulateDelete(context, at, sube, total);
            if ( !resN ) {
                context.thisProgram->error("integration error, simulateDelete returned null", "", "",
                                           at, CompilationError::missing_node );
            }
            return resN;
        } else if ( subexpr->type->baseType==Type::tLambda ) {
            return context.code->makeNode<SimNode_DeleteLambda>(at, sube, total);
        } else {
            DAS_ASSERTF(0, "we should not be here. this is delete for unsupported type. infer types should have failed.");
            context.thisProgram->error("internal compiler error: generating node for unsupported ExprDelete", "", "", at);
            return nullptr;
        }
    }
    SimNode * ExprCast::trySimulate (Context & context, uint32_t extraOffset, const TypeDeclPtr & r2vType ) const {
        return subexpr->trySimulate(context, extraOffset, r2vType);
    }

    SimNode * ExprCast::simulate (Context & context) const {
        return subexpr->simulate(context);
    }

    SimNode * ExprAscend::simulate (Context & context) const {
        auto se = subexpr->simulate(context);
        auto bytes = subexpr->type->getSizeOf();
        TypeInfo * typeInfo = nullptr;
        if ( needTypeInfo ) {
            typeInfo = context.thisHelper->makeTypeInfo(nullptr, subexpr->type);
        }
        if ( subexpr->type->baseType==Type::tHandle ) {
            DAS_ASSERTF(useStackRef,"new of handled type should always be over stackref");
            auto ne = subexpr->type->annotation->simulateGetNew(context, at);
            return context.code->makeNode<SimNode_AscendNewHandleAndRef>(at, se, ne, bytes, stackTop);
        } else {
            bool peristent = false;
            if ( subexpr->type->baseType==Type::tStructure ) {
                peristent = subexpr->type->structType->persistent;
            }
            if ( useStackRef ) {
                return context.code->makeNode<SimNode_AscendAndRef>(at, se, bytes, stackTop, typeInfo, peristent);
            } else {
                return context.code->makeNode<SimNode_Ascend<false>>(at, se, bytes, typeInfo, peristent);
            }
        }
    }

    SimNode * ExprNew::simulate (Context & context) const {
        SimNode * newNode;
        if ( typeexpr->baseType == Type::tHandle ) {
            DAS_ASSERT(typeexpr->annotation->canNew() && "how???");
            if ( initializer ) {
                auto pCall = static_cast<SimNode_CallBase *>(func->makeSimNode(context,arguments));
                ExprCall::simulateCall(func, this, context, pCall);
                pCall->cmresEval = typeexpr->annotation->simulateGetNew(context, at);
                if ( !pCall->cmresEval ) {
                    context.thisProgram->error("integration error, simulateGetNew returned null", "", "",
                                            at, CompilationError::missing_node );
                }
                return pCall;
            } else {
                newNode = typeexpr->annotation->simulateGetNew(context, at);
                if ( !newNode ) {
                    context.thisProgram->error("integration error, simulateGetNew returned null", "", "",
                                            at, CompilationError::missing_node );
                }
            }
        } else {
            bool persistent = false;
            if ( typeexpr->baseType == Type::tStructure ) {
                persistent = typeexpr->structType->persistent;
            }
            int32_t bytes = type->firstType->getBaseSizeOf();
            if ( initializer ) {
                auto pCall = (SimNode_CallBase *) context.code->makeNodeUnrollAny<SimNode_NewWithInitializer>(
                    int(arguments.size()),at,bytes,persistent);
                pCall->cmresEval = nullptr;
                newNode = ExprCall::simulateCall(func, this, context, pCall);
            } else {
                newNode = context.code->makeNode<SimNode_New>(at,bytes,persistent);
            }
        }
        if ( type->dim.size() ) {
            uint32_t count = type->getCountOf();
            return context.code->makeNode<SimNode_NewArray>(at,newNode,stackTop,count);
        } else {
            return newNode;
        }
    }

    SimNode * ExprAt::trySimulate (Context & context, uint32_t extraOffset, const TypeDeclPtr & r2vType ) const {
        if ( subexpr->type->isVectorType() ) {
            return nullptr;
        } else if ( subexpr->type->isGoodTableType() ) {
            return nullptr;
        } else if ( subexpr->type->isHandle() ) {
            SimNode * result;
            if ( r2vType->baseType!=Type::none ) {
                result = subexpr->type->annotation->simulateGetAtR2V(context, at, r2vType, subexpr, index, extraOffset);
                if ( !result ) {
                    context.thisProgram->error("integration error, simulateGetAtR2V returned null", "", "",
                                               at, CompilationError::missing_node );
                }
            } else {
                result = subexpr->type->annotation->simulateGetAt(context, at, r2vType, subexpr, index, extraOffset);
                if ( !result ) {
                    context.thisProgram->error("integration error, simulateGetAt returned null", "", "",
                                               at, CompilationError::missing_node );
                }
            }
            return result;
        } else if ( subexpr->type->isGoodArrayType() ) {
            auto prv = subexpr->simulate(context);
            auto pidx = index->simulate(context);
            uint32_t stride = subexpr->type->firstType->getSizeOf();
            if ( r2vType->baseType!=Type::none ) {
                return context.code->makeValueNode<SimNode_ArrayAtR2V>(r2vType->baseType, at, prv, pidx, stride, extraOffset);
            } else {
                return context.code->makeNode<SimNode_ArrayAt>(at, prv, pidx, stride, extraOffset);
            }
        } else if ( subexpr->type->isPointer() ) {
            uint32_t stride = subexpr->type->firstType->getSizeOf();
            auto prv = subexpr->simulate(context);
            auto pidx = index->simulate(context);
            if ( r2vType->baseType!=Type::none ) {
                switch ( index->type->baseType ) {
                case Type::tInt:    return context.code->makeValueNode<SimNode_PtrAtR2V_Int>(r2vType->baseType, at, prv, pidx, stride, extraOffset);
                case Type::tUInt:   return context.code->makeValueNode<SimNode_PtrAtR2V_UInt>(r2vType->baseType, at, prv, pidx, stride, extraOffset);
                case Type::tInt64:  return context.code->makeValueNode<SimNode_PtrAtR2V_Int64>(r2vType->baseType, at, prv, pidx, stride, extraOffset);
                case Type::tUInt64: return context.code->makeValueNode<SimNode_PtrAtR2V_UInt64>(r2vType->baseType, at, prv, pidx, stride, extraOffset);
                default:
                    context.thisProgram->error("internal compilation error, generating ptr at for unsupported index type " + index->type->describe(), "", "", at);
                    return nullptr;
                };
            } else {
                switch ( index->type->baseType ) {
                case Type::tInt:    return context.code->makeNode<SimNode_PtrAt<int32_t>>(at, prv, pidx, stride, extraOffset);
                case Type::tUInt:   return context.code->makeNode<SimNode_PtrAt<uint32_t>>(at, prv, pidx, stride, extraOffset);
                case Type::tInt64:  return context.code->makeNode<SimNode_PtrAt<int64_t>>(at, prv, pidx, stride, extraOffset);
                case Type::tUInt64: return context.code->makeNode<SimNode_PtrAt<uint64_t>>(at, prv, pidx, stride, extraOffset);
                default:
                    context.thisProgram->error("internal compilation error, generating ptr at for unsupported index type " + index->type->describe(), "", "", at);
                    return nullptr;
                };
            }
        } else {
            uint32_t range = subexpr->type->dim[0];
            uint32_t stride = subexpr->type->getStride();
            if ( index->rtti_isConstant() ) {
                // if its constant index, like a[3]..., we try to let node bellow simulate
                auto idxCE = static_pointer_cast<ExprConst>(index);
                uint32_t idxC = cast<uint32_t>::to(idxCE->value);
                if ( idxC >= range ) {
                    context.thisProgram->error("index out of range", "", "",
                        at, CompilationError::index_out_of_range);
                    return nullptr;
                }
                auto tnode = subexpr->trySimulate(context, extraOffset + idxC*stride, r2vType);
                if ( tnode ) {
                    return tnode;
                }
            }
            // regular scenario
            auto prv = subexpr->simulate(context);
            auto pidx = index->simulate(context);
            if ( r2vType->baseType!=Type::none ) {
                return context.code->makeValueNode<SimNode_AtR2V>(r2vType->baseType, at, prv, pidx, stride, extraOffset, range);
            } else {
                return context.code->makeNode<SimNode_At>(at, prv, pidx, stride, extraOffset, range);
            }
        }
    }

    SimNode * ExprAt::simulate (Context & context) const {
        if ( subexpr->type->isVectorType() ) {
            auto prv = subexpr->simulate(context);
            auto pidx = index->simulate(context);
            uint32_t range = subexpr->type->getVectorDim();
            uint32_t stride = type->getSizeOf();
            if ( subexpr->type->ref ) {
                auto res = context.code->makeNode<SimNode_At>(at, prv, pidx, stride, 0, range);
                if ( r2v ) {
                    return ExprRef2Value::GetR2V(context, at, type, res);
                } else {
                    return res;
                }
            } else {
                switch ( type->baseType ) {
                    case tInt:      return context.code->makeNode<SimNode_AtVector<int32_t>>(at, prv, pidx, range);
                    case tUInt:
                    case tBitfield:
                                    return context.code->makeNode<SimNode_AtVector<uint32_t>>(at, prv, pidx, range);
                    case tFloat:    return context.code->makeNode<SimNode_AtVector<float>>(at, prv, pidx, range);
                    default:
                        DAS_ASSERTF(0, "we should not even be here. infer type should have failed on unsupported_vector[blah]");
                        context.thisProgram->error("internal compilation error, generating vector at for unsupported vector type.", "", "", at);
                        return nullptr;
                }
            }
        } else if ( subexpr->type->isGoodTableType() ) {
            auto prv = subexpr->simulate(context);
            auto pidx = index->simulate(context);
            uint32_t valueTypeSize = subexpr->type->secondType->getSizeOf();
            Type keyType;
            if ( subexpr->type->firstType->isWorkhorseType() ) {
                keyType = subexpr->type->firstType->baseType;
            } else {
                auto keyValueType = subexpr->type->firstType->annotation->makeValueType();
                keyType = keyValueType->baseType;
                pidx = context.code->makeNode<SimNode_CastToWorkhorse>(at, pidx);
            }
            auto res = context.code->makeValueNode<SimNode_TableIndex>(keyType, at, prv, pidx, valueTypeSize, 0);
            if ( r2v ) {
                return ExprRef2Value::GetR2V(context, at, type, res);
            } else {
                return res;
            }
        } else {
            if ( r2v ) {
                return trySimulate(context, 0, type);
            } else {
                return trySimulate(context, 0, make_smart<TypeDecl>(Type::none));
            }
        }
    }

    SimNode * ExprSafeAt::trySimulate (Context &, uint32_t, const TypeDeclPtr &) const {
        return nullptr;
    }

    SimNode * ExprSafeAt::simulate (Context & context) const {
        if ( subexpr->type->isPointer() ) {
            const auto & seT = subexpr->type->firstType;
            if ( seT->isGoodArrayType() ) {
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                uint32_t stride = seT->firstType->getSizeOf();
                return context.code->makeNode<SimNode_SafeArrayAt>(at, prv, pidx, stride, 0);
            } else if ( seT->isGoodTableType() ) {
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                uint32_t valueTypeSize = seT->secondType->getSizeOf();
                Type valueType;
                if ( seT->firstType->isWorkhorseType() ) {
                    valueType = seT->firstType->baseType;
                } else {
                    auto valueT = seT->firstType->annotation->makeValueType();
                    valueType = valueT->baseType;
                    pidx = context.code->makeNode<SimNode_CastToWorkhorse>(at, pidx);
                }
                return context.code->makeValueNode<SimNode_SafeTableIndex>(valueType, at, prv, pidx, valueTypeSize, 0);
            } else if ( seT->dim.size() ) {
                uint32_t range = seT->dim[0];
                uint32_t stride = seT->getStride();
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                return context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range);
            } else if ( seT->isVectorType() ) {
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                uint32_t range = seT->getVectorDim();
                uint32_t stride = type->getSizeOf();
                return context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range);
            } else {
                DAS_VERIFY(0 && "TODO: safe-at not implemented");
            }
        } else {
            const auto & seT = subexpr->type;
            if ( seT->isGoodArrayType() ) {
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                uint32_t stride = seT->firstType->getSizeOf();
                return context.code->makeNode<SimNode_SafeArrayAt>(at, prv, pidx, stride, 0);
            } else if ( subexpr->type->isGoodTableType() ) {
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                uint32_t valueTypeSize = seT->secondType->getSizeOf();
                Type valueType;
                if ( seT->firstType->isWorkhorseType() ) {
                    valueType = seT->firstType->baseType;
                } else {
                    auto valueT = seT->firstType->annotation->makeValueType();
                    valueType = valueT->baseType;
                    pidx = context.code->makeNode<SimNode_CastToWorkhorse>(at, pidx);
                }
                return context.code->makeValueNode<SimNode_SafeTableIndex>(valueType, at, prv, pidx, valueTypeSize, 0);
            } else if ( seT->dim.size() ) {
                uint32_t range = seT->dim[0];
                uint32_t stride = seT->getStride();
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                return context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range);
            } else if ( seT->isVectorType() && seT->ref ) {
                auto prv = subexpr->simulate(context);
                auto pidx = index->simulate(context);
                uint32_t range = seT->getVectorDim();
                uint32_t stride = type->getSizeOf();
                return context.code->makeNode<SimNode_SafeAt>(at, prv, pidx, stride, 0, range);
            } else {
                DAS_VERIFY(0 && "TODO: safe-at not implemented");
            }
        }
        return nullptr;
    }

    vector<SimNode *> ExprBlock::collectExpressions ( Context & context,
                                                     const vector<ExpressionPtr> & lis,
                                                     das_map<int32_t,uint32_t> * ofsmap ) const {
        vector<SimNode *> simlist;
        for ( auto & node : lis ) {
            if ( node->rtti_isLet()) {
                auto pLet = static_pointer_cast<ExprLet>(node);
                auto letInit = ExprLet::simulateInit(context, pLet.get());
                simlist.insert(simlist.end(), letInit.begin(), letInit.end());
                continue;
            }
            if ( node->rtti_isLabel() ) {
                if ( ofsmap ) {
                    auto lnode = static_pointer_cast<ExprLabel>(node);
                    (*ofsmap)[lnode->label] = uint32_t(simlist.size());
                }
                continue;
            }
            if ( auto simE = node->simulate(context) ) {
                simlist.push_back(simE);
            }
        }
        return simlist;
    }

    void ExprBlock::simulateFinal ( Context & context, SimNode_Final * block ) const {
        vector<SimNode *> simFList = collectExpressions(context, finalList);
        block->totalFinal = int(simFList.size());
        if ( block->totalFinal ) {
            block->finalList = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->totalFinal);
            for ( uint32_t i=0, is=block->totalFinal; i!=is; ++i )
                block->finalList[i] = simFList[i];
        }
    }

    void ExprBlock::simulateBlock ( Context & context, SimNode_Block * block ) const {
        das_map<int32_t,uint32_t> ofsmap;
        vector<SimNode *> simlist = collectExpressions(context, list, &ofsmap);
        block->total = int(simlist.size());
        if ( block->total ) {
            block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
            for ( uint32_t i=0, is=block->total; i!=is; ++i )
                block->list[i] = simlist[i];
        }
        simulateLabels(context, block, ofsmap);
    }

    void ExprBlock::simulateLabels ( Context & context, SimNode_Block * block, const das_map<int32_t,uint32_t> & ofsmap ) const {
        if ( maxLabelIndex!=-1 ) {
            block->totalLabels = maxLabelIndex + 1;
            block->labels = (uint32_t *) context.code->allocate(block->totalLabels * sizeof(uint32_t));
            for ( uint32_t i=0, is=block->totalLabels; i!=is; ++i ) {
                block->labels[i] = -1U;
            }
            for ( auto & it : ofsmap ) {
                block->labels[it.first] = it.second;
            }
        }
    }

    SimNode * ExprBlock::simulate (Context & context) const {
        das_map<int32_t,uint32_t> ofsmap;
        vector<SimNode *> simlist = collectExpressions(context, list, &ofsmap);
        // wow, such empty
        if ( finalList.size()==0 && simlist.size()==0 && !annotationDataSid ) {
            return context.code->makeNode<SimNode_NOP>(at);
        }
        // we memzero block's stack memory, if there is a finally section
        // bad scenario we fight is ( in scope X ; return ; in scope Y )
        if ( finalList.size() ) {
            uint32_t blockDataSize = stackVarBottom - stackVarTop;
            if ( blockDataSize ) {
                for ( const auto & svr : stackCleanVars ) {
                    SimNode * fakeVar = context.code->makeNode<SimNode_GetLocal>(at, svr.first);
                    SimNode * memZ = context.code->makeNode<SimNode_MemZero>(at, fakeVar, svr.second );
                    simlist.insert( simlist.begin(), memZ );
                }
            }
        }
        // TODO: what if list size is 0?
        if ( simlist.size()!=1 || isClosure || finalList.size() ) {
            SimNode_Block * block;
            if ( isClosure ) {
                bool needResult = type!=nullptr && type->baseType!=Type::tVoid;
                bool C0 = !needResult && simlist.size()==1 && finalList.size()==0;
#if DAS_DEBUGGER
                if ( context.thisProgram->getDebugger() ) {
                    block = context.code->makeNode<SimNodeDebug_ClosureBlock>(at, needResult, C0, annotationData);
                } else
#endif
                {
                    block = context.code->makeNode<SimNode_ClosureBlock>(at, needResult, C0, annotationData);
                }
            } else {
                if ( maxLabelIndex!=-1 ) {
#if DAS_DEBUGGER
                    if ( context.thisProgram->getDebugger() ) {
                        block = context.code->makeNode<SimNodeDebug_BlockWithLabels>(at);
                    } else
#endif
                    {
                        block = context.code->makeNode<SimNode_BlockWithLabels>(at);
                    }
                    simulateLabels(context, block, ofsmap);
                } else {
                    if ( finalList.size()==0 ) {
#if DAS_DEBUGGER
                        if ( context.thisProgram->getDebugger() ) {
                            block = context.code->makeNode<SimNodeDebug_BlockNF>(at);
                        } else
#endif
                        {
                            auto lsize = int(simlist.size());
                            block = (SimNode_Block *) context.code->makeNodeUnrollAny<SimNode_BlockNFT>(lsize, at);
                        }
                    } else {
#if DAS_DEBUGGER
                        if ( context.thisProgram->getDebugger() ) {
                            block = context.code->makeNode<SimNodeDebug_Block>(at);
                        } else
#endif
                        {
                            block = context.code->makeNode<SimNode_Block>(at);
                        }
                    }
                }
            }
            block->annotationDataSid = annotationDataSid;
            block->total = int(simlist.size());
            if ( block->total ) {
                block->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*block->total);
                for ( uint32_t i=0, is=block->total; i!=is; ++i )
                    block->list[i] = simlist[i];
            }
            if ( !inTheLoop ) {
                simulateFinal(context, block);
            }
            return block;
        } else {
            return simlist[0];
        }
    }

    SimNode * ExprSwizzle::trySimulate (Context & context, uint32_t extraOffset, const TypeDeclPtr & r2vType ) const {
        if ( !value->type->ref ) {
            return nullptr;
        }
        int offset = value->type->getVectorFieldOffset(fields[0]);
        if  (offset==-1 ) {
            context.thisProgram->error("internal compilation error, swizzle field offset of unsupported type", "", "", at);
            return nullptr;
        }
        if ( auto chain = value->trySimulate(context, uint32_t(offset) + extraOffset, r2vType) ) {
            return chain;
        }
        auto simV = value->simulate(context);
        if ( r2vType->baseType!=Type::none ) {
            return context.code->makeValueNode<SimNode_FieldDerefR2V>(r2vType->baseType,at,simV,uint32_t(offset) + extraOffset);
        } else {
            return context.code->makeNode<SimNode_FieldDeref>(at,simV,uint32_t(offset) + extraOffset);
        }
    }

    SimNode * ExprSwizzle::simulate (Context & context) const {
        if ( !type->ref ) {
            bool seq = TypeDecl::isSequencialMask(fields);
            if (seq && value->type->ref) {
                return trySimulate(context, 0, type);
            } else {
                auto fsz = fields.size();
                uint8_t fs[4];
                fs[0] = fields[0];
                fs[1] = fsz >= 2 ? fields[1] : fields[0];
                fs[2] = fsz >= 3 ? fields[2] : fields[0];
                fs[3] = fsz >= 4 ? fields[3] : fields[0];
                auto simV = value->simulate(context);
                if ( type->baseType==Type::tRange64 || type->baseType==Type::tURange64 ) {
                    return context.code->makeNode<SimNode_Swizzle64>(at, simV, fs);
                } else {
                    return context.code->makeNode<SimNode_Swizzle>(at, simV, fs);
                }
            }
        } else {
            return trySimulate(context, 0, r2v ? type : make_smart<TypeDecl>(Type::none));
        }
    }

    SimNode * ExprField::simulate (Context & context) const {
        if ( value->type->isBitfield() ) {
            auto simV = value->simulate(context);
            uint32_t mask = 1u << fieldIndex;
            return context.code->makeNode<SimNode_GetBitField>(at, simV, mask);
        } else {
            return trySimulate(context, 0, r2v ? type : make_smart<TypeDecl>(Type::none));
        }
    }

    SimNode * ExprField::trySimulate (Context & context, uint32_t extraOffset, const TypeDeclPtr & r2vType ) const {
        if ( value->type->isBitfield() ) {
            return nullptr;
        }
        int fieldOffset = -1;
        if ( !field && fieldIndex==-1 ) {
            fieldOffset = (int) annotation->getFieldOffset(name);
        } else if ( fieldIndex != - 1 ) {
            if ( value->type->isPointer() ) {
                if ( value->type->firstType->isVariant() ) {
                    fieldOffset = value->type->firstType->getVariantFieldOffset(fieldIndex);
                } else {
                    fieldOffset = value->type->firstType->getTupleFieldOffset(fieldIndex);
                }
            } else {
                if ( value->type->isVariant() ) {
                    fieldOffset = value->type->getVariantFieldOffset(fieldIndex);
                } else {
                    fieldOffset = value->type->getTupleFieldOffset(fieldIndex);
                }
            }
        } else {
            DAS_ASSERTF(field, "field can't be null");
            if (!field) return nullptr;
            fieldOffset = field->offset;
        }
        DAS_ASSERTF(fieldOffset>=0,"field offset is somehow not there");
        if (value->type->isPointer()) {
            if ( unsafeDeref ) {
                auto simV = value->simulate(context);
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_FieldDerefR2V>(r2vType->baseType, at, simV, fieldOffset + extraOffset);
                }
                else {
                    return context.code->makeNode<SimNode_FieldDeref>(at, simV, fieldOffset + extraOffset);
                }
            } else {
                auto simV = value->simulate(context);
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_PtrFieldDerefR2V>(r2vType->baseType, at, simV, fieldOffset + extraOffset);
                }
                else {
                    return context.code->makeNode<SimNode_PtrFieldDeref>(at, simV, fieldOffset + extraOffset);
                }
            }
        } else {
            if ( auto chain = value->trySimulate(context, extraOffset + fieldOffset, r2vType) ) {
                return chain;
            }
            auto simV = value->simulate(context);
            if ( r2vType->baseType!=Type::none ) {
                return context.code->makeValueNode<SimNode_FieldDerefR2V>(r2vType->baseType, at, simV, extraOffset + fieldOffset);
            } else {
                return context.code->makeNode<SimNode_FieldDeref>(at, simV, extraOffset + fieldOffset);
            }
        }
    }

    SimNode * ExprIsVariant::simulate(Context & context) const {
        DAS_ASSERT(fieldIndex != -1);
        return context.code->makeNode<SimNode_IsVariant>(at, value->simulate(context), fieldIndex);
    }

    SimNode * ExprAsVariant::simulate (Context & context) const {
        int fieldOffset = value->type->getVariantFieldOffset(fieldIndex);
        auto simV = value->simulate(context);
        if ( r2v ) {
            return context.code->makeValueNode<SimNode_VariantFieldDerefR2V>(type->baseType, at, simV, fieldOffset, fieldIndex);
        } else {
            return context.code->makeNode<SimNode_VariantFieldDeref>(at, simV, fieldOffset, fieldIndex);
        }
    }

    SimNode * ExprSafeAsVariant::simulate (Context & context) const {
        int fieldOffset = value->type->isPointer() ?
            value->type->firstType->getVariantFieldOffset(fieldIndex) :
            value->type->getVariantFieldOffset(fieldIndex);
        auto simV = value->simulate(context);
        if ( skipQQ ) {
            return context.code->makeNode<SimNode_SafeVariantFieldDerefPtr>(at,simV,fieldOffset, fieldIndex);
        } else {
            return context.code->makeNode<SimNode_SafeVariantFieldDeref>(at,simV,fieldOffset, fieldIndex);
        }
    }

    SimNode * ExprSafeField::trySimulate(Context &, uint32_t, const TypeDeclPtr &) const {
        return nullptr;
    }

    SimNode * ExprSafeField::simulate (Context & context) const {
        int fieldOffset = -1;
        if ( !annotation ) {
            if ( fieldIndex != - 1 ) {
                if ( value->type->firstType->isVariant() ) {
                    fieldOffset = value->type->firstType->getVariantFieldOffset(fieldIndex);
                } else {
                    fieldOffset = value->type->firstType->getTupleFieldOffset(fieldIndex);
                }
            } else {
                fieldOffset = field->offset;
            }
            DAS_ASSERTF(fieldOffset>=0,"field offset is somehow not there");
        }
        if ( annotation ) fieldOffset = (int) annotation->getFieldOffset(name);
        if ( skipQQ ) {
            return context.code->makeNode<SimNode_SafeFieldDerefPtr>(at,value->simulate(context),fieldOffset);
        } else {
            return context.code->makeNode<SimNode_SafeFieldDeref>(at,value->simulate(context),fieldOffset);
        }
    }

    SimNode * ExprStringBuilder::simulate (Context & context) const {
        SimNode_StringBuilder * pSB = context.code->makeNode<SimNode_StringBuilder>(isTempString, at);
        if ( int nArg = (int) elements.size() ) {
            pSB->arguments = (SimNode **) context.code->allocate(nArg * sizeof(SimNode *));
            pSB->types = (TypeInfo **) context.code->allocate(nArg * sizeof(TypeInfo *));
            pSB->nArguments = nArg;
            for ( int a=0; a!=nArg; ++a ) {
                pSB->arguments[a] = elements[a]->simulate(context);
                pSB->types[a] = context.thisHelper->makeTypeInfo(nullptr, elements[a]->type);
            }
        } else {
            pSB->arguments = nullptr;
            pSB->types = nullptr;
            pSB->nArguments = 0;
        }
        return pSB;
    }

    SimNode * ExprVar::trySimulate (Context & context, uint32_t extraOffset, const TypeDeclPtr & r2vType ) const {
        if ( block ) {
        } else if ( local ) {
            if ( variable->type->ref ) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetLocalRefOffR2V>(r2vType->baseType, at,
                                                    variable->stackTop, extraOffset + variable->extraLocalOffset);
                } else {
                    return context.code->makeNode<SimNode_GetLocalRefOff>(at,
                                                    variable->stackTop, extraOffset + variable->extraLocalOffset);
                }
            } else if ( variable->aliasCMRES ) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetCMResOfsR2V>(r2vType->baseType, at,extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetCMResOfs>(at, extraOffset);
                }
            } else {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetLocalR2V>(r2vType->baseType, at,
                                                                            variable->stackTop + extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetLocal>(at, variable->stackTop + extraOffset);
                }
            }
        } else if ( argument ) {
            if ( variable->type->isPointer() && variable->type->isRef() ) {
                return nullptr;
            } else if ( variable->type->isPointer() ) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetArgumentRefOffR2V>(r2vType->baseType, at, argumentIndex, extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetArgumentRefOff>(at, argumentIndex, extraOffset);
                }
            } else if (variable->type->isRef()) {
                if ( r2vType->baseType!=Type::none ) {
                    return context.code->makeValueNode<SimNode_GetArgumentRefOffR2V>(r2vType->baseType, at, argumentIndex, extraOffset);
                } else {
                    return context.code->makeNode<SimNode_GetArgumentRefOff>(at, argumentIndex, extraOffset);
                }
            }
        } else { // global

        }
        return nullptr;
    }

    SimNode * ExprVar::simulate (Context & context) const {
        if ( block ) {
            auto blk = pBlock;
            if (variable->type->isRef()) {
                if (r2v && !type->isRefType()) {
                    if ( thisBlock ) {
                        return context.code->makeValueNode<SimNode_GetThisBlockArgumentR2V>(type->baseType, at, argumentIndex);
                    } else {
                        return context.code->makeValueNode<SimNode_GetBlockArgumentR2V>(type->baseType, at, argumentIndex, blk->stackTop);
                    }
                } else {
                    if ( thisBlock ) {
                        return context.code->makeNode<SimNode_GetThisBlockArgument>(at, argumentIndex);
                    } else {
                        return context.code->makeNode<SimNode_GetBlockArgument>(at, argumentIndex, blk->stackTop);
                    }
                }
            } else {
                if (r2v && !type->isRefType()) {
                    if ( thisBlock ) {
                        return context.code->makeNode<SimNode_GetThisBlockArgument>(at, argumentIndex);
                    } else {
                        return context.code->makeNode<SimNode_GetBlockArgument>(at, argumentIndex, blk->stackTop);
                    }
                }
                else {
                    if ( thisBlock ) {
                        return context.code->makeNode<SimNode_GetThisBlockArgumentRef>(at, argumentIndex);
                    } else {
                        return context.code->makeNode<SimNode_GetBlockArgumentRef>(at, argumentIndex, blk->stackTop);
                    }
                }
            }
        } else if ( local ) {
            if ( r2v ) {
                return trySimulate(context, variable->extraLocalOffset, type);
            } else {
                return trySimulate(context, variable->extraLocalOffset, make_smart<TypeDecl>(Type::none));
            }
        } else if ( argument) {
            if (variable->type->isRef()) {
                if (r2v && !type->isRefType()) {
                    return context.code->makeValueNode<SimNode_GetArgumentR2V>(type->baseType, at, argumentIndex);
                } else {
                    return context.code->makeNode<SimNode_GetArgument>(at, argumentIndex);
                }
            } else {
                if (r2v && !type->isRefType()) {
                    return context.code->makeNode<SimNode_GetArgument>(at, argumentIndex);
                }
                else {
                    return context.code->makeNode<SimNode_GetArgumentRef>(at, argumentIndex);
                }
            }
        } else {
            DAS_ASSERT(variable->index >= 0 && "using variable which is not used. how?");
            uint64_t mnh = variable->getMangledNameHash();
            if ( !variable->module->isSolidContext ) {
                if ( variable->global_shared ) {
                    if ( r2v ) {
                        return context.code->makeValueNode<SimNode_GetSharedMnhR2V>(type->baseType, at, variable->stackTop, mnh);
                    } else {
                        return context.code->makeNode<SimNode_GetSharedMnh>(at, variable->stackTop, mnh);
                    }
                } else {
                    if ( r2v ) {
                        return context.code->makeValueNode<SimNode_GetGlobalMnhR2V>(type->baseType, at, variable->stackTop, mnh);
                    } else {
                        return context.code->makeNode<SimNode_GetGlobalMnh>(at, variable->stackTop, mnh);
                    }
                }
            } else {
                if ( variable->global_shared ) {
                    if ( r2v ) {
                        return context.code->makeValueNode<SimNode_GetSharedR2V>(type->baseType, at, variable->stackTop, mnh);
                    } else {
                        return context.code->makeNode<SimNode_GetShared>(at, variable->stackTop, mnh);
                    }
                } else {
                    if ( r2v ) {
                        return context.code->makeValueNode<SimNode_GetGlobalR2V>(type->baseType, at, variable->stackTop, mnh);
                    } else {
                        return context.code->makeNode<SimNode_GetGlobal>(at, variable->stackTop, mnh);
                    }
                }
            }
        }
    }

    SimNode * ExprOp1::simulate (Context & context) const {
        vector<ExpressionPtr> sarguments = { subexpr };
        if ( func->builtIn && !func->callBased ) {
            auto pSimOp1 = static_cast<SimNode_Op1 *>(func->makeSimNode(context,sarguments));
            pSimOp1->debugInfo = at;
            pSimOp1->x = subexpr->simulate(context);
            return pSimOp1;
        } else {
            auto pCall = static_cast<SimNode_CallBase *>(func->makeSimNode(context,sarguments));
            pCall->debugInfo = at;
            pCall->fnPtr = context.getFunction(func->index);
            pCall->arguments = (SimNode **) context.code->allocate(1 * sizeof(SimNode *));
            pCall->nArguments = 1;
            pCall->arguments[0] = subexpr->simulate(context);
            pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at,stackTop);
            return pCall;
        }
    }

    // we flatten or(or(or(a,b),c),d) into a single vector {a,b,c,d}
    void flattenOrSequence ( const ExprOp2 * op, vector<ExpressionPtr> & ret ) {
        if ( op->left->rtti_isOp2() && static_pointer_cast<ExprOp2>(op->left)->op==op->op ) {
            flattenOrSequence(static_pointer_cast<ExprOp2>(op->left).get(), ret);
        } else {
            ret.push_back(op->left);
        }
        if ( op->right->rtti_isOp2() && static_pointer_cast<ExprOp2>(op->right)->op==op->op ) {
            flattenOrSequence(static_pointer_cast<ExprOp2>(op->right).get(), ret);
        } else {
            ret.push_back(op->right);
        }
    }

    SimNode * ExprOp2::simulate (Context & context) const {
        // special case for logical operator sequence
        if ( func->builtIn && func->module->name=="$" && func->arguments[0]->type->isBool() ) {
            if ( func->name=="||" || func->name=="&&" ) {
                vector<ExpressionPtr> flat;
                flattenOrSequence(this, flat);
                if ( flat.size()>2 && flat.size()<=7 ) {
                    SimNode_CallBase * pSimNode;
                    if ( func->name=="||" ) {
                        pSimNode = (SimNode_OrAny *) context.code->makeNodeUnrollAny<SimNode_OrT>((int32_t)flat.size(), at);
                    } else if ( func->name=="&&" ) {
                        pSimNode = (SimNode_AndAny *) context.code->makeNodeUnrollAny<SimNode_AndT>((int32_t)flat.size(), at);
                    } else {
                        DAS_ASSERTF(0, "unknown logical operator");
                        return nullptr;
                    }
                    pSimNode->nArguments = (uint32_t) flat.size();
                    pSimNode->arguments = (SimNode **) context.code->allocate((uint32_t)(flat.size() * sizeof(SimNode *)));
                    for ( uint32_t i=0; i!=uint32_t(flat.size()); ++i ) {
                        pSimNode->arguments[i] = flat[i]->simulate(context);
                    }
                    return pSimNode;
                }
            }
        }
        vector<ExpressionPtr> sarguments = { left, right };
        if ( func->builtIn && !func->callBased ) {
            auto pSimOp2 = static_cast<SimNode_Op2 *>(func->makeSimNode(context,sarguments));
            pSimOp2->debugInfo = at;
            pSimOp2->l = left->simulate(context);
            pSimOp2->r = right->simulate(context);
            return pSimOp2;
        } else {
            auto pCall = static_cast<SimNode_CallBase *>(func->makeSimNode(context,sarguments));
            pCall->debugInfo = at;
            pCall->fnPtr = context.getFunction(func->index);
            pCall->arguments = (SimNode **) context.code->allocate(2 * sizeof(SimNode *));
            pCall->nArguments = 2;
            pCall->arguments[0] = left->simulate(context);
            pCall->arguments[1] = right->simulate(context);
            pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at,stackTop);
            return pCall;
        }
    }

    SimNode * ExprOp3::simulate (Context & context) const {
        return context.code->makeNode<SimNode_IfThenElse>(at,
                                                    subexpr->simulate(context),
                                                    left->simulate(context),
                                                    right->simulate(context));
    }

    SimNode * ExprTag::simulate (Context & context) const {
        context.thisProgram->error("internal compilation error, trying to simulate a tag", "", "", at);
        return nullptr;
    }

    SimNode * ExprMove::simulate (Context & context) const {
        if ( takeOverRightStack ) {
            auto sl = left->simulate(context);
            auto sr = right->simulate(context);
            return context.code->makeNode<SimNode_SetLocalRefAndEval>(at, sl, sr, stackTop);
        } else {
            auto retN = makeMove(at,context,left,right);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate move", "", "", at);
            }
            return retN;
        }
    }

    SimNode * ExprClone::simulate (Context & context) const {
        SimNode * retN = nullptr;
        if ( left->type->isHandle() ) {
            auto lN = left->simulate(context);
            auto rN = right->simulate(context);
            auto ta = ((TypeAnnotation *)(right->type->annotation));
            if ( !ta->isRefType() && right->type->isRef() ) {
                rN = ta->simulateRef2Value(context, at, rN);
            }
            retN = left->type->annotation->simulateClone(context, at, lN, rN);
        } else if ( left->type->canCopy() ) {
            retN = makeCopy(at, context, left, right );
        } else {
            retN = nullptr;
        }
        if ( !retN ) {
            context.thisProgram->error("internal compilation error, can't generate clone", "", "", at);
        }
        return retN;
    }


    SimNode * ExprCopy::simulate (Context & context) const {
        if ( takeOverRightStack ) {
            auto sl = left->simulate(context);
            auto sr = right->simulate(context);
            return context.code->makeNode<SimNode_SetLocalRefAndEval>(at, sl, sr, stackTop);
        } else {
            auto retN = makeCopy(at, context, left, right);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate copy", "", "", at);
            }
            return retN;
        }
    }

    SimNode * ExprTryCatch::simulate (Context & context) const {
#if DAS_DEBUGGER
        if ( context.thisProgram->getDebugger() ) {
            return context.code->makeNode<SimNodeDebug_TryCatch>(at,
                                                    try_block->simulate(context),
                                                    catch_block->simulate(context));
        } else
#endif
        {
            return context.code->makeNode<SimNode_TryCatch>(at,
                                                    try_block->simulate(context),
                                                    catch_block->simulate(context));
        }
    }

    SimNode * ExprReturn::simulate (Context & context) const {
        // return string is its own thing
        if (subexpr && subexpr->type && subexpr->rtti_isConstant()) {
            if (subexpr->type->isSimpleType(Type::tString)) {
                auto cVal = static_pointer_cast<ExprConstString>(subexpr);
                char * str = context.constStringHeap->impl_allocateString(cVal->text);
                return context.code->makeNode<SimNode_ReturnConstString>(at, str);
            }
        }
        // now, lets do the standard everything
        bool skipIt = false;
        if ( subexpr && subexpr->rtti_isMakeLocal() ) {
            if ( static_pointer_cast<ExprMakeLocal>(subexpr)->useCMRES ) {
                skipIt = true;
            }
        }
        SimNode * simSubE = (subexpr && !skipIt) ? subexpr->simulate(context) : nullptr;
        if (!subexpr) {
            return context.code->makeNode<SimNode_ReturnNothing>(at);
        } else if ( subexpr->rtti_isConstant() ) {
            auto cVal = static_pointer_cast<ExprConst>(subexpr);
            return context.code->makeNode<SimNode_ReturnConst>(at, cVal->value);
        }
        if ( returnReference ) {
            if ( returnInBlock ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnReferenceFromBlock>(at, simSubE);
            } else {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnReference>(at, simSubE);
            }
        } else if ( returnInBlock ) {
            if ( returnCallCMRES ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                SimNode_CallBase * simRet = (SimNode_CallBase *) simSubE;
                simRet->cmresEval = context.code->makeNode<SimNode_GetBlockCMResOfs>(at,0,stackTop);
                return context.code->makeNode<SimNode_Return>(at, simSubE);
            } else if ( takeOverRightStack ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnRefAndEvalFromBlock>(at,
                            simSubE, refStackTop, stackTop);
            } else if ( block->copyOnReturn  ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnAndCopyFromBlock>(at,
                            simSubE, subexpr->type->getSizeOf(), stackTop);
            } else if ( block->moveOnReturn ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnAndMoveFromBlock>(at,
                    simSubE, subexpr->type->getSizeOf(), stackTop);
            }
        } else if ( subexpr ) {
            if ( returnCallCMRES ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                SimNode_CallBase * simRet = (SimNode_CallBase *) simSubE;
                simRet->cmresEval = context.code->makeNode<SimNode_GetCMResOfs>(at,0);
                return context.code->makeNode<SimNode_Return>(at, simSubE);
            } else if ( returnCMRES ) {
                // ReturnLocalCMRes
                if ( subexpr->rtti_isMakeLocal() ) {
                    auto mkl = static_pointer_cast<ExprMakeLocal>(subexpr);
                    if ( mkl->useCMRES ) {
                        SimNode_Block * blockT = context.code->makeNode<SimNode_ReturnLocalCMRes>(at);
                        auto simlist = mkl->simulateLocal(context);
                        blockT->total = int(simlist.size());
                        blockT->list = (SimNode **) context.code->allocate(sizeof(SimNode *)*blockT->total);
                        for ( uint32_t i=0, is=blockT->total; i!=is; ++i )
                            blockT->list[i] = simlist[i];
                        return blockT;
                    }
                }
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_Return>(at, simSubE);
            } else if ( takeOverRightStack ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnRefAndEval>(at, simSubE, refStackTop);
            } else if ( returnFunc && returnFunc->copyOnReturn ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnAndCopy>(at, simSubE, subexpr->type->getSizeOf());
            } else if ( returnFunc && returnFunc->moveOnReturn ) {
                DAS_VERIFYF(simSubE, "internal error. can't be zero");
                return context.code->makeNode<SimNode_ReturnAndMove>(at, simSubE, subexpr->type->getSizeOf());
            }
        }
        DAS_VERIFYF(simSubE, "internal error. can't be zero");
        if ( moveSemantics ) {
            if ( subexpr->type->isRef() ) {
                if ( subexpr->type->isHandle() && !subexpr->type->annotation->isRefType() ) {
                    auto r2v = subexpr->type->annotation->simulateRef2Value(context, at, simSubE);
                    return context.code->makeNode<SimNode_Return>(at, r2v);
                } else {
                    return context.code->makeValueNode<SimNode_ReturnAndMoveR2V>(subexpr->type->baseType, at, simSubE);
                }
            } else {
                return context.code->makeNode<SimNode_Return>(at, simSubE);
            }
        } else {
            return context.code->makeNode<SimNode_Return>(at, simSubE);
        }
    }

    SimNode * ExprBreak::simulate (Context & context) const {
        return context.code->makeNode<SimNode_Break>(at);
    }

    SimNode * ExprContinue::simulate (Context & context) const {
        return context.code->makeNode<SimNode_Continue>(at);
    }

    SimNode * ExprIfThenElse::simulate (Context & context) const {
        ExpressionPtr zeroCond;
        bool condIfZero = false;
        bool match0 = matchEquNequZero(cond, zeroCond, condIfZero);
#if DAS_DEBUGGER
        if ( context.thisProgram->getDebugger() ) {
            if ( match0 && zeroCond->type->isWorkhorseType() ) {
                if ( condIfZero ) {
                    if ( if_false ) {
                        return context.code->makeNumericValueNode<SimNodeDebug_IfZeroThenElse>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context),
                                                if_false->simulate(context));

                    } else {
                        return context.code->makeNumericValueNode<SimNodeDebug_IfZeroThen>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context));
                    }
                } else {
                    if ( if_false ) {
                        return context.code->makeNumericValueNode<SimNodeDebug_IfNotZeroThenElse>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context),
                                                if_false->simulate(context));
                    } else {
                        return context.code->makeNumericValueNode<SimNodeDebug_IfNotZeroThen>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context));
                    }
                }
            } else {
                // good old if
                if ( if_false ) {
                    return context.code->makeNode<SimNodeDebug_IfThenElse>(at, cond->simulate(context),
                                        if_true->simulate(context), if_false->simulate(context));
                } else {
                    return context.code->makeNode<SimNodeDebug_IfThen>(at, cond->simulate(context),
                                        if_true->simulate(context));
                }
            }
        } else
#endif
        {
            if ( match0 && zeroCond->type->isWorkhorseType() ) {
                if ( condIfZero ) {
                    if ( if_false ) {
                        return context.code->makeNumericValueNode<SimNode_IfZeroThenElse>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context),
                                                if_false->simulate(context));

                    } else {
                        return context.code->makeNumericValueNode<SimNode_IfZeroThen>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context));
                    }
                } else {
                    if ( if_false ) {
                        return context.code->makeNumericValueNode<SimNode_IfNotZeroThenElse>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context),
                                                if_false->simulate(context));
                    } else {
                        return context.code->makeNumericValueNode<SimNode_IfNotZeroThen>(zeroCond->type->baseType,
                                        at, zeroCond->simulate(context), if_true->simulate(context));
                    }
                }
            } else {
                // good old if
                if ( if_false ) {
                    return context.code->makeNode<SimNode_IfThenElse>(at, cond->simulate(context),
                                        if_true->simulate(context), if_false->simulate(context));
                } else {
                    return context.code->makeNode<SimNode_IfThen>(at, cond->simulate(context),
                                        if_true->simulate(context));
                }
            }
        }
    }

    SimNode * ExprWith::simulate (Context & context) const {
        return body->simulate(context);
    }

    SimNode * ExprAssume::simulate (Context &) const {
        return nullptr;
    }

    void ExprWhile::simulateFinal ( Context & context, const ExpressionPtr & bod, SimNode_Block * blk ) {
        if ( bod->rtti_isBlock() ) {
            auto pBlock = static_pointer_cast<ExprBlock>(bod);
            pBlock->simulateBlock(context, blk);
            pBlock->simulateFinal(context, blk);
        } else {
            context.thisProgram->error("internal error, expecting block", "", "", bod->at);
        }
    }

    SimNode * ExprWhile::simulate (Context & context) const {
#if DAS_DEBUGGER
        if ( context.thisProgram->getDebugger() ) {
            auto node = context.code->makeNode<SimNodeDebug_While>(at, cond->simulate(context));
            simulateFinal(context, body, node);
            return node;
        } else
#endif
        {
            auto node = context.code->makeNode<SimNode_While>(at, cond->simulate(context));
            simulateFinal(context, body, node);
            return node;
        }
    }

    SimNode * ExprUnsafe::simulate (Context & context) const {
        return body->simulate(context);
    }

    SimNode * ExprFor::simulate (Context & context) const {
        // determine iteration types
        bool nativeIterators = false;
        bool fixedArrays = false;
        bool dynamicArrays = false;
        bool stringChars = false;
        bool rangeBase = false;
        int32_t fixedSize = INT32_MAX;
        for ( auto & src : sources ) {
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
        int  total = int(sources.size());
        int  sourceTypes = int(dynamicArrays) + int(fixedArrays) + int(rangeBase) + int(stringChars);
        bool hybridRange = rangeBase && (total>1);
        if ( (sourceTypes>1) || hybridRange || nativeIterators || stringChars || /* this is how much we can unroll */ total>MAX_FOR_UNROLL ) {
            SimNode_ForWithIteratorBase * result;
#if DAS_DEBUGGER
            if ( context.thisProgram->getDebugger() ) {
                if ( total>MAX_FOR_UNROLL ) {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNode<SimNodeDebug_ForWithIteratorBase>(at);
                } else {
                    result = (SimNode_ForWithIteratorBase *) context.code->makeNodeUnrollNZ_FOR<SimNodeDebug_ForWithIterator>(total, at);
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
                if ( sources[t]->type->isGoodIteratorType() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_Seq2Iter>(
                        sources[t]->at,
                        sources[t]->simulate(context));
                } else if ( sources[t]->type->isGoodArrayType() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_GoodArrayIterator>(
                        sources[t]->at,
                        sources[t]->simulate(context),
                        sources[t]->type->firstType->getSizeOf());
                } else if ( sources[t]->type->isRange() ) {
                    result->source_iterators[t] = context.code->makeRangeNode<SimNode_RangeIterator>(
                        sources[t]->type->baseType, sources[t]->at,sources[t]->simulate(context));
                } else if ( sources[t]->type->isString() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_StringIterator>(
                        sources[t]->at,
                        sources[t]->simulate(context));
                } else if ( sources[t]->type->isHandle() ) {
                    if ( !result ) {
                        context.thisProgram->error("integration error, simulateGetIterator returned null", "", "",
                                                   at, CompilationError::missing_node );
                        return nullptr;
                    } else {
                        result->source_iterators[t] = sources[t]->type->annotation->simulateGetIterator(
                            context,
                            sources[t]->at,
                            sources[t]
                        );
                    }
                } else if ( sources[t]->type->dim.size() ) {
                    result->source_iterators[t] = context.code->makeNode<SimNode_FixedArrayIterator>(
                        sources[t]->at,
                        sources[t]->simulate(context),
                        sources[t]->type->dim[0],
                        sources[t]->type->getStride());
                } else {
                    DAS_ASSERTF(0, "we should not be here. we are doing iterator for on an unsupported type.");
                    context.thisProgram->error("internal compilation error, generating for-with-iterator", "", "", at);
                    return nullptr;
                }
                result->stackTop[t] = iteratorVariables[t]->stackTop;
            }
            ExprWhile::simulateFinal(context, body, result);
            return result;
        } else {
            auto flagsE = body->getEvalFlags();
            bool NF = flagsE == 0;
            SimNode_ForBase * result;
            DAS_ASSERT(body->rtti_isBlock() && "there would be internal error otherwise");
            auto subB = static_pointer_cast<ExprBlock>(body);
            bool loop1 = (subB->list.size() == 1);
#if DAS_DEBUGGER
            if ( context.thisProgram->getDebugger() ) {
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
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRangeNF1>(sources[0]->type->baseType,at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRangeNF>(sources[0]->type->baseType,at);
                        }
                    } else {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRange1>(sources[0]->type->baseType,at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNodeDebug_ForRange>(sources[0]->type->baseType,at);
                        }
                    }
                } else {
                    DAS_ASSERTF(0, "we should not be here yet. logic above assumes optimized for path of some kind.");
                    context.thisProgram->error("internal compilation error, generating for", "", "", at);
                    return nullptr;
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
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRangeNF1>(sources[0]->type->baseType,at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRangeNF>(sources[0]->type->baseType,at);
                        }
                    } else {
                        if (loop1) {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRange1>(sources[0]->type->baseType,at);
                        } else {
                            result = (SimNode_ForBase *)context.code->makeRangeNode<SimNode_ForRange>(sources[0]->type->baseType,at);
                        }
                    }
                } else {
                    DAS_ASSERTF(0, "we should not be here yet. logic above assumes optimized for path of some kind.");
                    context.thisProgram->error("internal compilation error, generating for", "", "", at);
                    return nullptr;
                }
            }
            result->allocateFor(context.code.get(), total);
            for ( int t=0; t!=total; ++t ) {
                result->sources[t] = sources[t]->simulate(context);
                if ( sources[t]->type->isGoodArrayType() ) {
                    result->strides[t] = sources[t]->type->firstType->getSizeOf();
                } else {
                    result->strides[t] = sources[t]->type->getStride();
                }
                result->stackTop[t] = iteratorVariables[t]->stackTop;
            }
            result->size = fixedSize;
            ExprWhile::simulateFinal(context, body, result);
            return result;
        }
    }

    vector<SimNode *> ExprLet::simulateInit(Context & context, const ExprLet * pLet) {
        vector<SimNode *> simlist;
        simlist.reserve(pLet->variables.size());
        for (auto & var : pLet->variables) {
            SimNode * init;
            if (var->init) {
                init = ExprLet::simulateInit(context, var, true);
            } else if (var->aliasCMRES ) {
                int bytes = var->type->getSizeOf();
                if ( bytes <= 32 ) {
                    init = context.code->makeNodeUnrollNZ<SimNode_InitLocalCMResN>(bytes, pLet->at,0);
                } else {
                    init = context.code->makeNode<SimNode_InitLocalCMRes>(pLet->at,0,bytes);
                }
            } else {
                init = context.code->makeNode<SimNode_InitLocal>(pLet->at, var->stackTop, var->type->getSizeOf());
            }
            if (init)
                simlist.push_back(init);
        }
        return simlist;
    }

    SimNode * ExprLet::simulateInit(Context & context, const VariablePtr & var, bool local) {
        SimNode * get;
        if ( local ) {
            if ( var->init && var->init->rtti_isMakeLocal() ) {
                return var->init->simulate(context);
            } else {
                get = context.code->makeNode<SimNode_GetLocal>(var->init->at, var->stackTop);
            }
        } else {
            if ( var->init && var->init->rtti_isMakeLocal() ) {
                return var->init->simulate(context);
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
                                                                 var->init->simulate(context));
        } else if ( var->init_via_move && (var->type->canMove() || var->type->isGoodBlockType()) ) {
            auto varExpr = make_smart<ExprVar>(var->at, var->name);
            varExpr->variable = var;
            varExpr->local = local;
            varExpr->type = make_smart<TypeDecl>(*var->type);
            auto retN = makeMove(var->init->at, context, varExpr, var->init);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate move", "", "", var->at);
            }
            return retN;
        } else if ( !var->init_via_move && (var->type->canCopy() || var->type->isGoodBlockType()) ) {
            auto varExpr = make_smart<ExprVar>(var->at, var->name);
            varExpr->variable = var;
            varExpr->local = local;
            varExpr->type = make_smart<TypeDecl>(*var->type);
            auto retN = makeCopy(var->init->at, context, varExpr, var->init);
            if ( !retN ) {
                context.thisProgram->error("internal compilation error, can't generate copy", "", "", var->at);
            }
            return retN;
        } else if ( var->isCtorInitialized() ) {
            auto varExpr = make_smart<ExprVar>(var->at, var->name);
            varExpr->variable = var;
            varExpr->local = local;
            varExpr->type = make_smart<TypeDecl>(*var->type);
            SimNode_CallBase * retN = nullptr; // it has to be CALL with CMRES
            const auto & rE = var->init;
            if ( rE->rtti_isCall() ) {
                auto cll = static_pointer_cast<ExprCall>(rE);
                if ( cll->allowCmresSkip() ) {
                    retN = (SimNode_CallBase *) rE->simulate(context);
                    retN->cmresEval = varExpr->simulate(context);
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

    SimNode * ExprLet::simulate (Context & context) const {
        auto let = context.code->makeNode<SimNode_Let>(at);
        let->total = (uint32_t) variables.size();
        let->list = (SimNode **) context.code->allocate(let->total * sizeof(SimNode*));
        auto simList = ExprLet::simulateInit(context, this);
        copy(simList.data(), simList.data() + simList.size(), let->list);
        return let;
    }

    SimNode_CallBase * ExprCall::simulateCall (const FunctionPtr & func,
                                               const ExprLooksLikeCall * expr,
                                               Context & context,
                                               SimNode_CallBase * pCall) {
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
                pCall->arguments[a] = expr->arguments[a]->simulate(context);
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

    SimNode * ExprCall::simulate (Context & context) const {
        auto pCall = static_cast<SimNode_CallBase *>(func->makeSimNode(context,arguments));
        simulateCall(func, this, context, pCall);
        if ( !doesNotNeedSp && stackTop ) {
            pCall->cmresEval = context.code->makeNode<SimNode_GetLocal>(at,stackTop);
        }
        return pCall;
    }

    SimNode * ExprNamedCall::simulate (Context &) const {
        DAS_ASSERTF(false, "we should not be here. named call should be promoted to regular call");
        return nullptr;
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

    void Program::updateSemanticHash() {
        thisModule->structures.foreach([&](StructurePtr & ps){
            HashBuilder hb;
            das_set<Structure *> dep;
            das_set<Annotation *> adep;
            ps->ownSemanticHash = ps->getOwnSemanticHash(hb,dep,adep);
        });
   }

    bool Program::simulate ( Context & context, TextWriter & logs, StackAllocator * sharedStack ) {
        auto time0 = ref_time_ticks();
        isSimulating = true;
        context.failed = true;
        astTypeInfo.clear();    // this is to be filled via typeinfo(ast_typedecl and such)
        auto disableInit = options.getBoolOption("no_init", policies.no_init);
        context.thisProgram = this;
        context.breakOnException |= policies.debugger;
        context.persistent = options.getBoolOption("persistent_heap", policies.persistent_heap);
        if ( context.persistent ) {
            context.heap = make_smart<PersistentHeapAllocator>();
            context.stringHeap = make_smart<PersistentStringAllocator>();
        } else {
            context.heap = make_smart<LinearHeapAllocator>();
            context.stringHeap = make_smart<LinearStringAllocator>();
        }
        context.heap->setInitialSize ( options.getIntOption("heap_size_hint", policies.heap_size_hint) );
        context.heap->setLimit ( options.getUInt64Option("heap_size_limit", policies.max_heap_allocated) );
        context.stringHeap->setInitialSize ( options.getIntOption("string_heap_size_hint", policies.string_heap_size_hint) );
        context.stringHeap->setLimit ( options.getUInt64Option("string_heap_size_limit", policies.max_string_heap_allocated) );
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
        context.globals = (char *) das_aligned_alloc16(context.globalsSize);
        context.shared = (char *) das_aligned_alloc16(context.sharedSize);
        if ( context.globalsSize && !context.globals ) {
            error("Failed to allocate memory for global variables", "Global variables size is " + to_string(context.globalsSize) + " bytes", "", LineInfo());
            canAllocateVariables = false;
        }
        if ( context.sharedSize && !context.shared ) {
            error("Failed to allocate memory for shared variables", "Shared variables size is " + to_string(context.sharedSize) + " bytes", "", LineInfo());
            canAllocateVariables = false;
        }
        if ( !canAllocateVariables ) {
            context.globalsSize = 0;
            context.sharedSize = 0;
            context.totalVariables = 0;
        }
        context.sharedOwner = true;
        context.totalVariables = totalVariables;
        context.functions = (SimFunction *) context.code->allocate( totalFunctions*sizeof(SimFunction) );
        context.totalFunctions = totalFunctions;
        auto debuggerOrGC = getDebugger()  || context.thisProgram->options.getBoolOption("gc",false);
        vector<FunctionPtr> lookupFunctionTable;
        das_hash_map<uint64_t,Function *> fnByMnh;
        bool anyPInvoke = false;
        if ( totalFunctions ) {
            for (auto & pm : library.modules) {
                pm->functions.foreach([&](auto pfun){
                    if (pfun->index < 0 || !pfun->used)
                        return;
                    if ( (pfun->init || pfun->shutdown) && disableInit ) {
                        error("[init] is disabled in the options or CodeOfPolicies",
                            "internal compiler error: [init] function made it all the way to simulate somehow", "",
                                pfun->at, CompilationError::no_init);
                    }
                    auto mangledName = pfun->getMangledName();
                    auto MNH = hash_blockz64((uint8_t *)mangledName.c_str());
                    if ( MNH==0 ) {
                        error("Internalc compiler errors. Mangled name hash is zero. Function " + pfun->name,
                            "\tMangled name " + mangledName + " hash is " + to_string(MNH), "",
                                pfun->at);
                    }
                    fnByMnh[MNH] = pfun.get();
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
                                auto sr = ExprLet::simulateInit(context, pvar, false);
                                auto gvari = context.code->makeNode<SimNode_SetLocalRefAndEval>(pvar->init->at, sl, sr, uint32_t(sizeof(Prologue)));
                                auto cndb = context.code->makeNode<SimNode_GetArgument>(pvar->init->at, 1); // arg 1 of init script is "init_globals"
                                gvar.init = context.code->makeNode<SimNode_IfThen>(pvar->init->at, cndb, gvari);
                            } else {
                                auto sl = context.code->makeNode<SimNode_GetGlobalMnh>(pvar->init->at, pvar->stackTop, pvar->getMangledNameHash());
                                auto sr = ExprLet::simulateInit(context, pvar, false);
                                gvar.init = context.code->makeNode<SimNode_SetLocalRefAndEval>(pvar->init->at, sl, sr, uint32_t(sizeof(Prologue)));
                            }
                        } else {
                            gvar.init = ExprLet::simulateInit(context, pvar, false);
                        }
                    } else {
                        gvar.init = nullptr;
                    }
                });
            }
        }
        // if there is anything pinvoke or the option is set
        if ( anyPInvoke || policies.threadlock_context || policies.debugger ) {
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
            fusion(context, logs);
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
                if (pfun->index < 0 || !pfun->used)
                    return;
                auto & gfun = context.functions[pfun->index];
                for ( const auto & an : pfun->annotations ) {
                    auto fna = static_pointer_cast<FunctionAnnotation>(an->annotation);
                    if (!fna->simulate(&context, &gfun)) {
                        error("function " + pfun->describe() + " annotation " + fna->name + " simulation failed", "", "",
                            LineInfo(), CompilationError::cant_initialize);
                    }
                }
                indexToFunction[pfun->index] = pfun.get();
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
                        auto fna = static_pointer_cast<FunctionAnnotation>(ann->annotation);
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
        // lockchecking
        context.skipLockChecks = options.getBoolOption("skip_lock_checks",false);
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

        // log CPP
        if (options.getBoolOption("log_cpp")) {
            aotCpp(context,logs);
            registerAotCpp(logs,context);
        }
        context.debugger = getDebugger();
        isSimulating = false;
        context.thisHelper = &helper;   // note - we may need helper for the 'complete'
        auto boundProgram = daScriptEnvironment::bound->g_Program;
        daScriptEnvironment::bound->g_Program = this;   // node - we are calling macros
        library.foreach_in_order([&](Module * pm) -> bool {
            for ( auto & sm : pm->simulateMacros ) {
                if ( !sm->preSimulate(this, &context) ) {
                    error("simulate macro " + pm->name + "::" + sm->name + " failed to preSimulate", "", "", LineInfo());
                    daScriptEnvironment::bound->g_Program = boundProgram;
                    return false;
                }
            }
            return true;
        }, thisModule.get());
        for ( int i=0, is=context.totalFunctions; i!=is; ++i ) {
            Function *func = indexToFunction[i];
            for (auto &ann : func->annotations) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto fann = static_pointer_cast<FunctionAnnotation>(ann->annotation);
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
                        auto sann = static_pointer_cast<StructureAnnotation>(ann->annotation);
                        sann->complete(&context, st);
                    }
                }
            });
        }
        library.foreach_in_order([&](Module * pm) -> bool {
            for ( auto & sm : pm->simulateMacros ) {
                if ( !sm->simulate(this, &context) ) {
                    error("simulate macro " + pm->name + "::" + sm->name + " failed to simulate", "", "", LineInfo());
                    daScriptEnvironment::bound->g_Program = boundProgram;
                    return false;
                }
            }
            return true;
        }, thisModule.get());
        context.thisHelper = nullptr;
        daScriptEnvironment::bound->g_Program = boundProgram;
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

    uint64_t Program::getInitSemanticHashWithDep( uint64_t initHash ) {
        vector<const Variable *> globs;
        globs.reserve(totalVariables);
        for (auto & pm : library.modules) {
            pm->globals.foreach([&](auto var){
                if (var->used) {
                    globs.push_back(var.get());
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
                if (pfun->index < 0 || !pfun->used)
                    return;
                fnn.push_back(pfun.get());
                indexToFunction[pfun->index] = pfun.get();
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
                uint64_t semHash = fnn[fni]->aotHash = getFunctionAotHash(fnn[fni]);
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
                    printSimFunction(tp, &context, indexToFunction[fni], fn.code, true);
                    linkError(string(fn.mangledName), tp.str() );
                }
            }
        }
    }
}
