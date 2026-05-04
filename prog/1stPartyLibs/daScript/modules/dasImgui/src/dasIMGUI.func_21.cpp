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
void Module_dasIMGUI::initFunctions_21() {
	using _method_22 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(),&ImGuiInputTextCallbackData::ClearSelection >;
// from imgui/imgui.h:2779:25
	makeExtern<DAS_CALL_METHOD(_method_22), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearSelection","das_call_member< void (ImGuiInputTextCallbackData::*)() , &ImGuiInputTextCallbackData::ClearSelection >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_23 = das::das_call_member< bool (ImGuiInputTextCallbackData::*)() const,&ImGuiInputTextCallbackData::HasSelection >;
// from imgui/imgui.h:2780:25
	makeExtern<DAS_CALL_METHOD(_method_23), SimNode_ExtFuncCall , imguiTempFn>(lib,"HasSelection","das_call_member< bool (ImGuiInputTextCallbackData::*)() const , &ImGuiInputTextCallbackData::HasSelection >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiWindowClass>(*this,lib,"ImGuiWindowClass","ImGuiWindowClass");
	addCtorAndUsing<ImGuiPayload>(*this,lib,"ImGuiPayload","ImGuiPayload");
	using _method_24 = das::das_call_member< void (ImGuiPayload::*)(),&ImGuiPayload::Clear >;
// from imgui/imgui.h:2831:10
	makeExtern<DAS_CALL_METHOD(_method_24), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiPayload::*)() , &ImGuiPayload::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_25 = das::das_call_member< bool (ImGuiPayload::*)(const char *) const,&ImGuiPayload::IsDataType >;
// from imgui/imgui.h:2832:10
	makeExtern<DAS_CALL_METHOD(_method_25), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsDataType","das_call_member< bool (ImGuiPayload::*)(const char *) const , &ImGuiPayload::IsDataType >::invoke")
		->args({"self","type"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_26 = das::das_call_member< bool (ImGuiPayload::*)() const,&ImGuiPayload::IsPreview >;
// from imgui/imgui.h:2833:10
	makeExtern<DAS_CALL_METHOD(_method_26), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPreview","das_call_member< bool (ImGuiPayload::*)() const , &ImGuiPayload::IsPreview >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_27 = das::das_call_member< bool (ImGuiPayload::*)() const,&ImGuiPayload::IsDelivery >;
// from imgui/imgui.h:2834:10
	makeExtern<DAS_CALL_METHOD(_method_27), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsDelivery","das_call_member< bool (ImGuiPayload::*)() const , &ImGuiPayload::IsDelivery >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiOnceUponAFrame>(*this,lib,"ImGuiOnceUponAFrame","ImGuiOnceUponAFrame");
	addCtorAndUsing<ImGuiTextFilter,const char *>(*this,lib,"ImGuiTextFilter","ImGuiTextFilter")
		->args({"default_filter"})
		->arg_init(0,make_smart<ExprConstString>(""));
	using _method_28 = das::das_call_member< bool (ImGuiTextFilter::*)(const char *,float),&ImGuiTextFilter::Draw >;
// from imgui/imgui.h:2862:25
	makeExtern<DAS_CALL_METHOD(_method_28), SimNode_ExtFuncCall , imguiTempFn>(lib,"Draw","das_call_member< bool (ImGuiTextFilter::*)(const char *,float) , &ImGuiTextFilter::Draw >::invoke")
		->args({"self","label","width"})
		->arg_init(1,make_smart<ExprConstString>("Filter (inc,-exc)"))
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_29 = das::das_call_member< void (ImGuiTextFilter::*)(),&ImGuiTextFilter::Build >;
// from imgui/imgui.h:2864:25
	makeExtern<DAS_CALL_METHOD(_method_29), SimNode_ExtFuncCall , imguiTempFn>(lib,"Build","das_call_member< void (ImGuiTextFilter::*)() , &ImGuiTextFilter::Build >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_30 = das::das_call_member< void (ImGuiTextFilter::*)(),&ImGuiTextFilter::Clear >;
// from imgui/imgui.h:2865:25
	makeExtern<DAS_CALL_METHOD(_method_30), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiTextFilter::*)() , &ImGuiTextFilter::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_31 = das::das_call_member< bool (ImGuiTextFilter::*)() const,&ImGuiTextFilter::IsActive >;
// from imgui/imgui.h:2866:25
	makeExtern<DAS_CALL_METHOD(_method_31), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsActive","das_call_member< bool (ImGuiTextFilter::*)() const , &ImGuiTextFilter::IsActive >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTextFilter::ImGuiTextRange>(*this,lib,"ImGuiTextRange","ImGuiTextFilter::ImGuiTextRange");
	addCtorAndUsing<ImGuiTextFilter::ImGuiTextRange,const char *,const char *>(*this,lib,"ImGuiTextRange","ImGuiTextFilter::ImGuiTextRange")
		->args({"_b","_e"});
	using _method_32 = das::das_call_member< bool (ImGuiTextFilter::ImGuiTextRange::*)() const,&ImGuiTextFilter::ImGuiTextRange::empty >;
// from imgui/imgui.h:2876:25
	makeExtern<DAS_CALL_METHOD(_method_32), SimNode_ExtFuncCall , imguiTempFn>(lib,"empty","das_call_member< bool (ImGuiTextFilter::ImGuiTextRange::*)() const , &ImGuiTextFilter::ImGuiTextRange::empty >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_33 = das::das_call_member< void (ImGuiTextFilter::ImGuiTextRange::*)(char,ImVector<ImGuiTextFilter::ImGuiTextRange> *) const,&ImGuiTextFilter::ImGuiTextRange::split >;
// from imgui/imgui.h:2877:25
	makeExtern<DAS_CALL_METHOD(_method_33), SimNode_ExtFuncCall , imguiTempFn>(lib,"split","das_call_member< void (ImGuiTextFilter::ImGuiTextRange::*)(char,ImVector<ImGuiTextFilter::ImGuiTextRange> *) const , &ImGuiTextFilter::ImGuiTextRange::split >::invoke")
		->args({"self","separator","out"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTextBuffer>(*this,lib,"ImGuiTextBuffer","ImGuiTextBuffer");
	using _method_34 = das::das_call_member< char (ImGuiTextBuffer::*)(int) const,&ImGuiTextBuffer::operator[] >;
// from imgui/imgui.h:2892:25
	makeExtern<DAS_CALL_METHOD(_method_34), SimNode_ExtFuncCall , imguiTempFn>(lib,"[]","das_call_member< char (ImGuiTextBuffer::*)(int) const , &ImGuiTextBuffer::operator[] >::invoke")
		->args({"self","i"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

