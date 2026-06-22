#define IMPLEMENT_SET_OP1_FUSION_POINT(INLINE,OPNAME,TYPE,CTYPE,RCTYPE) \
    struct Op1FusionPoint_##OPNAME##_##CTYPE : FusionPointOp1 { \
        Op1FusionPoint_##OPNAME##_##CTYPE ( ) {} \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,Global); \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,Local); \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,CMResOfs); \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,LocalRefOff); \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,ArgumentRef); \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,ArgumentRefOff); \
        IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,ThisBlockArgumentRef); \
        virtual SimNode * match(const SimNodeInfoLookup & info, SimNode *, SimNode * node_x, Context * context) override { \
            auto & ccode = *(context->code); \
            if ( !node_x ) { return nullptr; } \
            MATCH_ANY_OP1_NODE(CTYPE,"GetGlobal",Global) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetLocal",Local) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetCMResOfs",CMResOfs) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetLocalRefOff",LocalRefOff) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetArgument",ArgumentRef) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetArgumentRefOff",ArgumentRefOff) \
            MATCH_ANY_OP1_NODE(CTYPE,"GetThisBlockArgument",ThisBlockArgumentRef) \
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

