options gen2
require dastest/testing_boost
require strings

// Regression pins for the byte-view string surface.

// Every haystack goes through heap_str: with all-constant arguments the optimizer const-folds
// the whole call away, so the pin would freeze the folder instead of the runtime builtin.
def private heap_str(src : string) : string {
    return build_string() $(var w) {
        w |> write(src)
    }
}

// reinterpret to uint8 const?, not to uint64 - the LLVM JIT emits an invalid `bitcast ptr to i64`
// for a string-to-integer reinterpret (llvm_jit.das:5537 tests isPointer, which a string is not)
def private ptr_of(s : string) : uint8 const? => unsafe(reinterpret<uint8 const?>(s))

def private is_null_str(s : string) : bool => empty(s)

def private ptr_delta(src, res : string) : int64 => ptr_of(res) - ptr_of(src)

[test]
def test_pin_slice(t : T?) {
    let s = heap_str("abcdef")
    let none = heap_str("")
    t |> equal(slice(s, 1, 3), "bc", "slice inner range")
    t |> equal(slice(s, 0, 6), s, "slice whole string")
    t |> equal(slice(s, -2, -1), "e", "slice both indices negative-wrap")
    t |> equal(slice(s, -100, 100), s, "slice clamps after the negative wrap")
    t |> success(is_null_str(slice(s, 3, 1)), "slice start > end is null")
    t |> success(is_null_str(slice(s, 0, 0)), "slice empty range is null")
    t |> success(is_null_str(slice(s, 6, 6)), "slice at end is null")
    t |> success(is_null_str(slice(s, 100, 200)), "slice fully past end is null")
    t |> success(is_null_str(slice(s, -1, -100)), "slice wrapped start > wrapped end is null")
    t |> success(is_null_str(slice(none, 0, 1)), "slice of empty string is null")
    t |> equal(slice(s, 2), "cdef", "slice-to-end")
    t |> equal(slice(s, -2), "ef", "slice-to-end negative-wrap")
    t |> equal(slice(s, -100), s, "slice-to-end clamps after the negative wrap")
    t |> success(is_null_str(slice(s, 6)), "slice-to-end at length is null")
    t |> success(is_null_str(slice(s, 100)), "slice-to-end past end is null")
    t |> success(is_null_str(slice(none, 0)), "slice-to-end of empty string is null")
}

[test]
def test_pin_chop(t : T?) {
    let s = heap_str("abcdef")
    let none = heap_str("")
    t |> equal(chop(s, 1, 3), "bcd", "chop inner range")
    t |> equal(chop(s, 1, 100), "bcdef", "chop clamps the length to what is left")
    // chop does NOT negative-wrap the start the way slice does; it clamps to 0
    t |> equal(chop(s, -3, 2), "ab", "chop clamps a negative start to 0")
    t |> success(is_null_str(chop(s, 0, 0)), "chop zero length is null")
    t |> success(is_null_str(chop(s, 0, -1)), "chop negative length is null")
    t |> success(is_null_str(chop(s, 6, 1)), "chop starting at length is null")
    t |> success(is_null_str(chop(s, 100, 1)), "chop starting past end is null")
    t |> success(is_null_str(chop(none, 0, 1)), "chop of empty string is null")
}

[test]
def test_pin_find_substring(t : T?) {
    let s = heap_str("abcabc")
    let none = heap_str("")
    t |> equal(find(s, "bc"), 1, "find substring hit returns the first offset")
    t |> equal(find(s, "zz"), -1, "find substring miss")
    t |> equal(find(s, ""), -1, "find with empty needle is -1, not 0")
    t |> equal(find(none, "a"), -1, "find in empty string")
    t |> equal(find(none, ""), -1, "find empty in empty")
    t |> equal(find(s, "abcabc"), 0, "find whole-string needle")
    t |> equal(find(s, "abcabcd"), -1, "find needle longer than haystack")
    t |> equal(find(s, "bc", 0), 1, "find-from start 0")
    t |> equal(find(s, "bc", 2), 4, "find-from skips the earlier hit")
    t |> equal(find(s, "bc", 5), -1, "find-from past the last hit")
    t |> equal(find(s, "bc", 100), -1, "find-from start past length")
    // the substring form CLAMPS a negative start to 0 - the char form wraps instead
    t |> equal(find(s, "abc", -1), 0, "find-from clamps a negative start to 0")
    t |> equal(find(s, "", 2), -1, "find-from with empty needle is -1")
}

// a high byte cannot be written as a das literal, so it is assembled through the writer
def private heap_high_byte() : string {
    return build_string() $(var w) {
        w |> write("ab")
        w |> write_char(195)
        w |> write("cd")
    }
}

[test]
def test_pin_find_char_byte_value(t : T?) {
    // the char form matches an UNSIGNED byte value: 195 is byte 0xC3, and -61 - which is that
    // same byte read as a signed char - matches nothing
    let s = heap_high_byte()
    t |> equal(find(s, 195), 2, "find char matches the unsigned byte value")
    t |> equal(find(s, -61), -1, "find char does not match the signed reading of that byte")
    t |> equal(find(s, 256), -1, "find char above the byte range is -1")
}

[test]
def test_pin_find_char(t : T?) {
    let s = heap_str("abcabc")
    let none = heap_str("")
    t |> equal(find(s, 'c'), 2, "find char hit")
    t |> equal(find(s, 'z'), -1, "find char miss")
    t |> equal(find(none, 'a'), -1, "find char in empty string")
    t |> equal(find(s, 0), -1, "find NUL never hits - the scan is strlen-bounded")
    t |> equal(find(s, 'c', 0), 2, "find char from 0")
    t |> equal(find(s, 'c', 3), 5, "find char from skips the earlier hit")
    t |> equal(find(s, 'c', 6), -1, "find char from length")
    t |> equal(find(s, 'c', 100), -1, "find char from past length")
    // the char form WRAPS a negative start (len + start) - the substring form clamps
    t |> equal(find(s, 'a', -1), -1, "find char negative start wraps to len-1")
    t |> equal(find(s, 'a', -6), 0, "find char negative start wraps to 0")
}

[test]
def test_pin_rfind(t : T?) {
    let s = heap_str("abcabc")
    let none = heap_str("")
    t |> equal(rfind(s, "bc"), 4, "rfind returns the last offset")
    t |> equal(rfind(s, "zz"), -1, "rfind miss")
    t |> equal(rfind(s, ""), -1, "rfind with empty needle is -1")
    t |> equal(rfind(none, "a"), -1, "rfind in empty string")
    t |> equal(rfind(s, "abcabc"), 0, "rfind whole-string needle")
    t |> equal(rfind(s, "abcabcd"), -1, "rfind needle longer than haystack")
    t |> equal(rfind(s, "bc", 5), 4, "rfind-from start is the highest allowed match offset")
    t |> equal(rfind(s, "bc", 4), 4, "rfind-from exactly at the hit")
    t |> equal(rfind(s, "bc", 3), 1, "rfind-from below the last hit")
    t |> equal(rfind(s, "bc", 0), -1, "rfind-from 0 only tests offset 0")
    t |> equal(rfind(s, "bc", 100), 4, "rfind-from past length clamps to the last fitting offset")
    // ruling: a negative start is -1 in both forms - "search backward from before the start" has no answer
    t |> equal(rfind(s, "bc", -1), -1, "rfind-from a negative start is -1")
    t |> equal(rfind(s, "abc", -1), -1, "rfind-from a negative start is -1 even where offset 0 matches")
    t |> equal(rfind(s, "", 3), -1, "rfind-from with empty needle is -1")
}

[test]
def test_pin_starts_with(t : T?) {
    let s = heap_str("abcdef")
    let none = heap_str("")
    t |> success(starts_with(s, "abc"), "starts_with match")
    t |> success(!starts_with(s, "abd"), "starts_with mismatch")
    t |> success(starts_with(s, ""), "starts_with empty cmp is true")
    t |> success(!starts_with(s, "abcdefg"), "starts_with cmp longer than str")
    t |> success(starts_with(s, "abcdef"), "starts_with whole string")
    t |> success(!starts_with(none, "a"), "starts_with on empty str")
    t |> success(starts_with(none, ""), "starts_with empty on empty is true")
    t |> success(starts_with(s, "abcXX", 3u), "starts_with cmpLen limits the comparison")
    t |> success(!starts_with(s, "abcXX", 5u), "starts_with cmpLen covering the mismatch")
    t |> success(starts_with(s, "abc", 100u), "starts_with cmpLen is clamped to the cmp length")
    t |> success(starts_with(s, "zzz", 0u), "starts_with cmpLen 0 is always true")
    t |> success(starts_with(s, "", 3u), "starts_with empty cmp ignores cmpLen")
}

[test]
def test_pin_starts_with_offset(t : T?) {
    let s = heap_str("abcdef")
    t |> success(starts_with(s, 3, "def"), "starts_with at offset")
    t |> success(!starts_with(s, 3, "dee"), "starts_with at offset mismatch")
    t |> success(starts_with(s, 0, "abc"), "starts_with at offset 0")
    t |> success(starts_with(s, 5, "f"), "starts_with at the last offset")
    t |> success(!starts_with(s, 5, "fg"), "starts_with at offset overshooting the tail")
    t |> success(!starts_with(s, -1, "a"), "starts_with negative offset is false")
    t |> success(!starts_with(s, 100, "a"), "starts_with offset past length is false")
    t |> success(starts_with(s, 3, ""), "starts_with empty cmp at a valid offset is true")
    // the offset bound is checked BEFORE the empty-cmp shortcut, so offset == length is false
    t |> success(!starts_with(s, 6, ""), "starts_with empty cmp at offset == length is false")
    t |> success(starts_with(s, 3, "defXX", 3u), "starts_with offset + cmpLen")
    t |> success(!starts_with(s, 3, "defXX", 5u), "starts_with offset + cmpLen covering the mismatch")
    t |> success(starts_with(s, 3, "def", 100u), "starts_with offset + clamped cmpLen")
    t |> success(starts_with(s, 3, "zzz", 0u), "starts_with offset + cmpLen 0 is true")
    t |> success(!starts_with(s, 100, "a", 1u), "starts_with offset past length is false")
    t |> success(!starts_with(s, 6, "a", 0u), "starts_with offset == length is false even with cmpLen 0")
}

[test]
def test_pin_ends_with(t : T?) {
    let s = heap_str("abcdef")
    let none = heap_str("")
    t |> success(ends_with(s, "def"), "ends_with match")
    t |> success(!ends_with(s, "dee"), "ends_with mismatch")
    t |> success(ends_with(s, ""), "ends_with empty cmp is true")
    t |> success(!ends_with(s, "zabcdef"), "ends_with cmp longer than str")
    t |> success(ends_with(s, "abcdef"), "ends_with whole string")
    t |> success(!ends_with(none, "a"), "ends_with on empty str")
    t |> success(ends_with(none, ""), "ends_with empty on empty is true")
}

[test]
def test_pin_das_string_overloads(t : T?) {
    with_das_string() $(var ds) {
        ds := heap_str("abcdef")
        t |> success(starts_with(ds, "abc"), "das_string starts_with match")
        t |> success(!starts_with(ds, "abd"), "das_string starts_with mismatch")
        t |> success(starts_with(ds, ""), "das_string starts_with empty cmp is true")
        t |> success(!starts_with(ds, "abcdefg"), "das_string starts_with cmp longer than str")
        t |> success(ends_with(ds, "def"), "das_string ends_with match")
        t |> success(!ends_with(ds, "dee"), "das_string ends_with mismatch")
        t |> success(ends_with(ds, ""), "das_string ends_with empty cmp is true")
        t |> success(!ends_with(ds, "zabcdef"), "das_string ends_with cmp longer than str")
    }
}

def private pin_strip_removes(t : T?; ch, what : string) {
    let s = heap_str("{ch}ab{ch}")
    t |> equal(strip(s), "ab", "strip removes {what}")
    t |> equal(strip_left(s), "ab{ch}", "strip_left removes leading {what}")
    t |> equal(strip_right(s), "{ch}ab", "strip_right removes trailing {what}")
}

[test]
def test_pin_strip_whitespace_set(t : T?) {
    pin_strip_removes(t, " ", "space")
    pin_strip_removes(t, "\t", "tab")
    pin_strip_removes(t, "\n", "newline")
    pin_strip_removes(t, "\r", "carriage return")
    pin_strip_removes(t, "\f", "form feed")
    pin_strip_removes(t, "\v", "vertical tab")
}

[test]
def test_pin_strip_degenerate(t : T?) {
    let ws = heap_str("   ")
    let none = heap_str("")
    let plain = heap_str("ab")
    t |> success(is_null_str(strip(ws)), "strip of all-whitespace is null")
    t |> success(is_null_str(strip_left(ws)), "strip_left of all-whitespace is null")
    t |> success(is_null_str(strip_right(ws)), "strip_right of all-whitespace is null")
    t |> success(is_null_str(strip(none)), "strip of empty is null")
    t |> success(is_null_str(strip_left(none)), "strip_left of empty is null")
    t |> success(is_null_str(strip_right(none)), "strip_right of empty is null")
    t |> equal(strip(plain), "ab", "strip with nothing to remove")
    t |> equal(strip_left(plain), "ab", "strip_left with nothing to remove")
    t |> equal(strip_right(plain), "ab", "strip_right with nothing to remove")
}

def private pin_skip_removes(t : T?; ch, what : string) {
    let s = heap_str("{ch}ab")
    t |> equal(skip_white_space(s, 0), 1, "skip_white_space skips {what}")
    peek_data(s) $(d) {
        t |> equal(skip_white_space(d, 0), 1, "view skip_white_space skips {what}")
    }
}

[test]
def test_pin_skip_white_space_set(t : T?) {
    // skip_white_space is strip_left as a cursor, so it rides the same C isspace 6-set
    pin_skip_removes(t, " ", "space")
    pin_skip_removes(t, "\t", "tab")
    pin_skip_removes(t, "\n", "newline")
    pin_skip_removes(t, "\r", "carriage return")
    pin_skip_removes(t, "\f", "form feed")
    pin_skip_removes(t, "\v", "vertical tab")
}

[test]
def test_pin_skip_white_space_bounds(t : T?) {
    let s = heap_str("  ab  cd")
    let ws = heap_str("   ")
    let none = heap_str("")
    var inscope empty_view : array<uint8>
    t |> equal(skip_white_space(s, 0), 2, "skip from 0 lands on the first content byte")
    t |> equal(skip_white_space(s, 2), 2, "non-whitespace at from returns from")
    t |> equal(skip_white_space(s, 4), 6, "skip from mid-string crosses the interior run")
    t |> equal(skip_white_space(s, -100), 2, "a negative from clamps to 0")
    t |> equal(skip_white_space(s, 8), 8, "from == length returns length")
    t |> equal(skip_white_space(s, 100), 8, "from past length clamps to length")
    t |> equal(skip_white_space(ws, 0), 3, "an all-whitespace tail returns length")
    t |> equal(skip_white_space(ws, 1), 3, "an all-whitespace tail from mid returns length")
    t |> equal(skip_white_space(none, 0), 0, "the empty string returns 0")
    t |> equal(skip_white_space(none, 5), 0, "the empty string clamps any from to 0")
    t |> equal(skip_white_space(empty_view, 0), 0, "the empty view returns 0")
    t |> equal(skip_white_space(empty_view, -5), 0, "the empty view clamps a negative from to 0")
    t |> equal(skip_white_space(empty_view, 5), 0, "the empty view clamps a from past length to 0")
    peek_data(s) $(d) {
        t |> equal(skip_white_space(d, 0), 2, "view skip from 0 lands on the first content byte")
        t |> equal(skip_white_space(d, 2), 2, "view non-whitespace at from returns from")
        t |> equal(skip_white_space(d, 4), 6, "view skip from mid-string crosses the interior run")
        t |> equal(skip_white_space(d, -100), 2, "view negative from clamps to 0")
        t |> equal(skip_white_space(d, 8), 8, "view from == length returns length")
        t |> equal(skip_white_space(d, 100), 8, "view from past length clamps to length")
    }
    peek_data(ws) $(d) {
        t |> equal(skip_white_space(d, 0), 3, "view all-whitespace tail returns length")
        t |> equal(skip_white_space(d, 1), 3, "view all-whitespace tail from mid returns length")
    }
}

def private pin_trim_removes(t : T?; ch, what : string) {
    let s = heap_str("{ch}ab{ch}")
    t |> equal(trim(s), "ab", "trim removes {what}")
    t |> equal(ltrim(s), "ab{ch}", "ltrim removes leading {what}")
    t |> equal(rtrim(s), "{ch}ab", "rtrim removes trailing {what}")
}

[test]
def test_pin_trim_whitespace_set(t : T?) {
    // the trim family strips the same C isspace 6-set as the strip family
    pin_trim_removes(t, " ", "space")
    pin_trim_removes(t, "\t", "tab")
    pin_trim_removes(t, "\n", "newline")
    pin_trim_removes(t, "\r", "carriage return")
    pin_trim_removes(t, "\f", "form feed")
    pin_trim_removes(t, "\v", "vertical tab")
}

[test]
def test_pin_trim_degenerate(t : T?) {
    let ws = heap_str("   ")
    let none = heap_str("")
    let plain = heap_str("abc")
    let ffvt = heap_str("\f\v")
    t |> success(is_null_str(trim(ws)), "trim of all-whitespace is null")
    t |> success(is_null_str(ltrim(ws)), "ltrim of all-whitespace is null")
    t |> success(is_null_str(rtrim(ws)), "rtrim of all-whitespace is null")
    t |> success(is_null_str(trim(ffvt)), "trim of all-FF/VT is null")
    t |> success(is_null_str(ltrim(ffvt)), "ltrim of all-FF/VT is null")
    t |> success(is_null_str(rtrim(ffvt)), "rtrim of all-FF/VT is null")
    t |> success(is_null_str(trim(none)), "trim of empty is null")
    t |> success(is_null_str(ltrim(none)), "ltrim of empty is null")
    t |> success(is_null_str(rtrim(none)), "rtrim of empty is null")
    t |> equal(trim(plain), "abc", "trim with nothing to remove")
    t |> equal(rtrim(heap_str("abcxx"), "x"), "abc", "rtrim chars removes the trailing set")
    t |> equal(rtrim(heap_str("abcxy"), "xy"), "abc", "rtrim chars treats the argument as a set")
    t |> equal(rtrim(plain, "x"), "abc", "rtrim chars with nothing to remove")
    t |> success(is_null_str(rtrim(heap_str("xxx"), "x")), "rtrim chars consuming everything is null")
    t |> equal(rtrim(plain, ""), "abc", "rtrim with an empty char set")
    t |> success(is_null_str(rtrim(none, "x")), "rtrim chars on empty is null")
}

[test]
def test_pin_result_passthrough(t : T?) {
    let clean = heap_str("abc")
    let lead = heap_str("  abc")
    let trail = heap_str("abc  ")
    // there is no return-the-input passthrough left: every one of these allocates
    t |> success(ptr_of(rtrim(clean)) != ptr_of(clean), "rtrim allocates when nothing is trimmed")
    t |> success(ptr_of(trim(clean)) != ptr_of(clean), "trim allocates when nothing is trimmed")
    t |> success(ptr_of(rtrim(clean, "x")) != ptr_of(clean), "rtrim chars allocates when nothing is trimmed")
    t |> success(ptr_delta(lead, trim(lead)) != 2l, "trim allocates instead of returning an interior pointer")
    t |> success(ptr_of(replace(clean, "", "y")) != ptr_of(clean), "replace with an empty pattern allocates")
    t |> success(ptr_of(rtrim(trail)) != ptr_of(trail), "rtrim allocates when it trims")
    t |> success(ptr_of(ltrim(clean)) != ptr_of(clean), "ltrim always allocates")
    t |> success(ptr_of(ltrim(lead)) != ptr_of(lead), "ltrim allocates when it trims")
    t |> success(ptr_of(strip(clean)) != ptr_of(clean), "strip always allocates")
    t |> success(ptr_of(strip_left(clean)) != ptr_of(clean), "strip_left always allocates")
    t |> success(ptr_of(strip_right(clean)) != ptr_of(clean), "strip_right always allocates")
    // replace is NOT a passthrough on a miss - only the empty-pattern case aliases the input
    t |> success(ptr_of(replace(clean, "zz", "y")) != ptr_of(clean), "replace allocates a fresh copy on a miss")
    t |> success(ptr_of(slice(clean, 0)) != ptr_of(clean), "slice of the whole string allocates")
    t |> success(ptr_of(chop(clean, 0, 3)) != ptr_of(clean), "chop of the whole string allocates")
}

[test]
def test_pin_replace(t : T?) {
    let none = heap_str("")
    t |> equal(replace(heap_str("abcabc"), "bc", "X"), "aXaX", "replace all occurrences")
    t |> equal(replace(heap_str("aaa"), "aa", "b"), "ba", "replace does not rescan inside its own output")
    t |> equal(replace(heap_str("abc"), "b", ""), "ac", "replace with an empty replacement deletes")
    t |> equal(replace(heap_str("abc"), "zz", "y"), "abc", "replace with no match returns the same content")
    t |> equal(replace(heap_str("abc"), "", "y"), "abc", "replace with an empty pattern is a no-op")
    t |> equal(replace(heap_str("aaaa"), "a", "aa"), "aaaaaaaa", "replace with a growing replacement")
    t |> success(is_null_str(replace(none, "a", "b")), "replace on an empty string is null")
    t |> success(is_null_str(replace(none, "", "b")), "replace on empty with an empty pattern is null")
    t |> success(is_null_str(replace(heap_str("abc"), "abc", "")), "replace consuming everything is null")
}

struct private IntParse {
    v : int
    cr : ConversionResult
    off : int
}

def private parse_int(s : string; hex : bool = false; preset = ConversionResult.ok) : IntParse {
    var cr = preset
    var off = -1
    let v = int(s, cr, off, hex)
    return IntParse(v = v, cr = cr, off = off)
}

[test]
def test_pin_parse_int(t : T?) {
    let good = parse_int("123")
    t |> equal(good.v, 123, "int parse value")
    t |> equal(good.cr, ConversionResult.ok, "int parse result")
    t |> equal(good.off, 3, "int parse offset")
    let partial = parse_int("12abc")
    t |> equal(partial.v, 12, "int parse stops at trailing garbage")
    t |> equal(partial.cr, ConversionResult.ok, "trailing garbage is NOT an error")
    t |> equal(partial.off, 2, "trailing garbage only shows up in the offset")
    let bad = parse_int("abc")
    t |> equal(bad.v, 0, "int parse of garbage is 0")
    t |> equal(bad.cr, ConversionResult.invalid_argument, "int parse of garbage is invalid_argument")
    t |> equal(bad.off, 0, "int parse of garbage leaves offset 0")
    let none = parse_int("")
    t |> equal(none.cr, ConversionResult.invalid_argument, "int parse of empty is invalid_argument")
    t |> equal(none.off, 0, "int parse of empty leaves offset 0")
    let over = parse_int("99999999999999")
    t |> equal(over.v, 0, "int parse overflow value is 0")
    t |> equal(over.cr, ConversionResult.out_of_range, "int parse overflow result")
    t |> equal(over.off, 0, "int parse overflow leaves offset 0")
    let neg = parse_int("-7")
    t |> equal(neg.v, -7, "int parse of a negative")
    t |> equal(neg.off, 2, "int parse of a negative offset")
}

[test]
def test_pin_parse_int_offset_and_hex(t : T?) {
    let led = parse_int("  42")
    t |> equal(led.v, 42, "int parse skips leading whitespace")
    t |> equal(led.off, 4, "int parse offset counts the skipped whitespace")
    // on FAILURE the integer parse leaves offset 0 even though whitespace was skipped
    let ledBad = parse_int("  xyz")
    t |> equal(ledBad.cr, ConversionResult.invalid_argument, "int parse of leading-space garbage")
    t |> equal(ledBad.off, 0, "int parse failure offset stays 0, whitespace or not")
    let hexPrefixed = parse_int("0x1F", true)
    t |> equal(hexPrefixed.v, 31, "int parse with the hex flag accepts a 0x prefix")
    t |> equal(hexPrefixed.off, 4, "int parse hex offset counts the 0x prefix")
    let hexBare = parse_int("1F", true)
    t |> equal(hexBare.v, 31, "int parse with the hex flag accepts bare hex digits")
    t |> equal(hexBare.off, 2, "int parse bare hex offset")
    let hexOff = parse_int("0x1F", false)
    t |> equal(hexOff.v, 0, "int parse without the hex flag stops at the x")
    t |> equal(hexOff.cr, ConversionResult.ok, "int parse without the hex flag still succeeds on the leading 0")
    t |> equal(hexOff.off, 1, "int parse without the hex flag consumes just the 0")
    // a successful parse writes ConversionResult.ok over whatever the caller left there
    let stale = parse_int("5", false, ConversionResult.out_of_range)
    t |> equal(stale.v, 5, "int parse succeeds with a stale preset result")
    t |> equal(stale.cr, ConversionResult.ok, "a successful parse resets ConversionResult to ok")
}

[test]
def test_pin_parse_int_widths(t : T?) {
    var cr = ConversionResult.ok
    var off = -1
    t |> equal(int8("200", cr, off), int8(0), "int8 overflow value")
    t |> equal(cr, ConversionResult.out_of_range, "int8 overflow result")
    cr = ConversionResult.ok
    t |> equal(uint8("200", cr, off), uint8(200), "uint8 in range")
    t |> equal(cr, ConversionResult.ok, "uint8 in range result")
    t |> equal(off, 3, "uint8 in range offset")
    cr = ConversionResult.ok
    t |> equal(int16("70000", cr, off), int16(0), "int16 overflow value")
    t |> equal(cr, ConversionResult.out_of_range, "int16 overflow result")
    cr = ConversionResult.ok
    t |> equal(uint16("65535", cr, off), uint16(65535), "uint16 at the top of the range")
    cr = ConversionResult.ok
    t |> equal(uint("42", cr, off), 42u, "uint in range")
    cr = ConversionResult.ok
    t |> equal(uint("-1", cr, off), 0u, "uint rejects a minus sign")
    t |> equal(cr, ConversionResult.invalid_argument, "uint of a negative is invalid_argument")
    cr = ConversionResult.ok
    t |> equal(int64("9223372036854775807", cr, off), 9223372036854775807l, "int64 max")
    t |> equal(off, 19, "int64 max offset")
    cr = ConversionResult.ok
    t |> equal(uint64("18446744073709551615", cr, off), 18446744073709551615ul, "uint64 max")
    t |> equal(off, 20, "uint64 max offset")
}

struct private RealParse {
    v : double
    cr : ConversionResult
    off : int
}

def private parse_float(s : string) : RealParse {
    var cr = ConversionResult.ok
    var off = -1
    let v = float(s, cr, off)
    return RealParse(v = double(v), cr = cr, off = off)
}

def private parse_double(s : string) : RealParse {
    var cr = ConversionResult.ok
    var off = -1
    let v = double(s, cr, off)
    return RealParse(v = v, cr = cr, off = off)
}

[test]
def test_pin_parse_real(t : T?) {
    let good = parse_float("1.5")
    t |> equal(good.v, 1.5lf, "float parse value")
    t |> equal(good.cr, ConversionResult.ok, "float parse result")
    t |> equal(good.off, 3, "float parse offset")
    let partial = parse_float("1.5xyz")
    t |> equal(partial.v, 1.5lf, "float parse stops at trailing garbage")
    t |> equal(partial.cr, ConversionResult.ok, "float trailing garbage is NOT an error")
    t |> equal(partial.off, 3, "float trailing garbage only shows up in the offset")
    let bad = parse_float("xyz")
    t |> equal(bad.v, 0.0lf, "float parse of garbage is 0")
    t |> equal(bad.cr, ConversionResult.invalid_argument, "float parse of garbage is invalid_argument")
    let none = parse_float("")
    t |> equal(none.cr, ConversionResult.invalid_argument, "float parse of empty is invalid_argument")
    let led = parse_float("  2.25")
    t |> equal(led.v, 2.25lf, "float parse skips leading whitespace")
    t |> equal(led.off, 6, "float parse offset counts the skipped whitespace")
    // like the integer parse, the real parse leaves offset 0 on failure - whitespace or not
    let ledBad = parse_float("  xyz")
    t |> equal(ledBad.cr, ConversionResult.invalid_argument, "float parse of leading-space garbage")
    t |> equal(ledBad.off, 0, "float parse failure offset stays 0, whitespace or not")
    let over = parse_float("1e400")
    t |> equal(over.v, 0.0lf, "float parse overflow value is 0")
    t |> equal(over.cr, ConversionResult.out_of_range, "float parse overflow result")
    t |> equal(over.off, 0, "float parse overflow leaves offset 0")
    let dgood = parse_double("3.5")
    t |> equal(dgood.v, 3.5lf, "double parse value")
    t |> equal(dgood.off, 3, "double parse offset")
    let dover = parse_double("1e400")
    t |> equal(dover.cr, ConversionResult.out_of_range, "double parse overflow result")
}

[test]
def test_pin_string_from_array(t : T?) {
    var inscope normal : array<uint8>
    normal |> push_from([uint8('a'), uint8('b'), uint8('c')])
    t |> equal(string(normal), "abc", "string(array<uint8>) of a plain buffer")
    t |> equal(length(string(normal)), 3, "string(array<uint8>) length")
    var inscope none : array<uint8>
    t |> success(is_null_str(string(none)), "string(array<uint8>) of an empty buffer is null")
    var inscope holed : array<uint8>
    holed |> push_from([uint8('a'), uint8(0), uint8('c')])
    let s = string(holed)
    // the whole buffer is copied, but every downstream op is strlen-bounded
    t |> equal(length(s), 1, "an interior NUL truncates length()")
    t |> equal(s, "a", "an interior NUL truncates the value")
    t |> equal(find(s, 'c'), -1, "an interior NUL hides the tail from find")
    var seen = 0
    peek_data(s) $(arr) {
        seen = length(arr)
    }
    t |> equal(seen, 1, "an interior NUL truncates the peek_data view")
}

[test]
def test_pin_to_bytes(t : T?) {
    var inscope none <- to_bytes(heap_str(""))
    t |> equal(length(none), 0, "to_bytes of an empty string is an empty array")
    var inscope plain <- to_bytes(heap_str("abc"))
    t |> equal(length(plain), 3, "to_bytes copies the bytes and appends no terminator")
    t |> equal(int(plain[0]), 'a', "to_bytes byte 0")
    t |> equal(int(plain[1]), 'b', "to_bytes byte 1")
    t |> equal(int(plain[2]), 'c', "to_bytes byte 2")
    t |> equal(string(plain), "abc", "string(to_bytes(s)) round-trips")
    // the array owns its bytes, so writing it cannot reach back into the source string
    let src = heap_str("hello")
    var inscope owned <- to_bytes(src)
    owned[0] = uint8('H')
    t |> equal(src, "hello", "to_bytes returns an owned copy")
    t |> equal(string(owned), "Hello", "and the copy carries the write")
}
