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
void Module_dasLLVM::initFunctions_43() {
// from D:\Work\libclang\include\llvm/Config/Targets.def:29:1
	makeExtern< void (*)() , LLVMInitializeMipsTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeMipsTargetMC","LLVMInitializeMipsTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:30:1
	makeExtern< void (*)() , LLVMInitializePowerPCTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializePowerPCTargetMC","LLVMInitializePowerPCTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:31:1
	makeExtern< void (*)() , LLVMInitializeRISCVTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeRISCVTargetMC","LLVMInitializeRISCVTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:32:1
	makeExtern< void (*)() , LLVMInitializeNVPTXTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeNVPTXTargetMC","LLVMInitializeNVPTXTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:33:1
	makeExtern< void (*)() , LLVMInitializeHexagonTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeHexagonTargetMC","LLVMInitializeHexagonTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Targets.def:34:1
	makeExtern< void (*)() , LLVMInitializeWebAssemblyTargetMC , SimNode_ExtFuncCall >(lib,"LLVMInitializeWebAssemblyTargetMC","LLVMInitializeWebAssemblyTargetMC")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:27:1
	makeExtern< void (*)() , LLVMInitializeX86AsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeX86AsmPrinter","LLVMInitializeX86AsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:28:1
	makeExtern< void (*)() , LLVMInitializeARMAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeARMAsmPrinter","LLVMInitializeARMAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:29:1
	makeExtern< void (*)() , LLVMInitializeAArch64AsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeAArch64AsmPrinter","LLVMInitializeAArch64AsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:30:1
	makeExtern< void (*)() , LLVMInitializeMipsAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeMipsAsmPrinter","LLVMInitializeMipsAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:31:1
	makeExtern< void (*)() , LLVMInitializePowerPCAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializePowerPCAsmPrinter","LLVMInitializePowerPCAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:32:1
	makeExtern< void (*)() , LLVMInitializeRISCVAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeRISCVAsmPrinter","LLVMInitializeRISCVAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:33:1
	makeExtern< void (*)() , LLVMInitializeNVPTXAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeNVPTXAsmPrinter","LLVMInitializeNVPTXAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:34:1
	makeExtern< void (*)() , LLVMInitializeHexagonAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeHexagonAsmPrinter","LLVMInitializeHexagonAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:35:1
	makeExtern< void (*)() , LLVMInitializeWebAssemblyAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeWebAssemblyAsmPrinter","LLVMInitializeWebAssemblyAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:27:1
	makeExtern< void (*)() , LLVMInitializeX86AsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeX86AsmParser","LLVMInitializeX86AsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:28:1
	makeExtern< void (*)() , LLVMInitializeARMAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeARMAsmParser","LLVMInitializeARMAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:29:1
	makeExtern< void (*)() , LLVMInitializeAArch64AsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeAArch64AsmParser","LLVMInitializeAArch64AsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:30:1
	makeExtern< void (*)() , LLVMInitializeMipsAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeMipsAsmParser","LLVMInitializeMipsAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:31:1
	makeExtern< void (*)() , LLVMInitializePowerPCAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializePowerPCAsmParser","LLVMInitializePowerPCAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
}
}

