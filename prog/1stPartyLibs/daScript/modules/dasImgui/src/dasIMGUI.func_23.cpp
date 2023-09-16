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
void Module_dasIMGUI::initFunctions_23() {
	using _method_91 = das::das_call_member< void (ImDrawList::*)(int),&ImDrawList::ChannelsSetCurrent >;
	makeExtern<DAS_CALL_METHOD(_method_91), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsSetCurrent","das_call_member< void (ImDrawList::*)(int) , &ImDrawList::ChannelsSetCurrent >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_92 = das::das_call_member< void (ImDrawList::*)(int,int),&ImDrawList::PrimReserve >;
	makeExtern<DAS_CALL_METHOD(_method_92), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimReserve","das_call_member< void (ImDrawList::*)(int,int) , &ImDrawList::PrimReserve >::invoke")
		->args({"self","idx_count","vtx_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_93 = das::das_call_member< void (ImDrawList::*)(int,int),&ImDrawList::PrimUnreserve >;
	makeExtern<DAS_CALL_METHOD(_method_93), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimUnreserve","das_call_member< void (ImDrawList::*)(int,int) , &ImDrawList::PrimUnreserve >::invoke")
		->args({"self","idx_count","vtx_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_94 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimRect >;
	makeExtern<DAS_CALL_METHOD(_method_94), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimRect >::invoke")
		->args({"self","a","b","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_95 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimRectUV >;
	makeExtern<DAS_CALL_METHOD(_method_95), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimRectUV","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimRectUV >::invoke")
		->args({"self","a","b","uv_a","uv_b","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_96 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimQuadUV >;
	makeExtern<DAS_CALL_METHOD(_method_96), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimQuadUV","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimQuadUV >::invoke")
		->args({"self","a","b","c","d","uv_a","uv_b","uv_c","uv_d","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_97 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimWriteVtx >;
	makeExtern<DAS_CALL_METHOD(_method_97), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimWriteVtx","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimWriteVtx >::invoke")
		->args({"self","pos","uv","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_98 = das::das_call_member< void (ImDrawList::*)(ImDrawIdx),&ImDrawList::PrimWriteIdx >;
	makeExtern<DAS_CALL_METHOD(_method_98), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimWriteIdx","das_call_member< void (ImDrawList::*)(unsigned int) , &ImDrawList::PrimWriteIdx >::invoke")
		->args({"self","idx"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_99 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimVtx >;
	makeExtern<DAS_CALL_METHOD(_method_99), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimVtx","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimVtx >::invoke")
		->args({"self","pos","uv","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_100 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_ResetForNewFrame >;
	makeExtern<DAS_CALL_METHOD(_method_100), SimNode_ExtFuncCall , imguiTempFn>(lib,"_ResetForNewFrame","das_call_member< void (ImDrawList::*)() , &ImDrawList::_ResetForNewFrame >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_101 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_ClearFreeMemory >;
	makeExtern<DAS_CALL_METHOD(_method_101), SimNode_ExtFuncCall , imguiTempFn>(lib,"_ClearFreeMemory","das_call_member< void (ImDrawList::*)() , &ImDrawList::_ClearFreeMemory >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_102 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_PopUnusedDrawCmd >;
	makeExtern<DAS_CALL_METHOD(_method_102), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PopUnusedDrawCmd","das_call_member< void (ImDrawList::*)() , &ImDrawList::_PopUnusedDrawCmd >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_103 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedClipRect >;
	makeExtern<DAS_CALL_METHOD(_method_103), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedClipRect","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedClipRect >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_104 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedTextureID >;
	makeExtern<DAS_CALL_METHOD(_method_104), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedTextureID","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedTextureID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_105 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedVtxOffset >;
	makeExtern<DAS_CALL_METHOD(_method_105), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedVtxOffset","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedVtxOffset >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_106 = das::das_call_member< int (ImDrawList::*)(float) const,&ImDrawList::_CalcCircleAutoSegmentCount >;
	makeExtern<DAS_CALL_METHOD(_method_106), SimNode_ExtFuncCall , imguiTempFn>(lib,"_CalcCircleAutoSegmentCount","das_call_member< int (ImDrawList::*)(float) const , &ImDrawList::_CalcCircleAutoSegmentCount >::invoke")
		->args({"self","radius"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_107 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int,int),&ImDrawList::_PathArcToFastEx >;
	makeExtern<DAS_CALL_METHOD(_method_107), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PathArcToFastEx","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int,int) , &ImDrawList::_PathArcToFastEx >::invoke")
		->args({"self","center","radius","a_min_sample","a_max_sample","a_step"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_108 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int),&ImDrawList::_PathArcToN >;
	makeExtern<DAS_CALL_METHOD(_method_108), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PathArcToN","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int) , &ImDrawList::_PathArcToN >::invoke")
		->args({"self","center","radius","a_min","a_max","num_segments"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawData>(*this,lib,"ImDrawData","ImDrawData");
	using _method_109 = das::das_call_member< void (ImDrawData::*)(),&ImDrawData::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_109), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImDrawData::*)() , &ImDrawData::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

