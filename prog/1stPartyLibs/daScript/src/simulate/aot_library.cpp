#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_library.h"

namespace das {

    AotListBase * AotListBase::head = nullptr;

    SimNode* AotFactory::operator()(Context& ctx) const
    {
        if (is_jit) {
            return makeAotJitNode(ctx, fn);
        } else if (is_cmres) {
            return ctx.code->makeNode<SimNode_AotCMRES>(fn, wrappedFn);
        } else {
            return ctx.code->makeNode<SimNode_Aot>(fn, wrappedFn);
        }
    }

    void aotLibraryEmplace ( AotLibrary & lib, uint64_t hash, const AotFactory & factory ) {
        lib.emplace(hash, factory);
    }

    AotListBase::AotListBase( RegisterAotFunctions prfn ) {
        tail = head;
        head = this;
        regFn = prfn;
    }

    void AotListBase::registerAot ( AotLibrary & lib ) {
        auto it = head;
        while ( it ) {
            (*it->regFn)(lib);
            it = it->tail;
        }
    }

    DAS_THREAD_LOCAL(unique_ptr<AotLibrary>) g_AOT_lib;

    AotLibrary & getGlobalAotLibrary() {
        if ( !*g_AOT_lib ) {
            *g_AOT_lib = make_unique<AotLibrary>();
            AotListBase::registerAot(**g_AOT_lib);
        }
        return **g_AOT_lib;
    }

    void clearGlobalAotLibrary() {
        g_AOT_lib->reset();
    }

}
