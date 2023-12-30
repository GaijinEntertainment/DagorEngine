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
// from D:\Work\libclang\include\llvm-c/Core.h:2826:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMEraseGlobalIFunc >(*this,lib,"LLVMEraseGlobalIFunc",SideEffects::worstDefault,"LLVMEraseGlobalIFunc")
		->args({"IFunc"});
// from D:\Work\libclang\include\llvm-c/Core.h:2836:6
	addExtern< void (*)(LLVMOpaqueValue *) , LLVMRemoveGlobalIFunc >(*this,lib,"LLVMRemoveGlobalIFunc",SideEffects::worstDefault,"LLVMRemoveGlobalIFunc")
		->args({"IFunc"});
// from D:\Work\libclang\include\llvm-c/Core.h:2868:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,const char *,size_t) , LLVMMDStringInContext2 >(*this,lib,"LLVMMDStringInContext2",SideEffects::worstDefault,"LLVMMDStringInContext2")
		->args({"C","Str","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2876:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,LLVMOpaqueMetadata **,size_t) , LLVMMDNodeInContext2 >(*this,lib,"LLVMMDNodeInContext2",SideEffects::worstDefault,"LLVMMDNodeInContext2")
		->args({"C","MDs","Count"});
// from D:\Work\libclang\include\llvm-c/Core.h:2882:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,LLVMOpaqueMetadata *) , LLVMMetadataAsValue >(*this,lib,"LLVMMetadataAsValue",SideEffects::worstDefault,"LLVMMetadataAsValue")
		->args({"C","MD"});
// from D:\Work\libclang\include\llvm-c/Core.h:2887:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValue *) , LLVMValueAsMetadata >(*this,lib,"LLVMValueAsMetadata",SideEffects::worstDefault,"LLVMValueAsMetadata")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:2896:13
	addExtern< const char * (*)(LLVMOpaqueValue *,unsigned int *) , LLVMGetMDString >(*this,lib,"LLVMGetMDString",SideEffects::worstDefault,"LLVMGetMDString")
		->args({"V","Length"});
// from D:\Work\libclang\include\llvm-c/Core.h:2904:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMGetMDNodeNumOperands >(*this,lib,"LLVMGetMDNodeNumOperands",SideEffects::worstDefault,"LLVMGetMDNodeNumOperands")
		->args({"V"});
// from D:\Work\libclang\include\llvm-c/Core.h:2917:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueValue **) , LLVMGetMDNodeOperands >(*this,lib,"LLVMGetMDNodeOperands",SideEffects::worstDefault,"LLVMGetMDNodeOperands")
		->args({"V","Dest"});
// from D:\Work\libclang\include\llvm-c/Core.h:2920:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,const char *,unsigned int) , LLVMMDStringInContext >(*this,lib,"LLVMMDStringInContext",SideEffects::worstDefault,"LLVMMDStringInContext")
		->args({"C","Str","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2923:14
	addExtern< LLVMOpaqueValue * (*)(const char *,unsigned int) , LLVMMDString >(*this,lib,"LLVMMDString",SideEffects::worstDefault,"LLVMMDString")
		->args({"Str","SLen"});
// from D:\Work\libclang\include\llvm-c/Core.h:2925:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueContext *,LLVMOpaqueValue **,unsigned int) , LLVMMDNodeInContext >(*this,lib,"LLVMMDNodeInContext",SideEffects::worstDefault,"LLVMMDNodeInContext")
		->args({"C","Vals","Count"});
// from D:\Work\libclang\include\llvm-c/Core.h:2928:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueValue **,unsigned int) , LLVMMDNode >(*this,lib,"LLVMMDNode",SideEffects::worstDefault,"LLVMMDNode")
		->args({"Vals","Count"});
// from D:\Work\libclang\include\llvm-c/Core.h:2954:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMBasicBlockAsValue >(*this,lib,"LLVMBasicBlockAsValue",SideEffects::worstDefault,"LLVMBasicBlockAsValue")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:2959:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMValueIsBasicBlock >(*this,lib,"LLVMValueIsBasicBlock",SideEffects::worstDefault,"LLVMValueIsBasicBlock")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:2964:19
	addExtern< LLVMOpaqueBasicBlock * (*)(LLVMOpaqueValue *) , LLVMValueAsBasicBlock >(*this,lib,"LLVMValueAsBasicBlock",SideEffects::worstDefault,"LLVMValueAsBasicBlock")
		->args({"Val"});
// from D:\Work\libclang\include\llvm-c/Core.h:2969:13
	addExtern< const char * (*)(LLVMOpaqueBasicBlock *) , LLVMGetBasicBlockName >(*this,lib,"LLVMGetBasicBlockName",SideEffects::worstDefault,"LLVMGetBasicBlockName")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:2976:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetBasicBlockParent >(*this,lib,"LLVMGetBasicBlockParent",SideEffects::worstDefault,"LLVMGetBasicBlockParent")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:2988:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBasicBlock *) , LLVMGetBasicBlockTerminator >(*this,lib,"LLVMGetBasicBlockTerminator",SideEffects::worstDefault,"LLVMGetBasicBlockTerminator")
		->args({"BB"});
// from D:\Work\libclang\include\llvm-c/Core.h:2995:10
	addExtern< unsigned int (*)(LLVMOpaqueValue *) , LLVMCountBasicBlocks >(*this,lib,"LLVMCountBasicBlocks",SideEffects::worstDefault,"LLVMCountBasicBlocks")
		->args({"Fn"});
}
}

