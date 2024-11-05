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
void Module_dasLLVM::initFunctions_57() {
// from D:\Work\libclang\include\llvm-c/lto.h:832:19
	makeExtern< bool (*)(LLVMOpaqueLTOModule *) , lto_module_is_thinlto , SimNode_ExtFuncCall >(lib,"lto_module_is_thinlto","lto_module_is_thinlto")
		->args({"mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:842:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *,int) , thinlto_codegen_add_must_preserve_symbol , SimNode_ExtFuncCall >(lib,"thinlto_codegen_add_must_preserve_symbol","thinlto_codegen_add_must_preserve_symbol")
		->args({"cg","name","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:854:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *,int) , thinlto_codegen_add_cross_referenced_symbol , SimNode_ExtFuncCall >(lib,"thinlto_codegen_add_cross_referenced_symbol","thinlto_codegen_add_cross_referenced_symbol")
		->args({"cg","name","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:885:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *) , thinlto_codegen_set_cache_dir , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cache_dir","thinlto_codegen_set_cache_dir")
		->args({"cg","cache_dir"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:895:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,int) , thinlto_codegen_set_cache_pruning_interval , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cache_pruning_interval","thinlto_codegen_set_cache_pruning_interval")
		->args({"cg","interval"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:911:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_codegen_set_final_cache_size_relative_to_available_space , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_final_cache_size_relative_to_available_space","thinlto_codegen_set_final_cache_size_relative_to_available_space")
		->args({"cg","percentage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:920:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_codegen_set_cache_entry_expiration , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cache_entry_expiration","thinlto_codegen_set_cache_entry_expiration")
		->args({"cg","expiration"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:931:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_codegen_set_cache_size_bytes , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cache_size_bytes","thinlto_codegen_set_cache_size_bytes")
		->args({"cg","max_size_bytes"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:941:1
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_codegen_set_cache_size_megabytes , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cache_size_megabytes","thinlto_codegen_set_cache_size_megabytes")
		->args({"cg","max_size_megabytes"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:950:13
	makeExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_codegen_set_cache_size_files , SimNode_ExtFuncCall >(lib,"thinlto_codegen_set_cache_size_files","thinlto_codegen_set_cache_size_files")
		->args({"cg","max_size_files"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:76:15
	makeExtern< LLVMOpaqueBinary * (*)(LLVMOpaqueMemoryBuffer *,LLVMOpaqueContext *,char **) , LLVMCreateBinary , SimNode_ExtFuncCall >(lib,"LLVMCreateBinary","LLVMCreateBinary")
		->args({"MemBuf","Context","ErrorMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:86:6
	makeExtern< void (*)(LLVMOpaqueBinary *) , LLVMDisposeBinary , SimNode_ExtFuncCall >(lib,"LLVMDisposeBinary","LLVMDisposeBinary")
		->args({"BR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:97:21
	makeExtern< LLVMOpaqueMemoryBuffer * (*)(LLVMOpaqueBinary *) , LLVMBinaryCopyMemoryBuffer , SimNode_ExtFuncCall >(lib,"LLVMBinaryCopyMemoryBuffer","LLVMBinaryCopyMemoryBuffer")
		->args({"BR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:104:16
	makeExtern< LLVMBinaryType (*)(LLVMOpaqueBinary *) , LLVMBinaryGetType , SimNode_ExtFuncCall >(lib,"LLVMBinaryGetType","LLVMBinaryGetType")
		->args({"BR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:117:15
	makeExtern< LLVMOpaqueBinary * (*)(LLVMOpaqueBinary *,const char *,size_t,char **) , LLVMMachOUniversalBinaryCopyObjectForArch , SimNode_ExtFuncCall >(lib,"LLVMMachOUniversalBinaryCopyObjectForArch","LLVMMachOUniversalBinaryCopyObjectForArch")
		->args({"BR","Arch","ArchLen","ErrorMessage"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:133:24
	makeExtern< LLVMOpaqueSectionIterator * (*)(LLVMOpaqueBinary *) , LLVMObjectFileCopySectionIterator , SimNode_ExtFuncCall >(lib,"LLVMObjectFileCopySectionIterator","LLVMObjectFileCopySectionIterator")
		->args({"BR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:140:10
	makeExtern< int (*)(LLVMOpaqueBinary *,LLVMOpaqueSectionIterator *) , LLVMObjectFileIsSectionIteratorAtEnd , SimNode_ExtFuncCall >(lib,"LLVMObjectFileIsSectionIteratorAtEnd","LLVMObjectFileIsSectionIteratorAtEnd")
		->args({"BR","SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:154:23
	makeExtern< LLVMOpaqueSymbolIterator * (*)(LLVMOpaqueBinary *) , LLVMObjectFileCopySymbolIterator , SimNode_ExtFuncCall >(lib,"LLVMObjectFileCopySymbolIterator","LLVMObjectFileCopySymbolIterator")
		->args({"BR"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:161:10
	makeExtern< int (*)(LLVMOpaqueBinary *,LLVMOpaqueSymbolIterator *) , LLVMObjectFileIsSymbolIteratorAtEnd , SimNode_ExtFuncCall >(lib,"LLVMObjectFileIsSymbolIteratorAtEnd","LLVMObjectFileIsSymbolIteratorAtEnd")
		->args({"BR","SI"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/Object.h:164:6
	makeExtern< void (*)(LLVMOpaqueSectionIterator *) , LLVMDisposeSectionIterator , SimNode_ExtFuncCall >(lib,"LLVMDisposeSectionIterator","LLVMDisposeSectionIterator")
		->args({"SI"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

