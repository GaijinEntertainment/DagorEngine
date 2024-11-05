// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasLLVM.h"
#include "need_dasLLVM.h"
namespace das {
#include "dasLLVM.func.aot.decl.inc"
void Module_dasLLVM::initFunctions_26() {
// from D:\Work\libclang\include\llvm-c/Core.h:3202:1
	makeExtern< LLVMOpaqueValueMetadataEntry * (*)(LLVMOpaqueValue *,size_t *) , LLVMInstructionGetAllMetadataOtherThanDebugLoc , SimNode_ExtFuncCall >(lib,"LLVMInstructionGetAllMetadataOtherThanDebugLoc","LLVMInstructionGetAllMetadataOtherThanDebugLoc")
		->args({"Instr","NumEntries"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3210:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetInstructionParent , SimNode_ExtFuncCall >(lib,"LLVMGetInstructionParent","LLVMGetInstructionParent")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3220:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextInstruction , SimNode_ExtFuncCall >(lib,"LLVMGetNextInstruction","LLVMGetNextInstruction")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3228:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousInstruction , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousInstruction","LLVMGetPreviousInstruction")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3238:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMInstructionRemoveFromParent , SimNode_ExtFuncCall >(lib,"LLVMInstructionRemoveFromParent","LLVMInstructionRemoveFromParent")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3248:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMInstructionEraseFromParent , SimNode_ExtFuncCall >(lib,"LLVMInstructionEraseFromParent","LLVMInstructionEraseFromParent")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3258:6
	makeExtern< void (*)(LLVMOpaqueValue *) , LLVMDeleteInstruction , SimNode_ExtFuncCall >(lib,"LLVMDeleteInstruction","LLVMDeleteInstruction")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3265:12
	makeExtern< LLVMOpcode (*)(LLVMOpaqueValue *) , LLVMGetInstructionOpcode , SimNode_ExtFuncCall >(lib,"LLVMGetInstructionOpcode","LLVMGetInstructionOpcode")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3275:18
	makeExtern< LLVMIntPredicate (*)(LLVMOpaqueValue *) , LLVMGetICmpPredicate , SimNode_ExtFuncCall >(lib,"LLVMGetICmpPredicate","LLVMGetICmpPredicate")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3285:19
	makeExtern< LLVMRealPredicate (*)(LLVMOpaqueValue *) , LLVMGetFCmpPredicate , SimNode_ExtFuncCall >(lib,"LLVMGetFCmpPredicate","LLVMGetFCmpPredicate")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3295:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMInstructionClone , SimNode_ExtFuncCall >(lib,"LLVMInstructionClone","LLVMInstructionClone")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3304:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsATerminatorInst , SimNode_ExtFuncCall >(lib,"LLVMIsATerminatorInst","LLVMIsATerminatorInst")
		->args({"Inst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3326:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumArgOperands , SimNode_ExtFuncCall >(lib,"LLVMGetNumArgOperands","LLVMGetNumArgOperands")
		->args({"Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3337:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetInstructionCallConv , SimNode_ExtFuncCall >(lib,"LLVMSetInstructionCallConv","LLVMSetInstructionCallConv")
		->args({"Instr","CC"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3347:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetInstructionCallConv , SimNode_ExtFuncCall >(lib,"LLVMGetInstructionCallConv","LLVMGetInstructionCallConv")
		->args({"Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3349:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMSetInstrParamAlignment , SimNode_ExtFuncCall >(lib,"LLVMSetInstrParamAlignment","LLVMSetInstrParamAlignment")
		->args({"Instr","Idx","Align"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3352:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef *) , LLVMAddCallSiteAttribute , SimNode_ExtFuncCall >(lib,"LLVMAddCallSiteAttribute","LLVMAddCallSiteAttribute")
		->args({"C","Idx","A"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3354:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetCallSiteAttributeCount , SimNode_ExtFuncCall >(lib,"LLVMGetCallSiteAttributeCount","LLVMGetCallSiteAttributeCount")
		->args({"C","Idx"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3355:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef **) , LLVMGetCallSiteAttributes , SimNode_ExtFuncCall >(lib,"LLVMGetCallSiteAttributes","LLVMGetCallSiteAttributes")
		->args({"C","Idx","Attrs"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3357:18
	makeExtern< LLVMOpaqueAttributeRef * (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMGetCallSiteEnumAttribute , SimNode_ExtFuncCall >(lib,"LLVMGetCallSiteEnumAttribute","LLVMGetCallSiteEnumAttribute")
		->args({"C","Idx","KindID"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

