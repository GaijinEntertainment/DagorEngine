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
// from D:\Work\libclang\include\llvm-c/Core.h:3027:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetLastBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMGetLastBasicBlock","LLVMGetLastBasicBlock")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3032:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBasicBlock *) , LLVMGetNextBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMGetNextBasicBlock","LLVMGetNextBasicBlock")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3037:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBasicBlock *) , LLVMGetPreviousBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMGetPreviousBasicBlock","LLVMGetPreviousBasicBlock")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3045:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetEntryBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMGetEntryBasicBlock","LLVMGetEntryBasicBlock")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3054:6
	makeExtern< void (*)(LLVMOpaqueBuilder *,LLVMOpaqueBasicBlock *) , LLVMInsertExistingBasicBlockAfterInsertBlock , SimNode_ExtFuncCall >(lib,"LLVMInsertExistingBasicBlockAfterInsertBlock","LLVMInsertExistingBasicBlockAfterInsertBlock")
		->args({"Builder","BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3062:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock *) , LLVMAppendExistingBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMAppendExistingBasicBlock","LLVMAppendExistingBasicBlock")
		->args({"Fn","BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3070:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueContext *,const char *) , LLVMCreateBasicBlockInContext , SimNode_ExtFuncCall >(lib,"LLVMCreateBasicBlockInContext","LLVMCreateBasicBlockInContext")
		->args({"C","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3078:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueContext *,LLVMOpaqueValue *,const char *) , LLVMAppendBasicBlockInContext , SimNode_ExtFuncCall >(lib,"LLVMAppendBasicBlockInContext","LLVMAppendBasicBlockInContext")
		->args({"C","Fn","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3088:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *,const char *) , LLVMAppendBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMAppendBasicBlock","LLVMAppendBasicBlock")
		->args({"Fn","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3098:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueContext *,LLVMOpaqueBasicBlock *,const char *) , LLVMInsertBasicBlockInContext , SimNode_ExtFuncCall >(lib,"LLVMInsertBasicBlockInContext","LLVMInsertBasicBlockInContext")
		->args({"C","BB","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3107:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueBasicBlock *,const char *) , LLVMInsertBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMInsertBasicBlock","LLVMInsertBasicBlock")
		->args({"InsertBeforeBB","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3118:6
	makeExtern< void (*)(LLVMOpaqueBasicBlock *) , LLVMDeleteBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMDeleteBasicBlock","LLVMDeleteBasicBlock")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3128:6
	makeExtern< void (*)(LLVMOpaqueBasicBlock *) , LLVMRemoveBasicBlockFromParent , SimNode_ExtFuncCall >(lib,"LLVMRemoveBasicBlockFromParent","LLVMRemoveBasicBlockFromParent")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3135:6
	makeExtern< void (*)(LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *) , LLVMMoveBasicBlockBefore , SimNode_ExtFuncCall >(lib,"LLVMMoveBasicBlockBefore","LLVMMoveBasicBlockBefore")
		->args({"BB","MovePos"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3142:6
	makeExtern< void (*)(LLVMOpaqueBasicBlock *,LLVMOpaqueBasicBlock *) , LLVMMoveBasicBlockAfter , SimNode_ExtFuncCall >(lib,"LLVMMoveBasicBlockAfter","LLVMMoveBasicBlockAfter")
		->args({"BB","MovePos"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3150:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetFirstInstruction , SimNode_ExtFuncCall >(lib,"LLVMGetFirstInstruction","LLVMGetFirstInstruction")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3157:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetLastInstruction , SimNode_ExtFuncCall >(lib,"LLVMGetLastInstruction","LLVMGetLastInstruction")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3183:5
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMHasMetadata , SimNode_ExtFuncCall >(lib,"LLVMHasMetadata","LLVMHasMetadata")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3188:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue *,unsigned int) , LLVMGetMetadata , SimNode_ExtFuncCall >(lib,"LLVMGetMetadata","LLVMGetMetadata")
		->args({"Val","KindID"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3193:6
	makeExtern< void (*)(LLVMOpaqueValue *,unsigned int,LLVMOpaqueValue *) , LLVMSetMetadata , SimNode_ExtFuncCall >(lib,"LLVMSetMetadata","LLVMSetMetadata")
		->args({"Val","KindID","Node"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

