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
void Module_dasIMGUI::initFunctions_18() {
	using _method_6 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(int,int),&ImGuiInputTextCallbackData::DeleteChars >;
	makeExtern<DAS_CALL_METHOD(_method_6), SimNode_ExtFuncCall , imguiTempFn>(lib,"DeleteChars","das_call_member< void (ImGuiInputTextCallbackData::*)(int,int) , &ImGuiInputTextCallbackData::DeleteChars >::invoke")
		->args({"self","pos","bytes_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_7 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(),&ImGuiInputTextCallbackData::SelectAll >;
	makeExtern<DAS_CALL_METHOD(_method_7), SimNode_ExtFuncCall , imguiTempFn>(lib,"SelectAll","das_call_member< void (ImGuiInputTextCallbackData::*)() , &ImGuiInputTextCallbackData::SelectAll >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_8 = das::das_call_member< void (ImGuiInputTextCallbackData::*)(),&ImGuiInputTextCallbackData::ClearSelection >;
	makeExtern<DAS_CALL_METHOD(_method_8), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearSelection","das_call_member< void (ImGuiInputTextCallbackData::*)() , &ImGuiInputTextCallbackData::ClearSelection >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_9 = das::das_call_member< bool (ImGuiInputTextCallbackData::*)() const,&ImGuiInputTextCallbackData::HasSelection >;
	makeExtern<DAS_CALL_METHOD(_method_9), SimNode_ExtFuncCall , imguiTempFn>(lib,"HasSelection","das_call_member< bool (ImGuiInputTextCallbackData::*)() const , &ImGuiInputTextCallbackData::HasSelection >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiPayload>(*this,lib,"ImGuiPayload","ImGuiPayload");
	using _method_10 = das::das_call_member< void (ImGuiPayload::*)(),&ImGuiPayload::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_10), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiPayload::*)() , &ImGuiPayload::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_11 = das::das_call_member< bool (ImGuiPayload::*)(const char *) const,&ImGuiPayload::IsDataType >;
	makeExtern<DAS_CALL_METHOD(_method_11), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsDataType","das_call_member< bool (ImGuiPayload::*)(const char *) const , &ImGuiPayload::IsDataType >::invoke")
		->args({"self","type"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_12 = das::das_call_member< bool (ImGuiPayload::*)() const,&ImGuiPayload::IsPreview >;
	makeExtern<DAS_CALL_METHOD(_method_12), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPreview","das_call_member< bool (ImGuiPayload::*)() const , &ImGuiPayload::IsPreview >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_13 = das::das_call_member< bool (ImGuiPayload::*)() const,&ImGuiPayload::IsDelivery >;
	makeExtern<DAS_CALL_METHOD(_method_13), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsDelivery","das_call_member< bool (ImGuiPayload::*)() const , &ImGuiPayload::IsDelivery >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTableColumnSortSpecs>(*this,lib,"ImGuiTableColumnSortSpecs","ImGuiTableColumnSortSpecs");
	addCtorAndUsing<ImGuiTableSortSpecs>(*this,lib,"ImGuiTableSortSpecs","ImGuiTableSortSpecs");
	addCtorAndUsing<ImGuiOnceUponAFrame>(*this,lib,"ImGuiOnceUponAFrame","ImGuiOnceUponAFrame");
	addCtorAndUsing<ImGuiTextFilter,const char *>(*this,lib,"ImGuiTextFilter","ImGuiTextFilter")
		->args({"default_filter"})
		->arg_init(0,make_smart<ExprConstString>(""));
	using _method_14 = das::das_call_member< bool (ImGuiTextFilter::*)(const char *,float),&ImGuiTextFilter::Draw >;
	makeExtern<DAS_CALL_METHOD(_method_14), SimNode_ExtFuncCall , imguiTempFn>(lib,"Draw","das_call_member< bool (ImGuiTextFilter::*)(const char *,float) , &ImGuiTextFilter::Draw >::invoke")
		->args({"self","label","width"})
		->arg_init(1,make_smart<ExprConstString>("Filter (inc,-exc)"))
		->arg_init(2,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_15 = das::das_call_member< void (ImGuiTextFilter::*)(),&ImGuiTextFilter::Build >;
	makeExtern<DAS_CALL_METHOD(_method_15), SimNode_ExtFuncCall , imguiTempFn>(lib,"Build","das_call_member< void (ImGuiTextFilter::*)() , &ImGuiTextFilter::Build >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_16 = das::das_call_member< void (ImGuiTextFilter::*)(),&ImGuiTextFilter::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_16), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiTextFilter::*)() , &ImGuiTextFilter::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_17 = das::das_call_member< bool (ImGuiTextFilter::*)() const,&ImGuiTextFilter::IsActive >;
	makeExtern<DAS_CALL_METHOD(_method_17), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsActive","das_call_member< bool (ImGuiTextFilter::*)() const , &ImGuiTextFilter::IsActive >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTextFilter::ImGuiTextRange>(*this,lib,"ImGuiTextRange","ImGuiTextFilter::ImGuiTextRange");
	addCtorAndUsing<ImGuiTextFilter::ImGuiTextRange,const char *,const char *>(*this,lib,"ImGuiTextRange","ImGuiTextFilter::ImGuiTextRange")
		->args({"_b","_e"});
	using _method_18 = das::das_call_member< bool (ImGuiTextFilter::ImGuiTextRange::*)() const,&ImGuiTextFilter::ImGuiTextRange::empty >;
	makeExtern<DAS_CALL_METHOD(_method_18), SimNode_ExtFuncCall , imguiTempFn>(lib,"empty","das_call_member< bool (ImGuiTextFilter::ImGuiTextRange::*)() const , &ImGuiTextFilter::ImGuiTextRange::empty >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

