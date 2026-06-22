options gen2

require daslib/fio
require daslib/strings_boost


// Per-file line cache so a cluster of N members from the same file
// reads the file once. Keyed by absolute (or whatever the input
// records, raw) path. Lines are stored without trailing newline.
struct public FileCache {
    files : table<string; array<string>>
}


// Normalize CRLF → LF before splitting so Windows files don't leave
// a stray `\r` at the end of each line. Costs one extra allocation;
// trivial compared to the surrounding `fread`.
def private split_lines(text : string) : array<string> {
    let normalized = replace(text, "\r\n", "\n")
    return <- normalized |> split_by_chars("\n")
}


def private load_file(var cache : FileCache; path : string) : bool {
    if (key_exists(cache.files, path)) return true
    let text = fread(path)
    if (empty(text)) return false
    // `array<string>` is non-copyable — assign through the table
    // index with `<-` so ownership moves into the table slot.
    cache.files[path] <- split_lines(text)
    return true
}


// Return the source slice for `path[start_line..end_line]` (1-based,
// inclusive on both ends), or an empty string with `error` set on
// failure. `end_line <= 0` falls back to a single-line peek (just the
// start line, no inline marker) — happens only when consuming a v1
// detect-dupe report; `read_input_report` already logs a one-shot
// file-level warning in that case.
def public extract_source(var cache : FileCache; path : string;
                          start_line, end_line : int;
                          var error : string&) : string {
    error = ""
    if (!load_file(cache, path)) {
        error := "could not read file: {path}"
        return ""
    }
    let lines & = unsafe(cache.files[path])
    let n = length(lines)
    if (start_line < 1 || start_line > n) {
        error := "start_line {start_line} out of range [1..{n}] in {path}"
        return ""
    }
    let lo = start_line - 1
    var hi : int
    if (end_line <= 0) {
        // v1 fallback: single-line peek of the start line, no inline marker.
        // Legitimate path for legacy v1 reports that lack `end_line`.
        hi = lo
    } else {
        // For v2 inputs we treat out-of-range or inverted `end_line` as
        // an error rather than silently clamping. Stale detect-dupe
        // reports against an edited file are the realistic failure mode
        // here — silently extracting the wrong slice would feed the
        // judge model misleading source and waste an API call. Caller
        // marks the cluster skipped with this error string.
        hi = end_line - 1
        if (hi >= n) {
            error := "end_line {end_line} past EOF (file has {n} lines) in {path} — likely a stale report"
            return ""
        }
        if (hi < lo) {
            error := "end_line {end_line} < start_line {start_line} in {path} — degenerate range"
            return ""
        }
    }
    return build_string() $(var w) {
        for (i in range(lo, hi + 1)) {
            if (i > lo) {
                w |> write_char('\n')
            }
            w |> write(lines[i])
        }
    }
}
