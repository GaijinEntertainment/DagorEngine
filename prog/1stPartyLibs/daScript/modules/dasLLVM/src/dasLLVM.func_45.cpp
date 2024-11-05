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
// from D:\Work\libclang\include\llvm-c/Target.h:170:24
	makeExtern< int (*)() , LLVMInitializeNativeDisassembler , SimNode_ExtFuncCall >(lib,"LLVMInitializeNativeDisassembler","LLVMInitializeNativeDisassembler")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:186:19
	makeExtern< LLVMOpaqueTargetData * (*)(LLVMOpaqueModule *) , LLVMGetModuleDataLayout , SimNode_ExtFuncCall >(lib,"LLVMGetModuleDataLayout","LLVMGetModuleDataLayout")
		->args({"M"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:193:6
	makeExtern< void (*)(LLVMOpaqueModule *,LLVMOpaqueTargetData *) , LLVMSetModuleDataLayout , SimNode_ExtFuncCall >(lib,"LLVMSetModuleDataLayout","LLVMSetModuleDataLayout")
		->args({"M","DL"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:197:19
	makeExtern< LLVMOpaqueTargetData * (*)(const char *) , LLVMCreateTargetData , SimNode_ExtFuncCall >(lib,"LLVMCreateTargetData","LLVMCreateTargetData")
		->args({"StringRep"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:201:6
	makeExtern< void (*)(LLVMOpaqueTargetData *) , LLVMDisposeTargetData , SimNode_ExtFuncCall >(lib,"LLVMDisposeTargetData","LLVMDisposeTargetData")
		->args({"TD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:206:6
	makeExtern< void (*)(LLVMOpaqueTargetLibraryInfotData *,LLVMOpaquePassManager *) , LLVMAddTargetLibraryInfo , SimNode_ExtFuncCall >(lib,"LLVMAddTargetLibraryInfo","LLVMAddTargetLibraryInfo")
		->args({"TLI","PM"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:212:7
	makeExtern< char * (*)(LLVMOpaqueTargetData *) , LLVMCopyStringRepOfTargetData , SimNode_ExtFuncCall >(lib,"LLVMCopyStringRepOfTargetData","LLVMCopyStringRepOfTargetData")
		->args({"TD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:217:23
	makeExtern< LLVMByteOrdering (*)(LLVMOpaqueTargetData *) , LLVMByteOrder , SimNode_ExtFuncCall >(lib,"LLVMByteOrder","LLVMByteOrder")
		->args({"TD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:221:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *) , LLVMPointerSize , SimNode_ExtFuncCall >(lib,"LLVMPointerSize","LLVMPointerSize")
		->args({"TD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:226:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *,unsigned int) , LLVMPointerSizeForAS , SimNode_ExtFuncCall >(lib,"LLVMPointerSizeForAS","LLVMPointerSizeForAS")
		->args({"TD","AS"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:230:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueTargetData *) , LLVMIntPtrType , SimNode_ExtFuncCall >(lib,"LLVMIntPtrType","LLVMIntPtrType")
		->args({"TD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:235:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueTargetData *,unsigned int) , LLVMIntPtrTypeForAS , SimNode_ExtFuncCall >(lib,"LLVMIntPtrTypeForAS","LLVMIntPtrTypeForAS")
		->args({"TD","AS"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:239:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,LLVMOpaqueTargetData *) , LLVMIntPtrTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMIntPtrTypeInContext","LLVMIntPtrTypeInContext")
		->args({"C","TD"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:244:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *,LLVMOpaqueTargetData *,unsigned int) , LLVMIntPtrTypeForASInContext , SimNode_ExtFuncCall >(lib,"LLVMIntPtrTypeForASInContext","LLVMIntPtrTypeForASInContext")
		->args({"C","TD","AS"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:249:20
	makeExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMSizeOfTypeInBits , SimNode_ExtFuncCall >(lib,"LLVMSizeOfTypeInBits","LLVMSizeOfTypeInBits")
		->args({"TD","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:253:20
	makeExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMStoreSizeOfType , SimNode_ExtFuncCall >(lib,"LLVMStoreSizeOfType","LLVMStoreSizeOfType")
		->args({"TD","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:257:20
	makeExtern< unsigned long long (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMABISizeOfType , SimNode_ExtFuncCall >(lib,"LLVMABISizeOfType","LLVMABISizeOfType")
		->args({"TD","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:261:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMABIAlignmentOfType , SimNode_ExtFuncCall >(lib,"LLVMABIAlignmentOfType","LLVMABIAlignmentOfType")
		->args({"TD","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:265:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMCallFrameAlignmentOfType , SimNode_ExtFuncCall >(lib,"LLVMCallFrameAlignmentOfType","LLVMCallFrameAlignmentOfType")
		->args({"TD","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Target.h:269:10
	makeExtern< unsigned int (*)(LLVMOpaqueTargetData *,LLVMOpaqueType *) , LLVMPreferredAlignmentOfType , SimNode_ExtFuncCall >(lib,"LLVMPreferredAlignmentOfType","LLVMPreferredAlignmentOfType")
		->args({"TD","Ty"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

