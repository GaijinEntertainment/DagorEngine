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
void Module_dasLLVM::initFunctions_46() {
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:27:1
	addExtern< void (*)() , LLVMInitializeAArch64AsmParser >(*this,lib,"LLVMInitializeAArch64AsmParser",SideEffects::worstDefault,"LLVMInitializeAArch64AsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:28:1
	addExtern< void (*)() , LLVMInitializeAMDGPUAsmParser >(*this,lib,"LLVMInitializeAMDGPUAsmParser",SideEffects::worstDefault,"LLVMInitializeAMDGPUAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:29:1
	addExtern< void (*)() , LLVMInitializeARMAsmParser >(*this,lib,"LLVMInitializeARMAsmParser",SideEffects::worstDefault,"LLVMInitializeARMAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:30:1
	addExtern< void (*)() , LLVMInitializeAVRAsmParser >(*this,lib,"LLVMInitializeAVRAsmParser",SideEffects::worstDefault,"LLVMInitializeAVRAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:31:1
	addExtern< void (*)() , LLVMInitializeBPFAsmParser >(*this,lib,"LLVMInitializeBPFAsmParser",SideEffects::worstDefault,"LLVMInitializeBPFAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:32:1
	addExtern< void (*)() , LLVMInitializeHexagonAsmParser >(*this,lib,"LLVMInitializeHexagonAsmParser",SideEffects::worstDefault,"LLVMInitializeHexagonAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:33:1
	addExtern< void (*)() , LLVMInitializeLanaiAsmParser >(*this,lib,"LLVMInitializeLanaiAsmParser",SideEffects::worstDefault,"LLVMInitializeLanaiAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:34:1
	addExtern< void (*)() , LLVMInitializeMipsAsmParser >(*this,lib,"LLVMInitializeMipsAsmParser",SideEffects::worstDefault,"LLVMInitializeMipsAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:35:1
	addExtern< void (*)() , LLVMInitializeMSP430AsmParser >(*this,lib,"LLVMInitializeMSP430AsmParser",SideEffects::worstDefault,"LLVMInitializeMSP430AsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:36:1
	addExtern< void (*)() , LLVMInitializePowerPCAsmParser >(*this,lib,"LLVMInitializePowerPCAsmParser",SideEffects::worstDefault,"LLVMInitializePowerPCAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:37:1
	addExtern< void (*)() , LLVMInitializeRISCVAsmParser >(*this,lib,"LLVMInitializeRISCVAsmParser",SideEffects::worstDefault,"LLVMInitializeRISCVAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:38:1
	addExtern< void (*)() , LLVMInitializeSparcAsmParser >(*this,lib,"LLVMInitializeSparcAsmParser",SideEffects::worstDefault,"LLVMInitializeSparcAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:39:1
	addExtern< void (*)() , LLVMInitializeSystemZAsmParser >(*this,lib,"LLVMInitializeSystemZAsmParser",SideEffects::worstDefault,"LLVMInitializeSystemZAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:40:1
	addExtern< void (*)() , LLVMInitializeVEAsmParser >(*this,lib,"LLVMInitializeVEAsmParser",SideEffects::worstDefault,"LLVMInitializeVEAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:41:1
	addExtern< void (*)() , LLVMInitializeWebAssemblyAsmParser >(*this,lib,"LLVMInitializeWebAssemblyAsmParser",SideEffects::worstDefault,"LLVMInitializeWebAssemblyAsmParser");
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:42:1
	addExtern< void (*)() , LLVMInitializeX86AsmParser >(*this,lib,"LLVMInitializeX86AsmParser",SideEffects::worstDefault,"LLVMInitializeX86AsmParser");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:27:1
	addExtern< void (*)() , LLVMInitializeAArch64Disassembler >(*this,lib,"LLVMInitializeAArch64Disassembler",SideEffects::worstDefault,"LLVMInitializeAArch64Disassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:28:1
	addExtern< void (*)() , LLVMInitializeAMDGPUDisassembler >(*this,lib,"LLVMInitializeAMDGPUDisassembler",SideEffects::worstDefault,"LLVMInitializeAMDGPUDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:29:1
	addExtern< void (*)() , LLVMInitializeARMDisassembler >(*this,lib,"LLVMInitializeARMDisassembler",SideEffects::worstDefault,"LLVMInitializeARMDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:30:1
	addExtern< void (*)() , LLVMInitializeAVRDisassembler >(*this,lib,"LLVMInitializeAVRDisassembler",SideEffects::worstDefault,"LLVMInitializeAVRDisassembler");
}
}

