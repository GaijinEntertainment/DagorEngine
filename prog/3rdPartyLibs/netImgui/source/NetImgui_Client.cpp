#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED
#include "NetImgui_WarningDisable.h"
#include "NetImgui_Client.h"
#include "NetImgui_Network.h"
#include "NetImgui_CmdPackets.h"

namespace NetImgui { namespace Internal { namespace Client
{

//=================================================================================================
// SAVED IMGUI CONTEXT
// Because we overwrite some Imgui context IO values, we save them before makign any change
// and restore them after detecting a disconnection
//=================================================================================================
void SavedImguiContext::Save(ImGuiContext* copyFrom)
{
	ScopedImguiContext scopedContext(copyFrom);
	ImGuiIO& sourceIO		= ImGui::GetIO();
	mSavedContext			= true;
	mConfigFlags			= sourceIO.ConfigFlags;
	mBackendFlags			= sourceIO.BackendFlags;
	mBackendPlatformName	= sourceIO.BackendPlatformName;
	mBackendRendererName	= sourceIO.BackendRendererName;
	mDrawMouse				= sourceIO.MouseDrawCursor;
	mFontGlobalScale		= sourceIO.FontGlobalScale;
	mFontGeneratedSize		= sourceIO.Fonts->Fonts.Size > 0 ? sourceIO.Fonts->Fonts[0]->FontSize : 13.f; // Save size to restore the font to original size
#if IMGUI_VERSION_NUM < 18700
	memcpy(mKeyMap, sourceIO.KeyMap, sizeof(mKeyMap));
#endif
#if IMGUI_VERSION_NUM < 19110
	mGetClipboardTextFn		= sourceIO.GetClipboardTextFn;
    mSetClipboardTextFn		= sourceIO.SetClipboardTextFn;
    mClipboardUserData		= sourceIO.ClipboardUserData;
#else
	ImGuiPlatformIO& plaformIO	= ImGui::GetPlatformIO();
	mGetClipboardTextFn			= plaformIO.Platform_GetClipboardTextFn;
    mSetClipboardTextFn			= plaformIO.Platform_SetClipboardTextFn;
    mClipboardUserData			= plaformIO.Platform_ClipboardUserData;
#endif
}

void SavedImguiContext::Restore(ImGuiContext* copyTo)
{
	ScopedImguiContext scopedContext(copyTo);
	ImGuiIO& destIO				= ImGui::GetIO();
	mSavedContext				= false;
	destIO.ConfigFlags			= mConfigFlags;
	destIO.BackendFlags			= mBackendFlags;
	destIO.BackendPlatformName	= mBackendPlatformName;
	destIO.BackendRendererName	= mBackendRendererName;
	destIO.MouseDrawCursor		= mDrawMouse;
	destIO.FontGlobalScale		= mFontGlobalScale;
#if IMGUI_VERSION_NUM < 18700
	memcpy(destIO.KeyMap, mKeyMap, sizeof(destIO.KeyMap));
#endif
#if IMGUI_VERSION_NUM < 19110
	destIO.GetClipboardTextFn	= mGetClipboardTextFn;
    destIO.SetClipboardTextFn	= mSetClipboardTextFn;
	destIO.ClipboardUserData	= mClipboardUserData;
#else
	ImGuiPlatformIO& plaformIO				= ImGui::GetPlatformIO();
	plaformIO.Platform_GetClipboardTextFn 	= mGetClipboardTextFn;
    plaformIO.Platform_SetClipboardTextFn 	= mSetClipboardTextFn;
    plaformIO.Platform_ClipboardUserData 	= mClipboardUserData;
#endif
}

//=================================================================================================
// GET CLIPBOARD
// Content received from the Server
//=================================================================================================
#if IMGUI_VERSION_NUM < 19110
static const char* GetClipboardTextFn_NetImguiImpl(void* user_data_ctx)
{
	const ClientInfo* pClient = reinterpret_cast<const ClientInfo*>(user_data_ctx);
	return pClient && pClient->mpCmdClipboard ? pClient->mpCmdClipboard->mContentUTF8.Get() : nullptr;
}
#else
static const char* GetClipboardTextFn_NetImguiImpl(ImGuiContext* ctx)
{
	ScopedImguiContext scopedContext(ctx);
	const ClientInfo* pClient = reinterpret_cast<const ClientInfo*>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
	return pClient && pClient->mpCmdClipboard ? pClient->mpCmdClipboard->mContentUTF8.Get() : nullptr;
}
#endif


//=================================================================================================
// SET CLIPBOARD
//=================================================================================================
#if IMGUI_VERSION_NUM < 19110
static void SetClipboardTextFn_NetImguiImpl(void* user_data_ctx, const char* text)
{
	if(user_data_ctx){
		ClientInfo* pClient				= reinterpret_cast<ClientInfo*>(user_data_ctx);
		CmdClipboard* pClipboardOut		= CmdClipboard::Create(text);
		pClient->mPendingClipboardOut.Assign(pClipboardOut);
	}
}
#else
static void SetClipboardTextFn_NetImguiImpl(ImGuiContext* ctx, const char* text)
{
	ScopedImguiContext scopedContext(ctx);
	ClientInfo* pClient				= reinterpret_cast<ClientInfo*>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
	CmdClipboard* pClipboardOut		= CmdClipboard::Create(text);
	pClient->mPendingClipboardOut.Assign(pClipboardOut);
}
#endif

//=================================================================================================
// INCOM: INPUT
// Receive new keyboard/mouse/screen resolution input to pass on to dearImgui
//=================================================================================================
void Communications_Incoming_Input(ClientInfo& client)
{
	auto pCmdInput					= static_cast<CmdInput*>(client.mPendingRcv.pCommand);
	client.mPendingRcv.bAutoFree	= false; // Taking ownership of the data
	client.mDesiredFps 				= pCmdInput->mDesiredFps > 0.f ? pCmdInput->mDesiredFps : 0.f;
	size_t keyCount(pCmdInput->mKeyCharCount);
	client.mPendingKeyIn.AddData(pCmdInput->mKeyChars, keyCount);
	client.mPendingInputIn.Assign(pCmdInput);
}

//=================================================================================================
// INCOM: CLIPBOARD
// Receive server new clipboard content, updating internal cache
//=================================================================================================
void Communications_Incoming_Clipboard(ClientInfo& client)
{
	auto pCmdClipboard				= static_cast<CmdClipboard*>(client.mPendingRcv.pCommand);
	client.mPendingRcv.bAutoFree	= false; // Taking ownership of the data
	pCmdClipboard->ToPointers();
	client.mPendingClipboardIn.Assign(pCmdClipboard);
}

//=================================================================================================
// OUTCOM: TEXTURE
// Transmit all pending new/updated texture
//=================================================================================================
void Communications_Outgoing_Textures(ClientInfo& client)
{
	client.ProcessTexturePending();
	if( client.mbHasTextureUpdate )
	{
		for(auto& cmdTexture : client.mTextures)
		{
			if( !cmdTexture.mbSent && cmdTexture.mpCmdTexture )
			{
				client.mPendingSend.pCommand	= cmdTexture.mpCmdTexture;
				client.mPendingSend.bAutoFree	= cmdTexture.mpCmdTexture->mFormat == eTexFormat::kTexFmt_Invalid; // Texture as been marked for deletion, can now free memory allocated for this
				cmdTexture.mbSent				= true;
				return; // Exit as soon as we find a texture to send, so thread can proceed with transmitting it
			}
		}
		client.mbHasTextureUpdate = false;	// No pending texture detected, mark as 'all sent'
	}
}

//=================================================================================================
// OUTCOM: BACKGROUND
// Transmit the current client background settings
//=================================================================================================
void Communications_Outgoing_Background(ClientInfo& client)
{
	CmdBackground* pPendingBackground = client.mPendingBackgroundOut.Release();
	if( pPendingBackground )
	{
		client.mPendingSend.pCommand	= pPendingBackground;
		client.mPendingSend.bAutoFree	= false;
	}
}

//=================================================================================================
// OUTCOM: FRAME
// Transmit a new dearImgui frame to render
//=================================================================================================
void Communications_Outgoing_Frame(ClientInfo& client)
{
	CmdDrawFrame* pPendingDraw = client.mPendingFrameOut.Release();
	if( pPendingDraw )
	{
		pPendingDraw->mFrameIndex = client.mFrameIndex++;
		//---------------------------------------------------------------------
		// Apply delta compression to DrawCommand, when requested
		if( pPendingDraw->mCompressed )
		{
			// Create a new Compressed DrawFrame Command
			if( client.mpCmdDrawLast && !client.mServerCompressionSkip ){
				client.mpCmdDrawLast->ToPointers();
				CmdDrawFrame* pDrawCompressed	= CompressCmdDrawFrame(client.mpCmdDrawLast, pPendingDraw);
				netImguiDeleteSafe(client.mpCmdDrawLast);
				client.mpCmdDrawLast			= pPendingDraw;		// Keep original new command for next frame delta compression
				pPendingDraw					= pDrawCompressed;	// Request compressed copy to be sent to server
			}
			// Save DrawCmd for next frame delta compression
			else {
				pPendingDraw->mCompressed		= false;
				client.mpCmdDrawLast			= pPendingDraw;
			}
		}
		client.mServerCompressionSkip = false;

		//---------------------------------------------------------------------
		// Ready to send command to server
		pPendingDraw->ToOffsets();
		client.mPendingSend.pCommand	= pPendingDraw;
		client.mPendingSend.bAutoFree	= client.mpCmdDrawLast != pPendingDraw;
	}
}

//=================================================================================================
// OUTCOM: Clipboard
// Send client 'Copy' clipboard content to Server
//=================================================================================================
void Communications_Outgoing_Clipboard(ClientInfo& client)
{
	CmdClipboard* pPendingClipboard = client.mPendingClipboardOut.Release();
	if( pPendingClipboard ){
		pPendingClipboard->ToOffsets();
		client.mPendingSend.pCommand	= pPendingClipboard;
		client.mPendingSend.bAutoFree	= true;
	}
}

//=================================================================================================
// INCOMING COMMUNICATIONS
//=================================================================================================
void Communications_Incoming(ClientInfo& client)
{
	if( Network::DataReceivePending(client.mpSocketComs) )
	{
		//-----------------------------------------------------------------------------------------
		// 1. Ready to receive new command, starts the process by reading Header
		//-----------------------------------------------------------------------------------------
		if( client.mPendingRcv.IsReady() )
		{
			client.mCmdPendingRead 		= CmdPendingRead();
			client.mPendingRcv.pCommand	= &client.mCmdPendingRead;
			client.mPendingRcv.bAutoFree= false;
		}

		//-----------------------------------------------------------------------------------------
		// 2. Read incoming command from server
		//-----------------------------------------------------------------------------------------
		if( client.mPendingRcv.IsPending() )
		{
			Network::DataReceive(client.mpSocketComs, client.mPendingRcv);

			// Detected a new command bigger than header, allocate memory for it
			if( client.mPendingRcv.pCommand->mSize > sizeof(CmdPendingRead) &&
				client.mPendingRcv.pCommand == &client.mCmdPendingRead )
			{
				CmdPendingRead* pCmdHeader 	= reinterpret_cast<CmdPendingRead*>(netImguiSizedNew<uint8_t>(client.mPendingRcv.pCommand->mSize));
				*pCmdHeader					= client.mCmdPendingRead;
				client.mPendingRcv.pCommand	= pCmdHeader;
				client.mPendingRcv.bAutoFree= true;
			}
		}

		//-----------------------------------------------------------------------------------------
		// 3. Command fully received from Server, process it
		//-----------------------------------------------------------------------------------------
		if( client.mPendingRcv.IsDone() )
		{
			if( !client.mPendingRcv.IsError() )
			{
				switch( client.mPendingRcv.pCommand->mType )
				{
					case CmdHeader::eCommands::Input:		Communications_Incoming_Input(client); break;
					case CmdHeader::eCommands::Clipboard:	Communications_Incoming_Clipboard(client); break;
					// Commands not received in main loop, by Client
					case CmdHeader::eCommands::Version:
					case CmdHeader::eCommands::Texture:
					case CmdHeader::eCommands::DrawFrame:
					case CmdHeader::eCommands::Background:
					case CmdHeader::eCommands::Count: break;
				}
			}

			// Cleanup after read completion
			if( client.mPendingRcv.IsError() ){
				client.mbDisconnectPending = true;
			}
			if( client.mPendingRcv.bAutoFree ){
				netImguiDeleteSafe(client.mPendingRcv.pCommand);
			}
			client.mPendingRcv = PendingCom();
		}
	}
	// Prevent high CPU usage when waiting for new data
	else
	{
		//std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::microseconds(250));
	}
}

//=================================================================================================
// OUTGOING COMMUNICATIONS
//=================================================================================================
void Communications_Outgoing(ClientInfo& client)
{
	//---------------------------------------------------------------------------------------------
	// Try finishing sending a pending command to Server
	//---------------------------------------------------------------------------------------------
	if( client.mPendingSend.IsPending() )
	{
		Network::DataSend(client.mpSocketComs, client.mPendingSend);

		// Free allocated memory for command
		if( client.mPendingSend.IsDone() )
		{
			if( client.mPendingSend.IsError() ){
				client.mbDisconnectPending = true;
			}
			if( client.mPendingSend.bAutoFree ){
				netImguiDeleteSafe(client.mPendingSend.pCommand);
			}
			client.mPendingSend = PendingCom();
		}
	}
	//---------------------------------------------------------------------------------------------
	// Initiate sending next command to Server, when none are in flight
	// Keep track of what command was last send to prevent 1 type to monopolize coms
	//---------------------------------------------------------------------------------------------
	constexpr CmdHeader::eCommands kCommandsOrder[] = {
		CmdHeader::eCommands::Texture, CmdHeader::eCommands::Background,
		CmdHeader::eCommands::Clipboard, CmdHeader::eCommands::DrawFrame};
	constexpr uint32_t kCommandCounts = static_cast<uint32_t>(sizeof(kCommandsOrder)/sizeof(CmdHeader::eCommands));

	uint32_t Index(client.mPendingSendNext);
	while( client.mPendingSend.IsReady() && (Index-client.mPendingSendNext)<kCommandCounts )
	{
		CmdHeader::eCommands NextCmd = kCommandsOrder[Index++ % kCommandCounts];
		switch( NextCmd )
		{
			case CmdHeader::eCommands::Texture:		Communications_Outgoing_Textures(client); break;
			case CmdHeader::eCommands::Background:	Communications_Outgoing_Background(client); break;
			case CmdHeader::eCommands::Clipboard:	Communications_Outgoing_Clipboard(client); break;
			case CmdHeader::eCommands::DrawFrame:	Communications_Outgoing_Frame(client); break;
			// Commands not sent in main loop, by Client
			case CmdHeader::eCommands::Input:
			case CmdHeader::eCommands::Version:
			case CmdHeader::eCommands::Count: break;
		}
		if( client.mPendingSend.IsPending() ){
			client.mPendingSendNext = Index;
		}
	}
}

//=================================================================================================
// COMMUNICATIONS INITIALIZE
// Initialize a new connection to a RemoteImgui server
//=================================================================================================
bool Communications_Initialize(ClientInfo& client)
{
	CmdVersion cmdVersionSend, cmdVersionRcv;
	PendingCom PendingRcv, PendingSend;

	client.mbComInitActive = true;

	//---------------------------------------------------------------------
	// Handshake confirming connection validity
	//---------------------------------------------------------------------
	PendingRcv.pCommand = reinterpret_cast<CmdPendingRead*>(&cmdVersionRcv);
	while( !PendingRcv.IsDone() && cmdVersionRcv.mType == CmdHeader::eCommands::Version )
	{
		while( !client.mbDisconnectPending && !Network::DataReceivePending(client.mpSocketPending) ){
			std::this_thread::yield(); // Idle until we receive the remote data
		}
		Network::DataReceive(client.mpSocketPending, PendingRcv);
	}

	bool bForceConnect	= client.mServerForceConnectEnabled && (cmdVersionRcv.mFlags & static_cast<uint8_t>(CmdVersion::eFlags::ConnectForce)) != 0;
	bool bCanConnect 	= !PendingRcv.IsError() &&
						  cmdVersionRcv.mType		== cmdVersionSend.mType &&
						  cmdVersionRcv.mVersion	== cmdVersionSend.mVersion &&
						  cmdVersionRcv.mWCharSize	== cmdVersionSend.mWCharSize &&
						  (!client.IsConnected() || bForceConnect);

	StringCopy(cmdVersionSend.mClientName, client.mName);
	cmdVersionSend.mFlags			= client.IsConnected() && !bCanConnect ? static_cast<uint8_t>(CmdVersion::eFlags::IsConnected): 0;
	cmdVersionSend.mFlags			|= client.IsConnected() && !client.mServerForceConnectEnabled ? static_cast<uint8_t>(CmdVersion::eFlags::IsUnavailable) : 0;

	PendingSend.pCommand 	= reinterpret_cast<CmdPendingRead*>(&cmdVersionSend);
	while( !PendingSend.IsDone() ){
		Network::DataSend(client.mpSocketPending, PendingSend);
	}

	//---------------------------------------------------------------------
	// Connection established, init client
	//---------------------------------------------------------------------
	if( bCanConnect && !PendingSend.IsError() && (!client.IsConnected() || bForceConnect) )
	{
		Network::SocketInfo* pNewComSocket = client.mpSocketPending.exchange(nullptr);

		// If we detect an active connection with Server and 'ForceConnect was requested, close it first
		if( client.IsConnected() )
		{
			client.mbDisconnectPending = true;
			while( client.IsConnected() );
		}

		for(auto& texture : client.mTextures)
		{
			texture.mbSent = false;
		}

		client.mpSocketComs					= pNewComSocket;					// Take ownerhip of socket
		client.mbHasTextureUpdate			= true;								// Force sending the client textures
		client.mBGSettingSent.mTextureId	= client.mBGSetting.mTextureId-1u;	// Force sending the Background settings (by making different than current settings)
		client.mFrameIndex					= 0;
		client.mPendingSendNext				= 0;
		client.mServerForceConnectEnabled	= (cmdVersionRcv.mFlags & static_cast<uint8_t>(CmdVersion::eFlags::ConnectExclusive)) == 0;
		client.mPendingRcv					= PendingCom();
		client.mPendingSend					= PendingCom();
	}

	// Disconnect pending socket if init failed
	Network::SocketInfo* SocketPending	= client.mpSocketPending.exchange(nullptr);
	bool bValidConnection				= SocketPending == nullptr;
	if( SocketPending ){
		NetImgui::Internal::Network::Disconnect(SocketPending);
	}

	client.mbComInitActive = false;
	return bValidConnection;
}

//=================================================================================================
// COMMUNICATIONS MAIN LOOP
//=================================================================================================
void Communications_Loop(void* pClientVoid)
{
	IM_ASSERT(pClientVoid != nullptr);
	ClientInfo* pClient				= reinterpret_cast<ClientInfo*>(pClientVoid);
	pClient->mbDisconnectPending	= false;
	pClient->mbClientThreadActive 	= true;

	while( !pClient->mbDisconnectPending )
	{
		Communications_Outgoing(*pClient);
		Communications_Incoming(*pClient);
	}

	Network::SocketInfo* pSocket = pClient->mpSocketComs.exchange(nullptr);
	if (pSocket){
		NetImgui::Internal::Network::Disconnect(pSocket);
	}
	pClient->mbClientThreadActive 	= false;
}

//=================================================================================================
// COMMUNICATIONS CONNECT THREAD : Reach out and connect to a NetImGuiServer
//=================================================================================================
void CommunicationsConnect(void* pClientVoid)
{
	IM_ASSERT(pClientVoid != nullptr);
	ClientInfo* pClient	= reinterpret_cast<ClientInfo*>(pClientVoid);
	if( Communications_Initialize(*pClient) )
	{
		Communications_Loop(pClientVoid);
	}
}

//=================================================================================================
// COMMUNICATIONS HOST THREAD : Waiting NetImGuiServer reaching out to us.
//								Launch a new com loop when connection is established
//=================================================================================================
void CommunicationsHost(void* pClientVoid)
{
	ClientInfo* pClient				= reinterpret_cast<ClientInfo*>(pClientVoid);
	pClient->mbListenThreadActive	= true;
	pClient->mbDisconnectListen		= false;
	pClient->mpSocketListen			= pClient->mpSocketPending.exchange(nullptr);

	while( pClient->mpSocketListen.load() != nullptr && !pClient->mbDisconnectListen )
	{
		pClient->mpSocketPending = Network::ListenConnect(pClient->mpSocketListen);
		if( pClient->mpSocketPending.load() != nullptr && Communications_Initialize(*pClient) )
		{
			pClient->mThreadFunction(Client::Communications_Loop, pClient);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));	// Prevents this thread from taking entire core, waiting on server connection requests
	}

	Network::SocketInfo* pSocket 	= pClient->mpSocketListen.exchange(nullptr);
	NetImgui::Internal::Network::Disconnect(pSocket);
	pClient->mbListenThreadActive	= false;
}

//=================================================================================================
// Support of the Dear ImGui hooks
// (automatic call to NetImgui::BeginFrame()/EndFrame() on ImGui::BeginFrame()/Imgui::Render()
//=================================================================================================
#if NETIMGUI_IMGUI_CALLBACK_ENABLED
void HookBeginFrame(ImGuiContext*, ImGuiContextHook* hook)
{
	Client::ClientInfo& client = *reinterpret_cast<Client::ClientInfo*>(hook->UserData);
	if (!client.mbInsideNewEnd)
	{
		ScopedBool scopedInside(client.mbInsideHook, true);
		NetImgui::NewFrame(false);
	}
}

void HookEndFrame(ImGuiContext*, ImGuiContextHook* hook)
{
	Client::ClientInfo& client = *reinterpret_cast<Client::ClientInfo*>(hook->UserData);
	if (!client.mbInsideNewEnd)
	{
		ScopedBool scopedInside(client.mbInsideHook, true);
		NetImgui::EndFrame();
	}
}

#endif 	// NETIMGUI_IMGUI_CALLBACK_ENABLED

//=================================================================================================
// CLIENT INFO Constructor
//=================================================================================================
ClientInfo::ClientInfo()
: mpSocketPending(nullptr)
, mpSocketComs(nullptr)
, mpSocketListen(nullptr)
, mFontTextureID(TextureCastFromUInt(uint64_t(0u)))
, mTexturesPendingSent(0)
, mTexturesPendingCreated(0)
, mbClientThreadActive(false)
, mbListenThreadActive(false)
, mbComInitActive(false)
{
	memset(mTexturesPending, 0, sizeof(mTexturesPending));
}

//=================================================================================================
// CLIENT INFO Constructor
//=================================================================================================
ClientInfo::~ClientInfo()
{
	ContextRemoveHooks();

	for( auto& texture : mTextures ){
		texture.Set(nullptr);
	}

	for(size_t i(0); i<ArrayCount(mTexturesPending); ++i){
		netImguiDeleteSafe(mTexturesPending[i]);
	}

	netImguiDeleteSafe(mpCmdInputPending);
	netImguiDeleteSafe(mpCmdDrawLast);
	netImguiDeleteSafe(mpCmdClipboard);
}

//=================================================================================================
// Initialize the associated ImguiContext
//=================================================================================================
void ClientInfo::ContextInitialize()
{
	mpContext				= ImGui::GetCurrentContext();

#if NETIMGUI_IMGUI_CALLBACK_ENABLED
	ImGuiContextHook hookNewframe, hookEndframe;
	ContextRemoveHooks();
	hookNewframe.HookId		= 0;
	hookNewframe.Type		= ImGuiContextHookType_NewFramePre;
	hookNewframe.Callback	= HookBeginFrame;
	hookNewframe.UserData	= this;
	mhImguiHookNewframe		= ImGui::AddContextHook(mpContext, &hookNewframe);
	hookEndframe.HookId		= 0;
	hookEndframe.Type		= ImGuiContextHookType_RenderPost;
	hookEndframe.Callback	= HookEndFrame;
	hookEndframe.UserData	= this;
	mhImguiHookEndframe		= ImGui::AddContextHook(mpContext, &hookEndframe);
#endif
}

//=================================================================================================
// Take over a Dear ImGui context for use with NetImgui
//=================================================================================================
void ClientInfo::ContextOverride()
{
	ScopedImguiContext scopedSourceCtx(mpContext);

	// Keep a copy of original settings of this context
	mSavedContextValues.Save(mpContext);
	mLastOutgoingDrawCheckTime = std::chrono::steady_clock::now();

	// Override some settings
	// Note: Make sure every setting overwritten here, are handled in 'SavedImguiContext::Save(...)'
	{
		ImGuiIO& newIO						= ImGui::GetIO();
		newIO.MouseDrawCursor				= false;
		newIO.BackendPlatformName			= "NetImgui";
		newIO.BackendRendererName			= "DirectX11";
		if( mFontCreationFunction != nullptr )
		{
			newIO.FontGlobalScale = 1;
			mFontCreationScaling = -1;
		}

#if IMGUI_VERSION_NUM < 18700
		for (uint32_t i(0); i < ImGuiKey_COUNT; ++i) {
			newIO.KeyMap[i] = i;
		}
#endif
#if IMGUI_VERSION_NUM < 19110
		newIO.GetClipboardTextFn				= GetClipboardTextFn_NetImguiImpl;
		newIO.SetClipboardTextFn				= SetClipboardTextFn_NetImguiImpl;
		newIO.ClipboardUserData					= this;
#else
		ImGuiPlatformIO& platformIO 			= ImGui::GetPlatformIO();
		platformIO.Platform_GetClipboardTextFn	= GetClipboardTextFn_NetImguiImpl;
		platformIO.Platform_SetClipboardTextFn	= SetClipboardTextFn_NetImguiImpl;
		platformIO.Platform_ClipboardUserData	= this;
#endif
#if defined(IMGUI_HAS_VIEWPORT)
		newIO.ConfigFlags &= ~(ImGuiConfigFlags_ViewportsEnable); // Viewport unsupported at the moment
#endif
	}
}

//=================================================================================================
// Restore a Dear ImGui context to initial state before we modified it
//=================================================================================================
void ClientInfo::ContextRestore()
{
	// Note: only happens if context overriden is same as current one, to prevent trying to restore to a deleted context
	if (IsContextOverriden() && ImGui::GetCurrentContext() == mpContext)
	{
#ifdef IMGUI_HAS_VIEWPORT
		ImGui::UpdatePlatformWindows(); // Prevents issue with mismatched frame tracking, when restoring enabled viewport feature
#endif
		if( mFontCreationFunction && ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->Fonts.size() > 0)
		{
			float noScaleSize	= ImGui::GetIO().Fonts->Fonts[0]->FontSize / mFontCreationScaling;
			float originalScale = mSavedContextValues.mFontGeneratedSize / noScaleSize;
			mFontCreationFunction(mFontCreationScaling, originalScale);
		}
		mSavedContextValues.Restore(mpContext);
	}
}

//=================================================================================================
// Remove callback hooks, once we detect a disconnection
//=================================================================================================
void ClientInfo::ContextRemoveHooks()
{
#if NETIMGUI_IMGUI_CALLBACK_ENABLED
	if (mpContext && mhImguiHookNewframe != 0)
	{
		ImGui::RemoveContextHook(mpContext, mhImguiHookNewframe);
		ImGui::RemoveContextHook(mpContext, mhImguiHookEndframe);
		mhImguiHookNewframe = mhImguiHookNewframe = 0;
	}
#endif
}

//=================================================================================================
// Process textures waiting to be sent to server
// 1. New textures are added tp pending queue (Main Thread)
// 2. Pending textures are sent to Server and added to our active texture list (Com Thread) 
//=================================================================================================
void ClientInfo::ProcessTexturePending()
{
	while( mTexturesPendingCreated != mTexturesPendingSent )
	{
		mbHasTextureUpdate			|= true;
		uint32_t idx				= mTexturesPendingSent.fetch_add(1) % static_cast<uint32_t>(ArrayCount(mTexturesPending));
		CmdTexture* pCmdTexture		= mTexturesPending[idx];
		mTexturesPending[idx]		= nullptr;
		if( pCmdTexture )
		{
			// Find the TextureId from our list (or free slot)
			int texIdx		= 0;
			int texFreeSlot	= static_cast<int>(mTextures.size());
			while( texIdx < mTextures.size() && ( !mTextures[texIdx].IsValid() || mTextures[texIdx].mpCmdTexture->mTextureId != pCmdTexture->mTextureId) )
			{
				texFreeSlot = !mTextures[texIdx].IsValid() ? texIdx : texFreeSlot;
				++texIdx;
			}

			if( texIdx == mTextures.size() )
				texIdx = texFreeSlot;
			if( texIdx == mTextures.size() )
				mTextures.push_back(ClientTexture());

			mTextures[texIdx].Set( pCmdTexture );
			mTextures[texIdx].mbSent = false;
		}
	}
}

//=================================================================================================
// Create a new Draw Command from Dear Imgui draw data.
// 1. New ImGui frame has been completed, create a new draw command from draw data (Main Thread)
// 2. We see a pending Draw Command, take ownership of it and send it to Server (Com thread)
//=================================================================================================
void ClientInfo::ProcessDrawData(const ImDrawData* pDearImguiData, ImGuiMouseCursor mouseCursor)
{
	if( !mbValidDrawFrame )
		return;

	CmdDrawFrame* pDrawFrameNew = ConvertToCmdDrawFrame(pDearImguiData, mouseCursor);
	pDrawFrameNew->mCompressed	= mClientCompressionMode == eCompressionMode::kForceEnable || (mClientCompressionMode == eCompressionMode::kUseServerSetting && mServerCompressionEnabled);
	mPendingFrameOut.Assign(pDrawFrameNew);
}

}}} // namespace NetImgui::Internal::Client

#include "NetImgui_WarningReenable.h"
#endif //#if NETIMGUI_ENABLED
