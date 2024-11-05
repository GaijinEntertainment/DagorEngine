require webSocket

require rtti

class WebSocket
  _websocket : smart_ptr<WebSocketServer>
  def make_server_adapter()
    let classInfo = class_info(self)
    unsafe
      if !make_websocket(addr(self), classInfo)
        panic("can't make WebSocket")
  def init(port : int) : bool
    return websocket_init(_websocket, port)
  def init_path(port : int; document_root : string) : bool
    return websocket_init_path(_websocket, port, document_root)
  def restore(var shared_orphan : smart_ptr<WebSocketServer>&)
    _websocket <- shared_orphan
    let classInfo = class_info(self)
    unsafe
      websocket_restore(_websocket, addr(self), classInfo)
  def save(var shared_orphan : smart_ptr<WebSocketServer>&)
    shared_orphan <- _websocket
  def has_session : bool
    return _websocket != null
  def is_open : bool
    return websocket_is_open(_websocket)
  def is_connected : bool
    return websocket_is_connected(_websocket)
  def finalize()
    unsafe
      delete _websocket
  def tick : void
    if _websocket != null
      websocket_tick(_websocket)
  def send(data : uint8?; size : int) : bool
    return websocket_send(_websocket, data, size)
  def abstract onConnect : void
  def abstract onDisconnect : void
  def abstract onData(buf : uint8?; size : int) : void
  def abstract onError(msg : string; code : int) : void

