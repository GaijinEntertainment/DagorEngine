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
void Module_dasLLVM::initFunctions_33() {
// from D:\Work\libclang\include\llvm-c/Core.h:3934:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildStructGEP2 , SimNode_ExtFuncCall >(lib,"LLVMBuildStructGEP2","LLVMBuildStructGEP2")
		->args({"B","Ty","Pointer","Idx","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3937:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,const char *,const char *) , LLVMBuildGlobalString , SimNode_ExtFuncCall >(lib,"LLVMBuildGlobalString","LLVMBuildGlobalString")
		->args({"B","Str","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3939:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,const char *,const char *) , LLVMBuildGlobalStringPtr , SimNode_ExtFuncCall >(lib,"LLVMBuildGlobalStringPtr","LLVMBuildGlobalStringPtr")
		->args({"B","Str","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3941:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMGetVolatile , SimNode_ExtFuncCall >(lib,"LLVMGetVolatile","LLVMGetVolatile")
		->args({"MemoryAccessInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3942:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetVolatile , SimNode_ExtFuncCall >(lib,"LLVMSetVolatile","LLVMSetVolatile")
		->args({"MemoryAccessInst","IsVolatile"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3943:10
	makeExtern< int (*)(LLVMOpaqueValue *) , LLVMGetWeak , SimNode_ExtFuncCall >(lib,"LLVMGetWeak","LLVMGetWeak")
		->args({"CmpXchgInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3944:6
	makeExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetWeak , SimNode_ExtFuncCall >(lib,"LLVMSetWeak","LLVMSetWeak")
		->args({"CmpXchgInst","IsWeak"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3945:20
	makeExtern< LLVMAtomicOrdering (*)(LLVMOpaqueValue *) , LLVMGetOrdering , SimNode_ExtFuncCall >(lib,"LLVMGetOrdering","LLVMGetOrdering")
		->args({"MemoryAccessInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3946:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicOrdering) , LLVMSetOrdering , SimNode_ExtFuncCall >(lib,"LLVMSetOrdering","LLVMSetOrdering")
		->args({"MemoryAccessInst","Ordering"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3947:20
	makeExtern< LLVMAtomicRMWBinOp (*)(LLVMOpaqueValue *) , LLVMGetAtomicRMWBinOp , SimNode_ExtFuncCall >(lib,"LLVMGetAtomicRMWBinOp","LLVMGetAtomicRMWBinOp")
		->args({"AtomicRMWInst"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3948:6
	makeExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicRMWBinOp) , LLVMSetAtomicRMWBinOp , SimNode_ExtFuncCall >(lib,"LLVMSetAtomicRMWBinOp","LLVMSetAtomicRMWBinOp")
		->args({"AtomicRMWInst","BinOp"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3951:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildTrunc , SimNode_ExtFuncCall >(lib,"LLVMBuildTrunc","LLVMBuildTrunc")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3953:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildZExt , SimNode_ExtFuncCall >(lib,"LLVMBuildZExt","LLVMBuildZExt")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3955:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildSExt , SimNode_ExtFuncCall >(lib,"LLVMBuildSExt","LLVMBuildSExt")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3957:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPToUI , SimNode_ExtFuncCall >(lib,"LLVMBuildFPToUI","LLVMBuildFPToUI")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3959:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPToSI , SimNode_ExtFuncCall >(lib,"LLVMBuildFPToSI","LLVMBuildFPToSI")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3961:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildUIToFP , SimNode_ExtFuncCall >(lib,"LLVMBuildUIToFP","LLVMBuildUIToFP")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3963:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildSIToFP , SimNode_ExtFuncCall >(lib,"LLVMBuildSIToFP","LLVMBuildSIToFP")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3965:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPTrunc , SimNode_ExtFuncCall >(lib,"LLVMBuildFPTrunc","LLVMBuildFPTrunc")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:3967:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildFPExt , SimNode_ExtFuncCall >(lib,"LLVMBuildFPExt","LLVMBuildFPExt")
		->args({"","Val","DestTy","Name"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

