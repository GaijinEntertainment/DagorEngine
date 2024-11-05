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
void Module_dasIMGUI::initFunctions_15() {
// from imgui/imgui.h:907:37
	makeExtern< const ImGuiPayload * (*)(const char *,int) , ImGui::AcceptDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"AcceptDragDropPayload","ImGui::AcceptDragDropPayload")
		->args({"type","flags"})
		->arg_type(1,makeType<ImGuiDragDropFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiDragDropFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:908:37
	makeExtern< void (*)() , ImGui::EndDragDropTarget , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDragDropTarget","ImGui::EndDragDropTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:909:37
	makeExtern< const ImGuiPayload * (*)() , ImGui::GetDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDragDropPayload","ImGui::GetDragDropPayload")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:916:29
	makeExtern< void (*)(bool) , ImGui::BeginDisabled , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDisabled","ImGui::BeginDisabled")
		->args({"disabled"})
		->arg_init(0,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:917:29
	makeExtern< void (*)() , ImGui::EndDisabled , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDisabled","ImGui::EndDisabled")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:921:29
	makeExtern< void (*)(const ImVec2 &,const ImVec2 &,bool) , ImGui::PushClipRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRect","ImGui::PushClipRect")
		->args({"clip_rect_min","clip_rect_max","intersect_with_current_clip_rect"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:922:29
	makeExtern< void (*)() , ImGui::PopClipRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopClipRect","ImGui::PopClipRect")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:926:29
	makeExtern< void (*)() , ImGui::SetItemDefaultFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemDefaultFocus","ImGui::SetItemDefaultFocus")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:927:29
	makeExtern< void (*)(int) , ImGui::SetKeyboardFocusHere , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetKeyboardFocusHere","ImGui::SetKeyboardFocusHere")
		->args({"offset"})
		->arg_init(0,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:930:29
	makeExtern< void (*)() , ImGui::SetNextItemAllowOverlap , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemAllowOverlap","ImGui::SetNextItemAllowOverlap")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:935:29
	makeExtern< bool (*)(int) , ImGui::IsItemHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemHovered","ImGui::IsItemHovered")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiHoveredFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiHoveredFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:936:29
	makeExtern< bool (*)() , ImGui::IsItemActive , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemActive","ImGui::IsItemActive")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:937:29
	makeExtern< bool (*)() , ImGui::IsItemFocused , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemFocused","ImGui::IsItemFocused")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:938:29
	makeExtern< bool (*)(int) , ImGui::IsItemClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemClicked","ImGui::IsItemClicked")
		->args({"mouse_button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:939:29
	makeExtern< bool (*)() , ImGui::IsItemVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemVisible","ImGui::IsItemVisible")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:940:29
	makeExtern< bool (*)() , ImGui::IsItemEdited , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemEdited","ImGui::IsItemEdited")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:941:29
	makeExtern< bool (*)() , ImGui::IsItemActivated , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemActivated","ImGui::IsItemActivated")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:942:29
	makeExtern< bool (*)() , ImGui::IsItemDeactivated , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemDeactivated","ImGui::IsItemDeactivated")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:943:29
	makeExtern< bool (*)() , ImGui::IsItemDeactivatedAfterEdit , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemDeactivatedAfterEdit","ImGui::IsItemDeactivatedAfterEdit")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:944:29
	makeExtern< bool (*)() , ImGui::IsItemToggledOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemToggledOpen","ImGui::IsItemToggledOpen")
		->addToModule(*this, SideEffects::worstDefault);
}
}

