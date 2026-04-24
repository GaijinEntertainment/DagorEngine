#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"

namespace das {

    class PodVisitor : public Visitor {
    public:
        bool anyWork = false;
        PodVisitor ( TextWriter * logs_ = nullptr ) : logs(logs_) {}
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            if ( fun->fromGeneric && fun->fromGeneric->module->name=="builtin" ) return false;
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual void preVisit ( Function * fun ) override {
            Visitor::preVisit(fun);
            func = fun;
        }
        virtual FunctionPtr visit ( Function * fun ) override {
            func = nullptr;
            return Visitor::visit(fun);
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            blocks.push_back(block);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            blocks.pop_back();
            return Visitor::visit(block);
        }
        virtual VariablePtr visitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            if ( func && var->pod_delete && !var->pod_delete_gen ) {
                anyWork = true;
                var->pod_delete_gen = true;
                if ( var->single_return_via_move ) {
                    // we silently do nothing. because this pod is returned via move, no need to collect it
                    if ( logs ) {
                        *logs << "skipping POD optimization for single_return_via_move variable '" << var->name << "' in function '" << func->module->name << "::" << func->name << "'\n";
                    }
                } else if ( var->consumed ) {
                    // we silently do nothing. because this pod is consumed, no need to collect it
                    if ( logs ) {
                        *logs << "skipping POD optimization for consumed variable '" << var->name << "' in function '" << func->module->name << "::" << func->name << "'\n";
                    }
                } else if (
                           func->generated
                        || func->generator
                        || func->lambda
                        || func->hasTryRecover
                        || func->hasUnsafe
                        || !func->module->allowPodInscope
                        || (func->fromGeneric && !func->fromGeneric->module->allowPodInscope)
                        || blocks.back()->inTheLoop
                    ) {
                    if ( logs ) {
                        if ( !var->at.empty() && var->at.fileInfo ) {
                            *logs << var->at.fileInfo->name << ":" << var->at.line << ":" << var->at.column << " ";
                        }
                        *logs << "warning: POD optimization failed for '" << var->name << "' in function '" << func->module->name << "::" << func->name << "'\n";
                        if ( func->generated ) *logs << "\tfunction is generated\n";
                        if ( func->generator ) *logs << "\tfunction is generator\n";
                        if ( func->lambda ) *logs << "\tfunction is lambda\n";
                        if ( func->hasTryRecover ) *logs << "\tfunction has try-recover\n";
                        if ( func->hasUnsafe ) *logs << "\tfunction has unsafe\n";
                        if ( !func->module->allowPodInscope ) *logs << "\tmodule " << func->module->name << " does not allow in-scope POD\n";
                        if ( func->fromGeneric && !func->fromGeneric->module->allowPodInscope )
                            *logs << "\tgeneric function module " << func->fromGeneric->module->name << " does not allow in-scope POD\n";
                        if ( blocks.back()->inTheLoop ) *logs << "\tblock is in the loop, which does not have separate 'finally' scope. you can add 'if ( true )' to create scope, or take variable outside of loop\n";
                    }
                } else {
                    func->notInferred();
                    auto CallCollectLocal = make_smart<ExprCall>(expr->at,"_::builtin_collect_local_and_zero");
                    CallCollectLocal->arguments.push_back(make_smart<ExprVar>(expr->at, var->name));
                    CallCollectLocal->arguments.push_back(make_smart<ExprConstUInt>(expr->at, var->type->getSizeOf()));
                    CallCollectLocal->alwaysSafe = true;
                    blocks.back()->finalList.push_back(CallCollectLocal);
                    if ( logs ) {
                        if ( !var->at.empty() && var->at.fileInfo ) {
                            *logs << var->at.fileInfo->name << ":" << var->at.line << ":" << var->at.column << " ";
                        }
                        *logs << "In-scope POD applied to variable '" << var->name << "' in function '" << func->module->name << "::" << func->name << "'\n";
                    }
                }
            }
            return Visitor::visitLet(expr,var,last);
        }
        virtual ExpressionPtr visit ( ExprFor * expr ) override {
            if ( expr->generated || !func || func->generated || func->generator || func->lambda || func->hasTryRecover || func->hasUnsafe) {
                return Visitor::visit(expr);
            }
            // so, we check arguments, and if there are any POD with pod_delete, we make temp variables for them (which will cause yet another POD delete)
            // can't be any source - only temp PODS
            vector<int> podSourceIndices;
            for ( size_t i=0; i!=expr->sources.size(); ++i ) {
                auto src = expr->sources[i].get();
                if ( src->type && src->type->isGoodArrayType() && !src->type->constant ) {
                    auto good = false;
                    if ( src->rtti_isMakeLocal() ){                         // its [1,2,3,4]
                        good = true;
                    } else if ( src->rtti_isCall() ) {
                        auto csrc = (ExprCall *) src;
                        if ( csrc->func && !csrc->func->result->ref && !csrc->func->unsafeOutsideOfFor ) {     // its fn() returning array
                            good = true;
                        }
                    } else if ( src->rtti_isInvoke() ) {
                        auto csrc = (ExprInvoke *) src;
                        if ( csrc->type && !csrc->type->ref ) {             // its obj.method() returning array, not ref to array
                            good = true;
                        }
                    }
                    if ( good  ) {
                        podSourceIndices.push_back(int(i));
                    }
                }
            }
            if ( podSourceIndices.size() ) {
                anyWork = true;
                func->notInferred();
                // we make a new block, we make a new variable for each pod source, and we assign it before the for
                auto newBlock = make_smart<ExprBlock>();
                newBlock->at = expr->at;
                newBlock->isCollapseable = true;
                auto letPod = make_smart<ExprLet>();
                letPod->at = expr->at;
                letPod->alwaysSafe = true;  // this is for the array<smart_ptr> and some such
                newBlock->list.push_back(letPod);
                for ( auto i : podSourceIndices ) {
                    auto podVar = make_smart<Variable>();
                    podVar->at = expr->sources[i]->at;
                    podVar->name = "`pod`source`" + expr->iterators[i];
                    podVar->type = make_smart<TypeDecl>(Type::autoinfer);
                    podVar->init = move(expr->sources[i]);
                    podVar->init_via_move = true;
                    podVar->pod_delete = true;
                    podVar->pod_delete_gen = true;
                    letPod->variables.push_back(podVar);
                    expr->sources[i] = make_smart<ExprVar>(podVar->at, podVar->name);
                    // and collect
                    auto CallCollectLocal = make_smart<ExprCall>(expr->at,"_::builtin_collect_local_and_zero");
                    CallCollectLocal->arguments.push_back(make_smart<ExprVar>(expr->at, podVar->name));
                    CallCollectLocal->arguments.push_back(make_smart<ExprConstUInt>(expr->at, expr->iteratorVariables[i]->type->getSizeOf()));
                    CallCollectLocal->alwaysSafe = true;
                    newBlock->finalList.push_back(CallCollectLocal);
                    if ( logs ) {
                        if ( !podVar->at.empty() && podVar->at.fileInfo ) {
                            *logs << podVar->at.fileInfo->name << ":" << podVar->at.line << ":" << podVar->at.column << " ";
                        }
                        *logs << "In-scope POD applied to loop source '" << expr->iterators[i] << "' in function '" << func->module->name << "::" << func->name << "'\n";
                    }
                }
                newBlock->list.push_back(expr);
                return newBlock;
            }
            return Visitor::visit(expr);
        }
    protected:
        Function * func = nullptr;
        TextWriter * logs = nullptr;
        vector<ExprBlock *> blocks;
    };

    bool Program::inScopePodAnalysis(TextWriter & logs) {
        auto forceInscopePod = options.getBoolOption("force_inscope_pod", policies.force_inscope_pod);
        if ( !forceInscopePod ) return false;
        auto logInscopePod = options.getBoolOption("log_inscope_pod", policies.log_inscope_pod);
        PodVisitor pv(logInscopePod ? &logs : nullptr);
        visit(pv);
        return pv.anyWork;
    }

}
