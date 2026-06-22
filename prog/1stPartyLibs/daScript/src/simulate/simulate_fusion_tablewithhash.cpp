#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion_op1.h"
#include "daScript/simulate/runtime_table_nodes.h"
#include "daScript/simulate/simulate_visit_op.h"

// OP1 fusion for the const-string-key table nodes (SimNode_*_WithHash). Their single sub-node is
// the table container (tabExpr), evaluated to a Table* via GetLocal / GetArgument / GetArgumentRef;
// the key and hash are baked. Fusing folds that container load into the node, dropping one virtual
// dispatch per lookup. Uses the OP1-SET matcher (container/lvalue shapes), not the OP1 value matcher.

namespace das {

    /* TableIndexWithHash (insert/index, ptr, offset) */

    struct SimNode_Op1TableIndexWithHash : SimNode_Op1Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = op;
            name += getSimSourceName(subexpr.type);
            vis.op(name.c_str());
            subexpr.visit(vis);
            V_ARG(key);
            V_ARG(hashOf);
            V_ARG(valueTypeSize);
            V_ARG(offset);
            V_END();
        }
        char *    key;
        uint64_t  hashOf;
        uint32_t  valueTypeSize;
        uint32_t  offset;
    };

#undef IMPLEMENT_ANY_OP1_SET_NODE
#define IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1TableIndexWithHash { \
        INLINE char * compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            Table * tab = (Table *) subexpr.compute##COMPUTE(context); \
            if ( tab->isLocked() ) context.throw_error_at(debugInfo, "can't insert to a locked table"); \
            TableHash<char*> thh(&context, valueTypeSize); \
            int64_t index = thh.reserve(*tab, key, hashOf, &debugInfo); \
            return tab->data + index * valueTypeSize + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP1_SETUP_NODE
#define IMPLEMENT_OP1_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op1TableIndexWithHash *)result; \
    auto sn = (SimNode_TableIndex_WithHash *)node; \
    rn->key = sn->key; \
    rn->hashOf = sn->hashOf; \
    rn->valueTypeSize = sn->valueTypeSize; \
    rn->offset = sn->offset;

#undef FUSION_OP1_SUBEXPR
#define FUSION_OP1_SUBEXPR(CTYPE,node)      ((static_cast<SimNode_TableIndex_WithHash*>(node))->tabExpr)

    IMPLEMENT_SET_OP1_FUSION_POINT(__forceinline, TableIndexWithHash, Ptr, VoidPtr, VoidPtr);

    /* SafeTableIndexWithHash (find, ptr, null-check) */

    struct SimNode_Op1SafeTableIndexWithHash : SimNode_Op1Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = op;
            name += getSimSourceName(subexpr.type);
            vis.op(name.c_str());
            subexpr.visit(vis);
            V_ARG(key);
            V_ARG(hashOf);
            V_ARG(valueTypeSize);
            V_END();
        }
        char *    key;
        uint64_t  hashOf;
        uint32_t  valueTypeSize;
    };

#undef IMPLEMENT_ANY_OP1_SET_NODE
#define IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1SafeTableIndexWithHash { \
        INLINE char * compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            Table * tab = (Table *) subexpr.compute##COMPUTE(context); \
            if ( !tab ) return nullptr; \
            TableHash<char*> thh(&context, valueTypeSize); \
            int64_t index = thh.find(*tab, key, hashOf); \
            return index!=-1 ? tab->data + index * valueTypeSize : nullptr; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP1_SETUP_NODE
#define IMPLEMENT_OP1_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op1SafeTableIndexWithHash *)result; \
    auto sn = (SimNode_SafeTableIndex_WithHash *)node; \
    rn->key = sn->key; \
    rn->hashOf = sn->hashOf; \
    rn->valueTypeSize = sn->valueTypeSize;

#undef FUSION_OP1_SUBEXPR
#define FUSION_OP1_SUBEXPR(CTYPE,node)      ((static_cast<SimNode_SafeTableIndex_WithHash*>(node))->tabExpr)

    IMPLEMENT_SET_OP1_FUSION_POINT(__forceinline, SafeTableIndexWithHash, Ptr, VoidPtr, VoidPtr);

    void createFusionEngine_tablewithhash() {
        (*getFusionEngine())["TableIndexWithHash"].emplace_back(new Op1FusionPoint_TableIndexWithHash_VoidPtr());
        (*getFusionEngine())["SafeTableIndexWithHash"].emplace_back(new Op1FusionPoint_SafeTableIndexWithHash_VoidPtr());
    }
}

#endif
