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
void Module_dasIMGUI::initFunctions_20() {
	using _method_4 = das::das_call_member< void (ImGuiIO::*)(ImGuiKey,bool,float),&ImGuiIO::AddKeyAnalogEvent >;
// from imgui/imgui.h:2626:21
	makeExtern<DAS_CALL_METHOD(_method_4), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddKeyAnalogEvent","das_call_member< void (ImGuiIO::*)(ImGuiKey,bool,float) , &ImGuiIO::AddKeyAnalogEvent >::invoke")
		->args({"self","key","down","v"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_5 = das::das_call_member< void (ImGuiIO::*)(float,float),&ImGuiIO::AddMousePosEvent >;
// from imgui/imgui.h:2627:21
	makeExtern<DAS_CALL_METHOD(_method_5), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMousePosEvent","das_call_member< void (ImGuiIO::*)(float,float) , &ImGuiIO::AddMousePosEvent >::invoke")
		->args({"self","x","y"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_6 = das::das_call_member< void (ImGuiIO::*)(int,bool),&ImGuiIO::AddMouseButtonEvent >;
// from imgui/imgui.h:2628:21
	makeExtern<DAS_CALL_METHOD(_method_6), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseButtonEvent","das_call_member< void (ImGuiIO::*)(int,bool) , &ImGuiIO::AddMouseButtonEvent >::invoke")
		->args({"self","button","down"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_7 = das::das_call_member< void (ImGuiIO::*)(float,float),&ImGuiIO::AddMouseWheelEvent >;
// from imgui/imgui.h:2629:21
	makeExtern<DAS_CALL_METHOD(_method_7), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseWheelEvent","das_call_member< void (ImGuiIO::*)(float,float) , &ImGuiIO::AddMouseWheelEvent >::invoke")
		->args({"self","wheel_x","wheel_y"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_8 = das::das_call_member< void (ImGuiIO::*)(ImGuiMouseSource),&ImGuiIO::AddMouseSourceEvent >;
// from imgui/imgui.h:2630:21
	makeExtern<DAS_CALL_METHOD(_method_8), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseSourceEvent","das_call_member< void (ImGuiIO::*)(ImGuiMouseSource) , &ImGuiIO::AddMouseSourceEvent >::invoke")
		->args({"self","source"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_9 = das::das_call_member< void (ImGuiIO::*)(unsigned int),&ImGuiIO::AddMouseViewportEvent >;
// from imgui/imgui.h:2631:21
	makeExtern<DAS_CALL_METHOD(_method_9), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddMouseViewportEvent","das_call_member< void (ImGuiIO::*)(unsigned int) , &ImGuiIO::AddMouseViewportEvent >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_10 = das::das_call_member< void (ImGuiIO::*)(bool),&ImGuiIO::AddFocusEvent >;
// from imgui/imgui.h:2632:21
	makeExtern<DAS_CALL_METHOD(_method_10), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFocusEvent","das_call_member< void (ImGuiIO::*)(bool) , &ImGuiIO::AddFocusEvent >::invoke")
		->args({"self","focused"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_11 = das::das_call_member< void (ImGuiIO::*)(unsigned int),&ImGuiIO::AddInputCharacter >;
// from imgui/imgui.h:2633:21
	makeExtern<DAS_CALL_METHOD(_method_11), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharacter","das_call_member< void (ImGuiIO::*)(unsigned int) , &ImGuiIO::AddInputCharacter >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_12 = das::das_call_member< void (ImGuiIO::*)(unsigned short),&ImGuiIO::AddInputCharacterUTF16 >;
// from imgui/imgui.h:2634:21
	makeExtern<DAS_CALL_METHOD(_method_12), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharacterUTF16","das_call_member< void (ImGuiIO::*)(unsigned short) , &ImGuiIO::AddInputCharacterUTF16 >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_13 = das::das_call_member< void (ImGuiIO::*)(const char *),&ImGuiIO::AddInputCharactersUTF8 >;
// from imgui/imgui.h:2635:21
	makeExtern<DAS_CALL_METHOD(_method_13), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharactersUTF8","das_call_member< void (ImGuiIO::*)(const char *) , &ImGuiIO::AddInputCharactersUTF8 >::invoke")
		->args({"self","str"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_14 = das::das_call_member< void (ImGuiIO::*)(ImGuiKey,int,int,int),&ImGuiIO::SetKeyEventNativeData >;
// from imgui/imgui.h:2637:21
	makeExtern<DAS_CALL_METHOD(_method_14), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetKeyEventNativeData","das_call_member< void (ImGuiIO::*)(ImGuiKey,int,int,int) , &ImGuiIO::SetKeyEventNativeData >::invoke")
		->args({"self","key","native_keycode","native_scancode","native_legacy_index"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_15 = das::das_call_member< void (ImGuiIO::*)(bool),&ImGuiIO::SetAppAcceptingEvents >;
// from imgui/imgui.h:2638:21
	makeExtern<DAS_CALL_METHOD(_method_15), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetAppAcceptingEvents","das_call_member< void (ImGuiIO::*)(bool) , &ImGuiIO::SetAppAcceptingEvents >::invoke")
		->args({"self","accepting_events"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_16 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearEventsQueue >;
// from imgui/imgui.h:2639:21
	makeExtern<DAS_CALL_METHOD(_method_16), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearEventsQueue","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearEventsQueue >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_17 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearInputKeys >;
// from imgui/imgui.h:2640:21
	makeExtern<DAS_CALL_METHOD(_method_17), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputKeys","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearInputKeys >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_18 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearInputMouse >;
// from imgui/imgui.h:2641:21
	makeExtern<DAS_CALL_METHOD(_method_18), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputMouse","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearInputMouse >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiIO>(*this,lib,"ImGuiIO","ImGuiIO");
	addCtorAndUsing<ImGuiInputTextCallbackData>(*this,lib,"ImGuiInputTextCallbackData","ImGuiInputTextCallbackData");
	using _method_19 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(int,int),&ImGuiInputTextCallbackData::DeleteChars >;
// from imgui/imgui.h:2775:25
	makeExtern<DAS_CALL_METHOD(_method_19), SimNode_ExtFuncCall , imguiTempFn>(lib,"DeleteChars","das_call_member< void (ImGuiInputTextCallbackData::*)(int,int) , &ImGuiInputTextCallbackData::DeleteChars >::invoke")
		->args({"self","pos","bytes_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_20 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(),&ImGuiInputTextCallbackData::SelectAll >;
// from imgui/imgui.h:2777:25
	makeExtern<DAS_CALL_METHOD(_method_20), SimNode_ExtFuncCall , imguiTempFn>(lib,"SelectAll","das_call_member< void (ImGuiInputTextCallbackData::*)() , &ImGuiInputTextCallbackData::SelectAll >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_21 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(int,int),&ImGuiInputTextCallbackData::SetSelection >;
// from imgui/imgui.h:2778:25
	makeExtern<DAS_CALL_METHOD(_method_21), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetSelection","das_call_member< void (ImGuiInputTextCallbackData::*)(int,int) , &ImGuiInputTextCallbackData::SetSelection >::invoke")
		->args({"self","s","e"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

