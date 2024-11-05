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
void Module_dasLLVM::initFunctions_24() {
// from D:\Work\libclang\include\llvm-c/Core.h:2873:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,const char *,size_t) , LLVMMDStringInContext2 , SimNode_ExtFuncCall >(lib,"LLVMMDStringInContext2","LLVMMDStringInContext2")
		->args({"C","Str","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2881:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,LLVMOpaqueMetadata **,size_t) , LLVMMDNodeInContext2 , SimNode_ExtFuncCall >(lib,"LLVMMDNodeInContext2","LLVMMDNodeInContext2")
		->args({"C","MDs","Count"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2887:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,LLVMOpaqueMetadata *) , LLVMMetadataAsValue , SimNode_ExtFuncCall >(lib,"LLVMMetadataAsValue","LLVMMetadataAsValue")
		->args({"C","MD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2892:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValue *) , LLVMValueAsMetadata , SimNode_ExtFuncCall >(lib,"LLVMValueAsMetadata","LLVMValueAsMetadata")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2901:13
	makeExtern< const char * (*)(LLVMOpaqueValue *,unsigned int *) , LLVMGetMDString , SimNode_ExtFuncCall >(lib,"LLVMGetMDString","LLVMGetMDString")
		->args({"V","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2909:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetMDNodeNumOperands , SimNode_ExtFuncCall >(lib,"LLVMGetMDNodeNumOperands","LLVMGetMDNodeNumOperands")
		->args({"V"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2922:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue **) , LLVMGetMDNodeOperands , SimNode_ExtFuncCall >(lib,"LLVMGetMDNodeOperands","LLVMGetMDNodeOperands")
		->args({"V","Dest"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2925:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,const char *,unsigned int) , LLVMMDStringInContext , SimNode_ExtFuncCall >(lib,"LLVMMDStringInContext","LLVMMDStringInContext")
		->args({"C","Str","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2928:14
	makeExtern< LLVMOpaqueValue * (*)(const char *,unsigned int) , LLVMMDString , SimNode_ExtFuncCall >(lib,"LLVMMDString","LLVMMDString")
		->args({"Str","SLen"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2930:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,LLVMOpaqueValue **,unsigned int) , LLVMMDNodeInContext , SimNode_ExtFuncCall >(lib,"LLVMMDNodeInContext","LLVMMDNodeInContext")
		->args({"C","Vals","Count"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2933:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue **,unsigned int) , LLVMMDNode , SimNode_ExtFuncCall >(lib,"LLVMMDNode","LLVMMDNode")
		->args({"Vals","Count"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2959:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMBasicBlockAsValue , SimNode_ExtFuncCall >(lib,"LLVMBasicBlockAsValue","LLVMBasicBlockAsValue")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2964:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMValueIsBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMValueIsBasicBlock","LLVMValueIsBasicBlock")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2969:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMValueAsBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMValueAsBasicBlock","LLVMValueAsBasicBlock")
		->args({"Val"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2974:13
	makeExtern< const char * (*)(LLVMOpaqueBasicBlock *) , LLVMGetBasicBlockName , SimNode_ExtFuncCall >(lib,"LLVMGetBasicBlockName","LLVMGetBasicBlockName")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2981:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetBasicBlockParent , SimNode_ExtFuncCall >(lib,"LLVMGetBasicBlockParent","LLVMGetBasicBlockParent")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:2993:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetBasicBlockTerminator , SimNode_ExtFuncCall >(lib,"LLVMGetBasicBlockTerminator","LLVMGetBasicBlockTerminator")
		->args({"BB"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3000:10
	makeExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMCountBasicBlocks , SimNode_ExtFuncCall >(lib,"LLVMCountBasicBlocks","LLVMCountBasicBlocks")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3010:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueBasicBlock **) , LLVMGetBasicBlocks , SimNode_ExtFuncCall >(lib,"LLVMGetBasicBlocks","LLVMGetBasicBlocks")
		->args({"Fn","BasicBlocks"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3020:19
	makeExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMGetFirstBasicBlock , SimNode_ExtFuncCall >(lib,"LLVMGetFirstBasicBlock","LLVMGetFirstBasicBlock")
		->args({"Fn"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

