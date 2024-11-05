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
// from D:\Work\libclang\include\llvm-c/Core.h:3969:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildPtrToInt , SimNode_ExtFuncCall >(lib,"LLVMBuildPtrToInt","LLVMBuildPtrToInt")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3971:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildIntToPtr , SimNode_ExtFuncCall >(lib,"LLVMBuildIntToPtr","LLVMBuildIntToPtr")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3973:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildBitCast , SimNode_ExtFuncCall >(lib,"LLVMBuildBitCast","LLVMBuildBitCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3975:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildAddrSpaceCast , SimNode_ExtFuncCall >(lib,"LLVMBuildAddrSpaceCast","LLVMBuildAddrSpaceCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3977:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildZExtOrBitCast , SimNode_ExtFuncCall >(lib,"LLVMBuildZExtOrBitCast","LLVMBuildZExtOrBitCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3979:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildSExtOrBitCast , SimNode_ExtFuncCall >(lib,"LLVMBuildSExtOrBitCast","LLVMBuildSExtOrBitCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3981:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildTruncOrBitCast , SimNode_ExtFuncCall >(lib,"LLVMBuildTruncOrBitCast","LLVMBuildTruncOrBitCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3983:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpcode,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildCast , SimNode_ExtFuncCall >(lib,"LLVMBuildCast","LLVMBuildCast")
		->args({"B","Op","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3985:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildPointerCast , SimNode_ExtFuncCall >(lib,"LLVMBuildPointerCast","LLVMBuildPointerCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3987:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,int,const char *) , LLVMBuildIntCast2 , SimNode_ExtFuncCall >(lib,"LLVMBuildIntCast2","LLVMBuildIntCast2")
		->args({"","Val","DestTy","IsSigned","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3990:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPCast , SimNode_ExtFuncCall >(lib,"LLVMBuildFPCast","LLVMBuildFPCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3994:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildIntCast , SimNode_ExtFuncCall >(lib,"LLVMBuildIntCast","LLVMBuildIntCast")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3997:12
	makeExtern< LLVMOpcode (*)(LLVMOpaqueValue *,int,LLVMOpaqueType *,int) , LLVMGetCastOpcode , SimNode_ExtFuncCall >(lib,"LLVMGetCastOpcode","LLVMGetCastOpcode")
		->args({"Src","SrcIsSigned","DestTy","DestIsSigned"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4001:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMIntPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildICmp , SimNode_ExtFuncCall >(lib,"LLVMBuildICmp","LLVMBuildICmp")
		->args({"","Op","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4004:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMRealPredicate,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildFCmp , SimNode_ExtFuncCall >(lib,"LLVMBuildFCmp","LLVMBuildFCmp")
		->args({"","Op","LHS","RHS","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4009:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,const char *) , LLVMBuildPhi , SimNode_ExtFuncCall >(lib,"LLVMBuildPhi","LLVMBuildPhi")
		->args({"","Ty","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4010:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildCall2 , SimNode_ExtFuncCall >(lib,"LLVMBuildCall2","LLVMBuildCall2")
		->args({"","","Fn","Args","NumArgs","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4013:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildSelect , SimNode_ExtFuncCall >(lib,"LLVMBuildSelect","LLVMBuildSelect")
		->args({"","If","Then","Else","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4016:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildVAArg , SimNode_ExtFuncCall >(lib,"LLVMBuildVAArg","LLVMBuildVAArg")
		->args({"","List","Ty","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:4018:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *,const char *) , LLVMBuildExtractElement , SimNode_ExtFuncCall >(lib,"LLVMBuildExtractElement","LLVMBuildExtractElement")
		->args({"","VecVal","Index","Name"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

