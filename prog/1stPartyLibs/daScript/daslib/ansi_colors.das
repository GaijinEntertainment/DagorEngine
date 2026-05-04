options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module ansi_colors shared public

//! ANSI terminal color and style helpers.
//!
//! Provides functions for wrapping strings with ANSI escape codes
//! for use in terminal output: ``red_str``, ``green_str``, ``yellow_str``,
//! ``blue_str``, ``cyan_str``, ``magenta_str``, ``bold_str``, ``dim_str``,
//! ``underline_str``, and ``reset_str``.
//!
//! Color output is controlled by ``use_tty_colors``. Call ``init_ansi_colors``
//! to auto-detect based on ``--color`` / ``--no-color`` flags and ``TERM`` /
//! ``NO_COLOR`` environment variables, or set ``use_tty_colors`` directly.

require daslib/fio
require strings

var use_tty_colors = false

def init_ansi_colors() {
    //! Auto-detect color support from command-line arguments.
    //! Equivalent to ``init_ansi_colors(get_command_line_arguments())``.
    init_ansi_colors(get_command_line_arguments())
}

def init_ansi_colors(args : array<string>) {
    //! Detect color support from `args`, ``TERM``, and ``NO_COLOR`` environment variables.
    //! Sets ``use_tty_colors`` accordingly. Respects ``--color``, ``--no-color``, and ``NO_COLOR``.
    let no_color = has_env_variable("NO_COLOR") ? get_env_variable("NO_COLOR") : "0"
    let force_no_color = no_color != "0" && no_color != "false"
    if (force_no_color || has_value(args, "--no-color")) {
        use_tty_colors = false
    } else {
        let term = get_env_variable("TERM")
        let probably_color = length(term) > 3 && (find(term, "xterm") >= 0 || find(term, "color") >= 0 || find(term, "ansi") >= 0 || find(term, "vt100") >= 0)
        use_tty_colors = has_value(args, "--color") || probably_color
    }
}

def red_str(str : string) : string {
    //! Wraps `str` with ANSI red (``\\x1B[31m``) if colors are enabled.
    return use_tty_colors ? "\x1B[31m{str}\x1B[0m" : str
}

def green_str(str : string) : string {
    //! Wraps `str` with ANSI green (``\\x1B[32m``) if colors are enabled.
    return use_tty_colors ? "\x1B[32m{str}\x1B[0m" : str
}

def yellow_str(str : string) : string {
    //! Wraps `str` with ANSI yellow (``\\x1B[33m``) if colors are enabled.
    return use_tty_colors ? "\x1B[33m{str}\x1B[0m" : str
}

def blue_str(str : string) : string {
    //! Wraps `str` with ANSI blue (``\\x1B[34m``) if colors are enabled.
    return use_tty_colors ? "\x1B[34m{str}\x1B[0m" : str
}

def magenta_str(str : string) : string {
    //! Wraps `str` with ANSI magenta (``\\x1B[35m``) if colors are enabled.
    return use_tty_colors ? "\x1B[35m{str}\x1B[0m" : str
}

def cyan_str(str : string) : string {
    //! Wraps `str` with ANSI cyan (``\\x1B[36m``) if colors are enabled.
    return use_tty_colors ? "\x1B[36m{str}\x1B[0m" : str
}

def bold_str(str : string) : string {
    //! Wraps `str` with ANSI bold (``\\x1B[1m``) if colors are enabled.
    return use_tty_colors ? "\x1B[1m{str}\x1B[0m" : str
}

def dim_str(str : string) : string {
    //! Wraps `str` with ANSI dim (``\\x1B[2m``) if colors are enabled.
    return use_tty_colors ? "\x1B[2m{str}\x1B[0m" : str
}

def underline_str(str : string) : string {
    //! Wraps `str` with ANSI underline (``\\x1B[4m``) if colors are enabled.
    return use_tty_colors ? "\x1B[4m{str}\x1B[0m" : str
}

def reset_str() : string {
    //! Returns the ANSI reset sequence if colors are enabled, empty string otherwise.
    return use_tty_colors ? "\x1B[0m" : ""
}

def private has_value(args : array<string>; val : string) : bool {
    for (a in args) {
        if (a == val) {
            return true
        }
    }
    return false
}
