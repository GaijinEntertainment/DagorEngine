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
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:896:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateTypedef >(*this,lib,"LLVMDIBuilderCreateTypedef",SideEffects::worstDefault,"LLVMDIBuilderCreateTypedef")
		->args({"Builder","Type","Name","NameLen","File","LineNo","Scope","AlignInBits"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:912:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,uint64_t,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateInheritance >(*this,lib,"LLVMDIBuilderCreateInheritance",SideEffects::worstDefault,"LLVMDIBuilderCreateInheritance")
		->args({"Builder","Ty","BaseTy","BaseOffset","VBPtrOffset","Flags"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:933:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,const char *,size_t,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,unsigned int,uint64_t,unsigned int,const char *,size_t) , LLVMDIBuilderCreateForwardDecl >(*this,lib,"LLVMDIBuilderCreateForwardDecl",SideEffects::worstDefault,"LLVMDIBuilderCreateForwardDecl")
		->args({"Builder","Tag","Name","NameLen","Scope","File","Line","RuntimeLang","SizeInBits","AlignInBits","UniqueIdentifier","UniqueIdentifierLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:957:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,const char *,size_t,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int,unsigned int,uint64_t,unsigned int,LLVMDIFlags,const char *,size_t) , LLVMDIBuilderCreateReplaceableCompositeType >(*this,lib,"LLVMDIBuilderCreateReplaceableCompositeType",SideEffects::worstDefault,"LLVMDIBuilderCreateReplaceableCompositeType")
		->args({"Builder","Tag","Name","NameLen","Scope","File","Line","RuntimeLang","SizeInBits","AlignInBits","Flags","UniqueIdentifier","UniqueIdentifierLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:979:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,uint64_t,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateBitFieldMemberType >(*this,lib,"LLVMDIBuilderCreateBitFieldMemberType",SideEffects::worstDefault,"LLVMDIBuilderCreateBitFieldMemberType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","OffsetInBits","StorageOffsetInBits","Flags","Type"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1010:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,const char *,size_t) , LLVMDIBuilderCreateClassType >(*this,lib,"LLVMDIBuilderCreateClassType",SideEffects::worstDefault,"LLVMDIBuilderCreateClassType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","OffsetInBits","Flags","DerivedFrom","Elements","NumElements","VTableHolder","TemplateParamsNode","UniqueIdentifier","UniqueIdentifierLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1025:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateArtificialType >(*this,lib,"LLVMDIBuilderCreateArtificialType",SideEffects::worstDefault,"LLVMDIBuilderCreateArtificialType")
		->args({"Builder","Type"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1035:13
	addExtern< const char * (*)(LLVMOpaqueMetadata *,size_t *) , LLVMDITypeGetName >(*this,lib,"LLVMDITypeGetName",SideEffects::worstDefault,"LLVMDITypeGetName")
		->args({"DType","Length"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1043:10
	addExtern< uint64_t (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetSizeInBits >(*this,lib,"LLVMDITypeGetSizeInBits",SideEffects::worstDefault,"LLVMDITypeGetSizeInBits")
		->args({"DType"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1051:10
	addExtern< uint64_t (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetOffsetInBits >(*this,lib,"LLVMDITypeGetOffsetInBits",SideEffects::worstDefault,"LLVMDITypeGetOffsetInBits")
		->args({"DType"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1059:10
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetAlignInBits >(*this,lib,"LLVMDITypeGetAlignInBits",SideEffects::worstDefault,"LLVMDITypeGetAlignInBits")
		->args({"DType"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1067:10
	addExtern< unsigned int (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetLine >(*this,lib,"LLVMDITypeGetLine",SideEffects::worstDefault,"LLVMDITypeGetLine")
		->args({"DType"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1075:13
	addExtern< LLVMDIFlags (*)(LLVMOpaqueMetadata *) , LLVMDITypeGetFlags >(*this,lib,"LLVMDITypeGetFlags",SideEffects::worstDefault,"LLVMDITypeGetFlags")
		->args({"DType"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1083:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,int64_t,int64_t) , LLVMDIBuilderGetOrCreateSubrange >(*this,lib,"LLVMDIBuilderGetOrCreateSubrange",SideEffects::worstDefault,"LLVMDIBuilderGetOrCreateSubrange")
		->args({"Builder","LowerBound","Count"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1093:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata **,size_t) , LLVMDIBuilderGetOrCreateArray >(*this,lib,"LLVMDIBuilderGetOrCreateArray",SideEffects::worstDefault,"LLVMDIBuilderGetOrCreateArray")
		->args({"Builder","Data","NumElements"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1104:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned long long *,size_t) , LLVMDIBuilderCreateExpression >(*this,lib,"LLVMDIBuilderCreateExpression",SideEffects::worstDefault,"LLVMDIBuilderCreateExpression")
		->args({"Builder","Addr","Length"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1114:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,uint64_t) , LLVMDIBuilderCreateConstantValueExpression >(*this,lib,"LLVMDIBuilderCreateConstantValueExpression",SideEffects::worstDefault,"LLVMDIBuilderCreateConstantValueExpression")
		->args({"Builder","Value"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1136:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,unsigned int) , LLVMDIBuilderCreateGlobalVariableExpression >(*this,lib,"LLVMDIBuilderCreateGlobalVariableExpression",SideEffects::worstDefault,"LLVMDIBuilderCreateGlobalVariableExpression")
		->args({"Builder","Scope","Name","NameLen","Linkage","LinkLen","File","LineNo","Ty","LocalToUnit","Expr","Decl","AlignInBits"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1148:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIGlobalVariableExpressionGetVariable >(*this,lib,"LLVMDIGlobalVariableExpressionGetVariable",SideEffects::worstDefault,"LLVMDIGlobalVariableExpressionGetVariable")
		->args({"GVE"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:1156:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueMetadata *) , LLVMDIGlobalVariableExpressionGetExpression >(*this,lib,"LLVMDIGlobalVariableExpressionGetExpression",SideEffects::worstDefault,"LLVMDIGlobalVariableExpressionGetExpression")
		->args({"GVE"});
}
}

