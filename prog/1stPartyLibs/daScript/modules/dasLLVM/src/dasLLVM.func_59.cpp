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
void Module_dasLLVM::initFunctions_59() {
// from D:\Work\libclang\include\llvm-c/lto.h:565:1
	addExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *) , lto_codegen_debug_options >(*this,lib,"lto_codegen_debug_options",SideEffects::worstDefault,"lto_codegen_debug_options")
		->args({"cg",""});
// from D:\Work\libclang\include\llvm-c/lto.h:573:13
	addExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,const char *const *,int) , lto_codegen_debug_options_array >(*this,lib,"lto_codegen_debug_options_array",SideEffects::worstDefault,"lto_codegen_debug_options_array")
		->args({"cg","","number"});
// from D:\Work\libclang\include\llvm-c/lto.h:583:1
	addExtern< void (*)() , lto_initialize_disassembler >(*this,lib,"lto_initialize_disassembler",SideEffects::worstDefault,"lto_initialize_disassembler");
// from D:\Work\libclang\include\llvm-c/lto.h:592:1
	addExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,bool) , lto_codegen_set_should_internalize >(*this,lib,"lto_codegen_set_should_internalize",SideEffects::worstDefault,"lto_codegen_set_should_internalize")
		->args({"cg","ShouldInternalize"});
// from D:\Work\libclang\include\llvm-c/lto.h:604:1
	addExtern< void (*)(LLVMOpaqueLTOCodeGenerator *,bool) , lto_codegen_set_should_embed_uselists >(*this,lib,"lto_codegen_set_should_embed_uselists",SideEffects::worstDefault,"lto_codegen_set_should_embed_uselists")
		->args({"cg","ShouldEmbedUselists"});
// from D:\Work\libclang\include\llvm-c/lto.h:618:20
	addExtern< LLVMOpaqueLTOInput * (*)(const void *,size_t,const char *) , lto_input_create >(*this,lib,"lto_input_create",SideEffects::worstDefault,"lto_input_create")
		->args({"buffer","buffer_size","path"});
// from D:\Work\libclang\include\llvm-c/lto.h:628:13
	addExtern< void (*)(LLVMOpaqueLTOInput *) , lto_input_dispose >(*this,lib,"lto_input_dispose",SideEffects::worstDefault,"lto_input_dispose")
		->args({"input"});
// from D:\Work\libclang\include\llvm-c/lto.h:636:17
	addExtern< unsigned int (*)(LLVMOpaqueLTOInput *) , lto_input_get_num_dependent_libraries >(*this,lib,"lto_input_get_num_dependent_libraries",SideEffects::worstDefault,"lto_input_get_num_dependent_libraries")
		->args({"input"});
// from D:\Work\libclang\include\llvm-c/lto.h:645:21
	addExtern< const char * (*)(LLVMOpaqueLTOInput *,size_t,size_t *) , lto_input_get_dependent_library >(*this,lib,"lto_input_get_dependent_library",SideEffects::worstDefault,"lto_input_get_dependent_library")
		->args({"input","index","size"});
// from D:\Work\libclang\include\llvm-c/lto.h:655:27
	addExtern< const char *const * (*)(size_t *) , lto_runtime_lib_symbols_list >(*this,lib,"lto_runtime_lib_symbols_list",SideEffects::worstDefault,"lto_runtime_lib_symbols_list")
		->args({"size"});
// from D:\Work\libclang\include\llvm-c/lto.h:687:27
	addExtern< LLVMOpaqueThinLTOCodeGenerator * (*)() , thinlto_create_codegen >(*this,lib,"thinlto_create_codegen",SideEffects::worstDefault,"thinlto_create_codegen");
// from D:\Work\libclang\include\llvm-c/lto.h:695:13
	addExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_codegen_dispose >(*this,lib,"thinlto_codegen_dispose",SideEffects::worstDefault,"thinlto_codegen_dispose")
		->args({"cg"});
// from D:\Work\libclang\include\llvm-c/lto.h:708:13
	addExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *,const char *,int) , thinlto_codegen_add_module >(*this,lib,"thinlto_codegen_add_module",SideEffects::worstDefault,"thinlto_codegen_add_module")
		->args({"cg","identifier","data","length"});
// from D:\Work\libclang\include\llvm-c/lto.h:718:13
	addExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_codegen_process >(*this,lib,"thinlto_codegen_process",SideEffects::worstDefault,"thinlto_codegen_process")
		->args({"cg"});
// from D:\Work\libclang\include\llvm-c/lto.h:729:21
	addExtern< unsigned int (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_module_get_num_objects >(*this,lib,"thinlto_module_get_num_objects",SideEffects::worstDefault,"thinlto_module_get_num_objects")
		->args({"cg"});
// from D:\Work\libclang\include\llvm-c/lto.h:740:24
	addExtern< LTOObjectBuffer (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_module_get_object ,SimNode_ExtFuncCallAndCopyOrMove>(*this,lib,"thinlto_module_get_object",SideEffects::worstDefault,"thinlto_module_get_object")
		->args({"cg","index"});
// from D:\Work\libclang\include\llvm-c/lto.h:752:14
	addExtern< unsigned int (*)(LLVMOpaqueThinLTOCodeGenerator *) , thinlto_module_get_num_object_files >(*this,lib,"thinlto_module_get_num_object_files",SideEffects::worstDefault,"thinlto_module_get_num_object_files")
		->args({"cg"});
// from D:\Work\libclang\include\llvm-c/lto.h:763:13
	addExtern< const char * (*)(LLVMOpaqueThinLTOCodeGenerator *,unsigned int) , thinlto_module_get_object_file >(*this,lib,"thinlto_module_get_object_file",SideEffects::worstDefault,"thinlto_module_get_object_file")
		->args({"cg","index"});
// from D:\Work\libclang\include\llvm-c/lto.h:772:19
	addExtern< bool (*)(LLVMOpaqueThinLTOCodeGenerator *,lto_codegen_model) , thinlto_codegen_set_pic_model >(*this,lib,"thinlto_codegen_set_pic_model",SideEffects::worstDefault,"thinlto_codegen_set_pic_model")
		->args({"cg",""});
// from D:\Work\libclang\include\llvm-c/lto.h:782:13
	addExtern< void (*)(LLVMOpaqueThinLTOCodeGenerator *,const char *) , thinlto_codegen_set_savetemps_dir >(*this,lib,"thinlto_codegen_set_savetemps_dir",SideEffects::worstDefault,"thinlto_codegen_set_savetemps_dir")
		->args({"cg","save_temps_dir"});
}
}

