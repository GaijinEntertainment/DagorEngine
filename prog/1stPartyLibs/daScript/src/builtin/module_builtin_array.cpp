#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_policy_types.h"

#include "daScript/simulate/aot_builtin.h"

namespace das {

    int builtin_array_size ( const Array & arr ) {
        // Always-on guard (panics in both debug + release). Use long_length()
        // for arrays that may exceed INT_MAX elements — daslang's length() is
        // int-returning and cannot represent the larger range without lying.
        DAS_VERIFYF(arr.size <= uint64_t(INT32_MAX), "array size %llu exceeds INT_MAX; use long_length() instead", (unsigned long long)arr.size);
        return int(arr.size);
    }

    bool builtin_array_empty ( const Array & arr ) {
        return arr.size == 0;
    }

    int builtin_array_capacity ( const Array & arr ) {
        DAS_VERIFYF(arr.capacity <= uint64_t(INT32_MAX), "array capacity %llu exceeds INT_MAX; use long_capacity() instead", (unsigned long long)arr.capacity);
        return int(arr.capacity);
    }

    int64_t builtin_array_long_size ( const Array & arr ) {
        // The long_length surface returns int64; refuse to wrap negative if a host/interop
        // path somehow produced a size > INT64_MAX. array_resize / array_grow already cap
        // growth at INT64_MAX, so this catches embedder-side corruption.
        DAS_VERIFYF(arr.size <= uint64_t(INT64_MAX), "array size %llu exceeds INT64_MAX", (unsigned long long)arr.size);
        return int64_t(arr.size);
    }

    int64_t builtin_array_long_capacity ( const Array & arr ) {
        DAS_VERIFYF(arr.capacity <= uint64_t(INT64_MAX), "array capacity %llu exceeds INT64_MAX", (unsigned long long)arr.capacity);
        return int64_t(arr.capacity);
    }

    int builtin_array_lock_count ( const Array & arr ) {
        return arr.lock;
    }

    void builtin_array_resize ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at ) {
        if ( newSize<0 ) context->throw_error_at(at, "resizing array to negative size %i", newSize);
        array_resize ( *context, pArray, newSize, stride, /*zero*/ true, at );
    }

    void builtin_array_resize_no_init ( Array & pArray, int newSize, int stride, Context * context,  LineInfoArg * at ) {
        if ( newSize<0 ) context->throw_error_at(at, "resizing array to negative size %i", newSize);
        array_resize ( *context, pArray, newSize, stride, /*zero*/ false, at );
    }

    void builtin_array_reserve ( Array & pArray, int newSize, int stride, Context * context, LineInfoArg * at ) {
        if ( newSize<0 ) return; // no point of displaying errors, if reserve fails
        array_reserve( *context, pArray, newSize, stride, at );
    }

    void builtin_array_resize_i64 ( Array & pArray, int64_t newSize, int stride, Context * context, LineInfoArg * at ) {
        if ( newSize<0 ) context->throw_error_at(at, "resizing array to negative size %lld", (long long)newSize);
        array_resize ( *context, pArray, uint64_t(newSize), stride, /*zero*/ true, at );
    }

    void builtin_array_resize_no_init_i64 ( Array & pArray, int64_t newSize, int stride, Context * context, LineInfoArg * at ) {
        if ( newSize<0 ) context->throw_error_at(at, "resizing array to negative size %lld", (long long)newSize);
        array_resize ( *context, pArray, uint64_t(newSize), stride, /*zero*/ false, at );
    }

    void builtin_array_reserve_i64 ( Array & pArray, int64_t newSize, int stride, Context * context, LineInfoArg * at ) {
        if ( newSize<0 ) return; // no point of displaying errors, if reserve fails
        array_reserve( *context, pArray, uint64_t(newSize), stride, at );
    }

    void builtin_array_erase ( Array & pArray, int index, int stride, Context * context, LineInfoArg * at ) {
        if ( index < 0 || uint64_t(index) >= pArray.size ) {
            context->throw_error_at(at, "erase index out of range, %d of %llu", index, (unsigned long long)pArray.size);
            return;
        }
        memmove ( pArray.data+index*stride, pArray.data+(index+1)*stride, size_t(pArray.size-uint64_t(index)-1)*size_t(stride) );
        array_resize(*context, pArray, pArray.size-1, stride, false, at);
    }

    void builtin_array_erase_range ( Array & pArray, int index, int count, int stride, Context * context, LineInfoArg * at ) {
        // Compute end as uint64 sum AFTER non-negativity check to avoid signed overflow UB on index+count.
        if ( index < 0 || count < 0 || uint64_t(index) + uint64_t(count) > pArray.size ) {
            context->throw_error_at(at, "erasing array range is invalid: index=%d count=%d size=%llu", index, count, (unsigned long long)pArray.size);
            return;
        }
        memmove ( pArray.data+uint64_t(index)*stride, pArray.data+(uint64_t(index)+uint64_t(count))*stride, size_t(pArray.size-uint64_t(index)-uint64_t(count))*size_t(stride) );
        array_resize(*context, pArray, pArray.size-uint64_t(count), stride, false, at);
    }

    void builtin_array_erase_i64 ( Array & pArray, int64_t index, int stride, Context * context, LineInfoArg * at ) {
        if ( index < 0 || uint64_t(index) >= pArray.size ) {
            context->throw_error_at(at, "erase index out of range, %lld of %llu", (long long)index, (unsigned long long)pArray.size);
            return;
        }
        memmove ( pArray.data+index*stride, pArray.data+(index+1)*stride, size_t(pArray.size-uint64_t(index)-1)*size_t(stride) );
        array_resize(*context, pArray, pArray.size-1, stride, false, at);
    }

    void builtin_array_erase_range_i64 ( Array & pArray, int64_t index, int64_t count, int stride, Context * context, LineInfoArg * at ) {
        // Compute end as uint64 sum AFTER non-negativity check to avoid signed overflow UB on index+count.
        if ( index < 0 || count < 0 || uint64_t(index) + uint64_t(count) > pArray.size ) {
            context->throw_error_at(at, "erasing array range is invalid: index=%lld count=%lld size=%llu", (long long)index, (long long)count, (unsigned long long)pArray.size);
            return;
        }
        memmove ( pArray.data+uint64_t(index)*stride, pArray.data+(uint64_t(index)+uint64_t(count))*stride, size_t(pArray.size-uint64_t(index)-uint64_t(count))*size_t(stride) );
        array_resize(*context, pArray, pArray.size-uint64_t(count), stride, false, at);
    }

    void builtin_array_clear ( Array & pArray, Context * context, LineInfoArg * at ) {
        array_clear(*context, pArray, at);
    }

    void builtin_array_lock ( Array & arr, Context * context, LineInfoArg * at ) {
        array_lock(*context, arr, at);
    }

    void builtin_array_unlock ( Array & arr, Context * context, LineInfoArg * at ) {
        array_unlock(*context, arr, at);
    }

    void builtin_array_lock_mutable ( const Array & arr, Context * context, LineInfoArg * at ) {
        array_lock(*context, const_cast<Array&>(arr), at);
    }

    void builtin_array_unlock_mutable ( const Array & arr, Context * context, LineInfoArg * at ) {
        array_unlock(*context, const_cast<Array&>(arr), at);
    }

    void builtin_array_clear_lock ( const Array & arr, Context * ) {
        const_cast<Array&>(arr).hopeless = true;
    }

    void builtin_array_tag ( Array & arr, const char * name, Context * context ) {
        // Debug helper: tag the array's current heap block with `name` so it shows
        // up in heap reports under that name. Requires `options track_allocations`
        // (the heap's mark_comment is a no-op otherwise). The tag is preserved
        // across realloc by array_reserve, which reads the previous tag before
        // overwriting with the generic "array" default. `name` is stored as-is
        // in bigStuffComment; the caller owns its lifetime. The common case is
        // a daslang literal (constStringHeap, never swept); dynamic daslang
        // strings live in stringHeap, whose GC is skipped while track_allocations
        // is on (see Context::collectHeap).
        if ( arr.data && name ) context->heap->mark_comment(arr.data, name);
    }

    void Module_BuiltIn::addArrayTypes(ModuleLibrary & lib) {
        // array functions
        addExtern<DAS_BIND_FUN(builtin_array_clear)>(*this, lib, "clear",
            SideEffects::modifyArgument, "builtin_array_clear")
                ->args({"array","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_size)>(*this, lib, "length",
            SideEffects::none, "builtin_array_size")
                ->arg("array");
        addExtern<DAS_BIND_FUN(builtin_array_empty)>(*this, lib, "empty",
            SideEffects::none, "builtin_array_empty")
                ->arg("array");
        addExtern<DAS_BIND_FUN(builtin_array_capacity)>(*this, lib, "capacity",
            SideEffects::none, "builtin_array_capacity")
                ->arg("array");
        addExtern<DAS_BIND_FUN(builtin_array_long_size)>(*this, lib, "long_length",
            SideEffects::none, "builtin_array_long_size")
                ->arg("array");
        addExtern<DAS_BIND_FUN(builtin_array_long_capacity)>(*this, lib, "long_capacity",
            SideEffects::none, "builtin_array_long_capacity")
                ->arg("array");
        addExtern<DAS_BIND_FUN(builtin_array_lock_count)>(*this, lib, "lock_count",
            SideEffects::none, "builtin_array_lock_count")
                ->arg("array");
        // array built-in functions
        addExtern<DAS_BIND_FUN(builtin_array_resize)>(*this, lib, "__builtin_array_resize",
            SideEffects::modifyArgument, "builtin_array_resize")
                ->args({"array","newSize","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_resize_no_init)>(*this, lib, "__builtin_array_resize_no_init",
            SideEffects::modifyArgument, "builtin_array_resize_no_init")
                ->args({"array","newSize","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_reserve)>(*this, lib, "__builtin_array_reserve",
            SideEffects::modifyArgument, "builtin_array_reserve")
                ->args({"array","newSize","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_resize_i64)>(*this, lib, "__builtin_array_resize_i64",
            SideEffects::modifyArgument, "builtin_array_resize_i64")
                ->args({"array","newSize","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_resize_no_init_i64)>(*this, lib, "__builtin_array_resize_no_init_i64",
            SideEffects::modifyArgument, "builtin_array_resize_no_init_i64")
                ->args({"array","newSize","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_reserve_i64)>(*this, lib, "__builtin_array_reserve_i64",
            SideEffects::modifyArgument, "builtin_array_reserve_i64")
                ->args({"array","newSize","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_push)>(*this, lib, "__builtin_array_push",
            SideEffects::modifyArgument, "builtin_array_push")
                ->args({"array","index","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_push_zero)>(*this, lib, "__builtin_array_push_zero",
            SideEffects::modifyArgument, "builtin_array_push_zero")
                ->args({"array","index","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_push_back)>(*this, lib, "__builtin_array_push_back",
            SideEffects::modifyArgument, "builtin_array_push_back")
                ->args({"array","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_push_back_zero)>(*this, lib, "__builtin_array_push_back_zero",
            SideEffects::modifyArgument, "builtin_array_push_back_zero")
                ->args({"array","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_erase)>(*this, lib, "__builtin_array_erase",
            SideEffects::modifyArgument, "builtin_array_erase")
                ->args({"array","index","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_erase_range)>(*this, lib, "__builtin_array_erase_range",
            SideEffects::modifyArgument, "builtin_array_erase_range")
                ->args({"array","index","count","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_erase_i64)>(*this, lib, "__builtin_array_erase_i64",
            SideEffects::modifyArgument, "builtin_array_erase_i64")
                ->args({"array","index","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_erase_range_i64)>(*this, lib, "__builtin_array_erase_range_i64",
            SideEffects::modifyArgument, "builtin_array_erase_range_i64")
                ->args({"array","index","count","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_lock)>(*this, lib, "__builtin_array_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_array_lock")
                ->args({"array","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_lock_mutable)>(*this, lib, "__builtin_array_lock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_array_lock_mutable")
                ->args({"array","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_unlock)>(*this, lib, "__builtin_array_unlock",
            SideEffects::modifyArgumentAndExternal, "builtin_array_unlock")
                ->args({"array","context","at"});
        addExtern<DAS_BIND_FUN(builtin_array_unlock_mutable)>(*this, lib, "__builtin_array_unlock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_array_unlock_mutable")
                ->args({"array","context","at"});

        addExtern<DAS_BIND_FUN(builtin_array_clear_lock)>(*this, lib, "__builtin_array_clear_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_array_clear_lock")
                ->args({"array","context"});
        addExtern<DAS_BIND_FUN(builtin_array_tag)>(*this, lib, "tag_array",
            SideEffects::modifyExternal, "builtin_array_tag")
                ->args({"array","name","context"});
    }
}
