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
void Module_dasLLVM::initFunctions_25() {
// from D:\Work\libclang\include\llvm-c/Core.h:3005:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock **) , LLVMGetBasicBlocks >(*this,lib,"LLVMGetBasicBlocks",SideEffects::worstDefault,"LLVMGetBasicBlocks")
		->args({"Fn","BasicBlocks"});
// from D:\Work\libclang\include\llvm-c/Core.h:3015:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetFirstBasicBlock >(*this,lib,"LLVMGetFirstBasicBlock",SideEffects::worstDefault,"LLVMGetFirstBasicBlock")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:3022:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetLastBasicBlock >(*this,lib,"LLVMGetLastBasicBlock",SideEffects::worstDefault,"LLVMGetLastBasicBlock")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:3027:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBasicBlock *) , LLVMGetNextBasicBlock >(*this,lib,"LLVMGetNextBasicBlock",SideEffects::worstDefault,"LLVMGetNextBasicBlock")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3032:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBasicBlock *) , LLVMGetPreviousBasicBlock >(*this,lib,"LLVMGetPreviousBasicBlock",SideEffects::worstDefault,"LLVMGetPreviousBasicBlock")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3040:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetEntryBasicBlock >(*this,lib,"LLVMGetEntryBasicBlock",SideEffects::worstDefault,"LLVMGetEntryBasicBlock")
		->args({"Fn"});
// from D:\Work\libclang\include\llvm-c/Core.h:3049:6
	addExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *) , LLVMInsertExistingBasicBlockAfterInsertBlock >(*this,lib,"LLVMInsertExistingBasicBlockAfterInsertBlock",SideEffects::worstDefault,"LLVMInsertExistingBasicBlockAfterInsertBlock")
		->args({"Builder","BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3057:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAppendExistingBasicBlock >(*this,lib,"LLVMAppendExistingBasicBlock",SideEffects::worstDefault,"LLVMAppendExistingBasicBlock")
		->args({"Fn","BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3065:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueContext *,const char *) , LLVMCreateBasicBlockInContext >(*this,lib,"LLVMCreateBasicBlockInContext",SideEffects::worstDefault,"LLVMCreateBasicBlockInContext")
		->args({"C","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3073:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueContext *,LLVMOpaqueValue *,const char *) , LLVMAppendBasicBlockInContext >(*this,lib,"LLVMAppendBasicBlockInContext",SideEffects::worstDefault,"LLVMAppendBasicBlockInContext")
		->args({"C","Fn","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3083:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *,const char *) , LLVMAppendBasicBlock >(*this,lib,"LLVMAppendBasicBlock",SideEffects::worstDefault,"LLVMAppendBasicBlock")
		->args({"Fn","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3093:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueContext *,LLVMOpaqueBasicBlock *,const char *) , LLVMInsertBasicBlockInContext >(*this,lib,"LLVMInsertBasicBlockInContext",SideEffects::worstDefault,"LLVMInsertBasicBlockInContext")
		->args({"C","BB","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3102:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBasicBlock *,const char *) , LLVMInsertBasicBlock >(*this,lib,"LLVMInsertBasicBlock",SideEffects::worstDefault,"LLVMInsertBasicBlock")
		->args({"InsertBeforeBB","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3113:6
	addExtern< void (*)(LLVMOpaqueBasicBlock *) , LLVMDeleteBasicBlock >(*this,lib,"LLVMDeleteBasicBlock",SideEffects::worstDefault,"LLVMDeleteBasicBlock")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3123:6
	addExtern< void (*)(LLVMOpaqueBasicBlock *) , LLVMRemoveBasicBlockFromParent >(*this,lib,"LLVMRemoveBasicBlockFromParent",SideEffects::worstDefault,"LLVMRemoveBasicBlockFromParent")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3130:6
	addExtern< void (*)(LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *) , LLVMMoveBasicBlockBefore >(*this,lib,"LLVMMoveBasicBlockBefore",SideEffects::worstDefault,"LLVMMoveBasicBlockBefore")
		->args({"BB","MovePos"});
// from D:\Work\libclang\include\llvm-c/Core.h:3137:6
	addExtern< void (*)(LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *) , LLVMMoveBasicBlockAfter >(*this,lib,"LLVMMoveBasicBlockAfter",SideEffects::worstDefault,"LLVMMoveBasicBlockAfter")
		->args({"BB","MovePos"});
// from D:\Work\libclang\include\llvm-c/Core.h:3145:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetFirstInstruction >(*this,lib,"LLVMGetFirstInstruction",SideEffects::worstDefault,"LLVMGetFirstInstruction")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3152:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetLastInstruction >(*this,lib,"LLVMGetLastInstruction",SideEffects::worstDefault,"LLVMGetLastInstruction")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:3178:5
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMHasMetadata >(*this,lib,"LLVMHasMetadata",SideEffects::worstDefault,"LLVMHasMetadata")
		->args({"Val"});
}
}

