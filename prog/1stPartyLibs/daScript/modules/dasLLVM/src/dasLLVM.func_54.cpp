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
void Module_dasLLVM::initFunctions_54() {
// from D:\Work\libclang\include\llvm-c/lto.h:161:1
	makeExtern< bool (*)(const void *,size_t,const char *) , lto_module_is_object_file_in_memory_for_target , SimNode_ExtFuncCall >(lib,"lto_module_is_object_file_in_memory_for_target","lto_module_is_object_file_in_memory_for_target")
		->args({"mem","length","target_triple_prefix"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:171:1
	makeExtern< LLVMOpaqueLTOModule * (*)(const char *) , lto_module_create , SimNode_ExtFuncCall >(lib,"lto_module_create","lto_module_create")
		->args({"path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:180:1
	makeExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t) , lto_module_create_from_memory , SimNode_ExtFuncCall >(lib,"lto_module_create_from_memory","lto_module_create_from_memory")
		->args({"mem","length"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:189:1
	makeExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t,const char *) , lto_module_create_from_memory_with_path , SimNode_ExtFuncCall >(lib,"lto_module_create_from_memory_with_path","lto_module_create_from_memory_with_path")
		->args({"mem","length","path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:204:1
	makeExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t,const char *) , lto_module_create_in_local_context , SimNode_ExtFuncCall >(lib,"lto_module_create_in_local_context","lto_module_create_in_local_context")
		->args({"mem","length","path"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:218:1
	makeExtern< LLVMOpaqueLTOModule * (*)(const void *,size_t,const char *,LLVMOpaqueLTOCodeGenerator *) , lto_module_create_in_codegen_context , SimNode_ExtFuncCall >(lib,"lto_module_create_in_codegen_context","lto_module_create_in_codegen_context")
		->args({"mem","length","path","cg"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:228:1
	makeExtern< LLVMOpaqueLTOModule * (*)(int,const char *,size_t) , lto_module_create_from_fd , SimNode_ExtFuncCall >(lib,"lto_module_create_from_fd","lto_module_create_from_fd")
		->args({"fd","path","file_size"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:237:1
	makeExtern< LLVMOpaqueLTOModule * (*)(int,const char *,size_t,size_t,off_t) , lto_module_create_from_fd_at_offset , SimNode_ExtFuncCall >(lib,"lto_module_create_from_fd_at_offset","lto_module_create_from_fd_at_offset")
		->args({"fd","path","file_size","map_size","offset"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:247:1
	makeExtern< void (*)(LLVMOpaqueLTOModule *) , lto_module_dispose , SimNode_ExtFuncCall >(lib,"lto_module_dispose","lto_module_dispose")
		->args({"mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:255:1
	makeExtern< const char * (*)(LLVMOpaqueLTOModule *) , lto_module_get_target_triple , SimNode_ExtFuncCall >(lib,"lto_module_get_target_triple","lto_module_get_target_triple")
		->args({"mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:263:1
	makeExtern< void (*)(LLVMOpaqueLTOModule *,const char *) , lto_module_set_target_triple , SimNode_ExtFuncCall >(lib,"lto_module_set_target_triple","lto_module_set_target_triple")
		->args({"mod","triple"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:271:1
	makeExtern< unsigned int (*)(LLVMOpaqueLTOModule *) , lto_module_get_num_symbols , SimNode_ExtFuncCall >(lib,"lto_module_get_num_symbols","lto_module_get_num_symbols")
		->args({"mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:279:1
	makeExtern< const char * (*)(LLVMOpaqueLTOModule *,unsigned int) , lto_module_get_symbol_name , SimNode_ExtFuncCall >(lib,"lto_module_get_symbol_name","lto_module_get_symbol_name")
		->args({"mod","index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:287:1
	makeExtern< lto_symbol_attributes (*)(LLVMOpaqueLTOModule *,unsigned int) , lto_module_get_symbol_attribute , SimNode_ExtFuncCall >(lib,"lto_module_get_symbol_attribute","lto_module_get_symbol_attribute")
		->args({"mod","index"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:298:1
	makeExtern< const char * (*)(LLVMOpaqueLTOModule *) , lto_module_get_linkeropts , SimNode_ExtFuncCall >(lib,"lto_module_get_linkeropts","lto_module_get_linkeropts")
		->args({"mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:311:19
	makeExtern< bool (*)(LLVMOpaqueLTOModule *,unsigned int *,unsigned int *) , lto_module_get_macho_cputype , SimNode_ExtFuncCall >(lib,"lto_module_get_macho_cputype","lto_module_get_macho_cputype")
		->args({"mod","out_cputype","out_cpusubtype"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:324:19
	makeExtern< bool (*)(LLVMOpaqueLTOModule *) , lto_module_has_ctor_dtor , SimNode_ExtFuncCall >(lib,"lto_module_has_ctor_dtor","lto_module_has_ctor_dtor")
		->args({"mod"})
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:370:1
	makeExtern< LLVMOpaqueLTOCodeGenerator * (*)() , lto_codegen_create , SimNode_ExtFuncCall >(lib,"lto_codegen_create","lto_codegen_create")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:382:1
	makeExtern< LLVMOpaqueLTOCodeGenerator * (*)() , lto_codegen_create_in_local_context , SimNode_ExtFuncCall >(lib,"lto_codegen_create_in_local_context","lto_codegen_create_in_local_context")
		->addToModule(*this, SideEffects::worstDefault);
// from D:\Work\libclang\include\llvm-c/lto.h:391:1
	makeExtern< void (*)(LLVMOpaqueLTOCodeGenerator *) , lto_codegen_dispose , SimNode_ExtFuncCall >(lib,"lto_codegen_dispose","lto_codegen_dispose")
		->args({""})
		->addToModule(*this, SideEffects::worstDefault);
}
}

