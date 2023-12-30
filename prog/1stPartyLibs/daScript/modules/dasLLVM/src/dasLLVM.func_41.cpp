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
void Module_dasLLVM::initFunctions_41() {
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1165:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIVariableGetFile >(*this,lib,"LLVMDIVariableGetFile",SideEffects::worstDefault,"LLVMDIVariableGetFile")
		->args({"Var"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1173:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIVariableGetScope >(*this,lib,"LLVMDIVariableGetScope",SideEffects::worstDefault,"LLVMDIVariableGetScope")
		->args({"Var"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1181:10
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDIVariableGetLine >(*this,lib,"LLVMDIVariableGetLine",SideEffects::worstDefault,"LLVMDIVariableGetLine")
		->args({"Var"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1191:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueContext *,LLVMOpaqueMetadata **,size_t) , LLVMTemporaryMDNode >(*this,lib,"LLVMTemporaryMDNode",SideEffects::worstDefault,"LLVMTemporaryMDNode")
		->args({"Ctx","Data","NumElements"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1201:6
	addExtern< void (*)(LLVMOpaqueMetadata *) , LLVMDisposeTemporaryMDNode >(*this,lib,"LLVMDisposeTemporaryMDNode",SideEffects::worstDefault,"LLVMDisposeTemporaryMDNode")
		->args({"TempNode"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1208:6
	addExtern< void (*)(LLVMOpaqueMetadata *,LLVMOpaqueMetadata *) , LLVMMetadataReplaceAllUsesWith >(*this,lib,"LLVMMetadataReplaceAllUsesWith",SideEffects::worstDefault,"LLVMMetadataReplaceAllUsesWith")
		->args({"TempTargetMetadata","Replacement"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1228:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateTempGlobalVariableFwdDecl >(*this,lib,"LLVMDIBuilderCreateTempGlobalVariableFwdDecl",SideEffects::worstDefault,"LLVMDIBuilderCreateTempGlobalVariableFwdDecl")
		->args({"Builder","Scope","Name","NameLen","Linkage","LnkLen","File","LineNo","Ty","LocalToUnit","Decl","AlignInBits"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1243:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueValue *) , LLVMDIBuilderInsertDeclareBefore >(*this,lib,"LLVMDIBuilderInsertDeclareBefore",SideEffects::worstDefault,"LLVMDIBuilderInsertDeclareBefore")
		->args({"Builder","Storage","VarInfo","Expr","DebugLoc","Instr"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1258:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueBasicBlock *) , LLVMDIBuilderInsertDeclareAtEnd >(*this,lib,"LLVMDIBuilderInsertDeclareAtEnd",SideEffects::worstDefault,"LLVMDIBuilderInsertDeclareAtEnd")
		->args({"Builder","Storage","VarInfo","Expr","DebugLoc","Block"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1271:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueValue *) , LLVMDIBuilderInsertDbgValueBefore >(*this,lib,"LLVMDIBuilderInsertDbgValueBefore",SideEffects::worstDefault,"LLVMDIBuilderInsertDbgValueBefore")
		->args({"Builder","Val","VarInfo","Expr","DebugLoc","Instr"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1289:14
	addExtern< LLVMOpaqueValue * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueValue *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,LLVMOpaqueBasicBlock *) , LLVMDIBuilderInsertDbgValueAtEnd >(*this,lib,"LLVMDIBuilderInsertDbgValueAtEnd",SideEffects::worstDefault,"LLVMDIBuilderInsertDbgValueAtEnd")
		->args({"Builder","Val","VarInfo","Expr","DebugLoc","Block"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1309:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMDIFlags,unsigned int) , LLVMDIBuilderCreateAutoVariable >(*this,lib,"LLVMDIBuilderCreateAutoVariable",SideEffects::worstDefault,"LLVMDIBuilderCreateAutoVariable")
		->args({"Builder","Scope","Name","NameLen","File","LineNo","Ty","AlwaysPreserve","Flags","AlignInBits"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1327:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,unsigned int,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMDIFlags) , LLVMDIBuilderCreateParameterVariable >(*this,lib,"LLVMDIBuilderCreateParameterVariable",SideEffects::worstDefault,"LLVMDIBuilderCreateParameterVariable")
		->args({"Builder","Scope","Name","NameLen","ArgNo","File","LineNo","Ty","AlwaysPreserve","Flags"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1337:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValue *) , LLVMGetSubprogram >(*this,lib,"LLVMGetSubprogram",SideEffects::worstDefault,"LLVMGetSubprogram")
		->args({"Func"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1344:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueMetadata *) , LLVMSetSubprogram >(*this,lib,"LLVMSetSubprogram",SideEffects::worstDefault,"LLVMSetSubprogram")
		->args({"Func","SP"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1352:10
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDISubprogramGetLine >(*this,lib,"LLVMDISubprogramGetLine",SideEffects::worstDefault,"LLVMDISubprogramGetLine")
		->args({"Subprogram"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1359:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueValue *) , LLVMInstructionGetDebugLoc >(*this,lib,"LLVMInstructionGetDebugLoc",SideEffects::worstDefault,"LLVMInstructionGetDebugLoc")
		->args({"Inst"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1368:6
	addExtern< void (*)(LLVMOpaqueValue *,LLVMOpaqueMetadata *) , LLVMInstructionSetDebugLoc >(*this,lib,"LLVMInstructionSetDebugLoc",SideEffects::worstDefault,"LLVMInstructionSetDebugLoc")
		->args({"Inst","Loc"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1375:18
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMGetMetadataKind >(*this,lib,"LLVMGetMetadataKind",SideEffects::worstDefault,"LLVMGetMetadataKind")
		->args({"Metadata"});
// from D:\Work\libclang\include\llvm-c/Disassembler.h:72:5
	addExtern< int (*)(void *,uint64_t) , LLVMSetDisasmOptions >(*this,lib,"LLVMSetDisasmOptions",SideEffects::worstDefault,"LLVMSetDisasmOptions")
		->args({"DC","Options"});
}
}

