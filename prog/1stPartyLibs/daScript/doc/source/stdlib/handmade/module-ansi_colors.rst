The ANSI_COLORS module provides helpers for wrapping strings with ANSI escape
codes for colored and styled terminal output.

Color output is controlled by the ``use_tty_colors`` variable. Call
``init_ansi_colors`` to auto-detect support from command-line flags
(``--color`` / ``--no-color``) and environment variables (``TERM``,
``NO_COLOR``), or set ``use_tty_colors`` directly.

All functions and symbols are in "ansi_colors" module, use require to get access to it.

.. code-block:: das

    require daslib/ansi_colors

Example:

.. code-block:: das

    require daslib/ansi_colors

    [export]
    def main() {
        init_ansi_colors()
        print("{red_str("error")}: something went wrong\n")
        print("{green_str("ok")}: all good\n")
        print("{bold_str("important")}: pay attention\n")
    }
