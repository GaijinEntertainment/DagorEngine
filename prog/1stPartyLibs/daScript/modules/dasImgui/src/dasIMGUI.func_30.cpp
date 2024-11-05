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
void Module_dasIMGUI::initFunctions_30() {
	using _method_188 = das::das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,const ImVec4 &,const char *,const char *,float,bool) const,&ImFont::RenderText >;
// from imgui/imgui.h:3543:33
	makeExtern<DAS_CALL_METHOD(_method_188), SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderText","das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,const ImVec4 &,const char *,const char *,float,bool) const , &ImFont::RenderText >::invoke")
		->args({"self","draw_list","size","pos","col","clip_rect","text_begin","text_end","wrap_width","cpu_fine_clip"})
		->arg_init(8,make_smart<ExprConstFloat>(0))
		->arg_init(9,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_189 = das::das_call_member< void (ImFont::*)(),&ImFont::BuildLookupTable >;
// from imgui/imgui.h:3546:33
	makeExtern<DAS_CALL_METHOD(_method_189), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildLookupTable","das_call_member< void (ImFont::*)() , &ImFont::BuildLookupTable >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_190 = das::das_call_member< void (ImFont::*)(),&ImFont::ClearOutputData >;
// from imgui/imgui.h:3547:33
	makeExtern<DAS_CALL_METHOD(_method_190), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearOutputData","das_call_member< void (ImFont::*)() , &ImFont::ClearOutputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_191 = das::das_call_member< void (ImFont::*)(int),&ImFont::GrowIndex >;
// from imgui/imgui.h:3548:33
	makeExtern<DAS_CALL_METHOD(_method_191), SimNode_ExtFuncCall , imguiTempFn>(lib,"GrowIndex","das_call_member< void (ImFont::*)(int) , &ImFont::GrowIndex >::invoke")
		->args({"self","new_size"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_192 = das::das_call_member< void (ImFont::*)(const ImFontConfig *,unsigned short,float,float,float,float,float,float,float,float,float),&ImFont::AddGlyph >;
// from imgui/imgui.h:3549:33
	makeExtern<DAS_CALL_METHOD(_method_192), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddGlyph","das_call_member< void (ImFont::*)(const ImFontConfig *,unsigned short,float,float,float,float,float,float,float,float,float) , &ImFont::AddGlyph >::invoke")
		->args({"self","src_cfg","c","x0","y0","x1","y1","u0","v0","u1","v1","advance_x"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_193 = das::das_call_member< void (ImFont::*)(unsigned short,unsigned short,bool),&ImFont::AddRemapChar >;
// from imgui/imgui.h:3550:33
	makeExtern<DAS_CALL_METHOD(_method_193), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRemapChar","das_call_member< void (ImFont::*)(unsigned short,unsigned short,bool) , &ImFont::AddRemapChar >::invoke")
		->args({"self","dst","src","overwrite_dst"})
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_194 = das::das_call_member< void (ImFont::*)(unsigned short,bool),&ImFont::SetGlyphVisible >;
// from imgui/imgui.h:3551:33
	makeExtern<DAS_CALL_METHOD(_method_194), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetGlyphVisible","das_call_member< void (ImFont::*)(unsigned short,bool) , &ImFont::SetGlyphVisible >::invoke")
		->args({"self","c","visible"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_195 = das::das_call_member< bool (ImFont::*)(unsigned int,unsigned int),&ImFont::IsGlyphRangeUnused >;
// from imgui/imgui.h:3552:33
	makeExtern<DAS_CALL_METHOD(_method_195), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsGlyphRangeUnused","das_call_member< bool (ImFont::*)(unsigned int,unsigned int) , &ImFont::IsGlyphRangeUnused >::invoke")
		->args({"self","c_begin","c_last"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiViewport>(*this,lib,"ImGuiViewport","ImGuiViewport");
	using _method_196 = das::das_call_member< ImVec2 (ImGuiViewport::*)() const,&ImGuiViewport::GetCenter >;
// from imgui/imgui.h:3618:25
	makeExtern<DAS_CALL_METHOD(_method_196), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCenter","das_call_member< ImVec2 (ImGuiViewport::*)() const , &ImGuiViewport::GetCenter >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_197 = das::das_call_member< ImVec2 (ImGuiViewport::*)() const,&ImGuiViewport::GetWorkCenter >;
// from imgui/imgui.h:3619:25
	makeExtern<DAS_CALL_METHOD(_method_197), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWorkCenter","das_call_member< ImVec2 (ImGuiViewport::*)() const , &ImGuiViewport::GetWorkCenter >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImGuiPlatformIO>(*this,lib,"ImGuiPlatformIO","ImGuiPlatformIO");
	addCtorAndUsing<ImGuiPlatformMonitor>(*this,lib,"ImGuiPlatformMonitor","ImGuiPlatformMonitor");
	addCtorAndUsing<ImGuiPlatformImeData>(*this,lib,"ImGuiPlatformImeData","ImGuiPlatformImeData");
}
}

