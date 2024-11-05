// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <winsock2.h>
#include "ws2tcpip.h"

#include "ns_private.h"
#include <debug/dag_debug.h>

#pragma comment(lib, "ws2_32.lib")

//
// WinSock 2.2 initialization and shutdown
//
static int winsock32_init_count = 0;
bool NetSockets::winsock2_init()
{
  // just return success if already initialized
  if (winsock32_init_count > 0)
  {
    winsock32_init_count++;
    return true;
  }

  // initialize WinSock
  WSADATA wsaData;
  int val;

  // ask for Winsock version 2.2.
  if ((val = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
  {
    DEBUG_CTX("--- WSAStartup failed with error %d: %s", val, NetSockets::decode_error(val));
    WSACleanup();
    return false;
  }

  winsock32_init_count++;
  DEBUG_CTX("WinSock32 inited");
  return true;
}
void NetSockets::winsock2_term()
{
  if (winsock32_init_count < 1)
  {
    DEBUG_CTX("winsock32_init_count=%d in winsock2_term (bug?)", winsock32_init_count);
    return;
  }
  winsock32_init_count--;
  if (winsock32_init_count == 0)
  {
    WSACleanup();
    DEBUG_CTX("WinSock32 terminated");
  }
}

// helper functions
char *NetSockets::decode_error(int ErrorCode)
{
  static char Message[1024];

  // If this program was multi-threaded, we'd want to use
  // FORMAT_MESSAGE_ALLOCATE_BUFFER instead of a static buffer here.
  // (And of course, free the buffer when we were done with it)

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, ErrorCode,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)Message, 1024, NULL);
  return Message;
}

//
// debug info init
//
static bool debug_info_loaded = false;
void NetSockets::load_debug_info()
{
  if (debug_info_loaded)
    return;
}
