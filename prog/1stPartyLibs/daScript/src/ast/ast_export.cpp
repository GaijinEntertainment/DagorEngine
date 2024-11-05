#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class ClearUnusedSymbols : public Visitor {
    public:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return true; }
        virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override { return false; }
        virtual void preVisit(ExprAddr * expr) override {
            Visitor::preVisit(expr);
            if ( expr->func && !expr->func->used && !expr->func->builtIn ) {
                expr->func = nullptr;
            }
        }
        virtual void preVisitExpression(Expression * expr) override {
            Visitor::preVisitExpression(expr);
            if ( expr->rtti_isCallFunc() ) {
                auto cfe = static_cast<ExprCallFunc *>(expr);
                if ( cfe->func && !cfe->func->used && !cfe->func->builtIn ) {
                    cfe->func = nullptr;
                }
            }
        }
    };

    class MarkSymbolUse : public Visitor {
    public:
        MarkSymbolUse ( bool bid ) : builtInDependencies(bid) {
        }
        __forceinline void push ( const VariablePtr & pvar ) {
            if ( !tw ) return;
            *tw << string(logTab,'\t') << pvar->getMangledName() << "\n";
            logTab ++;
        }
        __forceinline void push ( const FunctionPtr & pfun ) {
            if ( !tw ) return;
            *tw << string(logTab,'\t') << pfun->getMangledName() << "\n";
            logTab ++;
        }
        __forceinline void pop () {
            if ( !tw ) return;
            logTab --;
        }
        void propageteVarUse(const VariablePtr & var) {
            assert(var);
            if (var->used) return;
            push(var);
            var->used = true;
            for (const auto & gv : var->useGlobalVariables) {
                propageteVarUse(gv);
            }
            for (const auto & it : var->useFunctions) {
                propagateFunctionUse(it);
            }
            pop();
        }
        void propagateFunctionUse(const FunctionPtr & fn) {
            assert(fn);
            if (fn->used) return;
            if (fn->builtIn) return;
            push(fn);
            fn->used = true;
            for (const auto & gv : fn->useGlobalVariables) {
                propageteVarUse(gv);
            }
            for (const auto & it : fn->useFunctions) {
                propagateFunctionUse(it);
            }
            pop();
        }
        void markVarsUsed( ModuleLibrary & lib, bool forceAll ){
            lib.foreach([&](Module * pm) {
                for ( auto & var : pm->globals.each() ) {
                    if ( forceAll || var->used ) {
                        var->used = false;
                        propageteVarUse(var);
                    }
                }
                return true;
            }, "*");
        }
        void markUsedFunctions( ModuleLibrary & lib, bool forceAll, bool initThis, Module * macroModule ) {
            lib.foreach([&](Module * pm) {
                if ( initThis && macroModule && pm!=macroModule ) return true;
                for ( auto & fn : pm->functions.each() ) {
                    if ( (forceAll && !fn->macroInit) || fn->exports || fn->init || fn->shutdown || (fn->macroInit && initThis) ) {
                        propagateFunctionUse(fn);
                    }
                }
                return true;
            }, "*");
        }
        void markModuleVarsUsed( ModuleLibrary &, Module * inWhichModule ) {
            for ( auto & var : inWhichModule->globals.each() ) {
                var->used = false;
                propageteVarUse(var);
            }
        }
        void markModuleUsedFunctions( ModuleLibrary &, Module * inWhichModule ) {
            for ( auto & fn : inWhichModule->functions.each() ) {
                if ( fn->builtIn || fn->macroInit || fn->macroFunction  ) continue;
                if ( fn->privateFunction && fn->generated && fn->fromGeneric ) continue;    // instances of templates are never roots
                if ( fn->isClassMethod && fn->classParent->macroInterface ) continue;       // methods of macro interfaces
                propagateFunctionUse(fn);
            }
        }
        void RemoveUnusedSymbols ( Module & mod ) {
            auto functions = das::move(mod.functions);
            auto globals = das::move(mod.globals);
            mod.functionsByName.clear();
            // mod.globals.clear();
            for ( auto & fn : functions.each() ) {
                if ( fn->used ) {
                    if ( !mod.addFunction(fn, true) ) {
                        program->error("internal error, failed to add function " + fn->name,"","", fn->at );
                    }
                }
            }
            for ( auto & var : globals.each() ) {
                if ( var->used ) {
                    if ( !mod.addVariable(var, true) ) {
                        program->error("internal error, failed to add variable " + var->name,"","", var->at );
                    }
                }
            }
        }
    protected:
        ProgramPtr  program;
        FunctionPtr func;
        Variable *  gVar = nullptr;
        bool        builtInDependencies;
        int         logTab = 0;
    public:
        TextWriter * tw = nullptr;
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override { return false; }
        // global variable declaration
        virtual void preVisitGlobalLet(const VariablePtr & var) override {
            Visitor::preVisitGlobalLet(var);
            gVar = var.get();
            var->useFunctions.clear();
            var->useGlobalVariables.clear();
            var->used = false;
        }
        virtual VariablePtr visitGlobalLet(const VariablePtr & var) override {
            gVar = nullptr;
            return Visitor::visitGlobalLet(var);
        }
        // function
        virtual void preVisit(Function * f) override {
            Visitor::preVisit(f);
            func = f;
            func->useFunctions.clear();
            func->useGlobalVariables.clear();
            func->used = false;
            func->callCaptureString = false;
            func->hasStringBuilder = false;
            DAS_ASSERTF(!func->builtIn, "visitor should never call 'visit' on builtin function at top level.");
        }
        virtual FunctionPtr visit(Function * that) override {
            func.reset();
            return Visitor::visit(that);
        }
        // string builder
        virtual void preVisit ( ExprStringBuilder * expr ) override {
            Visitor::preVisit(expr);
            if (func) func->hasStringBuilder = true;
        }
        // variable
        virtual void preVisit(ExprVar * expr) override {
            Visitor::preVisit(expr);
            if (!expr->variable) return;
            if (!expr->local && !expr->argument && !expr->block) {
                if (func) {
                    func->useGlobalVariables.insert(expr->variable.get());
                } else if (gVar) {
                    gVar->useGlobalVariables.insert(expr->variable.get());
                }
            }
        }
        // function address
        virtual void preVisit(ExprAddr * addr) override {
            Visitor::preVisit(addr);
            if (builtInDependencies || (addr->func && !addr->func->builtIn)) {
                assert(addr->func);
                if (func) {
                    func->useFunctions.insert(addr->func);
                } else if (gVar) {
                    gVar->useFunctions.insert(addr->func);
                }
            }
        }
        // function call
        virtual void preVisit(ExprCall * call) override {
            Visitor::preVisit(call);
            if ( !call->func ) return;
            if ( func && call->func->captureString ) {
                func->callCaptureString = true;
            }
            if (builtInDependencies || !call->func->builtIn) {
                if (func) {
                    func->useFunctions.insert(call->func);
                } else if (gVar) {
                    gVar->useFunctions.insert(call->func);
                }
            }
        }
        // new
        virtual void preVisit(ExprNew * call) override {
            Visitor::preVisit(call);
            if ( call->initializer ) {
                if (builtInDependencies || !call->func->builtIn) {
                    assert(call->func);
                    if (func) {
                        func->useFunctions.insert(call->func);
                    } else if (gVar) {
                        gVar->useFunctions.insert(call->func);
                    }
                }
            }
        }
        virtual void preVisit(ExprMakeStruct * call) override {
            Visitor::preVisit(call);
            if ( call->constructor ) {
                if (func) {
                    func->useFunctions.insert(call->constructor);
                } else if (gVar) {
                    gVar->useFunctions.insert(call->constructor);
                }
            }
        }
        // Op1
        virtual void preVisit(ExprOp1 * expr) override {
            Visitor::preVisit(expr);
            if (builtInDependencies || !expr->func->builtIn) {
                assert(expr->func);
                if (func) {
                    func->useFunctions.insert(expr->func);
                } else if (gVar) {
                    gVar->useFunctions.insert(expr->func);
                }
            }
        }
        // Op2
        virtual void preVisit(ExprOp2 * expr) override {
            Visitor::preVisit(expr);
            if (builtInDependencies || !expr->func->builtIn) {
                assert(expr->func);
                if (func) {
                    func->useFunctions.insert(expr->func);
                } else if (gVar) {
                    gVar->useFunctions.insert(expr->func);
                }
            }
        }
        // Op3
        virtual void preVisit(ExprOp3 * expr) override {
            Visitor::preVisit(expr);
            if ( expr->func && (builtInDependencies || !expr->func->builtIn) ) {
                assert(expr->func);
                if (func) {
                    func->useFunctions.insert(expr->func);
                } else if (gVar) {
                    gVar->useFunctions.insert(expr->func);
                }
            }
        }
    };

    void Program::clearSymbolUse() {
        for (auto & pm : library.modules) {
            for ( auto & var : pm->globals.each() ) {
                var->used = false;
            }
            for ( auto & fn : pm->functions.each() ) {
                fn->used = false;
            }
        }
    }

    void Program::markModuleSymbolUse(TextWriter * logs) {
        // this module public, this module export, this module init\shutdown
        clearSymbolUse();
        MarkSymbolUse vis(false);
        vis.tw = logs;
        visit(vis);
        vis.markModuleUsedFunctions(library, thisModule.get());
        vis.markModuleVarsUsed(library, thisModule.get());
    }

    void Program::markMacroSymbolUse(TextWriter * logs) {
        // this module macro init
        clearSymbolUse();
        MarkSymbolUse vis(false);
        vis.tw = logs;
        visit(vis);
        vis.markUsedFunctions(library, false, true, thisModule.get());
        vis.markVarsUsed(library, false);
    }

    void Program::markExecutableSymbolUse(TextWriter * logs) {
        clearSymbolUse();
        MarkSymbolUse vis(false);
        vis.tw = logs;
        visit(vis);
        vis.markUsedFunctions(library, false, false, nullptr);
        vis.markVarsUsed(library, false);
    }

    void Program::markFoldingSymbolUse(const vector<Function *> & needRun, TextWriter * logs) {
        clearSymbolUse();
        MarkSymbolUse vis(false);
        vis.tw = logs;
        visit(vis);
        for ( auto fun : needRun ) {
            vis.propagateFunctionUse(fun);
        }
    }

    void Program::markSymbolUse(bool builtInSym, bool forceAll, bool initThis, Module * macroModule, TextWriter * logs) {
        clearSymbolUse();
        MarkSymbolUse vis(builtInSym);
        vis.tw = logs;
        visit(vis);
        vis.markUsedFunctions(library, forceAll, initThis, macroModule);
        vis.markVarsUsed(library, forceAll);
    }

    void Program::removeUnusedSymbols() {
        if ( options.getBoolOption("remove_unused_symbols",true) ) {
            ClearUnusedSymbols cvis;
            visit(cvis);
            MarkSymbolUse vis(false);
            vis.RemoveUnusedSymbols(*thisModule);
        }
    }

    void Program::dumpSymbolUse(TextWriter & logs) {
        logs << "USED SYMBOLS ARE:\n";
        for (auto & pm : library.modules) {
            for ( auto & var : pm->globals.each() ) {
                if ( var->used ) {
                    logs << "let " << var->module->name << "::" << var->name << ": " << var->type->describe() << "\n";
                }
            }
            for ( auto & func : pm->functions.each() ) {
                if ( func->used  ) {
                    logs << func->module->name << "::" << func->describe() << "\n";
                }
            }
        }
        logs << "\n\n";
    }
}
