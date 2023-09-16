#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/simulate/aot_builtin_network.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "module_builtin_rtti.h"

#include <atomic>

MAKE_TYPE_FACTORY(NetworkServer,Server)

namespace das {

    static atomic<int32_t> g_moduleNetworkTotalServers{0};
    class ServerAdapter : public Server {
    public:
        ServerAdapter(char * pClass, const StructInfo * info, Context * ctx ) {
            update(pClass,info,ctx);
            if ( !g_moduleNetworkTotalServers++ )
                Server::startup();
        }
        virtual ~ServerAdapter() {
            if ( !--g_moduleNetworkTotalServers )
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
                    (context,nullptr,fnOnError,classPtr,msg);
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

    #include "network.das.inc"

    bool makeServer ( const void * pClass, const StructInfo * info, Context * context ) {
        auto server = make_smart<ServerAdapter>((char *)pClass,info,context);
        if ( !server->isValid() ) return false;
        server.orphan();
        return true;
    }

    bool server_init ( smart_ptr_raw<Server> server, int port, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->init(port);
    }

    bool server_is_open ( smart_ptr_raw<Server> server, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->is_open();
    }

    bool server_is_connected ( smart_ptr_raw<Server> server, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->is_connected();
    }

    bool server_send ( smart_ptr_raw<Server> server, uint8_t * data, int32_t size, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        return server->send_msg((char *)data, size);
    }

    void server_tick ( smart_ptr_raw<Server> server, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        server->tick();
    }

    void server_restore ( smart_ptr_raw<Server> server, const void * pClass, const StructInfo * info, Context * context, LineInfoArg * at ) {
        if ( !server ) context->throw_error_at(at, "null server");
        auto adapter = (ServerAdapter *) server.get();
        adapter->update((char *)pClass,info,context);
    }

    class Module_Network : public Module {
    public:
        Module_Network() : Module("network") {
            DAS_PROFILE_SECTION("Module_Network");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti"));
            // sever
            addAnnotation(make_smart<ServerAnnotation>(lib));
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
            // add builtin module
            compileBuiltinModule("network.das",network_das,sizeof(network_das));
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_network.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Network,das);
