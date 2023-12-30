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
// from D:\Work\libclang\include\llvm-c/Disassembler.h:88:6
	addExtern< void (*)(void *) , LLVMDisasmDispose >(*this,lib,"LLVMDisasmDispose",SideEffects::worstDefault,"LLVMDisasmDispose")
		->args({"DC"});
// from D:\Work\libclang\include\llvm-c/Disassembler.h:100:8
	addExtern< size_t (*)(void *,unsigned char *,uint64_t,uint64_t,char *,size_t) , LLVMDisasmInstruction >(*this,lib,"LLVMDisasmInstruction",SideEffects::worstDefault,"LLVMDisasmInstruction")
		->args({"DC","Bytes","BytesSize","PC","OutString","OutStringSize"});
// from D:\Work\libclang\include\llvm-c/Error.h:44:17
	addExtern< const void * (*)(LLVMOpaqueError *) , LLVMGetErrorTypeId >(*this,lib,"LLVMGetErrorTypeId",SideEffects::worstDefault,"LLVMGetErrorTypeId")
		->args({"Err"});
// from D:\Work\libclang\include\llvm-c/Error.h:52:6
	addExtern< void (*)(LLVMOpaqueError *) , LLVMConsumeError >(*this,lib,"LLVMConsumeError",SideEffects::worstDefault,"LLVMConsumeError")
		->args({"Err"});
// from D:\Work\libclang\include\llvm-c/Error.h:60:7
	addExtern< char * (*)(LLVMOpaqueError *) , LLVMGetErrorMessage >(*this,lib,"LLVMGetErrorMessage",SideEffects::worstDefault,"LLVMGetErrorMessage")
		->args({"Err"});
// from D:\Work\libclang\include\llvm-c/Error.h:65:6
	addExtern< void (*)(char *) , LLVMDisposeErrorMessage >(*this,lib,"LLVMDisposeErrorMessage",SideEffects::worstDefault,"LLVMDisposeErrorMessage")
		->args({"ErrMsg"});
// from D:\Work\libclang\include\llvm-c/Error.h:70:17
	addExtern< const void * (*)() , LLVMGetStringErrorTypeId >(*this,lib,"LLVMGetStringErrorTypeId",SideEffects::worstDefault,"LLVMGetStringErrorTypeId");
// from D:\Work\libclang\include\llvm-c/Error.h:75:14
	addExtern< LLVMOpaqueError * (*)(const char *) , LLVMCreateStringError >(*this,lib,"LLVMCreateStringError",SideEffects::worstDefault,"LLVMCreateStringError")
		->args({"ErrMsg"});
// from D:\Work\libclang\include\llvm/Config/Targets.def:26:1
	addExtern< void (*)() , LLVMInitializeAArch64TargetInfo >(*this,lib,"LLVMInitializeAArch64TargetInfo",SideEffects::worstDefault,"LLVMInitializeAArch64TargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:27:1
	addExtern< void (*)() , LLVMInitializeAMDGPUTargetInfo >(*this,lib,"LLVMInitializeAMDGPUTargetInfo",SideEffects::worstDefault,"LLVMInitializeAMDGPUTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:28:1
	addExtern< void (*)() , LLVMInitializeARMTargetInfo >(*this,lib,"LLVMInitializeARMTargetInfo",SideEffects::worstDefault,"LLVMInitializeARMTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:29:1
	addExtern< void (*)() , LLVMInitializeAVRTargetInfo >(*this,lib,"LLVMInitializeAVRTargetInfo",SideEffects::worstDefault,"LLVMInitializeAVRTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:30:1
	addExtern< void (*)() , LLVMInitializeBPFTargetInfo >(*this,lib,"LLVMInitializeBPFTargetInfo",SideEffects::worstDefault,"LLVMInitializeBPFTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:31:1
	addExtern< void (*)() , LLVMInitializeHexagonTargetInfo >(*this,lib,"LLVMInitializeHexagonTargetInfo",SideEffects::worstDefault,"LLVMInitializeHexagonTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:32:1
	addExtern< void (*)() , LLVMInitializeLanaiTargetInfo >(*this,lib,"LLVMInitializeLanaiTargetInfo",SideEffects::worstDefault,"LLVMInitializeLanaiTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:33:1
	addExtern< void (*)() , LLVMInitializeMipsTargetInfo >(*this,lib,"LLVMInitializeMipsTargetInfo",SideEffects::worstDefault,"LLVMInitializeMipsTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:34:1
	addExtern< void (*)() , LLVMInitializeMSP430TargetInfo >(*this,lib,"LLVMInitializeMSP430TargetInfo",SideEffects::worstDefault,"LLVMInitializeMSP430TargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:35:1
	addExtern< void (*)() , LLVMInitializeNVPTXTargetInfo >(*this,lib,"LLVMInitializeNVPTXTargetInfo",SideEffects::worstDefault,"LLVMInitializeNVPTXTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:36:1
	addExtern< void (*)() , LLVMInitializePowerPCTargetInfo >(*this,lib,"LLVMInitializePowerPCTargetInfo",SideEffects::worstDefault,"LLVMInitializePowerPCTargetInfo");
// from D:\Work\libclang\include\llvm/Config/Targets.def:37:1
	addExtern< void (*)() , LLVMInitializeRISCVTargetInfo >(*this,lib,"LLVMInitializeRISCVTargetInfo",SideEffects::worstDefault,"LLVMInitializeRISCVTargetInfo");
}
}

