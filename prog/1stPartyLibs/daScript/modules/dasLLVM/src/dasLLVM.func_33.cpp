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
// from D:\Work\libclang\include\llvm-c/Core.h:3930:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,const char *) , LLVMBuildLoad2 >(*this,lib,"LLVMBuildLoad2",SideEffects::worstDefault,"LLVMBuildLoad2")
		->args({"","Ty","PointerVal","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3932:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue *) , LLVMBuildStore >(*this,lib,"LLVMBuildStore",SideEffects::worstDefault,"LLVMBuildStore")
		->args({"","Val","Ptr"});
// from D:\Work\libclang\include\llvm-c/Core.h:3934:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildGEP >(*this,lib,"LLVMBuildGEP",SideEffects::worstDefault,"LLVMBuildGEP")
		->args({"B","Pointer","Indices","NumIndices","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3939:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildInBoundsGEP >(*this,lib,"LLVMBuildInBoundsGEP",SideEffects::worstDefault,"LLVMBuildInBoundsGEP")
		->args({"B","Pointer","Indices","NumIndices","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3944:18
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildStructGEP >(*this,lib,"LLVMBuildStructGEP",SideEffects::worstDefault,"LLVMBuildStructGEP")
		->args({"B","Pointer","Idx","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3947:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildGEP2 >(*this,lib,"LLVMBuildGEP2",SideEffects::worstDefault,"LLVMBuildGEP2")
		->args({"B","Ty","Pointer","Indices","NumIndices","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3950:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,LLVMOpaqueValue **,unsigned int,const char *) , LLVMBuildInBoundsGEP2 >(*this,lib,"LLVMBuildInBoundsGEP2",SideEffects::worstDefault,"LLVMBuildInBoundsGEP2")
		->args({"B","Ty","Pointer","Indices","NumIndices","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3953:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueType *,LLVMOpaqueValue *,unsigned int,const char *) , LLVMBuildStructGEP2 >(*this,lib,"LLVMBuildStructGEP2",SideEffects::worstDefault,"LLVMBuildStructGEP2")
		->args({"B","Ty","Pointer","Idx","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3956:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,const char *,const char *) , LLVMBuildGlobalString >(*this,lib,"LLVMBuildGlobalString",SideEffects::worstDefault,"LLVMBuildGlobalString")
		->args({"B","Str","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3958:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,const char *,const char *) , LLVMBuildGlobalStringPtr >(*this,lib,"LLVMBuildGlobalStringPtr",SideEffects::worstDefault,"LLVMBuildGlobalStringPtr")
		->args({"B","Str","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3960:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMGetVolatile >(*this,lib,"LLVMGetVolatile",SideEffects::worstDefault,"LLVMGetVolatile")
		->args({"MemoryAccessInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3961:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetVolatile >(*this,lib,"LLVMSetVolatile",SideEffects::worstDefault,"LLVMSetVolatile")
		->args({"MemoryAccessInst","IsVolatile"});
// from D:\Work\libclang\include\llvm-c/Core.h:3962:10
	addExtern< int (*)(LLVMOpaqueValue *) , LLVMGetWeak >(*this,lib,"LLVMGetWeak",SideEffects::worstDefault,"LLVMGetWeak")
		->args({"CmpXchgInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3963:6
	addExtern< void (*)(LLVMOpaqueValue *,int) , LLVMSetWeak >(*this,lib,"LLVMSetWeak",SideEffects::worstDefault,"LLVMSetWeak")
		->args({"CmpXchgInst","IsWeak"});
// from D:\Work\libclang\include\llvm-c/Core.h:3964:20
	addExtern< LLVMAtomicOrdering (*)(LLVMOpaqueValue *) , LLVMGetOrdering >(*this,lib,"LLVMGetOrdering",SideEffects::worstDefault,"LLVMGetOrdering")
		->args({"MemoryAccessInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3965:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicOrdering) , LLVMSetOrdering >(*this,lib,"LLVMSetOrdering",SideEffects::worstDefault,"LLVMSetOrdering")
		->args({"MemoryAccessInst","Ordering"});
// from D:\Work\libclang\include\llvm-c/Core.h:3966:20
	addExtern< LLVMAtomicRMWBinOp (*)(LLVMOpaqueValue *) , LLVMGetAtomicRMWBinOp >(*this,lib,"LLVMGetAtomicRMWBinOp",SideEffects::worstDefault,"LLVMGetAtomicRMWBinOp")
		->args({"AtomicRMWInst"});
// from D:\Work\libclang\include\llvm-c/Core.h:3967:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMAtomicRMWBinOp) , LLVMSetAtomicRMWBinOp >(*this,lib,"LLVMSetAtomicRMWBinOp",SideEffects::worstDefault,"LLVMSetAtomicRMWBinOp")
		->args({"AtomicRMWInst","BinOp"});
// from D:\Work\libclang\include\llvm-c/Core.h:3970:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildTrunc >(*this,lib,"LLVMBuildTrunc",SideEffects::worstDefault,"LLVMBuildTrunc")
		->args({"","Val","DestTy","Name"});
// from D:\Work\libclang\include\llvm-c/Core.h:3972:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueBuilder *,LLVMOpaqueValue *,LLVMOpaqueType *,const char *) , LLVMBuildZExt >(*this,lib,"LLVMBuildZExt",SideEffects::worstDefault,"LLVMBuildZExt")
		->args({"","Val","DestTy","Name"});
}
}

