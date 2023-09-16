#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class VarCMRes : public Visitor {
    public:
        VarCMRes( const ProgramPtr & prog ) {
            program = prog;
        }
    protected:
        vector<ExprBlock *>     blocks;
        ProgramPtr              program;
        FunctionPtr             func;
        VariablePtr             cmresVAR;
        bool                    failedToCMRES = false;
    protected:
    // function
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
        }
        virtual FunctionPtr visit ( Function * that ) override {
            if ( cmresVAR && !failedToCMRES ) cmresVAR->aliasCMRES = true;
            func.reset();
            cmresVAR.reset();
            failedToCMRES = false;
            assert(blocks.size()==0);
            return Visitor::visit(that);
        }
    // ExprBlock
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->isClosure ) {
                blocks.push_back(block);
            }
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
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
            expr->returnFunc = func.get();
            if ( failedToCMRES ) return;
            // we can safely skip makeLocal, invoke, or call
            // they are going to CMRES directly, and do not affect nothing
            if ( !func->result->isRefType() ) return;
            if ( expr->subexpr->rtti_isCall() ) return;
            if ( expr->subexpr->rtti_isInvoke() ) return;
            if ( expr->subexpr->rtti_isMakeLocal() ) return;
            // if its return X
            if ( expr->subexpr->rtti_isVar() ) {
                auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                if ( !evar->local ) return; // if its not local, we safely ignore
                auto var = evar->variable;
                if ( var->type->ref ) {     // we return local ref variable, so ... game over
                    failedToCMRES = true;
                } else if ( !cmresVAR ) {
                    cmresVAR = var;
                } else if ( cmresVAR!=var ) {
                    // TODO:    verify if we need to fail?
                    failedToCMRES = true;
                }
            }
        }
    };

    class AllocateStack : public Visitor {
    public:
        AllocateStack( const ProgramPtr & prog, TextWriter & ls ) : logs(ls) {
            program = prog;
            log = prog->options.getBoolOption("log_stack");
            log_var_scope = prog->options.getBoolOption("log_var_scope");
            optimize = prog->getOptimize();
            if( log ) {
                logs << "\nSTACK INFORMATION in " << prog->thisModule->name << ":\n";
            }
        }
    protected:
        ProgramPtr              program;
        FunctionPtr             func;
        uint32_t                stackTop = 0;
        vector<uint32_t>        stackTopStack;
        vector<ExprBlock *>     blocks;
        vector<ExprBlock *>     scopes;
        bool                    log = false;
        bool                    log_var_scope = false;
        bool                    optimize = false;
        TextWriter &            logs;
        bool                    inStruct = false;
    protected:
        uint32_t allocateStack ( uint32_t size ) {
            auto result = stackTop;
            stackTop += (size + 0xf) & ~0xf;
            return result;
        }
    // structure
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
            if ( var->init  ) {
                if ( var->init->rtti_isMakeLocal() ) {
                    uint32_t sz = sizeof(void *);
                    uint32_t refStackTop = allocateStack(sz);
                    if ( log ) {
                        logs << "\t" << refStackTop << "\t" << sz
                            << "\tinit global " << var->name << " [[ ]], line " << var->init->at.line << "\n";
                    }
                    auto mkl = static_pointer_cast<ExprMakeLocal>(var->init);
                    mkl->setRefSp(true, false, refStackTop, 0);
                    mkl->doesNotNeedInit = false;
                    mkl->doesNotNeedSp = true;
                } else if ( var->init->rtti_isCall() ) {
                    auto cll = static_pointer_cast<ExprCall>(var->init);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    }
                } else if ( var->init->rtti_isInvoke() ) {
                    auto cll = static_pointer_cast<ExprInvoke>(var->init);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                    }
                }
            }
        }
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override {
            var->initStackSize = stackTop;
            program->globalInitStackSize = das::max(program->globalInitStackSize, stackTop);
            return Visitor::visitGlobalLet(var);
        }
    // function
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
            func->totalStackSize = stackTop = sizeof(Prologue);
            if ( log ) {
                if (!func->used) logs << "unused ";
                logs << func->describe() << "\n";
            }
        }
        virtual FunctionPtr visit ( Function * that ) override {
            func->totalStackSize = das::max(func->totalStackSize, stackTop);
            // detecting fastcall
            if ( !program->getDebugger() && !program->getProfiler() ) {
                if ( !func->exports && !func->addr && func->totalStackSize==sizeof(Prologue) && func->arguments.size()<=32 ) {
                    if (func->body->rtti_isBlock()) {
                        auto block = static_pointer_cast<ExprBlock>(func->body);
                        if ( func->result->isWorkhorseType() ) {
                            if (block->list.size()==1 && block->finalList.size()==0 && block->list.back()->rtti_isReturn()) {
                                func->fastCall = true;
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
            func.reset();
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
                if ( expr->subexpr->rtti_isMakeLocal() ) {
                    uint32_t sz = sizeof(void *);
                    expr->refStackTop = allocateStack(sz);
                    expr->takeOverRightStack = true;
                    if ( log ) {
                        logs << "\t" << expr->refStackTop << "\t" << sz
                        << "\treturn ref and eval, line " << expr->at.line << "\n";
                    }
                    auto mkl = static_pointer_cast<ExprMakeLocal>(expr->subexpr);
                    if ( expr->returnInBlock ) {
                        mkl->setRefSp(true, false, expr->refStackTop, 0);
                    } else {
                        mkl->setRefSp(true, true, expr->refStackTop, 0);
                        expr->returnCMRES = true;
                    }
                    mkl->doesNotNeedInit = false;
                } else if ( expr->subexpr->rtti_isCall() ) {
                    auto cll = static_pointer_cast<ExprCall>(expr->subexpr);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                        expr->returnCallCMRES = true;
                    }
                } else if ( expr->subexpr->rtti_isInvoke() ) {
                    auto cll = static_pointer_cast<ExprInvoke>(expr->subexpr);
                    if ( cll->allowCmresSkip() ) {
                        cll->doesNotNeedSp = true;
                        expr->returnCallCMRES = true;
                    }
                } else if ( expr->subexpr->rtti_isVar() ) {
                    auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                    if ( evar->variable->aliasCMRES ) {
                        expr->returnCMRES = true;
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
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            block->stackVarBottom = allocateStack(0);
            if ( inStruct ) {
                return Visitor::visit(block);
            }
            scopes.pop_back();
            if ( block->isClosure ) {
                blocks.pop_back();
            }
            return Visitor::visit(block);
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
        }
    // ExprCall
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
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
                    auto mkl = static_pointer_cast<ExprMakeLocal>(src);
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
                    auto mkl = static_pointer_cast<ExprMakeLocal>(var->init);
                    mkl->setRefSp(false, var->aliasCMRES, var->stackTop, 0);
                    mkl->doesNotNeedInit = false;
                } else if ( var->init->rtti_isCall() ) {
                    auto cll = static_pointer_cast<ExprCall>(var->init);
                    if ( (cll->func->copyOnReturn || cll->func->moveOnReturn) && !cll->func->aliasCMRES ) { // note: let never aliases!!!
                        cll->doesNotNeedSp = true;
                    }
                } else if ( var->init->rtti_isInvoke() ) {
                    auto cll = static_pointer_cast<ExprInvoke>(var->init);
                    if ( cll->isCopyOrMove() ) {
                        cll->doesNotNeedSp = true;
                    }
                }
            }
        }
    // ExprMakeBlock
        virtual void preVisit ( ExprMakeBlock * expr ) override {
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
        }
    // ExprAscend
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->subexpr->rtti_isMakeLocal() ) {
                uint32_t sz = sizeof(void *);
                expr->stackTop = allocateStack(sz);
                expr->useStackRef = true;
                if ( log ) {
                    logs << "\t" << expr->stackTop << "\t" << sz
                    << "\tascend, line " << expr->at.line << "\n";
                }
                auto mkl = static_pointer_cast<ExprMakeLocal>(expr->subexpr);
                mkl->setRefSp(true, false, expr->stackTop, 0);
            }
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
                expr->setRefSp(false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
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
                expr->setRefSp(false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
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
                expr->setRefSp(false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
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
                expr->setRefSp(false, false, cStackTop, 0);
                expr->doesNotNeedSp = false;
                expr->doesNotNeedInit = false;
            }
        }
    // New
        virtual void preVisit ( ExprNew * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->type->dim.size() ) {
                auto sz = uint32_t(expr->type->getCountOf()*sizeof(char *));
                expr->stackTop = allocateStack(sz);
                if ( log ) {
                    logs << "\t" << expr->stackTop << "\t" << sz
                    << "\tNEW " << expr->typeexpr->describe() << ", line " << expr->at.line << "\n";
                }
            }
        }
    // ExprCopy
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->right->rtti_isMakeLocal() ) {
                auto mkl = static_pointer_cast<ExprMakeLocal>(expr->right);
                if ( !mkl->alwaysAlias ) {
                    uint32_t sz = sizeof(void *);
                    expr->stackTop = allocateStack(sz);
                    expr->takeOverRightStack = true;
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                            << "\tcopy [[ ]], line " << expr->at.line << "\n";
                    }
                    mkl->setRefSp(true, false, expr->stackTop, 0);
                    mkl->doesNotNeedInit = false;
                }
            } else if ( expr->right->rtti_isCall() ) {
                auto cll = static_pointer_cast<ExprCall>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( expr->right->rtti_isInvoke() ) {
                auto cll = static_pointer_cast<ExprInvoke>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
        }
    // ExprMove
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            if ( inStruct ) return;
            if ( expr->right->rtti_isMakeLocal() ) {
                auto mkl = static_pointer_cast<ExprMakeLocal>(expr->right);
                if ( !mkl->alwaysAlias ) {
                    uint32_t sz = sizeof(void *);
                    expr->stackTop = allocateStack(sz);
                    expr->takeOverRightStack = true;
                    if ( log ) {
                        logs << "\t" << expr->stackTop << "\t" << sz
                            << "\tcopy [[ ]], line " << expr->at.line << "\n";
                    }
                    mkl->setRefSp(true, false, expr->stackTop, 0);
                    mkl->doesNotNeedInit = false;
                }
            } else if ( expr->right->rtti_isCall() ) {
                auto cll = static_pointer_cast<ExprCall>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            } else if ( expr->right->rtti_isInvoke() ) {
                auto cll = static_pointer_cast<ExprInvoke>(expr->right);
                if ( cll->allowCmresSkip() ) {
                    cll->doesNotNeedSp = true;
                }
            }
        }
    };

    class AllocateConstString : public Visitor {
    public:
        uint32_t bytesTotal = 0;
        das_hash_set<string>    uniStr;
    public:
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

    // program

    void Program::allocateStack(TextWriter & logs) {
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
        VarCMRes vcm(this);
        visit(vcm);
        // allocate stack for the rest of them
        AllocateStack context(this, logs);
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
                if ( func->used && !func->builtIn ) {
                    func->index = totalFunctions++;
                    if ( log ) {
                        logs << "\t" << func->index << "\t" << func->totalStackSize << "\t" << func->getMangledName();
                        if ( func->init ) {
                            logs << " [init";
                            if (func->lateInit ) logs << "(late)";
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
            auto it = variables.find(var->variable.get());
            if ( it != variables.end() ) {
                var->name = it->second;
            }
        }
        virtual void preVisit ( ExprLet * let ) override {
            Visitor::preVisit(let);
            for ( auto & v : let->variables ) {
                auto it = variables.find(v.get());
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
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            scopes.push_back(block);
            onTopOfTheBlock.push_back(vector<ExpressionPtr>());
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            if ( onTopOfTheBlock.back().size() ) {
                RenameVariables ren;
                for ( auto & topE : onTopOfTheBlock.back() ) {
                    auto eLet = static_pointer_cast<ExprLet>(topE);
                    auto vVar = eLet->variables[0].get();
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
                    vector<ExpressionPtr> afterThisExpression;
                    for ( auto & var : expr->variables ) {
                        if ( var->init ) {
                            auto left = make_smart<ExprVar>(expr->at, var->name);
                            left->variable = var;
                            auto right = var->init->clone();
                            ExpressionPtr assign;
                            if ( var->init_via_move ) {
                                assign = make_smart<ExprMove>(expr->at, left, right);
                            } else {
                                assign = make_smart<ExprCopy>(expr->at, left, right);
                            }
                            assign->alwaysSafe = true;
                            afterThisExpression.push_back(assign);
                        }
                    }
                    auto elet = static_pointer_cast<ExprLet>(expr->clone());
                    elet->variables = move(expr->variables);
                    for ( auto & evar : elet->variables ) {
                        evar->init = nullptr;
                    }
                    onTopOfTheBlock.back().push_back(elet);
                    // lets see if we have anything to return
                    if ( afterThisExpression.size()==1 ) {
                        return afterThisExpression.back();
                    } else {
                        auto blk = make_smart<ExprBlock>();
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
    public:
        bool anyWork = false;
    };

    bool Program::relocatePotentiallyUninitialized(TextWriter & ) {
        RelocatePotentiallyUninitialized vis;
        visit(vis);
        return vis.anyWork;
    }
}

