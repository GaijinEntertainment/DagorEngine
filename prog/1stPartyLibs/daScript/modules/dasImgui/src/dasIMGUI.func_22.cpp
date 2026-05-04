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
void Module_dasIMGUI::initFunctions_22() {
	using _method_35 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::begin >;
// from imgui/imgui.h:2893:25
	makeExtern<DAS_CALL_METHOD(_method_35), SimNode_ExtFuncCall , imguiTempFn>(lib,"begin","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::begin >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_36 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::end >;
// from imgui/imgui.h:2894:25
	makeExtern<DAS_CALL_METHOD(_method_36), SimNode_ExtFuncCall , imguiTempFn>(lib,"end","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::end >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_37 = das::das_call_member< int (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::size >;
// from imgui/imgui.h:2895:25
	makeExtern<DAS_CALL_METHOD(_method_37), SimNode_ExtFuncCall , imguiTempFn>(lib,"size","das_call_member< int (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::size >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_38 = das::das_call_member< bool (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::empty >;
// from imgui/imgui.h:2896:25
	makeExtern<DAS_CALL_METHOD(_method_38), SimNode_ExtFuncCall , imguiTempFn>(lib,"empty","das_call_member< bool (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::empty >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_39 = das::das_call_member< void (ImGuiTextBuffer::*)(),&ImGuiTextBuffer::clear >;
// from imgui/imgui.h:2897:25
	makeExtern<DAS_CALL_METHOD(_method_39), SimNode_ExtFuncCall , imguiTempFn>(lib,"clear","das_call_member< void (ImGuiTextBuffer::*)() , &ImGuiTextBuffer::clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_40 = das::das_call_member< void (ImGuiTextBuffer::*)(int),&ImGuiTextBuffer::resize >;
// from imgui/imgui.h:2898:25
	makeExtern<DAS_CALL_METHOD(_method_40), SimNode_ExtFuncCall , imguiTempFn>(lib,"resize","das_call_member< void (ImGuiTextBuffer::*)(int) , &ImGuiTextBuffer::resize >::invoke")
		->args({"self","size"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_41 = das::das_call_member< void (ImGuiTextBuffer::*)(int),&ImGuiTextBuffer::reserve >;
// from imgui/imgui.h:2899:25
	makeExtern<DAS_CALL_METHOD(_method_41), SimNode_ExtFuncCall , imguiTempFn>(lib,"reserve","das_call_member< void (ImGuiTextBuffer::*)(int) , &ImGuiTextBuffer::reserve >::invoke")
		->args({"self","capacity"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_42 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::c_str >;
// from imgui/imgui.h:2900:25
	makeExtern<DAS_CALL_METHOD(_method_42), SimNode_ExtFuncCall , imguiTempFn>(lib,"c_str","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::c_str >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiStoragePair,unsigned int,int>(*this,lib,"ImGuiStoragePair","ImGuiStoragePair")
		->args({"_key","_val"});
	addCtorAndUsing<ImGuiStoragePair,unsigned int,float>(*this,lib,"ImGuiStoragePair","ImGuiStoragePair")
		->args({"_key","_val"});
	addCtorAndUsing<ImGuiStoragePair,unsigned int,void *>(*this,lib,"ImGuiStoragePair","ImGuiStoragePair")
		->args({"_key","_val"});
	using _method_43 = das::das_call_member< void (ImGuiStorage::*)(),&ImGuiStorage::Clear >;
// from imgui/imgui.h:2932:25
	makeExtern<DAS_CALL_METHOD(_method_43), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiStorage::*)() , &ImGuiStorage::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_44 = das::das_call_member< int (ImGuiStorage::*)(unsigned int,int) const,&ImGuiStorage::GetInt >;
// from imgui/imgui.h:2933:25
	makeExtern<DAS_CALL_METHOD(_method_44), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetInt","das_call_member< int (ImGuiStorage::*)(unsigned int,int) const , &ImGuiStorage::GetInt >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_45 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,int),&ImGuiStorage::SetInt >;
// from imgui/imgui.h:2934:25
	makeExtern<DAS_CALL_METHOD(_method_45), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetInt","das_call_member< void (ImGuiStorage::*)(unsigned int,int) , &ImGuiStorage::SetInt >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_46 = das::das_call_member< bool (ImGuiStorage::*)(unsigned int,bool) const,&ImGuiStorage::GetBool >;
// from imgui/imgui.h:2935:25
	makeExtern<DAS_CALL_METHOD(_method_46), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBool","das_call_member< bool (ImGuiStorage::*)(unsigned int,bool) const , &ImGuiStorage::GetBool >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_47 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,bool),&ImGuiStorage::SetBool >;
// from imgui/imgui.h:2936:25
	makeExtern<DAS_CALL_METHOD(_method_47), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetBool","das_call_member< void (ImGuiStorage::*)(unsigned int,bool) , &ImGuiStorage::SetBool >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_48 = das::das_call_member< float (ImGuiStorage::*)(unsigned int,float) const,&ImGuiStorage::GetFloat >;
// from imgui/imgui.h:2937:25
	makeExtern<DAS_CALL_METHOD(_method_48), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFloat","das_call_member< float (ImGuiStorage::*)(unsigned int,float) const , &ImGuiStorage::GetFloat >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_49 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,float),&ImGuiStorage::SetFloat >;
// from imgui/imgui.h:2938:25
	makeExtern<DAS_CALL_METHOD(_method_49), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetFloat","das_call_member< void (ImGuiStorage::*)(unsigned int,float) , &ImGuiStorage::SetFloat >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_50 = das::das_call_member< void * (ImGuiStorage::*)(unsigned int) const,&ImGuiStorage::GetVoidPtr >;
// from imgui/imgui.h:2939:25
	makeExtern<DAS_CALL_METHOD(_method_50), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVoidPtr","das_call_member< void * (ImGuiStorage::*)(unsigned int) const , &ImGuiStorage::GetVoidPtr >::invoke")
		->args({"self","key"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_51 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,void *),&ImGuiStorage::SetVoidPtr >;
// from imgui/imgui.h:2940:25
	makeExtern<DAS_CALL_METHOD(_method_51), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetVoidPtr","das_call_member< void (ImGuiStorage::*)(unsigned int,void *) , &ImGuiStorage::SetVoidPtr >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

