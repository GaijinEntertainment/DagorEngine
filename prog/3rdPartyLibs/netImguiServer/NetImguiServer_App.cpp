#include "NetImguiServer_App.h"
#include "NetImguiServer_Config.h"
#include "NetImguiServer_Network.h"
#include "NetImguiServer_RemoteClient.h"
#include "NetImguiServer_UI.h"


namespace NetImguiServer { namespace App
{

constexpr uint32_t			kClientCountMax			= 32;	//! @sammyfreg todo: support unlimited client count
static ServerTexture		gEmptyTexture;
std::atomic<ServerTexture*>	gPendingTextureDelete(nullptr);

bool Startup(const char* CmdLine)
{	
	//---------------------------------------------------------------------------------------------
	// Load Settings savefile and parse for auto connect commandline option
	//---------------------------------------------------------------------------------------------	
	NetImguiServer::Config::Client::LoadAll();
	AddTransientClientConfigFromString(CmdLine);
	
	//---------------------------------------------------------------------------------------------
    // Perform application initialization:
	//---------------------------------------------------------------------------------------------
	if (RemoteClient::Client::Startup(kClientCountMax) &&
		NetImguiServer::Network::Startup() &&
		NetImguiServer::UI::Startup())
	{
		uint8_t EmptyPixels[8*8];
		memset(EmptyPixels, 0, sizeof(EmptyPixels));
		NetImguiServer::App::HAL_CreateTexture(8, 8, NetImgui::eTexFormat::kTexFmtA8, EmptyPixels, gEmptyTexture);

		ImGui::GetIO().IniFilename	= nullptr;	// Disable server ImGui ini settings (not really needed, and avoid imgui.ini filename conflicts)
		ImGui::GetIO().LogFilename	= nullptr; 
		return HAL_Startup(CmdLine);
	}
	return false;
}

void Shutdown()
{
	NetImguiServer::Network::Shutdown();
	NetImguiServer::UI::Shutdown();
	NetImguiServer::App::HAL_DestroyTexture(gEmptyTexture);
	NetImguiServer::Config::Client::Clear();
	RemoteClient::Client::Shutdown();
	HAL_Shutdown();
}

//=================================================================================================
// INIT CLIENT CONFIG FROM STRING
// Take a commandline string, and create a ClientConfig from it.
// Simple format of (Hostname);(HostPort)
bool AddTransientClientConfigFromString(const char* string)
//=================================================================================================
{
	NetImguiServer::Config::Client cmdlineClient;
	const char* zEntryStart		= string;
	const char* zEntryCur		= string;
	int paramIndex				= 0;
	cmdlineClient.mConfigType	= NetImguiServer::Config::Client::eConfigType::Transient;
	NetImgui::Internal::StringCopy(cmdlineClient.mClientName, "Commandline");

	while( *zEntryCur != 0 )
	{
		zEntryCur++;
		// Skip commandline preamble holding path to executable
		if (*zEntryCur == ' ' && *(zEntryCur+1) != 0)
		{
			zEntryStart = zEntryCur + 1;
		}
		if( (*zEntryCur == ';' || *zEntryCur == 0) )
		{
			if (paramIndex == 0)
				NetImgui::Internal::StringCopy(cmdlineClient.mHostName, zEntryStart, zEntryCur-zEntryStart);
			else if (paramIndex == 1)
				cmdlineClient.mHostPort = static_cast<uint32_t>(atoi(zEntryStart));

			cmdlineClient.mConnectAuto	= paramIndex >= 1;	//Mark valid for connexion as soon as we have a HostAddress
			zEntryStart					= zEntryCur + 1;
			paramIndex++;
		}
	}

	if (cmdlineClient.mConnectAuto) {
		NetImguiServer::Config::Client::SetConfig(cmdlineClient);
		return true;
	}

	return false;
}

//=================================================================================================
// DRAW CLIENT BACKGROUND
// Create a separate Dear ImGui drawing context, to generate a commandlist that will fill the
// RenderTarget with a specific background picture
void DrawClientBackground(RemoteClient::Client& client)
//=================================================================================================
{
	NetImgui::Internal::CmdBackground* pBGUpdate = client.mPendingBackgroundIn.Release();
	if (pBGUpdate) {
		client.mBGSettings		= *pBGUpdate;
		client.mBGNeedUpdate	= true;
		netImguiDeleteSafe(pBGUpdate);
	}

	if (client.mBGNeedUpdate) 
	{
		if( client.mpBGContext == nullptr )
		{
			client.mpBGContext					= ImGui::CreateContext(ImGui::GetIO().Fonts);
			client.mpBGContext->IO.DeltaTime	= 1/30.f;
			client.mpBGContext->IO.IniFilename	= nullptr;	// Disable server ImGui ini settings (not really needed, and avoid imgui.ini filename conflicts)
			client.mpBGContext->IO.LogFilename	= nullptr; 
		}

		NetImgui::Internal::ScopedImguiContext scopedCtx(client.mpBGContext);
		ImGui::GetIO().DisplaySize = ImVec2(client.mAreaSizeX,client.mAreaSizeY);
		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(client.mAreaSizeX,client.mAreaSizeY));
		ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings);
		// Look for the desired texture (and use default if not found)
		auto texIt						= client.mTextureTable.find(client.mBGSettings.mTextureId);
		const ServerTexture* pTexture	= texIt == client.mTextureTable.end() ? &UI::GetBackgroundTexture() : &texIt->second;
		UI::DrawCenteredBackground(*pTexture, ImVec4(client.mBGSettings.mTextureTint[0],client.mBGSettings.mTextureTint[1],client.mBGSettings.mTextureTint[2],client.mBGSettings.mTextureTint[3]));
		ImGui::End();
		ImGui::Render();
		client.mBGNeedUpdate = false;
	}
}

//=================================================================================================
// UPDATE REMOTE CONTENT
// Create a render target for each connected remote client once, and update it every frame
// with the last drawing commands received from it. This Render Target will then be used
// normally as the background image of each client window renderered by this Server
void UpdateRemoteContent()
//=================================================================================================
{
	for (uint32_t i(0); i < RemoteClient::Client::GetCountMax(); ++i)
	{
		RemoteClient::Client& client = RemoteClient::Client::Get(i);
		if( client.mbIsConnected )
		{
			if (client.mbIsReleased) {
				client.Uninitialize();
			}
			else if( client.mbIsVisible ){
				client.ProcessPendingTextures();

				// Update the RenderTarget destination of each client, of size was updated
				if (client.mAreaSizeX > 0 && client.mAreaSizeY > 0 && (!client.mpHAL_AreaRT || client.mAreaRTSizeX != client.mAreaSizeX || client.mAreaRTSizeY != client.mAreaSizeY))
				{
					if (HAL_CreateRenderTarget(client.mAreaSizeX, client.mAreaSizeY, client))
					{
						client.mAreaRTSizeX		= client.mAreaSizeX;
						client.mAreaRTSizeY		= client.mAreaSizeY;
						client.mLastUpdateTime	= std::chrono::steady_clock::now() - std::chrono::hours(1); // Will redraw the client
						client.mBGNeedUpdate	= true;
					}
				}

				// Render the remote results
				ImDrawData* pDrawData = client.GetImguiDrawData(gEmptyTexture.mpHAL_Texture);
				if( pDrawData )
				{
					DrawClientBackground(client);
					HAL_RenderDrawData(client, pDrawData);
				}
			}
		}
	}
	CompleteHALTextureDestroy();
}

//=================================================================================================
bool CreateTexture_Default(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{	
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);

	auto eTexFmt = static_cast<NetImgui::eTexFormat>(cmdTexture.mFormat);
	if( eTexFmt < NetImgui::eTexFormat::kTexFmtCustom )
		return HAL_CreateTexture(cmdTexture.mWidth, cmdTexture.mHeight, eTexFmt, cmdTexture.mpTextureData.Get(), serverTexture);

	return false;
}

//=================================================================================================
bool DestroyTexture_Default(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	IM_UNUSED(cmdTexture);
	IM_UNUSED(customDataSize);
	
	if( serverTexture.mpHAL_Texture ){
		EnqueueHALTextureDestroy(serverTexture);
	}
	return true;
}

//=================================================================================================
bool CreateTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	// Default behavior is to always destroy then re-create the texture
	// But this can be changed inside the DestroyTexture_Custom function
	if(	(serverTexture.mpHAL_Texture != nullptr) ){
		DestroyTexture(serverTexture, cmdTexture, customDataSize);
	}

	if(	CreateTexture_Default(serverTexture, cmdTexture, customDataSize) )
	{
		serverTexture.mFormat	= cmdTexture.mFormat;
		serverTexture.mSize[0]	= cmdTexture.mWidth;
		serverTexture.mSize[1]	= cmdTexture.mHeight;
		return true;	
	}
	return false;
}
//=================================================================================================
void DestroyTexture(ServerTexture& serverTexture, const NetImgui::Internal::CmdTexture& cmdTexture, uint32_t customDataSize)
//=================================================================================================
{
	DestroyTexture_Default(serverTexture, cmdTexture, customDataSize);
}

//=================================================================================================
void EnqueueHALTextureDestroy(ServerTexture& serverTexture)
//=================================================================================================
{
	ServerTexture* pDeleteTexture	= new ServerTexture(serverTexture);
	pDeleteTexture->mpDeleteNext	= gPendingTextureDelete.exchange(pDeleteTexture);
	memset(&serverTexture, 0, sizeof(serverTexture)); // Making sure we don't double free this later
}

//=================================================================================================
void CompleteHALTextureDestroy()
//=================================================================================================
{
	ServerTexture* pTexture = gPendingTextureDelete.exchange(nullptr);
	while(pTexture)
	{
		ServerTexture* pDeleteMe	= pTexture;
		pTexture					= pTexture->mpDeleteNext;
		HAL_DestroyTexture(*pDeleteMe);
		delete pDeleteMe;
	}
}

}} // namespace NetImguiServer { namespace App
