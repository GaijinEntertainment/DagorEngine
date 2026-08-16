#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class SetRefSpVisitor : public Visitor {
    public:
        SetRefSpVisitor ( bool r, bool c, uint32_t s, uint32_t o )
            : ref(r), cmres(c), sp(s), off(o) {}
    protected:
        bool ref, cmres;
        uint32_t sp, off;

        void applyFields ( ExprMakeLocal * e ) {
            e->useStackRef = ref;
            e->useCMRES = cmres;
            e->doesNotNeedSp = true;
            e->doesNotNeedInit = true;
            e->stackTop = sp;
            e->extraOffset = off;
        }

        static void markCmresSkip ( Expression * val ) {
            if ( val->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(val);
                if ( cll->allowCmresSkip() ) cll->doesNotNeedSp = true;
            } else if ( val->rtti_isInvoke() ) {
                auto cll = static_cast<ExprInvoke*>(val);
                if ( cll->allowCmresSkip() ) cll->doesNotNeedSp = true;
            }
        }

        void recurse ( Expression * val, uint32_t childOff ) {
            if ( val->rtti_isMakeLocal() ) {
                SetRefSpVisitor sub(ref, cmres, sp, childOff);
                val->dispatch(sub);
            } else {
                markCmresSkip(val);
            }
        }

        virtual void preVisit ( ExprMakeTuple * expr ) override {
            applyFields(expr);
            int total = int(expr->values.size());
            for ( int index=0; index != total; ++index ) {
                recurse(expr->values[index], expr->extraOffset + expr->makeType->getTupleFieldOffset(index));
            }
        }

        virtual void preVisit ( ExprMakeArray * expr ) override {
            applyFields(expr);
            int total = int(expr->values.size());
            uint32_t stride = expr->recordType->getSizeOf();
            for ( int index=0; index != total; ++index ) {
                recurse(expr->values[index], expr->extraOffset + index*stride);
            }
        }

        virtual void preVisit ( ExprMakeStruct * expr ) override {
            applyFields(expr);
            auto mkBaseT = expr->makeType;
            while ( mkBaseT->baseType==Type::tFixedArray && mkBaseT->firstType ) mkBaseT = mkBaseT->firstType;
            if ( mkBaseT->baseType == Type::tHandle ) return;
            int total = int(expr->structs.size());
            int stride = expr->makeType->getStride();
            for ( int index=0; index != total; ++index ) {
                auto & fields = expr->structs[index];
                for ( const auto & decl : *fields ) {
                    auto field = mkBaseT->structType->findField(decl->name);
                    DAS_ASSERT(field && "should have failed in type infer otherwise");
                    recurse(decl->value, expr->extraOffset + index*stride + field->offset);
                }
            }
        }

        virtual void preVisit ( ExprMakeVariant * expr ) override {
            applyFields(expr);
            auto mkBaseT = expr->makeType;
            while ( mkBaseT->baseType==Type::tFixedArray && mkBaseT->firstType ) mkBaseT = mkBaseT->firstType;
            int stride = expr->makeType->getStride();
            int index = 0;
            for ( const auto & decl : expr->variants ) {
                auto fieldVariant = mkBaseT->findArgumentIndex(decl->name);
                DAS_ASSERT(fieldVariant!=-1 && "should have failed in type infer otherwise");
                if ( decl->value->rtti_isMakeLocal() ) {
                    auto fieldOffset = mkBaseT->getVariantFieldOffset(fieldVariant);
                    uint32_t offset = expr->extraOffset + index*stride + fieldOffset;
                    SetRefSpVisitor sub(ref, cmres, sp, offset);
                    decl->value->dispatch(sub);
                    static_cast<ExprMakeLocal*>(decl->value)->doesNotNeedInit = false;
                } else {
                    markCmresSkip(decl->value);
                }
                index++;
            }
        }
    };

    static void applySetRefSp ( ExprMakeLocal * mkl, bool ref, bool cmres, uint32_t sp, uint32_t off ) {
        SetRefSpVisitor vis(ref, cmres, sp, off);
        mkl->dispatch(vis);
    }

    // escape analysis stack-allocates a non-escaping `new` pointee into the frame (allocate_on_stack).
    // That pointee is the variable's backing storage and must live to scope exit, so the scoped
    // allocator must NOT reclaim it together with the initializer's temporaries.
    static bool initAllocatesOnStack ( Expression * init ) {
        if ( !init ) return false;
        if ( init->rtti_isAscend() ) return static_cast<ExprAscend *>(init)->allocate_on_stack;
        if ( init->rtti_isNewExpr() ) return static_cast<ExprNew *>(init)->allocate_on_stack;
        return false;
    }

    class VarCMRes : public Visitor {
    public:
        VarCMRes( const ProgramPtr & prog, bool everything ) {
            program = prog;
            isEverything = everything;
        }
    protected:
        vector<ExprBlock *>     blocks;
        vector<ExprBlock *>     scopes;
        ProgramPtr              program;
        FunctionPtr             func = nullptr;
        VariablePtr             cmresVAR = nullptr;
        bool                    failedToCMRES = false;
        bool                    isEverything = false;
        // SimNode_MakeArrayHeap repoints abiCMRES at the heap block while constructing
        // elements, so a CMRES-aliased local read inside a heap-mode literal would resolve
        // into the (zeroed) heap buffer instead of the return slot. Track such reads and
        // decline the elision for that variable — an extra move at return, always correct.
        das_hash_set<Variable *> usedInHeapLiteral;
        int                     heapLiteralDepth = 0;
    protected:

        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * var ) override { return isEverything || var->used; }
        virtual bool canVisitFunction ( Function * fun ) override { return !fun->isTemplate && !fun->stub && (isEverything || fun->used); }
    // function
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
        }
        virtual FunctionPtr visit ( Function * that ) override {
            if ( cmresVAR && !failedToCMRES
                && usedInHeapLiteral.find(cmresVAR) == usedInHeapLiteral.end() ) cmresVAR->aliasCMRES = true;
            func = nullptr;
            cmresVAR = nullptr;
            failedToCMRES = false;
            usedInHeapLiteral.clear();
            DAS_ASSERT(blocks.size()==0);
            DAS_ASSERT(heapLiteralDepth==0);
            return Visitor::visit(that);
        }
    // heap-mode array literal (gen2 [..] feeding to_array_move/to_table_move)
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->makeArrayOnHeap ) heapLiteralDepth ++;
        }
        virtual ExpressionPtr visit ( ExprMakeArray * expr ) override {
            if ( expr->makeArrayOnHeap ) heapLiteralDepth --;
            return Visitor::visit(expr);
        }
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            if ( heapLiteralDepth && expr->local && expr->variable ) {
                usedInHeapLiteral.insert(expr->variable);
            }
        }
    // ExprBlock
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->isClosure ) {
                blocks.push_back(block);
            }
            scopes.push_back(block);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            scopes.pop_back();
            if ( block->isClosure ) {
                blocks.pop_back();
            }
            return Visitor::visit(block);
        }
    // ExprReturn
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            if ( blocks.size() ) {
                expr->block = blocks.back();
                return;
            }
            expr->returnFunc = func;
            if ( failedToCMRES ) return;
            // we can safely skip makeLocal, invoke, or call
            // they are going to CMRES directly, and do not affect nothing
            ExprVar * evar = nullptr;
            if ( !func->result->isRefType() ) return;
            else if ( expr->subexpr->rtti_isInvoke() ) return;
            else if ( expr->subexpr->rtti_isMakeLocal() ) return;
            // if its return X
            else if ( expr->subexpr->rtti_isVar() ) {
                evar = (ExprVar *)(expr->subexpr);
            } else {
                return;
            }
            if ( evar ) {
                if ( !evar->local ) return; // if its not local, we safely ignore
                auto var = evar->variable;
                if ( var->inScope ) {               // if its in scope, we cannot return it as CMRES
                    failedToCMRES = true;
                } else if ( var->type->ref ) {     // we return local ref variable, so ... game over
                    failedToCMRES = true;
                } else if ( !cmresVAR ) {
                    cmresVAR = var;
                } else if ( cmresVAR!=var ) {
                    // TODO:    verify if we need to fail?
                    failedToCMRES = true;
                }
            }
            if ( !failedToCMRES ) {
                for ( auto blk = scopes.rbegin(); blk!=scopes.rend(); ++blk ) {
                    if ( (*blk)->isClosure ) {
                        break;
                    }
                    if ( (*blk)->finalList.size() ) {
                        // if we have final list, we cannot return as CMRES
                        failedToCMRES = true;
                        break;
                    }
                }
            }
        }
    };

    class AllocateStack : public Visitor {
    public:
        AllocateStack( const ProgramPtr & prog, bool permanent, bool everything, TextWriter & ls ) : logs(ls) {
            program = prog;
            log = prog->options.getBoolOption("log_stack");
            log_var_scope = prog->options.getBoolOption("log_var_scope");
            optimize = prog->getOptimize();
            noFastCall = prog->options.getBoolOption("no_fast_call", prog->policies.no_fast_call);
            scopedStackAllocator = prog->options.getBoolOption("scoped_stack_allocator", prog->policies.scoped_stack_allocator);
            if( log ) {
                logs << "\nSTACK INFORMATION in " << prog->thisModule->name << ":\n";
            }
            isPermanent = permanent;
            isEverything = everything;
        }
    protected:
        struct StackInfo {
            const uint32_t startSegment;
            uint32_t maxStack;
        };

        ProgramPtr              program;
        FunctionPtr             func = nullptr;
        uint32_t                stackTop = 0;
        vector<StackInfo>       stackTopStack;
        vector<ExprBlock *>     blocks;
        vector<ExprBlock *>     scopes;
        uint32_t                doNotOptimize = 0; // do not optimize stack when its non-zero
        bool                    log = false;
        bool                    log_var_scope = false;
        bool                    optimize = false;
        TextWriter &            logs;
        bool                    inStruct = false;
        bool                    noFastCall = false;
        bool                    isPermanent = false;
        bool                    isEverything = false;
        bool                    scopedStackAllocator = false;
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * var ) override {
            if ( var->stackResolved ) return false;
            if ( !var->used && !isEverything ) return false;
            var->stackResolved = isPermanent;
            return true;
        }
        virtual bool canVisitFunction ( Function * fun ) override {
            if ( fun->stub ) return false;
            if ( fun->isTemplate ) return false;
            if ( fun->stackResolved ) return false;
            if ( !fun->used && !isEverything ) return false;
            fun->stackResolved = isPermanent;
            return true;
        }
    protected:
        uint32_t allocateStack ( uint32_t size ) {
            auto result = stackTop;
            stackTop += (size + 0xf) & ~0xf;
            return result;
        }

        void pushSp() {
            if (scopedStackAllocator) {
                stackTopStack.emplace_back(StackInfo{stackTop, stackTop});
            }
        }

        StackInfo popSp() {
            if (!scopedStackAllocator) {
                return {stackTop, stackTop};
            }
            stackTopStack.back().maxStack = max(stackTopStack.back().maxStack, stackTop);
            const auto top = stackTopStack.back();
            stackTopStack.pop_back();
            if (!stackTopStack.empty()) {
                stackTopStack.back().maxStack = max(stackTopStack.back().maxStack, top.maxStack);
            }
            if (doNotOptimize == 0) {
                stackTop = top.startSegment; // reduce stackTop => reuse space
            }
            return top;
        }

    // structure
        virtual bool canVisitStructure ( Structure * st ) override {
            return !st->isTemplate;     // not a thing with templates
        }
        virtual void preVisit ( Structure * var ) override {
            Visitor::preVisit(var);
            inStruct = true;
        }
        virtual StructurePtr visit ( Structure * var ) override {
            inStruct = false;
            return Visitor::visit(var);
        }
    // global variable init
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            var->initStackSize = 0;
            stackTop = sizeof(Prologue);
            pushSp();
            if ( var->init  ) {
                if ( var->init->rtti_isMakeLocal() ) {
                    uint32_t sz = sizeof(void *);
                    uint32_t refStackTop = allocateStack(sz);
                    if ( log ) {
                        logs << "\t" << refStackTop << "\t" << sz
                            << "\tinit global " << var->name << " [[ ]], line " << var->init->at.line << "\n";
                    }
                    auto mkl = static_cast<ExprMakeLocal*>(var->init);
                    applySetRefSp(mkl, true, false, refStackTop, 0);
                    mkl->doesNotNeedInit = false;
                    mkl->doesNotNeedSp = true;
                } else if ( var->init->rtti_isCall() ) {
                    auto cll = static_cast<ExprCall*>(var->init);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    } else {
                        cll->doesNotNeedSp = false;
                    }
                } else if ( var->init->rtti_isInvoke() ) {
                    auto cll = static_cast<ExprInvoke*>(var->init);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    } else {
                        cll->doesNotNeedSp = false;
                    }
                }
            }
        }
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override {
            auto top = popSp();
            var->initStackSize = top.maxStack;
            program->globalInitStackSize = das::max(program->globalInitStackSize, top.maxStack);
            return Visitor::visitGlobalLet(var);
        }
    // function
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
            stackTop = sizeof(Prologue);
            pushSp();
            if ( log ) {
                if (!func->used) logs << "unused ";
                logs << func->describe() << "\n";
            }
        }
        virtual FunctionPtr visit ( Function * that ) override {
            auto top = popSp();
            func->totalStackSize = top.maxStack;
            DAS_ASSERT(stackTopStack.empty());
            // detecting fastcall
            func->fastCall = false;
            if ( !program->getDebugger() && !program->getProfiler() && !noFastCall ) {
                if ( !func->exports && !func->addr && func->totalStackSize==sizeof(Prologue) && func->arguments.size()<=32 ) {
                    if (func->body->rtti_isBlock()) {
                        auto block = static_cast<ExprBlock*>(func->body);
                        if ( func->result->isWorkhorseType() ) {
                            if (block->list.size()==1 && block->finalList.size()==0 && block->list.back()->rtti_isReturn()) {
                                auto ret = (ExprReturn *) block->list.back();
                                if ( !ret->moveSemantics ) func->fastCall = true;   // can't fast-call return <-
                            }
                        } else if ( func->result->isVoid() ) {
                            if ( block->list.size()==1 && block->finalList.size()==0 && !block->list.back()->rtti_isBlock() ) {
                                if ( !block->list.back()->rtti_isWith() ) {
                                    func->fastCall = true;
                                }
                            }
                        }
                    }
                }
            }
            if ( log ) {
                logs << func->totalStackSize << "\ttotal" << (func->fastCall ? ", fastcall" : "") << "\n";
            }
            func = nullptr;
            return Visitor::visit(that);
        }
    // ExprReturn
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( blocks.size() ) {
                auto block = blocks.back();
                expr->stackTop = block->stackTop;
                if ( log ) {
                    logs << "\t\t" << expr->stackTop << "\t\treturn, line " << expr->at.line << "\n";
                }
            } else {
                DAS_ASSERT(!expr->returnInBlock);
            }
            if ( expr->subexpr ) {
                // only route a make-local return through CMRES when the function returns via
                // CMRES; a register-returned (non-cmres) function has no result buffer, so
                // writing the make-local through one segfaults — build a normal local instead.
                bool makeLocalCMRES = expr->returnInBlock || !func || func->copyOnReturn || func->moveOnReturn;
                if ( expr->subexpr->rtti_isMakeLocal() && makeLocalCMRES ) {
                    uint32_t sz = sizeof(void *);
                    expr->refStackTop = allocateStack(sz);
                    expr->takeOverRightStack = true;
                    if ( log ) {
                        logs << "\t" << expr->refStackTop << "\t" << sz
                        << "\treturn ref and eval, line " << expr->at.line << "\n";
                    }
                    auto mkl = static_cast<ExprMakeLocal*>(expr->subexpr);
                    if ( expr->returnInBlock ) {
                        applySetRefSp(mkl, true, false, expr->refStackTop, 0);
                    } else {
                        applySetRefSp(mkl, true, true, expr->refStackTop, 0);
                        expr->returnCMRES = true;
                    }
                    mkl->doesNotNeedInit = false;
                } else if ( expr->subexpr->rtti_isCall() ) {
                    auto cll = static_cast<ExprCall*>(expr->subexpr);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                        expr->returnCallCMRES = true;
                    } else {
                        cll->doesNotNeedSp = false;
                        expr->returnCallCMRES = false;
                    }
                } else if ( expr->subexpr->rtti_isInvoke() ) {
                    auto cll = static_cast<ExprInvoke*>(expr->subexpr);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                        expr->returnCallCMRES = true;
                    } else {
                        cll->doesNotNeedSp = false;
                        expr->returnCallCMRES = false;
                    }
                } else if ( expr->subexpr->rtti_isVar() ) {
                    auto evar = static_cast<ExprVar*>(expr->subexpr);
                    if ( evar->variable->aliasCMRES ) {
                        expr->returnCMRES = true;
                    } else {
                        expr->returnCMRES = false;
                    }
                }
            }
        }
    // ExprLabel
        virtual void preVisit ( ExprLabel * expr ) override {
            Visitor::preVisit(expr);
            auto sc = scopes.back();
            sc->maxLabelIndex = das::max ( sc->maxLabelIndex, expr->label );
        }
    // ExprBlock
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            block->stackVarTop = allocateStack(0);
            block->stackCleanVars.clear();
            if ( inStruct ) return;
            // A block with a `finally` section runs that section on EVERY exit, including
            // an early return that precedes a later variable's declaration. Disable stack
            // reuse for the whole block so its locals keep distinct slots and the
            // block-entry memzero (SimulateVisitor::visit(ExprBlock*)) stays valid when
            // the finally fires before a variable's initializer ran.
            if ( block->finalList.size() ) doNotOptimize++;
            if ( block->isClosure ) {
                blocks.push_back(block);
            }
            scopes.push_back(block);
            block->maxLabelIndex = -1;
            if ( block->arguments.size() || block->copyOnReturn || block->moveOnReturn ) {
                auto sz = uint32_t(sizeof(BlockArguments));
                block->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << block->stackTop << "\t" << sz
                        << "\tblock arguments, line " << block->at.line << "\n";
                }
            }
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            if ( !inStruct && block->finalList.size() ) doNotOptimize--;
            auto top = inStruct ? stackTop : popSp().maxStack;
            block->stackVarBottom = top;

            if ( inStruct ) {
                return Visitor::visit(block);
            }
            scopes.pop_back();
            if ( block->isClosure ) {
                blocks.pop_back();
            }
            return Visitor::visit(block);
        }

    // ExprBlockExpr
        virtual void preVisitBlockExpression(ExprBlock* block, Expression* expr) override {
            Visitor::preVisitBlockExpression(block, expr);
            if (!expr->rtti_isLet()) {
                pushSp();
            }
        }

        virtual ExpressionPtr visitBlockExpression(ExprBlock* block, Expression* expr) override {
            if (!expr->rtti_isLet()) {
                popSp();
            }
            return Visitor::visitBlockExpression(block, expr);
        }

    // ExprOp1
        virtual void preVisit ( ExprOp1 * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->func->copyOnReturn || expr->func->moveOnReturn ) {
                auto sz = expr->func->result->getSizeOf();
                expr->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << expr->stackTop << "\t" << sz
                    << "\top1, line " << expr->at.line << "\n";
                }
            }
            onPreExprCallFunc(expr);
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprOp1 * expr ) override {
            if (!inStruct) {
                onExprCallFunc(expr);
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprOp2
        virtual void preVisit ( ExprOp2 * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->func->copyOnReturn || expr->func->moveOnReturn ) {
                auto sz = expr->func->result->getSizeOf();
                expr->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << expr->stackTop << "\t" << sz
                        << "\top2, line " << expr->at.line << "\n";
                }
            }
            onPreExprCallFunc(expr);
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprOp2 * expr ) override {
            if (!inStruct) {
                onExprCallFunc(expr);
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprCallFunc
        void onPreExprCallFunc ( const ExprCallFunc * expr ) {
            // No need to call for ExprOp3 because it's ternary operator => execute linearly
            const auto efun = expr->func;
            if (expr->func && expr->func->arguments.size() > 1) {
                doNotOptimize += efun->builtIn || efun->interopFn;
            }
        }

        void onExprCallFunc ( const ExprCallFunc * expr ) {
            const auto efun = expr->func;
            if (expr->func && expr->func->arguments.size() > 1) {
                doNotOptimize -= efun->builtIn || efun->interopFn;
            }
        }
    // ExprCall
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            // temp-string marking (builders and [temp_string_result] calls) lives in
            // MarkTempStrings - one global pass at the head of Program::allocateStack
            if ( !expr->func ) return;
            if ( inStruct ) return;
            if ( !expr->doesNotNeedSp ) {
                if ( expr->func->copyOnReturn || expr->func->moveOnReturn ) {
                    auto sz = expr->func->result->getSizeOf();
                    expr->stackTop = allocateStack(sz);
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                            << "\tcall, line " << expr->at.line
                            << " // " << expr->describe() << "\n";
                    }
                }
            }
            // order of evaluation for interop functions is not specified
            onPreExprCallFunc(expr);
            pushSp();
        }

        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            auto efun = expr->func;
            if (!inStruct && efun != nullptr) {
                onExprCallFunc(expr);
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprInvoke
        virtual void preVisit ( ExprInvoke * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( !expr->doesNotNeedSp ) {
                if ( expr->type->isRefType() ) {
                    auto sz = expr->type->getSizeOf();
                    expr->stackTop = allocateStack(sz);
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                            << "\tinvoke, line " << expr->at.line << "\n";
                    }
                }
            }
            pushSp();
        }

        virtual ExpressionPtr visit ( ExprInvoke * expr ) override {
            if (!inStruct) {
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprFor

        virtual void preVisitForStack ( ExprFor * expr ) override {
            Visitor::preVisitForStack(expr);
            if ( inStruct ) return;
            for ( auto & var : expr->iteratorVariables ) {
                auto sz = var->type->getSizeOf();
                var->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << var->stackTop << "\t" << sz
                        << "\tfor " << var->name << ", line " << var->at.line << "\n";
                }
            }
            /*
            for ( auto & src : expr->sources ) {
                if ( src->rtti_isMakeLocal() ) {
                    auto mkl = static_cast<ExprMakeLocal*>(src);
                    mkl->needTempSrc = true;
                }
            }
            */
        }
    // ExprLet
        virtual void preVisitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(expr,var,last);
            if ( inStruct ) return;
            uint32_t sz = var->type->ref ? sizeof(void *) : var->type->getSizeOf();
            if ( var->aliasCMRES ) {
                if ( log ) {
                    logs << "\tCR\t" << sz
                        << "\tlet " << var->name << ", line " << var->at.line << "\n";
                }
            } else {
                var->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << var->stackTop << "\t" << sz
                        << "\tlet " << var->name << ", line " << var->at.line << "\n";
                    if ( log_var_scope ) {
                        logs <<"\t\t\t\t" << expr->visibility.describe(true) << "\n";
                    }
                }
            }
            for ( auto blk=scopes.rbegin(), blks=scopes.rend(); blk!=blks; ++blk ) {
                auto pblock = *blk;
                pblock->stackCleanVars.push_back(make_pair(var->stackTop, var->type->ref ? int(sizeof(void *)) : var->type->getSizeOf()));
                if ( pblock->isClosure ) break;
            }
            if ( var->init ) {
                if ( var->init->rtti_isMakeLocal() ) {
                    auto mkl = static_cast<ExprMakeLocal*>(var->init);
                    applySetRefSp(mkl, false, var->aliasCMRES, var->stackTop, 0);
                    mkl->doesNotNeedInit = false;
                } else if ( var->init->rtti_isCall() ) {
                    auto cll = static_cast<ExprCall*>(var->init);
                    if ( (cll->func->copyOnReturn || cll->func->moveOnReturn) && !cll->func->aliasCMRES ) { // note: let never aliases!!!
                        cll->doesNotNeedSp = true;
                    } else {
                        cll->doesNotNeedSp = false;
                    }
                } else if ( var->init->rtti_isInvoke() ) {
                    auto cll = static_cast<ExprInvoke*>(var->init);
                    if ( cll->isCopyOrMove() ) {
                        cll->doesNotNeedSp = true;
                    } else {
                        cll->doesNotNeedSp = false;
                    }
                }
            }
            if (!var->type->ref && var->type->baseType != Type::tBlock) {
                // a stack-allocated pointee is the variable's storage, not a temporary - keep it past
                // the init's stack-reuse scope so a later local does not clobber the live pointee
                if ( initAllocatesOnStack(var->init) ) doNotOptimize++;
                pushSp(); // Free everything allocated to init let (not let itself)
            }
        }
        virtual VariablePtr visitLet ( ExprLet * /*expr*/, const VariablePtr & var, bool /*last*/ ) override {
            if (!inStruct && !var->type->ref && var->type->baseType != Type::tBlock) {
                popSp();
                if ( initAllocatesOnStack(var->init) ) doNotOptimize--;
            }
            return var;
        }

    // ExprMakeBlock
        virtual void preVisit ( ExprMakeBlock * expr ) override {
            doNotOptimize++;
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            auto sz = uint32_t(sizeof(Block));
            expr->stackTop = allocateStack(sz);
            if ( log ) {
                logs << "\t" << expr->stackTop << "\t" << sz
                << "\tmake block " << expr->type->describe() << ", line " << expr->at.line << "\n";
            }
            if ( func ) {
                func->hasMakeBlock = true;
                for ( auto blk : blocks ) {
                    blk->hasMakeBlock = true;
                }
            }
            // pushSp();
        }

        virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override {
            doNotOptimize--;
            // if (!inStruct) {
            //     popSp(); // Variables inside block and regular ones share stack.
            // }
            return Visitor::visit(expr);
        }

    // ExprAscend
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->subexpr->rtti_isMakeLocal() ) {
                auto mkl = static_cast<ExprMakeLocal*>(expr->subexpr);
                if ( expr->allocate_on_stack && !expr->needTypeInfo ) {
                    // build the value directly into the frame, no ref slot and no heap copy
                    uint32_t sz = expr->subexpr->type->getSizeOf();
                    expr->stackTop = allocateStack(sz);
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                        << "\tascend stack, line " << expr->at.line << "\n";
                    }
                    applySetRefSp(mkl, false, false, expr->stackTop, 0);
                } else {
                    uint32_t sz = sizeof(void *);
                    expr->stackTop = allocateStack(sz);
                    expr->useStackRef = true;
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                        << "\tascend, line " << expr->at.line << "\n";
                    }
                    applySetRefSp(mkl, true, false, expr->stackTop, 0);
                }
            }
            pushSp();
        }

        virtual ExpressionPtr visit ( ExprAscend * expr ) override {
            if (!inStruct) {
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprMakeStructure
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( !expr->doesNotNeedSp ) {
                auto sz = expr->type->getSizeOf();
                uint32_t cStackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << cStackTop << "\t" << sz
                        << "\t[[" << expr->type->describe() << "]], line " << expr->at.line << "\n";
                }
                applySetRefSp(expr, false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
            pushSp();
        }

        virtual ExpressionPtr visit ( ExprMakeStruct * expr ) override {
            if (!inStruct) {
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprMakeArray
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( !expr->doesNotNeedSp ) {
                auto sz = expr->type->getSizeOf();
                uint32_t cStackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << cStackTop << "\t" << sz
                    << "\t[[" << expr->type->describe() << "]], line " << expr->at.line << "\n";
                }
                // heap array literal: slot holds the array<T> value (sizeof Array); element writes
                // go into the heap buffer via cmres (set to arr.data by SimNode_MakeArrayHeap).
                applySetRefSp(expr, false, expr->makeArrayOnHeap, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
            pushSp();
        }

        virtual ExpressionPtr visit ( ExprMakeArray * expr ) override {
            if (!inStruct) {
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprMakeTuple
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( !expr->doesNotNeedSp ) {
                auto sz = expr->type->getSizeOf();
                uint32_t cStackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << cStackTop << "\t" << sz
                    << "\t[[" << expr->type->describe() << "]], line " << expr->at.line << "\n";
                }
                applySetRefSp(expr, false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
            pushSp();
        }

        virtual ExpressionPtr visit ( ExprMakeTuple * expr ) override {
            if (!inStruct) {
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprMakeVariant
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( !expr->doesNotNeedSp ) {
                auto sz = expr->type->getSizeOf();
                uint32_t cStackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << cStackTop << "\t" << sz
                    << "\t[[" << expr->type->describe() << "]], line " << expr->at.line << "\n";
                }
                applySetRefSp(expr, false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprMakeVariant * expr ) override {
            if (!inStruct) {
                popSp();
            }
            return Visitor::visit(expr);
        }

    // New
        virtual void preVisit ( ExprNew * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->type->baseType==Type::tFixedArray ) {
                auto sz = uint32_t(expr->type->getCountOf()*sizeof(char *));
                expr->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << expr->stackTop << "\t" << sz
                    << "\tNEW " << expr->typeexpr->describe() << ", line " << expr->at.line << "\n";
                }
            } else if ( expr->allocate_on_stack ) {
                auto sz = expr->type->firstType->getBaseSizeOf();
                expr->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << expr->stackTop << "\t" << sz
                    << "\tNEW stack " << expr->typeexpr->describe() << ", line " << expr->at.line << "\n";
                }
            }
            onPreExprCallFunc(expr);
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprNew * expr ) override {
            if (!inStruct) {
                onExprCallFunc(expr);
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprCopy
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->right->rtti_isMakeLocal() ) {
                auto mkl = static_cast<ExprMakeLocal*>(expr->right);
                if ( !mkl->alwaysAlias ) {
                    uint32_t sz = sizeof(void *);
                    expr->stackTop = allocateStack(sz);
                    expr->takeOverRightStack = true;
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                            << "\tcopy [[ ]], line " << expr->at.line << "\n";
                    }
                    applySetRefSp(mkl, true, false, expr->stackTop, 0);
                    mkl->doesNotNeedInit = false;
                }
            } else if ( expr->right->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                } else {
                    cll->doesNotNeedSp = false;
                }
            } else if ( expr->right->rtti_isInvoke() ) {
                auto cll = static_cast<ExprInvoke*>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                } else {
                    cll->doesNotNeedSp = false;
                }
            }
            onPreExprCallFunc(expr);
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprCopy * expr ) override {
            if (!inStruct) {
                onExprCallFunc(expr);
                popSp();
            }
            return Visitor::visit(expr);
        }

    // ExprMove
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->right->rtti_isMakeLocal() ) {
                auto mkl = static_cast<ExprMakeLocal*>(expr->right);
                if ( !mkl->alwaysAlias ) {
                    uint32_t sz = sizeof(void *);
                    expr->stackTop = allocateStack(sz);
                    expr->takeOverRightStack = true;
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                            << "\tcopy [[ ]], line " << expr->at.line << "\n";
                    }
                    applySetRefSp(mkl, true, false, expr->stackTop, 0);
                    mkl->doesNotNeedInit = false;
                }
            } else if ( expr->right->rtti_isCall() ) {
                auto cll = static_cast<ExprCall*>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                } else {
                    cll->doesNotNeedSp = false;
                }
            } else if ( expr->right->rtti_isInvoke() ) {
                auto cll = static_cast<ExprInvoke*>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                } else  {
                    cll->doesNotNeedSp = false;
                }
            }
            onPreExprCallFunc(expr);
            pushSp();
        }
        virtual ExpressionPtr visit ( ExprMove * expr ) override {
            if (!inStruct) {
                onExprCallFunc(expr);
                popSp();
            }
            return Visitor::visit(expr);
        }
    // ExprClone
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            onPreExprCallFunc(expr);
        }

        virtual ExpressionPtr visit ( ExprClone * expr ) override {
            Visitor::visit(expr);
            onExprCallFunc(expr);
            return expr;
        }

    };

    class AllocateConstString : public Visitor {
    public:
        uint32_t bytesTotal = 0;
        das_hash_set<string>    uniStr;
    public:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * var ) override { return var->used; }
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->isTemplate && fun->used;
        }
        void allocateString ( const string & message ) {
            if ( !message.empty() ) {
                if ( uniStr.find(message)==uniStr.end() ) {
                    uniStr.insert(message);
                    uint32_t allocSize = uint32_t(message.length()) + 1;
                    allocSize = (allocSize + 3) & ~3;
                    bytesTotal += allocSize;
                }
            }
        }
        virtual void preVisit ( ExprConstString * expr ) override {
            Visitor::preVisit(expr);
            allocateString(expr->text);
        }
    };

    // Temp-string reclaim: pick ONE queue site per consuming call and route it through the
    // 1-slot dispose queue - a marked string builder (allocates, then queues itself) or a
    // [temp_string_result] call wrapped in _temp_string_result (frees the previously queued
    // temp, parks the fresh result).
    //
    // Soundness: a queued temp lives until its consuming call returns, and the only code that
    // can run in that window is the evaluation of the SIBLING arguments. Extern (interop)
    // argument evaluation order is UNSPECIFIED (right-to-left on MSVC), so a nested queue site
    // in ANY sibling subtree could flush the site while it is still live and the persistent-heap
    // shoe would reissue the cell - a silent use-after-free. Hence ONE site per call, and site
    // creation is inhibited in every sibling subtree. (The old per-call "mark the last builder"
    // scan violated this: compare(to_upper("{b}"), "{a}") corrupted arg1 on every iteration.)
    static bool isFreshStringCall ( Expression * e ) {
        if ( !e->rtti_isCall() ) return false;
        auto c = static_cast<ExprCall *>(e);
        return c->func && c->func->tempStringResult && c->type && c->type->isString() && !c->type->ref;
    }

    static ExprCall * makeTempStringWrapper ( Function * wrapper, Expression * inner ) {
        auto w = new ExprCall(inner->at, wrapper->name);
        w->func = wrapper;
        w->generated = true;
        w->notDiscarded = true;
        w->type = new TypeDecl(*wrapper->result);
        w->arguments.push_back(inner);
        auto fakeContext = new ExprFakeContext(inner->at);
        fakeContext->generated = true;
        fakeContext->type = new TypeDecl(Type::fakeContext);
        w->arguments.push_back(fakeContext);
        auto fakeLineInfo = new ExprFakeLineInfo(inner->at);
        fakeLineInfo->generated = true;
        fakeLineInfo->type = new TypeDecl(Type::fakeLineInfo);
        w->arguments.push_back(fakeLineInfo);
        return w;
    }

    // does this subtree CALL INTO code that may hit a queue site? Lexical sites are handled
    // by inhibition/scanning; this is the interprocedural half - a call to a may-queue das
    // function, an invoke (unknown target), or an invoke-carrying extern (runs lambdas we
    // cannot see). A parked temp must not be live across any of these.
    class CallsIntoQueueSite : public Visitor {
    public:
        bool found = false;
        virtual void preVisit ( ExprInvoke * expr ) override {
            Visitor::preVisit(expr);
            found = true;
        }
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            auto f = expr->func;
            // a das function the side-effect pass never computed reads as all-clear - treat as danger
            if ( !f || f->mayQueueTempString || (f->builtIn ? f->invoke : !f->knownSideEffects) ) found = true;
        }
    };

    static bool callsIntoQueueSite ( Expression * e ) {
        CallsIntoQueueSite cq;
        e->visit(cq);
        return cq.found;
    }

    class MarkTempStrings : public Visitor {
    public:
        MarkTempStrings ( Function * wrapperFn, bool insertWrappers_, bool everything_ )
            : wrapper(wrapperFn), insertWrappers(insertWrappers_), isEverything(everything_) {}
    protected:
        Function *  wrapper = nullptr;
        bool        insertWrappers = false;
        bool        isEverything = false;
        int32_t     inhibit = 0;
        // same gates as every pass in this file: templates are uninstantiated (never mutate),
        // argument/field inits were cloned to their use sites at infer, quotes are inert AST
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * var ) override { return isEverything || var->used; }
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->isTemplate && !fun->stub && (isEverything || fun->used);
        }
        das_hash_map<ExprCall *, Expression *> siteOf;
        bool isWrapperCall ( Expression * e ) const {
            return wrapper && e->rtti_isCall() && static_cast<ExprCall *>(e)->func == wrapper;
        }
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            auto efun = expr->func;
            if ( !efun || efun==wrapper ) return;
            if ( inhibit ) return;
            // BBATKIN: if captureString side effects are not calculated correctly, this will blow up!!!
            if ( efun->policyBased || efun->invoke || efun->captureString ) return;
            if ( efun->mayQueueTempString ) return;     // the callee's own body could flush the parked temp while still reading it
            if ( !efun->builtIn && !efun->knownSideEffects ) return;    // uncomputed das function - its flags cannot be trusted
            for ( int ai=int(expr->arguments.size())-1; ai>=0; ai-- ) {
                auto & arg = expr->arguments[ai];
                bool eligible = arg->rtti_isStringBuilder() || isWrapperCall(arg)
                    || (insertWrappers && isFreshStringCall(arg));
                if ( !eligible ) continue;
                // a SIBLING calling into may-queue code flushes this site while it is live
                // (argument evaluation order is unspecified) - then no site at all
                bool siblingDanger = false;
                for ( int aj=int(expr->arguments.size())-1; aj>=0 && !siblingDanger; aj-- ) {
                    if ( aj!=ai && callsIntoQueueSite(expr->arguments[aj]) ) siblingDanger = true;
                }
                if ( siblingDanger ) break;
                if ( arg->rtti_isStringBuilder() ) {
                    static_cast<ExprStringBuilder *>(arg)->isTempString = true;
                    siteOf[expr] = arg;
                } else if ( isWrapperCall(arg) ) {          // already wrapped - a re-run on the same tree
                    siteOf[expr] = arg;
                } else {
                    auto w = makeTempStringWrapper(wrapper, arg);
                    arg = w;
                    siteOf[expr] = w;
                }
                break;
            }
        }
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            Visitor::preVisitCallArg(call, arg, last);
            auto it = siteOf.find(call);
            if ( it!=siteOf.end() && it->second!=arg ) inhibit ++;
        }
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            auto it = siteOf.find(call);
            if ( it!=siteOf.end() && it->second!=arg ) inhibit --;
            return Visitor::visitCallArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            siteOf.erase(expr);
            return Visitor::visit(expr);
        }
    };

    // counts references to one variable inside a statement, and how many are SAFE:
    // a safe reference is a plain read that is the DIRECT argument of a non-capturing,
    // non-invoke, non-policy call - what a nested call returns is a different value, so
    // only the immediate consumer of the reference matters. any reference inside a block
    // literal, and any other shape (return, store, addr, operator operand), stays unsafe
    class VarUseClassifier : public Visitor {
    public:
        VarUseClassifier ( Variable * v ) : var(v) {}
        uint32_t total = 0;
        uint32_t safe = 0;
    protected:
        Variable *  var = nullptr;
        int32_t     blockDepth = 0;
        static Expression * peelR2V ( Expression * e ) {
            return e->rtti_isR2V() ? static_cast<ExprRef2Value *>(e)->subexpr : e;
        }
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->variable==var ) total ++;
        }
        virtual void preVisit ( ExprMakeBlock * expr ) override {
            Visitor::preVisit(expr);
            blockDepth ++;
        }
        virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override {
            blockDepth --;
            return Visitor::visit(expr);
        }
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            Visitor::preVisitCallArg(call, arg, last);
            if ( blockDepth ) return;
            auto f = call->func;
            if ( !f || f->captureString || f->invoke || f->policyBased ) return;
            if ( f->mayQueueTempString ) return;    // its body could flush the parked temp mid-read
            if ( !f->builtIn && !f->knownSideEffects ) return;  // uncomputed das function - flags untrusted
            auto barg = peelR2V(arg);
            if ( barg->rtti_isVar() && static_cast<ExprVar *>(barg)->variable==var ) safe ++;
        }
    };

    class HasQueueSite : public Visitor {
    public:
        HasQueueSite ( Function * w ) : wrapper(w) {}
        bool found = false;
    protected:
        Function * wrapper = nullptr;
        virtual void preVisit ( ExprStringBuilder * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->isTempString ) found = true;
        }
        virtual void preVisit ( ExprInvoke * expr ) override {
            Visitor::preVisit(expr);
            found = true;       // unknown target - may queue
        }
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            auto f = expr->func;
            // lexical sites (the wrapper) plus the interprocedural half: calls whose bodies
            // may queue. A plain [temp_string_result] call is NOT a site by itself. An
            // uncomputed das function (no knownSideEffects) reads as all-clear - treat as a site
            if ( !f || f==wrapper || f->mayQueueTempString
                || (f->builtIn ? f->invoke : !f->knownSideEffects) ) found = true;
        }
    };

    // Phase B of temp-string reclaim: the let-local live range. `let s = <flagged call>`
    // wraps its initializer when s provably dies before the next queue site:
    //  - every reference to s is a safe read per VarUseClassifier;
    //  - no statement from the let through the LAST-reference statement contains a queue
    //    site (marked builder or wrapper call). Statements after the last use may queue
    //    freely - s is dead by then. A loop re-executing the let is the same case: the
    //    local's scope ends with the block, so the previous iteration's value is dead.
    // Runs AFTER MarkTempStrings (per-call sites are final); candidates are processed in
    // REVERSE statement order so a wrap here is visible as a site to earlier candidates.
    // Sites inside the initializer's own subtree are fine - that is the chain pattern
    // (each nested temp dies before the next link queues).
    class WrapLetTempStrings : public Visitor {
    public:
        WrapLetTempStrings ( Function * wrapperFn, bool everything_ )
            : wrapper(wrapperFn), isEverything(everything_) {}
    protected:
        Function *  wrapper = nullptr;
        bool        isEverything = false;
        // same gates as MarkTempStrings above - this pass mutates let initializers
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * var ) override { return isEverything || var->used; }
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->isTemplate && !fun->stub && (isEverything || fun->used);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            auto & stmts = block->list;
            for ( int i=int(stmts.size())-1; i>=0; --i ) {
                if ( !stmts[i]->rtti_isLet() ) continue;
                auto elet = static_cast<ExprLet *>(stmts[i]);
                if ( elet->variables.size()!=1 ) continue;
                auto & var = elet->variables[0];
                if ( !var->init || !var->type || !var->type->isString() || var->type->ref ) continue;
                // a builder initializer self-queues when marked - no wrapper call needed;
                // an already-marked builder or an existing wrapper fails both tests - idempotent
                bool builderInit = var->init->rtti_isStringBuilder()
                    && !static_cast<ExprStringBuilder *>(var->init)->isTempString;
                if ( !builderInit && !isFreshStringCall(var->init) ) continue;
                int lastUse = -1;
                bool ok = true;
                for ( size_t j=i+1; j<stmts.size() && ok; ++j ) {
                    VarUseClassifier uc(var);
                    stmts[j]->visit(uc);
                    if ( uc.total ) {
                        if ( uc.total!=uc.safe ) ok = false;
                        else lastUse = int(j);
                    }
                }
                if ( ok ) {
                    for ( auto & fs : block->finalList ) {          // finally is outside the range model
                        VarUseClassifier uc(var);
                        fs->visit(uc);
                        if ( uc.total ) { ok = false; break; }
                    }
                }
                if ( !ok || lastUse<0 ) continue;
                for ( int j=i+1; j<=lastUse && ok; ++j ) {
                    HasQueueSite qs(wrapper);
                    stmts[j]->visit(qs);
                    if ( qs.found ) ok = false;
                }
                if ( !ok ) continue;
                if ( builderInit ) {
                    static_cast<ExprStringBuilder *>(var->init)->isTempString = true;
                } else {
                    var->init = makeTempStringWrapper(wrapper, var->init);
                }
            }
            return Visitor::visit(block);
        }
    };

    // program

    void Program::allocateStack(TextWriter & logs, bool permanent, bool everything) {
        // temp-string sites: builder marking + [temp_string_result] wrapping (must precede AllocateStack).
        // ALWAYS wrap, regardless of heap options: this pass mutates function bodies (shared-module
        // ASTs included), so gating it on the driving program's options makes a function's tree — and
        // its AOT hash — depend on who compiled it first (macro-context compiles run with
        // macro_context_persistent_heap and wrapped shared daslib functions that AOT stub generation
        // left bare → error 50101 on link). The heap modes are handled at runtime instead:
        // freeTempString no-ops for interned heaps and when reclaim is disabled, and linear-heap
        // frees are safe bump-retreat no-ops.
        {
            Function * wrapperFn = nullptr;
            if ( auto bmod = Module::require("$") ) {
                wrapperFn = bmod->findUniqueFunction("_temp_string_result");
            }
            MarkTempStrings mts(wrapperFn, wrapperFn!=nullptr, everything);
            visit(mts);
            if ( wrapperFn ) {
                WrapLetTempStrings wlt(wrapperFn, everything);
                visit(wlt);
            }
        }
        // string heap
        AllocateConstString vstr;
        for (auto & pm : library.modules) {
            for ( auto & var : pm->globals.each() ) {
                if (var->used && var->init) {
                    var->init->visit(vstr);
                }
            }
            for ( auto & func : pm->functions.each() ) {
                if (func->used) {
                    func->visit(vstr);
                }
            }
        }
        globalStringHeapSize = vstr.bytesTotal;
        // move some variables to CMRES
        VarCMRes vcm(this, everything);
        visit(vcm);
        // allocate stack for the rest of them
        AllocateStack context(this, permanent, everything, logs);
        visit(context);
        // adjust stack size for all the used variables
        for (auto & pm : library.modules) {
            for ( auto & var : pm->globals.each() ) {
                if ( var->used ) {
                    globalInitStackSize = das::max(globalInitStackSize, var->initStackSize);
                }
            }
        }
        // allocate used variables and functions indices
        totalVariables = 0;
        totalFunctions = 0;
        auto log = options.getBoolOption("log_stack");
        if ( log ) {
            logs << "INIT STACK SIZE:\t" << globalInitStackSize << "\n";
            logs << "FUNCTION TABLE:\n";
        }
        for (auto & pm : library.modules) {
            for ( auto & func : pm->functions.each() ) {
                if ( func->used && !func->builtIn && !func->isTemplate ) {
                    func->index = totalFunctions++;
                    if ( log ) {
                        logs << "\t" << func->index << "\t" << func->totalStackSize << "\t" << func->getMangledName();
                        if ( func->init ) {
                            logs << " [init";
                            if (func->lateInit ) logs << "(late)";
                            logs << "]";
                        }
                        if ( func->shutdown ) {
                            logs << " [finalize";
                            if (func->lateShutdown ) logs << "(late)";
                            logs << "]";
                        }
                        logs << " // " << HEX << func->getMangledNameHash() << DEC;
                        logs << "\n";
                    }
                }
                else {
                    func->index = -2;
                }
            }
        }
        if ( log ) {
            logs << "VARIABLE TABLE:\n";
        }
        library.foreach_in_order([&](Module * pm){
            for ( auto & var : pm->globals.each() ) {
                if (var->used) {
                    var->index = totalVariables++;
                    if ( log ) {
                        logs << "\t" << var->index << "\t"  << var->stackTop << "\t"
                            << var->type->getSizeOf() << "\t" << var->getMangledName() << "\n";
                    }
                }
                else {
                    var->index = -2;
                }
            }
            return true;
        }, thisModule.get());
    }

    class RenameVariables : public Visitor {
    public:
        virtual void preVisit ( ExprVar * var ) override {
            Visitor::preVisit(var);
            auto it = variables.find(var->variable);
            if ( it != variables.end() ) {
                var->name = it->second;
            }
        }
        virtual void preVisit ( ExprLet * let ) override {
            Visitor::preVisit(let);
            for ( auto & v : let->variables ) {
                auto it = variables.find(v);
                if ( it != variables.end() ) {
                    v->name = it->second;
                }
            }
        }
    public:
        das_hash_map<Variable *, string> variables;
    };

    class RelocatePotentiallyUninitialized : public Visitor {
    protected:
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
        }
        virtual FunctionPtr visit ( Function * that ) override {
            func = nullptr;
            return Visitor::visit(that);
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            scopes.push_back(block);
            onTopOfTheBlock.push_back(vector<ExpressionPtr>());
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            if ( onTopOfTheBlock.back().size() ) {
                RenameVariables ren;
                for ( auto & topE : onTopOfTheBlock.back() ) {
                    auto eLet = static_cast<ExprLet*>(topE);
                    auto vVar = eLet->variables[0];
                    ren.variables[vVar] = vVar->name + "`at`" + to_string(vVar->at.line) + "`" + to_string(vVar->at.column);
                    block->list.insert(block->list.begin(), topE);
                }
                block->visit(ren);
            }
            onTopOfTheBlock.pop_back();
            scopes.pop_back();
            return Visitor::visit(block);
        }
        virtual ExpressionPtr visit ( ExprLet * expr ) override {
            if ( expr->hasEarlyOut ) {
                auto anyNeedRelocate = false;
                for ( auto & var : expr->variables ) {
                    if ( var->used_in_finally ) {
                        anyNeedRelocate = true;
                        break;
                    }
                }
                if ( anyNeedRelocate ) {
                    anyWork = true;
                    if ( func ) func->notInferred();
                    vector<ExpressionPtr> afterThisExpression;
                    for ( auto & var : expr->variables ) {
                        if ( var->init ) {
                            auto left = new ExprVar(expr->at, var->name);
                            left->variable = var;
                            auto right = var->init->clone();
                            ExpressionPtr assign = nullptr;
                            if ( var->init_via_move ) {
                                assign = new ExprMove(expr->at, left, right);
                                ((ExprMove *)assign)->allowConstantLValue = true;
                            } else {
                                assign = new ExprCopy(expr->at, left, right);
                                ((ExprCopy *)assign)->allowConstantLValue = true;
                            }
                            assign->alwaysSafe = true;
                            assign->generated = true;
                            if ( var->type->constant ) {
                                auto exprOp2 = (ExprOp2 *)(assign);
                                auto pLeft = exprOp2->left->clone();
                                auto pLeftType = new TypeDecl(*var->type);
                                pLeftType->constant = false;
                                auto pLeftCast = new ExprCast(exprOp2->left->at, pLeft, pLeftType);
                                pLeftCast->reinterpret = true;
                                pLeftCast->alwaysSafe = true;
                                exprOp2->left = pLeftCast;
                            }
                            afterThisExpression.push_back(assign);
                        }
                    }
                    auto elet = static_cast<ExprLet*>(expr->clone());
                    elet->alwaysSafe = true;
                    elet->variables = das::move(expr->variables);
                    for ( auto & evar : elet->variables ) {
                        evar->init = nullptr;
                    }
                    onTopOfTheBlock.back().push_back(elet);
                    // lets see if we have anything to return
                    if ( afterThisExpression.size()==1 ) {
                        return afterThisExpression.back();
                    } else {
                        auto blk = new ExprBlock();
                        blk->at = expr->at;
                        blk->list.reserve(afterThisExpression.size());
                        for ( auto & ee : afterThisExpression ) {
                            blk->list.push_back(ee);
                        }
                        return blk;
                    }
                }
            }
            return Visitor::visit(expr);
        }
    protected:
        vector<ExprBlock *> scopes;
        vector<vector<ExpressionPtr>> onTopOfTheBlock;
        Function * func = nullptr;
    public:
        bool anyWork = false;
    };

    bool Program::relocatePotentiallyUninitialized(TextWriter & ) {
        RelocatePotentiallyUninitialized vis;
        visit(vis);
        return vis.anyWork;
    }
}

