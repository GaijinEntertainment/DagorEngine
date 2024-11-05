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
void Module_dasIMGUI::initFunctions_29() {
	using _method_169 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesJapanese >;
// from imgui/imgui.h:3435:33
	makeExtern<DAS_CALL_METHOD(_method_169), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesJapanese","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesJapanese >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_170 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesChineseFull >;
// from imgui/imgui.h:3436:33
	makeExtern<DAS_CALL_METHOD(_method_170), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesChineseFull","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesChineseFull >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_171 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon >;
// from imgui/imgui.h:3437:33
	makeExtern<DAS_CALL_METHOD(_method_171), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesChineseSimplifiedCommon","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_172 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesCyrillic >;
// from imgui/imgui.h:3438:33
	makeExtern<DAS_CALL_METHOD(_method_172), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesCyrillic","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesCyrillic >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_173 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesThai >;
// from imgui/imgui.h:3439:33
	makeExtern<DAS_CALL_METHOD(_method_173), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesThai","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesThai >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_174 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesVietnamese >;
// from imgui/imgui.h:3440:33
	makeExtern<DAS_CALL_METHOD(_method_174), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesVietnamese","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesVietnamese >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_175 = das::das_call_member< int (ImFontAtlas::*)(int,int),&ImFontAtlas::AddCustomRectRegular >;
// from imgui/imgui.h:3453:33
	makeExtern<DAS_CALL_METHOD(_method_175), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCustomRectRegular","das_call_member< int (ImFontAtlas::*)(int,int) , &ImFontAtlas::AddCustomRectRegular >::invoke")
		->args({"self","width","height"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_176 = das::das_call_member< int (ImFontAtlas::*)(ImFont *,unsigned short,int,int,float,const ImVec2 &),&ImFontAtlas::AddCustomRectFontGlyph >;
// from imgui/imgui.h:3454:33
	makeExtern<DAS_CALL_METHOD(_method_176), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCustomRectFontGlyph","das_call_member< int (ImFontAtlas::*)(ImFont *,unsigned short,int,int,float,const ImVec2 &) , &ImFontAtlas::AddCustomRectFontGlyph >::invoke")
		->args({"self","font","id","width","height","advance_x","offset"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_177 = das::das_call_member< ImFontAtlasCustomRect * (ImFontAtlas::*)(int),&ImFontAtlas::GetCustomRectByIndex >;
// from imgui/imgui.h:3455:33
	makeExtern<DAS_CALL_METHOD(_method_177), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCustomRectByIndex","das_call_member< ImFontAtlasCustomRect * (ImFontAtlas::*)(int) , &ImFontAtlas::GetCustomRectByIndex >::invoke")
		->args({"self","index"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_178 = das::das_call_member< void (ImFontAtlas::*)(const ImFontAtlasCustomRect *,ImVec2 *,ImVec2 *) const,&ImFontAtlas::CalcCustomRectUV >;
// from imgui/imgui.h:3458:33
	makeExtern<DAS_CALL_METHOD(_method_178), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcCustomRectUV","das_call_member< void (ImFontAtlas::*)(const ImFontAtlasCustomRect *,ImVec2 *,ImVec2 *) const , &ImFontAtlas::CalcCustomRectUV >::invoke")
		->args({"self","rect","out_uv_min","out_uv_max"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_179 = das::das_call_member< bool (ImFontAtlas::*)(int,ImVec2 *,ImVec2 *,ImVec2[2],ImVec2[2]),&ImFontAtlas::GetMouseCursorTexData >;
// from imgui/imgui.h:3459:33
	makeExtern<DAS_CALL_METHOD(_method_179), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMouseCursorTexData","das_call_member< bool (ImFontAtlas::*)(int,ImVec2 *,ImVec2 *,ImVec2[2],ImVec2[2]) , &ImFontAtlas::GetMouseCursorTexData >::invoke")
		->args({"self","cursor","out_offset","out_size","out_uv_border","out_uv_fill"})
		->arg_type(1,makeType<ImGuiMouseCursor_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFont>(*this,lib,"ImFont","ImFont");
	using _method_180 = das::das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const,&ImFont::FindGlyph >;
// from imgui/imgui.h:3532:33
	makeExtern<DAS_CALL_METHOD(_method_180), SimNode_ExtFuncCall , imguiTempFn>(lib,"FindGlyph","das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const , &ImFont::FindGlyph >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_181 = das::das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const,&ImFont::FindGlyphNoFallback >;
// from imgui/imgui.h:3533:33
	makeExtern<DAS_CALL_METHOD(_method_181), SimNode_ExtFuncCall , imguiTempFn>(lib,"FindGlyphNoFallback","das_call_member< const ImFontGlyph * (ImFont::*)(unsigned short) const , &ImFont::FindGlyphNoFallback >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_182 = das::das_call_member< float (ImFont::*)(unsigned short) const,&ImFont::GetCharAdvance >;
// from imgui/imgui.h:3534:33
	makeExtern<DAS_CALL_METHOD(_method_182), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCharAdvance","das_call_member< float (ImFont::*)(unsigned short) const , &ImFont::GetCharAdvance >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_183 = das::das_call_member< bool (ImFont::*)() const,&ImFont::IsLoaded >;
// from imgui/imgui.h:3535:33
	makeExtern<DAS_CALL_METHOD(_method_183), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsLoaded","das_call_member< bool (ImFont::*)() const , &ImFont::IsLoaded >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_184 = das::das_call_member< const char * (ImFont::*)() const,&ImFont::GetDebugName >;
// from imgui/imgui.h:3536:33
	makeExtern<DAS_CALL_METHOD(_method_184), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDebugName","das_call_member< const char * (ImFont::*)() const , &ImFont::GetDebugName >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_185 = das::das_call_member< ImVec2 (ImFont::*)(float,float,float,const char *,const char *,const char **) const,&ImFont::CalcTextSizeA >;
// from imgui/imgui.h:3540:33
	makeExtern<DAS_CALL_METHOD(_method_185), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcTextSizeA","das_call_member< ImVec2 (ImFont::*)(float,float,float,const char *,const char *,const char **) const , &ImFont::CalcTextSizeA >::invoke")
		->args({"self","size","max_width","wrap_width","text_begin","text_end","remaining"})
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_init(6,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_186 = das::das_call_member< const char * (ImFont::*)(float,const char *,const char *,float) const,&ImFont::CalcWordWrapPositionA >;
// from imgui/imgui.h:3541:33
	makeExtern<DAS_CALL_METHOD(_method_186), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcWordWrapPositionA","das_call_member< const char * (ImFont::*)(float,const char *,const char *,float) const , &ImFont::CalcWordWrapPositionA >::invoke")
		->args({"self","scale","text","text_end","wrap_width"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_187 = das::das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,unsigned short) const,&ImFont::RenderChar >;
// from imgui/imgui.h:3542:33
	makeExtern<DAS_CALL_METHOD(_method_187), SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderChar","das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,unsigned short) const , &ImFont::RenderChar >::invoke")
		->args({"self","draw_list","size","pos","col","c"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

