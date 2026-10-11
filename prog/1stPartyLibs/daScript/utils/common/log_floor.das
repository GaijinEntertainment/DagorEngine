options gen2

module log_floor

require daslib/fio

def public default_log_floor_to_info() {
    //! to_log drops records below DAS_LOG_LEVEL (default warning); an unset floor becomes info, for this process and the programs it starts
    if (empty(get_env_variable("DAS_LOG_LEVEL"))) {
        set_env_variable("DAS_LOG_LEVEL", "info")
    }
}
