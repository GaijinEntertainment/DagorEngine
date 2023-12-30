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
void Module_dasLLVM::initFunctions_39() {
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:569:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,unsigned int,LLVMDWARFMacinfoRecordType,const char *,size_t,const char *,size_t) , LLVMDIBuilderCreateMacro >(*this,lib,"LLVMDIBuilderCreateMacro",SideEffects::worstDefault,"LLVMDIBuilderCreateMacro")
		->args({"Builder","ParentMacroFile","Line","RecordType","Name","NameLen","Value","ValueLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:586:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateTempMacroFile >(*this,lib,"LLVMDIBuilderCreateTempMacroFile",SideEffects::worstDefault,"LLVMDIBuilderCreateTempMacroFile")
		->args({"Builder","ParentMacroFile","Line","File"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:598:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,int64_t,int) , LLVMDIBuilderCreateEnumerator >(*this,lib,"LLVMDIBuilderCreateEnumerator",SideEffects::worstDefault,"LLVMDIBuilderCreateEnumerator")
		->args({"Builder","Name","NameLen","Value","IsUnsigned"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:617:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,LLVMOpaqueMetadata **,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateEnumerationType >(*this,lib,"LLVMDIBuilderCreateEnumerationType",SideEffects::worstDefault,"LLVMDIBuilderCreateEnumerationType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","Elements","NumElements","ClassTy"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:640:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,LLVMDIFlags,LLVMOpaqueMetadata **,unsigned int,unsigned int,const char *,size_t) , LLVMDIBuilderCreateUnionType >(*this,lib,"LLVMDIBuilderCreateUnionType",SideEffects::worstDefault,"LLVMDIBuilderCreateUnionType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","Flags","Elements","NumElements","RunTimeLang","UniqueId","UniqueIdLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:658:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,uint64_t,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateArrayType >(*this,lib,"LLVMDIBuilderCreateArrayType",SideEffects::worstDefault,"LLVMDIBuilderCreateArrayType")
		->args({"Builder","Size","AlignInBits","Ty","Subscripts","NumSubscripts"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:673:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,uint64_t,unsigned int,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int) , LLVMDIBuilderCreateVectorType >(*this,lib,"LLVMDIBuilderCreateVectorType",SideEffects::worstDefault,"LLVMDIBuilderCreateVectorType")
		->args({"Builder","Size","AlignInBits","Ty","Subscripts","NumSubscripts"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:685:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t) , LLVMDIBuilderCreateUnspecifiedType >(*this,lib,"LLVMDIBuilderCreateUnspecifiedType",SideEffects::worstDefault,"LLVMDIBuilderCreateUnspecifiedType")
		->args({"Builder","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:699:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,uint64_t,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateBasicType >(*this,lib,"LLVMDIBuilderCreateBasicType",SideEffects::worstDefault,"LLVMDIBuilderCreateBasicType")
		->args({"Builder","Name","NameLen","SizeInBits","Encoding","Flags"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:714:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,uint64_t,unsigned int,unsigned int,const char *,size_t) , LLVMDIBuilderCreatePointerType >(*this,lib,"LLVMDIBuilderCreatePointerType",SideEffects::worstDefault,"LLVMDIBuilderCreatePointerType")
		->args({"Builder","PointeeTy","SizeInBits","AlignInBits","AddressSpace","Name","NameLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:737:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,LLVMDIFlags,LLVMOpaqueMetadata *,LLVMOpaqueMetadata **,unsigned int,unsigned int,LLVMOpaqueMetadata *,const char *,size_t) , LLVMDIBuilderCreateStructType >(*this,lib,"LLVMDIBuilderCreateStructType",SideEffects::worstDefault,"LLVMDIBuilderCreateStructType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","SizeInBits","AlignInBits","Flags","DerivedFrom","Elements","NumElements","RunTimeLang","VTableHolder","UniqueId","UniqueIdLen"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:759:17
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateMemberType >(*this,lib,"LLVMDIBuilderCreateMemberType",SideEffects::worstDefault,"LLVMDIBuilderCreateMemberType")
		->args({"Builder","Scope","Name","NameLen","File","LineNo","SizeInBits","AlignInBits","OffsetInBits","Flags","Ty"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:780:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,LLVMOpaqueMetadata *,LLVMDIFlags,LLVMOpaqueValue *,unsigned int) , LLVMDIBuilderCreateStaticMemberType >(*this,lib,"LLVMDIBuilderCreateStaticMemberType",SideEffects::worstDefault,"LLVMDIBuilderCreateStaticMemberType")
		->args({"Builder","Scope","Name","NameLen","File","LineNumber","Type","Flags","ConstantVal","AlignInBits"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:796:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *,uint64_t,unsigned int,LLVMDIFlags) , LLVMDIBuilderCreateMemberPointerType >(*this,lib,"LLVMDIBuilderCreateMemberPointerType",SideEffects::worstDefault,"LLVMDIBuilderCreateMemberPointerType")
		->args({"Builder","PointeeType","ClassType","SizeInBits","AlignInBits","Flags"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:817:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,uint64_t,unsigned int,uint64_t,LLVMDIFlags,LLVMOpaqueMetadata *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateObjCIVar >(*this,lib,"LLVMDIBuilderCreateObjCIVar",SideEffects::worstDefault,"LLVMDIBuilderCreateObjCIVar")
		->args({"Builder","Name","NameLen","File","LineNo","SizeInBits","AlignInBits","OffsetInBits","Flags","Ty","PropertyNode"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:839:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,const char *,size_t,LLVMOpaqueMetadata *,unsigned int,const char *,size_t,const char *,size_t,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateObjCProperty >(*this,lib,"LLVMDIBuilderCreateObjCProperty",SideEffects::worstDefault,"LLVMDIBuilderCreateObjCProperty")
		->args({"Builder","Name","NameLen","File","LineNo","GetterName","GetterNameLen","SetterName","SetterNameLen","PropertyAttributes","Ty"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:853:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateObjectPointerType >(*this,lib,"LLVMDIBuilderCreateObjectPointerType",SideEffects::worstDefault,"LLVMDIBuilderCreateObjectPointerType")
		->args({"Builder","Type"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:865:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateQualifiedType >(*this,lib,"LLVMDIBuilderCreateQualifiedType",SideEffects::worstDefault,"LLVMDIBuilderCreateQualifiedType")
		->args({"Builder","Tag","Type"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:876:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *,unsigned int,LLVMOpaqueMetadata *) , LLVMDIBuilderCreateReferenceType >(*this,lib,"LLVMDIBuilderCreateReferenceType",SideEffects::worstDefault,"LLVMDIBuilderCreateReferenceType")
		->args({"Builder","Tag","Type"});
// from D:\Work\libclang\include\llvm-c/DebugInfo.h:884:1
	addExtern< LLVMOpaqueMetadata * (*)(LLVMOpaqueDIBuilder *) , LLVMDIBuilderCreateNullPtrType >(*this,lib,"LLVMDIBuilderCreateNullPtrType",SideEffects::worstDefault,"LLVMDIBuilderCreateNullPtrType")
		->args({"Builder"});
}
}

