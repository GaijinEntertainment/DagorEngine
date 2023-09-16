require network
require fio
require strings

var
    isContDev = false

class TelnetServer : Server
    current_string : array<uint8>
    done : bool = false
    def TelnetServer
        Server`Server(cast<Server> self)
    def override onError(msg:string; code:int)
        print("server error:{code} - {msg}\n")
    def override onConnect
        print("connected\n")
    def override onDisconnect
        if length(current_string) > 0
            print(string(current_string))
            current_string |> clear()
        print("disconnected\n")
    def restart
        if _server != null
            var session : smart_ptr<NetworkServer>
            self->save(session)
            gc0_save_smart_ptr("telnet-session", session)
    def override onData(msg:uint8?; size:int)
        for i in range(size)
            var ch: uint8
            unsafe
                ch = msg[i]
            let ich = int(ch)
            if ich=='\r'
                continue
            elif ich=='\n'
                print("`{string(current_string)}` length={length(current_string)}\n")
                var new_msg = "length = {length(current_string)}\n"
                unsafe
                    self->send(reinterpret<uint8?> new_msg, length(new_msg))
                var str = string(current_string)
                if str=="quit"
                    done = true
                elif str=="restart"
                    if isContDev
                        print("restarting via telnet command")
                        done = true
                        self->restart()
                    else
                        print("restart request ignored, not running from dasContDev\n")
                current_string |> clear()
                unsafe
                    delete_string(str)
                    delete_string(new_msg)
            else
                current_string |> push(ch)

[export]
def main
    let args <- get_command_line_arguments()
    let fn = args[1]
    isContDev = true
    let main_time = stat(fn).mtime
    print("server at {fn} {main_time}\n")
    let port = 9000
    var telnet = new TelnetServer()
    telnet->make_server_adapter()
    var session : smart_ptr<NetworkServer> <- gc0_restore_smart_ptr("telnet-session")
    if session != null
        print("restoring server after restart\n")
        telnet->restore(session)
    elif telnet->init(9000)
        print("sever at port {port}\n")
    else
        print("server failed to initialize\n")
        return false
    while !telnet.done
        telnet->tick()
        let new_time = stat(fn).mtime
        if new_time != main_time
            print("restarting via source change {new_time}\n")
            telnet->restart()
            break
    unsafe
        delete telnet
    return true

[export]
def test
    let port = 9000
    print("sever at port {port}\n")
    var telnet = new TelnetServer()
    if !telnet->init(9000)
        return false
    while !telnet.done
        telnet->tick()
        sleep(0u)
    unsafe
        delete telnet
    return true
