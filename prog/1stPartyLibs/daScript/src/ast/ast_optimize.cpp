#include "daScript/misc/platform.h"
#include "daScript/ast/ast_post_infer.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    void optimizeProgram(Program * program, TextWriter & logs, ModuleGroup & libGroup) {
        bool logOpt = program->options.getBoolOption("log_optimization", program->policies.log_optimization);
        bool logPass = program->options.getBoolOption("log_optimization_passes", program->policies.log_optimization_passes);
        bool log = logOpt || logPass;
        bool any, last;
        int optimizationRound = 1;
        if (log) {
            logs << *program << "\n";
        }
        do {
            if ( log ) logs << "OPTIMIZE " << optimizationRound << ":\n";
            if ( logPass ) logs << *program;
            any = false;
            last = program->optimizationRefFolding(optimizationRound);    if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "REF FOLDING: " << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            applyPostInferMacros(program);                                if ( program->failed() ) break;
            last = program->optimizationUnused(logs, optimizationRound);    if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            last = program->optimizationConstFolding(optimizationRound);  if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "CONST FOLDING:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            last = program->optimizationCondFolding(optimizationRound);  if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "COND FOLDING:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            last = program->optimizationBlockFolding(optimizationRound);  if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "BLOCK FOLDING:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            last = program->optimizationCSE(optimizationRound);  if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "CSE:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            last = program->optimizationDeadStores(optimizationRound);  if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "DEAD STORES:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            // this is here again for a reason
            applyPostInferMacros(program);                                if ( program->failed() ) break;
            last = program->optimizationUnused(logs, optimizationRound);    if ( program->failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            // now, user macros
            last = false;
            auto modMacro = [&](Module * mod) -> bool {    // we run all macros for each module
                if ( program->thisModule->isVisibleDirectly(mod) && mod!=program->thisModule.get() ) {
                    for ( const auto & pm : mod->optimizationMacros ) {
                        last |= pm->apply(program, program->thisModule.get());
                        if ( program->failed() ) {                       // if macro failed, we report it, and we are done
                            program->error("optimization macro " + mod->name + "::" + pm->name + " failed", "","",LineInfo(), CompilationError::runtime_macro);
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( program->failed() ) break;
            any |= last;
            libGroup.foreach(modMacro,"*");
            if ( program->failed() ) break;
            any |= last;
            if ( log ) logs << "MACROS:" << (last ? "optimized" : "nothing") << "\n";
            if ( logPass ) logs << *program;
            optimizationRound++;
        } while ( any );
    }

}
