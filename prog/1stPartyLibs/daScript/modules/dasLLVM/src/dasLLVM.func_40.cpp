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
void Module_dasLLVM::initFunctions_40() {
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1061:10
	makeExtern< uint64_t (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetOffsetInBits , SimNode_ExtFuncCall >(lib,"LLVMDITypeGetOffsetInBits","LLVMDITypeGetOffsetInBits")
		->args({"DType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1069:10
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetAlignInBits , SimNode_ExtFuncCall >(lib,"LLVMDITypeGetAlignInBits","LLVMDITypeGetAlignInBits")
		->args({"DType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1077:10
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetLine , SimNode_ExtFuncCall >(lib,"LLVMDITypeGetLine","LLVMDITypeGetLine")
		->args({"DType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1085:13
	makeExtern< LLVMDIFlags (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetFlags , SimNode_ExtFuncCall >(lib,"LLVMDITypeGetFlags","LLVMDITypeGetFlags")
		->args({"DType"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1093:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,int64_t,int64_t) , LLVMDIBuilderGetOrCreateSubrange , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderGetOrCreateSubrange","LLVMDIBuilderGetOrCreateSubrange")
		->args({"Builder","LowerBound","Count"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1103:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata **,size_t) , LLVMDIBuilderGetOrCreateArray , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderGetOrCreateArray","LLVMDIBuilderGetOrCreateArray")
		->args({"Builder","Data","NumElements"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1114:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned long long *,size_t) , LLVMDIBuilderCreateExpression , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateExpression","LLVMDIBuilderCreateExpression")
		->args({"Builder","Addr","Length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1124:1
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,uint64_t) , LLVMDIBuilderCreateConstantValueExpression , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateConstantValueExpression","LLVMDIBuilderCreateConstantValueExpression")
		->args({"Builder","Value"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1146:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateGlobalVariableExpression , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateGlobalVariableExpression","LLVMDIBuilderCreateGlobalVariableExpression")
		->args({"Builder","Scope","Name","NameLen","Linkage","LinkLen","File","LineNo","Ty","LocalToUnit","Expr","Decl","AlignInBits"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1158:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIGlobalVariableExpressionGetVariable , SimNode_ExtFuncCall >(lib,"LLVMDIGlobalVariableExpressionGetVariable","LLVMDIGlobalVariableExpressionGetVariable")
		->args({"GVE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1166:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIGlobalVariableExpressionGetExpression , SimNode_ExtFuncCall >(lib,"LLVMDIGlobalVariableExpressionGetExpression","LLVMDIGlobalVariableExpressionGetExpression")
		->args({"GVE"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1175:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIVariableGetFile , SimNode_ExtFuncCall >(lib,"LLVMDIVariableGetFile","LLVMDIVariableGetFile")
		->args({"Var"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1183:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIVariableGetScope , SimNode_ExtFuncCall >(lib,"LLVMDIVariableGetScope","LLVMDIVariableGetScope")
		->args({"Var"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1191:10
	makeExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDIVariableGetLine , SimNode_ExtFuncCall >(lib,"LLVMDIVariableGetLine","LLVMDIVariableGetLine")
		->args({"Var"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1201:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,LLVMOpaqueMetadata **,size_t) , LLVMTemporaryMDNode , SimNode_ExtFuncCall >(lib,"LLVMTemporaryMDNode","LLVMTemporaryMDNode")
		->args({"Ctx","Data","NumElements"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1211:6
	makeExtern< void (*)(LLVMOpaqueMetadata *) , LLVMDisposeTemporaryMDNode , SimNode_ExtFuncCall >(lib,"LLVMDisposeTemporaryMDNode","LLVMDisposeTemporaryMDNode")
		->args({"TempNode"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1218:6
	makeExtern< void (*)(LLVMOpaqueMetadata *,LLVMOpaqueMetadata *) , LLVMMetadataReplaceAllUsesWith , SimNode_ExtFuncCall >(lib,"LLVMMetadataReplaceAllUsesWith","LLVMMetadataReplaceAllUsesWith")
		->args({"TempTargetMetadata","Replacement"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1238:17
	makeExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateTempGlobalVariableFwdDecl , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderCreateTempGlobalVariableFwdDecl","LLVMDIBuilderCreateTempGlobalVariableFwdDecl")
		->args({"Builder","Scope","Name","NameLen","Linkage","LnkLen","File","LineNo","Ty","LocalToUnit","Decl","AlignInBits"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1253:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueValue *) , LLVMDIBuilderInsertDeclareBefore , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderInsertDeclareBefore","LLVMDIBuilderInsertDeclareBefore")
		->args({"Builder","Storage","VarInfo","Expr","DebugLoc","Instr"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1268:14
	makeExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueBasicBlock *) , LLVMDIBuilderInsertDeclareAtEnd , SimNode_ExtFuncCall >(lib,"LLVMDIBuilderInsertDeclareAtEnd","LLVMDIBuilderInsertDeclareAtEnd")
		->args({"Builder","Storage","VarInfo","Expr","DebugLoc","Block"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

