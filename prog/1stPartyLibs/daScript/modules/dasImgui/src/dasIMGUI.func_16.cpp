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
void Module_dasIMGUI::initFunctions_16() {
	makeExtern< void (*)(float,float,float,float &,float &,float &) , ImGui::ColorConvertRGBtoHSV , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertRGBtoHSV","ImGui::ColorConvertRGBtoHSV")
		->args({"r","g","b","out_h","out_s","out_v"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float,float,float,float &,float &,float &) , ImGui::ColorConvertHSVtoRGB , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertHSVtoRGB","ImGui::ColorConvertHSVtoRGB")
		->args({"h","s","v","out_r","out_g","out_b"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)(int) , ImGui::GetKeyIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetKeyIndex","ImGui::GetKeyIndex")
		->args({"imgui_key"})
		->arg_type(0,makeType<ImGuiKey_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsKeyDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyDown","ImGui::IsKeyDown")
		->args({"user_key_index"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int,bool) , ImGui::IsKeyPressed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyPressed","ImGui::IsKeyPressed")
		->args({"user_key_index","repeat"})
		->arg_init(1,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsKeyReleased , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyReleased","ImGui::IsKeyReleased")
		->args({"user_key_index"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)(int,float,float) , ImGui::GetKeyPressedAmount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetKeyPressedAmount","ImGui::GetKeyPressedAmount")
		->args({"key_index","repeat_delay","rate"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool) , ImGui::CaptureKeyboardFromApp , SimNode_ExtFuncCall , imguiTempFn>(lib,"CaptureKeyboardFromApp","ImGui::CaptureKeyboardFromApp")
		->args({"want_capture_keyboard_value"})
		->arg_init(0,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsMouseDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDown","ImGui::IsMouseDown")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int,bool) , ImGui::IsMouseClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseClicked","ImGui::IsMouseClicked")
		->args({"button","repeat"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsMouseReleased , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseReleased","ImGui::IsMouseReleased")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsMouseDoubleClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDoubleClicked","ImGui::IsMouseDoubleClicked")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const ImVec2 &,const ImVec2 &,bool) , ImGui::IsMouseHoveringRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseHoveringRect","ImGui::IsMouseHoveringRect")
		->args({"r_min","r_max","clip"})
		->arg_init(2,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const ImVec2 *) , ImGui::IsMousePosValid , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMousePosValid","ImGui::IsMousePosValid")
		->args({"mouse_pos"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsAnyMouseDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyMouseDown","ImGui::IsAnyMouseDown")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetMousePos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMousePos","ImGui::GetMousePos")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetMousePosOnOpeningCurrentPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMousePosOnOpeningCurrentPopup","ImGui::GetMousePosOnOpeningCurrentPopup")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int,float) , ImGui::IsMouseDragging , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDragging","ImGui::IsMouseDragging")
		->args({"button","lock_threshold"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(1,make_smart<ExprConstFloat>(-1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)(int,float) , ImGui::GetMouseDragDelta , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseDragDelta","ImGui::GetMouseDragDelta")
		->args({"button","lock_threshold"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->arg_init(1,make_smart<ExprConstFloat>(-1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ImGui::ResetMouseDragDelta , SimNode_ExtFuncCall , imguiTempFn>(lib,"ResetMouseDragDelta","ImGui::ResetMouseDragDelta")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

