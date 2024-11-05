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
void Module_dasLLVM::initFunctions_42() {
// from D:\Work\libclang\include\llvm/Config/Targets.def:27:1
	makeExtern< void (*)() , LLVMInitializeARMTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeARMTargetInfo","LLVMInitializeARMTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:28:1
	makeExtern< void (*)() , LLVMInitializeAArch64TargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeAArch64TargetInfo","LLVMInitializeAArch64TargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:29:1
	makeExtern< void (*)() , LLVMInitializeMipsTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeMipsTargetInfo","LLVMInitializeMipsTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:30:1
	makeExtern< void (*)() , LLVMInitializePowerPCTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializePowerPCTargetInfo","LLVMInitializePowerPCTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:31:1
	makeExtern< void (*)() , LLVMInitializeRISCVTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeRISCVTargetInfo","LLVMInitializeRISCVTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:32:1
	makeExtern< void (*)() , LLVMInitializeNVPTXTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeNVPTXTargetInfo","LLVMInitializeNVPTXTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:33:1
	makeExtern< void (*)() , LLVMInitializeHexagonTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeHexagonTargetInfo","LLVMInitializeHexagonTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:34:1
	makeExtern< void (*)() , LLVMInitializeWebAssemblyTargetInfo , SimNode_ExtFuncCall >(lib,"LLVMInitializeWebAssemblyTargetInfo","LLVMInitializeWebAssemblyTargetInfo")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:26:1
	makeExtern< void (*)() , LLVMInitializeX86Target , SimNode_ExtFuncCall >(lib,"LLVMInitializeX86Target","LLVMInitializeX86Target")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:27:1
	makeExtern< void (*)() , LLVMInitializeARMTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeARMTarget","LLVMInitializeARMTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:28:1
	makeExtern< void (*)() , LLVMInitializeAArch64Target , SimNode_ExtFuncCall >(lib,"LLVMInitializeAArch64Target","LLVMInitializeAArch64Target")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:29:1
	makeExtern< void (*)() , LLVMInitializeMipsTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeMipsTarget","LLVMInitializeMipsTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:30:1
	makeExtern< void (*)() , LLVMInitializePowerPCTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializePowerPCTarget","LLVMInitializePowerPCTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:31:1
	makeExtern< void (*)() , LLVMInitializeRISCVTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeRISCVTarget","LLVMInitializeRISCVTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:32:1
	makeExtern< void (*)() , LLVMInitializeNVPTXTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeNVPTXTarget","LLVMInitializeNVPTXTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:33:1
	makeExtern< void (*)() , LLVMInitializeHexagonTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeHexagonTarget","LLVMInitializeHexagonTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:34:1
	makeExtern< void (*)() , LLVMInitializeWebAssemblyTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeWebAssemblyTarget","LLVMInitializeWebAssemblyTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:26:1
	makeExtern< void (*)() , LLVMInitializeX86TargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeX86TargetMC","LLVMInitializeX86TargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:27:1
	makeExtern< void (*)() , LLVMInitializeARMTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeARMTargetMC","LLVMInitializeARMTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:28:1
	makeExtern< void (*)() , LLVMInitializeAArch64TargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeAArch64TargetMC","LLVMInitializeAArch64TargetMC")
		->addToModule(*this, SideEffects::worstDefault);
}
}

