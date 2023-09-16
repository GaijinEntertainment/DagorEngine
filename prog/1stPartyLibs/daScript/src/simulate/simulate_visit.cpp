#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/runtime_range.h"
#include "daScript/simulate/runtime_table_nodes.h"
#include "daScript/simulate/runtime_array.h"
#include "daScript/simulate/runtime_string_delete.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    void SimSource::visit(SimVisitor & vis) {
        switch (type) {
        case SimSourceType::sSimNode:
            V_SUB(subexpr);
            break;
        case SimSourceType::sConstValue:
            if ( isStringConstant ) {
                V_ARG(valuePtr);
            } else {
                V_ARG(value);
            }
            break;
        case SimSourceType::sCMResOff:
            V_ARG(offset);
            break;
        case SimSourceType::sGlobal:
            V_ARG(mangledNameHash);
            break;
        case SimSourceType::sShared:
            V_ARG(mangledNameHash);
            break;
        case SimSourceType::sBlockCMResOff:
            V_SP(argStackTop);
            V_ARG(offset);
            break;
        case SimSourceType::sLocal:
            V_SP(stackTop);
            break;
        case SimSourceType::sLocalRefOff:
            V_SP(stackTop);
            V_ARG(offset);
            break;
        case SimSourceType::sArgument:
        case SimSourceType::sArgumentRef:
        case SimSourceType::sThisBlockArgument:
            V_SP(index);
            break;
        case SimSourceType::sArgumentRefOff:
        case SimSourceType::sThisBlockArgumentRef:
            V_SP(index);
            V_ARG(offset);
            break;
        case SimSourceType::sBlockArgument:
            V_SP(argStackTop);
            V_SP(index);
            break;
        case SimSourceType::sBlockArgumentRef:
            V_SP(argStackTop);
            V_SP(index);
            V_ARG(offset);
            break;
        }
    }

    SimNode * SimNode::visit ( SimVisitor & vis ) {
        V_BEGIN();
        vis.op("??");
        V_END();
    }

    void SimVisitor::sub ( SimNode ** nodes, uint32_t count, const char * ) {
        for ( uint32_t t=0; t!=count; ++t ) {
            nodes[t] = nodes[t]->visit(*this);
        }
    }

    void SimNode_CallBase::visitCall ( SimVisitor & vis ) {
        if ( fnPtr ) {
            vis.arg(fnPtr->name,"fnPtr");
            vis.arg(Func(), fnPtr->mangledName, "fnIndex");
        }
        if ( cmresEval ) {
            V_SUB(cmresEval);
        }
        vis.sub(arguments, nArguments, "arguments");
    }

    SimNode* SimNode_FastCallAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(FastCall);
        V_CALL();
        V_END();
    }

    SimNode * SimNode_CallAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(Call);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_CallAndCopyOrMoveAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(CallAndCopyOrMove);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(Invoke);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeAndCopyOrMoveAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(InvokeAndCopyOrMove);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeFnAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(InvokeFn);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeFnByNameAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(InvokeFnByName);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeLambdaAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(InvokeLambda);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeAndCopyOrMoveFnAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(InvokeAndCopyOrMoveFn);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_InvokeAndCopyOrMoveLambdaAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(InvokeAndCopyOrMoveLambda);
        V_CALL();
        V_END();
    }

    SimNode* SimNode_NewWithInitializerAny::visit(SimVisitor& vis) {
        V_BEGIN();
        V_OP(NewWithInitializer);
        V_CALL();
        V_ARG(bytes);
        V_ARG(persistent);
        V_END();
    }

    SimNode * SimNode_Jit::visit ( SimVisitor & vis ) {
        uint64_t fptr = (uint64_t) func;
        V_BEGIN();
        V_OP(Jit);
        V_ARG(fptr);
        V_END();
    }


    SimNode * SimNode_JitBlock::visit ( SimVisitor & vis ) {
        uint64_t fptr = (uint64_t) func;
        V_BEGIN();
        V_OP(JitBlock);
        V_ARG(fptr);
        V_END();
    }

    SimNode * SimNode_NOP::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(NOP);
        V_END();
    }

    SimNode * SimNode_DeleteStructPtr::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(DeleteStructPtr);
        V_ARG(total);
        V_ARG(structSize);
        V_ARG(persistent);
        V_ARG(isLambda);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNode_DeleteClassPtr::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(DeleteClassPtr);
        V_ARG(total);
        V_SUB(sizeexpr);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNode_DeleteLambda::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(DeleteLambda);
        V_ARG(total);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNode_MakeBlock::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(MakeBlock);
        V_SUB(subexpr);
        V_SP(stackTop);
        V_SP_EX(argStackTop);
        V_END();
    }

    SimNode * SimNode_Assert::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Assert);
        V_SUB(subexpr);
        V_ARG(message);
        V_END();
    }

    SimNode * SimNode_Swizzle::visit ( SimVisitor & vis ) {
        const char * xyzw = "xyzw";
        char swizzle[5] = { xyzw[fields[0]], xyzw[fields[1]], xyzw[fields[2]], xyzw[fields[3]], 0 };
        V_BEGIN();
        V_OP(Swizzle);
        V_SUB(value);
        V_ARG(swizzle);
        V_END();
    }

    SimNode * SimNode_FieldDeref::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(FieldDeref);
        V_SUB(value);
        V_ARG(offset);
        V_END();
    }

    SimNode * SimNode_VariantFieldDeref::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(VariantFieldDeref);
        V_SUB(value);
        V_ARG(offset);
        V_ARG(variant);
        V_END();
    }

    SimNode * SimNode_SafeVariantFieldDeref::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SafeVariantFieldDeref);
        V_SUB(value);
        V_ARG(offset);
        V_ARG(variant);
        V_END();
    }

    SimNode * SimNode_SafeVariantFieldDerefPtr::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SafeVariantFieldDerefPtr);
        V_SUB(value);
        V_ARG(offset);
        V_ARG(variant);
        V_END();
    }

    SimNode * SimNode_PtrFieldDeref::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(PtrFieldDeref);
        V_SUB(subexpr);
        V_ARG(offset);
        V_END();
    }

    SimNode * SimNode_SafeFieldDeref::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SafeFieldDeref);
        V_SUB(value);
        V_ARG(offset);
        V_END();
    }

    SimNode * SimNode_SafeFieldDerefPtr::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SafeFieldDerefPtr);
        V_SUB(value);
        V_ARG(offset);
        V_END();
    }

    SimNode * SimNode_IsVariant::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(IsVariant);
        V_SUB(subexpr);
        V_ARG(variant);
        V_END();
    }

    SimNode * SimNode_At::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(At);
        V_SUB(value);
        V_SUB(index);
        V_ARG(stride);
        V_ARG(offset);
        V_ARG(range);
        V_END();
    }

    SimNode * SimNode_SafeAt::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SafeAt);
        V_SUB(value);
        V_SUB(index);
        V_ARG(stride);
        V_ARG(offset);
        V_ARG(range);
        V_END();
    }

    SimNode * SimNode_StringBuilder::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(StringBuilder);
        // TODO: types?
        V_CALL();
        V_END();
    }

    SimNode * SimNode_Debug::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Debug);
        V_SUB(subexpr);
        string dt = debug_type(typeInfo);
        V_ARG(dt.c_str());
        V_ARG(message);
        V_END();
    }

    SimNode * SimNode_TypeInfo::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(TypeInfo);
        string dt = debug_type(typeInfo);
        V_ARG(dt.c_str());
        V_END();
    }

    SimNode * SimNode_GetCMResOfs::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetCMResOfs);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetBlockCMResOfs::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetBlockCMResOfs);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_SetLocalRefAndEval::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SetLocalRefAndEval);
        V_SP(stackTop);
        V_SUB(refValue);
        V_SUB(evalValue);
        V_END();
    }

    SimNode * SimNode_GetLocal::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetLocal);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetLocalRefOff::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetLocalRefOff);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_MemZero::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(MemZero);
        V_SUB(subexpr);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_InitLocal::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(InitLocal);
        V_SP(stackTop);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_InitLocalCMRes::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(InitLocalCMRes);
        V_SP(offset);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_InitLocalRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(InitLocalRef);
        V_SP(stackTop);
        V_SP(offset);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_GetArgument::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetArgument);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetArgumentRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetArgumentRef);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetArgumentRefOff::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetArgumentRefOff);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetBlockArgument::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetBlockArgument);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetBlockArgumentRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetBlockArgumentRef);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetThisBlockArgument::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetThisBlockArgument);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetThisBlockArgumentRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetThisBlockArgumentRef);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetGlobal::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetGlobal);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetShared::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetShared);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetGlobalMnh::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetGlobalMnh);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_GetSharedMnh::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetSharedMnh);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_TryCatch::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(TryCatch);
        V_SUB(try_block);
        V_SUB(catch_block);
        V_END();
    }

    SimNode * SimNode_Return::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Return);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_ReturnNothing::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnNothing);
        V_END();
    }

    SimNode * SimNode_ReturnConst::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnConst);
        V_ARG(value);
        V_END();
    }

    SimNode * SimNode_ReturnConstString::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnConstString);
        V_ARG(value);
        V_END();
    }

    SimNode * SimNode_ReturnRefAndEval::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnRefAndEval);
        V_SP(stackTop);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_ReturnAndCopy::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnAndCopy);
        V_SUB_OPT(subexpr);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_ReturnAndMove::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnAndMove);
        V_SUB_OPT(subexpr);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_ReturnReference::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnReference);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_ReturnRefAndEvalFromBlock::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnRefAndEvalFromBlock);
        V_SP(stackTop);
        V_SP(argStackTop);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_ReturnAndCopyFromBlock::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnAndCopyFromBlock);
        V_SUB_OPT(subexpr);
        V_ARG(size);
        V_SP_EX(argStackTop);
        V_END();
    }

    SimNode * SimNode_ReturnAndMoveFromBlock::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnAndMoveFromBlock);
        V_SUB_OPT(subexpr);
        V_ARG(size);
        V_SP_EX(argStackTop);
        V_END();
    }

    SimNode * SimNode_ReturnReferenceFromBlock::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ReturnReferenceFromBlock);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_GotoLabel::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GotoLabel);
        V_ARG(label);
        V_END();
    }

    SimNode * SimNode_Goto::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Goto);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNode_Break::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Break);
        V_END();
    }

    SimNode * SimNode_Continue::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Continue);
        V_END();
    }

    SimNode * SimNode_Ptr2Ref::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Ptr2Ref);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_Seq2Iter::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Seq2Iter);
        V_SUB_OPT(subexpr);
        V_END();
    }

    SimNode * SimNode_NullCoalescingRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(NullCoalescingRef);
        V_SUB(subexpr);
        V_SUB(value);
        V_END();
    }

    SimNode * SimNode_Delete::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Delete);
        V_ARG(total);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNode_AscendAndRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(AscendAndRef);
        V_SUB(subexpr);
        V_ARG(bytes);
        V_ARG(typeInfo ? typeInfo->hash : 0);
        V_ARG(persistent);
        V_SP(stackTop);
        V_END();
    }

    SimNode * SimNode_AscendNewHandleAndRef::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(AscendNewHandleAndRef);
        V_SUB(newexpr);
        V_SUB(subexpr);
        V_ARG(bytes);
        V_SP(stackTop);
        V_END();
    }

    SimNode * SimNode_New::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(New);
        V_ARG(bytes);
        V_ARG(persistent);
        V_END();
    }

    SimNode * SimNode_NewArray::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(NewArray);
        V_SUB(newNode);
        V_SP(stackTop);
        V_ARG(count);
        V_END();
    }

    SimNode * SimNode_ConstValue::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ConstValue);
        subexpr.visit(vis);
        V_END();
    }

    SimNode * SimNode_FuncConstValueMnh::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(FuncAddrMnh);
        vis.arg(Func(),subexpr.valueU,"mnh");
        V_END();
    }

    SimNode * SimNode_FuncConstValue::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(FuncAddr);
        vis.arg(Func(),subexpr.valueU,"index");
        V_END();
    }

    SimNode * SimNode_ConstString::visit ( SimVisitor & vis ) {
        using TT = char *;
        V_BEGIN();
        V_OP_TT(ConstValue);
        V_ARG(subexpr.valuePtr);
        V_END();
    }

    SimNode * SimNode_Zero::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Zero);
        V_END();
    }

    SimNode * SimNode_CopyReference::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(CopyReference);
        V_SUB(l);
        V_SUB(r);
        V_END();
    }

    SimNode * SimNode_CopyRefValue::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(CopyRefValue);
        V_SUB(l);
        V_SUB(r);
        V_ARG(size);
        V_END();
    }

    SimNode * SimNode_MoveRefValue::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(MoveRefValue);
        V_SUB(l);
        V_SUB(r);
        V_ARG(size);
        V_END();
    }

    void SimNode_Final::visitFinal ( SimVisitor & vis ) {
        vis.sub(finalList, totalFinal, "final");
    }

    SimNode * SimNode_Final::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(Final);
        V_FINAL()
        V_END();
    }

    void SimNode_Block::visitBlock ( SimVisitor & vis ) {
        vis.sub(list, total, "block");
    }

    void SimNode_Block::visitLabels ( SimVisitor & vis ) {
        if ( labels ) {
            V_ARG(totalLabels);
            for ( uint32_t i=0, is=totalLabels; i!=is; ++i ) {
                if ( labels[i]!=-1U ) {
                    char name[32];
                    snprintf(name, 32, "label_%i", i);
                    vis.arg(labels[i], name);
                }
            }
        }
    }

    SimNode * SimNode_Block::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(Block);
        V_BLOCK();
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_BlockWithLabels::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(BlockWithLabels);
        V_LABELS();
        V_BLOCK();
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_ClosureBlock::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(Block);
        V_BLOCK();
        V_FINAL();
        V_ARG(needResult);
        V_ARG(annotationDataSid);
        V_END();
    }

    SimNode * SimNode_MakeLocal::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(MakeLocal);
        V_SP(stackTop);
        V_BLOCK();
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_MakeLocalCMRes::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(MakeLocalCMRes);
        V_BLOCK();
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_ReturnLocalCMRes::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(ReturnLocalCMRes);
        V_BLOCK();
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_Let::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(Let);
        V_BLOCK();
        V_FINAL();
        V_END();
    }

#if DAS_DEBUGGER
    SimNode * SimNodeDebug_Instrument::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Instrument);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNodeDebug_InstrumentFunction::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(Instrument);
        vis.arg(func->name,"fnPtr");
        vis.arg(Func(), func->mangledName, "fnIndex");
        V_SUB(subexpr);
        V_END();
    }
#endif

    SimNode * SimNode_IfThenElse::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(IfThenElse);
        V_SUB(cond);
        V_SUB(if_true);
        V_SUB(if_false);
        V_END();
    }

    SimNode * SimNode_IfThen::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(IfThen);
        V_SUB(cond);
        V_SUB(if_true);
        V_END();
    }

    SimNode * SimNode_While::visit ( SimVisitor & vis ) {
        V_BEGIN_CR();
        V_OP(While);
        V_SUB(cond);
        vis.sub(list,total,"list");
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_ForBase::visitFor ( SimVisitor & vis, int totalC, const char * loopName ) {
        char nbuf[32];
        V_BEGIN_CR();
        snprintf(nbuf, sizeof(nbuf), "%s_%i", loopName, total );
        vis.op(nbuf);
        for ( int t=0; t!=totalC; ++t ) {
            snprintf(nbuf, sizeof(nbuf), "stackTop[%i]", t );
            vis.sp(stackTop[t],nbuf);
            snprintf(nbuf, sizeof(nbuf), "strides[%i]", t );
            vis.arg(strides[t],nbuf);
            sources[t] = vis.sub(sources[t]);
        }
        vis.sub(list,total,"list");
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_ForWithIteratorBase::visitFor ( SimVisitor & vis, int totalC ) {
        char nbuf[32];
        V_BEGIN_CR();
        snprintf(nbuf, sizeof(nbuf), "ForWithIterator_%i", total );
        vis.op(nbuf);
        for ( int t=0; t!=totalC; ++t ) {
            snprintf(nbuf, sizeof(nbuf), "stackTop[%i]", t );
            vis.sp(stackTop[t],nbuf);
            source_iterators[t] = vis.sub(source_iterators[t]);
        }
        vis.sub(list,total,"list");
        V_FINAL();
        V_END();
    }

    SimNode * SimNode_ForWithIteratorBase::visit ( SimVisitor & vis ) {
        return visitFor(vis, totalSources);
    }

    SimNode * SimNode_Op1::visitOp1 ( SimVisitor & vis, const char * op, int typeSize, const string & typeName ) {
        V_BEGIN();
        vis.op(op, typeSize, typeName);
        V_SUB(x);
        V_END();
    }

    SimNode * SimNode_Op2::visitOp2 ( SimVisitor & vis, const char * op, int typeSize, const string & typeName ) {
        V_BEGIN();
        vis.op(op, typeSize, typeName);
        V_SUB(l);
        V_SUB(r);
        V_END();
    }

    SimNode * SimNode_CallBase::visitOp1 ( SimVisitor & vis, const char * op, int typeSize, const char * typeName ) {
        V_BEGIN();
        vis.op(op, typeSize, typeName);
        V_SUB(arguments[0]);
        V_END();
    }

    SimNode * SimNode_CallBase::visitOp2 ( SimVisitor & vis, const char * op, int typeSize, const char * typeName ) {
        V_BEGIN();
        vis.op(op, typeSize, typeName);
        V_SUB(arguments[0]);
        V_SUB(arguments[1]);
        V_END();
    }

    SimNode * SimNode_CallBase::visitOp3 ( SimVisitor & vis, const char * op, int typeSize, const char * typeName ) {
        V_BEGIN();
        vis.op(op, typeSize, typeName);
        V_SUB(arguments[0]);
        V_SUB(arguments[1]);
        V_SUB(arguments[2]);
        V_END();
    }

    SimNode * SimNode_GetBitField::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GetBitField);
        V_SUB(x);
        V_ARG(mask);
        V_END();
    }

    SimNode * SimNode_DeleteTable::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(DeleteTable);
        V_ARG(total);
        V_ARG(vts_add_kts);
        V_SUB(subexpr);
        V_END();
    }

    SimNode * SimNode_ArrayAt::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(ArrayAt);
        V_SUB(l);
        V_SUB(r);
        V_ARG(stride);
        V_ARG(offset);
        V_END();
    }

    SimNode * SimNode_SafeArrayAt::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(SafeArrayAt);
        V_SUB(l);
        V_SUB(r);
        V_ARG(stride);
        V_ARG(offset);
        V_END();
    }

    SimNode * SimNode_GoodArrayIterator::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(GoodArrayIterator);
        V_SUB(source);
        V_ARG(stride);
        V_END();
    }

    SimNode * SimNode_StringIterator::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(StringIterator);
        V_SUB(source);
        V_END();
    }

    SimNode * SimNode_FixedArrayIterator::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(FixedArrayIterator);
        V_SUB(source);
        V_ARG(size);
        V_ARG(stride);
        V_END();
    }

    SimNode * SimNode_DeleteArray::visit ( SimVisitor & vis ) {
        V_BEGIN();
        V_OP(DeleteArray);
        V_ARG(total);
        V_ARG(stride);
        V_SUB(subexpr);
        V_END();
    }
}

