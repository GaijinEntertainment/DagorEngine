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
void Module_dasIMGUI::initFunctions_26() {
	using _method_146 = das::das_call_member< void (ImFontAtlas::*)(const ImFontAtlasCustomRect *,ImVec2 *,ImVec2 *) const,&ImFontAtlas::CalcCustomRectUV >;
	makeExtern<DAS_CALL_METHOD(_method_146), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcCustomRectUV","das_call_member< void (ImFontAtlas::*)(const ImFontAtlasCustomRect *,ImVec2 *,ImVec2 *) const , &ImFontAtlas::CalcCustomRectUV >::invoke")
		->args({"self","rect","out_uv_min","out_uv_max"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_147 = das::das_call_member< bool (ImFontAtlas::*)(int,ImVec2 *,ImVec2 *,ImVec2[2],ImVec2[2]),&ImFontAtlas::GetMouseCursorTexData >;
	makeExtern<DAS_CALL_METHOD(_method_147), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseCursorTexData","das_call_member< bool (ImFontAtlas::*)(int,ImVec2 *,ImVec2 *,ImVec2[2],ImVec2[2]) , &ImFontAtlas::GetMouseCursorTexData >::invoke")
		->args({"self","cursor","out_offset","out_size","out_uv_border","out_uv_fill"})
		->arg_type(1,makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFont>(*this,lib,"ImFont","ImFont");
	using _method_148 = das::das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const,&ImFont::FindGlyph >;
	makeExtern<DAS_CALL_METHOD(_method_148), SimNode_ExtFuncCall , imguiTempFn>(lib,"FindGlyph","das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const , &ImFont::FindGlyph >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_149 = das::das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const,&ImFont::FindGlyphNoFallback >;
	makeExtern<DAS_CALL_METHOD(_method_149), SimNode_ExtFuncCall , imguiTempFn>(lib,"FindGlyphNoFallback","das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const , &ImFont::FindGlyphNoFallback >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_150 = das::das_call_member< float (ImFont::*)(unsigned short) const,&ImFont::GetCharAdvance >;
	makeExtern<DAS_CALL_METHOD(_method_150), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCharAdvance","das_call_member< float (ImFont::*)(unsigned short) const , &ImFont::GetCharAdvance >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_151 = das::das_call_member< bool (ImFont::*)() const,&ImFont::IsLoaded >;
	makeExtern<DAS_CALL_METHOD(_method_151), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsLoaded","das_call_member< bool (ImFont::*)() const , &ImFont::IsLoaded >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_152 = das::das_call_member< const char * (ImFont::*)() const,&ImFont::GetDebugName >;
	makeExtern<DAS_CALL_METHOD(_method_152), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDebugName","das_call_member< const char * (ImFont::*)() const , &ImFont::GetDebugName >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_153 = das::das_call_member< ImVec2 (ImFont::*)(float,float,float,const char *,const char *,const char **) const,&ImFont::CalcTextSizeA >;
	makeExtern<DAS_CALL_METHOD(_method_153), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcTextSizeA","das_call_member< ImVec2 (ImFont::*)(float,float,float,const char *,const char *,const char **) const , &ImFont::CalcTextSizeA >::invoke")
		->args({"self","size","max_width","wrap_width","text_begin","text_end","remaining"})
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_init(6,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_154 = das::das_call_member< const char * (ImFont::*)(float,const char *,const char *,float) const,&ImFont::CalcWordWrapPositionA >;
	makeExtern<DAS_CALL_METHOD(_method_154), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcWordWrapPositionA","das_call_member< const char * (ImFont::*)(float,const char *,const char *,float) const , &ImFont::CalcWordWrapPositionA >::invoke")
		->args({"self","scale","text","text_end","wrap_width"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_155 = das::das_call_member< void (ImFont::*)(ImDrawList *,float,ImVec2,unsigned int,unsigned short) const,&ImFont::RenderChar >;
	makeExtern<DAS_CALL_METHOD(_method_155), SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderChar","das_call_member< void (ImFont::*)(ImDrawList *,float,ImVec2,unsigned int,unsigned short) const , &ImFont::RenderChar >::invoke")
		->args({"self","draw_list","size","pos","col","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_156 = das::das_call_member< void (ImFont::*)(ImDrawList *,float,ImVec2,unsigned int,const ImVec4 &,const char *,const char *,float,bool) const,&ImFont::RenderText >;
	makeExtern<DAS_CALL_METHOD(_method_156), SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderText","das_call_member< void (ImFont::*)(ImDrawList *,float,ImVec2,unsigned int,const ImVec4 &,const char *,const char *,float,bool) const , &ImFont::RenderText >::invoke")
		->args({"self","draw_list","size","pos","col","clip_rect","text_begin","text_end","wrap_width","cpu_fine_clip"})
		->arg_init(8,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(9,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_157 = das::das_call_member< void (ImFont::*)(),&ImFont::BuildLookupTable >;
	makeExtern<DAS_CALL_METHOD(_method_157), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildLookupTable","das_call_member< void (ImFont::*)() , &ImFont::BuildLookupTable >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_158 = das::das_call_member< void (ImFont::*)(),&ImFont::ClearOutputData >;
	makeExtern<DAS_CALL_METHOD(_method_158), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearOutputData","das_call_member< void (ImFont::*)() , &ImFont::ClearOutputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_159 = das::das_call_member< void (ImFont::*)(int),&ImFont::GrowIndex >;
	makeExtern<DAS_CALL_METHOD(_method_159), SimNode_ExtFuncCall , imguiTempFn>(lib,"GrowIndex","das_call_member< void (ImFont::*)(int) , &ImFont::GrowIndex >::invoke")
		->args({"self","new_size"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_160 = das::das_call_member< void (ImFont::*)(const ImFontConfig *,unsigned short,float,float,float,float,float,float,float,float,float),&ImFont::AddGlyph >;
	makeExtern<DAS_CALL_METHOD(_method_160), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddGlyph","das_call_member< void (ImFont::*)(const ImFontConfig *,unsigned short,float,float,float,float,float,float,float,float,float) , &ImFont::AddGlyph >::invoke")
		->args({"self","src_cfg","c","x0","y0","x1","y1","u0","v0","u1","v1","advance_x"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_161 = das::das_call_member< void (ImFont::*)(unsigned short,unsigned short,bool),&ImFont::AddRemapChar >;
	makeExtern<DAS_CALL_METHOD(_method_161), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRemapChar","das_call_member< void (ImFont::*)(unsigned short,unsigned short,bool) , &ImFont::AddRemapChar >::invoke")
		->args({"self","dst","src","overwrite_dst"})
		->arg_init(3,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_162 = das::das_call_member< void (ImFont::*)(unsigned short,bool),&ImFont::SetGlyphVisible >;
	makeExtern<DAS_CALL_METHOD(_method_162), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetGlyphVisible","das_call_member< void (ImFont::*)(unsigned short,bool) , &ImFont::SetGlyphVisible >::invoke")
		->args({"self","c","visible"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_163 = das::das_call_member< void (ImFont::*)(unsigned short),&ImFont::SetFallbackChar >;
	makeExtern<DAS_CALL_METHOD(_method_163), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetFallbackChar","das_call_member< void (ImFont::*)(unsigned short) , &ImFont::SetFallbackChar >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_164 = das::das_call_member< bool (ImFont::*)(unsigned int,unsigned int),&ImFont::IsGlyphRangeUnused >;
	makeExtern<DAS_CALL_METHOD(_method_164), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsGlyphRangeUnused","das_call_member< bool (ImFont::*)(unsigned int,unsigned int) , &ImFont::IsGlyphRangeUnused >::invoke")
		->args({"self","c_begin","c_last"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

