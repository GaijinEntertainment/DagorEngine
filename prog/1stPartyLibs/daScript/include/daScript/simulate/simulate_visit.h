#pragma once

namespace das {

    template <typename TRange>
    SimNode * SimNode_ForRange<TRange>::visit ( SimVisitor & vis ) {
        using TT = typename TRange::baseType;
        V_BEGIN_CR();
        V_OP_TT(ForRange);
        V_SP(this->stackTop[0]);
        V_SUB(this->sources[0]);
        vis.sub(this->list,this->total,"list");
        V_FINAL();
        V_END();
    }

    template <typename TRange>
    SimNode * SimNode_ForRangeNF<TRange>::visit ( SimVisitor & vis ) {
        using TT = typename TRange::baseType;
        V_BEGIN_CR();
        V_OP_TT(ForRangeNF);
        V_SP(this->stackTop[0]);
        V_SUB(this->sources[0]);
        vis.sub(this->list,this->total,"list");
        V_FINAL();
        V_END();
    }

    template <typename TRange>
    SimNode * SimNode_ForRange1<TRange>::visit ( SimVisitor & vis ) {
        using TT = typename TRange::baseType;
        V_BEGIN_CR();
        V_OP_TT(ForRange1);
        V_SP(this->stackTop[0]);
        V_SUB(this->sources[0]);
        V_SUB(this->list[0]);
        V_FINAL();
        V_END();
    }

    template <typename TRange>
    SimNode * SimNode_ForRangeNF1<TRange>::visit ( SimVisitor & vis ) {
        using TT = typename TRange::baseType;
        V_BEGIN_CR();
        V_OP_TT(ForRangeNF1);
        V_SP(this->stackTop[0]);
        V_SUB(this->sources[0]);
        V_SUB(this->list[0]);
        V_FINAL();
        V_END();
    }

    template <typename TRange>
    SimNode * SimNode_RangeIterator<TRange>::visit ( SimVisitor & vis ) {
        using TT = typename TRange::baseType;
        V_BEGIN();
        V_OP_TT(RangeIterator);
        V_SUB(subexpr);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_NewHandle<TT,false>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(NewHandle);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_NewHandle<TT,true>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(NewSmartHandle);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_DeleteHandlePtr<TT,false>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(DeleteHandlePtr);
        V_SUB(subexpr);
        V_ARG(total);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_DeleteHandlePtr<TT,true>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(DeleteSmartHandlePtr);
        V_SUB(subexpr);
        V_ARG(total);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_FieldDerefR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(FieldDerefR2V);
        V_SUB(value);
        V_ARG(offset);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_VariantFieldDerefR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(FieldDerefR2V);
        V_SUB(value);
        V_ARG(offset);
        V_ARG(variant);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_PtrFieldDerefR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(PtrFieldDerefR2V);
        V_SUB(subexpr);
        V_ARG(offset);
        V_END();
    }

    template <typename CastTo, typename CastFrom>
    SimNode * SimNode_Cast<CastTo,CastFrom>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        string opName = string("Cast_to_") + typeName<CastTo>::name();
        vis.op(opName.c_str(), sizeof(CastFrom), typeName<CastFrom>::name());
        V_SUB(arguments[0]);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_LexicalCast<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(LexicalCast);
        V_SUB(arguments[0]);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_AtR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(AtR2V);
        V_SUB(value);
        V_SUB(index);
        V_ARG(stride);
        V_ARG(offset);
        V_ARG(range);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetCMResOfsR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetCMResOfsR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetLocalR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetLocalR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetLocalRefOffR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetLocalRefOffR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetArgumentR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetArgumentR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetArgumentRefOffR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetArgumentRefOffR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetBlockArgumentR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetBlockArgumentR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetThisBlockArgumentR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetThisBlockArgumentR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetGlobalR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetGlobalR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetSharedR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetSharedR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetGlobalMnhR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetGlobalMnhR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_GetSharedMnhR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(GetSharedMnhR2V);
        subexpr.visit(vis);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_Ref2Value<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(Ref2Value);
        V_SUB(subexpr);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_NullCoalescing<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(NullCoalescing);
        V_SUB(subexpr);
        V_SUB(value);
        V_END();
    }

    template <bool move>
    SimNode * SimNode_Ascend<move>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Ascend);
        V_SUB(subexpr);
        V_ARG(bytes);
        V_ARG(persistent);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_Set<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(Set);
        V_SUB(l);
        V_SUB(r);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_CloneRefValueT<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(CloneRefValue);
        V_SUB(l);
        V_SUB(r);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_IfZeroThenElse<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP_TT(IfZeroThenElse);
        V_SUB(cond);
        V_SUB(if_true);
        V_SUB(if_false);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_IfNotZeroThenElse<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP_TT(IfNotZeroThenElse);
        V_SUB(cond);
        V_SUB(if_true);
        V_SUB(if_false);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_IfZeroThen<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP_TT(IfZeroThen);
        V_SUB(cond);
        V_SUB(if_true);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_IfNotZeroThen<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP_TT(IfNotZeroThen);
        V_SUB(cond);
        V_SUB(if_true);
        V_END();
    }

    template <typename TT>
    SimNode * SimNode_ReturnAndMoveR2V<TT>::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP_TT(ReturnAndMoveR2V);
        V_SUB(subexpr);
        V_END();
    }
}
