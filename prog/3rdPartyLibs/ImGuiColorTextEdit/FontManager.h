#pragma once

struct ImFont;

typedef ImFont* (*GetUiFont_t)();

namespace FontManager
{
	ImFont* GetUiFont();
	ImFont* GetCodeFont();

	extern GetUiFont_t GetUiFontFn;
	extern GetUiFont_t GetCodeFontFn;
}
