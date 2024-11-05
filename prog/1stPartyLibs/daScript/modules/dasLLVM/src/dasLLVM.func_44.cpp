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
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:32:1
	makeExtern< void (*)() , LLVMInitializeRISCVAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeRISCVAsmParser","LLVMInitializeRISCVAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:33:1
	makeExtern< void (*)() , LLVMInitializeHexagonAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeHexagonAsmParser","LLVMInitializeHexagonAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/AsmParsers.def:34:1
	makeExtern< void (*)() , LLVMInitializeWebAssemblyAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeWebAssemblyAsmParser","LLVMInitializeWebAssemblyAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:27:1
	makeExtern< void (*)() , LLVMInitializeX86Disassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeX86Disassembler","LLVMInitializeX86Disassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:28:1
	makeExtern< void (*)() , LLVMInitializeARMDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeARMDisassembler","LLVMInitializeARMDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:29:1
	makeExtern< void (*)() , LLVMInitializeAArch64Disassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeAArch64Disassembler","LLVMInitializeAArch64Disassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:30:1
	makeExtern< void (*)() , LLVMInitializeMipsDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeMipsDisassembler","LLVMInitializeMipsDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:31:1
	makeExtern< void (*)() , LLVMInitializePowerPCDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializePowerPCDisassembler","LLVMInitializePowerPCDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:32:1
	makeExtern< void (*)() , LLVMInitializeRISCVDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeRISCVDisassembler","LLVMInitializeRISCVDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:33:1
	makeExtern< void (*)() , LLVMInitializeHexagonDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeHexagonDisassembler","LLVMInitializeHexagonDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm/Config/Disassemblers.def:34:1
	makeExtern< void (*)() , LLVMInitializeWebAssemblyDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeWebAssemblyDisassembler","LLVMInitializeWebAssemblyDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:76:20
	makeExtern< void (*)() , LLVMInitializeAllTargetInfos , SimNode_ExtFuncCall >(lib,"LLVMInitializeAllTargetInfos","LLVMInitializeAllTargetInfos")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:85:20
	makeExtern< void (*)() , LLVMInitializeAllTargets , SimNode_ExtFuncCall >(lib,"LLVMInitializeAllTargets","LLVMInitializeAllTargets")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:94:20
	makeExtern< void (*)() , LLVMInitializeAllTargetMCs , SimNode_ExtFuncCall >(lib,"LLVMInitializeAllTargetMCs","LLVMInitializeAllTargetMCs")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:103:20
	makeExtern< void (*)() , LLVMInitializeAllAsmPrinters , SimNode_ExtFuncCall >(lib,"LLVMInitializeAllAsmPrinters","LLVMInitializeAllAsmPrinters")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:112:20
	makeExtern< void (*)() , LLVMInitializeAllAsmParsers , SimNode_ExtFuncCall >(lib,"LLVMInitializeAllAsmParsers","LLVMInitializeAllAsmParsers")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:121:20
	makeExtern< void (*)() , LLVMInitializeAllDisassemblers , SimNode_ExtFuncCall >(lib,"LLVMInitializeAllDisassemblers","LLVMInitializeAllDisassemblers")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:131:24
	makeExtern< int (*)() , LLVMInitializeNativeTarget , SimNode_ExtFuncCall >(lib,"LLVMInitializeNativeTarget","LLVMInitializeNativeTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:146:24
	makeExtern< int (*)() , LLVMInitializeNativeAsmParser , SimNode_ExtFuncCall >(lib,"LLVMInitializeNativeAsmParser","LLVMInitializeNativeAsmParser")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:158:24
	makeExtern< int (*)() , LLVMInitializeNativeAsmPrinter , SimNode_ExtFuncCall >(lib,"LLVMInitializeNativeAsmPrinter","LLVMInitializeNativeAsmPrinter")
		->addToModule(*this, SideEffects::worstDefault);
}
}

