#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_table_nodes.h"

namespace das
{
    template <typename KeyType>
    void table_reserve_internal ( Context & context, Table & arr, uint32_t newCapacity, uint32_t valueTypeSize, LineInfo * at ) {
        TableHash<KeyType> hash(&context, valueTypeSize);
        hash.reserve(arr, newCapacity, at);
    }

    void table_reserve_impl ( Context & context, Table & arr, int32_t baseType, uint32_t newCapacity, uint32_t valueTypeSize, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't reserve locked table");
        if ( !newCapacity ) return;
        switch ( Type(baseType) ) {
            case Type::tBool:           table_reserve_internal<bool>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt8:           table_reserve_internal<int8_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt8:          table_reserve_internal<uint8_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt16:          table_reserve_internal<int16_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt16:         table_reserve_internal<uint16_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt64:          table_reserve_internal<int64_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt64:         table_reserve_internal<uint64_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tEnumeration:    table_reserve_internal<int32_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tEnumeration8:   table_reserve_internal<int8_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tEnumeration16:  table_reserve_internal<int16_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tEnumeration64:  table_reserve_internal<int64_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tBitfield:       table_reserve_internal<uint32_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tBitfield8:      table_reserve_internal<uint8_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tBitfield16:     table_reserve_internal<uint16_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tBitfield64:     table_reserve_internal<uint64_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt:            table_reserve_internal<int32_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt2:           table_reserve_internal<int2>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt3:           table_reserve_internal<int3>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tInt4:           table_reserve_internal<int4>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt:           table_reserve_internal<uint32_t>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt2:          table_reserve_internal<uint2>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt3:          table_reserve_internal<uint3>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tUInt4:          table_reserve_internal<uint4>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tFloat:          table_reserve_internal<float>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tFloat2:         table_reserve_internal<float2>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tFloat3:         table_reserve_internal<float3>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tFloat4:         table_reserve_internal<float4>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tRange:          table_reserve_internal<range>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tURange:         table_reserve_internal<urange>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tRange64:        table_reserve_internal<range64>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tURange64:       table_reserve_internal<urange64>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tString:         table_reserve_internal<char *>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tPointer:        table_reserve_internal<void *>(context, arr, newCapacity, valueTypeSize, at); break;
            case Type::tDouble:         table_reserve_internal<double>(context, arr, newCapacity, valueTypeSize, at); break;
            default:
                context.throw_error_at(at, "usupported table reserve type");
                return;
        }
    }

    void table_clear ( Context & context, Table & arr, LineInfo * at ) {
        if ( arr.isLocked() ) context.throw_error_at(at, "can't clear locked table");
        if ( arr.data ) {
            memset(arr.hashes, 0, arr.capacity*sizeof(TableHashKey));
            memset(arr.data, 0, arr.keys - arr.data);
        }
        arr.size = 0;
        arr.tombstones = 0;
    }

    void table_lock ( Context & context, Table & arr, LineInfo * at ) {
        if ( arr.shared || arr.hopeless ) return;
        if ( arr.lock==0 ) {
            if ( arr.magic != 0 ) {
                context.throw_error_at(at, "table magic mismatch on first lock, was it moved or overwritten?");
            }
            arr.lock = 1;
            arr.magic = DAS_ARRAY_MAGIC;
        } else {
            if ( arr.magic != DAS_ARRAY_MAGIC ) {
                context.throw_error_at(at, "table magic mismatch on lock, was it moved or overwritten?");
            }
            arr.lock ++;
            if ( arr.lock==0 ) {
                context.throw_error_at(at, "table lock overflow, was it moved or overwritten?");
            }
        }
    }

    void table_unlock ( Context & context, Table & arr, LineInfo * at ) {
        if ( arr.shared || arr.hopeless ) return;
        if ( arr.magic != DAS_ARRAY_MAGIC ) {
            context.throw_error_at(at, "table magic mismatch on unlock, was it moved or overwritten?");
        }
        if ( arr.lock==0 ) {
            context.throw_error_at(at, "table lock underflow, was it moved or overwritten?");
        }
        arr.lock --;
        if ( arr.lock==0 ) {
            arr.magic = 0;
        }
    }

    // TableIterator

    size_t TableIterator::nextValid ( size_t index ) const {
        for ( auto indexs=table->capacity; index < indexs; index++) {
            if (table->hashes[index] > HASH_KILLED64) {
                break;
            }
        }
        return index;
    }

    bool TableIterator::first ( Context & context, char * _value ) {
        char ** value = (char **)_value;
        table_lock(context, *(Table *)table, nullptr);
        data  = getData();
        originData = data;
        table_end = data + table->capacity*stride;
        size_t index = nextValid(0);
        data += index * stride;
        *value = data;
        return (bool) table->size;
    }

    bool TableIterator::next  ( Context &, char * _value ) {
        char ** value = (char **) _value;
        size_t index = (data-originData)/stride;
        index = nextValid(index + 1);
        data = originData + index * stride;
        *value = data;
        return data != table_end;
    }

    void TableIterator::close ( Context & context, char * _value ) {
        if ( _value ) {
            char ** value = (char **) _value;
            *value = nullptr;
        }
        if ( getData()!=originData ) {
            context.throw_error_at(debugInfo, "table was modified during iteration");
        }
        table_unlock(context, *(Table *)table, nullptr);
        context.freeIterator((char *)this, debugInfo);
    }

    // keys and values

    template <typename KeyType>
    struct TableKeysIterator : TableIterator {
        TableKeysIterator ( const Table * tab, uint32_t st, LineInfo * at ) : TableIterator(tab,st,at) {}
        virtual char * getData ( ) const override {
            return table->keys;
        }
        virtual bool first ( Context & context, char * _value ) override {
            table_lock(context, *(Table *)table, nullptr);
            data  = getData();
            originData = data;
            table_end = data + table->capacity*stride;
            size_t index = nextValid(0);
            data += index * stride;
            if ( data ) *(KeyType *)_value = *(KeyType *)data;
            return (bool) table->size;
        }
        virtual bool next  ( Context &, char * _value ) override {
            size_t index = (data-originData)/stride;
            index = nextValid(index + 1);
            data = originData + index * stride;
            *(KeyType *)_value = *(KeyType *)data;
            return data != table_end;
        }
        virtual void close ( Context & context, char * ) override {
            if ( getData()!=originData ) {
                context.throw_error_at(debugInfo, "table was modified during iteration");
            }
            table_unlock(context, *(Table *)table, nullptr);
            context.freeIterator((char *)this, debugInfo);
        }
    };

    void builtin_table_keys ( Sequence & result, const Table & tab, int32_t stride, Context * __context__, LineInfoArg * at ) {
        char * iter = __context__->allocateIterator(sizeof(TableKeysIterator<uint8_t>),"table keys iterator", at);
        switch ( stride ) {
        case 1:     new (iter) TableKeysIterator<uint8_t>(&tab, stride, at); break;
        case 2:     new (iter) TableKeysIterator<uint16_t>(&tab, stride, at); break;
        case 4:     new (iter) TableKeysIterator<uint32_t>(&tab, stride, at); break;
        case 8:     new (iter) TableKeysIterator<uint64_t>(&tab, stride, at); break;
        case 12:    new (iter) TableKeysIterator<float3>(&tab, stride, at); break;
        case 16:    new (iter) TableKeysIterator<vec4f>(&tab, stride, at); break;
        default:    __context__->throw_error_at(at, "unsupported key type, stride=%i", stride);
        }
        result = { (Iterator *) iter };
    }

    char * TableValuesIterator::getData ( ) const {
        return table->data;
    }

    void builtin_table_values ( Sequence & result, const Table & tab, int32_t stride, Context * __context__, LineInfoArg * at ) {
        char * iter = __context__->allocateIterator(sizeof(TableValuesIterator),"table values iterator", at);
        new (iter) TableValuesIterator(&tab, stride, at);
        result = { (Iterator *) iter };
    }

    void builtin_table_get_key ( void * result, const Table & tab, const void * value_ptr, int32_t value_stride, int32_t key_stride, Context * __context__, LineInfoArg * at ) {
        const char * vp = (const char *)value_ptr;
        if ( vp < tab.data || vp >= tab.data + tab.capacity * value_stride ) {
            __context__->throw_error_at(at, "get_key: value pointer is not inside the table");
            return;
        }
        ptrdiff_t offset = vp - tab.data;
        if ( offset % value_stride != 0 ) {
            __context__->throw_error_at(at, "get_key: value pointer is not aligned to value stride");
            return;
        }
        ptrdiff_t index = offset / value_stride;
        if ( tab.hashes[index] <= HASH_KILLED64 ) {
            __context__->throw_error_at(at, "get_key: value points to an empty or deleted table slot");
            return;
        }
        memcpy(result, tab.keys + index * key_stride, key_stride);
    }

    // delete

    vec4f SimNode_DeleteTable::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto pTable = (Table *) subexpr->evalPtr(context);
        pTable = pTable + total - 1;
        for ( uint32_t i=0, is=total; i!=is; ++i, pTable-- ) {
            if ( pTable->data ) {
                if ( !pTable->isLocked() ) {
                    uint32_t oldSize = pTable->capacity*(vts_add_kts + sizeof(TableHashKey));
                    context.free(pTable->data, oldSize, &debugInfo);
                } else {
                    context.throw_error_at(debugInfo, "deleting locked table%s", errorMessage);
                    return v_zero();
                }
            }
            memset ( pTable, 0, sizeof(Table) );
        }
        return v_zero();
    }
}

