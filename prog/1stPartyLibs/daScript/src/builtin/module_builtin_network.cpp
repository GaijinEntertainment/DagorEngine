#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/simulate/aot_builtin_network.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "module_builtin_rtti.h"

#include <atomic>

MAKE_TYPE_FACTORY(NetworkServer,Server)
MAKE_TYPE_FACTORY(NetworkClient,Client)

namespace das {

    static atomic<int32_t> g_moduleNetworkTotalSockets{0};
    class ServerAdapter : public Server {
    public:
        ServerAdapter(char * pClass, const StructInfo * info, Context * ctx ) {
            update(pClass,info,ctx);
            if ( !g_moduleNetworkTotalSockets++ )
                Server::startup();
        }
        virtual ~ServerAdapter() {
            if ( !--g_moduleNetworkTotalSockets )
                Server::shutdown();
        }
        void update ( char * pClass, const StructInfo * info, Context * ctx ) {
            context = ctx;
            classPtr = pClass;
            pServer = (void **) adapt_field("_server",pClass,info);
            if ( pServer ) *pServer = this;
            fnOnConnect = adapt("onConnect",pClass,info);
            fnOnDisconnect = adapt("onDisconnect",pClass,info);
            fnOnData = adapt("onData",pClass,info);
            fnOnError = adapt("onError",pClass,info);
            fnOnLog = adapt("onLog",pClass,info);
        }
        virtual void onConnect() override {
            if ( fnOnConnect ) {
                return das_invoke_function<void>::invoke<void *>
                    (context,nullptr,fnOnConnect,classPtr);
            }
        }
        virtual void onDisconnect() override {
            if ( fnOnDisconnect ) {
                return das_invoke_function<void>::invoke<void *>
                    (context,nullptr,fnOnDisconnect,classPtr);
            }
        }
        virtual void onData ( char * buf, int size ) override {
            if ( fnOnData ) {
                return das_invoke_function<void>::invoke<void *,char *,int32_t>
                    (context,nullptr,fnOnData,classPtr,buf,size);
            }
        }
        virtual void onError ( const char * msg, int code ) override {
            if ( fnOnError ) {
                return das_invoke_function<void>::invoke<void *,const char *,int32_t>
                    (context,nullptr,fnOnError,classPtr,msg,code);
            }
        }
        virtual void onLog ( const char * msg ) override {
            if ( fnOnLog ) {
                return das_invoke_function<void>::invoke<void *,const char *>
                    (context,nullptr,fnOnLog,classPtr,msg);
            }
        }
        bool isValid() const { return pServer != nullptr; }
    protected:
        void ** pServer = nullptr;
        Func    fnOnConnect;
        Func    fnOnDisconnect;
        Func    fnOnData;
        Func    fnOnError;
        Func    fnOnLog;
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct ServerAnnotation : ManagedStructureAnnotation<Server> {
        ServerAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("NetworkServer", ml, "Server") {
        }
    };

    class ClientAdapter : public Client {
    public:
        ClientAdapter(char * pClass, const StructInfo * info, Context * ctx ) {
            context = ctx;
            classPtr = pClass;
            pClient = (void **) adapt_field("_client",pClass,info);
            if ( pClient ) *pClient = this;
            fnOnConnect = adapt("onConnect",pClass,info);
            fnOnDisconnect = adapt("onDisconnect",pClass,info);
            fnOnData = adapt("onData",pClass,info);
            fnOnError = adapt("onError",pClass,info);
            fnOnLog = adapt("onLog",pClass,info);
            if ( !g_moduleNetworkTotalSockets++ )
                Server::startup();
        }
        virtual ~ClientAdapter() {
            if ( !--g_moduleNetworkTotalSockets )
                Server::shutdown();
        }
        virtual void onConnect() override {
            if ( fnOnConnect ) {
                return das_invoke_function<void>::invoke<void *>
                    (context,nullptr,fnOnConnect,classPtr);
            }
        }
        virtual void onDisconnect() override {
            if ( fnOnDisconnect ) {
                return das_invoke_function<void>::invoke<void *>
                    (context,nullptr,fnOnDisconnect,classPtr);
            }
        }
        virtual void onData ( char * buf, int size ) override {
            if ( fnOnData ) {
                return das_invoke_function<void>::invoke<void *,char *,int32_t>
                    (context,nullptr,fnOnData,classPtr,buf,size);
            }
        }
        virtual void onError ( const char * msg, int code ) override {
            if ( fnOnError ) {
                return das_invoke_function<void>::invoke<void *,const char *,int32_t>
                    (context,nullptr,fnOnError,classPtr,msg,code);
            }
        }
        virtual void onLog ( const char * msg ) override {
            if ( fnOnLog ) {
                return das_invoke_function<void>::invoke<void *,const char *>
                    (context,nullptr,fnOnLog,classPtr,msg);
            }
        }
        bool isValid() const { return pClient != nullptr; }
    protected:
        void ** pClient = nullptr;
        Func    fnOnConnect;
        Func    fnOnDisconnect;
        Func    fnOnData;
        Func    fnOnError;
        Func    fnOnLog;
    protected:
        void *      classPtr;
        Context *   context;
    };

    struct ClientAnnotation : ManagedStructureAnnotation<Client> {
        ClientAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("NetworkClient", ml, "Client") {
        }
    };

    bool makeClient ( const void * pClass, const StructInfo * info, Context * context ) {
        auto client = new ClientAdapter((char *)pClass,info,context);
        if ( !client->isValid() ) { delete client; return false; }
        return true;
    }

    bool client_connect ( Client * client, const char * host, int port, int timeout_ms, Context * context, LineInfoArg * at ) {
        if ( !client ) context->throw_error_at(at, "null client");
        return client->connect(host, port, timeout_ms);
    }

    bool client_is_connected ( Client * client, Context * context, LineInfoArg * at ) {
        if ( !client ) context->throw_error_at(at, "null client");
        return client->is_connected();
    }

    bool client_send ( Client * client, uint8_t * data, int32_t size, Context * context, LineInfoArg * at ) {
        if ( !client ) context->throw_error_at(at, "null client");
        return client->send_msg((char *)data, size);
    }

    void client_tick ( Client * client, Context * context, LineInfoArg * at ) {
        if ( !client ) context->throw_error_at(at, "null client");
        client->tick();
    }

    void client_close ( Client * client, Context * context, LineInfoArg * at ) {
        if ( !client ) context->throw_error_at(at, "null client");
        client->disconnect();
    }

    int32_t probe_local_port_das ( const char * host, int32_t port ) {
        if ( !g_moduleNetworkTotalSockets++ ) Server::startup();
        int bound = probe_local_port(host, port);
        if ( !--g_moduleNetworkTotalSockets ) Server::shutdown();
        return bound;
    }

    bool makeServer ( const void * pClass, const StructInfo * info, Context * context ) {
        auto server = new ServerAdapter((char *)pClass,info,context);
        if ( !server->isValid() ) { delete server; return false; }
        return true;
    }

    bool server_init ( Server * server, int port, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->init(port);
    }

    bool server_is_open ( Server * server, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->is_open();
    }

    bool server_is_connected ( Server * server, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->is_connected();
    }

    bool server_send ( Server * server, uint8_t * data, int32_t size, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->send_msg((char *)data, size);
    }

    void server_tick ( Server * server, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        server->tick();
    }

    void server_restore ( Server * server, const void * pClass, const StructInfo * info, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        auto adapter = (ServerAdapter *) server;
        adapter->update((char *)pClass,info,context);
    }

    class Module_Network : public Module {
    public:
        Module_Network() : Module("network_core") {
            DAS_PROFILE_SECTION("Module_Network");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti_core"));
            // sever
            addAnnotation(new ServerAnnotation(lib));
            addExtern<DAS_BIND_FUN(makeServer)>(*this, lib,  "make_server",
                SideEffects::modifyArgumentAndExternal, "makeServer")
                    ->args({"class","info","context"});
            addExtern<DAS_BIND_FUN(server_init)>(*this, lib,  "server_init",
                SideEffects::modifyArgumentAndExternal, "server_init")
                    ->args({"server","port","context","at"});
            addExtern<DAS_BIND_FUN(server_is_open)>(*this, lib,  "server_is_open",
                SideEffects::modifyArgumentAndExternal, "server_is_open")
                    ->args({"server","context","at"});
            addExtern<DAS_BIND_FUN(server_is_connected)>(*this, lib,  "server_is_connected",
                SideEffects::modifyArgumentAndExternal, "server_is_connected")
                    ->args({"server","context","at"});
            addExtern<DAS_BIND_FUN(server_tick)>(*this, lib,  "server_tick",
                SideEffects::modifyArgumentAndExternal, "server_tick")
                    ->args({"server","context","at"});
            addExtern<DAS_BIND_FUN(server_send)>(*this, lib,  "server_send",
                SideEffects::modifyArgumentAndExternal, "server_send")
                    ->args({"server","data","size","context","at"});
            addExtern<DAS_BIND_FUN(server_restore)>(*this, lib,  "server_restore",
                SideEffects::modifyArgumentAndExternal, "server_restore")
                    ->args({"server","class","info","context","at"});
            // client
            addAnnotation(new ClientAnnotation(lib));
            addExtern<DAS_BIND_FUN(makeClient)>(*this, lib,  "make_client",
                SideEffects::modifyArgumentAndExternal, "makeClient")
                    ->args({"class","info","context"});
            addExtern<DAS_BIND_FUN(client_connect)>(*this, lib,  "client_connect",
                SideEffects::modifyArgumentAndExternal, "client_connect")
                    ->args({"client","host","port","timeout_ms","context","at"});
            addExtern<DAS_BIND_FUN(client_is_connected)>(*this, lib,  "client_is_connected",
                SideEffects::modifyArgumentAndExternal, "client_is_connected")
                    ->args({"client","context","at"});
            addExtern<DAS_BIND_FUN(client_tick)>(*this, lib,  "client_tick",
                SideEffects::modifyArgumentAndExternal, "client_tick")
                    ->args({"client","context","at"});
            addExtern<DAS_BIND_FUN(client_send)>(*this, lib,  "client_send",
                SideEffects::modifyArgumentAndExternal, "client_send")
                    ->args({"client","data","size","context","at"});
            addExtern<DAS_BIND_FUN(client_close)>(*this, lib,  "client_close",
                SideEffects::modifyArgumentAndExternal, "client_close")
                    ->args({"client","context","at"});
            addExtern<DAS_BIND_FUN(probe_local_port_das)>(*this, lib,  "probe_local_port",
                SideEffects::modifyExternal, "probe_local_port_das")
                    ->args({"host","port"});
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_network.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Network,das);
