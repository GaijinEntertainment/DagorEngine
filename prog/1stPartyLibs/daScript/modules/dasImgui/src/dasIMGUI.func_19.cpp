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
// from imgui/imgui.h:1079:33
	makeExtern< ImGuiPlatformIO & (*)() , ImGui::GetPlatformIO , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetPlatformIO","ImGui::GetPlatformIO")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1080:33
	makeExtern< void (*)() , ImGui::UpdatePlatformWindows , SimNode_ExtFuncCall , imguiTempFn>(lib,"UpdatePlatformWindows","ImGui::UpdatePlatformWindows")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1081:33
	makeExtern< void (*)(void *,void *) , ImGui::RenderPlatformWindowsDefault , SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderPlatformWindowsDefault","ImGui::RenderPlatformWindowsDefault")
		->args({"platform_render_arg","renderer_render_arg"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1082:33
	makeExtern< void (*)() , ImGui::DestroyPlatformWindows , SimNode_ExtFuncCall , imguiTempFn>(lib,"DestroyPlatformWindows","ImGui::DestroyPlatformWindows")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1083:33
	makeExtern< ImGuiViewport * (*)(unsigned int) , ImGui::FindViewportByID , SimNode_ExtFuncCall , imguiTempFn>(lib,"FindViewportByID","ImGui::FindViewportByID")
		->args({"id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1084:33
	makeExtern< ImGuiViewport * (*)(void *) , ImGui::FindViewportByPlatformHandle , SimNode_ExtFuncCall , imguiTempFn>(lib,"FindViewportByPlatformHandle","ImGui::FindViewportByPlatformHandle")
		->args({"platform_handle"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTableSortSpecs>(*this,lib,"ImGuiTableSortSpecs","ImGuiTableSortSpecs");
	addCtorAndUsing<ImGuiTableColumnSortSpecs>(*this,lib,"ImGuiTableColumnSortSpecs","ImGuiTableColumnSortSpecs");
	addCtorAndUsing<ImGuiStyle>(*this,lib,"ImGuiStyle","ImGuiStyle");
	using _method_1 = das::das_call_member< void (ImGuiStyle::*)(float),&ImGuiStyle::ScaleAllSizes >;
// from imgui/imgui.h:2264:20
	makeExtern<DAS_CALL_METHOD(_method_1), SimNode_ExtFuncCall , imguiTempFn>(lib,"ScaleAllSizes","das_call_member< void (ImGuiStyle::*)(float) , &ImGuiStyle::ScaleAllSizes >::invoke")
		->args({"self","scale_factor"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_2 = das::das_call_member< void (ImGuiIO::*)(ImGuiKey,bool),&ImGuiIO::AddKeyEvent >;
// from imgui/imgui.h:2401:21
	makeExtern<DAS_CALL_METHOD(_method_2), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddKeyEvent","das_call_member< void (ImGuiIO::*)(ImGuiKey,bool) , &ImGuiIO::AddKeyEvent >::invoke")
		->args({"self","key","down"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_3 = das::das_call_member< void (ImGuiIO::*)(ImGuiKey,bool,float),&ImGuiIO::AddKeyAnalogEvent >;
// from imgui/imgui.h:2402:21
	makeExtern<DAS_CALL_METHOD(_method_3), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddKeyAnalogEvent","das_call_member< void (ImGuiIO::*)(ImGuiKey,bool,float) , &ImGuiIO::AddKeyAnalogEvent >::invoke")
		->args({"self","key","down","v"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_4 = das::das_call_member< void (ImGuiIO::*)(float,float),&ImGuiIO::AddMousePosEvent >;
// from imgui/imgui.h:2403:21
	makeExtern<DAS_CALL_METHOD(_method_4), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMousePosEvent","das_call_member< void (ImGuiIO::*)(float,float) , &ImGuiIO::AddMousePosEvent >::invoke")
		->args({"self","x","y"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_5 = das::das_call_member< void (ImGuiIO::*)(int,bool),&ImGuiIO::AddMouseButtonEvent >;
// from imgui/imgui.h:2404:21
	makeExtern<DAS_CALL_METHOD(_method_5), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseButtonEvent","das_call_member< void (ImGuiIO::*)(int,bool) , &ImGuiIO::AddMouseButtonEvent >::invoke")
		->args({"self","button","down"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_6 = das::das_call_member< void (ImGuiIO::*)(float,float),&ImGuiIO::AddMouseWheelEvent >;
// from imgui/imgui.h:2405:21
	makeExtern<DAS_CALL_METHOD(_method_6), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseWheelEvent","das_call_member< void (ImGuiIO::*)(float,float) , &ImGuiIO::AddMouseWheelEvent >::invoke")
		->args({"self","wheel_x","wheel_y"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_7 = das::das_call_member< void (ImGuiIO::*)(ImGuiMouseSource),&ImGuiIO::AddMouseSourceEvent >;
// from imgui/imgui.h:2406:21
	makeExtern<DAS_CALL_METHOD(_method_7), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseSourceEvent","das_call_member< void (ImGuiIO::*)(ImGuiMouseSource) , &ImGuiIO::AddMouseSourceEvent >::invoke")
		->args({"self","source"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_8 = das::das_call_member< void (ImGuiIO::*)(unsigned int),&ImGuiIO::AddMouseViewportEvent >;
// from imgui/imgui.h:2407:21
	makeExtern<DAS_CALL_METHOD(_method_8), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseViewportEvent","das_call_member< void (ImGuiIO::*)(unsigned int) , &ImGuiIO::AddMouseViewportEvent >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_9 = das::das_call_member< void (ImGuiIO::*)(bool),&ImGuiIO::AddFocusEvent >;
// from imgui/imgui.h:2408:21
	makeExtern<DAS_CALL_METHOD(_method_9), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFocusEvent","das_call_member< void (ImGuiIO::*)(bool) , &ImGuiIO::AddFocusEvent >::invoke")
		->args({"self","focused"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_10 = das::das_call_member< void (ImGuiIO::*)(unsigned int),&ImGuiIO::AddInputCharacter >;
// from imgui/imgui.h:2409:21
	makeExtern<DAS_CALL_METHOD(_method_10), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharacter","das_call_member< void (ImGuiIO::*)(unsigned int) , &ImGuiIO::AddInputCharacter >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_11 = das::das_call_member< void (ImGuiIO::*)(unsigned short),&ImGuiIO::AddInputCharacterUTF16 >;
// from imgui/imgui.h:2410:21
	makeExtern<DAS_CALL_METHOD(_method_11), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharacterUTF16","das_call_member< void (ImGuiIO::*)(unsigned short) , &ImGuiIO::AddInputCharacterUTF16 >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

