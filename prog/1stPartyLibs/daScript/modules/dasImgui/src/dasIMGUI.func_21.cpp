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
	using _method_26 = das::das_call_member< bool (ImGuiTextFilter::*)(const char *,float),&ImGuiTextFilter::Draw >;
// from imgui/imgui.h:2627:25
	makeExtern<DAS_CALL_METHOD(_method_26), SimNode_ExtFuncCall , imguiTempFn>(lib,"Draw","das_call_member< bool (ImGuiTextFilter::*)(const char *,float) , &ImGuiTextFilter::Draw >::invoke")
		->args({"self","label","width"})
		->arg_init(1,make_smart<ExprConstString>("Filter (inc,-exc)"))
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_27 = das::das_call_member< void (ImGuiTextFilter::*)(),&ImGuiTextFilter::Build >;
// from imgui/imgui.h:2629:25
	makeExtern<DAS_CALL_METHOD(_method_27), SimNode_ExtFuncCall , imguiTempFn>(lib,"Build","das_call_member< void (ImGuiTextFilter::*)() , &ImGuiTextFilter::Build >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_28 = das::das_call_member< void (ImGuiTextFilter::*)(),&ImGuiTextFilter::Clear >;
// from imgui/imgui.h:2630:25
	makeExtern<DAS_CALL_METHOD(_method_28), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiTextFilter::*)() , &ImGuiTextFilter::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_29 = das::das_call_member< bool (ImGuiTextFilter::*)() const,&ImGuiTextFilter::IsActive >;
// from imgui/imgui.h:2631:25
	makeExtern<DAS_CALL_METHOD(_method_29), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsActive","das_call_member< bool (ImGuiTextFilter::*)() const , &ImGuiTextFilter::IsActive >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTextFilter::ImGuiTextRange>(*this,lib,"ImGuiTextRange","ImGuiTextFilter::ImGuiTextRange");
	addCtorAndUsing<ImGuiTextFilter::ImGuiTextRange,const char *,const char *>(*this,lib,"ImGuiTextRange","ImGuiTextFilter::ImGuiTextRange")
		->args({"_b","_e"});
	using _method_30 = das::das_call_member< bool (ImGuiTextFilter::ImGuiTextRange::*)() const,&ImGuiTextFilter::ImGuiTextRange::empty >;
// from imgui/imgui.h:2641:25
	makeExtern<DAS_CALL_METHOD(_method_30), SimNode_ExtFuncCall , imguiTempFn>(lib,"empty","das_call_member< bool (ImGuiTextFilter::ImGuiTextRange::*)() const , &ImGuiTextFilter::ImGuiTextRange::empty >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_31 = das::das_call_member< void (ImGuiTextFilter::ImGuiTextRange::*)(char,ImVector<ImGuiTextFilter::ImGuiTextRange> *) const,&ImGuiTextFilter::ImGuiTextRange::split >;
// from imgui/imgui.h:2642:25
	makeExtern<DAS_CALL_METHOD(_method_31), SimNode_ExtFuncCall , imguiTempFn>(lib,"split","das_call_member< void (ImGuiTextFilter::ImGuiTextRange::*)(char,ImVector<ImGuiTextFilter::ImGuiTextRange> *) const , &ImGuiTextFilter::ImGuiTextRange::split >::invoke")
		->args({"self","separator","out"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTextBuffer>(*this,lib,"ImGuiTextBuffer","ImGuiTextBuffer");
	using _method_32 = das::das_call_member< char (ImGuiTextBuffer::*)(int) const,&ImGuiTextBuffer::operator[] >;
// from imgui/imgui.h:2657:25
	makeExtern<DAS_CALL_METHOD(_method_32), SimNode_ExtFuncCall , imguiTempFn>(lib,"[]","das_call_member< char (ImGuiTextBuffer::*)(int) const , &ImGuiTextBuffer::operator[] >::invoke")
		->args({"self","i"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_33 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::begin >;
// from imgui/imgui.h:2658:25
	makeExtern<DAS_CALL_METHOD(_method_33), SimNode_ExtFuncCall , imguiTempFn>(lib,"begin","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::begin >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_34 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::end >;
// from imgui/imgui.h:2659:25
	makeExtern<DAS_CALL_METHOD(_method_34), SimNode_ExtFuncCall , imguiTempFn>(lib,"end","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::end >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_35 = das::das_call_member< int (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::size >;
// from imgui/imgui.h:2660:25
	makeExtern<DAS_CALL_METHOD(_method_35), SimNode_ExtFuncCall , imguiTempFn>(lib,"size","das_call_member< int (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::size >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_36 = das::das_call_member< bool (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::empty >;
// from imgui/imgui.h:2661:25
	makeExtern<DAS_CALL_METHOD(_method_36), SimNode_ExtFuncCall , imguiTempFn>(lib,"empty","das_call_member< bool (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::empty >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_37 = das::das_call_member< void (ImGuiTextBuffer::*)(),&ImGuiTextBuffer::clear >;
// from imgui/imgui.h:2662:25
	makeExtern<DAS_CALL_METHOD(_method_37), SimNode_ExtFuncCall , imguiTempFn>(lib,"clear","das_call_member< void (ImGuiTextBuffer::*)() , &ImGuiTextBuffer::clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_38 = das::das_call_member< void (ImGuiTextBuffer::*)(int),&ImGuiTextBuffer::reserve >;
// from imgui/imgui.h:2663:25
	makeExtern<DAS_CALL_METHOD(_method_38), SimNode_ExtFuncCall , imguiTempFn>(lib,"reserve","das_call_member< void (ImGuiTextBuffer::*)(int) , &ImGuiTextBuffer::reserve >::invoke")
		->args({"self","capacity"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_39 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::c_str >;
// from imgui/imgui.h:2664:25
	makeExtern<DAS_CALL_METHOD(_method_39), SimNode_ExtFuncCall , imguiTempFn>(lib,"c_str","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::c_str >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiStoragePair,unsigned int,int>(*this,lib,"ImGuiStoragePair","ImGuiStoragePair")
		->args({"_key","_val"});
	addCtorAndUsing<ImGuiStoragePair,unsigned int,float>(*this,lib,"ImGuiStoragePair","ImGuiStoragePair")
		->args({"_key","_val"});
	addCtorAndUsing<ImGuiStoragePair,unsigned int,void *>(*this,lib,"ImGuiStoragePair","ImGuiStoragePair")
		->args({"_key","_val"});
}
}

