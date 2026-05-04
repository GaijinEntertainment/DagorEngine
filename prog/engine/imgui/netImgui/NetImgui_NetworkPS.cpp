// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "NetImgui_Shared.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4221)
#endif

#if NETIMGUI_ENABLED
#include <sys/types.h>
#include <sys/socket.h>
#include <net.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h> // Required for TCP_NODELAY
#include <netinet/in.h>  // Gaijin: Required for IPPROTO_TCP
#include <arpa/inet.h>   // Gaijin: Required for inet_pton

#include "NetImgui_CmdPackets.h"

// NOTE: Removed static_assert(0) as requested changes are implemented below

namespace NetImgui
{
namespace Internal
{
namespace Network
{

//=================================================================================================
// Wrapper around native socket object and init some socket options
//=================================================================================================
struct SocketInfo
{
  SocketInfo(int socket) : mSocket(socket)
  {
    if (mSocket != -1)
    {
      // Set Non-Blocking
      int flags = fcntl(mSocket, F_GETFL, 0);
      fcntl(mSocket, F_SETFL, flags | O_NONBLOCK);

      // Set TCP No Delay
      int flag = 1;
      setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

      // Optional: Set Send Buffer Size (Mirroring Win32's attempt)
      // int kComsSendBuffer = 2 * mSendSizeMax;
      // setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (char*)&kComsSendBuffer, sizeof(kComsSendBuffer));
    }
  }

  int mSocket = -1;
  int mSendSizeMax = 1024 * 1024; // Limit tx data to avoid socket error on large amount (texture) [cite: 258]
};


static int net_library_memory_id = -1;
static SceNetId dns_resolver_id = -1;

bool Startup()
{
  net_library_memory_id = sceNetPoolCreate("NetImguiMemory", 64 * 1024, 0);
  if (net_library_memory_id < 0)
    return false;

  dns_resolver_id = sceNetResolverCreate("NetImguiDNSResolver", net_library_memory_id, 0);
  if (dns_resolver_id < 0)
    return false;

  return true;
}

void Shutdown()
{
  sceNetResolverDestroy(dns_resolver_id);
  sceNetPoolDestroy(net_library_memory_id);
}

//=================================================================================================
// Try establishing a connection to a remote client at given address (Non-Blocking)
//=================================================================================================
SocketInfo *Connect(const char *serverHost, uint32_t serverPort32)
{
  SceNetResolverInfo resolveResults{};
  if (sceNetResolverStartNtoaMultipleRecords(dns_resolver_id, serverHost, &resolveResults, 0, 0, 0) < 0)
    return nullptr;

  SceNetInPort_t serverPort = static_cast<SceNetInPort_t>(serverPort32);
  SceNetId clientSocket = -1;
  SceNetSockaddrIn sin;

  SocketInfo *pSocketInfo = nullptr;

  for (int i = 0; i < resolveResults.records; ++i)
  {
    const auto &addrEntry = resolveResults.addrs[i];

    memset(&sin, 0, sizeof(sin));
    if (inet_pton(addrEntry.af, serverHost, &sin.sin_addr) == 0)
      continue;
    sin.sin_len = sizeof(sin);
    sin.sin_family = addrEntry.af;
    sin.sin_port = htons(serverPort);


    clientSocket = socket(addrEntry.af, SCE_NET_SOCK_STREAM, 0);
    if (clientSocket < 0)
      continue; // Failed to create socket for this address

    // Set non-blocking *before* connect for non-blocking connect
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

    int connectResult = connect(clientSocket, (sockaddr *)&sin, sizeof(sin));
    bool isConnected = (connectResult == 0);

    if (connectResult == -1 && errno == EINPROGRESS)
    {
      // Connection attempt is in progress, use select to wait
      fd_set WriteSet;
      FD_ZERO(&WriteSet);
      FD_SET(clientSocket, &WriteSet);

      timeval kConnectTimeout = {1, 0}; // 1 second timeout
      connectResult = select(clientSocket + 1, nullptr, &WriteSet, nullptr, &kConnectTimeout);

      if (connectResult > 0)
      {
        // Select indicated socket is writable, check for connection errors
        int optVal;
        socklen_t optLen = sizeof(optVal);
        if (getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &optVal, &optLen) == 0 && optVal == 0)
        {
          isConnected = true;
        }
        else
        {
          // Connection failed
          isConnected = false;
        }
      }
      else
      {
        // Select timed out or error
        isConnected = false;
      }
    }
    else if (connectResult == -1)
    {
      // Immediate connection error
      isConnected = false;
    }

    if (isConnected)
    {
      pSocketInfo = netImguiNew<SocketInfo>(clientSocket);
      // Socket is already non-blocking from the SocketInfo constructor
    }
    else if (clientSocket >= 0)
    {
      close(clientSocket); // Close socket if connection failed
      clientSocket = -1;
    }
  }

  if (!pSocketInfo && clientSocket >= 0)
    close(clientSocket); // Clean up socket if loop finished without success

  return pSocketInfo;
}

//=================================================================================================
// Start waiting for connection request on this socket
//=================================================================================================
SocketInfo *ListenStart(uint32_t listenPort32)
{
  SceNetInPort_t listenPort = static_cast<SceNetInPort_t>(listenPort32);
  SceNetId listenSocket = -1;

  listenSocket = socket(SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
  if (listenSocket < 0)
    return nullptr;

  int flag = 1;
  setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
#ifdef SO_REUSEPORT
  setsockopt(listenSocket, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
#endif

  SceNetSockaddrIn sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_len = sizeof(sin);
  sin.sin_family = SCE_NET_AF_INET;
  sin.sin_port = htons(listenPort);

  if (bind(listenSocket, (sockaddr *)&sin, sizeof(sin)) >= 0 && listen(listenSocket, 5) >= 0)
  {
    // Keep listening socket blocking for accept() simplicity,
    // the *accepted* socket will be non-blocking via SocketInfo ctor
    int flags = fcntl(listenSocket, F_GETFL, 0);
    fcntl(listenSocket, F_SETFL, flags & (~O_NONBLOCK)); // Ensure blocking

    // Note: We create SocketInfo wrapper here just for consistency,
    // but it doesn't set options on the *listening* socket.
    return netImguiNew<SocketInfo>(listenSocket);
  }

  close(listenSocket);
  return nullptr;
}

//=================================================================================================
// Accept a new connection (blocking call on ListenSocket)
//=================================================================================================
SocketInfo *ListenConnect(SocketInfo *pListenSocket)
{
  if (pListenSocket && pListenSocket->mSocket != -1)
  {
    sockaddr_storage ClientAddress;
    socklen_t Size = sizeof(ClientAddress);
    // ListenSocket should be blocking (set in ListenStart)
    int ServerSocket = accept(pListenSocket->mSocket, (sockaddr *)&ClientAddress, &Size);
    if (ServerSocket != -1)
    {
      // Create SocketInfo wrapper, which sets the new socket to non-blocking
      return netImguiNew<SocketInfo>(ServerSocket);
    }
  }
  return nullptr;
}

//=================================================================================================
// Close a connection and free allocated object
//=================================================================================================
void Disconnect(SocketInfo *pClientSocket)
{
  if (pClientSocket && pClientSocket->mSocket != -1)
  {
    // Set SO_LINGER option to force close and discard pending data
    // to ensure the socket is closed immediately and exits the CLOSE_WAIT state more reliably
    struct linger sl;
    sl.l_onoff = 1;  // Enable linger
    sl.l_linger = 0; // Set timeout to 0 seconds (force RST)
    setsockopt(pClientSocket->mSocket, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

    shutdown(pClientSocket->mSocket, SHUT_RDWR);
    close(pClientSocket->mSocket);
    pClientSocket->mSocket = -1; // Mark as closed
  }
  netImguiDelete(pClientSocket);
}


//=================================================================================================
// Return true if data has been received, or there's a connection error
//=================================================================================================
bool DataReceivePending(SocketInfo *pClientSocket)
{
  const timeval kZeroTimeout = {0, 0}; // No wait time [cite: 272]
  if (!pClientSocket || pClientSocket->mSocket == -1)
  {
    return true; // Error condition
  }

  fd_set fdSetRead;
  fd_set fdSetErr;
  FD_ZERO(&fdSetRead);
  FD_ZERO(&fdSetErr);
  FD_SET(pClientSocket->mSocket, &fdSetRead);
  FD_SET(pClientSocket->mSocket, &fdSetErr);

  // Check if socket is readable or has an error [cite: 274]
  int result = select(pClientSocket->mSocket + 1, &fdSetRead, nullptr, &fdSetErr, const_cast<timeval *>(&kZeroTimeout));

  // Return true if select indicates data ready (result > 0) or error (result == -1)
  // Return false only if select times out (result == 0), meaning nothing to read yet
  return result != 0; // [cite: 275]
}

//=================================================================================================
// Receive as much as possible into PendingCom buffer (Non-Blocking)
//=================================================================================================
void DataReceive(SocketInfo *pClientSocket, NetImgui::Internal::PendingCom &PendingComRcv)
{
  // Invalid command or socket state
  if (!pClientSocket || pClientSocket->mSocket == -1 || !PendingComRcv.pCommand)
  {
    PendingComRcv.bError = true;
    return;
  }

  size_t BytesToRead = PendingComRcv.pCommand->mSize - PendingComRcv.SizeCurrent;
  if (BytesToRead == 0)
  {
    return; // Already fully received
  }

  // Receive data from remote connection (non-blocking)
  ssize_t resultRcv =
    recv(pClientSocket->mSocket, &reinterpret_cast<uint8_t *>(PendingComRcv.pCommand)[PendingComRcv.SizeCurrent], BytesToRead,
      0); // No flags, non-blocking behavior comes from socket setting

  if (resultRcv > 0)
  {
    // Successfully received some data
    PendingComRcv.SizeCurrent += static_cast<size_t>(resultRcv);
    PendingComRcv.bError = false; // Reset error flag on successful read
  }
  else if (resultRcv == 0)
  {
    // Connection closed gracefully by peer
    PendingComRcv.bError = true;
  }
  else
  { // resultRcv < 0
    // Error occurred
    if (errno == EWOULDBLOCK || errno == EAGAIN)
    {
      // Not an error, just no data available right now on non-blocking socket
      PendingComRcv.bError = false;
    }
    else
    {
      // Actual socket error
      PendingComRcv.bError = true;
    }
  }
}

//=================================================================================================
// Send as much as possible from PendingCom buffer (Non-Blocking)
//=================================================================================================
void DataSend(SocketInfo *pClientSocket, NetImgui::Internal::PendingCom &PendingComSend)
{
  // Invalid command or socket state
  if (!pClientSocket || pClientSocket->mSocket == -1 || !PendingComSend.pCommand)
  {
    PendingComSend.bError = true;
    return;
  }

  size_t BytesRemaining = PendingComSend.pCommand->mSize - PendingComSend.SizeCurrent;
  if (BytesRemaining == 0)
  {
    return; // Already fully sent
  }

  // Limit send size per call [cite: 281]
  size_t BytesToSend = BytesRemaining > static_cast<size_t>(pClientSocket->mSendSizeMax)
                         ? static_cast<size_t>(pClientSocket->mSendSizeMax)
                         : BytesRemaining;

  // Send data to remote connection (non-blocking)
  ssize_t resultSent =
    send(pClientSocket->mSocket, &reinterpret_cast<const uint8_t *>(PendingComSend.pCommand)[PendingComSend.SizeCurrent], BytesToSend,
      MSG_NOSIGNAL); // Use MSG_NOSIGNAL to prevent SIGPIPE on Linux if connection is broken

  if (resultSent > 0)
  {
    // Successfully sent some data
    PendingComSend.SizeCurrent += static_cast<size_t>(resultSent);
    PendingComSend.bError = false; // Reset error flag on successful send
  }
  else if (resultSent == 0)
  {
    // This shouldn't typically happen with TCP unless BytesToSend was 0
    PendingComSend.bError = false; // Treat as non-error for now
  }
  else
  { // resultSent < 0
    // Error occurred
    if (errno == EWOULDBLOCK || errno == EAGAIN)
    {
      // Not an error, socket buffer is full, try again later
      PendingComSend.bError = false;
    }
    else
    {
      // Actual socket error (e.g., EPIPE if connection broken and MSG_NOSIGNAL not used/supported)
      PendingComSend.bError = true;
    }
  }
}

} // namespace Network
} // namespace Internal
} // namespace NetImgui
#else

// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it
// will not be used by any link operation that consumes this library)
extern int sSuppresstLNK4221_NetImgui_NetworkPosix;
int sSuppresstLNK4221_NetImgui_NetworkPosix(0);

#endif // #if NETIMGUI_ENABLED
