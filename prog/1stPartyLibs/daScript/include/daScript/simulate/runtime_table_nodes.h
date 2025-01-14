#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/runtime_table.h"
#include "daScript/misc/arraytype.h"
#include "daScript/simulate/hash.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    struct SimNode_CastToWorkhorse : SimNode {
        SimNode_CastToWorkhorse(const LineInfo & at, SimNode * k)
            : SimNode(at), keyExpr(k) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(CastToWorkhorse);
            V_SUB(keyExpr);
            V_END();
        }
        SimNode * keyExpr;
        DAS_EVAL_ABI  virtual vec4f eval ( Context & context ) override { return keyExpr->eval(context); }
        virtual char *      evalPtr ( Context & context )    override { return cast<char *>::to(keyExpr->eval(context)); }
        virtual bool        evalBool ( Context & context )   override { return cast<bool>::to(keyExpr->eval(context)); }
        virtual float       evalFloat ( Context & context )  override { return cast<float>::to(keyExpr->eval(context)); }
        virtual double      evalDouble ( Context & context ) override { return cast<double>::to(keyExpr->eval(context)); }
        virtual int32_t     evalInt ( Context & context )    override { return cast<int32_t>::to(keyExpr->eval(context)); }
        virtual uint32_t    evalUInt ( Context & context )   override { return cast<uint32_t>::to(keyExpr->eval(context)); }
        virtual int64_t     evalInt64 ( Context & context )  override { return cast<int64_t>::to(keyExpr->eval(context)); }
        virtual uint64_t    evalUInt64 ( Context & context ) override { return cast<uint64_t>::to(keyExpr->eval(context)); }
    };

    struct SimNode_Table : SimNode {
        SimNode_Table(const LineInfo & at, SimNode * t, SimNode * k, uint32_t vts)
            : SimNode(at), tabExpr(t), keyExpr(k), valueTypeSize(vts) {}
        SimNode * visitTable ( SimVisitor & vis, const char * op ) {
            V_BEGIN();
            vis.op(op);
            V_SUB(tabExpr);
            V_SUB(keyExpr);
            V_ARG(valueTypeSize);
            V_END();
        }
        SimNode * tabExpr;
        SimNode * keyExpr;
        uint32_t valueTypeSize;
    };

    template <typename KeyType>
    struct SimNode_TableIndex : SimNode_Table {
        DAS_PTR_NODE;
        SimNode_TableIndex(const LineInfo & at, SimNode * t, SimNode * k, uint32_t vts, uint32_t o)
            : SimNode_Table(at,t,k,vts), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            using TT = KeyType;
            V_BEGIN();
            V_OP_TT(TableIndex);
            V_SUB(tabExpr);
            V_SUB(keyExpr);
            V_ARG(valueTypeSize);
            V_ARG(offset);
            V_END();
        }
        __forceinline char * compute ( Context & context ) {
            DAS_PROFILE_NODE
            Table * tab = (Table *) tabExpr->evalPtr(context);
            if ( tab->lock ) context.throw_error_at(debugInfo, "can't insert to a locked table");
            auto key = EvalTT<KeyType>::eval(context,keyExpr);
            TableHash<KeyType> thh(&context,valueTypeSize);
            auto hfn = hash_function(context, key);
            int index = thh.reserve(*tab, key, hfn, &debugInfo);    // if index==-1, it was a through, so safe to do
            return tab->data + index * valueTypeSize + offset;
        }
        uint32_t offset;
    };

    template <typename KeyType>
    struct SimNode_SafeTableIndex : SimNode_TableIndex<KeyType> {
        using PT = SimNode_TableIndex<KeyType>;
        DAS_PTR_NODE;
        SimNode_SafeTableIndex(const LineInfo & at, SimNode * t, SimNode * k, uint32_t vts, uint32_t o)
            : SimNode_TableIndex<KeyType>(at,t,k,vts,o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            using TT = KeyType;
            V_BEGIN();
            V_OP_TT(SafeTableIndex);
            V_SUB(PT::tabExpr);
            V_SUB(PT::keyExpr);
            V_ARG(PT::valueTypeSize);
            V_ARG(PT::offset);
            V_END();
        }
        __forceinline char * compute ( Context & context ) {
            DAS_PROFILE_NODE
            Table * tab = (Table *) PT::tabExpr->evalPtr(context);
            if (!tab) return nullptr;
            auto key = EvalTT<KeyType>::eval(context,PT::keyExpr);
            TableHash<KeyType> thh(&context,PT::valueTypeSize);
            auto hfn = hash_function(context, key);
            int index = thh.find(*tab, key, hfn);
            return index!=-1 ? tab->data + index * PT::valueTypeSize : nullptr;
        }
    };

    template <typename KeyType>
    struct SimNode_TableErase : SimNode_Table {
        DAS_BOOL_NODE;
        SimNode_TableErase(const LineInfo & at, SimNode * t, SimNode * k, uint32_t vts)
            : SimNode_Table(at,t,k,vts) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitTable(vis,"TableErase");
        }
        __forceinline bool compute ( Context & context ) {
            DAS_PROFILE_NODE
            Table * tab = (Table *) tabExpr->evalPtr(context);
            if ( tab->isLocked() ) context.throw_error_at(debugInfo, "can't erase from locked table");
            auto key = EvalTT<KeyType>::eval(context,keyExpr);
            auto hfn = hash_function(context, key);
            TableHash<KeyType> thh(&context,valueTypeSize);
            return thh.erase(*tab, key, hfn) != -1;
        }
    };

    template <typename KeyType>
    struct SimNode_TableSetInsert : SimNode_Table {
        SimNode_TableSetInsert(const LineInfo & at, SimNode * t, SimNode * k)
            : SimNode_Table(at,t,k, 0) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitTable(vis,"TableSetInsert");
        }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override {
            DAS_PROFILE_NODE
            Table * tab = (Table *) tabExpr->evalPtr(context);
            if ( tab->isLocked() ) context.throw_error_at(debugInfo, "can't insert to a locked table");
            auto key = EvalTT<KeyType>::eval(context,keyExpr);
            auto hfn = hash_function(context, key);
            TableHash<KeyType> thh(&context,valueTypeSize);
            thh.reserve(*tab, key, hfn, &debugInfo);
            return v_zero();
        }
    };

    template <typename KeyType>
    struct SimNode_TableFind : SimNode_Table {
        DAS_PTR_NODE;
        SimNode_TableFind(const LineInfo & at, SimNode * t, SimNode * k, uint32_t vts)
            : SimNode_Table(at,t,k,vts) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitTable(vis,"TableFind");
        }
        __forceinline char * compute(Context & context) {
            DAS_PROFILE_NODE
            Table * tab = (Table *)tabExpr->evalPtr(context);
            auto key = EvalTT<KeyType>::eval(context,keyExpr);
            auto hfn = hash_function(context, key);
            TableHash<KeyType> thh(&context,valueTypeSize);
            int index = thh.find(*tab, key, hfn);
            return index!=-1 ? tab->data + index * valueTypeSize : nullptr;
        }
    };

    template <typename KeyType>
    struct SimNode_KeyExists : SimNode_Table {
        DAS_BOOL_NODE;
        SimNode_KeyExists(const LineInfo & at, SimNode * t, SimNode * k, uint32_t vts)
            : SimNode_Table(at,t,k,vts) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitTable(vis,"KeyExists");
        }
        __forceinline bool compute(Context & context) {
            DAS_PROFILE_NODE
            Table * tab = (Table *)tabExpr->evalPtr(context);
            auto key = EvalTT<KeyType>::eval(context,keyExpr);
            auto hfn = hash_function(context, key);
            TableHash<KeyType> thh(&context,valueTypeSize);
            return thh.find(*tab, key, hfn) != -1;
        }
    };

    struct TableIterator : Iterator {
        TableIterator ( const Table * tab, uint32_t st, LineInfo * at ) : Iterator(at), table(tab), stride(st) {}
        size_t nextValid ( size_t index ) const;
        virtual bool first ( Context & context, char * value ) override;
        virtual bool next  ( Context & context, char * value ) override;
        virtual void close ( Context & context, char * value ) override;
        virtual char * getData () const = 0;
        const Table *   table;
        uint32_t        stride = 0;
        char *          data = nullptr;
        char *          table_end = nullptr;
    };

    struct TableValuesIterator : TableIterator {
        TableValuesIterator ( const Table * tab, uint32_t st, LineInfo * at ) : TableIterator(tab,st,at) {}
        virtual char * getData ( ) const override;
    };

    struct SimNode_DeleteTable : SimNode_Delete {
        SimNode_DeleteTable ( const LineInfo & a, SimNode * s, uint32_t t, uint32_t va )
            : SimNode_Delete(a,s,t), vts_add_kts(va) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        uint32_t vts_add_kts;
    };
}

#include "daScript/simulate/simulate_visit_op_undef.h"

