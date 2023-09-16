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
void Module_dasIMGUI::initFunctions_20() {
	using _method_35 = das::das_call_member< void * (ImGuiStorage::*)(unsigned int) const,&ImGuiStorage::GetVoidPtr >;
	makeExtern<DAS_CALL_METHOD(_method_35), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVoidPtr","das_call_member< void * (ImGuiStorage::*)(unsigned int) const , &ImGuiStorage::GetVoidPtr >::invoke")
		->args({"self","key"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_36 = das::das_call_member< void (ImGuiStorage::*)(unsigned int,void *),&ImGuiStorage::SetVoidPtr >;
	makeExtern<DAS_CALL_METHOD(_method_36), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetVoidPtr","das_call_member< void (ImGuiStorage::*)(unsigned int,void *) , &ImGuiStorage::SetVoidPtr >::invoke")
		->args({"self","key","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_37 = das::das_call_member< int * (ImGuiStorage::*)(unsigned int,int),&ImGuiStorage::GetIntRef >;
	makeExtern<DAS_CALL_METHOD(_method_37), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetIntRef","das_call_member< int * (ImGuiStorage::*)(unsigned int,int) , &ImGuiStorage::GetIntRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_38 = das::das_call_member< bool * (ImGuiStorage::*)(unsigned int,bool),&ImGuiStorage::GetBoolRef >;
	makeExtern<DAS_CALL_METHOD(_method_38), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBoolRef","das_call_member< bool * (ImGuiStorage::*)(unsigned int,bool) , &ImGuiStorage::GetBoolRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_39 = das::das_call_member< float * (ImGuiStorage::*)(unsigned int,float),&ImGuiStorage::GetFloatRef >;
	makeExtern<DAS_CALL_METHOD(_method_39), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFloatRef","das_call_member< float * (ImGuiStorage::*)(unsigned int,float) , &ImGuiStorage::GetFloatRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_40 = das::das_call_member< void ** (ImGuiStorage::*)(unsigned int,void *),&ImGuiStorage::GetVoidPtrRef >;
	makeExtern<DAS_CALL_METHOD(_method_40), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVoidPtrRef","das_call_member< void ** (ImGuiStorage::*)(unsigned int,void *) , &ImGuiStorage::GetVoidPtrRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_41 = das::das_call_member< void (ImGuiStorage::*)(int),&ImGuiStorage::SetAllInt >;
	makeExtern<DAS_CALL_METHOD(_method_41), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetAllInt","das_call_member< void (ImGuiStorage::*)(int) , &ImGuiStorage::SetAllInt >::invoke")
		->args({"self","val"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_42 = das::das_call_member< void (ImGuiStorage::*)(),&ImGuiStorage::BuildSortByKey >;
	makeExtern<DAS_CALL_METHOD(_method_42), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildSortByKey","das_call_member< void (ImGuiStorage::*)() , &ImGuiStorage::BuildSortByKey >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiListClipper>(*this,lib,"ImGuiListClipper","ImGuiListClipper");
	using _method_43 = das::das_call_member< void (ImGuiListClipper::*)(int,float),&ImGuiListClipper::Begin >;
	makeExtern<DAS_CALL_METHOD(_method_43), SimNode_ExtFuncCall , imguiTempFn>(lib,"Begin","das_call_member< void (ImGuiListClipper::*)(int,float) , &ImGuiListClipper::Begin >::invoke")
		->args({"self","items_count","items_height"})
		->arg_init(2,make_smart<ExprConstFloat>(-1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_44 = das::das_call_member< void (ImGuiListClipper::*)(),&ImGuiListClipper::End >;
	makeExtern<DAS_CALL_METHOD(_method_44), SimNode_ExtFuncCall , imguiTempFn>(lib,"End","das_call_member< void (ImGuiListClipper::*)() , &ImGuiListClipper::End >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_45 = das::das_call_member< bool (ImGuiListClipper::*)(),&ImGuiListClipper::Step >;
	makeExtern<DAS_CALL_METHOD(_method_45), SimNode_ExtFuncCall , imguiTempFn>(lib,"Step","das_call_member< bool (ImGuiListClipper::*)() , &ImGuiListClipper::Step >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawCmd>(*this,lib,"ImDrawCmd","ImDrawCmd");
	addCtorAndUsing<ImDrawListSplitter>(*this,lib,"ImDrawListSplitter","ImDrawListSplitter");
	using _method_46 = das::das_call_member< void (ImDrawListSplitter::*)(),&ImDrawListSplitter::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_46), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImDrawListSplitter::*)() , &ImDrawListSplitter::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_47 = das::das_call_member< void (ImDrawListSplitter::*)(),&ImDrawListSplitter::ClearFreeMemory >;
	makeExtern<DAS_CALL_METHOD(_method_47), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearFreeMemory","das_call_member< void (ImDrawListSplitter::*)() , &ImDrawListSplitter::ClearFreeMemory >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_48 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int),&ImDrawListSplitter::Split >;
	makeExtern<DAS_CALL_METHOD(_method_48), SimNode_ExtFuncCall , imguiTempFn>(lib,"Split","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int) , &ImDrawListSplitter::Split >::invoke")
		->args({"self","draw_list","count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_49 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *),&ImDrawListSplitter::Merge >;
	makeExtern<DAS_CALL_METHOD(_method_49), SimNode_ExtFuncCall , imguiTempFn>(lib,"Merge","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *) , &ImDrawListSplitter::Merge >::invoke")
		->args({"self","draw_list"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_50 = das::das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int),&ImDrawListSplitter::SetCurrentChannel >;
	makeExtern<DAS_CALL_METHOD(_method_50), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCurrentChannel","das_call_member< void (ImDrawListSplitter::*)(ImDrawList *,int) , &ImDrawListSplitter::SetCurrentChannel >::invoke")
		->args({"self","draw_list","channel_idx"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawList,const ImDrawListSharedData *>(*this,lib,"ImDrawList","ImDrawList")
		->args({"shared_data"});
}
}

