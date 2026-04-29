#pragma once

namespace NetImguiServer { namespace App { struct ServerTexture; } } // Forward Declare

namespace NetImguiServer { namespace UI
{
	constexpr uint32_t			kWindowDPIDefault	= 96;
	bool						Startup();
	void						Shutdown();
	ImVec4						DrawImguiContent();
	void						DrawCenteredBackground( const App::ServerTexture* Texture, const ImVec4& tint=ImVec4(1.f,1.f,1.f,1.f));
	float						GetDisplayFPS();
	const App::ServerTexture*	GetBackgroundTexture();
}} //namespace NetImguiServer { namespace UI