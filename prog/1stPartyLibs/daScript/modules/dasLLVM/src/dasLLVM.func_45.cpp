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
void Module_dasLLVM::initFunctions_45() {
// from D:\Work\libclang\include\llvm/Config/Targets.def:42:1
	addExtern< void (*)() , LLVMInitializeX86TargetMC >(*this,lib,"LLVMInitializeX86TargetMC",SideEffects::worstDefault,"LLVMInitializeX86TargetMC");
// from D:\Work\libclang\include\llvm/Config/Targets.def:43:1
	addExtern< void (*)() , LLVMInitializeXCoreTargetMC >(*this,lib,"LLVMInitializeXCoreTargetMC",SideEffects::worstDefault,"LLVMInitializeXCoreTargetMC");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:27:1
	addExtern< void (*)() , LLVMInitializeAArch64AsmPrinter >(*this,lib,"LLVMInitializeAArch64AsmPrinter",SideEffects::worstDefault,"LLVMInitializeAArch64AsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:28:1
	addExtern< void (*)() , LLVMInitializeAMDGPUAsmPrinter >(*this,lib,"LLVMInitializeAMDGPUAsmPrinter",SideEffects::worstDefault,"LLVMInitializeAMDGPUAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:29:1
	addExtern< void (*)() , LLVMInitializeARMAsmPrinter >(*this,lib,"LLVMInitializeARMAsmPrinter",SideEffects::worstDefault,"LLVMInitializeARMAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:30:1
	addExtern< void (*)() , LLVMInitializeAVRAsmPrinter >(*this,lib,"LLVMInitializeAVRAsmPrinter",SideEffects::worstDefault,"LLVMInitializeAVRAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:31:1
	addExtern< void (*)() , LLVMInitializeBPFAsmPrinter >(*this,lib,"LLVMInitializeBPFAsmPrinter",SideEffects::worstDefault,"LLVMInitializeBPFAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:32:1
	addExtern< void (*)() , LLVMInitializeHexagonAsmPrinter >(*this,lib,"LLVMInitializeHexagonAsmPrinter",SideEffects::worstDefault,"LLVMInitializeHexagonAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:33:1
	addExtern< void (*)() , LLVMInitializeLanaiAsmPrinter >(*this,lib,"LLVMInitializeLanaiAsmPrinter",SideEffects::worstDefault,"LLVMInitializeLanaiAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:34:1
	addExtern< void (*)() , LLVMInitializeMipsAsmPrinter >(*this,lib,"LLVMInitializeMipsAsmPrinter",SideEffects::worstDefault,"LLVMInitializeMipsAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:35:1
	addExtern< void (*)() , LLVMInitializeMSP430AsmPrinter >(*this,lib,"LLVMInitializeMSP430AsmPrinter",SideEffects::worstDefault,"LLVMInitializeMSP430AsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:36:1
	addExtern< void (*)() , LLVMInitializeNVPTXAsmPrinter >(*this,lib,"LLVMInitializeNVPTXAsmPrinter",SideEffects::worstDefault,"LLVMInitializeNVPTXAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:37:1
	addExtern< void (*)() , LLVMInitializePowerPCAsmPrinter >(*this,lib,"LLVMInitializePowerPCAsmPrinter",SideEffects::worstDefault,"LLVMInitializePowerPCAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:38:1
	addExtern< void (*)() , LLVMInitializeRISCVAsmPrinter >(*this,lib,"LLVMInitializeRISCVAsmPrinter",SideEffects::worstDefault,"LLVMInitializeRISCVAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:39:1
	addExtern< void (*)() , LLVMInitializeSparcAsmPrinter >(*this,lib,"LLVMInitializeSparcAsmPrinter",SideEffects::worstDefault,"LLVMInitializeSparcAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:40:1
	addExtern< void (*)() , LLVMInitializeSystemZAsmPrinter >(*this,lib,"LLVMInitializeSystemZAsmPrinter",SideEffects::worstDefault,"LLVMInitializeSystemZAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:41:1
	addExtern< void (*)() , LLVMInitializeVEAsmPrinter >(*this,lib,"LLVMInitializeVEAsmPrinter",SideEffects::worstDefault,"LLVMInitializeVEAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:42:1
	addExtern< void (*)() , LLVMInitializeWebAssemblyAsmPrinter >(*this,lib,"LLVMInitializeWebAssemblyAsmPrinter",SideEffects::worstDefault,"LLVMInitializeWebAssemblyAsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:43:1
	addExtern< void (*)() , LLVMInitializeX86AsmPrinter >(*this,lib,"LLVMInitializeX86AsmPrinter",SideEffects::worstDefault,"LLVMInitializeX86AsmPrinter");
// from D:\Work\libclang\include\llvm/Config/AsmPrinters.def:44:1
	addExtern< void (*)() , LLVMInitializeXCoreAsmPrinter >(*this,lib,"LLVMInitializeXCoreAsmPrinter",SideEffects::worstDefault,"LLVMInitializeXCoreAsmPrinter");
}
}

