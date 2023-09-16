#include "daScript/misc/platform.h"

#include "daScript/misc/network.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#else

#ifdef _TARGET_C3

#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define closesocket close

#ifdef __APPLE__
#include <sys/errno.h>
#endif

#endif

namespace das {

#ifdef _WIN32
    #define invalid_socket(x)   ((x)==socket_t(~0))
#else
    #define invalid_socket(x)   ((x)<0)
#endif

    bool Server::startup() {
#ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2,2), &wsaData)==0;
#else
        return true;
#endif
    }

    void Server::shutdown() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool set_socket_blocking ( socket_t fd, bool blocking ) {
#ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
    }

    Server::Server() {
    }

    bool Server::init ( int port ) {
        errno = 0;
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if ( !server_fd ) {
            onError("can't socket", errno);
            return false;
        }
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(uint16_t(port) );
#if defined(__APPLE__)
        int val = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
#endif
    	if ( ::bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0 ) {
            onError("can't bind", errno);
            closesocket(server_fd);
            return false;
	    }
    	if ( listen(server_fd, 3) < 0) {
            onError("can't listen", errno);
            closesocket(server_fd);
            return false;
        }
        if ( !set_socket_blocking(server_fd,false) ) {
            onError("can't set nbio", errno);
            closesocket(server_fd);
            return false;
        }
        return true;
    }

    void Server::onData(char *, int) {
        // printf("%s\n", buf);
        // send_msg(buf, size);
    }

    void Server::onConnect() {
    }

    void Server::onDisconnect() {
    }

    void Server::onError(const char *, int) {
        // printf("server error %i - %s\n", code, msg);
    }

    void Server::onLog(const char *) {
        // printf("server log %s\n", msg);
    }

    bool Server::send_msg ( char * data, int size ) {
        errno = 0;
        if ( client_fd <= 0 ) {
            onError("can't send, not connected", -1);
            return false;
        }
        int res = 0;
        for ( ;; ) {
            res = send(client_fd, data, size, 0);
            if ( res>0 ) {
                DAS_ASSERT(size>=res);
                data += res;
                size -= res;
                if ( size==0 ) {
                    return true;
                }
            } else {
                res = errno;
                if ( res!=0 && res!=EAGAIN && res!=EWOULDBLOCK ) {
                    onError ( "can't send", res);
                    closesocket(client_fd);
                    client_fd = 0;
                    return false;
                }
            }
        }
    }

    void Server::tick() {
        errno = 0;
        if ( client_fd==0 ) {
            struct sockaddr_in address;
            int addrlen = sizeof(address);
            client_fd = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen);
            if ( !invalid_socket(client_fd) ) {
                if ( !set_socket_blocking(client_fd,false) ) {
                    onError("can't set client nbio", errno);
                    closesocket(client_fd);
                    client_fd = 0;
                }
                onLog("connection accepted");
                onConnect();

            } else {
                client_fd = 0;
            }
        } else {
            char buffer[1025];
            int res = recv(client_fd, buffer, 1024, 0);
            if ( res >0 ) {
                buffer[res] = 0;
                onData(buffer, res);
            } else if ( res==0 ) {
                onLog("connection closed");
                onDisconnect();
                closesocket(client_fd);
                client_fd = 0;
            } else { // res<0
                res = errno;
                if ( res!=0 && res!=EAGAIN && res!=EWOULDBLOCK ) {
                    onError("connection closed on error", res);
                    onDisconnect();
                    closesocket(client_fd);
                    client_fd = 0;
                }
            }
        }
    }

    Server::~Server() {
        errno = 0;
        if ( client_fd >0 ) {
            closesocket(client_fd);
        }
        if ( server_fd ) {
            closesocket(server_fd);
        }
    }

    bool Server::is_open() const {
        return server_fd != 0;
    }

    bool Server::is_connected() const {
        return client_fd > 0;
    }
}
