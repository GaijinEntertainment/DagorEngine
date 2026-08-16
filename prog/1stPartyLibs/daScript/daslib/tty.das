options gen2
options indenting = 4
options strict_smart_pointers = true

module tty shared public

//! Terminal capability probes — is a stream a real terminal, and how wide is it.
//!
//! This is the gate for anything that redraws itself with ``\r``: a progress bar written to a
//! pipe, a redirect, or a supervising process is garbage in the resulting log. Sniffing ``TERM``
//! is not enough — it stays set when stdout is redirected. ``daslib/ansi_colors`` handles
//! styling; this module answers the capability question underneath it.

require fio_core

//! Standard stream numbers, for ``is_stream_terminal``.
let TTY_STDIN = 0
let TTY_STDOUT = 1
let TTY_STDERR = 2

//! Width assumed by ``terminal_columns`` when nothing reports one.
let TTY_DEFAULT_WIDTH = 80

def is_stream_terminal(fd : int) : bool {
    //! True when the standard stream `fd` (`TTY_STDIN` / `TTY_STDOUT` / `TTY_STDERR`) is
    //! attached to a terminal. Any other descriptor is false.
    return is_terminal(fd)
}

def is_stdin_terminal() : bool {
    //! True when standard input is attached to a terminal.
    return is_terminal(TTY_STDIN)
}

def is_stdout_terminal() : bool {
    //! True when standard output is attached to a terminal. This is the gate for a redrawn
    //! progress display — false under a pipe, a redirect, or a supervising process.
    return is_terminal(TTY_STDOUT)
}

def is_stderr_terminal() : bool {
    //! True when standard error is attached to a terminal.
    return is_terminal(TTY_STDERR)
}

def terminal_columns(fallback = TTY_DEFAULT_WIDTH) : int {
    //! Terminal width in columns, substituting `fallback` when neither the terminal nor
    //! ``COLUMNS`` reports one. Pass `fallback = 0` for the raw probe.
    let w = terminal_width()
    return w > 0 ? w : fallback
}
