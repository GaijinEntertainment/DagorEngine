options gen2
options indenting = 4
options strict_smart_pointers = true

module md_boost shared public

//! Markdown generation helpers.
//!
//! GitHub-flavored table rendering plus small text utilities, for tools that emit ``.md``
//! scoreboards and reports from structured data. Numeric cell formatting is left to the caller
//! (the ``"{x:.2f}"`` interpolation-format form); this module handles alignment and layout.

require math
require daslib/strings_boost public

//! Column alignment for [[render_md_table]].
enum ColAlign {
    left
    right
}

//! Wraps ``s`` in ``**`` bold markers.
def public md_bold(s : string) : string {
    return "**{s}**"
}

//! Escapes the ``|`` cell separator in ``s`` so it renders as literal text inside a table cell.
[temp_string_result]
def public md_escape(s : string) : string {
    return s |> replace("|", "\\|")
}

def private md_pad(cell : string; align : ColAlign; width : int) : string {
    return align == ColAlign.right ? pad_left(cell, width) : pad_right(cell, width)
}

def private md_row(cells : array<string>; aligns : array<ColAlign>; width : array<int>) : string {
    return build_string() $(writer) {
        writer |> write("|")
        for (c in range(length(width))) {
            let cell = c < length(cells) ? cells[c] : ""
            writer |> write(" ")
            writer |> write(md_pad(cell, aligns[c], width[c]))
            writer |> write(" |")
        }
    }
}

def private md_sep(aligns : array<ColAlign>; width : array<int>) : string {
    return build_string() $(writer) {
        writer |> write("|")
        for (c in range(length(width))) {
            writer |> write(" ")
            if (aligns[c] == ColAlign.right) {
                writer |> write_chars('-', width[c] - 1)
                writer |> write(":")
            } else {
                writer |> write(":")
                writer |> write_chars('-', width[c] - 1)
            }
            writer |> write(" |")
        }
    }
}

//! Renders a GitHub-Markdown table from PRE-FORMATTED cells. ``headers`` names the columns, ``aligns`` sets each column's alignment (right ``---:`` / left ``:---``), and ``rows`` holds the already-formatted cell strings — numbers pre-formatted (e.g. ``"{x:.2f}"``), winners already wrapped via [[md_bold]].
[temp_string_result]
def public render_md_table(headers : array<string>; aligns : array<ColAlign>; rows : array<array<string>>) : string {
    let ncol = length(headers)
    var width : array<int>
    width |> resize(ncol)
    for (c in range(ncol)) {
        width[c] = max(length(headers[c]), 3)
    }
    for (row in rows) {
        for (c in range(min(ncol, length(row)))) {
            width[c] = max(width[c], length(row[c]))
        }
    }
    let out = build_string() $(writer) {
        writer |> write(md_row(headers, aligns, width))
        writer |> write("\n")
        writer |> write(md_sep(aligns, width))
        for (row in rows) {
            writer |> write("\n")
            writer |> write(md_row(row, aligns, width))
        }
    }
    delete width
    return out
}
