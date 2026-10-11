options gen2

module arch_sites

require common public
require arch_common public
require daslib/fio
require strings
require daslib/strings_boost

//! `arch_sites` - the census of an architecture document: who cites it, and from where.
//!
//! The write direction of the annotation, and the question to ask before editing a section:
//! which code answers for it. The search subtree is the folder that OWNS the document, which is
//! the scope a citation's relative path can reach and the scope LINT026's reverse pass walks.

// One citation of the queried document.
struct private Site {
    file : string
    line : int
    symbol : string
    anchor : string
}

def private write_sites(var w : StringBuilderWriter; sites : array<Site>; which : array<int>) {
    for (i in which) {
        let s = sites[i]
        let tail = empty(s.symbol) ? "" : "  {s.symbol}"
        w |> write("  {s.file}:{s.line}{tail}\n")
    }
}

// Every citation of `doc` (of its `anchor` alone, when one was named) made by a .das under
// `folder`, in file then line order.
def private collect_sites(folder, doc, anchor : string) : array<Site> {
    var out : array<Site>
    var files : array<string>
    scan_das_tree(folder, files)
    files |> sort
    let root = get_das_root()
    for (f in files) {
        let text = fread(f)
        continue if (empty(text))
        let cites <- collect_arch_citations(arch_path(f), text)
        continue if (empty(cites))
        let lines <- split(text, "\n")
        let rel = make_relative_path(f, root)
        for (c in cites) {
            continue if (c.doc != doc || (!empty(anchor) && c.anchor != anchor))
            out |> emplace(Site(file = rel, line = c.line, anchor = c.anchor,
                symbol = cited_symbol(arch_path(f), lines, c.line)))
        }
    }
    return <- out
}

// The census grouped under the document's own headings, in document order, followed by the
// citations naming an anchor the document does not declare - a rename that got away.
def private write_census(var w : StringBuilderWriter; doc : string; sites : array<Site>) {
    var by_anchor : table<string; array<int>>
    for (i in range(length(sites))) {
        by_anchor[sites[i].anchor] |> push(i) // nolint:PERF006 - one bucket per anchor cited, so no bucket's size is known up front
    }
    var declared : table<string>
    for (a in collect_md_anchors(doc)) {
        continue if (declared |> key_exists(a.name))
        declared |> insert(a.name)
        let span = section_span(doc, a.name)
        // guarded: reading a mutable table by key would INSERT the anchor as a bucket of its own
        if (!(by_anchor |> key_exists(a.name))) {
            w |> write("\{#{a.name}\} (lines {span.x}-{span.y - 1}) - no citations\n")
            continue
        }
        let count = length(by_anchor[a.name])
        w |> write("\{#{a.name}\} (lines {span.x}-{span.y - 1}) - {count} citation(s)\n")
        write_sites(w, sites, by_anchor[a.name])
    }
    var orphans : array<string>
    for (s in sites) {
        continue if (declared |> key_exists(s.anchor) || find_index(orphans, s.anchor) >= 0)
        orphans |> push(s.anchor)
    }
    return if (empty(orphans))
    w |> write("\ncitations naming an anchor this document does not declare:\n")
    for (name in orphans) {
        w |> write("\{#{name}\}\n")
        write_sites(w, sites, by_anchor[name])
    }
}

def private anchor_declared(doc, name : string) : bool {
    for (a in collect_md_anchors(doc)) {
        return true if (a.name == name)
    }
    return false
}

def do_arch_sites(doc : string) : string {
    return make_tool_result("'doc' argument is required", true) if (empty(doc))
    let hash = find(doc, "#")
    let anchor = hash >= 0 ? slice(doc, hash + 1) : ""
    let doc_arg = hash >= 0 ? slice(doc, 0, hash) : doc
    return make_tool_result("'doc' names an anchor but no document: {doc}", true) if (empty(doc_arg))
    let resolved = arch_path(resolve_path(doc_arg))
    let st = stat(resolved)
    return make_tool_result("Document not found: {doc_arg}", true) if (!(st.is_valid && st.is_reg))
    let folder = dir_name(resolved)
    var sites <- collect_sites(folder, resolved, anchor)
    // a C++ pointer is not bound to the document's folder tree (a header under include/ cites
    // src/'s document), so the census also reads the C++ homes outside the subtree
    var searched_in <- [make_relative_path(folder, get_das_root())]
    // a site is a citation, not a line: one comment can point at two anchors of this document
    var seen <- {for (s in sites); "{s.file}:{s.line}#{s.anchor}" => null}
    let roots <- cpp_roots()
    searched_in |> reserve(1 + length(roots))
    for (croot in roots) {
        searched_in |> push(make_relative_path(croot, get_das_root()))
        let more <- collect_sites(croot, resolved, anchor)
        // a C++ root may contain the document's own folder: the same site is not listed twice
        sites |> push_from([for (s in more); s; where !(seen |> key_exists("{s.file}:{s.line}#{s.anchor}"))])
        for (s in more) {
            seen |> insert("{s.file}:{s.line}#{s.anchor}", null)
        }
    }
    var files : array<string>
    for (s in sites) {
        continue if (find_index(files, s.file) >= 0)
        files |> push(s.file)
    }
    let title = empty(anchor) ? resolved : "{resolved} \{#{anchor}\}"
    let searched = join(searched_in, ", ")
    return make_tool_result(build_string() $(var w) {
        w |> write("{title}\nsearched: {searched}\n")
        if (empty(anchor)) {
            w |> write("\n")
            write_census(w, resolved, sites)
        } else {
            if (!anchor_declared(resolved, anchor)) {
                w |> write("note: this document declares no such anchor - a citation naming it does not resolve\n")
            } else {
                let span = section_span(resolved, anchor)
                w |> write("section: lines {span.x}-{span.y - 1}\n")
            }
            w |> write("\n")
            let which <- [for (i in range(length(sites))); i]
            write_sites(w, sites, which)
        }
        w |> write("\n{length(sites)} citation(s) in {length(files)} file(s).\n")
    })
}
