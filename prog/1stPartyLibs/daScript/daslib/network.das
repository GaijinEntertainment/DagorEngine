options gen2
options indenting = 4
options remove_unused_symbols = false
options no_aot

module network shared public

require network_core public
require daslib/rtti

class Server {
    _server : NetworkServer?
    def Server {
        pass
    }
    def make_server_adapter {
        let classInfo = class_info(self)
        if (!make_server(unsafe(addr(self)), classInfo)) {
            panic("can't make server")
        }
    }
    def init(port : int) : bool {
        return server_init(_server, port)
    }
    def restore(var shared_orphan : NetworkServer?&) {
        _server = shared_orphan
        shared_orphan = null
        let classInfo = class_info(self)
        server_restore(_server, unsafe(addr(self)), classInfo)
    }
    def save(var shared_orphan : NetworkServer?&) {
        shared_orphan = _server
        _server = null
    }
    def has_session : bool {
        return _server != null
    }
    def is_open : bool {
        return server_is_open(_server)
    }
    def is_connected : bool {
        return server_is_connected(_server)
    }
    def tick : void {
        if (_server != null) {
            server_tick(_server)
        }
    }
    def send(data : uint8?; size : int) : bool {
        return server_send(_server, data, size)
    }
    def operator delete {
        unsafe {
            delete _server
        }
    }
    def abstract onConnect : void
    def abstract onDisconnect : void
    def abstract onData(buf : uint8?; size : int) : void
    def abstract onError(msg : string; code : int) : void
    def abstract onLog(msg : string) : void
}

//! one outgoing TCP connection: `connect` resolves the host (a name, an IPv4 or IPv6 literal)
//! and blocks until the peer answers, refuses, or `timeout_ms` passes; then the socket is
//! non-blocking and `tick` hands every byte it holds to `onData`
class Client {
    _client : NetworkClient?
    def Client {
        pass
    }
    def make_client_adapter {
        let classInfo = class_info(self)
        if (!make_client(unsafe(addr(self)), classInfo)) {
            panic("can't make client")
        }
    }
    def connect(host : string; port : int; timeout_ms : int = 5000) : bool {
        return client_connect(_client, host, port, timeout_ms)
    }
    def is_connected : bool {
        return client_is_connected(_client)
    }
    def tick : void {
        if (_client != null) {
            client_tick(_client)
        }
    }
    def send(data : uint8?; size : int) : bool {
        return client_send(_client, data, size)
    }
    def close : void {
        if (_client != null) {
            client_close(_client)
        }
    }
    def operator delete {
        unsafe {
            delete _client
        }
    }
    def abstract onConnect : void
    def abstract onDisconnect : void
    def abstract onData(buf : uint8?; size : int) : void
    def abstract onError(msg : string; code : int) : void
    def abstract onLog(msg : string) : void
}

