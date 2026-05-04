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
void Module_dasIMGUI::initFunctions_23() {
	using _method_52 = das::das_call_member< int * (ImGuiStorage::*)(unsigned int,int),&ImGuiStorage::GetIntRef >;
// from imgui/imgui.h:2946:25
	makeExtern<DAS_CALL_METHOD(_method_52), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetIntRef","das_call_member< int * (ImGuiStorage::*)(unsigned int,int) , &ImGuiStorage::GetIntRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_53 = das::das_call_member< bool * (ImGuiStorage::*)(unsigned int,bool),&ImGuiStorage::GetBoolRef >;
// from imgui/imgui.h:2947:25
	makeExtern<DAS_CALL_METHOD(_method_53), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBoolRef","das_call_member< bool * (ImGuiStorage::*)(unsigned int,bool) , &ImGuiStorage::GetBoolRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_54 = das::das_call_member< float * (ImGuiStorage::*)(unsigned int,float),&ImGuiStorage::GetFloatRef >;
// from imgui/imgui.h:2948:25
	makeExtern<DAS_CALL_METHOD(_method_54), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFloatRef","das_call_member< float * (ImGuiStorage::*)(unsigned int,float) , &ImGuiStorage::GetFloatRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_55 = das::das_call_member< void ** (ImGuiStorage::*)(unsigned int,void *),&ImGuiStorage::GetVoidPtrRef >;
// from imgui/imgui.h:2949:25
	makeExtern<DAS_CALL_METHOD(_method_55), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVoidPtrRef","das_call_member< void ** (ImGuiStorage::*)(unsigned int,void *) , &ImGuiStorage::GetVoidPtrRef >::invoke")
		->args({"self","key","default_val"})
		->arg_init(2,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_56 = das::das_call_member< void (ImGuiStorage::*)(),&ImGuiStorage::BuildSortByKey >;
// from imgui/imgui.h:2952:25
	makeExtern<DAS_CALL_METHOD(_method_56), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildSortByKey","das_call_member< void (ImGuiStorage::*)() , &ImGuiStorage::BuildSortByKey >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_57 = das::das_call_member< void (ImGuiStorage::*)(int),&ImGuiStorage::SetAllInt >;
// from imgui/imgui.h:2954:25
	makeExtern<DAS_CALL_METHOD(_method_57), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetAllInt","das_call_member< void (ImGuiStorage::*)(int) , &ImGuiStorage::SetAllInt >::invoke")
		->args({"self","val"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiListClipper>(*this,lib,"ImGuiListClipper","ImGuiListClipper");
	using _method_58 = das::das_call_member< void (ImGuiListClipper::*)(int,float),&ImGuiListClipper::Begin >;
// from imgui/imgui.h:3004:21
	makeExtern<DAS_CALL_METHOD(_method_58), SimNode_ExtFuncCall , imguiTempFn>(lib,"Begin","das_call_member< void (ImGuiListClipper::*)(int,float) , &ImGuiListClipper::Begin >::invoke")
		->args({"self","items_count","items_height"})
		->arg_init(2,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_59 = das::das_call_member< void (ImGuiListClipper::*)(),&ImGuiListClipper::End >;
// from imgui/imgui.h:3005:21
	makeExtern<DAS_CALL_METHOD(_method_59), SimNode_ExtFuncCall , imguiTempFn>(lib,"End","das_call_member< void (ImGuiListClipper::*)() , &ImGuiListClipper::End >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_60 = das::das_call_member< bool (ImGuiListClipper::*)(),&ImGuiListClipper::Step >;
// from imgui/imgui.h:3006:21
	makeExtern<DAS_CALL_METHOD(_method_60), SimNode_ExtFuncCall , imguiTempFn>(lib,"Step","das_call_member< bool (ImGuiListClipper::*)() , &ImGuiListClipper::Step >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_61 = das::das_call_member< void (ImGuiListClipper::*)(int),&ImGuiListClipper::IncludeItemByIndex >;
// from imgui/imgui.h:3010:21
	makeExtern<DAS_CALL_METHOD(_method_61), SimNode_ExtFuncCall , imguiTempFn>(lib,"IncludeItemByIndex","das_call_member< void (ImGuiListClipper::*)(int) , &ImGuiListClipper::IncludeItemByIndex >::invoke")
		->args({"self","item_index"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_62 = das::das_call_member< void (ImGuiListClipper::*)(int,int),&ImGuiListClipper::IncludeItemsByIndex >;
// from imgui/imgui.h:3011:21
	makeExtern<DAS_CALL_METHOD(_method_62), SimNode_ExtFuncCall , imguiTempFn>(lib,"IncludeItemsByIndex","das_call_member< void (ImGuiListClipper::*)(int,int) , &ImGuiListClipper::IncludeItemsByIndex >::invoke")
		->args({"self","item_begin","item_end"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_63 = das::das_call_member< void (ImGuiListClipper::*)(int),&ImGuiListClipper::SeekCursorForItem >;
// from imgui/imgui.h:3016:21
	makeExtern<DAS_CALL_METHOD(_method_63), SimNode_ExtFuncCall , imguiTempFn>(lib,"SeekCursorForItem","das_call_member< void (ImGuiListClipper::*)(int) , &ImGuiListClipper::SeekCursorForItem >::invoke")
		->args({"self","item_index"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiSelectionBasicStorage>(*this,lib,"ImGuiSelectionBasicStorage","ImGuiSelectionBasicStorage");
	using _method_64 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiMultiSelectIO *),&ImGuiSelectionBasicStorage::ApplyRequests >;
// from imgui/imgui.h:3233:21
	makeExtern<DAS_CALL_METHOD(_method_64), SimNode_ExtFuncCall , imguiTempFn>(lib,"ApplyRequests","das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiMultiSelectIO *) , &ImGuiSelectionBasicStorage::ApplyRequests >::invoke")
		->args({"self","ms_io"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_65 = das::das_call_member< bool (ImGuiSelectionBasicStorage::*)(unsigned int) const,&ImGuiSelectionBasicStorage::Contains >;
// from imgui/imgui.h:3234:21
	makeExtern<DAS_CALL_METHOD(_method_65), SimNode_ExtFuncCall , imguiTempFn>(lib,"Contains","das_call_member< bool (ImGuiSelectionBasicStorage::*)(unsigned int) const , &ImGuiSelectionBasicStorage::Contains >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_66 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(),&ImGuiSelectionBasicStorage::Clear >;
// from imgui/imgui.h:3235:21
	makeExtern<DAS_CALL_METHOD(_method_66), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImGuiSelectionBasicStorage::*)() , &ImGuiSelectionBasicStorage::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_67 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiSelectionBasicStorage &),&ImGuiSelectionBasicStorage::Swap >;
// from imgui/imgui.h:3236:21
	makeExtern<DAS_CALL_METHOD(_method_67), SimNode_ExtFuncCall , imguiTempFn>(lib,"Swap","das_call_member< void (ImGuiSelectionBasicStorage::*)(ImGuiSelectionBasicStorage &) , &ImGuiSelectionBasicStorage::Swap >::invoke")
		->args({"self","r"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_68 = das::das_call_member< void (ImGuiSelectionBasicStorage::*)(unsigned int,bool),&ImGuiSelectionBasicStorage::SetItemSelected >;
// from imgui/imgui.h:3237:21
	makeExtern<DAS_CALL_METHOD(_method_68), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemSelected","das_call_member< void (ImGuiSelectionBasicStorage::*)(unsigned int,bool) , &ImGuiSelectionBasicStorage::SetItemSelected >::invoke")
		->args({"self","id","selected"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_69 = das::das_call_member< bool (ImGuiSelectionBasicStorage::*)(void **,unsigned int *),&ImGuiSelectionBasicStorage::GetNextSelectedItem >;
// from imgui/imgui.h:3238:21
	makeExtern<DAS_CALL_METHOD(_method_69), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetNextSelectedItem","das_call_member< bool (ImGuiSelectionBasicStorage::*)(void **,unsigned int *) , &ImGuiSelectionBasicStorage::GetNextSelectedItem >::invoke")
		->args({"self","opaque_it","out_id"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

