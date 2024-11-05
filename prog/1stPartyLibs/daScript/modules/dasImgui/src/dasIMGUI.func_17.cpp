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
void Module_dasIMGUI::initFunctions_17() {
// from imgui/imgui.h:979:29
	makeExtern< void (*)(float,float,float,float &,float &,float &) , ImGui::ColorConvertRGBtoHSV , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertRGBtoHSV","ImGui::ColorConvertRGBtoHSV")
		->args({"r","g","b","out_h","out_s","out_v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:980:29
	makeExtern< void (*)(float,float,float,float &,float &,float &) , ImGui::ColorConvertHSVtoRGB , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertHSVtoRGB","ImGui::ColorConvertHSVtoRGB")
		->args({"h","s","v","out_r","out_g","out_b"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:987:29
	makeExtern< bool (*)(ImGuiKey) , ImGui::IsKeyDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyDown","ImGui::IsKeyDown")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:988:29
	makeExtern< bool (*)(ImGuiKey,bool) , ImGui::IsKeyPressed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyPressed","ImGui::IsKeyPressed")
		->args({"key","repeat"})
		->arg_init(1,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:989:29
	makeExtern< bool (*)(ImGuiKey) , ImGui::IsKeyReleased , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyReleased","ImGui::IsKeyReleased")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:990:29
	makeExtern< bool (*)(int) , ImGui::IsKeyChordPressed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyChordPressed","ImGui::IsKeyChordPressed")
		->args({"key_chord"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:991:29
	makeExtern< int (*)(ImGuiKey,float,float) , ImGui::GetKeyPressedAmount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetKeyPressedAmount","ImGui::GetKeyPressedAmount")
		->args({"key","repeat_delay","rate"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:992:29
	makeExtern< const char * (*)(ImGuiKey) , ImGui::GetKeyName , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetKeyName","ImGui::GetKeyName")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:993:29
	makeExtern< void (*)(bool) , ImGui::SetNextFrameWantCaptureKeyboard , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextFrameWantCaptureKeyboard","ImGui::SetNextFrameWantCaptureKeyboard")
		->args({"want_capture_keyboard"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1010:29
	makeExtern< bool (*)(int,int) , ImGui::Shortcut , SimNode_ExtFuncCall , imguiTempFn>(lib,"Shortcut","ImGui::Shortcut")
		->args({"key_chord","flags"})
		->arg_init(1,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1011:29
	makeExtern< void (*)(int,int) , ImGui::SetNextItemShortcut , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemShortcut","ImGui::SetNextItemShortcut")
		->args({"key_chord","flags"})
		->arg_init(1,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1019:29
	makeExtern< void (*)(ImGuiKey) , ImGui::SetItemKeyOwner , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemKeyOwner","ImGui::SetItemKeyOwner")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1025:29
	makeExtern< bool (*)(int) , ImGui::IsMouseDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDown","ImGui::IsMouseDown")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1026:29
	makeExtern< bool (*)(int,bool) , ImGui::IsMouseClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseClicked","ImGui::IsMouseClicked")
		->args({"button","repeat"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(1,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1027:29
	makeExtern< bool (*)(int) , ImGui::IsMouseReleased , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseReleased","ImGui::IsMouseReleased")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1028:29
	makeExtern< bool (*)(int) , ImGui::IsMouseDoubleClicked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDoubleClicked","ImGui::IsMouseDoubleClicked")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1029:29
	makeExtern< int (*)(int) , ImGui::GetMouseClickedCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseClickedCount","ImGui::GetMouseClickedCount")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1030:29
	makeExtern< bool (*)(const ImVec2 &,const ImVec2 &,bool) , ImGui::IsMouseHoveringRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseHoveringRect","ImGui::IsMouseHoveringRect")
		->args({"r_min","r_max","clip"})
		->arg_init(2,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1031:29
	makeExtern< bool (*)(const ImVec2 *) , ImGui::IsMousePosValid , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMousePosValid","ImGui::IsMousePosValid")
		->args({"mouse_pos"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1032:29
	makeExtern< bool (*)() , ImGui::IsAnyMouseDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyMouseDown","ImGui::IsAnyMouseDown")
		->addToModule(*this, SideEffects::worstDefault);
}
}

