#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/sim_policy.h"

namespace das
{
    struct LockDataWalker : DataWalker {
        bool locked = false;
        virtual bool canVisitArray ( Array * ar, TypeInfo * ) override {
            return !ar->forego_lock_check;
        }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t ) override {
            return (ti->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck;
        }
        virtual bool canVisitTable ( char * pa, TypeInfo * ) override {
            return !((Table *)pa)->forego_lock_check;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            if ( ti->secondType ) {
                return (ti->secondType->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck;
            } else {
                return false;
            }
        }
        virtual bool canVisitHandle ( char *, TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitPointer ( TypeInfo * ) override {
            return false; }
        virtual bool canVisitLambda ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitIterator ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitStructure ( char *, StructInfo * si ) override {
            return (si->flags & StructInfo::flag_lockCheck) == StructInfo::flag_lockCheck;
        }
        virtual void beforeArray ( Array * pa, TypeInfo * ) override {
            if ( pa->lock ) {
                locked = true;
                cancel = true;
            }
        }
        virtual void beforeTable ( Table * pa, TypeInfo * ) override {
            if ( pa->lock ) {
                locked = true;
                cancel = true;
            }
        }
    };

    LineInfo rtti_get_line_info ( int depth, Context * context, LineInfoArg * at );

    vec4f builtin_verify_locks ( Context & context, SimNode_CallBase * node, vec4f * args ) {
        if ( context.skipLockChecks ) return v_zero();
        auto typeInfo = node->types[0];
        auto value = args[0];
        bool failed = false;
        {
            LockDataWalker walker;
            walker.walk(value,typeInfo);
            failed = walker.locked;
        }
        if ( failed ) {
            LineInfo atProblem = rtti_get_line_info(2,&context,(LineInfoArg *) &node->debugInfo);
            context.throw_error_at(atProblem, "object contains locked elements and can't be resized");
        }
        return v_zero();
    }

    bool builtin_set_verify_context ( bool check, Context * context ) {
        bool result = !context->skipLockChecks;
        context->skipLockChecks = !check;
        return result;
    }

    bool builtin_set_verify_array_locks ( Array & arr, bool value ) {
        auto result = !arr.forego_lock_check;
        arr.forego_lock_check = !value;
        return result;
    }

    bool builtin_set_verify_table_locks ( Table & tab, bool value ) {
        auto result = !tab.forego_lock_check;
        tab.forego_lock_check = !value;
        return result;
    }
}
