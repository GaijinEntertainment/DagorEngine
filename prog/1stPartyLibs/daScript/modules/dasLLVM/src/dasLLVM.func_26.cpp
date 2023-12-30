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
// from D:\Work\libclang\include\llvm-c/Core.h:3183:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetMetadata >(*this,lib,"LLVMGetMetadata",SideEffects::worstDefault,"LLVMGetMetadata")
		->args({"Val","KindID"});
// from D:\Work\libclang\include\llvm-c/Core.h:3188:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMSetMetadata >(*this,lib,"LLVMSetMetadata",SideEffects::worstDefault,"LLVMSetMetadata")
		->args({"Val","KindID","Node"});
// from D:\Work\libclang\include\llvm-c/Core.h:3197:1
	addExtern< LLVMOpaqueValueMetadataEntry * (*)(LLVMOpaqueValue *,size_t *) , LLVMInstructionGetAllMetadataOtherThanDebugLoc >(*this,lib,"LLVMInstructionGetAllMetadataOtherThanDebugLoc",SideEffects::worstDefault,"LLVMInstructionGetAllMetadataOtherThanDebugLoc")
		->args({"Instr","NumEntries"});
// from D:\Work\libclang\include\llvm-c/Core.h:3205:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetInstructionParent >(*this,lib,"LLVMGetInstructionParent",SideEffects::worstDefault,"LLVMGetInstructionParent")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3215:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetNextInstruction >(*this,lib,"LLVMGetNextInstruction",SideEffects::worstDefault,"LLVMGetNextInstruction")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3223:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMGetPreviousInstruction >(*this,lib,"LLVMGetPreviousInstruction",SideEffects::worstDefault,"LLVMGetPreviousInstruction")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3233:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMInstructionRemoveFromParent >(*this,lib,"LLVMInstructionRemoveFromParent",SideEffects::worstDefault,"LLVMInstructionRemoveFromParent")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3243:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMInstructionEraseFromParent >(*this,lib,"LLVMInstructionEraseFromParent",SideEffects::worstDefault,"LLVMInstructionEraseFromParent")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3253:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMDeleteInstruction >(*this,lib,"LLVMDeleteInstruction",SideEffects::worstDefault,"LLVMDeleteInstruction")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3260:12
	addExtern< LLVMOpcode (*)(LLVMOpaqueValue *) , LLVMGetInstructionOpcode >(*this,lib,"LLVMGetInstructionOpcode",SideEffects::worstDefault,"LLVMGetInstructionOpcode")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3270:18
	addExtern< LLVMIntPredicate (*)(LLVMOpaqueValue *) , LLVMGetICmpPredicate >(*this,lib,"LLVMGetICmpPredicate",SideEffects::worstDefault,"LLVMGetICmpPredicate")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3280:19
	addExtern< LLVMRealPredicate (*)(LLVMOpaqueValue *) , LLVMGetFCmpPredicate >(*this,lib,"LLVMGetFCmpPredicate",SideEffects::worstDefault,"LLVMGetFCmpPredicate")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3290:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMInstructionClone >(*this,lib,"LLVMInstructionClone",SideEffects::worstDefault,"LLVMInstructionClone")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3299:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *) , LLVMIsATerminatorInst >(*this,lib,"LLVMIsATerminatorInst",SideEffects::worstDefault,"LLVMIsATerminatorInst")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3321:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetNumArgOperands >(*this,lib,"LLVMGetNumArgOperands",SideEffects::worstDefault,"LLVMGetNumArgOperands")
		->args({"Instr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3332:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int) , LLVMSetInstructionCallConv >(*this,lib,"LLVMSetInstructionCallConv",SideEffects::worstDefault,"LLVMSetInstructionCallConv")
		->args({"Instr","CC"});
// from D:\Work\libclang\include\llvm-c/Core.h:3342:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetInstructionCallConv >(*this,lib,"LLVMGetInstructionCallConv",SideEffects::worstDefault,"LLVMGetInstructionCallConv")
		->args({"Instr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3344:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,unsigned int) , LLVMSetInstrParamAlignment >(*this,lib,"LLVMSetInstrParamAlignment",SideEffects::worstDefault,"LLVMSetInstrParamAlignment")
		->args({"Instr","Idx","Align"});
// from D:\Work\libclang\include\llvm-c/Core.h:3347:6
	addExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueAttributeRef *) , LLVMAddCallSiteAttribute >(*this,lib,"LLVMAddCallSiteAttribute",SideEffects::worstDefault,"LLVMAddCallSiteAttribute")
		->args({"C","Idx","A"});
// from D:\Work\libclang\include\llvm-c/Core.h:3349:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetCallSiteAttributeCount >(*this,lib,"LLVMGetCallSiteAttributeCount",SideEffects::worstDefault,"LLVMGetCallSiteAttributeCount")
		->args({"C","Idx"});
}
}

