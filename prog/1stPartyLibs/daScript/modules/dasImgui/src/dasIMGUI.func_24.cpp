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
void Module_dasIMGUI::initFunctions_24() {
	using _method_70 = das::das_call_member< unsigned int (ImGuiSelectionBasicStorage::*)(int),&ImGuiSelectionBasicStorage::GetStorageIdFromIndex >;
// from imgui/imgui.h:3239:21
	makeExtern<DAS_CALL_METHOD(_method_70), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStorageIdFromIndex","das_call_member< unsigned int (ImGuiSelectionBasicStorage::*)(int) , &ImGuiSelectionBasicStorage::GetStorageIdFromIndex >::invoke")
		->args({"self","idx"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiSelectionExternalStorage>(*this,lib,"ImGuiSelectionExternalStorage","ImGuiSelectionExternalStorage");
	using _method_71 = das::das_call_member< void (ImGuiSelectionExternalStorage::*)(ImGuiMultiSelectIO *),&ImGuiSelectionExternalStorage::ApplyRequests >;
// from imgui/imgui.h:3252:21
	makeExtern<DAS_CALL_METHOD(_method_71), SimNode_ExtFuncCall , imguiTempFn>(lib,"ApplyRequests","das_call_member< void (ImGuiSelectionExternalStorage::*)(ImGuiMultiSelectIO *) , &ImGuiSelectionExternalStorage::ApplyRequests >::invoke")
		->args({"self","ms_io"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawCmd>(*this,lib,"ImDrawCmd","ImDrawCmd");
	using _method_72 = das::das_call_member< void * (ImDrawCmd::*)() const,&ImDrawCmd::GetTexID >;
// from imgui/imgui.h:3310:24
	makeExtern<DAS_CALL_METHOD(_method_72), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexID","das_call_member< void * (ImDrawCmd::*)() const , &ImDrawCmd::GetTexID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawListSplitter>(*this,lib,"ImDrawListSplitter","ImDrawListSplitter");
	using _method_73 = das::das_call_member< void (ImDrawListSplitter::*)(),&ImDrawListSplitter::Clear >;
// from imgui/imgui.h:3354:33
	makeExtern<DAS_CALL_METHOD(_method_73), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImDrawListSplitter::*)() , &ImDrawListSplitter::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_74 = das::das_call_member< void (ImDrawListSplitter::*)(),&ImDrawListSplitter::ClearFreeMemory >;
// from imgui/imgui.h:3355:33
	makeExtern<DAS_CALL_METHOD(_method_74), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearFreeMemory","das_call_member< void (ImDrawListSplitter::*)() , &ImDrawListSplitter::ClearFreeMemory >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_75 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int),&ImDrawListSplitter::Split >;
// from imgui/imgui.h:3356:33
	makeExtern<DAS_CALL_METHOD(_method_75), SimNode_ExtFuncCall , imguiTempFn>(lib,"Split","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int) , &ImDrawListSplitter::Split >::invoke")
		->args({"self","draw_list","count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_76 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *),&ImDrawListSplitter::Merge >;
// from imgui/imgui.h:3357:33
	makeExtern<DAS_CALL_METHOD(_method_76), SimNode_ExtFuncCall , imguiTempFn>(lib,"Merge","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *) , &ImDrawListSplitter::Merge >::invoke")
		->args({"self","draw_list"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_77 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int),&ImDrawListSplitter::SetCurrentChannel >;
// from imgui/imgui.h:3358:33
	makeExtern<DAS_CALL_METHOD(_method_77), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCurrentChannel","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int) , &ImDrawListSplitter::SetCurrentChannel >::invoke")
		->args({"self","draw_list","channel_idx"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawList,ImDrawListSharedData *>(*this,lib,"ImDrawList","ImDrawList")
		->args({"shared_data"});
	using _method_78 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,bool),&ImDrawList::PushClipRect >;
// from imgui/imgui.h:3428:21
	makeExtern<DAS_CALL_METHOD(_method_78), SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,bool) , &ImDrawList::PushClipRect >::invoke")
		->args({"self","clip_rect_min","clip_rect_max","intersect_with_current_clip_rect"})
		->arg_init(3,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_79 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PushClipRectFullScreen >;
// from imgui/imgui.h:3429:21
	makeExtern<DAS_CALL_METHOD(_method_79), SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRectFullScreen","das_call_member< void (ImDrawList::*)() , &ImDrawList::PushClipRectFullScreen >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_80 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PopClipRect >;
// from imgui/imgui.h:3430:21
	makeExtern<DAS_CALL_METHOD(_method_80), SimNode_ExtFuncCall , imguiTempFn>(lib,"PopClipRect","das_call_member< void (ImDrawList::*)() , &ImDrawList::PopClipRect >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_81 = das::das_call_member< void (ImDrawList::*)(ImTextureRef),&ImDrawList::PushTexture >;
// from imgui/imgui.h:3431:21
	makeExtern<DAS_CALL_METHOD(_method_81), SimNode_ExtFuncCall , imguiTempFn>(lib,"PushTexture","das_call_member< void (ImDrawList::*)(ImTextureRef) , &ImDrawList::PushTexture >::invoke")
		->args({"self","tex_ref"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_82 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PopTexture >;
// from imgui/imgui.h:3432:21
	makeExtern<DAS_CALL_METHOD(_method_82), SimNode_ExtFuncCall , imguiTempFn>(lib,"PopTexture","das_call_member< void (ImDrawList::*)() , &ImDrawList::PopTexture >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_83 = das::das_call_member< ImVec2 (ImDrawList::*)() const,&ImDrawList::GetClipRectMin >;
// from imgui/imgui.h:3433:21
	makeExtern<DAS_CALL_METHOD(_method_83), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipRectMin","das_call_member< ImVec2 (ImDrawList::*)() const , &ImDrawList::GetClipRectMin >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_84 = das::das_call_member< ImVec2 (ImDrawList::*)() const,&ImDrawList::GetClipRectMax >;
// from imgui/imgui.h:3434:21
	makeExtern<DAS_CALL_METHOD(_method_84), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipRectMax","das_call_member< ImVec2 (ImDrawList::*)() const , &ImDrawList::GetClipRectMax >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_85 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float),&ImDrawList::AddLine >;
// from imgui/imgui.h:3443:21
	makeExtern<DAS_CALL_METHOD(_method_85), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddLine","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float) , &ImDrawList::AddLine >::invoke")
		->args({"self","p1","p2","col","thickness"})
		->arg_init(4,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
}
}

