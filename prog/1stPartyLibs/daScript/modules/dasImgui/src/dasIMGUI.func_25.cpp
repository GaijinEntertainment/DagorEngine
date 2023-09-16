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
void Module_dasIMGUI::initFunctions_25() {
	using _method_126 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearInputData >;
	makeExtern<DAS_CALL_METHOD(_method_126), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputData","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearInputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_127 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearTexData >;
	makeExtern<DAS_CALL_METHOD(_method_127), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearTexData","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearTexData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_128 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearFonts >;
	makeExtern<DAS_CALL_METHOD(_method_128), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearFonts","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearFonts >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_129 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_129), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_130 = das::das_call_member< bool (ImFontAtlas::*)(),&ImFontAtlas::Build >;
	makeExtern<DAS_CALL_METHOD(_method_130), SimNode_ExtFuncCall , imguiTempFn>(lib,"Build","das_call_member< bool (ImFontAtlas::*)() , &ImFontAtlas::Build >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_131 = das::das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *),&ImFontAtlas::GetTexDataAsAlpha8 >;
	makeExtern<DAS_CALL_METHOD(_method_131), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexDataAsAlpha8","das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *) , &ImFontAtlas::GetTexDataAsAlpha8 >::invoke")
		->args({"self","out_pixels","out_width","out_height","out_bytes_per_pixel"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_132 = das::das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *),&ImFontAtlas::GetTexDataAsRGBA32 >;
	makeExtern<DAS_CALL_METHOD(_method_132), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexDataAsRGBA32","das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *) , &ImFontAtlas::GetTexDataAsRGBA32 >::invoke")
		->args({"self","out_pixels","out_width","out_height","out_bytes_per_pixel"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_133 = das::das_call_member< bool (ImFontAtlas::*)() const,&ImFontAtlas::IsBuilt >;
	makeExtern<DAS_CALL_METHOD(_method_133), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsBuilt","das_call_member< bool (ImFontAtlas::*)() const , &ImFontAtlas::IsBuilt >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_134 = das::das_call_member< void (ImFontAtlas::*)(void *),&ImFontAtlas::SetTexID >;
	makeExtern<DAS_CALL_METHOD(_method_134), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetTexID","das_call_member< void (ImFontAtlas::*)(void *) , &ImFontAtlas::SetTexID >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_135 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesDefault >;
	makeExtern<DAS_CALL_METHOD(_method_135), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesDefault","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesDefault >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_136 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesKorean >;
	makeExtern<DAS_CALL_METHOD(_method_136), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesKorean","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesKorean >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_137 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesJapanese >;
	makeExtern<DAS_CALL_METHOD(_method_137), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesJapanese","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesJapanese >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_138 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesChineseFull >;
	makeExtern<DAS_CALL_METHOD(_method_138), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesChineseFull","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesChineseFull >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_139 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon >;
	makeExtern<DAS_CALL_METHOD(_method_139), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesChineseSimplifiedCommon","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_140 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesCyrillic >;
	makeExtern<DAS_CALL_METHOD(_method_140), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesCyrillic","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesCyrillic >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_141 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesThai >;
	makeExtern<DAS_CALL_METHOD(_method_141), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesThai","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesThai >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_142 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesVietnamese >;
	makeExtern<DAS_CALL_METHOD(_method_142), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesVietnamese","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesVietnamese >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_143 = das::das_call_member< int (ImFontAtlas::*)(int,int),&ImFontAtlas::AddCustomRectRegular >;
	makeExtern<DAS_CALL_METHOD(_method_143), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCustomRectRegular","das_call_member< int (ImFontAtlas::*)(int,int) , &ImFontAtlas::AddCustomRectRegular >::invoke")
		->args({"self","width","height"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_144 = das::das_call_member< int (ImFontAtlas::*)(ImFont *,unsigned short,int,int,float,const ImVec2 &),&ImFontAtlas::AddCustomRectFontGlyph >;
	makeExtern<DAS_CALL_METHOD(_method_144), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCustomRectFontGlyph","das_call_member< int (ImFontAtlas::*)(ImFont *,unsigned short,int,int,float,const ImVec2 &) , &ImFontAtlas::AddCustomRectFontGlyph >::invoke")
		->args({"self","font","id","width","height","advance_x","offset"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_145 = das::das_call_member< ImFontAtlasCustomRect * (ImFontAtlas::*)(int),&ImFontAtlas::GetCustomRectByIndex >;
	makeExtern<DAS_CALL_METHOD(_method_145), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCustomRectByIndex","das_call_member< ImFontAtlasCustomRect * (ImFontAtlas::*)(int) , &ImFontAtlas::GetCustomRectByIndex >::invoke")
		->args({"self","index"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

