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
	using _method_40 = das::das_call_member< void (ImGuiStorage::*)(),&ImGuiStorage::Clear >;
// from imgui/imgui.h:2696:25
	makeExtern<DAS_CALL_METHOD(_method_40), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiStorage::*)() , &ImGuiStorage::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_41 = das::das_call_member< int (ImGuiStorage::*)(unsigned int,int) const,&ImGuiStorage::GetInt >;
// from imgui/imgui.h:2697:25
	makeExtern<DAS_CALL_METHOD(_method_41), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetInt","das_call_member< int (ImGuiStorage::*)(unsigned int,int) const , &ImGuiStorage::GetInt >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_42 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,int),&ImGuiStorage::SetInt >;
// from imgui/imgui.h:2698:25
	makeExtern<DAS_CALL_METHOD(_method_42), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetInt","das_call_member< void (ImGuiStorage::*)(unsigned int,int) , &ImGuiStorage::SetInt >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_43 = das::das_call_member< bool (ImGuiStorage::*)(unsigned int,bool) const,&ImGuiStorage::GetBool >;
// from imgui/imgui.h:2699:25
	makeExtern<DAS_CALL_METHOD(_method_43), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBool","das_call_member< bool (ImGuiStorage::*)(unsigned int,bool) const , &ImGuiStorage::GetBool >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_44 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,bool),&ImGuiStorage::SetBool >;
// from imgui/imgui.h:2700:25
	makeExtern<DAS_CALL_METHOD(_method_44), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetBool","das_call_member< void (ImGuiStorage::*)(unsigned int,bool) , &ImGuiStorage::SetBool >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_45 = das::das_call_member< float (ImGuiStorage::*)(unsigned int,float) const,&ImGuiStorage::GetFloat >;
// from imgui/imgui.h:2701:25
	makeExtern<DAS_CALL_METHOD(_method_45), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFloat","das_call_member< float (ImGuiStorage::*)(unsigned int,float) const , &ImGuiStorage::GetFloat >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_46 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,float),&ImGuiStorage::SetFloat >;
// from imgui/imgui.h:2702:25
	makeExtern<DAS_CALL_METHOD(_method_46), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetFloat","das_call_member< void (ImGuiStorage::*)(unsigned int,float) , &ImGuiStorage::SetFloat >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_47 = das::das_call_member< void * (ImGuiStorage::*)(unsigned int) const,&ImGuiStorage::GetVoidPtr >;
// from imgui/imgui.h:2703:25
	makeExtern<DAS_CALL_METHOD(_method_47), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVoidPtr","das_call_member< void * (ImGuiStorage::*)(unsigned int) const , &ImGuiStorage::GetVoidPtr >::invoke")
		->args({"self","key"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_48 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,void *),&ImGuiStorage::SetVoidPtr >;
// from imgui/imgui.h:2704:25
	makeExtern<DAS_CALL_METHOD(_method_48), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetVoidPtr","das_call_member< void (ImGuiStorage::*)(unsigned int,void *) , &ImGuiStorage::SetVoidPtr >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_49 = das::das_call_member< int * (ImGuiStorage::*)(unsigned int,int),&ImGuiStorage::GetIntRef >;
// from imgui/imgui.h:2710:25
	makeExtern<DAS_CALL_METHOD(_method_49), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetIntRef","das_call_member< int * (ImGuiStorage::*)(unsigned int,int) , &ImGuiStorage::GetIntRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_50 = das::das_call_member< bool * (ImGuiStorage::*)(unsigned int,bool),&ImGuiStorage::GetBoolRef >;
// from imgui/imgui.h:2711:25
	makeExtern<DAS_CALL_METHOD(_method_50), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBoolRef","das_call_member< bool * (ImGuiStorage::*)(unsigned int,bool) , &ImGuiStorage::GetBoolRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_51 = das::das_call_member< float * (ImGuiStorage::*)(unsigned int,float),&ImGuiStorage::GetFloatRef >;
// from imgui/imgui.h:2712:25
	makeExtern<DAS_CALL_METHOD(_method_51), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFloatRef","das_call_member< float * (ImGuiStorage::*)(unsigned int,float) , &ImGuiStorage::GetFloatRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_52 = das::das_call_member< void ** (ImGuiStorage::*)(unsigned int,void *),&ImGuiStorage::GetVoidPtrRef >;
// from imgui/imgui.h:2713:25
	makeExtern<DAS_CALL_METHOD(_method_52), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVoidPtrRef","das_call_member< void ** (ImGuiStorage::*)(unsigned int,void *) , &ImGuiStorage::GetVoidPtrRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_53 = das::das_call_member< void (ImGuiStorage::*)(),&ImGuiStorage::BuildSortByKey >;
// from imgui/imgui.h:2716:25
	makeExtern<DAS_CALL_METHOD(_method_53), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildSortByKey","das_call_member< void (ImGuiStorage::*)() , &ImGuiStorage::BuildSortByKey >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_54 = das::das_call_member< void (ImGuiStorage::*)(int),&ImGuiStorage::SetAllInt >;
// from imgui/imgui.h:2718:25
	makeExtern<DAS_CALL_METHOD(_method_54), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetAllInt","das_call_member< void (ImGuiStorage::*)(int) , &ImGuiStorage::SetAllInt >::invoke")
		->args({"self","val"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiListClipper>(*this,lib,"ImGuiListClipper","ImGuiListClipper");
	using _method_55 = das::das_call_member< void (ImGuiListClipper::*)(int,float),&ImGuiListClipper::Begin >;
// from imgui/imgui.h:2760:21
	makeExtern<DAS_CALL_METHOD(_method_55), SimNode_ExtFuncCall , imguiTempFn>(lib,"Begin","das_call_member< void (ImGuiListClipper::*)(int,float) , &ImGuiListClipper::Begin >::invoke")
		->args({"self","items_count","items_height"})
		->arg_init(2,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_56 = das::das_call_member< void (ImGuiListClipper::*)(),&ImGuiListClipper::End >;
// from imgui/imgui.h:2761:21
	makeExtern<DAS_CALL_METHOD(_method_56), SimNode_ExtFuncCall , imguiTempFn>(lib,"End","das_call_member< void (ImGuiListClipper::*)() , &ImGuiListClipper::End >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_57 = das::das_call_member< bool (ImGuiListClipper::*)(),&ImGuiListClipper::Step >;
// from imgui/imgui.h:2762:21
	makeExtern<DAS_CALL_METHOD(_method_57), SimNode_ExtFuncCall , imguiTempFn>(lib,"Step","das_call_member< bool (ImGuiListClipper::*)() , &ImGuiListClipper::Step >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_58 = das::das_call_member< void (ImGuiListClipper::*)(int),&ImGuiListClipper::IncludeItemByIndex >;
// from imgui/imgui.h:2766:21
	makeExtern<DAS_CALL_METHOD(_method_58), SimNode_ExtFuncCall , imguiTempFn>(lib,"IncludeItemByIndex","das_call_member< void (ImGuiListClipper::*)(int) , &ImGuiListClipper::IncludeItemByIndex >::invoke")
		->args({"self","item_index"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

