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
void Module_dasIMGUI::initFunctions_18() {
// from imgui/imgui.h:1133:29
	makeExtern< void (*)(ImGuiKey) , ImGui::SetItemKeyOwner , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemKeyOwner","ImGui::SetItemKeyOwner")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1139:29
	makeExtern< bool (*)(int) , ImGui::IsMouseDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDown","ImGui::IsMouseDown")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1140:29
	makeExtern< bool (*)(int,bool) , ImGui::IsMouseClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseClicked","ImGui::IsMouseClicked")
		->args({"button","repeat"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1141:29
	makeExtern< bool (*)(int) , ImGui::IsMouseReleased , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseReleased","ImGui::IsMouseReleased")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1142:29
	makeExtern< bool (*)(int) , ImGui::IsMouseDoubleClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDoubleClicked","ImGui::IsMouseDoubleClicked")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1143:29
	makeExtern< bool (*)(int,float) , ImGui::IsMouseReleasedWithDelay , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseReleasedWithDelay","ImGui::IsMouseReleasedWithDelay")
		->args({"button","delay"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1144:29
	makeExtern< int (*)(int) , ImGui::GetMouseClickedCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseClickedCount","ImGui::GetMouseClickedCount")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1145:29
	makeExtern< bool (*)(const ImVec2 &,const ImVec2 &,bool) , ImGui::IsMouseHoveringRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseHoveringRect","ImGui::IsMouseHoveringRect")
		->args({"r_min","r_max","clip"})
		->arg_init(2,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1146:29
	makeExtern< bool (*)(const ImVec2 *) , ImGui::IsMousePosValid , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMousePosValid","ImGui::IsMousePosValid")
		->args({"mouse_pos"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1147:29
	makeExtern< bool (*)() , ImGui::IsAnyMouseDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyMouseDown","ImGui::IsAnyMouseDown")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1148:29
	makeExtern< ImVec2 (*)() , ImGui::GetMousePos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMousePos","ImGui::GetMousePos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1149:29
	makeExtern< ImVec2 (*)() , ImGui::GetMousePosOnOpeningCurrentPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMousePosOnOpeningCurrentPopup","ImGui::GetMousePosOnOpeningCurrentPopup")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1150:29
	makeExtern< bool (*)(int,float) , ImGui::IsMouseDragging , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDragging","ImGui::IsMouseDragging")
		->args({"button","lock_threshold"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(1,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1151:29
	makeExtern< ImVec2 (*)(int,float) , ImGui::GetMouseDragDelta , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseDragDelta","ImGui::GetMouseDragDelta")
		->args({"button","lock_threshold"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->arg_init(1,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1152:29
	makeExtern< void (*)(int) , ImGui::ResetMouseDragDelta , SimNode_ExtFuncCall , imguiTempFn>(lib,"ResetMouseDragDelta","ImGui::ResetMouseDragDelta")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1153:32
	makeExtern< int (*)() , ImGui::GetMouseCursor , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseCursor","ImGui::GetMouseCursor")
		->res_type(makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1154:29
	makeExtern< void (*)(int) , ImGui::SetMouseCursor , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetMouseCursor","ImGui::SetMouseCursor")
		->args({"cursor_type"})
		->arg_type(0,makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1155:29
	makeExtern< void (*)(bool) , ImGui::SetNextFrameWantCaptureMouse , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextFrameWantCaptureMouse","ImGui::SetNextFrameWantCaptureMouse")
		->args({"want_capture_mouse"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1159:29
	makeExtern< const char * (*)() , ImGui::GetClipboardText , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipboardText","ImGui::GetClipboardText")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1160:29
	makeExtern< void (*)(const char *) , ImGui::SetClipboardText , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetClipboardText","ImGui::SetClipboardText")
		->args({"text"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

