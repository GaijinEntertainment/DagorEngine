require math
require strings

require utils

struct TableData {
    rows : array<TableRow?>
    col_gap : int = 1
    indent : string
}

struct TableRow {
    columns : array<string>
}

def format_table(tab : TableData?) : string {
    if (length(tab.rows) == 0) {
        return ""
    }

    return build_string() $(var w) {
        var col_widths : array<int>
        for (col_index in range(length(tab.rows[0].columns))) {
            var max_width = 0
            for (row in tab.rows) {
                max_width = max(max_width, length(bare_str(row.columns[col_index])))
            }
            col_widths |> push(max_width)
        }

        for (row in tab.rows) {
            w |> write(tab.indent)
            for (i, col in range(length(row.columns)), row.columns) {
                let width = col_widths[i]
                let col_width = length(bare_str(col))
                let sep = repeat(" ", max(width - col_width, 0) + (tab.col_gap - 1))
                w |> write(col)
                w |> write(sep)
            }
            w |> write("\n")
        }
    }
}
