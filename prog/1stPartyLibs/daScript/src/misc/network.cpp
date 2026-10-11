#include "daScript/misc/platform.h"

#include "daScript/misc/network.h"

#include <chrono>

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
#if !defined(_TARGET_C1) && !defined(_TARGET_C2)
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define closesocket close

#ifdef __APPLE__
#include <sys/errno.h>
#endif

#endif

#if defined(_TARGET_C1) || defined(_TARGET_C2)

#else
#define DAS_NETWORK_CLIENT 1
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

    static int last_socket_error () {
#ifdef _WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }

    static bool socket_would_block ( int err ) {
#ifdef _WIN32
        return err==WSAEWOULDBLOCK;
#else
        return err==EAGAIN || err==EWOULDBLOCK || err==EINTR;
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
            onError("can't socket", last_socket_error());
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
            onError("can't bind", last_socket_error());
            closesocket(server_fd);
            return false;
        }
        if ( listen(server_fd, 3) < 0) {
            onError("can't listen", last_socket_error());
            closesocket(server_fd);
            return false;
        }
        if ( !set_socket_blocking(server_fd,false) ) {
            onError("can't set nbio", last_socket_error());
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
#ifdef MSG_NOSIGNAL
        const int sendFlags = MSG_NOSIGNAL;
#else
        const int sendFlags = 0;
#endif
        for ( ;; ) {
            res = send(client_fd, data, size, sendFlags);
            if ( res>0 ) {
                DAS_ASSERT(size>=res);
                data += res;
                size -= res;
                if ( size==0 ) {
                    return true;
                }
            } else {
                res = last_socket_error();
                if ( !socket_would_block(res) ) {
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
#if defined(__APPLE__)
                int nosigpipe = 1;
                setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
#endif
                if ( !set_socket_blocking(client_fd,false) ) {
                    onError("can't set client nbio", last_socket_error());
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
                res = last_socket_error();
                if ( !socket_would_block(res) ) {
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

    static bool loopback_address ( const char * host, struct sockaddr_in & address ) {
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        if ( !host || !*host || strcmp(host, "localhost")==0 ) {
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            return true;
        }
#if DAS_NETWORK_CLIENT
        return inet_pton(AF_INET, host, &address.sin_addr) == 1;
#else
        return false;
#endif
    }

    static const int SEND_WAIT_MS = 10000;

#if DAS_NETWORK_CLIENT
    static bool connect_pending ( int err ) {
#ifdef _WIN32
        return socket_would_block(err);
#else
        return err==EINPROGRESS || socket_would_block(err);
#endif
    }
#endif

    typedef std::chrono::steady_clock::time_point deadline_t;

    static deadline_t deadline_after_ms ( int timeout_ms ) {
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    }

    static int wait_writable_until ( socket_t sock, deadline_t deadline ) {
#if !DAS_NETWORK_CLIENT
        (void)sock;
        (void)deadline;
        return -1;
#else
        for ( ;; ) {
            auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
            if ( left < 0 ) left = 0;
#ifdef _WIN32
            fd_set wset;
            FD_ZERO(&wset);
            FD_SET(sock, &wset);
            fd_set eset;
            FD_ZERO(&eset);
            FD_SET(sock, &eset);
            struct timeval tv;
            tv.tv_sec = long(left / 1000);
            tv.tv_usec = long((left % 1000) * 1000);
            int res = select(0, nullptr, &wset, &eset, &tv);
#else
            struct pollfd pfd;
            pfd.fd = sock;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            int res = poll(&pfd, 1, int(left));
#endif
            if ( res > 0 ) return 0;
            if ( res == 0 ) return -1;
            int err = last_socket_error();
            if ( !socket_would_block(err) ) return err;
        }
#endif
    }

    Client::Client() {
    }

    Client::~Client() {
        disconnect();
    }

    bool Client::connect ( const char * host, int port, int timeout_ms ) {
        errno = 0;
        if ( connected ) {
            onError("already connected", -1);
            return false;
        }
#if !DAS_NETWORK_CLIENT
        (void)host;
        (void)port;
        (void)timeout_ms;
        onError("no client connections on this platform", -1);
        return false;
#else
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        char portText[16];
        snprintf(portText, sizeof(portText), "%d", port);
        const char * node = (!host || !*host) ? "localhost" : host;
        struct addrinfo * list = nullptr;
        if ( getaddrinfo(node, portText, &hints, &list)!=0 || !list ) {
            onError("can't resolve host", -1);
            return false;
        }
        const char * failure = "can't connect";
        int failureCode = -1;
        socket_t sock = 0;
        bool ok = false;
        auto deadline = deadline_after_ms(timeout_ms);
        for ( auto ai = list; ai && !ok; ai = ai->ai_next ) {
            sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if ( invalid_socket(sock) ) {
                failure = "can't socket";
                failureCode = last_socket_error();
                continue;
            }
#if defined(__APPLE__)
            int nosigpipe = 1;
            setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
#endif
            if ( !set_socket_blocking(sock,false) ) {
                failure = "can't set nbio";
                failureCode = last_socket_error();
                closesocket(sock);
                continue;
            }
            if ( ::connect(sock, ai->ai_addr, socklen_t(ai->ai_addrlen)) == 0 ) {
                ok = true;
            } else {
                int err = last_socket_error();
                if ( connect_pending(err) ) {
                    int waited = wait_writable_until(sock, deadline);
                    if ( waited == 0 ) {
                        int soerr = 0;
                        socklen_t len = sizeof(soerr);
                        if ( getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&soerr, &len) != 0 ) {
                            failureCode = last_socket_error();
                        } else if ( soerr != 0 ) {
                            failureCode = soerr;
                        } else {
                            ok = true;
                        }
                    } else if ( waited < 0 ) {
                        failure = "connect timed out";
                        failureCode = -1;
                    } else {
                        failureCode = waited;
                    }
                } else {
                    failureCode = err;
                }
            }
            if ( !ok ) closesocket(sock);
        }
        freeaddrinfo(list);
        if ( !ok ) {
            onError(failure, failureCode);
            return false;
        }
        fd = sock;
        connected = true;
        onLog("connected");
        onConnect();
        return true;
#endif
    }

    bool Client::is_connected() const {
        return connected;
    }

    void Client::disconnect() {
        if ( connected ) {
            connected = false;
            closesocket(fd);
            fd = 0;
        }
    }

    bool Client::send_msg ( char * data, int size ) {
        errno = 0;
        if ( !connected ) {
            onError("can't send, not connected", -1);
            return false;
        }
#ifdef MSG_NOSIGNAL
        const int sendFlags = MSG_NOSIGNAL;
#else
        const int sendFlags = 0;
#endif
        while ( size > 0 ) {
            int res = send(fd, data, size, sendFlags);
            if ( res > 0 ) {
                data += res;
                size -= res;
            } else {
                res = last_socket_error();
                const char * failure = "can't send";
                if ( socket_would_block(res) ) {
                    int waited = wait_writable_until(fd, deadline_after_ms(SEND_WAIT_MS));
                    if ( waited == 0 ) continue;
                    if ( waited < 0 ) {
                        failure = "send timed out";
                        res = -1;
                    } else {
                        res = waited;
                    }
                }
                onError(failure, res);
                disconnect();
                onDisconnect();
                return false;
            }
        }
        return true;
    }

    void Client::tick() {
        errno = 0;
        if ( !connected ) return;
        char buffer[4097];
        for ( ;; ) {
            int res = recv(fd, buffer, 4096, 0);
            if ( res > 0 ) {
                buffer[res] = 0;
                onData(buffer, res);
                if ( !connected ) return;
            } else if ( res == 0 ) {
                onLog("connection closed");
                disconnect();
                onDisconnect();
                return;
            } else {
                res = last_socket_error();
                if ( !socket_would_block(res) ) {
                    onError("connection closed on error", res);
                    disconnect();
                    onDisconnect();
                }
                return;
            }
        }
    }

    void Client::onData(char *, int) {
    }

    void Client::onConnect() {
    }

    void Client::onDisconnect() {
    }

    void Client::onError(const char *, int) {
    }

    void Client::onLog(const char *) {
    }

    int probe_local_port ( const char * host, int port ) {
        struct sockaddr_in address;
        if ( !loopback_address(host, address) ) return -1;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(uint16_t(port));
        socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
        if ( invalid_socket(sock) ) return -1;
        int bound = -1;
        if ( ::bind(sock, (struct sockaddr *)&address, sizeof(address)) == 0 ) {
            socklen_t len = sizeof(address);
            if ( getsockname(sock, (struct sockaddr *)&address, &len) == 0 ) {
                bound = ntohs(address.sin_port);
            }
        }
        closesocket(sock);
        return bound;
    }
}
