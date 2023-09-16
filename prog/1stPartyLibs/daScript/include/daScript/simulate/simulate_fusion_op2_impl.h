#define MATCH_OP2(OPNAME,LNODENAME,RNODENAME,COMPUTEL,COMPUTER) \
    else if ( is2(info,node_l,node_r,LNODENAME,RNODENAME) ) { \
        return ccode.makeNode<SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER>(); \
    }

#define MATCH_OP2_ANYR(OPNAME,LNODENAME,COMPUTEL) \
    else if ( is(info,node_l,LNODENAME) ) { \
        anyRight = true; \
        return ccode.makeNode<SimNode_##OPNAME##_##COMPUTEL##_Any>(); \
    }

#define MATCH_OP2_ANYL(OPNAME,RNODENAME,COMPUTER) \
    else if ( is(info,node_r,RNODENAME) ) { \
        anyLeft = true; \
        return ccode.makeNode<SimNode_##OPNAME##_Any_##COMPUTER>(); \
    }

#if DAS_FUSION>=2

//  A opname B
#define IMPLEMENT_ANY_OP2(INLINE,OPNAME,TYPE,CTYPE) \
    struct FusionPoint_##OPNAME##_##CTYPE : FusionPointOp2 { \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Global,Local); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgumentRef,ThisBlockArgument); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgument,ThisBlockArgument); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgument,Argument); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Argument,Argument); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Argument,Local); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Argument,Const); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Argument,ArgumentRefOff); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff,LocalRefOff) \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff,Local) \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff,Argument) \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,ThisBlockArgumentRef); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,Local); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,Argument); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Local,Const); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Const,Local); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Const,LocalRefOff); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,Const,Argument); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff,ArgumentRefOff); \
        IMPLEMENT_OP2_NODE(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff,Argument); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,Argument); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,Argument); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgument); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgument); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,Const); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,Const); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,Local); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,Local); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff); \
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_l, SimNode * node_r, Context * context) override { \
            auto & ccode = *(context->code); \
            /* match op2 */ if ( !node_l || !node_r ) { return nullptr; } \
            MATCH_OP2(OPNAME,"GetThisBlockArgumentR2V","GetThisBlockArgument",ThisBlockArgumentRef,ThisBlockArgument) \
            MATCH_OP2(OPNAME,"GetThisBlockArgument","GetThisBlockArgument",ThisBlockArgument,ThisBlockArgument) \
            MATCH_OP2(OPNAME,"GetThisBlockArgument","GetArgument",ThisBlockArgument,Argument) \
            \
            MATCH_OP2(OPNAME,"GetArgument","GetArgument",Argument,Argument) \
            MATCH_OP2(OPNAME,"GetArgument","GetLocalR2V",Argument,Local) \
            MATCH_OP2(OPNAME,"GetArgument","ConstValue",Argument,Const) \
            MATCH_OP2(OPNAME,"GetArgument","GetArgumentRefOffR2V",Argument,ArgumentRefOff) \
            \
            MATCH_OP2(OPNAME,"GetLocalRefOffR2V","GetLocalRefOffR2V",LocalRefOff,LocalRefOff) \
            MATCH_OP2(OPNAME,"GetLocalRefOffR2V","GetLocalR2V",LocalRefOff,Local) \
            MATCH_OP2(OPNAME,"GetLocalRefOffR2V","GetArgument",LocalRefOff,Argument) \
            \
            MATCH_OP2(OPNAME,"GetLocalR2V","GetLocalR2V",Local,Local) \
            MATCH_OP2(OPNAME,"GetLocalR2V","GetThisBlockArgumentR2V",Local,ThisBlockArgumentRef) \
            MATCH_OP2(OPNAME,"GetLocalR2V","GetArgument",Local,Argument) \
            MATCH_OP2(OPNAME,"GetLocalR2V","ConstValue",Local,Const) \
            \
            MATCH_OP2(OPNAME,"ConstValue","GetLocalR2V",Const,Local) \
            MATCH_OP2(OPNAME,"ConstValue","GetLocalRefOffR2V",Const,LocalRefOff) \
            MATCH_OP2(OPNAME,"ConstValue","GetArgument",Const,Argument) \
            \
            MATCH_OP2(OPNAME,"GetArgumentRefOffR2V","GetArgumentRefOffR2V",ArgumentRefOff,ArgumentRefOff) \
            MATCH_OP2(OPNAME,"GetArgumentRefOffR2V","GetArgument",ArgumentRefOff,Argument) \
            \
            MATCH_OP2_ANYR(OPNAME,"GetArgument",Argument) \
            MATCH_OP2_ANYL(OPNAME,"GetArgument",Argument) \
            MATCH_OP2_ANYR(OPNAME,"GetArgumentRefOffR2V",ArgumentRefOff) \
            MATCH_OP2_ANYL(OPNAME,"GetArgumentRefOffR2V",ArgumentRefOff) \
            MATCH_OP2_ANYR(OPNAME,"GetThisBlockArgument",ThisBlockArgument) \
            MATCH_OP2_ANYL(OPNAME,"GetThisBlockArgument",ThisBlockArgument) \
            MATCH_OP2_ANYR(OPNAME,"ConstValue",Const) \
            MATCH_OP2_ANYL(OPNAME,"ConstValue",Const) \
            MATCH_OP2_ANYR(OPNAME,"GetLocalR2V",Local) \
            MATCH_OP2_ANYL(OPNAME,"GetLocalR2V",Local) \
            MATCH_OP2_ANYR(OPNAME,"GetLocalRefOffR2V",LocalRefOff) \
            MATCH_OP2_ANYL(OPNAME,"GetLocalRefOffR2V",LocalRefOff) \
            return nullptr; \
        } \
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) override { \
            result->set(#OPNAME,Type(ToBasicType<CTYPE>::type),node->debugInfo); \
            IMPLEMENT_OP2_SETUP_NODE(result,node); \
        } \
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override { \
            return fuseOp2(info, node, FUSION_OP2_SUBEXPR_LEFT(CTYPE,node), FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node), context); \
        } \
    };

#else

//  A opname B
#define IMPLEMENT_ANY_OP2(INLINE,OPNAME,TYPE,CTYPE) \
    struct FusionPoint_##OPNAME##_##CTYPE : FusionPointOp2 { \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,Argument); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,Argument); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,ArgumentRefOff); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgument); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,ThisBlockArgument); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,Const); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,Const); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,Local); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,Local); \
        IMPLEMENT_OP2_NODE_ANYR(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff); \
        IMPLEMENT_OP2_NODE_ANYL(INLINE,OPNAME,TYPE,CTYPE,LocalRefOff); \
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_l, SimNode * node_r, Context * context) override { \
            auto & ccode = *(context->code); \
            /* match op2 */ if ( !node_l || !node_r ) { return nullptr; } \
            MATCH_OP2_ANYR(OPNAME,"GetArgument",Argument) \
            MATCH_OP2_ANYL(OPNAME,"GetArgument",Argument) \
            MATCH_OP2_ANYR(OPNAME,"GetArgumentRefOffR2V",ArgumentRefOff) \
            MATCH_OP2_ANYL(OPNAME,"GetArgumentRefOffR2V",ArgumentRefOff) \
            MATCH_OP2_ANYR(OPNAME,"GetThisBlockArgument",ThisBlockArgument) \
            MATCH_OP2_ANYL(OPNAME,"GetThisBlockArgument",ThisBlockArgument) \
            MATCH_OP2_ANYR(OPNAME,"ConstValue",Const) \
            MATCH_OP2_ANYL(OPNAME,"ConstValue",Const) \
            MATCH_OP2_ANYR(OPNAME,"GetLocalR2V",Local) \
            MATCH_OP2_ANYL(OPNAME,"GetLocalR2V",Local) \
            MATCH_OP2_ANYR(OPNAME,"GetLocalRefOffR2V",LocalRefOff) \
            MATCH_OP2_ANYL(OPNAME,"GetLocalRefOffR2V",LocalRefOff) \
            return nullptr; \
        } \
        virtual void set(SimNode_Op2Fusion * result, SimNode * node) override { \
            result->set(#OPNAME,Type(ToBasicType<CTYPE>::type),node->debugInfo); \
            IMPLEMENT_OP2_SETUP_NODE(result,node); \
        } \
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override { \
            return fuseOp2(info, node, FUSION_OP2_SUBEXPR_LEFT(CTYPE,node), FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node), context); \
        } \
    };

#endif
