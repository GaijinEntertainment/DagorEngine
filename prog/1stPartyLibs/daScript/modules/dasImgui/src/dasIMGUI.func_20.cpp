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
	using _method_12 = das::das_call_member< void (ImGuiIO::*)(const char *),&ImGuiIO::AddInputCharactersUTF8 >;
// from imgui/imgui.h:2411:21
	makeExtern<DAS_CALL_METHOD(_method_12), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddInputCharactersUTF8","das_call_member< void (ImGuiIO::*)(const char *) , &ImGuiIO::AddInputCharactersUTF8 >::invoke")
		->args({"self","str"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_13 = das::das_call_member< void (ImGuiIO::*)(ImGuiKey,int,int,int),&ImGuiIO::SetKeyEventNativeData >;
// from imgui/imgui.h:2413:21
	makeExtern<DAS_CALL_METHOD(_method_13), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetKeyEventNativeData","das_call_member< void (ImGuiIO::*)(ImGuiKey,int,int,int) , &ImGuiIO::SetKeyEventNativeData >::invoke")
		->args({"self","key","native_keycode","native_scancode","native_legacy_index"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_14 = das::das_call_member< void (ImGuiIO::*)(bool),&ImGuiIO::SetAppAcceptingEvents >;
// from imgui/imgui.h:2414:21
	makeExtern<DAS_CALL_METHOD(_method_14), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetAppAcceptingEvents","das_call_member< void (ImGuiIO::*)(bool) , &ImGuiIO::SetAppAcceptingEvents >::invoke")
		->args({"self","accepting_events"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_15 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearEventsQueue >;
// from imgui/imgui.h:2415:21
	makeExtern<DAS_CALL_METHOD(_method_15), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearEventsQueue","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearEventsQueue >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_16 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearInputKeys >;
// from imgui/imgui.h:2416:21
	makeExtern<DAS_CALL_METHOD(_method_16), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputKeys","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearInputKeys >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_17 = das::das_call_member< void (ImGuiIO::*)(),&ImGuiIO::ClearInputMouse >;
// from imgui/imgui.h:2417:21
	makeExtern<DAS_CALL_METHOD(_method_17), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputMouse","das_call_member< void (ImGuiIO::*)() , &ImGuiIO::ClearInputMouse >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiIO>(*this,lib,"ImGuiIO","ImGuiIO");
	addCtorAndUsing<ImGuiInputTextCallbackData>(*this,lib,"ImGuiInputTextCallbackData","ImGuiInputTextCallbackData");
	using _method_18 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(int,int),&ImGuiInputTextCallbackData::DeleteChars >;
// from imgui/imgui.h:2541:25
	makeExtern<DAS_CALL_METHOD(_method_18), SimNode_ExtFuncCall , imguiTempFn>(lib,"DeleteChars","das_call_member< void (ImGuiInputTextCallbackData::*)(int,int) , &ImGuiInputTextCallbackData::DeleteChars >::invoke")
		->args({"self","pos","bytes_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_19 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(),&ImGuiInputTextCallbackData::SelectAll >;
// from imgui/imgui.h:2543:25
	makeExtern<DAS_CALL_METHOD(_method_19), SimNode_ExtFuncCall , imguiTempFn>(lib,"SelectAll","das_call_member< void (ImGuiInputTextCallbackData::*)() , &ImGuiInputTextCallbackData::SelectAll >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_20 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(),&ImGuiInputTextCallbackData::ClearSelection >;
// from imgui/imgui.h:2544:25
	makeExtern<DAS_CALL_METHOD(_method_20), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearSelection","das_call_member< void (ImGuiInputTextCallbackData::*)() , &ImGuiInputTextCallbackData::ClearSelection >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_21 = das::das_call_member< bool (ImGuiInputTextCallbackData::*)() const,&ImGuiInputTextCallbackData::HasSelection >;
// from imgui/imgui.h:2545:25
	makeExtern<DAS_CALL_METHOD(_method_21), SimNode_ExtFuncCall , imguiTempFn>(lib,"HasSelection","das_call_member< bool (ImGuiInputTextCallbackData::*)() const , &ImGuiInputTextCallbackData::HasSelection >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiWindowClass>(*this,lib,"ImGuiWindowClass","ImGuiWindowClass");
	addCtorAndUsing<ImGuiPayload>(*this,lib,"ImGuiPayload","ImGuiPayload");
	using _method_22 = das::das_call_member< void (ImGuiPayload::*)(),&ImGuiPayload::Clear >;
// from imgui/imgui.h:2596:10
	makeExtern<DAS_CALL_METHOD(_method_22), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiPayload::*)() , &ImGuiPayload::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_23 = das::das_call_member< bool (ImGuiPayload::*)(const char *) const,&ImGuiPayload::IsDataType >;
// from imgui/imgui.h:2597:10
	makeExtern<DAS_CALL_METHOD(_method_23), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsDataType","das_call_member< bool (ImGuiPayload::*)(const char *) const , &ImGuiPayload::IsDataType >::invoke")
		->args({"self","type"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_24 = das::das_call_member< bool (ImGuiPayload::*)() const,&ImGuiPayload::IsPreview >;
// from imgui/imgui.h:2598:10
	makeExtern<DAS_CALL_METHOD(_method_24), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPreview","das_call_member< bool (ImGuiPayload::*)() const , &ImGuiPayload::IsPreview >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_25 = das::das_call_member< bool (ImGuiPayload::*)() const,&ImGuiPayload::IsDelivery >;
// from imgui/imgui.h:2599:10
	makeExtern<DAS_CALL_METHOD(_method_25), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsDelivery","das_call_member< bool (ImGuiPayload::*)() const , &ImGuiPayload::IsDelivery >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiOnceUponAFrame>(*this,lib,"ImGuiOnceUponAFrame","ImGuiOnceUponAFrame");
	addCtorAndUsing<ImGuiTextFilter,const char *>(*this,lib,"ImGuiTextFilter","ImGuiTextFilter")
		->args({"default_filter"})
		->arg_init(0,make_smart<ExprConstString>(""));
}
}

