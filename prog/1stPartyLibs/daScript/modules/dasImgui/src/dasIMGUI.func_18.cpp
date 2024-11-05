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
// from imgui/imgui.h:1033:29
	makeExtern< ImVec2 (*)() , ImGui::GetMousePos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMousePos","ImGui::GetMousePos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1034:29
	makeExtern< ImVec2 (*)() , ImGui::GetMousePosOnOpeningCurrentPopup , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMousePosOnOpeningCurrentPopup","ImGui::GetMousePosOnOpeningCurrentPopup")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1035:29
	makeExtern< bool (*)(int,float) , ImGui::IsMouseDragging , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsMouseDragging","ImGui::IsMouseDragging")
		->args({"button","lock_threshold"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(1,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1036:29
	makeExtern< ImVec2 (*)(int,float) , ImGui::GetMouseDragDelta , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseDragDelta","ImGui::GetMouseDragDelta")
		->args({"button","lock_threshold"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->arg_init(1,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1037:29
	makeExtern< void (*)(int) , ImGui::ResetMouseDragDelta , SimNode_ExtFuncCall , imguiTempFn>(lib,"ResetMouseDragDelta","ImGui::ResetMouseDragDelta")
		->args({"button"})
		->arg_type(0,makeType<ImGuiMouseButton_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiMouseButton_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1038:32
	makeExtern< int (*)() , ImGui::GetMouseCursor , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseCursor","ImGui::GetMouseCursor")
		->res_type(makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1039:29
	makeExtern< void (*)(int) , ImGui::SetMouseCursor , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetMouseCursor","ImGui::SetMouseCursor")
		->args({"cursor_type"})
		->arg_type(0,makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1040:29
	makeExtern< void (*)(bool) , ImGui::SetNextFrameWantCaptureMouse , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextFrameWantCaptureMouse","ImGui::SetNextFrameWantCaptureMouse")
		->args({"want_capture_mouse"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1044:29
	makeExtern< const char * (*)() , ImGui::GetClipboardText , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipboardText","ImGui::GetClipboardText")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1045:29
	makeExtern< void (*)(const char *) , ImGui::SetClipboardText , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetClipboardText","ImGui::SetClipboardText")
		->args({"text"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1051:29
	makeExtern< void (*)(const char *) , ImGui::LoadIniSettingsFromDisk , SimNode_ExtFuncCall , imguiTempFn>(lib,"LoadIniSettingsFromDisk","ImGui::LoadIniSettingsFromDisk")
		->args({"ini_filename"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1052:29
	makeExtern< void (*)(const char *,uint64_t) , ImGui::LoadIniSettingsFromMemory , SimNode_ExtFuncCall , imguiTempFn>(lib,"LoadIniSettingsFromMemory","ImGui::LoadIniSettingsFromMemory")
		->args({"ini_data","ini_size"})
		->arg_init(1,make_smart<ExprConstUInt64>(0x0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1053:29
	makeExtern< void (*)(const char *) , ImGui::SaveIniSettingsToDisk , SimNode_ExtFuncCall , imguiTempFn>(lib,"SaveIniSettingsToDisk","ImGui::SaveIniSettingsToDisk")
		->args({"ini_filename"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1054:29
	makeExtern< const char * (*)(size_t *) , ImGui::SaveIniSettingsToMemory , SimNode_ExtFuncCall , imguiTempFn>(lib,"SaveIniSettingsToMemory","ImGui::SaveIniSettingsToMemory")
		->args({"out_ini_size"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1058:29
	makeExtern< void (*)(const char *) , ImGui::DebugTextEncoding , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugTextEncoding","ImGui::DebugTextEncoding")
		->args({"text"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1059:29
	makeExtern< void (*)(int) , ImGui::DebugFlashStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugFlashStyleColor","ImGui::DebugFlashStyleColor")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1060:29
	makeExtern< void (*)() , ImGui::DebugStartItemPicker , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugStartItemPicker","ImGui::DebugStartItemPicker")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1061:29
	makeExtern< bool (*)(const char *,size_t,size_t,size_t,size_t,size_t,size_t) , ImGui::DebugCheckVersionAndDataLayout , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugCheckVersionAndDataLayout","ImGui::DebugCheckVersionAndDataLayout")
		->args({"version_str","sz_io","sz_style","sz_vec2","sz_vec4","sz_drawvert","sz_drawidx"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1073:29
	makeExtern< void * (*)(size_t) , ImGui::MemAlloc , SimNode_ExtFuncCall , imguiTempFn>(lib,"MemAlloc","ImGui::MemAlloc")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1074:29
	makeExtern< void (*)(void *) , ImGui::MemFree , SimNode_ExtFuncCall , imguiTempFn>(lib,"MemFree","ImGui::MemFree")
		->args({"ptr"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

