#pragma once

#include "daScript/simulate/aot.h"
#include "daScript/simulate/simulate.h"

namespace das {
    struct DAS_API AotFactory {
        bool is_cmres;
        void * fn;
        vec4f (*wrappedFn)(Context*);
        AotFactory(bool is_cmres, void *fn, vec4f(*wrappedFn)(Context*))
            : is_cmres(is_cmres), fn(fn), wrappedFn(wrappedFn) {}

        SimNode * operator ()(Context &ctx) const;
    };
    typedef unordered_map<uint64_t,AotFactory> AotLibrary;  // unordered map for thread safety

    typedef void ( * RegisterAotFunctions ) ( AotLibrary & );

    struct DAS_API AotListBase {
        AotListBase( RegisterAotFunctions prfn );
        static void registerAot ( AotLibrary & lib );

        AotListBase * tail = nullptr;
        static AotListBase * head;
        RegisterAotFunctions regFn;
    };

    DAS_API AotLibrary & getGlobalAotLibrary();
    DAS_API void clearGlobalAotLibrary();

    // Test standalone context

    typedef Context * ( * RegisterTestCreator ) ();

    struct StandaloneContextNode {
        StandaloneContextNode( RegisterTestCreator prfn ) {
            regFn = prfn;
            tail = head;
            head = this;
        }

        StandaloneContextNode * tail = nullptr;
        static StandaloneContextNode * head;
        RegisterTestCreator regFn;
    };

}

