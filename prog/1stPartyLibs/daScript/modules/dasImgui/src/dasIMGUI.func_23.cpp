// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI.h"
#include "need_dasIMGUI.h"
namespace das {
#include "dasIMGUI.func.aot.decl.inc"
void Module_dasIMGUI::initFunctions_23() {
	using _method_59 = das::das_call_member< void (ImGuiListClipper::*)(int,int),&ImGuiListClipper::IncludeItemsByIndex >;
// from imgui/imgui.h:2767:21
	makeExtern<DAS_CALL_METHOD(_method_59), SimNode_ExtFuncCall , imguiTempFn>(lib,"IncludeItemsByIndex","das_call_member< void (ImGuiListClipper::*)(int,int) , &ImGuiListClipper::IncludeItemsByIndex >::invoke")
		->args({"self","item_begin","item_end"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_60 = das::das_call_member< void (ImGuiListClipper::*)(int),&ImGuiListClipper::SeekCursorForItem >;
// from imgui/imgui.h:2772:21
	makeExtern<DAS_CALL_METHOD(_method_60), SimNode_ExtFuncCall , imguiTempFn>(lib,"SeekCursorForItem","das_call_member< void (ImGuiListClipper::*)(int) , &ImGuiListClipper::SeekCursorForItem >::invoke")
		->args({"self","item_index"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiSelectionBasicStorage>(*this,lib,"ImGuiSelectionBasicStorage","ImGuiSelectionBasicStorage");
	using _method_61 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiMultiSelectIO *),&ImGuiSelectionBasicStorage::ApplyRequests >;
// from imgui/imgui.h:2980:21
	makeExtern<DAS_CALL_METHOD(_method_61), SimNode_ExtFuncCall , imguiTempFn>(lib,"ApplyRequests","das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiMultiSelectIO *) , &ImGuiSelectionBasicStorage::ApplyRequests >::invoke")
		->args({"self","ms_io"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_62 = das::das_call_member< bool (ImGuiSelectionBasicStorage::*)(unsigned int) const,&ImGuiSelectionBasicStorage::Contains >;
// from imgui/imgui.h:2981:21
	makeExtern<DAS_CALL_METHOD(_method_62), SimNode_ExtFuncCall , imguiTempFn>(lib,"Contains","das_call_member< bool (ImGuiSelectionBasicStorage::*)(unsigned int) const , &ImGuiSelectionBasicStorage::Contains >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_63 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(),&ImGuiSelectionBasicStorage::Clear >;
// from imgui/imgui.h:2982:21
	makeExtern<DAS_CALL_METHOD(_method_63), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiSelectionBasicStorage::*)() , &ImGuiSelectionBasicStorage::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_64 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiSelectionBasicStorage &),&ImGuiSelectionBasicStorage::Swap >;
// from imgui/imgui.h:2983:21
	makeExtern<DAS_CALL_METHOD(_method_64), SimNode_ExtFuncCall , imguiTempFn>(lib,"Swap","das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiSelectionBasicStorage &) , &ImGuiSelectionBasicStorage::Swap >::invoke")
		->args({"self","r"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_65 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(unsigned int,bool),&ImGuiSelectionBasicStorage::SetItemSelected >;
// from imgui/imgui.h:2984:21
	makeExtern<DAS_CALL_METHOD(_method_65), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemSelected","das_call_member< void (ImGuiSelectionBasicStorage::*)(unsigned int,bool) , &ImGuiSelectionBasicStorage::SetItemSelected >::invoke")
		->args({"self","id","selected"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_66 = das::das_call_member< bool (ImGuiSelectionBasicStorage::*)(void **,unsigned int *),&ImGuiSelectionBasicStorage::GetNextSelectedItem >;
// from imgui/imgui.h:2985:21
	makeExtern<DAS_CALL_METHOD(_method_66), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetNextSelectedItem","das_call_member< bool (ImGuiSelectionBasicStorage::*)(void **,unsigned int *) , &ImGuiSelectionBasicStorage::GetNextSelectedItem >::invoke")
		->args({"self","opaque_it","out_id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_67 = das::das_call_member< unsigned int (ImGuiSelectionBasicStorage::*)(int),&ImGuiSelectionBasicStorage::GetStorageIdFromIndex >;
// from imgui/imgui.h:2986:21
	makeExtern<DAS_CALL_METHOD(_method_67), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStorageIdFromIndex","das_call_member< unsigned int (ImGuiSelectionBasicStorage::*)(int) , &ImGuiSelectionBasicStorage::GetStorageIdFromIndex >::invoke")
		->args({"self","idx"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiSelectionExternalStorage>(*this,lib,"ImGuiSelectionExternalStorage","ImGuiSelectionExternalStorage");
	using _method_68 = das::das_call_member< void (ImGuiSelectionExternalStorage::*)(ImGuiMultiSelectIO *),&ImGuiSelectionExternalStorage::ApplyRequests >;
// from imgui/imgui.h:2999:21
	makeExtern<DAS_CALL_METHOD(_method_68), SimNode_ExtFuncCall , imguiTempFn>(lib,"ApplyRequests","das_call_member< void (ImGuiSelectionExternalStorage::*)(ImGuiMultiSelectIO *) , &ImGuiSelectionExternalStorage::ApplyRequests >::invoke")
		->args({"self","ms_io"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawCmd>(*this,lib,"ImDrawCmd","ImDrawCmd");
	using _method_69 = das::das_call_member< void * (ImDrawCmd::*)() const,&ImDrawCmd::GetTexID >;
// from imgui/imgui.h:3047:24
	makeExtern<DAS_CALL_METHOD(_method_69), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexID","das_call_member< void * (ImDrawCmd::*)() const , &ImDrawCmd::GetTexID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawListSplitter>(*this,lib,"ImDrawListSplitter","ImDrawListSplitter");
	using _method_70 = das::das_call_member< void (ImDrawListSplitter::*)(),&ImDrawListSplitter::Clear >;
// from imgui/imgui.h:3092:33
	makeExtern<DAS_CALL_METHOD(_method_70), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImDrawListSplitter::*)() , &ImDrawListSplitter::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_71 = das::das_call_member< void (ImDrawListSplitter::*)(),&ImDrawListSplitter::ClearFreeMemory >;
// from imgui/imgui.h:3093:33
	makeExtern<DAS_CALL_METHOD(_method_71), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearFreeMemory","das_call_member< void (ImDrawListSplitter::*)() , &ImDrawListSplitter::ClearFreeMemory >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_72 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int),&ImDrawListSplitter::Split >;
// from imgui/imgui.h:3094:33
	makeExtern<DAS_CALL_METHOD(_method_72), SimNode_ExtFuncCall , imguiTempFn>(lib,"Split","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int) , &ImDrawListSplitter::Split >::invoke")
		->args({"self","draw_list","count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_73 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *),&ImDrawListSplitter::Merge >;
// from imgui/imgui.h:3095:33
	makeExtern<DAS_CALL_METHOD(_method_73), SimNode_ExtFuncCall , imguiTempFn>(lib,"Merge","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *) , &ImDrawListSplitter::Merge >::invoke")
		->args({"self","draw_list"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_74 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int),&ImDrawListSplitter::SetCurrentChannel >;
// from imgui/imgui.h:3096:33
	makeExtern<DAS_CALL_METHOD(_method_74), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCurrentChannel","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int) , &ImDrawListSplitter::SetCurrentChannel >::invoke")
		->args({"self","draw_list","channel_idx"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

