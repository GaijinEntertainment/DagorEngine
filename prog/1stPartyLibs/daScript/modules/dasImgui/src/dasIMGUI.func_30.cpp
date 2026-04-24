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
	using _method_179 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearTexData >;
// from imgui/imgui.h:3807:33
	makeExtern<DAS_CALL_METHOD(_method_179), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearTexData","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearTexData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_180 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesDefault >;
// from imgui/imgui.h:3831:33
	makeExtern<DAS_CALL_METHOD(_method_180), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesDefault","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesDefault >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_181 = das::das_call_member< int (ImFontAtlas::*)(int,int,ImFontAtlasRect *),&ImFontAtlas::AddCustomRect >;
// from imgui/imgui.h:3868:33
	makeExtern<DAS_CALL_METHOD(_method_181), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCustomRect","das_call_member< int (ImFontAtlas::*)(int,int,ImFontAtlasRect *) , &ImFontAtlas::AddCustomRect >::invoke")
		->args({"self","width","height","out_r"})
		->arg_init(3,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_182 = das::das_call_member< void (ImFontAtlas::*)(int),&ImFontAtlas::RemoveCustomRect >;
// from imgui/imgui.h:3869:33
	makeExtern<DAS_CALL_METHOD(_method_182), SimNode_ExtFuncCall , imguiTempFn>(lib,"RemoveCustomRect","das_call_member< void (ImFontAtlas::*)(int) , &ImFontAtlas::RemoveCustomRect >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_183 = das::das_call_member< bool (ImFontAtlas::*)(int,ImFontAtlasRect *) const,&ImFontAtlas::GetCustomRect >;
// from imgui/imgui.h:3870:33
	makeExtern<DAS_CALL_METHOD(_method_183), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCustomRect","das_call_member< bool (ImFontAtlas::*)(int,ImFontAtlasRect *) const , &ImFontAtlas::GetCustomRect >::invoke")
		->args({"self","id","out_r"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontBaked>(*this,lib,"ImFontBaked","ImFontBaked");
	using _method_184 = das::das_call_member< void (ImFontBaked::*)(),&ImFontBaked::ClearOutputData >;
// from imgui/imgui.h:3963:33
	makeExtern<DAS_CALL_METHOD(_method_184), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearOutputData","das_call_member< void (ImFontBaked::*)() , &ImFontBaked::ClearOutputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_185 = das::das_call_member< ImFontGlyph * (ImFontBaked::*)(unsigned short),&ImFontBaked::FindGlyph >;
// from imgui/imgui.h:3964:33
	makeExtern<DAS_CALL_METHOD(_method_185), SimNode_ExtFuncCall , imguiTempFn>(lib,"FindGlyph","das_call_member< ImFontGlyph * (ImFontBaked::*)(unsigned short) , &ImFontBaked::FindGlyph >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_186 = das::das_call_member< ImFontGlyph * (ImFontBaked::*)(unsigned short),&ImFontBaked::FindGlyphNoFallback >;
// from imgui/imgui.h:3965:33
	makeExtern<DAS_CALL_METHOD(_method_186), SimNode_ExtFuncCall , imguiTempFn>(lib,"FindGlyphNoFallback","das_call_member< ImFontGlyph * (ImFontBaked::*)(unsigned short) , &ImFontBaked::FindGlyphNoFallback >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_187 = das::das_call_member< float (ImFontBaked::*)(unsigned short),&ImFontBaked::GetCharAdvance >;
// from imgui/imgui.h:3966:33
	makeExtern<DAS_CALL_METHOD(_method_187), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCharAdvance","das_call_member< float (ImFontBaked::*)(unsigned short) , &ImFontBaked::GetCharAdvance >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_188 = das::das_call_member< bool (ImFontBaked::*)(unsigned short),&ImFontBaked::IsGlyphLoaded >;
// from imgui/imgui.h:3967:33
	makeExtern<DAS_CALL_METHOD(_method_188), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsGlyphLoaded","das_call_member< bool (ImFontBaked::*)(unsigned short) , &ImFontBaked::IsGlyphLoaded >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFont>(*this,lib,"ImFont","ImFont");
	using _method_189 = das::das_call_member< bool (ImFont::*)(unsigned short),&ImFont::IsGlyphInFont >;
// from imgui/imgui.h:4010:33
	makeExtern<DAS_CALL_METHOD(_method_189), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsGlyphInFont","das_call_member< bool (ImFont::*)(unsigned short) , &ImFont::IsGlyphInFont >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_190 = das::das_call_member< bool (ImFont::*)() const,&ImFont::IsLoaded >;
// from imgui/imgui.h:4011:33
	makeExtern<DAS_CALL_METHOD(_method_190), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsLoaded","das_call_member< bool (ImFont::*)() const , &ImFont::IsLoaded >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_191 = das::das_call_member< const char * (ImFont::*)() const,&ImFont::GetDebugName >;
// from imgui/imgui.h:4012:33
	makeExtern<DAS_CALL_METHOD(_method_191), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDebugName","das_call_member< const char * (ImFont::*)() const , &ImFont::GetDebugName >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_192 = das::das_call_member< ImFontBaked * (ImFont::*)(float,float),&ImFont::GetFontBaked >;
// from imgui/imgui.h:4017:33
	makeExtern<DAS_CALL_METHOD(_method_192), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontBaked","das_call_member< ImFontBaked * (ImFont::*)(float,float) , &ImFont::GetFontBaked >::invoke")
		->args({"self","font_size","density"})
		->arg_init(2,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_193 = das::das_call_member< ImVec2 (ImFont::*)(float,float,float,const char *,const char *,const char **),&ImFont::CalcTextSizeA >;
// from imgui/imgui.h:4018:33
	makeExtern<DAS_CALL_METHOD(_method_193), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcTextSizeA","das_call_member< ImVec2 (ImFont::*)(float,float,float,const char *,const char *,const char **) , &ImFont::CalcTextSizeA >::invoke")
		->args({"self","size","max_width","wrap_width","text_begin","text_end","out_remaining"})
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_init(6,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_194 = das::das_call_member< const char * (ImFont::*)(float,const char *,const char *,float),&ImFont::CalcWordWrapPosition >;
// from imgui/imgui.h:4019:33
	makeExtern<DAS_CALL_METHOD(_method_194), SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcWordWrapPosition","das_call_member< const char * (ImFont::*)(float,const char *,const char *,float) , &ImFont::CalcWordWrapPosition >::invoke")
		->args({"self","size","text","text_end","wrap_width"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_195 = das::das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,unsigned short,const ImVec4 *),&ImFont::RenderChar >;
// from imgui/imgui.h:4020:33
	makeExtern<DAS_CALL_METHOD(_method_195), SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderChar","das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,unsigned short,const ImVec4 *) , &ImFont::RenderChar >::invoke")
		->args({"self","draw_list","size","pos","col","c","cpu_fine_clip"})
		->arg_init(6,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_196 = das::das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,const ImVec4 &,const char *,const char *,float,int),&ImFont::RenderText >;
// from imgui/imgui.h:4021:33
	makeExtern<DAS_CALL_METHOD(_method_196), SimNode_ExtFuncCall , imguiTempFn>(lib,"RenderText","das_call_member< void (ImFont::*)(ImDrawList *,float,const ImVec2 &,unsigned int,const ImVec4 &,const char *,const char *,float,int) , &ImFont::RenderText >::invoke")
		->args({"self","draw_list","size","pos","col","clip_rect","text_begin","text_end","wrap_width","flags"})
		->arg_init(8,make_smart<ExprConstFloat>(0))
		->arg_init(9,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
}
}

