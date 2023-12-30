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
void Module_dasLLVM::initFunctions_47() {
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:31:1
	addExtern< void (*)() , LLVMInitializeBPFDisassembler >(*this,lib,"LLVMInitializeBPFDisassembler",SideEffects::worstDefault,"LLVMInitializeBPFDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:32:1
	addExtern< void (*)() , LLVMInitializeHexagonDisassembler >(*this,lib,"LLVMInitializeHexagonDisassembler",SideEffects::worstDefault,"LLVMInitializeHexagonDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:33:1
	addExtern< void (*)() , LLVMInitializeLanaiDisassembler >(*this,lib,"LLVMInitializeLanaiDisassembler",SideEffects::worstDefault,"LLVMInitializeLanaiDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:34:1
	addExtern< void (*)() , LLVMInitializeMipsDisassembler >(*this,lib,"LLVMInitializeMipsDisassembler",SideEffects::worstDefault,"LLVMInitializeMipsDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:35:1
	addExtern< void (*)() , LLVMInitializeMSP430Disassembler >(*this,lib,"LLVMInitializeMSP430Disassembler",SideEffects::worstDefault,"LLVMInitializeMSP430Disassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:36:1
	addExtern< void (*)() , LLVMInitializePowerPCDisassembler >(*this,lib,"LLVMInitializePowerPCDisassembler",SideEffects::worstDefault,"LLVMInitializePowerPCDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:37:1
	addExtern< void (*)() , LLVMInitializeRISCVDisassembler >(*this,lib,"LLVMInitializeRISCVDisassembler",SideEffects::worstDefault,"LLVMInitializeRISCVDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:38:1
	addExtern< void (*)() , LLVMInitializeSparcDisassembler >(*this,lib,"LLVMInitializeSparcDisassembler",SideEffects::worstDefault,"LLVMInitializeSparcDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:39:1
	addExtern< void (*)() , LLVMInitializeSystemZDisassembler >(*this,lib,"LLVMInitializeSystemZDisassembler",SideEffects::worstDefault,"LLVMInitializeSystemZDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:40:1
	addExtern< void (*)() , LLVMInitializeVEDisassembler >(*this,lib,"LLVMInitializeVEDisassembler",SideEffects::worstDefault,"LLVMInitializeVEDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:41:1
	addExtern< void (*)() , LLVMInitializeWebAssemblyDisassembler >(*this,lib,"LLVMInitializeWebAssemblyDisassembler",SideEffects::worstDefault,"LLVMInitializeWebAssemblyDisassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:42:1
	addExtern< void (*)() , LLVMInitializeX86Disassembler >(*this,lib,"LLVMInitializeX86Disassembler",SideEffects::worstDefault,"LLVMInitializeX86Disassembler");
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:43:1
	addExtern< void (*)() , LLVMInitializeXCoreDisassembler >(*this,lib,"LLVMInitializeXCoreDisassembler",SideEffects::worstDefault,"LLVMInitializeXCoreDisassembler");
// from D:\Work\libclang\include\llvm-c/Target.h:76:20
	addExtern< void (*)() , LLVMInitializeAllTargetInfos >(*this,lib,"LLVMInitializeAllTargetInfos",SideEffects::worstDefault,"LLVMInitializeAllTargetInfos");
// from D:\Work\libclang\include\llvm-c/Target.h:85:20
	addExtern< void (*)() , LLVMInitializeAllTargets >(*this,lib,"LLVMInitializeAllTargets",SideEffects::worstDefault,"LLVMInitializeAllTargets");
// from D:\Work\libclang\include\llvm-c/Target.h:94:20
	addExtern< void (*)() , LLVMInitializeAllTargetMCs >(*this,lib,"LLVMInitializeAllTargetMCs",SideEffects::worstDefault,"LLVMInitializeAllTargetMCs");
// from D:\Work\libclang\include\llvm-c/Target.h:103:20
	addExtern< void (*)() , LLVMInitializeAllAsmPrinters >(*this,lib,"LLVMInitializeAllAsmPrinters",SideEffects::worstDefault,"LLVMInitializeAllAsmPrinters");
// from D:\Work\libclang\include\llvm-c/Target.h:112:20
	addExtern< void (*)() , LLVMInitializeAllAsmParsers >(*this,lib,"LLVMInitializeAllAsmParsers",SideEffects::worstDefault,"LLVMInitializeAllAsmParsers");
// from D:\Work\libclang\include\llvm-c/Target.h:121:20
	addExtern< void (*)() , LLVMInitializeAllDisassemblers >(*this,lib,"LLVMInitializeAllDisassemblers",SideEffects::worstDefault,"LLVMInitializeAllDisassemblers");
// from D:\Work\libclang\include\llvm-c/Target.h:131:24
	addExtern< int (*)() , LLVMInitializeNativeTarget >(*this,lib,"LLVMInitializeNativeTarget",SideEffects::worstDefault,"LLVMInitializeNativeTarget");
}
}

