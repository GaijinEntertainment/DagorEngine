#define IMPLEMENT_ANY_OP1_FUSION_POINT(INLINE,OPNAME,TYPE,CTYPE,RCTYPE) \
    struct Op1FusionPoint_##OPNAME##_##CTYPE : FusionPointOp1 { \
        Op1FusionPoint_##OPNAME##_##CTYPE ( ) {} \
        IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,Const); \
        IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,Local); \
        IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,Argument); \
        IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,ArgumentRef); \
        IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,ArgumentRefOff); \
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_x, Context * context) override { \
            auto & ccode = *(context->code); \
            if ( !node_x ) { return nullptr; } \
            MATCH_ANY_OP1_NODE(CTYPE,"ConstValue",Const) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetLocalR2V",Local) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetArgument",Argument) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetArgumentR2V",ArgumentRef) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetArgumentRefOffR2V",ArgumentRefOff) \
            return nullptr; \
        } \
        virtual void set(SimNode_Op1Fusion * result, SimNode * node) override { \
            result->set(#OPNAME,Type(ToBasicType<CTYPE>::type),node->debugInfo); \
            IMPLEMENT_OP1_SETUP_NODE(result,node); \
        } \
        virtual SimNode * fuse(const SimNodeInfoLookup & info, SimNode * node, Context * context) override { \
            return fuseOp1(info, node, FUSION_OP1_SUBEXPR(CTYPE,node), context); \
        } \
    };

