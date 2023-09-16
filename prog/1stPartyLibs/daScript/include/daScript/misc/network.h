#pragma once

namespace das {
    #ifdef _WIN32
        #ifdef _WIN64
            typedef uint64_t socket_t;
        #else
            typedef uint32_t socket_t;
        #endif
    #else
        typedef int socket_t;
    #endif

    class Server : public ptr_ref_count {
    public:
        Server ();
        virtual ~Server();
        bool init( int port = 9000 );
        bool is_open() const;
        bool is_connected() const;
        void tick();
        bool send_msg ( char * data, int size );
    public:
        static bool startup();      // platform specific startup sockets (WSAStartup etc)
        static void shutdown();     // platform specific shutdown sockets
    protected:
        virtual void onConnect();
        virtual void onDisconnect();
        virtual void onData ( char * buf, int size );
        virtual void onError ( const char * msg, int code );
        virtual void onLog ( const char * msg );
    protected:
        socket_t server_fd = 0;
        socket_t client_fd = 0;
    };
}