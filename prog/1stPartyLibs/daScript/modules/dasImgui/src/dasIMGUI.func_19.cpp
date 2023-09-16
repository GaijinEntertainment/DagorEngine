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
void Module_dasIMGUI::initFunctions_19() {
	using _method_19 = das::das_call_member< void (ImGuiTextFilter::ImGuiTextRange::*)(char,ImVector<ImGuiTextFilter::ImGuiTextRange> *) const,&ImGuiTextFilter::ImGuiTextRange::split >;
	makeExtern<DAS_CALL_METHOD(_method_19), SimNode_ExtFuncCall , imguiTempFn>(lib,"split","das_call_member< void (ImGuiTextFilter::ImGuiTextRange::*)(char,ImVector<ImGuiTextFilter::ImGuiTextRange> *) const , &ImGuiTextFilter::ImGuiTextRange::split >::invoke")
		->args({"self","separator","out"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiTextBuffer>(*this,lib,"ImGuiTextBuffer","ImGuiTextBuffer");
	using _method_20 = das::das_call_member< char (ImGuiTextBuffer::*)(int) const,&ImGuiTextBuffer::operator[] >;
	makeExtern<DAS_CALL_METHOD(_method_20), SimNode_ExtFuncCall , imguiTempFn>(lib,"[]","das_call_member< char (ImGuiTextBuffer::*)(int) const , &ImGuiTextBuffer::operator[] >::invoke")
		->args({"self","i"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_21 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::begin >;
	makeExtern<DAS_CALL_METHOD(_method_21), SimNode_ExtFuncCall , imguiTempFn>(lib,"begin","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::begin >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_22 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::end >;
	makeExtern<DAS_CALL_METHOD(_method_22), SimNode_ExtFuncCall , imguiTempFn>(lib,"end","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::end >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_23 = das::das_call_member< int (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::size >;
	makeExtern<DAS_CALL_METHOD(_method_23), SimNode_ExtFuncCall , imguiTempFn>(lib,"size","das_call_member< int (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::size >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_24 = das::das_call_member< bool (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::empty >;
	makeExtern<DAS_CALL_METHOD(_method_24), SimNode_ExtFuncCall , imguiTempFn>(lib,"empty","das_call_member< bool (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::empty >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_25 = das::das_call_member< void (ImGuiTextBuffer::*)(),&ImGuiTextBuffer::clear >;
	makeExtern<DAS_CALL_METHOD(_method_25), SimNode_ExtFuncCall , imguiTempFn>(lib,"clear","das_call_member< void (ImGuiTextBuffer::*)() , &ImGuiTextBuffer::clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_26 = das::das_call_member< void (ImGuiTextBuffer::*)(int),&ImGuiTextBuffer::reserve >;
	makeExtern<DAS_CALL_METHOD(_method_26), SimNode_ExtFuncCall , imguiTempFn>(lib,"reserve","das_call_member< void (ImGuiTextBuffer::*)(int) , &ImGuiTextBuffer::reserve >::invoke")
		->args({"self","capacity"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_27 = das::das_call_member< const char * (ImGuiTextBuffer::*)() const,&ImGuiTextBuffer::c_str >;
	makeExtern<DAS_CALL_METHOD(_method_27), SimNode_ExtFuncCall , imguiTempFn>(lib,"c_str","das_call_member< const char * (ImGuiTextBuffer::*)() const , &ImGuiTextBuffer::c_str >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiStorage::ImGuiStoragePair,unsigned int,int>(*this,lib,"ImGuiStoragePair","ImGuiStorage::ImGuiStoragePair")
		->args({"_key","_val_i"});
	addCtorAndUsing<ImGuiStorage::ImGuiStoragePair,unsigned int,float>(*this,lib,"ImGuiStoragePair","ImGuiStorage::ImGuiStoragePair")
		->args({"_key","_val_f"});
	addCtorAndUsing<ImGuiStorage::ImGuiStoragePair,unsigned int,void *>(*this,lib,"ImGuiStoragePair","ImGuiStorage::ImGuiStoragePair")
		->args({"_key","_val_p"});
	using _method_28 = das::das_call_member< void (ImGuiStorage::*)(),&ImGuiStorage::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_28), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiStorage::*)() , &ImGuiStorage::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_29 = das::das_call_member< int (ImGuiStorage::*)(unsigned int,int) const,&ImGuiStorage::GetInt >;
	makeExtern<DAS_CALL_METHOD(_method_29), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetInt","das_call_member< int (ImGuiStorage::*)(unsigned int,int) const , &ImGuiStorage::GetInt >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_30 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,int),&ImGuiStorage::SetInt >;
	makeExtern<DAS_CALL_METHOD(_method_30), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetInt","das_call_member< void (ImGuiStorage::*)(unsigned int,int) , &ImGuiStorage::SetInt >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_31 = das::das_call_member< bool (ImGuiStorage::*)(unsigned int,bool) const,&ImGuiStorage::GetBool >;
	makeExtern<DAS_CALL_METHOD(_method_31), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBool","das_call_member< bool (ImGuiStorage::*)(unsigned int,bool) const , &ImGuiStorage::GetBool >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_32 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,bool),&ImGuiStorage::SetBool >;
	makeExtern<DAS_CALL_METHOD(_method_32), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetBool","das_call_member< void (ImGuiStorage::*)(unsigned int,bool) , &ImGuiStorage::SetBool >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_33 = das::das_call_member< float (ImGuiStorage::*)(unsigned int,float) const,&ImGuiStorage::GetFloat >;
	makeExtern<DAS_CALL_METHOD(_method_33), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFloat","das_call_member< float (ImGuiStorage::*)(unsigned int,float) const , &ImGuiStorage::GetFloat >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_34 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,float),&ImGuiStorage::SetFloat >;
	makeExtern<DAS_CALL_METHOD(_method_34), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetFloat","das_call_member< void (ImGuiStorage::*)(unsigned int,float) , &ImGuiStorage::SetFloat >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

