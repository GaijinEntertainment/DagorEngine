#include "FontManager.h"
#include <imgui.h>

namespace FontManager
{
	GetUiFont_t GetUiFontFn = nullptr;
	GetUiFont_t GetCodeFontFn = nullptr;

	ImFont* GetUiFont()
	{
		ImFont* font = GetUiFontFn ? GetUiFontFn() : nullptr;
		return font ? font : ImGui::GetIO().FontDefault;
	}

	ImFont* GetCodeFont()
	{
		ImFont* font = GetCodeFontFn ? GetCodeFontFn() : nullptr;
		return font ? font : ImGui::GetIO().FontDefault;
	}
}
