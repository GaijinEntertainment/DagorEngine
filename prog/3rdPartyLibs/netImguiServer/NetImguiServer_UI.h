#pragma once

#include <cstdint>

#include <imgui/imgui.h>


namespace NetImguiServer { namespace App { struct ServerTexture; } } // Forward Declare

namespace NetImguiServer { namespace UI
{
	constexpr uint32_t			kWindowDPIDefault	= 96;
	bool						Startup();
	void						Shutdown();
	ImVec4						DrawImguiContent();
	void						DrawCenteredBackground( const App::ServerTexture& Texture, const ImVec4& tint=ImVec4(1.f,1.f,1.f,1.f));
	float						GetDisplayFPS();
	const App::ServerTexture&	GetBackgroundTexture();
	void						SetWindowDPI(uint32_t dpi);
	float						GetFontDPIScale();
}} //namespace NetImguiServer { namespace UI