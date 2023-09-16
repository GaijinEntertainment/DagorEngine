/** 
 @file  unix.h
 @brief ENet Unix header
*/
#ifndef __ENET_UNIX_H__
#define __ENET_UNIX_H__

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#if _TARGET_C3

#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#if _TARGET_PC
#include <netinet/ip.h>
#endif
#include <unistd.h>
#if _TARGET_APPLE
  #define _DARWIN_UNLIMITED_SELECT 1
#endif
#include <sys/select.h> // dagor - for fd_set
#include <arpa/inet.h>

typedef int ENetSocket;

#define ENET_SOCKET_NULL -1

#define ENET_HOST_TO_NET_16(value) (htons (value)) /**< macro that converts host to net byte-order of a 16-bit value */
#define ENET_HOST_TO_NET_32(value) (htonl (value)) /**< macro that converts host to net byte-order of a 32-bit value */

#define ENET_NET_TO_HOST_16(value) (ntohs (value)) /**< macro that converts net to host byte-order of a 16-bit value */
#define ENET_NET_TO_HOST_32(value) (ntohl (value)) /**< macro that converts net to host byte-order of a 32-bit value */

typedef struct _ENetBuffer
{
  // dagor - this struct must be equal iovec struct (sys/socket.h)
  void * data;
  size_t dataLength;
} ENetBuffer;

#define ENET_CALLBACK

#define ENET_API extern

typedef fd_set ENetSocketSet;

#define ENET_SOCKETSET_EMPTY(sockset)          FD_ZERO (& (sockset))
#define ENET_SOCKETSET_ADD(sockset, socket)    FD_SET (socket, & (sockset))
#define ENET_SOCKETSET_REMOVE(sockset, socket) FD_CLR (socket, & (sockset))
#define ENET_SOCKETSET_CHECK(sockset, socket)  FD_ISSET (socket, & (sockset))
    
#endif /* __ENET_UNIX_H__ */

