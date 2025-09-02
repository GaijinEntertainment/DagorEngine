
namespace NetImgui { namespace Internal { namespace Client {

void ClientTexture::Set( CmdTexture* pCmdTexture )
{
	netImguiDeleteSafe(mpCmdTexture);
	mpCmdTexture	= pCmdTexture;
	mbSent			= pCmdTexture == nullptr;
}

bool ClientTexture::IsValid()const
{
	return mpCmdTexture != nullptr;
}

bool ClientInfo::IsConnected()const
{
	return mpSocketComs.load() != nullptr;
}

bool ClientInfo::IsConnectPending()const
{
	return mbComInitActive || mpSocketPending.load() != nullptr || mpSocketListen.load() != nullptr;
}

bool ClientInfo::IsActive()const
{
	return mbClientThreadActive || mbListenThreadActive;
}

bool ClientInfo::IsContextOverriden()const
{
	return mSavedContextValues.mSavedContext;
}

}}} // namespace NetImgui::Internal::Client
