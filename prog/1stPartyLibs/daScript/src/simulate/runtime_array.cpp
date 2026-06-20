#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_array.h"

namespace das
{
    void array_clear ( Context & context, Array & arr, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't clear locked array");
        arr.size = 0;
    }

    void array_mark_locked ( Array & arr, void * data, uint64_t capacity ) {
        arr.data = (char *)data;
        arr.size = arr.capacity = capacity;
        arr.lock = 1;
        arr.magic = DAS_ARRAY_MAGIC;
    }

    void array_mark_locked ( Array & arr, void * data, uint64_t size, uint64_t capacity ) {
        arr.data = (char *)data;
        arr.size = size;
        arr.capacity = capacity;
        arr.lock = 1;
        arr.magic = DAS_ARRAY_MAGIC;
    }

    void array_lock ( Context & context, Array & arr, LineInfo * at ) {
        if ( arr.shared || arr.hopeless ) return;
        if ( arr.lock==0 ) {
            if ( arr.magic != 0 ) {
                context.throw_error_at(at, "array magic mismatch on first lock, was it moved or overwritten?");
            }
            arr.lock = 1;
            arr.magic = DAS_ARRAY_MAGIC;
        } else {
            if ( arr.magic != DAS_ARRAY_MAGIC ) {
                context.throw_error_at(at, "array magic mismatch on lock, was it moved or overwritten?");
            }
            arr.lock ++;
            if ( arr.lock==0 ) {
                context.throw_error_at(at, "array lock overflow, was it moved or overwritten?");
            }
        }
    }

    void array_unlock ( Context & context, Array & arr, LineInfo * at ) {
        if ( arr.shared || arr.hopeless ) return;
        if ( arr.magic != DAS_ARRAY_MAGIC ) {
            context.throw_error_at(at, "array magic mismatch on unlock, was it moved or overwritten?");
        }
        if ( arr.lock==0 ) {
            context.throw_error_at(at, "array lock underflow, was it moved or overwritten?");
        }
        arr.lock --;
        if ( arr.lock==0 ) {
            arr.magic = 0;
        }
    }

    void array_reserve(Context & context, Array & arr, uint64_t newCapacity, uint32_t stride, LineInfo * at) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't change capacity of a locked array");
        if ( arr.capacity >= newCapacity ) return;
        // Explicit mul_overflow guard: stride is uint32, newCapacity is uint64.
        // The product can overflow uint64 when newCapacity > UINT64_MAX / stride
        // (with stride > 0). Panic before passing a wrap-around byte count to
        // the heap allocator (where it would silently allocate a small block
        // and the next memcpy would overrun random memory).
        if ( stride && newCapacity > UINT64_MAX / uint64_t(stride) ) {
            context.throw_error_at(at, "array_reserve: capacity*stride overflows uint64 [capacity=%llu] [stride=%u]", (unsigned long long)newCapacity, stride);
        }
        uint64_t memSize64 = newCapacity * uint64_t(stride);
        const char * prev_comment = arr.data ? context.heap->get_comment(arr.data) : nullptr;
        char * newData = nullptr;
        if ( context.verySafeContext ) {
            newData = (char *)context.allocate(memSize64, at);
            if ( newData && arr.data ) {
                memcpy(newData, arr.data, arr.size*stride);
            }
        } else {
            newData = (char *)context.reallocate(arr.data, arr.capacity*stride, memSize64, at);
        }
        context.heap->mark_comment(newData, prev_comment ? prev_comment : "array");
        if ( newData != arr.data ) {
            // memcpy(newData, arr.data, arr.capacity);
            arr.data = newData;
        }
        arr.capacity = newCapacity;
    }

    void array_resize ( Context & context, Array & arr, uint64_t newSize, uint32_t stride, bool zero, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't resize locked array");
        // The daslang surface contract is `long_length(arr) : int64`, so a size that doesn't
        // fit in int64_t would wrap negative on the way out. Enforce the cap up front so the
        // int64 API stays well-defined.
        if ( newSize > uint64_t(INT64_MAX) ) {
            context.throw_error_at(at, "array_resize: newSize exceeds INT64_MAX [newSize=%llu]", (unsigned long long)newSize);
        }
        if ( newSize > arr.capacity ) {
            uint64_t newCapacity = uint64_t(1) << (64 - das_clz64(das::max(newSize, uint64_t(2)) - 1));
            newCapacity = das::max(newCapacity, uint64_t(16));
            // The pow2 round-up overflows past INT64_MAX when newSize > 2^62; clamp so the
            // resulting capacity stays representable in the int64 long_capacity() surface.
            if ( newCapacity > uint64_t(INT64_MAX) ) newCapacity = uint64_t(INT64_MAX);
            array_reserve(context, arr, newCapacity, stride, at);
        }
        if ( zero && newSize>arr.size ) {
            memset ( arr.data + arr.size*stride, 0, size_t(newSize-arr.size)*size_t(stride) );
        }
        arr.size = newSize;
    }

    // GoodArrayIterator

    bool GoodArrayIterator::first ( Context & context, char * _value )  {
        char ** value = (char **) _value;
        array_lock(context, *array, nullptr);
        data = array->data;
        *value = data;
        array_end  = data + array->size * stride;
        return (bool) array->size;
    }

    bool GoodArrayIterator::next  ( Context &, char * _value )  {
        char ** value = (char **) _value;
        data += stride;
        *value = data;
        return data != array_end;
    }

    void GoodArrayIterator::close ( Context & context, char * _value )  {
        if ( _value ) {
            char ** value = (char **) _value;
            *value = nullptr;
        }
        array_unlock(context, *array, nullptr);
        context.freeIterator((char *)this, debugInfo);
    }

    vec4f SimNode_GoodArrayIterator::eval ( Context & context ) {
        DAS_PROFILE_NODE
        vec4f ll = source->eval(context);
        Array * arr = cast<Array *>::to(ll);
        char * iter = context.allocateIterator(sizeof(GoodArrayIterator),"array<> iterator", &debugInfo);
        new (iter) GoodArrayIterator(arr, stride, &debugInfo);
        return cast<char *>::from(iter);
    }

    // FixedArrayIterator

    bool FixedArrayIterator::first ( Context &, char * _value )  {
        char ** value = (char **) _value;
        *value = data;
        fixed_array_end = data + size*stride;
        return (bool) size;
    }

    bool FixedArrayIterator::next  ( Context & , char * _value )  {
        char ** value = (char **) _value;
        data += stride;
        *value = data;
        return data != fixed_array_end;
    }

    void FixedArrayIterator::close ( Context & context, char * _value )  {
        if ( _value ) {
            char ** value = (char **) _value;
            *value = nullptr;
        }
        context.freeIterator((char *)this, debugInfo);
    }

    vec4f SimNode_FixedArrayIterator::eval ( Context & context ) {
        DAS_PROFILE_NODE
        vec4f ll = source->eval(context);
        char * data = cast<char *>::to(ll);
        char * iter = context.allocateIterator(sizeof(FixedArrayIterator),"fixed array iterator", &debugInfo);
        new (iter) FixedArrayIterator(data, size, stride, &debugInfo);
        return cast<char *>::from(iter);
    }

    // delete

    vec4f SimNode_DeleteArray::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pArray = (Array *) subexpr->evalPtr(context);
        pArray = pArray + total - 1;
        for ( uint32_t i=0, is=total; i!=is; ++i, pArray-- ) {
            if ( pArray->data ) {
                if ( !pArray->isLocked() ) {
                    uint64_t oldSize = pArray->capacity * uint64_t(stride);
                    context.free(pArray->data, oldSize, &debugInfo);
                } else {
                    context.throw_error_at(debugInfo, "deleting locked array%s", errorMessage);
                    return v_zero();
                }
            }
            memset ( pArray, 0, sizeof(Array) );
        }
        return v_zero();
    }
}
