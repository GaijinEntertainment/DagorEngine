// when adding more visitor macros, make sure op_undef is updated

#define V_BEGIN()           vis.preVisit(this);
#define V_BEGIN_CR()        vis.preVisit(this); vis.cr();
#define V_END()             return vis.visit(this);
#define V_OP(x)             vis.op(#x);
#define V_OP_TT(x)          vis.op(#x, sizeof(TT), typeName<TT>::name());
#define V_SP(x)             vis.sp(x);
#define V_SP_EX(x)          vis.sp(x,#x);
#define V_ARG(x)            vis.arg(x,#x);
#define V_SUB(x)            x = vis.sub(x,#x);
#define V_ARG_THIS(x)       vis.arg(this->x,#x);
#define V_SUB_THIS(x)       this->x = vis.sub(this->x,#x);
#define V_SUB_OPT(x)        x = x ? vis.sub(x,#x) : nullptr;
#define V_CALL()            visitCall(vis);
#define V_FINAL()           visitFinal(vis);
#define V_BLOCK()           visitBlock(vis);
#define V_LABELS()          visitLabels(vis);




