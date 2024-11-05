#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/runtime_table_nodes.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    struct SimNode_OpTableIndex : SimNode_Op2Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = op;
            name += getSimSourceName(l.type);
            name += getSimSourceName(r.type);
            if (baseType != Type::none) {
                vis.op(name.c_str(), getTypeBaseSize(baseType), das_to_string(baseType));
            } else {
                vis.op(name.c_str());
            }
            l.visit(vis);
            r.visit(vis);
            V_ARG(valueTypeSize);
            V_ARG(offset);
            V_END();
        }
        uint32_t valueTypeSize;
        uint32_t offset;
    };

#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_OpTableIndex { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto tab = (Table *) l.compute##COMPUTEL(context); \
            auto key = EvalTT<CTYPE>::eval(context,r.subexpr); \
            TableHash<CTYPE> thh(&context,valueTypeSize); \
            auto hfn = hash_function(context, key); \
            int index = thh.reserve(*tab, key, hfn, &debugInfo); \
            return tab->data + index * valueTypeSize + offset; \
        } \
        DAS_PTR_NODE; \
    };

#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_OpTableIndex { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto tab = (Table *) l.compute##COMPUTEL(context); \
            auto key = *((CTYPE *)r.compute##COMPUTER(context)); \
            TableHash<CTYPE> thh(&context,valueTypeSize); \
            auto hfn = hash_function(context, key); \
            int index = thh.reserve(*tab, key, hfn, &debugInfo); \
            return tab->data + index * valueTypeSize + offset; \
        } \
        DAS_PTR_NODE; \
    };

#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_OpTableIndex *)result; \
    auto sn = (SimNode_TableIndex<int> *)node; \
    rn->valueTypeSize = sn->valueTypeSize; \
    rn->offset = sn->offset;

#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_Table *>(node))->tabExpr)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_Table *>(node))->keyExpr)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC(TableIndex);
    IMPLEMENT_ANY_SETOP(__forceinline,TableIndex,Ptr,StringPtr,StringPtr); \

    void createFusionEngine_tableindex() {
        REGISTER_SETOP_NUMERIC(TableIndex);
        REGISTER_SETOP(TableIndex,Ptr,StringPtr);
    }
}

#endif

