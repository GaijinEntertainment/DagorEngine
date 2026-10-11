options gen2
options indenting = 4

require daslib/rtti
require math
require strings

//! Source text addressed by absolute byte offset.
//!
//! Every span in this tool is an offset pair into ONE immutable source string. `LineInfo`
//! (1-based line, 0-based byte column, end-exclusive) is converted once, up front, so no
//! analysis step ever does line/column arithmetic — and no offset is ever adjusted, because
//! nothing is written until the whole patch set is built.

struct public Source {
    //! An immutable source text plus its line index.
    text : string
    lineStart : array<int>
    size : int = 0
    eol : string = "\n"
}


def public make_source(text : string) : Source {
    //! Builds the line index for `text`. O(n), once per file.
    var src = Source(text = text, size = length(text))
    src.lineStart |> push(0)
    peek_data(text) $(bytes) {
        for (i in range(length(bytes))) {
            if (bytes[i] == uint8('\n')) {
                if (i > 0 && bytes[i - 1] == uint8('\r')) {
                    src.eol = "\r\n"
                }
                src.lineStart |> push(i + 1)
            }
        }
    }
    return <- src
}


def public line_count(src : Source) : int {
    //! Number of lines in the source.
    return length(src.lineStart)
}


def public line_begin(src : Source; line : int) : int {
    //! Offset of the first byte of `line` (1-based). Clamped to the source.
    if (line < 1) return 0
    if (line > length(src.lineStart)) return src.size
    return src.lineStart[line - 1]
}


def public line_end(src : Source; line : int) : int {
    //! Offset one past the last byte of `line`, excluding the newline. Clamped.
    if (line < 1) return 0
    if (line >= length(src.lineStart)) return src.size
    var e = src.lineStart[line] - 1
    if (e > 0 && e <= src.size) {
        peek_data(src.text) $(bytes) {
            if (e - 1 < length(bytes) && bytes[e - 1] == uint8('\r')) {
                e--
            }
        }
    }
    return max(e, 0)
}


def public offset_of(src : Source; line, column : int) : int {
    //! Absolute offset of a (1-based line, 0-based byte column) position.
    //!
    //! Columns are clamped to the line's end: v1 block spans routinely report one column past
    //! the last line's end (the lexer's synthesized statement terminator), and a rewriter must
    //! not address past the text it was handed.
    let b = line_begin(src, line)
    let e = line_end(src, line)
    let p = b + max(column, 0)
    return min(p, e)
}


def public span_begin(src : Source; at : LineInfo) : int {
    //! Start offset of `at`.
    return offset_of(src, int(at.line), int(at.column))
}


def public span_end(src : Source; at : LineInfo) : int {
    //! End offset of `at` (exclusive).
    return offset_of(src, int(at.last_line), int(at.last_column))
}


def public text_of(src : Source; from, to : int) : string {
    //! The source bytes in [from, to). Empty when the range is degenerate.
    let a = clamp(from, 0, src.size)
    let b = clamp(to, a, src.size)
    return a == b ? "" : slice(src.text, a, b)
}


def public text_at(src : Source; at : LineInfo) : string {
    //! The source bytes covered by `at`.
    return text_of(src, span_begin(src, at), span_end(src, at))
}


def public indent_of(src : Source; offset : int) : int {
    //! Width of the leading whitespace on the line containing `offset`, tabs counted as one.
    let ln = line_at(src, offset)
    let b = line_begin(src, ln)
    let e = line_end(src, ln)
    var n = 0
    peek_data(src.text) $(bytes) {
        for (i in range(b, e)) {
            if (bytes[i] != uint8(' ') && bytes[i] != uint8('\t')) break
            n++
        }
    }
    return n
}


def public line_at(src : Source; offset : int) : int {
    //! 1-based line containing `offset`. Binary search over the line index.
    if (offset <= 0) return 1
    var lo = 0
    var hi = length(src.lineStart) - 1
    while (lo < hi) {
        let mid = (lo + hi + 1) / 2
        if (src.lineStart[mid] <= offset) {
            lo = mid
        } else {
            hi = mid - 1
        }
    }
    return lo + 1
}
