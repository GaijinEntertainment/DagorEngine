#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/runtime_table.h"
#include "daScript/simulate/aot_builtin.h"

namespace das {

    // this one frees data from under arrays and tables only
    struct  GcPod : public DataWalker {
        enum {
            gcFlags = TypeInfo::flag_heapGC,
            gcStructFlags = StructInfo::flag_heapGC
        };
        Context *  __context__ = nullptr;
        LineInfo * __at__ = nullptr;
        const char * errorMessage = nullptr;
        GcPod ( Context * ctx, LineInfo * at ) : __context__(ctx), __at__(at) {}
        void refuse ( const char * message ) {
            errorMessage = message;
            _cancel = true;
        }
        virtual bool canVisitHandle ( char *, TypeInfo * ) override { return false; }
        virtual bool canVisitStructure ( char *, StructInfo * ) override { return true; }
        virtual bool canVisitTuple ( char *, TypeInfo * ) override { return true; }
        virtual bool canVisitVariant ( char *, TypeInfo * ) override { return true; }
        virtual bool canVisitPointer ( TypeInfo * ) override { return false; }
        virtual bool canVisitLambda ( TypeInfo * ) override { return false; }
        virtual bool canVisitIterator ( TypeInfo * ) override { return true; }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint64_t ) override {
            return ti->flags & gcFlags;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            return (ti->secondType->flags & gcFlags);
        }
        virtual void beforeArray ( Array * pa, TypeInfo * ) override {
            if ( pa->borrowed ) {
                if ( !array_forget_locked(*pa) ) {
                    refuse("can't delete locked array");
                }
            } else if ( pa->isLocked() && !pa->hopeless ) {
                refuse("can't delete locked array");
            }
        }
        virtual void beforeTable ( Table * pa, TypeInfo * ) override {
            if ( pa->isLocked() && !pa->hopeless ) {
                refuse("can't delete locked table");
            }
        }
        virtual void afterArray ( Array * pa, TypeInfo * ti ) override {
            if ( pa->data ) {
                if ( !pa->isLocked() || pa->hopeless ) {
                    uint64_t oldSize = pa->capacity*ti->firstType->size;
                    __context__->free(pa->data, oldSize, __at__);
                } else {
                    refuse("can't delete locked array");
                    return;
                }
                if ( pa->hopeless ) {
                    memset ( pa, 0, sizeof(Array) );
                    pa->hopeless = true;
                } else {
                    memset ( pa, 0, sizeof(Array) );
                }
            }
        }
        virtual void afterTable ( Table * pa, TypeInfo * ti ) override {
            if ( pa->data ) {
                if ( !pa->isLocked() || pa->hopeless ) {
                    uint64_t oldSize = pa->capacity*uint64_t(ti->firstType->size+ti->secondType->size) + pa->capacity*tableHashSlotBytes(*pa);
                    __context__->free(pa->data, oldSize, __at__);
                } else {
                    refuse("can't delete locked table");
                    return;
                }
                if ( pa->hopeless ) {
                    memset ( pa, 0, sizeof(Table) );
                    pa->hopeless = true;
                } else {
                    memset ( pa, 0, sizeof(Table) );
                }
            }
        }
    };

    static void collect_pod ( Context & context, LineInfo * at, vec4f value, TypeInfo * type ) {
        const char * errorMessage;
        {
            GcPod gcpod(&context, at);
            gcpod.walk(value, type);
            errorMessage = gcpod.errorMessage;
        }
        if ( errorMessage ) {
            context.throw_error_at(at, "%s", errorMessage);
        }
    }

    vec4f builtin_collect_local_and_zero ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( context.persistent ) {  // only doing any work if its a persistent heap
            collect_pod(context, &call->debugInfo, args[0], call->types[0]);
            auto ptr = cast<void *>::to(args[0]);
            auto tsize = cast<uint32_t>::to(args[1]);
            memset ( ptr, 0, tsize );
        }
        return v_zero();
    }

    // the no-zero variant: frees the value's owned heap through the same GcPod walk (which
    // by construction cannot run user code) but leaves the bytes alone - for container
    // erase/clear/shrink, where the container itself disposes of or reuses the slot memory
    vec4f builtin_collect_local ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( context.persistent ) {  // only doing any work if its a persistent heap
            collect_pod(context, &call->debugInfo, args[0], call->types[0]);
        }
        return v_zero();
    }

    vec4f builtin_scope_free ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( context.persistent ) {  // only persistent heaps free individually
            auto ptr = cast<char *>::to(args[0]);
            if ( ptr ) {
                collect_pod(context, &call->debugInfo, args[0], call->types[0]->firstType);
                auto tsize = cast<uint32_t>::to(args[1]);
                if ( !context.stack.is_stack_ptr(ptr) ) {  // the shell may live in the stack frame (allocate_on_stack)
                    context.free(ptr, tsize, &call->debugInfo);
                }
            }
        }
        return v_zero();
    }
}
