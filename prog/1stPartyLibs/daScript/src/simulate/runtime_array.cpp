#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_array.h"

namespace das
{
    void array_clear ( Context & context, Array & arr, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't clear locked array");
        arr.size = 0;
    }

    void array_mark_locked ( Array & arr, void * data, uint32_t capacity ) {
        arr.data = (char *)data;
        arr.size = arr.capacity = capacity;
        arr.lock = 1;
        arr.magic = DAS_ARRAY_MAGIC;
    }

    void array_mark_locked ( Array & arr, void * data, uint32_t size, uint32_t capacity ) {
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

    void array_reserve(Context & context, Array & arr, uint32_t newCapacity, uint32_t stride, LineInfo * at) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't change capacity of a locked array");
        if ( arr.capacity >= newCapacity ) return;
        uint64_t memSize64 = uint64_t(newCapacity) * uint64_t(stride);
        if ( memSize64>=0xffffffff ) {
            context.throw_error_at(at, "can't grow array, out of index space [capacity=%i] [stride=%i]", newCapacity, stride);
        }
        char * newData = nullptr;
        if ( context.verySafeContext ) {
            newData = (char *)context.allocate(newCapacity*stride, at);
            if ( newData && arr.data ) {
                memcpy(newData, arr.data, arr.size*stride);
            }
        } else {
            newData = (char *)context.reallocate(arr.data, arr.capacity*stride, newCapacity*stride, at);
        }
        context.heap->mark_comment(newData, "array");
        if ( newData != arr.data ) {
            // memcpy(newData, arr.data, arr.capacity);
            arr.data = newData;
        }
        arr.capacity = newCapacity;
    }

    void array_resize ( Context & context, Array & arr, uint32_t newSize, uint32_t stride, bool zero, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't resize locked array");
        if ( newSize > arr.capacity ) {
            uint32_t newCapacity = 1 << (32 - das_clz (das::max(newSize,2u) - 1));
            newCapacity = das::max(newCapacity, 16u);
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
                    uint32_t oldSize = pArray->capacity*stride;
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
