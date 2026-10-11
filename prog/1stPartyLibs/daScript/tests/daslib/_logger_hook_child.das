options gen2

// the runtime hook's probe process test_logger.das spawns: install the hook on the log path the
// last argument names (an `echo` argument before it turns the stderr echo on), log one line
// through it, exit - the global debug agent dies with the process

require daslib/logger

[export]
def main() {
    let args <- get_command_line_arguments()
    logger_set_path(args[length(args) - 1])
    logger_set_echo_stderr(length(args) >= 2 && args[length(args) - 2] == "echo")
    logger_install_hook()
    to_log(LOG_INFO, "hook at-field probe\n")
    logger_close()
}
