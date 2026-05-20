#include "daScript/misc/platform.h"
#include "daScript/misc/handle_registry.h"

namespace das {

    // Singletons live in the main DAS library so all DLLs (daslang.exe + each
    // dasModule*.shared_module) share one hooks vector and one mutex.
    // Without this, inline function-local statics would be per-DLL and dump
    // callbacks registered from a module DLL would never be seen by the host.

    DAS_API vector<HandleLeakDumpFn> & handleRegistry_dumpHooks_impl () {
        static vector<HandleLeakDumpFn> hooks;
        return hooks;
    }

    DAS_API mutex & handleRegistry_dumpMutex_impl () {
        static mutex m;
        return m;
    }

    DAS_API void handleRegistry_registerDump_impl ( HandleLeakDumpFn fn ) {
        lock_guard<mutex> g(handleRegistry_dumpMutex_impl());
        auto & hooks = handleRegistry_dumpHooks_impl();
        for ( auto e : hooks ) if ( e == fn ) return;
        hooks.push_back(fn);
    }

    DAS_API void handleRegistry_dumpAll_impl () {
        // Snapshot hooks under the registry mutex, then invoke outside so
        // that a callback freeing handles (or re-registering) cannot deadlock
        // against a concurrent addHandleAnnotation<T> on another thread.
        vector<HandleLeakDumpFn> hooks;
        {
            lock_guard<mutex> g(handleRegistry_dumpMutex_impl());
            hooks = handleRegistry_dumpHooks_impl();
        }
        for ( auto fn : hooks ) fn();
    }

    static vector<HandleLeakCountFn> & handleRegistry_countHooks_impl () {
        static vector<HandleLeakCountFn> hooks;
        return hooks;
    }

    DAS_API void handleRegistry_registerCount_impl ( HandleLeakCountFn fn ) {
        lock_guard<mutex> g(handleRegistry_dumpMutex_impl());
        auto & hooks = handleRegistry_countHooks_impl();
        for ( auto e : hooks ) if ( e == fn ) return;
        hooks.push_back(fn);
    }

    DAS_API uint64_t handleRegistry_countAll_impl () {
        vector<HandleLeakCountFn> hooks;
        {
            lock_guard<mutex> g(handleRegistry_dumpMutex_impl());
            hooks = handleRegistry_countHooks_impl();
        }
        uint64_t total = 0;
        for ( auto fn : hooks ) total += fn();
        return total;
    }

}
