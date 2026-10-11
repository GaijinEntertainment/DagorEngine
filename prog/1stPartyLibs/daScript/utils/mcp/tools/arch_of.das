options gen2

module arch_of

require common public
require arch_common public
require daslib/fio
require strings
require daslib/strings_boost

//! `arch_of` - the `[arch]` citations one file makes, each located in the document it cites.
//!
//! The read direction of the annotation: given code, say WHERE the architecture text it answers
//! for is - document, heading line and section end - so a read or an edit of the section follows
//! from the coordinates. One line per citation; the text itself is the document's to show.
//! Resolution is LINT026's, failures included - a citation this tool cannot follow is one lint
//! will report. A C/C++ source is read for its `// <doc>#<anchor>` pointers.

// `<citer>  ->  <doc>:<first>-<last>  #<anchor>`, or the reason it does not resolve.
def private write_citation(var w : StringBuilderWriter; var docs : ArchDocs;
                           file, rel : string; c : ArchCitation; symbol : string) {
    let who = empty(symbol) ? "{rel}:{c.line}" : "{rel}:{c.line}  {symbol}"
    let reason = citation_failure_reason(docs, file, c)
    if (!empty(reason)) {
        w |> write("{who}  ->  does not resolve - {reason} (\"{c.raw}\")\n")
        return
    }
    let span = section_span(c.doc, c.anchor)
    w |> write("{who}  ->  {make_relative_path(c.doc, get_das_root())}:{span.x}-{span.y - 1}  #{c.anchor}\n")
}

def do_arch_of(file : string; symbol : string = "") : string {
    return make_tool_result("'file' argument is required", true) if (empty(file))
    let resolved = resolve_path(file)
    let st = stat(resolved)
    return make_tool_result("File not found: {file}", true) if (!(st.is_valid && st.is_reg))
    let text = fread(resolved)
    return make_tool_result("File is empty or unreadable: {file}", true) if (empty(text))
    let cites <- collect_arch_citations(arch_path(resolved), text)
    let rel = make_relative_path(resolved, get_das_root())
    return make_tool_result("No [arch] citations in {rel}") if (empty(cites))
    let lines <- split(text, "\n")
    var docs : ArchDocs
    var shown = 0
    let body = build_string() $(var w) {
        for (c in cites) {
            let owner = cited_symbol(arch_path(resolved), lines, c.line)
            continue if (!empty(symbol) && owner != symbol)
            shown ++
            write_citation(w, docs, arch_path(resolved), rel, c, owner)
        }
    }
    if (shown == 0) {
        return make_tool_result("No [arch] citation on '{symbol}' in {rel} ({length(cites)} citation(s) in the file)")
    }
    return make_tool_result("{body}{shown} citation(s) shown.")
}
