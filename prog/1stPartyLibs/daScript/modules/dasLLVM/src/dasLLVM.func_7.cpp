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
void Module_dasLLVM::initFunctions_7() {
// from D:\Work\libclang\include\llvm-c/Core.h:1187:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMInt1Type , SimNode_ExtFuncCall >(lib,"LLVMInt1Type","LLVMInt1Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1188:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMInt8Type , SimNode_ExtFuncCall >(lib,"LLVMInt8Type","LLVMInt8Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1189:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMInt16Type , SimNode_ExtFuncCall >(lib,"LLVMInt16Type","LLVMInt16Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1190:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMInt32Type , SimNode_ExtFuncCall >(lib,"LLVMInt32Type","LLVMInt32Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1191:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMInt64Type , SimNode_ExtFuncCall >(lib,"LLVMInt64Type","LLVMInt64Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1192:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMInt128Type , SimNode_ExtFuncCall >(lib,"LLVMInt128Type","LLVMInt128Type")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1193:13
	makeExtern< LLVMOpaqueType * (*)(unsigned int) , LLVMIntType , SimNode_ExtFuncCall >(lib,"LLVMIntType","LLVMIntType")
		->args({"NumBits"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1194:10
	makeExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetIntTypeWidth , SimNode_ExtFuncCall >(lib,"LLVMGetIntTypeWidth","LLVMGetIntTypeWidth")
		->args({"IntegerTy"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1209:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMHalfTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMHalfTypeInContext","LLVMHalfTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1214:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMBFloatTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMBFloatTypeInContext","LLVMBFloatTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1219:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMFloatTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMFloatTypeInContext","LLVMFloatTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1224:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMDoubleTypeInContext , SimNode_ExtFuncCall >(lib,"LLVMDoubleTypeInContext","LLVMDoubleTypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1229:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMX86FP80TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMX86FP80TypeInContext","LLVMX86FP80TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1235:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMFP128TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMFP128TypeInContext","LLVMFP128TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1240:13
	makeExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMPPCFP128TypeInContext , SimNode_ExtFuncCall >(lib,"LLVMPPCFP128TypeInContext","LLVMPPCFP128TypeInContext")
		->args({"C"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1247:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMHalfType , SimNode_ExtFuncCall >(lib,"LLVMHalfType","LLVMHalfType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1248:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMBFloatType , SimNode_ExtFuncCall >(lib,"LLVMBFloatType","LLVMBFloatType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1249:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMFloatType , SimNode_ExtFuncCall >(lib,"LLVMFloatType","LLVMFloatType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1250:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMDoubleType , SimNode_ExtFuncCall >(lib,"LLVMDoubleType","LLVMDoubleType")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Core.h:1251:13
	makeExtern< LLVMOpaqueType * (*)() , LLVMX86FP80Type , SimNode_ExtFuncCall >(lib,"LLVMX86FP80Type","LLVMX86FP80Type")
		->addToModule(*this, SideEffects::worstDefault);
}
}

