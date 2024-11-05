#pragma once

#include "daScript/simulate/simulate.h"

namespace das {
    typedef SimNode * (*AotFactory) (Context &);
    typedef unordered_map<uint64_t,AotFactory> AotLibrary;  // unordered map for thread safety

    typedef void ( * RegisterAotFunctions ) ( AotLibrary & );

    struct AotListBase {
        AotListBase( RegisterAotFunctions prfn );
        static void registerAot ( AotLibrary & lib );

        AotListBase * tail = nullptr;
        static AotListBase * head;
        RegisterAotFunctions regFn;
    };

    AotLibrary & getGlobalAotLibrary();
    void clearGlobalAotLibrary();

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

