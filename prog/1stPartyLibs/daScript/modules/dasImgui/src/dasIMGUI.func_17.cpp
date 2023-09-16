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
void Module_dasIMGUI::initFunctions_17() {
	makeExtern< int (*)() , ImGui::GetMouseCursor , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseCursor","ImGui::GetMouseCursor")
		->res_type(makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ImGui::SetMouseCursor , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetMouseCursor","ImGui::SetMouseCursor")
		->args({"cursor_type"})
		->arg_type(0,makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool) , ImGui::CaptureMouseFromApp , SimNode_ExtFuncCall , imguiTempFn>(lib,"CaptureMouseFromApp","ImGui::CaptureMouseFromApp")
		->args({"want_capture_mouse_value"})
		->arg_init(0,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const char * (*)() , ImGui::GetClipboardText , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipboardText","ImGui::GetClipboardText")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::SetClipboardText , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetClipboardText","ImGui::SetClipboardText")
		->args({"text"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::LoadIniSettingsFromDisk , SimNode_ExtFuncCall , imguiTempFn>(lib,"LoadIniSettingsFromDisk","ImGui::LoadIniSettingsFromDisk")
		->args({"ini_filename"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,size_t) , ImGui::LoadIniSettingsFromMemory , SimNode_ExtFuncCall , imguiTempFn>(lib,"LoadIniSettingsFromMemory","ImGui::LoadIniSettingsFromMemory")
		->args({"ini_data","ini_size"})
		->arg_init(1,make_smart<ExprConstUInt64>(0x0))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::SaveIniSettingsToDisk , SimNode_ExtFuncCall , imguiTempFn>(lib,"SaveIniSettingsToDisk","ImGui::SaveIniSettingsToDisk")
		->args({"ini_filename"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const char * (*)(size_t *) , ImGui::SaveIniSettingsToMemory , SimNode_ExtFuncCall , imguiTempFn>(lib,"SaveIniSettingsToMemory","ImGui::SaveIniSettingsToMemory")
		->args({"out_ini_size"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,size_t,size_t,size_t,size_t,size_t,size_t) , ImGui::DebugCheckVersionAndDataLayout , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugCheckVersionAndDataLayout","ImGui::DebugCheckVersionAndDataLayout")
		->args({"version_str","sz_io","sz_style","sz_vec2","sz_vec4","sz_drawvert","sz_drawidx"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void * (*)(size_t) , ImGui::MemAlloc , SimNode_ExtFuncCall , imguiTempFn>(lib,"MemAlloc","ImGui::MemAlloc")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(void *) , ImGui::MemFree , SimNode_ExtFuncCall , imguiTempFn>(lib,"MemFree","ImGui::MemFree")
		->args({"ptr"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiStyle>(*this,lib,"ImGuiStyle","ImGuiStyle");
	using _method_1 = das::das_call_member< void (ImGuiStyle::*)(float),&ImGuiStyle::ScaleAllSizes >;
	makeExtern<DAS_CALL_METHOD(_method_1), SimNode_ExtFuncCall , imguiTempFn>(lib,"ScaleAllSizes","das_call_member< void (ImGuiStyle::*)(float) , &ImGuiStyle::ScaleAllSizes >::invoke")
		->args({"self","scale_factor"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_2 = das::das_call_member< void (ImGuiIO::*)(unsigned int),&ImGuiIO::AddInputCharacter >;
	makeExtern<DAS_CALL_METHOD(_method_2), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharacter","das_call_member< void (ImGuiIO::*)(unsigned int) , &ImGuiIO::AddInputCharacter >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_3 = das::das_call_member< void (ImGuiIO::*)(unsigned short),&ImGuiIO::AddInputCharacterUTF16 >;
	makeExtern<DAS_CALL_METHOD(_method_3), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharacterUTF16","das_call_member< void (ImGuiIO::*)(unsigned short) , &ImGuiIO::AddInputCharacterUTF16 >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_4 = das::das_call_member< void (ImGuiIO::*)(const char *),&ImGuiIO::AddInputCharactersUTF8 >;
	makeExtern<DAS_CALL_METHOD(_method_4), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharactersUTF8","das_call_member< void (ImGuiIO::*)(const char *) , &ImGuiIO::AddInputCharactersUTF8 >::invoke")
		->args({"self","str"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_5 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearInputCharacters >;
	makeExtern<DAS_CALL_METHOD(_method_5), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputCharacters","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearInputCharacters >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiIO>(*this,lib,"ImGuiIO","ImGuiIO");
	addCtorAndUsing<ImGuiInputTextCallbackData>(*this,lib,"ImGuiInputTextCallbackData","ImGuiInputTextCallbackData");
}
}

