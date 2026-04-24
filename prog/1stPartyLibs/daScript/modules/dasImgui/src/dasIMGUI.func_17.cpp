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
// from imgui/imgui.h:1075:29
	makeExtern< bool (*)(const ImVec2 &,const ImVec2 &) , ImGui::IsRectVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsRectVisible","ImGui::IsRectVisible")
		->args({"rect_min","rect_max"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1076:29
	makeExtern< double (*)() , ImGui::GetTime , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTime","ImGui::GetTime")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1077:29
	makeExtern< int (*)() , ImGui::GetFrameCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameCount","ImGui::GetFrameCount")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1078:37
	makeExtern< ImDrawListSharedData * (*)() , ImGui::GetDrawListSharedData , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDrawListSharedData","ImGui::GetDrawListSharedData")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1079:29
	makeExtern< const char * (*)(int) , ImGui::GetStyleColorName , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStyleColorName","ImGui::GetStyleColorName")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1080:29
	makeExtern< void (*)(ImGuiStorage *) , ImGui::SetStateStorage , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetStateStorage","ImGui::SetStateStorage")
		->args({"storage"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1081:29
	makeExtern< ImGuiStorage * (*)() , ImGui::GetStateStorage , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStateStorage","ImGui::GetStateStorage")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1087:29
	makeExtern< ImVec4 (*)(unsigned int) , ImGui::ColorConvertU32ToFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertU32ToFloat4","ImGui::ColorConvertU32ToFloat4")
		->args({"in"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1088:29
	makeExtern< unsigned int (*)(const ImVec4 &) , ImGui::ColorConvertFloat4ToU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertFloat4ToU32","ImGui::ColorConvertFloat4ToU32")
		->args({"in"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1089:29
	makeExtern< void (*)(float,float,float,float &,float &,float &) , ImGui::ColorConvertRGBtoHSV , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertRGBtoHSV","ImGui::ColorConvertRGBtoHSV")
		->args({"r","g","b","out_h","out_s","out_v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1090:29
	makeExtern< void (*)(float,float,float,float &,float &,float &) , ImGui::ColorConvertHSVtoRGB , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertHSVtoRGB","ImGui::ColorConvertHSVtoRGB")
		->args({"h","s","v","out_r","out_g","out_b"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1096:29
	makeExtern< bool (*)(ImGuiKey) , ImGui::IsKeyDown , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyDown","ImGui::IsKeyDown")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1097:29
	makeExtern< bool (*)(ImGuiKey,bool) , ImGui::IsKeyPressed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyPressed","ImGui::IsKeyPressed")
		->args({"key","repeat"})
		->arg_init(1,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1098:29
	makeExtern< bool (*)(ImGuiKey) , ImGui::IsKeyReleased , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyReleased","ImGui::IsKeyReleased")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1099:29
	makeExtern< bool (*)(int) , ImGui::IsKeyChordPressed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsKeyChordPressed","ImGui::IsKeyChordPressed")
		->args({"key_chord"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1100:29
	makeExtern< int (*)(ImGuiKey,float,float) , ImGui::GetKeyPressedAmount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetKeyPressedAmount","ImGui::GetKeyPressedAmount")
		->args({"key","repeat_delay","rate"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1101:29
	makeExtern< const char * (*)(ImGuiKey) , ImGui::GetKeyName , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetKeyName","ImGui::GetKeyName")
		->args({"key"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1102:29
	makeExtern< void (*)(bool) , ImGui::SetNextFrameWantCaptureKeyboard , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextFrameWantCaptureKeyboard","ImGui::SetNextFrameWantCaptureKeyboard")
		->args({"want_capture_keyboard"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1124:29
	makeExtern< bool (*)(int,int) , ImGui::Shortcut , SimNode_ExtFuncCall , imguiTempFn>(lib,"Shortcut","ImGui::Shortcut")
		->args({"key_chord","flags"})
		->arg_init(1,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1125:29
	makeExtern< void (*)(int,int) , ImGui::SetNextItemShortcut , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemShortcut","ImGui::SetNextItemShortcut")
		->args({"key_chord","flags"})
		->arg_init(1,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
}
}

