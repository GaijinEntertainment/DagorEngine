#pragma once

#include <netImgui/source/NetImgui_Shared.h>
#include <netImgui/source/NetImgui_Network.h>


// Forward declare
namespace NetImguiServer { namespace RemoteClient { struct Client; } }
namespace NetImgui { namespace Internal { struct CmdTexture; } }

namespace NetImguiServer { namespace App
{
	//=============================================================================================
	// Code specific to 'NetImgui Server' application and needed inside platform specific code
	//=============================================================================================
	// Additional initialisation needed by 'NetImGui Server' and not part of default ImGui sample code
	bool	Startup(const char* CmdLine);
	// Prepare for shutdown of application
	void	Shutdown();
	// Receive rendering request of each Remote client and output it to their own RenderTarget
	void	UpdateRemoteContent();
	// Add a new remote client config to our list (to attempt connexion)
	bool	AddTransientClientConfigFromString(const char* string);
	// Check for monitor DPI update and regenerate font when needed 
	// (return true when font texture needs to be created)
	// bool	UpdateFont();
	// Descriptor of each textures by Server. Format always RGBA8
	// @sammyfreg todo: make this safer with smart pointer instead of manually deleting the HALTexture
	struct ServerTexture 
	{
		inline bool	IsValid(){ return mpHAL_Texture != nullptr; }
		void*			mpHAL_Texture		= nullptr;
		ServerTexture*	mpDeleteNext		= nullptr;	// Used to insert in a list of textures to be deleted next frame
		uint64_t		mImguiId			= 0u;		// Associated ImGui TextureId in Imgui commandlist
		uint64_t		mCustomData			= 0u;		// Memory available to custom command
		uint16_t		mSize[2]			= {0,0};
		uint8_t			mFormat				= 0;
		uint8_t			mPadding[3]			= {};
	};

	//=============================================================================================
	// Handling of texture data
	//=============================================================================================
	bool	CreateTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);
	void	DestroyTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);

	// Library users can implement their own texture format (on client/server). Useful for vidoe streaming, new format, etc.
	bool	CreateTexture_Custom(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);
	bool	DestroyTexture_Custom(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize);

	// Texture destruction is postponed until the end of the frame update to avoid rendering issues
	void	EnqueueHALTextureDestroy(ServerTexture& serverTexture);
	void	CompleteHALTextureDestroy();

	//=============================================================================================
	// Note (H)ardware (A)bstraction (L)ayer
	//		When porting the 'NetImgui Server' application to other platform, 
	//		theses are the few functions needed to be supported by each specific API that 
	//		are not already supported by de 'Dear ImGui' provided backends
	//=============================================================================================
	// Additional initialisation that are platform specific
	bool	HAL_Startup(const char* CmdLine);
	// Prepare for shutdown of application, with platform specific code
	void	HAL_Shutdown();
	// Receive a platform specific socket, and return us with info on the connection
	bool	HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort);
	// Provide the current user setting folder (used to save the shared config file)
	const char* HAL_GetUserSettingFolder();
	// Return true when new content should be retrieved from Clipboard (avoid constantly reading/converting content)
	bool	HAL_GetClipboardUpdated();
	// Receive a ImDrawData drawlist and request Dear ImGui's backend to output it into a texture
	void	HAL_RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData);
	// Allocate a texture resource
	bool	HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, ServerTexture& OutTexture);
	// Free a Texture resource
	void	HAL_DestroyTexture( ServerTexture& OutTexture );
	// Allocate a RenderTarget that each client will use to output their ImGui drawing into.
	bool	HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, RemoteClient::Client &client);
	// Free a RenderTarget resource
	void	HAL_DestroyRenderTarget(RemoteClient::Client &client);

}}
