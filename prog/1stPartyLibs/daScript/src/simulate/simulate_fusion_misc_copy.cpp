#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/misc/copy_bytes.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion_op2.h"

namespace das {

    // copy reference
    //  &a = b
    struct FusionPoint_MiscCopyReference : FusionPointOp2 {
        struct SimNode_CopyReferenceLocAny : SimNode_Op2Fusion {
            virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override {
                DAS_PROFILE_NODE
                char  ** pl = (char **)l.computeLocal(context);
                char * pr   = r.computeAnyPtr(context);
                *pl = pr;
                return v_zero();
            }
        };
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_l, SimNode *, Context * context) override {
            if (false) {}
            else if ( is(info, node_l, "GetLocal" )) { anyRight = true; return context->code->makeNode<SimNode_CopyReferenceLocAny>();  }
            return nullptr;
        }
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) override {
            result->set("CopyReference",Type::none,node->debugInfo);
        }
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override {
            SimNode_CopyReference * crnode = static_cast<SimNode_CopyReference *>(node);
            return fuseOp2(info, node, crnode->l, crnode->r, context);
        }
    };

    struct SimNode_Op2FusionCopyRef : SimNode_Op2Fusion {
        uint32_t size;
    };

#define IMPLEMENT_OP2_COPYREF_NODE(COMPUTEL,COMPUTER) \
    template <int typeSize> \
    struct SimNode_CopyRefValueFixed_##COMPUTEL##_##COMPUTER : SimNode_Op2FusionCopyRef { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto pr = r.compute##COMPUTER(context); \
            CopyBytes<typeSize>::copy(pl, pr); \
            return v_zero(); \
        } \
    }; \
    struct SimNode_CopyRefValue_##COMPUTEL##_##COMPUTER : SimNode_Op2FusionCopyRef { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto pr = r.compute##COMPUTER(context); \
            memcpy(pl, pr, size); \
            return v_zero(); \
        } \
    };

#define MATCH_OP2_COPYREF_NODE(COMPUTEL,COMPUTER) \
    if (isFastCopyBytes(size)) { \
        return ccode.makeNodeUnrollNZ<SimNode_CopyRefValueFixed_##COMPUTEL##_##COMPUTER>(size); \
    } else { \
        return ccode.makeNode<SimNode_CopyRefValue_##COMPUTEL##_##COMPUTER>(); \
    }

#define MATCH_OP2_COPYREF_LEFT_ANY(NODENAME,COMPUTEL) \
    else if (is(info, node_l, NODENAME) ) { \
        anyRight = true; \
        MATCH_OP2_COPYREF_NODE(COMPUTEL,AnyPtr); \
    }

#define MATCH_OP2_COPYREF_RIGHT_ANY(NODENAME,COMPUTER) \
    else if (is(info, node_r, NODENAME) ) { \
        anyLeft = true; \
        MATCH_OP2_COPYREF_NODE(AnyPtr,COMPUTER); \
    }

#define MATCH_OP2_COPYREF(LNODENAME,RNODENAME,COMPUTEL,COMPUTER) \
    else if ( is2(info,node_l,node_r,LNODENAME,RNODENAME) ) { \
        MATCH_OP2_COPYREF_NODE(COMPUTEL,COMPUTER); \
    }

    // copy reference value
    //  *a = *b
    struct FusionPoint_MiscCopyRefValue : FusionPointOp2 {
        IMPLEMENT_OP2_COPYREF_NODE(AnyPtr, AnyPtr);
        IMPLEMENT_OP2_COPYREF_NODE(Local, AnyPtr);
        IMPLEMENT_OP2_COPYREF_NODE(AnyPtr, Local);
        IMPLEMENT_OP2_COPYREF_NODE(Local, Local);
        IMPLEMENT_OP2_COPYREF_NODE(Local, ThisBlockArgumentRef);
        IMPLEMENT_OP2_COPYREF_NODE(Local, ArgumentRefOff);
        IMPLEMENT_OP2_COPYREF_NODE(CMResOfs, AnyPtr);
        IMPLEMENT_OP2_COPYREF_NODE(CMResOfs, Local);
        IMPLEMENT_OP2_COPYREF_NODE(CMResOfs, Argument);
        IMPLEMENT_OP2_COPYREF_NODE(AnyPtr, CMResOfs);
        IMPLEMENT_OP2_COPYREF_NODE(LocalRefOff, AnyPtr);
        IMPLEMENT_OP2_COPYREF_NODE(AnyPtr, LocalRefOff);
        IMPLEMENT_OP2_COPYREF_NODE(LocalRefOff, Local);
        IMPLEMENT_OP2_COPYREF_NODE(LocalRefOff, ArgumentRefOff);
        IMPLEMENT_OP2_COPYREF_NODE(ArgumentRefOff, Local);
        IMPLEMENT_OP2_COPYREF_NODE(ArgumentRef, Argument);
        IMPLEMENT_OP2_COPYREF_NODE(ArgumentRef, AnyPtr);
        IMPLEMENT_OP2_COPYREF_NODE(AnyPtr, ArgumentRef);
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode * node, SimNode * node_l, SimNode * node_r, Context * context) override {
            auto crnode = static_cast<SimNode_CopyRefValue *> (node);
            uint32_t size = crnode->size;
            auto & ccode = *(context->code);
            if (false) { }
            // *, *
            MATCH_OP2_COPYREF("GetLocal","GetLocal",Local,Local)
            MATCH_OP2_COPYREF("GetLocal","GetThisBlockArgument",Local,ThisBlockArgumentRef)
            MATCH_OP2_COPYREF("GetLocal","GetArgumentRefOff",Local,ArgumentRefOff)
            MATCH_OP2_COPYREF("GetLocalRefOff","GetLocal",LocalRefOff,Local)
            MATCH_OP2_COPYREF("GetLocalRefOff","GetArgumentRefOff",LocalRefOff,ArgumentRefOff)
            MATCH_OP2_COPYREF("GetCMResOfs","GetLocal",CMResOfs,Local)
            MATCH_OP2_COPYREF("GetCMResOfs","GetArgumentRef",CMResOfs,Argument)
            MATCH_OP2_COPYREF("GetArgumentRefOff","GetLocal",ArgumentRefOff,Local)
            MATCH_OP2_COPYREF("GetArgument","GetArgumentRef",ArgumentRef,Argument)
            // *, any
            MATCH_OP2_COPYREF_LEFT_ANY("GetLocal",Local)
            MATCH_OP2_COPYREF_LEFT_ANY("GetCMResOfs",CMResOfs)
            MATCH_OP2_COPYREF_LEFT_ANY("GetLocalRefOff",LocalRefOff)
            MATCH_OP2_COPYREF_LEFT_ANY("GetArgument",ArgumentRef)
            // any, *
            MATCH_OP2_COPYREF_RIGHT_ANY("GetLocal",Local)
            MATCH_OP2_COPYREF_RIGHT_ANY("GetCMResOfs",CMResOfs)
            MATCH_OP2_COPYREF_RIGHT_ANY("GetLocalRefOff",LocalRefOff)
            MATCH_OP2_COPYREF_RIGHT_ANY("GetArgument",ArgumentRef)
            // fallback
            else {
                if (isFastCopyBytes(size)) {
                    anyLeft = anyRight = true;
                    ccode.makeNodeUnrollNZ<SimNode_CopyRefValueFixed_AnyPtr_AnyPtr>(size);
                }
            }
            return nullptr;
        }
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) override {
            auto opcresult = static_cast<SimNode_Op2FusionCopyRef *>(result);
            auto crnode = static_cast<SimNode_CopyRefValue *> (node);
            opcresult->set("CopyRefValue",Type::none,node->debugInfo);
            opcresult->size = crnode->size;
        }
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override {
            SimNode_CopyRefValue * crnode = static_cast<SimNode_CopyRefValue *>(node);
            return fuseOp2(info, node, crnode->l, crnode->r, context);
        }
    };

    void createFusionEngine_misc_copy_reference() {
        (*g_fusionEngine)["CopyReference"].emplace_back(new FusionPoint_MiscCopyReference());
        (*g_fusionEngine)["CopyRefValue"].emplace_back(new FusionPoint_MiscCopyRefValue());
    }
}

#endif

