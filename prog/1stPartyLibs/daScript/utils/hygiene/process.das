options gen2

module process shared private

require strings
require daslib/fio
require daslib/strings_boost
require rules


enum public ProcessResultKind {
    no_change
    would_change
    changed
    missing_file
    not_a_file
    write_failed
}


struct public ProcessResult {
    kind : ProcessResultKind
    lines_removed : int
}


def public process_path(path : string; check : bool) : ProcessResult {
    let st = stat(path)
    if (!st.is_valid) return ProcessResult(kind = ProcessResultKind.missing_file)
    if (!st.is_reg) return ProcessResult(kind = ProcessResultKind.not_a_file)
    let original = fread(path)
    let trimmed = apply_all_rules(original)
    if (original == trimmed) return ProcessResult(kind = ProcessResultKind.no_change)
    if (check) return ProcessResult(kind = ProcessResultKind.would_change)
    if (!fwrite(path, trimmed)) return ProcessResult(kind = ProcessResultKind.write_failed)
    let removed = length(split(original, "\n")) - length(split(trimmed, "\n"))
    return ProcessResult(kind = ProcessResultKind.changed, lines_removed = removed)
}
