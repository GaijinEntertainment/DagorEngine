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
void Module_dasIMGUI::initFunctions_19() {
// from imgui/imgui.h:1166:29
	makeExtern< void (*)(const char *) , ImGui::LoadIniSettingsFromDisk , SimNode_ExtFuncCall , imguiTempFn>(lib,"LoadIniSettingsFromDisk","ImGui::LoadIniSettingsFromDisk")
		->args({"ini_filename"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1167:29
	makeExtern< void (*)(const char *,uint64_t) , ImGui::LoadIniSettingsFromMemory , SimNode_ExtFuncCall , imguiTempFn>(lib,"LoadIniSettingsFromMemory","ImGui::LoadIniSettingsFromMemory")
		->args({"ini_data","ini_size"})
		->arg_init(1,make_smart<ExprConstUInt64>(0x0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1168:29
	makeExtern< void (*)(const char *) , ImGui::SaveIniSettingsToDisk , SimNode_ExtFuncCall , imguiTempFn>(lib,"SaveIniSettingsToDisk","ImGui::SaveIniSettingsToDisk")
		->args({"ini_filename"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1169:29
	makeExtern< const char * (*)(size_t *) , ImGui::SaveIniSettingsToMemory , SimNode_ExtFuncCall , imguiTempFn>(lib,"SaveIniSettingsToMemory","ImGui::SaveIniSettingsToMemory")
		->args({"out_ini_size"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1175:29
	makeExtern< void (*)(const char *) , ImGui::DebugTextEncoding , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugTextEncoding","ImGui::DebugTextEncoding")
		->args({"text"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1176:29
	makeExtern< void (*)(int) , ImGui::DebugFlashStyleColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugFlashStyleColor","ImGui::DebugFlashStyleColor")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1177:29
	makeExtern< void (*)() , ImGui::DebugStartItemPicker , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugStartItemPicker","ImGui::DebugStartItemPicker")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1178:29
	makeExtern< bool (*)(const char *,size_t,size_t,size_t,size_t,size_t,size_t) , ImGui::DebugCheckVersionAndDataLayout , SimNode_ExtFuncCall , imguiTempFn>(lib,"DebugCheckVersionAndDataLayout","ImGui::DebugCheckVersionAndDataLayout")
		->args({"version_str","sz_io","sz_style","sz_vec2","sz_vec4","sz_drawvert","sz_drawidx"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1190:29
	makeExtern< void * (*)(size_t) , ImGui::MemAlloc , SimNode_ExtFuncCall , imguiTempFn>(lib,"MemAlloc","ImGui::MemAlloc")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1191:29
	makeExtern< void (*)(void *) , ImGui::MemFree , SimNode_ExtFuncCall , imguiTempFn>(lib,"MemFree","ImGui::MemFree")
		->args({"ptr"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1196:29
	makeExtern< void (*)() , ImGui::UpdatePlatformWindows , SimNode_ExtFuncCall , imguiTempFn>(lib,"UpdatePlatformWindows","ImGui::UpdatePlatformWindows")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1197:29
	makeExtern< void (*)(void *,void *) , ImGui::RenderPlatformWindowsDefault , SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderPlatformWindowsDefault","ImGui::RenderPlatformWindowsDefault")
		->args({"platform_render_arg","renderer_render_arg"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1198:29
	makeExtern< void (*)() , ImGui::DestroyPlatformWindows , SimNode_ExtFuncCall , imguiTempFn>(lib,"DestroyPlatformWindows","ImGui::DestroyPlatformWindows")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1199:30
	makeExtern< ImGuiViewport * (*)(unsigned int) , ImGui::FindViewportByID , SimNode_ExtFuncCall , imguiTempFn>(lib,"FindViewportByID","ImGui::FindViewportByID")
		->args({"viewport_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1200:30
	makeExtern< ImGuiViewport * (*)(void *) , ImGui::FindViewportByPlatformHandle , SimNode_ExtFuncCall , imguiTempFn>(lib,"FindViewportByPlatformHandle","ImGui::FindViewportByPlatformHandle")
		->args({"platform_handle"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTableSortSpecs>(*this,lib,"ImGuiTableSortSpecs","ImGuiTableSortSpecs");
	addCtorAndUsing<ImGuiTableColumnSortSpecs>(*this,lib,"ImGuiTableColumnSortSpecs","ImGuiTableColumnSortSpecs");
	addCtorAndUsing<ImGuiStyle>(*this,lib,"ImGuiStyle","ImGuiStyle");
	using _method_2 = das::das_call_member< void (ImGuiStyle::*)(float),&ImGuiStyle::ScaleAllSizes >;
// from imgui/imgui.h:2455:22
	makeExtern<DAS_CALL_METHOD(_method_2), SimNode_ExtFuncCall , imguiTempFn>(lib,"ScaleAllSizes","das_call_member< void (ImGuiStyle::*)(float) , &ImGuiStyle::ScaleAllSizes >::invoke")
		->args({"self","scale_factor"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_3 = das::das_call_member< void (ImGuiIO::*)(ImGuiKey,bool),&ImGuiIO::AddKeyEvent >;
// from imgui/imgui.h:2625:21
	makeExtern<DAS_CALL_METHOD(_method_3), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddKeyEvent","das_call_member< void (ImGuiIO::*)(ImGuiKey,bool) , &ImGuiIO::AddKeyEvent >::invoke")
		->args({"self","key","down"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

