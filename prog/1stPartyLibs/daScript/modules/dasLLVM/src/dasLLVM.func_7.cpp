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
// from D:\Work\libclang\include\llvm-c/Core.h:1176:13
	addExtern< LLVMOpaqueType * (*)() , LLVMInt8Type >(*this,lib,"LLVMInt8Type",SideEffects::worstDefault,"LLVMInt8Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1177:13
	addExtern< LLVMOpaqueType * (*)() , LLVMInt16Type >(*this,lib,"LLVMInt16Type",SideEffects::worstDefault,"LLVMInt16Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1178:13
	addExtern< LLVMOpaqueType * (*)() , LLVMInt32Type >(*this,lib,"LLVMInt32Type",SideEffects::worstDefault,"LLVMInt32Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1179:13
	addExtern< LLVMOpaqueType * (*)() , LLVMInt64Type >(*this,lib,"LLVMInt64Type",SideEffects::worstDefault,"LLVMInt64Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1180:13
	addExtern< LLVMOpaqueType * (*)() , LLVMInt128Type >(*this,lib,"LLVMInt128Type",SideEffects::worstDefault,"LLVMInt128Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1181:13
	addExtern< LLVMOpaqueType * (*)(unsigned int) , LLVMIntType >(*this,lib,"LLVMIntType",SideEffects::worstDefault,"LLVMIntType")
		->args({"NumBits"});
// from D:\Work\libclang\include\llvm-c/Core.h:1182:10
	addExtern< unsigned int (*)(LLVMOpaqueType *) , LLVMGetIntTypeWidth >(*this,lib,"LLVMGetIntTypeWidth",SideEffects::worstDefault,"LLVMGetIntTypeWidth")
		->args({"IntegerTy"});
// from D:\Work\libclang\include\llvm-c/Core.h:1197:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMHalfTypeInContext >(*this,lib,"LLVMHalfTypeInContext",SideEffects::worstDefault,"LLVMHalfTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1202:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMBFloatTypeInContext >(*this,lib,"LLVMBFloatTypeInContext",SideEffects::worstDefault,"LLVMBFloatTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1207:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMFloatTypeInContext >(*this,lib,"LLVMFloatTypeInContext",SideEffects::worstDefault,"LLVMFloatTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1212:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMDoubleTypeInContext >(*this,lib,"LLVMDoubleTypeInContext",SideEffects::worstDefault,"LLVMDoubleTypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1217:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMX86FP80TypeInContext >(*this,lib,"LLVMX86FP80TypeInContext",SideEffects::worstDefault,"LLVMX86FP80TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1223:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMFP128TypeInContext >(*this,lib,"LLVMFP128TypeInContext",SideEffects::worstDefault,"LLVMFP128TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1228:13
	addExtern< LLVMOpaqueType * (*)(LLVMOpaqueContext *) , LLVMPPCFP128TypeInContext >(*this,lib,"LLVMPPCFP128TypeInContext",SideEffects::worstDefault,"LLVMPPCFP128TypeInContext")
		->args({"C"});
// from D:\Work\libclang\include\llvm-c/Core.h:1235:13
	addExtern< LLVMOpaqueType * (*)() , LLVMHalfType >(*this,lib,"LLVMHalfType",SideEffects::worstDefault,"LLVMHalfType");
// from D:\Work\libclang\include\llvm-c/Core.h:1236:13
	addExtern< LLVMOpaqueType * (*)() , LLVMBFloatType >(*this,lib,"LLVMBFloatType",SideEffects::worstDefault,"LLVMBFloatType");
// from D:\Work\libclang\include\llvm-c/Core.h:1237:13
	addExtern< LLVMOpaqueType * (*)() , LLVMFloatType >(*this,lib,"LLVMFloatType",SideEffects::worstDefault,"LLVMFloatType");
// from D:\Work\libclang\include\llvm-c/Core.h:1238:13
	addExtern< LLVMOpaqueType * (*)() , LLVMDoubleType >(*this,lib,"LLVMDoubleType",SideEffects::worstDefault,"LLVMDoubleType");
// from D:\Work\libclang\include\llvm-c/Core.h:1239:13
	addExtern< LLVMOpaqueType * (*)() , LLVMX86FP80Type >(*this,lib,"LLVMX86FP80Type",SideEffects::worstDefault,"LLVMX86FP80Type");
// from D:\Work\libclang\include\llvm-c/Core.h:1240:13
	addExtern< LLVMOpaqueType * (*)() , LLVMFP128Type >(*this,lib,"LLVMFP128Type",SideEffects::worstDefault,"LLVMFP128Type");
}
}

