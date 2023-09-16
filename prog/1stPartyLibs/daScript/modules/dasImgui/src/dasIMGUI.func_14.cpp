// this file is generated via daScript automatic C++ binder
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
void Module_dasIMGUI::initFunctions_14() {
	makeExtern< const ImGuiPayload * (*)(const char *,int) , ImGui::AcceptDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"AcceptDragDropPayload","ImGui::AcceptDragDropPayload")
		->args({"type","flags"})
		->arg_type(1,makeType<ImGuiDragDropFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiDragDropFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndDragDropTarget , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDragDropTarget","ImGui::EndDragDropTarget")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const ImGuiPayload * (*)() , ImGui::GetDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDragDropPayload","ImGui::GetDragDropPayload")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,const ImVec2 &,bool) , ImGui::PushClipRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRect","ImGui::PushClipRect")
		->args({"clip_rect_min","clip_rect_max","intersect_with_current_clip_rect"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::PopClipRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopClipRect","ImGui::PopClipRect")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::SetItemDefaultFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemDefaultFocus","ImGui::SetItemDefaultFocus")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ImGui::SetKeyboardFocusHere , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetKeyboardFocusHere","ImGui::SetKeyboardFocusHere")
		->args({"offset"})
		->arg_init(0,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsItemHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemHovered","ImGui::IsItemHovered")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiHoveredFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiHoveredFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemActive , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemActive","ImGui::IsItemActive")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemFocused , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemFocused","ImGui::IsItemFocused")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsItemClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemClicked","ImGui::IsItemClicked")
		->args({"mouse_button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemVisible","ImGui::IsItemVisible")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemEdited , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemEdited","ImGui::IsItemEdited")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemActivated , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemActivated","ImGui::IsItemActivated")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemDeactivated , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemDeactivated","ImGui::IsItemDeactivated")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemDeactivatedAfterEdit , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemDeactivatedAfterEdit","ImGui::IsItemDeactivatedAfterEdit")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsItemToggledOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemToggledOpen","ImGui::IsItemToggledOpen")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsAnyItemHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyItemHovered","ImGui::IsAnyItemHovered")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsAnyItemActive , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyItemActive","ImGui::IsAnyItemActive")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsAnyItemFocused , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyItemFocused","ImGui::IsAnyItemFocused")
		->addToModule(*this, SideEffects::worstDefault);
}
}

