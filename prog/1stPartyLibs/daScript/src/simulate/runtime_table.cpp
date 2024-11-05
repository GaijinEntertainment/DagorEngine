#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_table_nodes.h"

namespace das
{
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
        arr.lock ++;
        if ( arr.lock==0 ) context.throw_error_at(at, "table lock overflow");
    }

    void table_unlock ( Context & context, Table & arr, LineInfo * at ) {
        if ( arr.shared || arr.hopeless ) return;
        if ( arr.lock==0 ) context.throw_error_at(at, "table lock underflow");
        arr.lock --;
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
        table_end = data + table->capacity*stride;
        size_t index = nextValid(0);
        data += index * stride;
        *value = data;
        return (bool) table->size;
    }

    bool TableIterator::next  ( Context &, char * _value ) {
        char ** value = (char **) _value;
        char * tableData = getData();
        size_t index = (data-tableData)/stride;
        index = nextValid(index + 1);
        data = tableData + index * stride;
        *value = data;
        return data != table_end;
    }

    void TableIterator::close ( Context & context, char * _value ) {
        if ( _value ) {
            char ** value = (char **) _value;
            *value = nullptr;
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
            table_end = data + table->capacity*stride;
            size_t index = nextValid(0);
            data += index * stride;
            if ( data ) *(KeyType *)_value = *(KeyType *)data;
            return (bool) table->size;
        }
        virtual bool next  ( Context &, char * _value ) override {
            char * tableData = getData();
            size_t index = (data-tableData)/stride;
            index = nextValid(index + 1);
            data = tableData + index * stride;
            *(KeyType *)_value = *(KeyType *)data;
            return data != table_end;
        }
        virtual void close ( Context & context, char * ) override {
            table_unlock(context, *(Table *)table, nullptr);
            context.freeIterator((char *)this, debugInfo);
        }
    };

    void builtin_table_keys ( Sequence & result, const Table & tab, int32_t stride, Context * __context__, LineInfoArg * at ) {
        char * iter = __context__->allocateIterator(sizeof(TableKeysIterator<uint8_t>),"table keys iterator", at);
        if ( !iter ) __context__->throw_out_of_memory(false, sizeof(TableKeysIterator<uint8_t>)+16, at);
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
        if ( !iter ) __context__->throw_out_of_memory(false, sizeof(TableValuesIterator)+16, at);
        new (iter) TableValuesIterator(&tab, stride, at);
        result = { (Iterator *) iter };
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
                    context.throw_error_at(debugInfo, "deleting locked table");
                    return v_zero();
                }
            }
            memset ( pTable, 0, sizeof(Table) );
        }
        return v_zero();
    }
}

