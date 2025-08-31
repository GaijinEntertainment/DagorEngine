#include "NetImgui_Shared.h"

// Tested with Unreal Engine 4.27, 5.3, 5.4, 5.5

#if NETIMGUI_ENABLED && defined(__UNREAL__)

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Misc/OutputDeviceRedirector.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "HAL/PlatformProcess.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
#include "IPAddressAsyncResolve.h"
#endif

namespace NetImgui { namespace Internal { namespace Network
{

//=================================================================================================
// Wrapper around native socket object and init some socket options
//=================================================================================================
struct SocketInfo
{
	SocketInfo(FSocket* pSocket) 
	: mpSocket(pSocket) 
	{
		if( mpSocket )
		{
			mpSocket->SetNonBlocking(true);
			mpSocket->SetNoDelay(true);

			int32 NewSize(0);
			while( !mpSocket->SetSendBufferSize(2*mSendSize, NewSize) ){
				mSendSize /= 2;
			}
			mSendSize = NewSize/2;
		}
	}

	~SocketInfo() 
	{ 
		Close(); 
	}

	void Close()
	{
		if(mpSocket )
		{
			mpSocket->Close();
			ISocketSubsystem::Get()->DestroySocket(mpSocket);
			mpSocket = nullptr;
		}
	}
	FSocket* mpSocket	= nullptr;
	int32 mSendSize		= 1024*1024;	// Limit tx data to avoid socket error on large amount (texture)
};

bool Startup()
{
	return true;
}

void Shutdown()
{
}

//=================================================================================================
// Try establishing a connection to a remote client at given address
//=================================================================================================
SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	SocketInfo* pSocketInfo					= nullptr;
	ISocketSubsystem* SocketSubSystem		= ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedPtr<FInternetAddr> IpAddressFind	= SocketSubSystem->GetAddressFromString((TCHAR*)StringCast<TCHAR>(static_cast<const ANSICHAR*>(ServerHost)).Get());
	if(IpAddressFind)
	{
		TSharedRef<FInternetAddr> IpAddress	= IpAddressFind->Clone();
		IpAddress->SetPort(ServerPort);
		if (IpAddress->IsValid())
		{
			FSocket* pNewSocket	= SocketSubSystem->CreateSocket(NAME_Stream, "NetImgui", IpAddress->GetProtocolType());
			if (pNewSocket)
			{
				pNewSocket->SetNonBlocking(true);
				if (pNewSocket->Connect(*IpAddress))
				{
					bool bConnectionReady = false;
					pNewSocket->WaitForPendingConnection(bConnectionReady, FTimespan::FromSeconds(1.0f));
					if( bConnectionReady )
					{
						pSocketInfo = netImguiNew<SocketInfo>(pNewSocket);
						return pSocketInfo;
					}
				}
			}
		}
	}
	netImguiDelete(pSocketInfo);
	return nullptr;
}

//=================================================================================================
// Start waiting for connection request on this socket
//=================================================================================================
SocketInfo* ListenStart(uint32_t ListenPort)
{
	ISocketSubsystem* PlatformSocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedPtr<FInternetAddr> IpAddress = PlatformSocketSub->GetLocalBindAddr(*GLog);
	IpAddress->SetPort(ListenPort);

	FSocket* pNewListenSocket			= PlatformSocketSub->CreateSocket(NAME_Stream, "NetImguiListen", IpAddress->GetProtocolType());
	if( pNewListenSocket )
	{
		SocketInfo* pListenSocketInfo	= netImguiNew<SocketInfo>(pNewListenSocket);
	#if NETIMGUI_FORCE_TCP_LISTEN_BINDING
		pNewListenSocket->SetReuseAddr();
	#endif
		pNewListenSocket->SetNonBlocking(false);
		pNewListenSocket->SetRecvErr();
		if (pNewListenSocket->Bind(*IpAddress))
		{		
			if (pNewListenSocket->Listen(1))
			{
				return pListenSocketInfo;
			}
		}
		netImguiDelete(pListenSocketInfo);
	}	
	return nullptr;
}

//=================================================================================================
// Establish a new connection to a remote request
//=================================================================================================
SocketInfo* ListenConnect(SocketInfo* pListenSocket)
{
	if (pListenSocket)
	{
		FSocket* pNewSocket = pListenSocket->mpSocket->Accept(FString("NetImgui"));
		if( pNewSocket )
		{
			SocketInfo* pSocketInfo = netImguiNew<SocketInfo>(pNewSocket);
			return pSocketInfo;
		}
	}
	return nullptr;
}

//=================================================================================================
// Close a connection and free allocated object
//=================================================================================================
void Disconnect(SocketInfo* pClientSocket)
{
	netImguiDelete(pClientSocket);
}

//=================================================================================================
// Return true if data has been received, or there's a connection error
//=================================================================================================
bool DataReceivePending(SocketInfo* pClientSocket)
{
	// Note: return true on a connection error, to exit code looping on the data wait. Will handle error after DataReceive()
	uint32 PendingDataSize;
	return !pClientSocket || pClientSocket->mpSocket->HasPendingData(PendingDataSize) || (pClientSocket->mpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected);
}

//=================================================================================================
// Receive as much as possible a command and keep track of transfer status
//=================================================================================================
void DataReceive(SocketInfo* pClientSocket, NetImgui::Internal::PendingCom& PendingComRcv)
{
	// Invalid command
	if( !pClientSocket || !PendingComRcv.pCommand || !pClientSocket->mpSocket  || (pClientSocket->mpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected)){
		PendingComRcv.bError = true;
		return;
	}

	int32 sizeRcv(0);
	if( pClientSocket->mpSocket->Recv(	&reinterpret_cast<uint8*>(PendingComRcv.pCommand)[PendingComRcv.SizeCurrent],
										static_cast<int>(PendingComRcv.pCommand->mSize-PendingComRcv.SizeCurrent),
										sizeRcv,
										ESocketReceiveFlags::None) )
	{
		PendingComRcv.SizeCurrent	+= static_cast<size_t>(sizeRcv);
		PendingComRcv.bError		|= sizeRcv <= 0; // Error if no data read since DataReceivePending() said there was some available
	}
	else
	{
		// Connection error, abort transmission
		const ESocketErrors SocketError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
		PendingComRcv.bError			= SocketError != SE_NO_ERROR && SocketError != ESocketErrors::SE_EWOULDBLOCK;
	}
}

//=================================================================================================
// Receive as much as possible a command and keep track of transfer status
//=================================================================================================
void DataSend(SocketInfo* pClientSocket, NetImgui::Internal::PendingCom& PendingComSend)
{
	// Invalid command
	if( !pClientSocket || !PendingComSend.pCommand || !pClientSocket->mpSocket || (pClientSocket->mpSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected) ){
		PendingComSend.bError = true;
		return;
	}

	int32 sizeSent		= 0;
	int32 sizeToSend	= PendingComSend.pCommand->mSize-PendingComSend.SizeCurrent;
	sizeToSend			= sizeToSend > pClientSocket->mSendSize ? pClientSocket->mSendSize : sizeToSend;

	if( pClientSocket->mpSocket->Send(	&reinterpret_cast<uint8*>(PendingComSend.pCommand)[PendingComSend.SizeCurrent],
										static_cast<int>(sizeToSend),
										sizeSent) )
	{
		PendingComSend.SizeCurrent += static_cast<size_t>(sizeSent);
	}
	else
	{
		// Connection error, abort transmission
		const ESocketErrors SocketError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
		PendingComSend.bError			= SocketError != SE_NO_ERROR && SocketError != ESocketErrors::SE_EWOULDBLOCK;
	}
}

}}} // namespace NetImgui::Internal::Network

#else

// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library)
extern int sSuppresstLNK4221_NetImgui_NetworkUE4; 
int sSuppresstLNK4221_NetImgui_NetworkUE4(0);

#endif // #if NETIMGUI_ENABLED && defined(__UNREAL__)
