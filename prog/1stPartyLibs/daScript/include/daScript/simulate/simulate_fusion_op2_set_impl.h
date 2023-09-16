#define MATCH_OP2_SET(OPNAME,LNODENAME,RNODENAME,COMPUTEL,COMPUTER) \
    else if ( is2(info,node_l,node_r,LNODENAME,RNODENAME) ) { \
        return ccode.makeNode<SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER>(); \
    }

#define MATCH_OP2_SET_ANY(OPNAME,LNODENAME,COMPUTEL) \
    else if ( is(info, node_l,LNODENAME) ) { \
        anyRight = true; \
        return ccode.makeNode<SimNode_##OPNAME##_##COMPUTEL##_Any>(); \
    }

#if DAS_FUSION>=2

//  a SetOPNAME b
#define IMPLEMENT_ANY_SETOP(INLINE,OPNAME,TYPE,CTYPE,RCTYPE) \
    struct FusionPoint_Set_##OPNAME##_##CTYPE : FusionPointOp2 { \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,CMResOfs,Const); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,Global,Local); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,Const); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,Local); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,LocalRefOff); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff,LocalRefOff); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff,Argument); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff,Const); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff,ArgumentRefOff); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff,Local); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRef,Const); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRef,Argument); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRef,LocalRefOff); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRef,Local); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgumentRef,ThisBlockArgument); \
        IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgumentRef,ThisBlockArgumentRef); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,Global); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,Local); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,CMResOfs); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,ArgumentRef); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgumentRef); \
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_l, SimNode * node_r, Context * context) override { \
            auto & ccode = *(context->code); \
            /* match op2 */ if ( !node_l || !node_r ) { return nullptr; } \
            MATCH_OP2_SET(OPNAME,"GetCMResOfs","ConstValue",CMResOfs,Const) \
            MATCH_OP2_SET(OPNAME,"GetGlobal","GetLocalR2V",Global,Local) \
            MATCH_OP2_SET(OPNAME,"GetLocal","ConstValue",Local,Const) \
            MATCH_OP2_SET(OPNAME,"GetLocal","GetLocalR2V",Local,Local) \
            MATCH_OP2_SET(OPNAME,"GetLocal","GetLocalRefOffR2V",Local,LocalRefOff) \
            MATCH_OP2_SET(OPNAME,"GetLocalRefOff","GetLocalRefOffR2V",LocalRefOff,LocalRefOff) \
            MATCH_OP2_SET(OPNAME,"GetLocalRefOff","GetArgument",LocalRefOff,Argument) \
            MATCH_OP2_SET(OPNAME,"GetLocalRefOff","ConstValue",LocalRefOff,Const) \
            MATCH_OP2_SET(OPNAME,"GetArgumentRefOff","GetArgumentRefOffR2V",ArgumentRefOff,ArgumentRefOff) \
            MATCH_OP2_SET(OPNAME,"GetArgumentRefOff","GetLocalR2V",ArgumentRefOff,Local) \
            MATCH_OP2_SET(OPNAME,"GetArgument","ConstValue",ArgumentRef,Const) \
            MATCH_OP2_SET(OPNAME,"GetArgument","GetArgument",ArgumentRef,Argument) \
            MATCH_OP2_SET(OPNAME,"GetArgument","GetLocalRefOffR2V",ArgumentRef,LocalRefOff) \
            MATCH_OP2_SET(OPNAME,"GetArgument","GetLocalR2V",ArgumentRef,Local) \
            MATCH_OP2_SET(OPNAME,"GetThisBlockArgument","GetThisBlockArgument",ThisBlockArgumentRef,ThisBlockArgument) \
            MATCH_OP2_SET(OPNAME,"GetThisBlockArgument","GetThisBlockArgumentR2V",ThisBlockArgumentRef,ThisBlockArgumentRef) \
            MATCH_OP2_SET_ANY(OPNAME,"GetGlobal",Global) \
            MATCH_OP2_SET_ANY(OPNAME,"GetLocal",Local) \
            MATCH_OP2_SET_ANY(OPNAME,"GetCMResOfs",CMResOfs) \
            MATCH_OP2_SET_ANY(OPNAME,"GetLocalRefOff",LocalRefOff) \
            MATCH_OP2_SET_ANY(OPNAME,"GetArgument",ArgumentRef) \
            MATCH_OP2_SET_ANY(OPNAME,"GetArgumentRefOff",ArgumentRefOff) \
            MATCH_OP2_SET_ANY(OPNAME,"GetThisBlockArgument",ThisBlockArgumentRef) \
            return nullptr; \
        } \
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) override { \
            result->set(#OPNAME,Type(ToBasicType<CTYPE>::type),node->debugInfo); \
            IMPLEMENT_OP2_SET_SETUP_NODE(result,node); \
        } \
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override { \
            return fuseOp2(info, node, FUSION_OP2_SUBEXPR_LEFT(CTYPE,node), FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node), context); \
        } \
    };

#else

//  a SetOPNAME b
#define IMPLEMENT_ANY_SETOP(INLINE,OPNAME,TYPE,CTYPE,RCTYPE) \
    struct FusionPoint_Set_##OPNAME##_##CTYPE : FusionPointOp2 { \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,Global); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,Local); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,CMResOfs); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,ArgumentRef); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff); \
        IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgumentRef); \
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_l, SimNode * node_r, Context * context) override { \
            auto & ccode = *(context->code); \
            /* match op2 */ if ( !node_l || !node_r ) { return nullptr; } \
            MATCH_OP2_SET_ANY(OPNAME,"GetGlobal",Global) \
            MATCH_OP2_SET_ANY(OPNAME,"GetLocal",Local) \
            MATCH_OP2_SET_ANY(OPNAME,"GetCMResOfs",CMResOfs) \
            MATCH_OP2_SET_ANY(OPNAME,"GetLocalRefOff",LocalRefOff) \
            MATCH_OP2_SET_ANY(OPNAME,"GetArgument",ArgumentRef) \
            MATCH_OP2_SET_ANY(OPNAME,"GetArgumentRefOff",ArgumentRefOff) \
            MATCH_OP2_SET_ANY(OPNAME,"GetThisBlockArgument",ThisBlockArgumentRef) \
            return nullptr; \
        } \
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) override { \
            result->set(#OPNAME,Type(ToBasicType<CTYPE>::type),node->debugInfo); \
            IMPLEMENT_OP2_SET_SETUP_NODE(result,node); \
        } \
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override { \
            return fuseOp2(info, node, FUSION_OP2_SUBEXPR_LEFT(CTYPE,node), FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node), context); \
        } \
    };

#endif