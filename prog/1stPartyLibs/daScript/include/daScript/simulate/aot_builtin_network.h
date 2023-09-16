#pragma once

#include "daScript/misc/network.h"
#include "daScript/simulate/debug_info.h"

namespace das {
    bool makeServer ( const void * pClass, const StructInfo * info, Context * context );
    bool server_init ( smart_ptr_raw<Server> server, int port, Context * ctx, LineInfoArg * at );
    bool server_is_open ( smart_ptr_raw<Server> server, Context * context, LineInfoArg * at );
    bool server_is_connected ( smart_ptr_raw<Server> server, Context * context, LineInfoArg * at );
    bool server_send ( smart_ptr_raw<Server> server, uint8_t * data, int32_t size, Context * context, LineInfoArg * at );
    void server_tick ( smart_ptr_raw<Server> server, Context * context, LineInfoArg * at );
    void server_restore ( smart_ptr_raw<Server> server, const void * pClass, const StructInfo * info, Context * context, LineInfoArg * at );
}
