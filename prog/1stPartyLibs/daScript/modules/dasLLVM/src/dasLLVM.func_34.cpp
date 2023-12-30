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
void Module_dasLLVM::initFunctions_34() {
// from D:\Work\libclang\include\llvm-c/Core.h:3974:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildSExt >(*this,lib,"LLVMBuildSExt",SideEffects::worstDefault,"LLVMBuildSExt")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3976:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPToUI >(*this,lib,"LLVMBuildFPToUI",SideEffects::worstDefault,"LLVMBuildFPToUI")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3978:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPToSI >(*this,lib,"LLVMBuildFPToSI",SideEffects::worstDefault,"LLVMBuildFPToSI")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3980:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildUIToFP >(*this,lib,"LLVMBuildUIToFP",SideEffects::worstDefault,"LLVMBuildUIToFP")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3982:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildSIToFP >(*this,lib,"LLVMBuildSIToFP",SideEffects::worstDefault,"LLVMBuildSIToFP")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3984:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPTrunc >(*this,lib,"LLVMBuildFPTrunc",SideEffects::worstDefault,"LLVMBuildFPTrunc")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3986:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPExt >(*this,lib,"LLVMBuildFPExt",SideEffects::worstDefault,"LLVMBuildFPExt")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3988:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildPtrToInt >(*this,lib,"LLVMBuildPtrToInt",SideEffects::worstDefault,"LLVMBuildPtrToInt")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3990:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildIntToPtr >(*this,lib,"LLVMBuildIntToPtr",SideEffects::worstDefault,"LLVMBuildIntToPtr")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3992:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildBitCast >(*this,lib,"LLVMBuildBitCast",SideEffects::worstDefault,"LLVMBuildBitCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3994:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildAddrSpaceCast >(*this,lib,"LLVMBuildAddrSpaceCast",SideEffects::worstDefault,"LLVMBuildAddrSpaceCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3996:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildZExtOrBitCast >(*this,lib,"LLVMBuildZExtOrBitCast",SideEffects::worstDefault,"LLVMBuildZExtOrBitCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3998:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildSExtOrBitCast >(*this,lib,"LLVMBuildSExtOrBitCast",SideEffects::worstDefault,"LLVMBuildSExtOrBitCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4000:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildTruncOrBitCast >(*this,lib,"LLVMBuildTruncOrBitCast",SideEffects::worstDefault,"LLVMBuildTruncOrBitCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4002:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpcode,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildCast >(*this,lib,"LLVMBuildCast",SideEffects::worstDefault,"LLVMBuildCast")
		->args({"B","Op","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4004:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildPointerCast >(*this,lib,"LLVMBuildPointerCast",SideEffects::worstDefault,"LLVMBuildPointerCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4006:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,int,const char *) , LLVMBuildIntCast2 >(*this,lib,"LLVMBuildIntCast2",SideEffects::worstDefault,"LLVMBuildIntCast2")
		->args({"","Val","DestTy","IsSigned","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4009:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPCast >(*this,lib,"LLVMBuildFPCast",SideEffects::worstDefault,"LLVMBuildFPCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4013:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildIntCast >(*this,lib,"LLVMBuildIntCast",SideEffects::worstDefault,"LLVMBuildIntCast")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:4016:12
	addExtern< LLVMOpcode (*)(LLVMOpaqueValue *,int,LLVMOpaqueType *,int) , LLVMGetCastOpcode >(*this,lib,"LLVMGetCastOpcode",SideEffects::worstDefault,"LLVMGetCastOpcode")
		->args({"Src","SrcIsSigned","DestTy","DestIsSigned"});
}
}

