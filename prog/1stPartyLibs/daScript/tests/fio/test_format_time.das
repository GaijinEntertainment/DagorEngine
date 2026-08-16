options gen2

require dastest/testing_boost

// `format_time(clock, fmt)` renders a clock as local-time text via strftime.
// Assertions stay timezone/DST-independent: at 09:22 the date part is stable (the
// largest DST normalization is one hour, which never carries 09:22 across midnight),
// and the formatting mechanism (default pattern, literals, %% escaping) is tz-agnostic.

[test]
def test_format_time(t : T?) {
    let c = mktime(2026, 6, 13, 9, 22, 45)
    t |> run("date part formats as YYYY-MM-DD") @(t : T?) {
        t |> success(format_time(c, "%Y-%m-%d") == "2026-06-13")
        t |> success(format_time(c, "%Y") == "2026")
    }
    t |> run("empty format falls back to the default pattern") @(t : T?) {
        t |> success(format_time(c, "") == format_time(c, "%Y-%m-%d %H:%M"))
    }
    t |> run("non-specifier text and %% pass through literally") @(t : T?) {
        t |> success(format_time(c, "hello %% world") == "hello % world")
    }
}
