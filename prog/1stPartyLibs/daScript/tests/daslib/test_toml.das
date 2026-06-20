options gen2

require dastest/testing_boost
require daslib/toml
require daslib/json
require strings

def expect_ok(t : T?; what : string; input : string) : JsonValue? {
    var err : string
    var v : JsonValue? = read_toml(input, err)
    if (v == null) {
        t |> failure("{what}: expected OK but got error: {err}")
    }
    return v
}

def expect_fail(t : T?; what : string; input : string) {
    var err : string
    var v : JsonValue? = read_toml(input, err)
    if (v != null) {
        t |> failure("{what}: expected parse failure but parsed OK")
        return
    }
    if (empty(err)) {
        t |> failure("{what}: returned null but no error message")
    }
}

def get_object_field(var v : JsonValue?; key : string) : JsonValue? {
    if (v == null || !(v.value is _object)) return null
    var obj & = unsafe(v.value as _object)
    if (!key_exists(obj, key)) return null
    return obj[key]
}

[test]
def test_empty(t : T?) {
    t |> run("empty input parses to empty table") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "empty", "")
        t |> success(v != null && (v.value is _object))
        if (v != null) {
            t |> equal(length(unsafe(v.value as _object)), 0)
        }
    }
}

[test]
def test_comments_only(t : T?) {
    t |> run("comments and blank lines only") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "comments", "# top comment\n\n# another\n")
        t |> success(v != null)
        if (v != null) {
            t |> equal(length(unsafe(v.value as _object)), 0)
        }
    }
}

[test]
def test_string_value(t : T?) {
    t |> run("basic string with escapes") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "string", "name = \"a\\tb\\nc\"\n")
        var s : JsonValue? = get_object_field(v, "name")
        t |> success(s != null && (s.value is _string))
        if (s != null && (s.value is _string)) {
            t |> equal(s.value as _string, "a\tb\nc")
        }
    }
}

[test]
def test_literal_string(t : T?) {
    t |> run("literal string preserves backslashes") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "literal", "p = 'C:\\path'\n")
        var s : JsonValue? = get_object_field(v, "p")
        if (s != null && (s.value is _string)) {
            t |> equal(s.value as _string, "C:\\path")
        }
    }
}

[test]
def test_int_decimal(t : T?) {
    t |> run("decimal integer with sign and _ separator") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "int", "a = -1\nb = 1_000_000\n")
        var a : JsonValue? = get_object_field(v, "a")
        var b : JsonValue? = get_object_field(v, "b")
        if (a != null && (a.value is _longint)) {
            t |> equal(a.value as _longint, -1l)
        }
        if (b != null && (b.value is _longint)) {
            t |> equal(b.value as _longint, 1000000l)
        }
    }
}

[test]
def test_int_radix(t : T?) {
    t |> run("hex / octal / binary integers") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "radix", "h = 0xff\no = 0o755\nb = 0b1010\n")
        var h : JsonValue? = get_object_field(v, "h")
        var o : JsonValue? = get_object_field(v, "o")
        var b : JsonValue? = get_object_field(v, "b")
        if (h != null && (h.value is _longint)) t |> equal(h.value as _longint, 255l)
        if (o != null && (o.value is _longint)) t |> equal(o.value as _longint, 493l)
        if (b != null && (b.value is _longint)) t |> equal(b.value as _longint, 10l)
    }
}

[test]
def test_float(t : T?) {
    t |> run("float with decimal point and exponent") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "float", "pi = 3.14\nsci = 1e6\n")
        var pi : JsonValue? = get_object_field(v, "pi")
        var sci : JsonValue? = get_object_field(v, "sci")
        if (pi != null && (pi.value is _number)) {
            t |> equal(pi.value as _number, 3.14lf)
        }
        if (sci != null && (sci.value is _number)) {
            t |> equal(sci.value as _number, 1000000.0lf)
        }
    }
}

[test]
def test_bool(t : T?) {
    t |> run("boolean values") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "bool", "x = true\ny = false\n")
        var x : JsonValue? = get_object_field(v, "x")
        var y : JsonValue? = get_object_field(v, "y")
        if (x != null && (x.value is _bool)) t |> success(x.value as _bool)
        if (y != null && (y.value is _bool)) t |> success(!(y.value as _bool))
    }
}

[test]
def test_array(t : T?) {
    t |> run("inline and multi-line arrays both work") @@(t : T?) {
        var v1 : JsonValue? = expect_ok(t, "inline", "xs = [1, 2, 3]\n")
        var v2 : JsonValue? = expect_ok(t, "multiline", "xs = [\n  1,\n  2,\n  3,\n]\n")
        for (vv in [v1, v2]) {
            var xs : JsonValue? = get_object_field(vv, "xs")
            if (xs != null && (xs.value is _array)) {
                let arr & = unsafe(xs.value as _array)
                t |> equal(length(arr), 3)
            }
        }
    }
}

[test]
def test_inline_table(t : T?) {
    t |> run("inline table single-line") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "inline_table", "p = \{ x = 1, y = 2 \}\n")
        var p : JsonValue? = get_object_field(v, "p")
        var x : JsonValue? = get_object_field(p, "x")
        var y : JsonValue? = get_object_field(p, "y")
        if (x != null && (x.value is _longint)) t |> equal(x.value as _longint, 1l)
        if (y != null && (y.value is _longint)) t |> equal(y.value as _longint, 2l)
    }
}

[test]
def test_section(t : T?) {
    t |> run("[section] table header") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "section", "[server]\nhost = \"localhost\"\nport = 8080\n")
        var server : JsonValue? = get_object_field(v, "server")
        var host : JsonValue? = get_object_field(server, "host")
        var port : JsonValue? = get_object_field(server, "port")
        if (host != null && (host.value is _string)) {
            t |> equal(host.value as _string, "localhost")
        }
        if (port != null && (port.value is _longint)) {
            t |> equal(port.value as _longint, 8080l)
        }
    }
}

[test]
def test_dotted_section(t : T?) {
    t |> run("[a.b] dotted section creates nested tables") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "dotted", "[a.b]\nv = 1\n")
        var a : JsonValue? = get_object_field(v, "a")
        var b : JsonValue? = get_object_field(a, "b")
        var vv : JsonValue? = get_object_field(b, "v")
        if (vv != null && (vv.value is _longint)) {
            t |> equal(vv.value as _longint, 1l)
        }
    }
}

[test]
def test_dotted_key(t : T?) {
    t |> run("a.b.c = 1 creates nested tables") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "dotted_key", "a.b.c = 1\n")
        var a : JsonValue? = get_object_field(v, "a")
        var b : JsonValue? = get_object_field(a, "b")
        var c : JsonValue? = get_object_field(b, "c")
        if (c != null && (c.value is _longint)) {
            t |> equal(c.value as _longint, 1l)
        }
    }
}

[test]
def test_array_of_tables(t : T?) {
    t |> run("[[items]] appends entries to an array") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "aot", "[[items]]\nx = 1\n[[items]]\nx = 2\n")
        var items : JsonValue? = get_object_field(v, "items")
        if (items != null && (items.value is _array)) {
            let arr & = unsafe(items.value as _array)
            t |> equal(length(arr), 2)
        }
    }
}

[test]
def test_datetime_preserved(t : T?) {
    t |> run("datetime values preserved as raw strings") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "datetime", "t = 2024-01-15T10:30:00Z\n")
        var dt : JsonValue? = get_object_field(v, "t")
        if (dt != null && (dt.value is _string)) {
            t |> equal(dt.value as _string, "2024-01-15T10:30:00Z")
        }
    }
}

[test]
def test_rules_shape(t : T?) {
    t |> run("[rules] table-of-bools (the .lint_config shape)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "rules", "[rules]\nSTYLE005 = true\nPERF007 = false\n")
        var r : JsonValue? = get_object_field(v, "rules")
        var s5 : JsonValue? = get_object_field(r, "STYLE005")
        var p7 : JsonValue? = get_object_field(r, "PERF007")
        if (s5 != null && (s5.value is _bool)) t |> success(s5.value as _bool)
        if (p7 != null && (p7.value is _bool)) t |> success(!(p7.value as _bool))
    }
}

[test]
def test_date_shaped_bare_key(t : T?) {
    t |> run("date-shaped bare key is accepted (per TOML ABNF)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "date_key", "2024-01-15 = 1\n")
        var dk : JsonValue? = get_object_field(v, "2024-01-15")
        if (dk != null && (dk.value is _longint)) {
            t |> equal(dk.value as _longint, 1l)
        }
    }
}

[test]
def test_inf_bare_key(t : T?) {
    t |> run("'inf' / 'nan' are valid bare keys (no leading sign)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "inf_key", "inf = 1\nnan = 2\n")
        var ik : JsonValue? = get_object_field(v, "inf")
        var nk : JsonValue? = get_object_field(v, "nan")
        if (ik != null && (ik.value is _longint)) t |> equal(ik.value as _longint, 1l)
        if (nk != null && (nk.value is _longint)) t |> equal(nk.value as _longint, 2l)
    }
}

[test]
def test_inf_prefix_bare_key(t : T?) {
    t |> run("identifiers starting with 'inf' / 'nan' are not split (infinity_count)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "inf_prefix", "infinity_count = 1\nnanoseconds = 2\n")
        var a : JsonValue? = get_object_field(v, "infinity_count")
        var b : JsonValue? = get_object_field(v, "nanoseconds")
        if (a != null && (a.value is _longint)) t |> equal(a.value as _longint, 1l)
        if (b != null && (b.value is _longint)) t |> equal(b.value as _longint, 2l)
    }
}

[test]
def test_dash_prefix_bare_key(t : T?) {
    t |> run("bare keys may start with '-' per TOML ABNF") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "dash_key", "-foo = 1\n")
        var dk : JsonValue? = get_object_field(v, "-foo")
        if (dk != null && (dk.value is _longint)) {
            t |> equal(dk.value as _longint, 1l)
        }
    }
}

[test]
def test_alphanum_bare_key(t : T?) {
    t |> run("bare keys may start with digits and contain alpha (123abc)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "alphanum_key", "123abc = 1\n0xff_oops = 2\n")
        var a : JsonValue? = get_object_field(v, "123abc")
        var b : JsonValue? = get_object_field(v, "0xff_oops")
        if (a != null && (a.value is _longint)) t |> equal(a.value as _longint, 1l)
        if (b != null && (b.value is _longint)) t |> equal(b.value as _longint, 2l)
    }
}

[test]
def test_fail_underscore_trailing(t : T?) {
    t |> run("trailing underscore in number is rejected") @@(t : T?) {
        expect_fail(t, "trail_underscore", "x = 42_\n")
    }
}

[test]
def test_fail_underscore_double(t : T?) {
    t |> run("doubled underscore in number is rejected") @@(t : T?) {
        expect_fail(t, "double_underscore", "x = 4__2\n")
    }
}

[test]
def test_fail_underscore_hex_leading(t : T?) {
    t |> run("leading underscore after radix prefix is rejected") @@(t : T?) {
        expect_fail(t, "hex_underscore", "x = 0x_ff\n")
    }
}

[test]
def test_fail_underscore_float_fractional(t : T?) {
    t |> run("trailing underscore in fractional part is rejected") @@(t : T?) {
        expect_fail(t, "frac_underscore", "x = 3.14_\n")
    }
}

[test]
def test_valid_underscores(t : T?) {
    t |> run("'_' between digits parses correctly across decimal / hex / float / exp") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "valid_underscores",
            "a = 1_000\nb = 0xff_aa\nc = 3.141_592\nd = 1_0e1_0\n")
        var a : JsonValue? = get_object_field(v, "a")
        var b : JsonValue? = get_object_field(v, "b")
        if (a != null && (a.value is _longint)) t |> equal(a.value as _longint, 1000l)
        if (b != null && (b.value is _longint)) t |> equal(b.value as _longint, int64(0xffaa))
    }
}

[test]
def test_fail_exponent_no_digits(t : T?) {
    t |> run("exponent without digits ('1e') is rejected") @@(t : T?) {
        expect_fail(t, "exp_no_digits", "x = 1e\n")
    }
}

[test]
def test_fail_exponent_sign_no_digits(t : T?) {
    t |> run("exponent with sign but no digits ('1e+') is rejected") @@(t : T?) {
        expect_fail(t, "exp_sign_no_digits", "x = 1e+\n")
    }
}

def expect_fail_with_eof(t : T?; what : string; input : string) {
    var err : string
    var v : JsonValue? = read_toml(input, err)
    if (v != null) {
        t |> failure("{what}: expected parse failure but parsed OK")
        return
    }
    // EOF diagnostics must NOT report the default '0:0' position and SHOULD
    // mention end-of-input so the user knows the input was truncated.
    if (find(err, "0:0") >= 0) {
        t |> failure("{what}: EOF error reports stale '0:0' location: '{err}'")
    }
    if (find(err, "end of input") < 0) {
        t |> failure("{what}: EOF error doesn't say 'end of input': '{err}'")
    }
}

[test]
def test_fail_eof_after_key_no_equals(t : T?) {
    t |> run("EOF after a top-level key reports 'end of input' (not '0:0')") @@(t : T?) {
        expect_fail_with_eof(t, "eof_no_equals", "a")
    }
}

[test]
def test_fail_eof_in_section_header(t : T?) {
    t |> run("EOF inside section header reports 'end of input'") @@(t : T?) {
        expect_fail_with_eof(t, "eof_section", "[a")
    }
}

[test]
def test_fail_eof_in_aot_header(t : T?) {
    t |> run("EOF inside [[items header reports 'end of input'") @@(t : T?) {
        expect_fail_with_eof(t, "eof_aot", "[[a]")
    }
}

[test]
def test_fail_eof_in_inline_table(t : T?) {
    t |> run("EOF inside inline table reports 'end of input'") @@(t : T?) {
        expect_fail_with_eof(t, "eof_inline", "x = \{ a")
    }
}

[test]
def test_fail_unclosed_string(t : T?) {
    t |> run("unterminated basic string is rejected") @@(t : T?) {
        expect_fail(t, "unclosed", "name = \"oops\n")
    }
}

[test]
def test_fail_unclosed_array(t : T?) {
    t |> run("unterminated array is rejected") @@(t : T?) {
        expect_fail(t, "unclosed_array", "xs = [1, 2\n")
    }
}

[test]
def test_fail_duplicate_key(t : T?) {
    t |> run("duplicate key in same table is rejected") @@(t : T?) {
        expect_fail(t, "dup", "a = 1\na = 2\n")
    }
}

[test]
def test_fail_missing_equals(t : T?) {
    t |> run("key without = is rejected") @@(t : T?) {
        expect_fail(t, "no_equals", "key value\n")
    }
}

// ---------------------------------------------------------------------------
// TOML 1.0 closed-table semantics (F4/F5/F6)
// ---------------------------------------------------------------------------

def expect_fail_with(t : T?; what : string; input : string; needle : string) {
    var err : string
    var v : JsonValue? = read_toml(input, err)
    if (v != null) {
        t |> failure("{what}: expected parse failure but parsed OK")
        return
    }
    if (find(err, needle) < 0) {
        t |> failure("{what}: expected error to contain '{needle}', got '{err}'")
    }
}

[test]
def test_fail_section_after_dotted_implicit(t : T?) {
    // F4: [a]\nb.c=1 implicitly creates 'a.b' via dotted key. [a.b] then tries
    // to re-open that implicit table — invalid per TOML 1.0.
    t |> run("[a.b] after `b.c = 1` under [a] is rejected") @@(t : T?) {
        expect_fail_with(t, "implicit_then_header",
            "[a]\nb.c = 1\n[a.b]\n",
            "implicit")
    }
}

[test]
def test_ok_subtable_header_after_dotted(t : T?) {
    // The complementary VALID case: a NEW sub-sub-table header is still OK after
    // dotted-key creation of an intermediate.
    t |> run("[a.b.d] after `b.c = 1` under [a] is accepted (new subtree)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "subsubtable",
            "[a]\nb.c = 1\n[a.b.d]\nx = 2\n")
        var a : JsonValue? = get_object_field(v, "a")
        var b : JsonValue? = get_object_field(a, "b")
        var c : JsonValue? = get_object_field(b, "c")
        var d : JsonValue? = get_object_field(b, "d")
        var x : JsonValue? = get_object_field(d, "x")
        if (c != null && (c.value is _longint)) t |> equal(c.value as _longint, 1l)
        if (x != null && (x.value is _longint)) t |> equal(x.value as _longint, 2l)
    }
}

[test]
def test_ok_parent_header_after_child(t : T?) {
    // Opening a parent header after a child header is permitted (the parent
    // was created via header walk, not dotted-key, so it remains openable).
    t |> run("[a] after [a.b] is accepted (parent after child)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "parent_after_child",
            "[a.b]\nx = 1\n[a]\ny = 2\n")
        var a : JsonValue? = get_object_field(v, "a")
        var b : JsonValue? = get_object_field(a, "b")
        var x : JsonValue? = get_object_field(b, "x")
        var y : JsonValue? = get_object_field(a, "y")
        if (x != null && (x.value is _longint)) t |> equal(x.value as _longint, 1l)
        if (y != null && (y.value is _longint)) t |> equal(y.value as _longint, 2l)
    }
}

[test]
def test_fail_dotted_key_extends_inline_table(t : T?) {
    // F5: inline tables are immutable per TOML 1.0; once `a = { x = 1 }` is
    // written, `a.y = 2` cannot extend it.
    t |> run("`a = \{x=1\}` followed by `a.y = 2` is rejected (inline table closed)") @@(t : T?) {
        expect_fail_with(t, "extend_inline_toplevel",
            "a = \{ x = 1 \}\na.y = 2\n",
            "inline")
    }
}

[test]
def test_fail_dotted_key_extends_inline_under_section(t : T?) {
    // F5: same rule inside a section header.
    t |> run("`a = \{x=1\}` then `a.y = 2` under [t] is rejected") @@(t : T?) {
        expect_fail_with(t, "extend_inline_section",
            "[t]\na = \{ x = 1 \}\na.y = 2\n",
            "inline")
    }
}

[test]
def test_fail_section_header_into_inline_table(t : T?) {
    // A [section] header cannot open a path under an inline-table value either.
    t |> run("[a.sub] after `a = \{x=1\}` is rejected") @@(t : T?) {
        expect_fail_with(t, "header_into_inline",
            "a = \{ x = 1 \}\n[a.sub]\ny = 2\n",
            "inline")
    }
}

[test]
def test_fail_inline_internal_dotted_extends_inline(t : T?) {
    // F6: same rule applies WITHIN an inline-table literal — once a key is
    // bound to an inline-table value, a later dotted key cannot extend it.
    t |> run("inline `\{ p = \{x=1\}, p.y = 2 \}` is rejected") @@(t : T?) {
        expect_fail_with(t, "inline_extends_inline",
            "a = \{ p = \{ x = 1 \}, p.y = 2 \}\n",
            "inline")
    }
}

[test]
def test_ok_inline_dotted_keys_share_implicit(t : T?) {
    // The complementary VALID inline case: two dotted keys sharing an implicit
    // intermediate (the spec allows this; only the inline-literal closure is
    // tight).
    t |> run("inline `\{ p.x = 1, p.y = 2 \}` is accepted (shared implicit `p`)") @@(t : T?) {
        var v : JsonValue? = expect_ok(t, "inline_shared_implicit",
            "a = \{ p.x = 1, p.y = 2 \}\n")
        var a : JsonValue? = get_object_field(v, "a")
        var p : JsonValue? = get_object_field(a, "p")
        var x : JsonValue? = get_object_field(p, "x")
        var y : JsonValue? = get_object_field(p, "y")
        if (x != null && (x.value is _longint)) t |> equal(x.value as _longint, 1l)
        if (y != null && (y.value is _longint)) t |> equal(y.value as _longint, 2l)
    }
}

[test]
def test_fail_plus_prefix_identifier(t : T?) {
    // F13: `+foo = 1` previously emitted an empty `_bare` token because the
    // rewind-to-bare path lands the cursor on the '+' (which is not a
    // bare-key char) and `read_bare_key` consumes nothing. '+' is not a valid
    // bare-key prefix per TOML ABNF, so the correct outcome is a parse error
    // that mentions the stray sign.
    t |> run("`+foo = 1` is rejected (stray '+', not empty bare key)") @@(t : T?) {
        expect_fail_with(t, "plus_prefix_bare", "+foo = 1\n", "'+'")
    }
}

[test]
def test_fail_plus_inf_with_bare_tail(t : T?) {
    // F13 cousin: `+infinity_count = 1` enters classify_number_or_datetime
    // via the value-continuation gate, consumes '+inf', sees a bare-char
    // continuation, then rewinds. Because the rewind lands on '+' the bare
    // scan would be empty — must error instead.
    t |> run("`+infinity_count = 1` is rejected (stray '+')") @@(t : T?) {
        expect_fail_with(t, "plus_inf_bare_tail", "+infinity_count = 1\n", "'+'")
    }
}

[test]
def test_fail_date_followed_by_colon(t : T?) {
    // F16: `2024-01-15:00` is a malformed datetime (missing the T/space
    // separator). Surface a dedicated "expected 'T' or space" diagnostic
    // rather than letting the parser hit ':' as an "unexpected character".
    t |> run("`x = 2024-01-15:00` errors with a date-specific message") @@(t : T?) {
        expect_fail_with(t, "date_colon", "x = 2024-01-15:00\n", "expected 'T' or space")
    }
}

[test]
def test_fail_time_trailing_colon(t : T?) {
    // F16: `12:34:56:` is a malformed time (trailing colon). Surface a
    // dedicated message anchored at the time value.
    t |> run("`x = 12:34:56:` errors with a time-specific message") @@(t : T?) {
        expect_fail_with(t, "time_trailing_colon", "x = 12:34:56:\n", "trailing ':'")
    }
}
