options gen2
options rtti
options persistent_heap

require telegram/tbot
require daslib/fio
require strings

[export]
def main() {
    // read token from env var or .bot_token file
    var token = ""
    unsafe {
        token = get_env_variable("TELEGRAM_BOT_TOKEN")
    }
    if (empty(token)) {
        fopen(".bot_token", "rb") <| $(f) {
            if (f != null) {
                token = fread(f)
                token = strip(token)
            }
        }
    }
    if (empty(token)) {
        print("No bot token found. Set TELEGRAM_BOT_TOKEN or create .bot_token file\n")
        return
    }
    telegram_set_configuration(configuration(token = token))
    print("Echo bot started. Send me a message!\n")
    var offset = 0l
    while (true) {
        var updates <- telegram_getUpdates(get_updates(offset = offset, timeout = 30l, limit = 100l))
        let err = telegram_get_last_error()
        if (!empty(err)) {
            print("ERROR: {err}\n")
            break
        }
        for (u in updates) {
            offset = u.update_id + 1l
            if (u.message != null) {
                assume msg = *u.message
                let chat_id = "{msg.chat.id}"
                if (!empty(msg.text)) {
                    print("{msg._from.first_name}: {msg.text}\n")
                    telegram_sendMessage(send_message(chat_id = chat_id, text = msg.text))
                    let send_err = telegram_get_last_error()
                    if (!empty(send_err)) {
                        print("Send error: {send_err}\n")
                    }
                }
            }
        }
    }
}
