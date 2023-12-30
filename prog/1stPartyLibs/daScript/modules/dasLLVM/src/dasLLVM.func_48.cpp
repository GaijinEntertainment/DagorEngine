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
void Module_dasLLVM::initFunctions_48() {
// from D:\Work\libclang\include\llvm-c/Target.h:146:24
	addExtern< int (*)() , LLVMInitializeNativeAsmParser >(*this,lib,"LLVMInitializeNativeAsmParser",SideEffects::worstDefault,"LLVMInitializeNativeAsmParser");
// from D:\Work\libclang\include\llvm-c/Target.h:158:24
	addExtern< int (*)() , LLVMInitializeNativeAsmPrinter >(*this,lib,"LLVMInitializeNativeAsmPrinter",SideEffects::worstDefault,"LLVMInitializeNativeAsmPrinter");
// from D:\Work\libclang\include\llvm-c/Target.h:170:24
	addExtern< int (*)() , LLVMInitializeNativeDisassembler >(*this,lib,"LLVMInitializeNativeDisassembler",SideEffects::worstDefault,"LLVMInitializeNativeDisassembler");
// from D:\Work\libclang\include\llvm-c/Target.h:186:19
	addExtern< LLVMOpaqueTargetData * (*)(LLVMOpaqueModule *) , LLVMGetModuleDataLayout >(*this,lib,"LLVMGetModuleDataLayout",SideEffects::worstDefault,"LLVMGetModuleDataLayout")
		->args({"M"});
// from D:\Work\libclang\include\llvm-c/Target.h:193:6
	addExtern< void (*)(LLVMOpaqueModule *,LLVMOpaqueTargetData *) , LLVMSetModuleDataLayout >(*this,lib,"LLVMSetModuleDataLayout",SideEffects::worstDefault,"LLVMSetModuleDataLayout")
		->args({"M","DL"});
// from D:\Work\libclang\include\llvm-c/Target.h:197:19
	addExtern< LLVMOpaqueTargetData * (*)(const char *) , LLVMCreateTargetData >(*this,lib,"LLVMCreateTargetData",SideEffects::worstDefault,"LLVMCreateTargetData")
		->args({"StringRep"});
// from D:\Work\libclang\include\llvm-c/Target.h:201:6
	addExtern< void (*)(LLVMOpaqueTargetData *) , LLVMDisposeTargetData >(*this,lib,"LLVMDisposeTargetData",SideEffects::worstDefault,"LLVMDisposeTargetData")
		->args({"TD"});
// from D:\Work\libclang\include\llvm-c/Target.h:206:6
	addExtern< void (*)(LLVMOpaqueTargetLibraryInfotData *,LLVMOpaquePassManager *) , LLVMAddTargetLibraryInfo >(*this,lib,"LLVMAddTargetLibraryInfo",SideEffects::worstDefault,"LLVMAddTargetLibraryInfo")
		->args({"TLI","PM"});
// from D:\Work\libclang\include\llvm-c/Target.h:212:7
	addExtern< char * (*)(LLVMOpaqueTargetData *) , LLVMCopyStringRepOfTargetData >(*this,lib,"LLVMCopyStringRepOfTargetData",SideEffects::worstDefault,"LLVMCopyStringRepOfTargetData")
		->args({"TD"});
// from D:\Work\libclang\include\llvm-c/Target.h:217:23
	addExtern< LLVMByteOrdering (*)(LLVMOpaqueTargetData *) , LLVMByteOrder >(*this,lib,"LLVMByteOrder",SideEffects::worstDefault,"LLVMByteOrder")
		->args({"TD"});
// from D:\Work\libclang\include\llvm-c/Target.h:221:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *) , LLVMPointerSize >(*this,lib,"LLVMPointerSize",SideEffects::worstDefault,"LLVMPointerSize")
		->args({"TD"});
// from D:\Work\libclang\include\llvm-c/Target.h:226:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *,unsigned int) , LLVMPointerSizeForAS >(*this,lib,"LLVMPointerSizeForAS",SideEffects::worstDefault,"LLVMPointerSizeForAS")
		->args({"TD","AS"});
// from D:\Work\libclang\include\llvm-c/Target.h:230:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueTargetData *) , LLVMIntPtrType >(*this,lib,"LLVMIntPtrType",SideEffects::worstDefault,"LLVMIntPtrType")
		->args({"TD"});
// from D:\Work\libclang\include\llvm-c/Target.h:235:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueTargetData *,unsigned int) , LLVMIntPtrTypeForAS >(*this,lib,"LLVMIntPtrTypeForAS",SideEffects::worstDefault,"LLVMIntPtrTypeForAS")
		->args({"TD","AS"});
// from D:\Work\libclang\include\llvm-c/Target.h:239:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,LLVMOpaqueTargetData *) , LLVMIntPtrTypeInContext >(*this,lib,"LLVMIntPtrTypeInContext",SideEffects::worstDefault,"LLVMIntPtrTypeInContext")
		->args({"C","TD"});
// from D:\Work\libclang\include\llvm-c/Target.h:244:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,LLVMOpaqueTargetData *,unsigned int) , LLVMIntPtrTypeForASInContext >(*this,lib,"LLVMIntPtrTypeForASInContext",SideEffects::worstDefault,"LLVMIntPtrTypeForASInContext")
		->args({"C","TD","AS"});
// from D:\Work\libclang\include\llvm-c/Target.h:249:20
	addExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMSizeOfTypeInBits >(*this,lib,"LLVMSizeOfTypeInBits",SideEffects::worstDefault,"LLVMSizeOfTypeInBits")
		->args({"TD","Ty"});
// from D:\Work\libclang\include\llvm-c/Target.h:253:20
	addExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMStoreSizeOfType >(*this,lib,"LLVMStoreSizeOfType",SideEffects::worstDefault,"LLVMStoreSizeOfType")
		->args({"TD","Ty"});
// from D:\Work\libclang\include\llvm-c/Target.h:257:20
	addExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMABISizeOfType >(*this,lib,"LLVMABISizeOfType",SideEffects::worstDefault,"LLVMABISizeOfType")
		->args({"TD","Ty"});
// from D:\Work\libclang\include\llvm-c/Target.h:261:10
	addExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMABIAlignmentOfType >(*this,lib,"LLVMABIAlignmentOfType",SideEffects::worstDefault,"LLVMABIAlignmentOfType")
		->args({"TD","Ty"});
}
}

