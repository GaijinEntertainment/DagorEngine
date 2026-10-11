// The `[arch]` citation scanner: one implementation of LINT026's resolution semantics, shared by
// the rule that enforces them (`utils/lint/main.das`), the install-time excerpt writer
// (arch-extract) and the tools that read them (`utils/mcp/tools/arch_of`,
// `utils/mcp/tools/arch_sites`). The failure strings live here too, so a citation lint reports
// and a citation a tool refuses cannot describe themselves differently.
//
// Two spellings, one resolution: `[arch(at="<doc>#<anchor>")]` on a .das declaration, and
// `// <doc>#<anchor>` inside a `//` comment of a C or C++ source, which has no annotation to
// carry it. Everything here reads SOURCE rather than the AST: the annotation is an inert builtin
// marker, so citing a document costs a build nothing, and a citation must be checkable in a file
// the reading environment cannot compile.

options gen2

module arch_citations

require daslib/fio public
require strings public
require daslib/strings_boost public
require daslib/regex
require daslib/regex_boost


// The citation as written in a .das source. A match with `//` earlier on its line is prose or a
// commented-out annotation and is skipped; one inside a string literal or a block comment is not,
// so a fixture that plants citation text assembles it rather than spelling it out.
var private ARCH_CITE <- %regex~arch\s*\(\s*at\s*=\s*"([^"]*)"%%


// One citation, split at the first '#'. `doc` is normalized against the CITING file's folder, so
// it compares equal to the same document reached from anywhere else in the tree; `anchor` is
// empty when the citation carries no '#' at all.
struct public ArchCitation {
    line : int
    doc : string
    anchor : string
    raw : string
}


// One `{#anchor}` a heading declares: the name, the line that declares it, and the heading level
// that bounds its section.
struct public MdAnchor {
    name : string
    line : int
    level : int
}


// The anchors of every document a scan has resolved so far. `counts` is keyed "<doc>#<anchor>" so
// it answers the duplicate question in one lookup; `known` records existence separately, because
// a citation naming a missing file must not read as a missing anchor.
struct public ArchDocs {
    counts : table<string; int>
    known : table<string; bool>
    anchors : table<string; array<MdAnchor>>
}


// True for an entry name every [arch] walk skips: `_`/`.`-prefixed entries, and
// `builtin.das`, which sits outside the citation system in LINT026's walk
// (utils/lint/main.das), the census walk (arch_common), and extraction alike.
def public arch_walk_skips(name : string) : bool {
    return name |> starts_with("_") || name |> starts_with(".") || name == "builtin.das" || name == "node_modules"
}


// The one spelling a path is compared under: lexical, so it works on a document that is not
// there, and '/'-folded, so a Windows walk and a citation agree.
def public arch_path(path : string) : string {
    return normalize(path) |> replace("\\", "/")
}


// Where a match sits: its 1-based line, and the offset that line starts at - which is what lets
// the scan tell an annotation from the same text inside a `//` comment.
struct private SrcPos {
    line : int
    line_start : int
}


// Byte offsets (ascending) to source positions in one pass over the bytes. Counting newlines with
// find() per citation would re-walk the source from the top each time.
def private positions_of_offsets(text : string; offsets : array<int>) : array<SrcPos> {
    var out : array<SrcPos>
    out |> reserve(length(offsets))
    peek_data(text) $(d) {
        var line = 1
        var start = 0
        var at = 0
        for (off in offsets) {
            while (at < off) {
                if (int(d[at]) == '\n') {
                    line ++
                    start = at + 1
                }
                at ++
            }
            out |> push(SrcPos(line = line, line_start = start))
        }
    }
    return <- out
}


let private CPP_SOURCE_SUFFIXES <- [".c", ".cpp", ".cc", ".cxx", ".mm", ".h", ".hpp", ".inl", ".inc"]


// True for a C or C++ source, whose citation is the `// <doc>#<anchor>` pointer form.
def public is_cpp_source(name : string) : bool {
    for (s in CPP_SOURCE_SUFFIXES) {
        return true if (name |> ends_with(s))
    }
    return false
}


// True for a file an [arch] walk reads for citations: a .das, or a C/C++ source (a markdown document cites too; the lint walks those on its own list).
def public arch_cites_source(name : string) : bool {
    return name |> ends_with(".das") || is_cpp_source(name)
}


def private is_path_char(c : int) : bool {
    return is_alpha(c) || is_number(c) || c == '_' || c == '.' || c == '/' || c == '-'
}


def private is_anchor_char(c : int) : bool {
    return is_alpha(c) || is_number(c) || c == '_' || c == '-'
}


// A C++ pointer's document is spelled root-relative (`src/ast/ARCHITECTURE.md`), and the root is
// the first folder, from the citing file's own upward, that the path exists under: a header in
// include/ reaches src/'s document, a bare `ARCHITECTURE.md` still means the citer's own folder,
// and a planted fixture tree resolves inside itself. A path found nowhere resolves at the
// citer's folder, where the forward check reports it missing.
def private resolve_pointer_doc(folder, rel : string) : string {
    var dir = arch_path(folder)
    while (true) {
        let candidate = arch_path(path_join(dir, rel))
        let st = stat(candidate)
        return candidate if (st.is_valid && st.is_reg)
        let up = dir_name(dir)
        break if (empty(up) || up == dir)
        dir = up
    }
    return arch_path(path_join(folder, rel))
}


def private pointers_in_line(line : string; scan_start, lineno : int; folder : string; var out : array<ArchCitation>) {
    peek_data(line) $(d) {
        let len = length(d)
        var from = scan_start
        while (true) {
            let ext = find(line, ".md#", from)
            break if (ext < 0)
            let hash = ext + 3
            var start = ext
            while (start > from && is_path_char(int(d[start - 1]))) {
                start --
            }
            var stop = hash + 1
            while (stop < len && is_anchor_char(int(d[stop]))) {
                stop ++
            }
            from = stop
            // prose `.md#` (no path or no name) and a web URL (`/`-led, or a dotted first segment) are not pointers; `./` and `../` lead a relative path
            continue if (stop == hash + 1 || start == ext || int(d[start]) == '/')
            let first_slash = find(line, "/", start)
            let first_seg = first_slash > start ? slice(d, start, first_slash) : ""
            continue if (first_slash > start && first_slash < ext && first_seg != "." && first_seg != ".." && find(line, ".", start) < first_slash)
            out |> push(ArchCitation(line = lineno, raw = slice(d, start, stop),
                anchor = slice(d, hash + 1, stop),
                doc = resolve_pointer_doc(folder, slice(d, start, hash))))
        }
    }
}


//! every `<path>.md#<anchor>` inside a `//` comment of a C or C++ source, in file order; a block comment is not read
def public collect_arch_pointers(file, text : string) : array<ArchCitation> {
    var out : array<ArchCitation>
    let folder = dir_name(file)
    var lineno = 0
    for (line in split(text, "\n")) {
        lineno ++
        let slashes = find(line, "//")
        continue if (slashes < 0)
        pointers_in_line(line, slashes + 2, lineno, folder, out)
    }
    return <- out
}


//! every `<path>.md#<anchor>` anywhere in a markdown document, in file order, its path resolved as a C++ pointer's
def public collect_md_citations(file, text : string) : array<ArchCitation> {
    var out : array<ArchCitation>
    let folder = dir_name(file)
    var lineno = 0
    for (line in split(text, "\n")) {
        lineno ++
        continue if (find(line, ".md#") < 0)
        pointers_in_line(line, 0, lineno, folder, out)
    }
    return <- out
}


// Every citation `text` makes, in file order, in whichever spelling `file`'s language carries.
// `file` is only read for its extension and the folder a relative citation resolves against, so
// it may name a file that is no longer on disk.
def public collect_arch_citations(file, text : string) : array<ArchCitation> {
    return <- collect_arch_pointers(file, text) if (is_cpp_source(file))
    return <- collect_md_citations(file, text) if (file |> ends_with(".md"))
    var out : array<ArchCitation>
    var raws : array<string>
    var offsets : array<int>
    regex_foreach(ARCH_CITE, text) $(at) {
        offsets |> push(at.x)
        raws |> push(regex_group(ARCH_CITE, 1, text))
        return true
    }
    return <- out if (empty(raws))
    let positions <- positions_of_offsets(text, offsets)
    let folder = dir_name(file)
    for (raw, off, pos in raws, offsets, positions) {
        let slashes = find(text, "//", pos.line_start)
        continue if (slashes >= 0 && slashes < off)
        let hash = find(raw, "#")
        if (hash <= 0 || hash == length(raw) - 1) {
            // shape, not resolution: the compiler rejects this on a function, but a structure
            // takes the marker unvalidated (one annotation per name), so the scan owns it there
            out |> push(ArchCitation(line = pos.line, raw = raw))
            continue
        }
        out |> push(ArchCitation(line = pos.line, raw = raw, anchor = slice(raw, hash + 1),
            doc = arch_path(path_join(folder, slice(raw, 0, hash)))))
    }
    return <- out
}


// The ATX heading level of a line, 0 when the line is not a heading. One to six '#' whose run ends
// the token - so `#!shebang` and `#include` are prose. A line carrying an anchor suffix passes
// whatever follows the run, because that is the shape an anchor is read under.
def public heading_level(line : string) : int {
    let t = line |> strip
    return 0 if (!(t |> starts_with("#")))
    var n = 0
    peek_data(t) $(d) {
        let len = length(d)
        while (n < len && int(d[n]) == '#') {
            n ++
        }
    }
    return 0 if (n > 6)
    let rest = slice(t, n)
    return n if (empty(rest) || first_character(rest) == ' ' || first_character(rest) == '\t'
        || t |> ends_with("}"))
    return 0
}


// The anchor a heading line declares, name empty when it declares none. Only a heading SUFFIX
// counts - `## <title> {#anchor}` at any level, the brace group last on the line. A `{#...}`
// elsewhere is prose about anchors, not a section identity.
def public anchor_of_heading(line : string) : MdAnchor {
    var out : MdAnchor
    let t = line |> strip
    return out if (!(t |> starts_with("#")) || !(t |> ends_with("}")))
    let open = rfind(t, "\{#")
    return out if (open < 0)
    let name = slice(t, open + 2, length(t) - 1)
    return out if (empty(name))
    out.name = name
    out.level = heading_level(t)
    return out
}


// Every anchor the document at `path` declares, in file order. A repeated name is kept twice -
// the duplicate is what makes a citation ambiguous.
def public collect_md_anchors(path : string) : array<MdAnchor> {
    var out : array<MdAnchor>
    let text = fread(path)
    if (!empty(text)) {
        var lineno = 0
        for (line in split(text, "\n")) {
            lineno ++
            var a = anchor_of_heading(line)
            continue if (empty(a.name))
            a.line = lineno
            out |> push(a)
        }
    }
    return <- out
}


// True when `doc` exists, indexing its anchors on first sight.
def private arch_doc_exists(var docs : ArchDocs; doc : string) : bool {
    if (!key_exists(docs.known, doc)) {
        let st = stat(doc)
        let ok = st.is_valid && st.is_reg
        docs.known[doc] = ok
        if (ok) {
            var found <- collect_md_anchors(doc)
            for (a in found) {
                let key = "{doc}#{a.name}"
                docs.counts[key] = (docs.counts ?[key] ?? 0) + 1
            }
            docs.anchors[doc] <- found
        }
    }
    return docs.known[doc]
}


// Every anchor `doc` declares, through the same one-parse index the forward check uses; nothing
// when the document is missing.
def public each_arch_anchor(var docs : ArchDocs; doc : string; blk : block<(a : MdAnchor) : void>) {
    return if (!arch_doc_exists(docs, doc))
    for (a in docs.anchors[doc]) {
        invoke(blk, a)
    }
}


// The line span of the anchor's section: x = the heading's 1-based line, y = the 1-based line
// AFTER the section (the next heading of the same or a higher level, trailing blank lines
// dropped). (0,0) when the document declares no such anchor. A deeper heading's span nests
// inside its parent's - `y <= parent.y` - which is what lets a caller emitting several
// sections tell a subsection it already carries from one it still owes.
def public section_span(doc, anchor : string) : int2 {
    let text = fread(doc)
    return int2(0, 0) if (empty(text))
    let lines <- split(text, "\n")
    var start = -1
    var level = 0
    for (i in range(length(lines))) {
        let a = anchor_of_heading(lines[i])
        if (a.name == anchor) {
            start = i
            level = a.level
            break
        }
    }
    return int2(0, 0) if (start < 0)
    var stop = length(lines)
    for (i in range(start + 1, length(lines))) {
        let lv = heading_level(lines[i])
        if (lv > 0 && lv <= level) {
            stop = i
            break
        }
    }
    while (stop > start + 1 && empty(strip(lines[stop - 1]))) {
        stop --
    }
    return int2(start + 1, stop + 1)
}


// The cited section: the anchor's heading line through the last line before the next heading of
// the same or a higher level, trailing blank lines dropped. "" when the document declares no
// such anchor.
def public section_text(doc, anchor : string) : string {
    let span = section_span(doc, anchor)
    return "" if (span.x == 0)
    let lines <- split(fread(doc), "\n")
    return build_string() $(var w) {
        for (i in range(span.x - 1, span.y - 1)) {
            // split() leaves the '\r' of a CRLF document on the line
            let raw = lines[i]
            w |> write(raw |> ends_with("\r") ? slice(raw, 0, length(raw) - 1) : raw)
            w |> write("\n")
        }
    }
}


// Why a citation does not resolve, in the words LINT026 reports; "" when it resolves. A citation
// names a document section that must be there, exactly once: an anchor declared twice is worse
// than a missing one - the citation reads as precise and points at whichever section the reader
// finds first.
def public citation_failure_reason(var docs : ArchDocs; c : ArchCitation) : string {
    return "malformed citation (expected <doc>.md#<anchor>)" if (empty(c.anchor))
    return "no such file" if (!arch_doc_exists(docs, c.doc))
    let n = docs.counts ?["{c.doc}#{c.anchor}"] ?? 0
    return "no such anchor" if (n == 0)
    return "anchor appears {n} times" if (n > 1)
    return ""
}


// True when the cited document does not sit in the citing file's own folder tree - the document's
// folder must be the file's folder or one of its ancestors. A mechanism another folder's document
// states is restated in prose in a document of the citer's own tree and cited there.
def public citation_outside_subtree(file : string; c : ArchCitation) : bool {
    let fdir = arch_path(dir_name(file))
    let ddir = dir_name(c.doc)
    return !(fdir == ddir || fdir |> starts_with("{ddir}/"))
}


// `citation_failure_reason` plus the scope check: a citation that resolves but reaches outside
// the citing file's folder tree is reported after the resolution reasons, so a broken far
// citation is named for what is broken first.
def public citation_failure_reason(var docs : ArchDocs; file : string; c : ArchCitation) : string {
    let reason = citation_failure_reason(docs, c)
    return reason if (!empty(reason))
    // a C++ pointer is exempt from the tree rule: the layout puts a header under include/ far
    // from the src/ folder whose document owns the mechanism it implements
    return "" if (is_cpp_source(file))
    return "cites a document outside this file's folder tree - restate the mechanism in an architecture doc of this folder's tree and cite that" if (citation_outside_subtree(file, c))
    return ""
}
