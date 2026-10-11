#pragma once

#include "daScript/misc/network.h"
#include "daScript/simulate/debug_info.h"

namespace das {
    bool makeServer ( const void * pClass, const StructInfo * info, Context * context );
    bool server_init ( Server * server, int port, Context * ctx, LineInfoArg * at );
    bool server_is_open ( Server * server, Context * context, LineInfoArg * at );
    bool server_is_connected ( Server * server, Context * context, LineInfoArg * at );
    bool server_send ( Server * server, uint8_t * data, int32_t size, Context * context, LineInfoArg * at );
    void server_tick ( Server * server, Context * context, LineInfoArg * at );
    void server_restore ( Server * server, const void * pClass, const StructInfo * info, Context * context, LineInfoArg * at );
    bool makeClient ( const void * pClass, const StructInfo * info, Context * context );
    bool client_connect ( Client * client, const char * host, int port, int timeout_ms, Context * context, LineInfoArg * at );
    bool client_is_connected ( Client * client, Context * context, LineInfoArg * at );
    bool client_send ( Client * client, uint8_t * data, int32_t size, Context * context, LineInfoArg * at );
    void client_tick ( Client * client, Context * context, LineInfoArg * at );
    void client_close ( Client * client, Context * context, LineInfoArg * at );
    int32_t probe_local_port_das ( const char * host, int32_t port );
}
