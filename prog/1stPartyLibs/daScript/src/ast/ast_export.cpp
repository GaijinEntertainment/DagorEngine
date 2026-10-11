#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class ClearUnusedSymbols : public Visitor {
    public:
        ClearUnusedSymbols ( Program * prog, Module * tm ) : program(prog), thisModule(tm) {}
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return true; }
        // default-argument initializers are the same dangling-ref story as field inits:
        // MarkSymbolUse never visits them (canVisitArgumentInit is false there), so a
        // function/global whose ONLY anchor is `def f(x = helper())` is never marked used,
        // removeUnusedSymbols frees it, and the surviving f keeps the raw pointer in
        // arg->init - serialization then reads it (SIGSEGV in getMangledName under --ser).
        // infer already inlined every default into its call sites, so nulling the husk here
        // costs nothing
        virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override { return true; }
        virtual void preVisit(ExprAddr * expr) override {
            Visitor::preVisit(expr);
            if ( expr->func && !program->isUsed(expr->func) && !expr->func->builtIn ) {
                expr->func = nullptr;
            }
        }
        // `WithInit()` in a struct field initializer infers to ExprMakeStruct, whose
        // constructor ref is NOT an ExprCallFunc - left unnulled, removeUnusedSymbols frees
        // the ctor while the surviving structure's field-init expression keeps the raw
        // pointer, and AST serialization reads a dangling Function* (nondeterministic
        // SIGSEGV in getMangledName under --ser)
        virtual void preVisit(ExprMakeStruct * expr) override {
            Visitor::preVisit(expr);
            if ( expr->constructor && !program->isUsed(expr->constructor) && !expr->constructor->builtIn ) {
                expr->constructor = nullptr;
            }
        }
        // same dangling-pointer story for a global referenced only from a field initializer
        // (`s : int = g_seed`). only this module's globals get freed (RemoveUnusedSymbols
        // runs on thisModule alone), so cross-module refs - e.g. folded builtin constants
        // like math::PI - stay untouched and keep their valid metadata
        virtual void preVisit(ExprVar * expr) override {
            Visitor::preVisit(expr);
            if ( expr->variable && expr->variable->module == thisModule && !program->isUsed(expr->variable) ) {
                expr->variable = nullptr;
            }
        }
        virtual void preVisitExpression(Expression * expr) override {
            Visitor::preVisitExpression(expr);
            if ( expr->rtti_isCallFunc() ) {
                auto cfe = static_cast<ExprCallFunc *>(expr);
                if ( cfe->func && !program->isUsed(cfe->func) && !cfe->func->builtIn ) {
                    cfe->func = nullptr;
                }
            }
        }
    protected:
        Program * program = nullptr;
        Module * thisModule = nullptr;
    };

    class MarkSymbolUse : public Visitor {
    public:
        MarkSymbolUse ( Program * prog, bool bid ) : program(prog), builtInDependencies(bid) {
        }
        __forceinline void push ( Variable* pvar ) {
            if ( !tw ) return;
            *tw << string(logTab,'\t') << pvar->getMangledName() << "\n";
            logTab ++;
        }
        __forceinline void push ( Function* pfun ) {
            if ( !tw ) return;
            *tw << string(logTab,'\t') << pfun->getMangledName() << "\n";
            logTab ++;
        }
        __forceinline void pop () {
            if ( !tw ) return;
            logTab --;
        }
        void propageteVarUse(Variable * var) {
            DAS_ASSERT(var);
            if (program->isUsed(var)) return;
            push(var);
            program->setUsed(var, true);
            for (const auto & gv : var->useGlobalVariables) {
                propageteVarUse(gv);
            }
            for (const auto & it : var->useFunctions) {
                propagateFunctionUse(it);
            }
            pop();
        }
        void propagateFunctionUse(Function * fn) {
            DAS_ASSERT(fn);
            if (fn->isTemplate) return;
            if (program->isUsed(fn)) return;
            if (fn->builtIn) return;
            push(fn);
            program->setUsed(fn, true);
            for (const auto & gv : fn->useGlobalVariables) {
                propageteVarUse(gv);
            }
            for (const auto & it : fn->useFunctions) {
                propagateFunctionUse(it);
            }
            pop();
        }
        bool isVarExported ( const VariablePtr & var ) const {
            return var->annotation.getBoolOption("export",false);
        }
        void markVarsUsed( ModuleLibrary & lib, bool forceAll ){
            lib.foreach([&](Module * pm) {
                for ( auto & var : pm->globals.each() ) {
                    if ( forceAll || program->isUsed(var) || isVarExported(var) ) {
                        program->setUsed(var, false);
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
        void exportPublicFunctions( Module * thisModule ) {
            for ( auto & fn : thisModule->functions.each() ) {
                if ( fn->privateFunction || fn->builtIn || fn->generated || fn->isTemplate ) continue;
                if ( fn->macroInit || fn->macroFunction || fn->init || fn->shutdown ) continue;
                if ( fn->isClassMethod || fn->lambda || fn->generator || fn->fromGeneric ) continue;
                fn->exports = true;
            }
        }
        void markModuleVarsUsed( ModuleLibrary &, Module * inWhichModule ) {
            for ( auto & var : inWhichModule->globals.each() ) {
                program->setUsed(var, false);
                propageteVarUse(var);
            }
        }
        void markModuleUsedFunctions( ModuleLibrary &, Module * inWhichModule ) {
            for ( auto & fn : inWhichModule->functions.each() ) {
                if ( fn->isTemplate ) continue;
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
                if ( program->isUsed(fn) ) {
                    if ( !mod.addFunction(fn, true) ) {
                        program->error("internal error, failed to add function " + fn->name,"","", fn->at, CompilationError::internal_function );
                    }
                }
            }
            for ( auto & var : globals.each() ) {
                if ( program->isUsed(var) ) {
                    if ( !mod.addVariable(var, true) ) {
                        program->error("internal error, failed to add variable " + var->name,"","", var->at, CompilationError::internal_variable );
                    }
                }
            }
        }
    protected:
        ProgramPtr  program;
        FunctionPtr func = nullptr;
        Variable *  gVar = nullptr;
        bool        builtInDependencies;
        int         logTab = 0;
    public:
        TextWriter * tw = nullptr;
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructure ( Structure * st ) override {
            return !st->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override { return false; }
        // global variable declaration
        virtual void preVisitGlobalLet(const VariablePtr & var) override {
            Visitor::preVisitGlobalLet(var);
            gVar = var;
            var->useFunctions.clear();
            var->useGlobalVariables.clear();
            program->setUsed(var, false);
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
            program->setUsed(f, false);
            func->callCaptureString = false;
            func->hasStringBuilder = false;
            DAS_ASSERTF(!func->builtIn, "visitor should never call 'visit' on builtin function at top level.");
        }
        virtual FunctionPtr visit(Function * that) override {
            func = nullptr;
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
                    func->useGlobalVariables.insert(expr->variable);
                } else if (gVar) {
                    gVar->useGlobalVariables.insert(expr->variable);
                }
            }
        }
        // function address
        virtual void preVisit(ExprAddr * addr) override {
            Visitor::preVisit(addr);
            if ( !addr->func ) return;
            if (builtInDependencies || !addr->func->builtIn) {
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
            if ( call->func && call->initializer ) {
                if (builtInDependencies || !call->func->builtIn) {
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
            if ( !expr->func ) return;
            if (builtInDependencies || !expr->func->builtIn) {
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
            if ( !expr->func ) return;
            if (builtInDependencies || !expr->func->builtIn) {
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
            if ( !expr->func ) return;
            if (builtInDependencies || !expr->func->builtIn) {
                if (func) {
                    func->useFunctions.insert(expr->func);
                } else if (gVar) {
                    gVar->useFunctions.insert(expr->func);
                }
            }
        }
    };

    void Program::clearSymbolUse() {
        usedFunctions.clear();
        usedVariables.clear();
    }

    void Program::markModuleSymbolUse(TextWriter * logs) {
        // this module public, this module export, this module init\shutdown
        clearSymbolUse();
        MarkSymbolUse vis(this, false);
        vis.tw = logs;
        visit(vis);
        vis.markModuleUsedFunctions(library, thisModule.get());
        vis.markModuleVarsUsed(library, thisModule.get());
    }

    void Program::markMacroSymbolUse(TextWriter * logs) {
        // this module macro init
        clearSymbolUse();
        MarkSymbolUse vis(this, false);
        vis.tw = logs;
        visit(vis);
        vis.markUsedFunctions(library, false, true, thisModule.get());
        vis.markVarsUsed(library, false);
    }

    void Program::markExecutableSymbolUse(TextWriter * logs) {
        clearSymbolUse();
        MarkSymbolUse vis(this, false);
        vis.tw = logs;
        visit(vis);
        if ( policies.export_public_functions ) vis.exportPublicFunctions(thisModule.get());
        vis.markUsedFunctions(library, false, false, nullptr);
        vis.markVarsUsed(library, false);
    }

    void Program::markFoldingSymbolUse(const vector<Function *> & needRun, TextWriter * logs) {
        clearSymbolUse();
        MarkSymbolUse vis(this, false);
        vis.tw = logs;
        visit(vis);
        for ( auto fun : needRun ) {
            vis.propagateFunctionUse(fun);
        }
    }

    void Program::markFunctionDependencies(Function * fun) {
        // refresh use-edges over the closure reachable from fun only: a macro asking for one
        // function's dependencies must not pay for a walk of every module in the program
        MarkSymbolUse vis(this, false);
        Visitor & bvis = vis;
        das_hash_set<Function *> seenFn;
        das_hash_set<Variable *> seenVar;
        vector<Function *> workFn;
        vector<Variable *> workVar;
        auto pushFn = [&](Function * f) {
            if ( f->builtIn || !bvis.canVisitFunction(f) ) return;
            if ( seenFn.insert(f).second ) workFn.push_back(f);
        };
        auto pushVar = [&](Variable * v) {
            if ( seenVar.insert(v).second ) workVar.push_back(v);
        };
        pushFn(fun);
        while ( !workFn.empty() || !workVar.empty() ) {
            if ( !workFn.empty() ) {
                auto f = workFn.back();
                workFn.pop_back();
                f->visit(vis);
                for ( auto df : f->useFunctions ) pushFn(df);
                for ( auto dv : f->useGlobalVariables ) pushVar(dv);
            } else {
                auto v = workVar.back();
                workVar.pop_back();
                visitGlobalVariable(bvis, v);
                for ( auto df : v->useFunctions ) pushFn(df);
                for ( auto dv : v->useGlobalVariables ) pushVar(dv);
            }
        }
        vis.propagateFunctionUse(fun);
    }

    void Program::markSymbolUse(bool builtInSym, bool forceAll, bool initThis, Module * macroModule, TextWriter * logs) {
        clearSymbolUse();
        MarkSymbolUse vis(this, builtInSym);
        vis.tw = logs;
        visit(vis);
        vis.markUsedFunctions(library, forceAll, initThis, macroModule);
        vis.markVarsUsed(library, forceAll);
    }

    void Program::removeUnusedSymbols() {
        if ( options.getBoolOption("remove_unused_symbols",true) ) {
            ClearUnusedSymbols cvis(this, thisModule.get());
            visit(cvis);
            MarkSymbolUse vis(this, false);
            vis.RemoveUnusedSymbols(*thisModule);
        }
    }

    void Program::dumpSymbolUse(TextWriter & logs) {
        logs << "USED SYMBOLS ARE:\n";
        for (auto & pm : library.modules) {
            for ( auto & var : pm->globals.each() ) {
                if ( isUsed(var) ) {
                    logs << "let " << var->module->name << "::" << var->name << ": " << var->type->describe() << "\n";
                }
            }
            for ( auto & func : pm->functions.each() ) {
                if ( isUsed(func) ) {
                    logs << func->module->name << "::" << func->describe() << "\n";
                }
            }
        }
        logs << "\n\n";
    }
}
