options indenting=4
options remove_unused_symbols = false

require network

require rtti

class Server
    _server : smart_ptr<NetworkServer>
    def Server
        pass
    def make_server_adapter
        let classInfo = class_info(self)
        unsafe
            if !make_server(addr(self),classInfo)
                panic("can't make server")
    def init ( port : int ) : bool
        return server_init(_server,port)
    def restore ( var shared_orphan : smart_ptr<NetworkServer>& )
        _server <- shared_orphan
        let classInfo = class_info(self)
        unsafe
            server_restore(_server,addr(self),classInfo)
    def save ( var shared_orphan : smart_ptr<NetworkServer>& )
        shared_orphan <- _server
    def has_session : bool
        return _server != null
    def is_open : bool
        return server_is_open(_server)
    def is_connected : bool
        return server_is_connected(_server)
    def tick : void
        if _server != null
            server_tick(_server)
    def send ( data : uint8?; size:int ) : bool
        return server_send(_server, data, size)
    def operator delete
        unsafe
            delete _server
    def abstract onConnect : void
    def abstract onDisconnect : void
    def abstract onData ( buf : uint8?; size : int ) : void
    def abstract onError ( msg : string; code : int ) : void
    def abstract onLog ( msg : string ) : void

