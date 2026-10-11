#pragma once

#include <cstdlib>

#include "daScript/ast/ast.h"
#include "daScript/daScriptModule.h"

namespace das {

    // The module registry of a standalone context that links C++ modules. Every generated TU
    // adds its modules before main; the first context constructed registers the union, lowest
    // rank first, and initializes once; the destruction of the last live context shuts the
    // registry down - inside main, never from a static destructor, which runs after the
    // runtime's own statics are gone - with ShutdownStandalone, since no interpreter is linked
    // to own a fusion reset. A host that registered the builtin module owns the registry
    // instead: it must have registered every module the contexts link (a missing one stops
    // the program, by name), and it initializes and shuts down itself - the runtime has no way
    // to add to an initialized registry and keep Initialize/Shutdown balanced.
    struct StandaloneModule {
        const char *    name;
        Module *        (*pull)();
        int             rank;
    };

    inline vector<StandaloneModule> & standaloneModules () {
        static vector<StandaloneModule> modules;
        return modules;
    }

    inline bool standaloneAddModules ( const StandaloneModule * first, size_t count ) {
        auto & modules = standaloneModules();
        modules.insert(modules.end(), first, first + count);
        return true;
    }

    struct StandaloneModuleState {
        int     live = 0;       // contexts alive
        bool    owner = false;  // this process's contexts registered the modules and own the shutdown
    };

    inline StandaloneModuleState & standaloneModuleState () {
        static StandaloneModuleState state;
        return state;
    }

    inline void standaloneAcquireModules () {
        auto & state = standaloneModuleState();
        if ( state.live++ != 0 ) return;
        auto & modules = standaloneModules();
        for ( size_t i = 1; i < modules.size(); ++i ) {          // stable insertion sort by rank
            auto entry = modules[i];
            size_t j = i;
            for ( ; j > 0 && modules[j-1].rank > entry.rank; --j ) modules[j] = modules[j-1];
            modules[j] = entry;
        }
        const bool hostOwns = Module::require("$") != nullptr;
        for ( auto & m : modules ) {
            if ( Module::require(m.name) ) continue;
            if ( hostOwns ) {
                // a configuration error, not an invariant: the message names the fix, so no
                // stack trace and no debugger break - the same clean exit in every build config
                DAS_FATAL_LOG("standalone context: module '%s' is not registered, and the host owns "
                    "the module registry - register it before constructing the context\n", m.name);
                exit(-1);
            }
            *ModuleKarma += unsigned(intptr_t(m.pull()));
            state.owner = true;
        }
        if ( state.owner ) Module::Initialize();
    }

    inline void standaloneReleaseModules () {
        auto & state = standaloneModuleState();
        if ( --state.live != 0 || !state.owner ) return;
        state.owner = false;
        Module::ShutdownStandalone(false);
    }

    // The first base of every generated context that links C++ modules: constructed before the
    // Context base, destroyed after it, so the registry outlives every context's teardown.
    struct StandaloneModuleScope {
        StandaloneModuleScope () { standaloneAcquireModules(); }
        ~StandaloneModuleScope () { standaloneReleaseModules(); }
        StandaloneModuleScope ( const StandaloneModuleScope & ) = delete;
        StandaloneModuleScope & operator = ( const StandaloneModuleScope & ) = delete;
    };
}
