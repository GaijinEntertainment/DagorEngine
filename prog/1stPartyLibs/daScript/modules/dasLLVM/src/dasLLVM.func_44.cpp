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
void Module_dasLLVM::initFunctions_44() {
// from D:\Work\libclang\include\llvm/Config/Targets.def:40:1
	addExtern< void (*)() , LLVMInitializeVETarget >(*this,lib,"LLVMInitializeVETarget",SideEffects::worstDefault,"LLVMInitializeVETarget");
// from D:\Work\libclang\include\llvm/Config/Targets.def:41:1
	addExtern< void (*)() , LLVMInitializeWebAssemblyTarget >(*this,lib,"LLVMInitializeWebAssemblyTarget",SideEffects::worstDefault,"LLVMInitializeWebAssemblyTarget");
// from D:\Work\libclang\include\llvm/Config/Targets.def:42:1
	addExtern< void (*)() , LLVMInitializeX86Target >(*this,lib,"LLVMInitializeX86Target",SideEffects::worstDefault,"LLVMInitializeX86Target");
// from D:\Work\libclang\include\llvm/Config/Targets.def:43:1
	addExtern< void (*)() , LLVMInitializeXCoreTarget >(*this,lib,"LLVMInitializeXCoreTarget",SideEffects::worstDefault,"LLVMInitializeXCoreTarget");
// from D:\Work\libclang\include\llvm/Config/Targets.def:26:1
	addExtern< void (*)() , LLVMInitializeAArch64TargetMC >(*this,lib,"LLVMInitializeAArch64TargetMC",SideEffects::worstDefault,"LLVMInitializeAArch64TargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:27:1
	addExtern< void (*)() , LLVMInitializeAMDGPUTargetMC >(*this,lib,"LLVMInitializeAMDGPUTargetMC",SideEffects::worstDefault,"LLVMInitializeAMDGPUTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:28:1
	addExtern< void (*)() , LLVMInitializeARMTargetMC >(*this,lib,"LLVMInitializeARMTargetMC",SideEffects::worstDefault,"LLVMInitializeARMTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:29:1
	addExtern< void (*)() , LLVMInitializeAVRTargetMC >(*this,lib,"LLVMInitializeAVRTargetMC",SideEffects::worstDefault,"LLVMInitializeAVRTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:30:1
	addExtern< void (*)() , LLVMInitializeBPFTargetMC >(*this,lib,"LLVMInitializeBPFTargetMC",SideEffects::worstDefault,"LLVMInitializeBPFTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:31:1
	addExtern< void (*)() , LLVMInitializeHexagonTargetMC >(*this,lib,"LLVMInitializeHexagonTargetMC",SideEffects::worstDefault,"LLVMInitializeHexagonTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:32:1
	addExtern< void (*)() , LLVMInitializeLanaiTargetMC >(*this,lib,"LLVMInitializeLanaiTargetMC",SideEffects::worstDefault,"LLVMInitializeLanaiTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:33:1
	addExtern< void (*)() , LLVMInitializeMipsTargetMC >(*this,lib,"LLVMInitializeMipsTargetMC",SideEffects::worstDefault,"LLVMInitializeMipsTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:34:1
	addExtern< void (*)() , LLVMInitializeMSP430TargetMC >(*this,lib,"LLVMInitializeMSP430TargetMC",SideEffects::worstDefault,"LLVMInitializeMSP430TargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:35:1
	addExtern< void (*)() , LLVMInitializeNVPTXTargetMC >(*this,lib,"LLVMInitializeNVPTXTargetMC",SideEffects::worstDefault,"LLVMInitializeNVPTXTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:36:1
	addExtern< void (*)() , LLVMInitializePowerPCTargetMC >(*this,lib,"LLVMInitializePowerPCTargetMC",SideEffects::worstDefault,"LLVMInitializePowerPCTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:37:1
	addExtern< void (*)() , LLVMInitializeRISCVTargetMC >(*this,lib,"LLVMInitializeRISCVTargetMC",SideEffects::worstDefault,"LLVMInitializeRISCVTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:38:1
	addExtern< void (*)() , LLVMInitializeSparcTargetMC >(*this,lib,"LLVMInitializeSparcTargetMC",SideEffects::worstDefault,"LLVMInitializeSparcTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:39:1
	addExtern< void (*)() , LLVMInitializeSystemZTargetMC >(*this,lib,"LLVMInitializeSystemZTargetMC",SideEffects::worstDefault,"LLVMInitializeSystemZTargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:40:1
	addExtern< void (*)() , LLVMInitializeVETargetMC >(*this,lib,"LLVMInitializeVETargetMC",SideEffects::worstDefault,"LLVMInitializeVETargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:41:1
	addExtern< void (*)() , LLVMInitializeWebAssemblyTargetMC >(*this,lib,"LLVMInitializeWebAssemblyTargetMC",SideEffects::worstDefault,"LLVMInitializeWebAssemblyTargetMC");
}
}

