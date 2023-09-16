require debugapi

class LogAgent : DapiDebugAgent
    def override onLog ( context : Context?; at : LineInfo const?; level:int; text:string# ) : bool
        print("onLog - {level} - '{text}'\n'{context}' <{at}>\n")
        return true // false for not consuming

[export]
def logger_agent ( ctx : Context )
    install_new_debug_agent(new LogAgent(), "log_example")

[export]
def main
    to_log(LOG_TRACE,   "trace")
    to_log(LOG_DEBUG,   "debug")
    to_log(LOG_INFO,    "info")
    to_log(LOG_WARNING, "warning")
    to_log(LOG_ERROR,   "error")

    fork_debug_agent_context(@@logger_agent)

    to_log(LOG_TRACE,   "one more time, trace")
    to_log(LOG_DEBUG,   "one more time, debug")
    to_log(LOG_INFO,    "one more time, info")
    to_log(LOG_WARNING, "one more time, warning")
    to_log(LOG_ERROR,   "one more time, error")

