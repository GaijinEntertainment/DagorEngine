"using strict"

var cfg = new JsonRpcConfig()
cfg.missingMethodPolicy = cfg.MM_LOG
var jrpc = new simple_jsonrpc(cfg)
var socket = null

var newSocketListeners = []

function createSocket(reconnect) {
  socket = new WebSocket("ws://localhost:9000")
  for (let fn of newSocketListeners)
    fn.call(null, socket)

  var data = ""
  socket.onmessage = async event => {
    if (typeof (event.data) == "string") {
      jrpc.messageHandler(event.data)
      return
    }
    // binary socket
    let text = data
    text += await event.data.text()
    // split multiple json messages
    let start = 0
    let oc = 0
    let ac = 0
    let tryParse = false
    for (let i = 0; i < text.length; ++i) {
      let ch = text[i]
      if (ch == "{")
        oc += 1
      else if (ch == '[')
        ac += 1
      else if (ch == '}')
        oc -= 1
      else if (ch == ']')
        ac -= 1
      tryParse ||= oc > 0 || ac > 0
      if (tryParse && oc == 0 && ac == 0 && (ch == "}" || ch == "]")) {
        tryParse = false
        try {
          let sub = text.substring(start, i + 1)
          JSON.parse(sub)
          jrpc.messageHandler(sub)
          start = i + 1
        } catch (e) { }
      }
    }
    if (start < text.length - 1)
      data = text.substring(start)
  }
  jrpc.toStream = _msg => socket.send(_msg)

  socket.onopen = data => console.log("Socket connected: ", data)
  socket.onerror = error => console.error("socket error: " + error.message)

  socket.onclose = function (event) {
    if (event.wasClean) {
      console.info('Connection close was clean')
    } else {
      console.error('Connection suddenly close')
    }
    console.info('socket close code : ' + event.code + ' reason: ' + event.reason)
    if (data.length > 0)
      console.error("unhandled request data", data)
    if (reconnect) {
      console.log(`reconnect... `)
      setTimeout(createSocket, 3000, reconnect)
    }
  }
}
