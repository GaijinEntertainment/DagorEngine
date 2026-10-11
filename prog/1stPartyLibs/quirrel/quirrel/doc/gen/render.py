"""Turns the site model into HTML pages."""

import html
import os
import re

from markdown import Renderer, highlight
from model import TYPE_HINT, load_page_example, anchor as model_anchor

PAGE = """<!doctype html>
<html lang="en" data-depth="{depth}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<script>
try {{
  const theme = localStorage.getItem("quirrel:theme");
  if (theme === "light" || theme === "light-plain" || theme === "dark" || theme === "dark-plain") document.documentElement.dataset.theme = theme;
}} catch (e) {{}}
</script>
<link rel="stylesheet" href="{root}style.css">
</head>
<body>
<a class="skip" href="#main">Skip to the page</a>
<header class="topbar">
  <button id="menu" class="menu" type="button" aria-controls="sidebar" aria-expanded="false">Sections</button>
  <span class="title"><a class="brand" href="{root}index.html">Quirrel</a>{version}</span>
  <nav class="crumbs">{crumbs}</nav>
  <div class="themes" role="radiogroup" aria-label="Colour theme">
    <input id="theme-light" class="theme-choice sr" type="radio" name="theme" value="light" data-theme-control aria-label="Light theme">
    <label class="theme" for="theme-light" title="Light theme"></label>
    <input id="theme-light-plain" class="theme-choice sr" type="radio" name="theme" value="light-plain" data-theme-control aria-label="Light plain theme">
    <label class="theme" for="theme-light-plain" title="Light plain theme"></label>
    <input id="theme-dark" class="theme-choice sr" type="radio" name="theme" value="dark" data-theme-control aria-label="Dark theme">
    <label class="theme" for="theme-dark" title="Dark theme"></label>
    <input id="theme-dark-plain" class="theme-choice sr" type="radio" name="theme" value="dark-plain" data-theme-control aria-label="Dark plain theme">
    <label class="theme" for="theme-dark-plain" title="Dark plain theme"></label>
  </div>
  <label class="sr" for="q">Search the reference</label>
  <input id="q" type="search" placeholder="Search  /" autocomplete="off" spellcheck="false"
         role="combobox" aria-autocomplete="list" aria-expanded="false" aria-controls="results-list">
  <div id="results" hidden>
    <div id="results-status" role="status"></div>
    <div id="results-list" role="listbox" aria-label="Search results"></div>
    <div id="results-more" role="status"></div>
  </div>
</header>
<div class="scrim" hidden></div>
<div class="layout">
<nav id="sidebar" class="sidebar" aria-label="Sections">{sidebar}</nav>
<main id="main" class="{main_class}" tabindex="-1">{main}</main>
</div>
<script src="{root}theme.js" defer></script>
<script src="{root}nav.js" defer></script>
<script src="{root}search.js" defer></script>
<script src="{root}lexer.js" defer></script>
<script src="{root}highlight.js" defer></script>
<script src="{root}run.js" defer></script>
</body>
</html>
"""


# The sidebar heading the landing page and the "Coming from ..." pages share. The
# landing page is that group's overview, so the heading links to it.
INTRO_GROUP = "Introduction"



def _version_tag(site):
    """The interpreter version, next to the brand on every page.

    Which build the pages describe is not a detail: a signature read from one VM
    can be wrong for another. Empty when build.py could not run the interpreter.
    """
    if not site.version:
        return ""
    return f'<span class="ver">{html.escape(site.version)}</span>'


def _rel(depth, url):
    return "../" * depth + url


def _depth_of(url):
    return url.count("/")


def _group_heading(label, url, depth, here=False):
    """A sidebar heading, a link when the group has an overview page.

    A chapter a reader cannot open is a dead end: the overview says what the chapter
    is and describes each page in it, so the heading carries that link."""
    if not url:
        return f'<div class="group">{html.escape(label)}</div>'
    cls = ' class="here"' if here else ""
    return (f'<div class="group"><a{cls} href="{_rel(depth, url)}">'
            f'{html.escape(label)}</a></div>')


def _sidebar(site, current_container, depth, current_page=None, on_index=False):
    """The whole sidebar, group by group in the order content/_meta.json lists them.

    A group holds pages or containers, so which of the two it is says how to head it:
    a page group has an overview page, a container group has a section of the landing
    page. INTRO_GROUP is the exception, because its overview is the landing page.
    """
    out = []
    for group in site.groups:
        pages = site.pages_of(group)
        containers = site.containers_of(group)
        if group == INTRO_GROUP:
            out.append(_group_heading(group, "index.html", depth, on_index))
        elif pages:
            index = site.group_index(group)
            out.append(_group_heading(group, index.url if index else None, depth,
                                      index is current_page))
        else:
            out.append(_group_heading(group, f"index.html#{model_anchor(group)}", depth))
        for page in pages:
            if page.group_index:
                continue    # the heading above it is its link
            cls = ' class="here"' if current_page is page else ""
            out.append(f'<a{cls} href="{_rel(depth, page.url)}">{html.escape(page.title)}</a>')
        for c in containers:
            cls = ' class="here"' if current_container is c else ""
            out.append(f'<a{cls} href="{_rel(depth, c.url)}">{html.escape(c.title)}</a>')
    return "\n".join(out)


def _crumbs(site, parts, depth):
    """The trail after the brand. It does not repeat the site name: the brand beside
    it is already the link home."""
    items = []
    for label, url in parts:
        if url:
            items.append(f'<a href="{_rel(depth, url)}">{html.escape(label)}</a>')
        else:
            items.append(f"<span>{html.escape(label)}</span>")
    return '<span class="sep">&rsaquo;</span>'.join(items)


def _type_link(site, depth):
    def link(name):
        url = site.type_url(name)
        if url:
            return f'<a class="type" href="{_rel(depth, url)}">{html.escape(name)}</a>'
        hint = TYPE_HINT.get(name)
        if hint:
            return f'<span class="type" title="{html.escape(hint)}">{html.escape(name)}</span>'
        return f'<span class="type">{html.escape(name)}</span>'
    return link


def _defined_in(symbol, depth):
    c = symbol.container
    if c.kind == "root":
        return "Global, available without an import"
    if c.kind == "module":
        import_line = highlight(f'from "{c.id}" import {symbol.name}')
        return f'Defined in module <code>"{html.escape(c.id)}"</code><pre class="code import"><code>{import_line}</code></pre>'
    if c.group == "Types":
        return f"Method on every <code>{html.escape(c.title)}</code>"
    if c.inlined:
        href = _rel(depth, c.link_url)
        return (f'Method of <code><a href="{href}">{html.escape(c.title)}</a></code>, '
                f'from module <code>"{html.escape(c.parent.id)}"</code>')
    return f'Method of <code>{html.escape(c.title)}</code>'


_PARAM_LINE = re.compile(r"^-\s+`([^`]+)`\s*-\s*(.*)$")


def _param_docs(section_text, symbol):
    """Reads the `- \\`name\\` - description` list of a Parameters section.

    A name that is not in the signature is an error: it means the prose and the VM
    disagree, which is exactly what this site exists to prevent."""
    docs = {}
    last = None
    for line in section_text.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        m = _PARAM_LINE.match(stripped)
        if m:
            last = m.group(1)
            docs[last] = m.group(2)
        elif last is not None:
            docs[last] += " " + stripped        # a wrapped bullet continues the last one
        else:
            raise SystemExit(f"{symbol.body.path}: cannot read parameter line {stripped!r}")

    known = {p.name for p in symbol.signature.params} if symbol.signature else set()
    unknown = set(docs) - known - {"..."}
    if unknown:
        raise SystemExit(f"{symbol.body.path}: documents parameters {sorted(unknown)} "
                         f"that {symbol.ref} does not take (it takes {sorted(known)})")
    return docs


def _container_crumbs(container):
    """The trail of a container. A class the module page carries appears under the
    module, so a reader can get back to either one."""
    if container.inlined:
        return [(container.parent.title, container.parent.url),
                (container.title, container.link_url)]
    return [(container.title, container.url)]


def render_symbol(site, symbol, missing_refs):
    depth = _depth_of(symbol.url)
    shadow = {symbol.name}
    if symbol.signature:
        shadow.update(p.name for p in symbol.signature.params)
    md = Renderer(site, depth, lambda ref: missing_refs.append((symbol.ref, ref)), shadow,
                  local=site.local_names(symbol.container))
    type_link = _type_link(site, depth)

    body = [f'<h1 class="symbol">{html.escape(symbol.ref)}</h1>']

    if symbol.signature:
        badges = "".join(
            f'<a class="badge" href="{_rel(depth, "attributes.html")}#{a}">{a}</a>'
            for a in symbol.signature.attrs)
        body.append(f'<div class="badges">{badges}</div>')

    box = [f'<div class="defined">{_defined_in(symbol, depth)}</div>']
    if symbol.signature:
        box.append(f'<div class="sig">{symbol.signature.render(link_type=type_link)}</div>')
    elif symbol.kind in ("integer", "float", "string", "bool"):
        value = html.escape(symbol.raw.get("value", ""))
        box.append(f'<div class="sig"><span class="fn">{html.escape(symbol.name)}</span> '
                   f'= <span class="n">{value}</span> <span class="type">({symbol.kind})</span></div>')
    body.append('<div class="declbox">' + "".join(box) + "</div>")

    if symbol.summary:
        body.append(f'<p class="summary">{md.inline(symbol.summary)}</p>')

    if symbol.signature and symbol.signature.has_placeholder_names:
        body.append('<div class="warn">This function is bound with a type mask instead of a '
                    'declaration string. The VM cannot report parameter names, and cannot tell '
                    'an optional parameter from a variadic tail, so the brackets and the '
                    'trailing <code>...</code> above are not evidence either way.</div>')
    elif symbol.names_from_doc:
        body.append('<div class="warn">The binding carries no declaration string, so the VM '
                    'cannot report parameter names. The names above are from this page; the '
                    'types and attributes still come from the VM. The VM also cannot tell an '
                    'optional parameter from a variadic tail here, so read the brackets and any '
                    'trailing <code>...</code> from the prose below, not from the signature.</div>')

    if symbol.basic is not None:
        # The small example goes before the reference sections: what the thing is
        # for, ahead of every way it can behave.
        body.append(_example_block(
            f"examples/{symbol.container.slug}/{symbol.name}-basic.nut", symbol.basic))

    if not symbol.documented:
        body.append('<div class="warn">This page has no written reference yet. '
                    'Everything above comes from the VM.</div>')
    else:
        for heading, text in symbol.body.sections:
            if not heading:
                continue
            if heading == "Parameters" and symbol.signature:
                body.append(f'<h2 id="{_anchor("Parameters")}">Parameters</h2>')
                body.append(_render_params(symbol, _param_docs(text, symbol), type_link, md))
            elif heading == "Example":
                body.append(_render_example(symbol, text, md))
            else:
                body.append(f'<h2 id="{html.escape(_anchor(heading))}">{html.escape(heading)}</h2>')
                body.append(md.block(text))

    body.append(_render_see_also(site, symbol, depth, md))

    crumbs = _crumbs(site, [
        *_container_crumbs(symbol.container),
        (symbol.name, None),
    ], depth)

    return PAGE.format(
        depth=depth, root="../" * depth, main_class="", version=_version_tag(site),
        title=f"{symbol.ref} - Quirrel reference",
        crumbs=crumbs, sidebar=_sidebar(site, symbol.container, depth),
        main="\n".join(body))


def _render_params(symbol, docs, type_link, md):
    if not symbol.signature.params:
        return "<p>none</p>"
    rows = []
    for p in symbol.signature.params:
        name = "..." if p.vararg else p.name
        types = "|".join(type_link(t) for t in p.types) if p.types else '<span class="type">any</span>'
        note = md.inline(docs.get(name, ""))
        if p.optional:
            note += ' <span class="tag">optional</span>'
        if p.vararg:
            note += ' <span class="tag">repeats</span>'
        rows.append(f"<tr><td><code>{html.escape(name)}</code></td><td>{types}</td><td>{note}</td></tr>")
    return '<table class="params"><tbody>' + "".join(rows) + "</tbody></table>"


def _render_example(symbol, text, md):
    out = [f'<h2 id="{_anchor("Example")}">Example</h2>']
    prose = "\n".join(l for l in text.splitlines() if not l.strip().startswith("{{example:"))
    if prose.strip():
        out.append(md.block(prose))
    if "{{example:" not in text:
        return "\n".join(out)
    if symbol.example is None:
        raise SystemExit(f"{symbol.body.path}: asks for an example but "
                         f"examples/{symbol.container.slug}/{symbol.name}.nut does not exist")
    out.append(
        '<div class="example">'
        f'<div class="exbar"><span>examples/{html.escape(symbol.container.slug)}/{html.escape(symbol.name)}.nut</span>'
        '<button class="run">Run this code</button></div>'
        f'<pre class="code"><code id="src">{highlight(symbol.example.source)}</code></pre>'
        '<div class="outlabel">Output:</div>'
        f'<pre class="out"><code>{html.escape(symbol.example.output)}</code></pre>'
        "</div>")
    return "\n".join(out)


def _see_also_operator(site, symbol, op, depth, md):
    """A See also row for an operator, taken from the operator table.

    An operator is not a symbol and has no page, but `type` next to `typeof` is
    exactly the pair a reader has to see, so see_also takes `op:typeof` as well.
    """
    for group in site.operators:
        for entry in group["operators"]:
            if entry["op"] != op:
                continue
            page, _, anchor = entry["target"].partition("#")
            url = site.page_urls[page] + (f"#{anchor}" if anchor else "")
            return (f'<tr><td><a href="{_rel(depth, url)}"><code>{html.escape(op)}</code></a></td>'
                    f'<td>{md.inline(entry["what"])}</td></tr>')
    raise SystemExit(f"{symbol.ref}: see_also names the operator {op!r}, "
                     "which content/_operators.json does not have")


def _render_see_also(site, symbol, depth, md):
    refs = list(symbol.body.see_also) if symbol.documented else []
    if not refs:
        # Fall back to the rest of the container, which is what a reader wants next
        # anyway, capped so the block stays scannable.
        refs = [s.ref for s in symbol.container.functions if s is not symbol][:8]
    rows = []
    for ref in refs:
        if ref.startswith("op:"):
            rows.append(_see_also_operator(site, symbol, ref[3:], depth, md))
            continue
        target = site.by_ref.get(ref)
        if target is None:
            raise SystemExit(f"{symbol.ref}: see_also names {ref!r}, which does not exist")
        rows.append(
            f'<tr><td><a href="{_rel(depth, target.url)}"><code>{html.escape(target.name)}</code></a></td>'
            f"<td>{md.inline(target.summary)}</td></tr>")
    what = "module index" if symbol.container.kind in ("module", "root") else "class index"
    rows.append(f'<tr><td><a href="{_rel(depth, symbol.container.link_url)}">'
                f'{html.escape(symbol.container.title)}</a></td>'
                f"<td>{what}</td></tr>")
    return ('<h2 id="see-also">See also</h2><table class="seealso"><tbody>'
            + "".join(rows) + "</tbody></table>")


def _member_sections(container, depth, md, type_link, level):
    """Values then functions: a constant is read before the call that takes it."""
    out = []
    for label, members in (("Values", container.values), ("Functions", container.functions)):
        if not members:
            continue
        ident = _anchor(f"{container.title} {label}") if level > 2 else _anchor(label)
        out.append(f'<h{level} id="{html.escape(ident)}">{label}</h{level}>')
        rows = []
        for m in members:
            sig = (m.signature.render(link_type=type_link)
                   if m.signature else f'<span class="fn">{html.escape(m.name)}</span>')
            link = f'<a href="{_rel(depth, m.url)}">{html.escape(m.name)}</a>'
            rows.append(f'<tr><td class="nm">{link}</td><td class="sg">{sig}</td>'
                        f"<td>{md.inline(m.summary)}</td></tr>")
        out.append('<table class="members"><tbody>' + "".join(rows) + "</tbody></table>")
    return out


def render_container(site, container, missing_refs):
    depth = _depth_of(container.url)
    md = Renderer(site, depth, lambda ref: missing_refs.append((container.id, ref)),
                  local=site.local_names(container))
    type_link = _type_link(site, depth)

    body = [f'<h1>{html.escape(container.title)}</h1>']
    if container.blurb:
        body.append(f'<p class="summary">{html.escape(container.blurb)}</p>')
    if container.kind == "module":
        import_line = 'from "%s" import ...' % container.id
        body.append(f'<pre class="code import"><code>{highlight(import_line)}</code></pre>')

    if container.note is not None:
        for heading, text in container.note.sections:
            if heading:
                body.append(f'<h2 id="{html.escape(_anchor(heading))}">{html.escape(heading)}</h2>')
            if text:
                body.append(_page_body(site, container.note.path, text, md, depth))

    body.extend(_member_sections(container, depth, md, type_link, level=2))

    # A class with a page of its own is only listed here; one the module carries is
    # printed below in full.
    linked = [c for c in container._children if not c.inlined]
    if linked:
        rows = []
        for c in linked:
            rows.append(f'<tr><td class="nm"><a href="{_rel(depth, c.url)}">{html.escape(c.title)}</a></td>'
                        f'<td>{html.escape(c.blurb)}</td>'
                        f'<td class="count">{len(c.members)}</td></tr>')
        body.append('<h2 id="classes">Classes</h2><table class="members"><tbody>'
                    + "".join(rows) + "</tbody></table>")

    # A class the module carries follows everything the module itself has: a reader
    # after io.open should not scroll past the whole of file to reach it.
    for child in container.inlined_children:
        # The badge tells the class heading apart from the Values and Functions of
        # the module, which sit at the same level.
        body.append(f'<h2 id="{html.escape(child.anchor)}">{html.escape(child.title)}'
                    ' <span class="badge">class</span></h2>')
        if child.blurb:
            body.append(f'<p class="summary">{html.escape(child.blurb)}</p>')
        if child.note is not None:
            for heading, text in child.note.sections:
                if heading:
                    body.append(f'<h3 id="{html.escape(_anchor(heading))}">{html.escape(heading)}</h3>')
                if text:
                    body.append(_page_body(site, child.note.path, text, md, depth))
        body.extend(_member_sections(child, depth, md, type_link, level=3))

    crumbs = _crumbs(site, _container_crumbs(container)[:-1] + [(container.title, None)],
                     depth)
    return PAGE.format(
        depth=depth, root="../" * depth, main_class="", version=_version_tag(site),
        title=f"{container.title} - Quirrel reference",
        crumbs=crumbs, sidebar=_sidebar(site, container, depth), main="\n".join(body))


def _group_index_url(site, group):
    index = site.group_index(group)
    return index.url if index else None


def _page_crumbs(site, page):
    """The trail of a narrative page. The overview of a group does not repeat itself:
    it is the group, so the group name alone is the whole trail."""
    if page.group_index:
        return [(page.group, None)]
    return [(page.group, _group_index_url(site, page.group)), (page.title, None)]


def render_page(site, page, missing_refs):
    depth = page.depth
    md = Renderer(site, depth, lambda ref: missing_refs.append((page.name, ref)),
                  local=site.local_names())
    body = [f"<h1>{html.escape(page.title)}</h1>"]
    for heading, text in page.content.sections:
        if heading:
            # The badge on every symbol page links to attributes.html#pure and the
            # like, so a heading id has to be the heading text itself.
            body.append(f'<h2 id="{html.escape(_anchor(heading))}">{html.escape(heading)}</h2>')
        if text:
            body.append(_page_body(site, page.content.path, text, md, depth, page.group))
    return PAGE.format(
        depth=depth, root="../" * depth, main_class=page.main_class, version=_version_tag(site),
        title=f"{page.title} - Quirrel reference",
        crumbs=_crumbs(site, _page_crumbs(site, page), depth),
        sidebar=_sidebar(site, None, depth, page), main="\n".join(body))


_DIRECTIVE_LINE = re.compile(
    r"^(?:\{\{example:(?P<example>[^}]+)\}\}"
    r"|\{\{capi:(?P<capi>[^}]+)\}\}"
    r"|\{\{subtopics:(?P<subtopics>[^}]+)\}\}"
    r"|\{\{(?P<subtopics_here>subtopics)\}\}"
    r"|\{\{(?P<keywords>keywords)\}\}"
    r"|\{\{(?P<benchmarks>benchmarks)\}\}"
    r"|\{\{(?P<vm_benchmarks>vm_benchmarks)\}\}"
    r"|\{\{(?P<metamethods>metamethods)\}\}"
    r"|\{\{(?P<operators>operators)\}\})\s*$", re.M)


CAPI_DIRECTIVE = re.compile(r"^\{\{capi:([^}]+)\}\}\s*$", re.M)


def capi_group_pages(site):
    """C API section name -> the url of the page that lists it.

    Read from the pages themselves rather than configured twice, so a section moved
    from one page to another takes its search entries and anchors with it.
    """
    where = {}
    for page in site.pages:
        for _heading, text in page.content.sections:
            for m in CAPI_DIRECTIVE.finditer(text):
                for group in m.group(1).split(","):
                    where.setdefault(group.strip(), page.url)
    return where


def capi_declaration(row):
    """The declaration as one line of plain text, for the search index."""
    params = ", ".join(row["params"]) or "void"
    ret = row["ret"] if row["ret"].endswith("*") else row["ret"] + " "
    return f'{ret}{row["qualified"]}({params})'


def _capi_signature(row):
    """The declaration as the header spells it, with the name picked out.

    The C types are not linked: they are the host's, not the language's, and a page
    that linked `SQInteger` to nothing useful would only add noise."""
    params = ", ".join(html.escape(p) for p in row["params"]) or "void"
    ret = html.escape(row["ret"])
    if not ret.endswith("*"):
        ret += " "
    return (f'<span class="type">{ret}</span>'
            f'<span class="fn">{html.escape(row["qualified"])}</span>'
            f"({params})")


def _capi_table(site, groups, where):
    """Every function in the named header sections, straight from the headers.

    Sections are named rather than listed function by function, so a function added
    to the header appears here without anyone editing this page."""
    wanted = [g.strip() for g in groups.split(",") if g.strip()]
    known = {row["group"] for row in site.capi}
    for g in wanted:
        if g not in known:
            raise SystemExit(f"{where}: no C API section named {g!r}; the headers have "
                             f"{sorted(known)}")
    # The declaration goes on a line of its own, with the description under it. Some
    # of these run past 900px and must not wrap, so a signature column would either
    # squeeze the prose to a ribbon or push the page sideways.
    out = []
    for row in site.capi:
        if row["group"] not in wanted:
            continue
        doc = site.capi_docs.get(row["name"], "")
        if not doc and row["note"]:
            # Fall back to what the header itself says, cut to its first sentence:
            # a comment written for an implementer runs longer than an entry here.
            doc = row["note"].split(". ")[0].rstrip(".") + "."
        body = (f"<p>{html.escape(doc)}</p>" if doc
                else '<p class="undoc">not described yet</p>')
        out.append(f'<div class="capifn" id="{html.escape(row["name"])}">'
                   f'<div class="capisig">{_capi_signature(row)}</div>{body}</div>')
    return '<div class="capi">' + "".join(out) + "</div>"


def _keyword_table(site, depth):
    """Every keyword the lexer registers, linked to the section that explains it.

    Rendered from content/_keywords.json so the list cannot drift from the lexer:
    gen/check.py compares the two.
    """
    cells = []
    for word in sorted(site.keywords, key=lambda w: w.lower().strip("_")):
        page_name, _, anchor = site.keywords[word].partition("#")
        url = site.page_urls[page_name] + (f"#{anchor}" if anchor else "")
        cells.append(f'<a class="kw" href="{_rel(depth, url)}"><code>{html.escape(word)}</code></a>')
    return '<div class="kwgrid">' + "".join(cells) + "</div>"


def _metamethod_table(site, depth, md):
    """Every metamethod the VM recognizes, from content/_metamethods.json.

    Rendered rather than written out, so the page cannot list sixteen of seventeen:
    gen/check.py compares the file with the MM_IMPL list in the VM.
    """
    rows = []
    for name, entry in site.metamethods.items():
        page_name, _, anchor = entry["target"].partition("#")
        url = site.page_urls[page_name] + (f"#{anchor}" if anchor else "")
        rows.append(
            f'<tr id="mm-{html.escape(name)}"><td class="mm"><a href="{_rel(depth, url)}">'
            f'<code>{html.escape(entry["sig"])}</code></a></td>'
            f'<td>{md.inline(entry["when"])}</td>'
            f'<td>{md.inline(entry["this"])}</td></tr>')
    return ('<table class="mms"><thead><tr><th>metamethod</th><th>runs when</th>'
            '<th><code>this</code></th></tr></thead><tbody>'
            + "".join(rows) + "</tbody></table>")


def _operator_table(site, depth):
    """Every operator, grouped, each linked to the section that explains it and to
    the metamethod that overloads it. Rendered from content/_operators.json."""
    rows = []
    for group in site.operators:
        rows.append(f'<tr class="opgroup"><th colspan="3">{html.escape(group["name"])}</th></tr>')
        for entry in group["operators"]:
            page_name, _, anchor = entry["target"].partition("#")
            url = site.page_urls[page_name] + (f"#{anchor}" if anchor else "")
            mm = entry.get("mm")
            mm_cell = ""
            if mm:
                # Link the overload to its contract rather than just naming it: an
                # operator table is where someone looks first for "how do I overload
                # this", and the answer is a page away.
                mm_entry = site.metamethods.get(mm)
                code = f"<code>{html.escape(mm)}</code>"
                if mm_entry:
                    mm_page, _, mm_anchor = mm_entry["target"].partition("#")
                    mm_url = site.page_urls[mm_page] + (f"#{mm_anchor}" if mm_anchor else "")
                    code = f'<a href="{_rel(depth, mm_url)}">{code}</a>'
                mm_cell = code
            rows.append(
                f'<tr><td class="op"><a href="{_rel(depth, url)}"><code>'
                f'{html.escape(entry["op"])}</code></a></td>'
                f'<td>{html.escape(entry["what"])}</td>'
                f'<td class="mm">{mm_cell}</td></tr>')
    return ('<table class="ops"><thead><tr><th>operator</th><th></th>'
            '<th>metamethod</th></tr></thead><tbody>' + "".join(rows) + "</tbody></table>")


def _subtopics_table(site, group, depth, where, md):
    """The pages of one group, each with its summary.

    Generated rather than written out, so a new page in a group cannot be missing
    from its overview, and the order matches the sidebar."""
    if not group:
        raise SystemExit(f"{where}: {{{{subtopics}}}} works on a page of a group; "
                         "name the group as {{subtopics:Language}}")
    pages = [p for p in site.pages if p.group == group and not p.group_index]
    if not pages:
        raise SystemExit(f"{where}: no page has group {group!r}")
    rows = []
    for page in pages:
        if not page.summary:
            raise SystemExit(f"content/pages/{page.name}.md: needs a summary:, "
                             f"because the {group} overview lists it")
        rows.append(f'<tr><td class="nm"><a href="{_rel(depth, page.url)}">'
                    f'{html.escape(page.title)}</a></td>'
                    f'<td>{md.inline(page.summary)}</td></tr>')
    return '<table class="members subtopics"><tbody>' + "".join(rows) + "</tbody></table>"


def _missing_bench(what, how):
    """A gap the reader can see, the way an undocumented symbol says so on its page."""
    return (f'<p class="nobench">No {what} are committed yet. '
            f'Run <code>{how}</code> and commit the side-car it writes.</p>')


def _bench_info(info, note=""):
    """What the committed numbers were measured on, so a reader can weigh them."""
    parts = []
    if info:
        where = [info.get("cpu") or info.get("arch"), info.get("platform"), info.get("when")]
        parts.append("Measured on " + ", ".join(p for p in where if p) + ".")
    if note:
        parts.append(note)
    if not parts:
        return ""
    return '<p class="benchinfo">' + html.escape(" ".join(parts)) + "</p>"


def _bench_bars(site):
    """One bar chart per workload, longest bar last.

    The width is the share of the slowest row, which is what makes two charts with
    different absolute times comparable at a glance.
    """
    if not site.bench:
        return _missing_bench("cross-language benchmarks", "python bench/benchmarks.py")
    featured = site.bench.get("featured", "")
    out = []
    for workload, rows in site.bench["results"].items():
        timed = sorted(((lang, t) for lang, t in rows.items() if isinstance(t, (int, float))),
                       key=lambda row: row[1])
        if not timed:
            continue
        # A workload under a coarse clock can report 0, and it still gets a row.
        slowest = timed[-1][1] or 1
        bars = []
        for lang, seconds in timed:
            cls = "chart-bar featured" if lang == featured else "chart-bar"
            bars.append(f'<tr><th>{html.escape(lang)}</th>'
                        f'<td><div class="{cls}" style="width: {seconds / slowest * 100:.0f}%">'
                        f"{seconds:.3f}s</div></td></tr>")
        out.append(f'<h3 id="{_anchor(workload)}">{html.escape(workload)}</h3>'
                   '<table class="chart"><tbody>' + "".join(bars) + "</tbody></table>")
    runs = site.bench.get("runs")
    note = f"Best iteration of {runs} process runs." if runs else ""
    return "\n".join(out) + _bench_info(site.bench.get("info"), note)


def _vm_bench_table(site):
    """Quirrel against the other interpreters on the paired workloads.

    The ratio is quirrel over the other VM, so above 1.0 means Quirrel is slower -
    the same convention the harness prints.
    """
    if not site.vm_bench:
        return _missing_bench("Lua and Luau comparison numbers", "python bench/run_vm_bench.py")
    results = site.vm_bench["results"]
    vms = []
    for rows in results.values():
        for vm in rows:
            if vm not in vms:
                vms.append(vm)
    head = "".join(f"<th>{html.escape(vm)}</th>" for vm in vms)
    body = []
    for workload, rows in results.items():
        cells = []
        base = rows.get("quirrel")
        for vm in vms:
            ms = rows.get(vm)
            if not isinstance(ms, (int, float)):
                cells.append('<td class="num">-</td>')
            elif vm == "quirrel" or not base:
                cells.append(f'<td class="num">{ms:.0f}</td>')
            else:
                cells.append(f'<td class="num">{ms:.0f} '
                             f'<span class="ratio">({base / ms:.2f}x)</span></td>')
        body.append(f'<tr><th>{html.escape(workload)}</th>' + "".join(cells) + "</tr>")
    runs = site.vm_bench.get("runs")
    note = f"Median of the best rep over {runs} process runs." if runs else ""
    return ('<table class="vmbench"><thead><tr><th>workload</th>' + head
            + "</tr></thead><tbody>" + "".join(body) + "</tbody></table>"
            + _bench_info(site.vm_bench.get("info"), note))


def _page_body(site, where, text, md, depth, group=None):
    """Renders a narrative section, expanding the {{...}} directives in it.

    `where` is the source file, named only when a directive cannot be satisfied."""
    out, pos = [], 0
    for m in _DIRECTIVE_LINE.finditer(text):
        before = text[pos:m.start()].strip()
        if before:
            out.append(md.block(before))
        pos = m.end()
        if m.group("subtopics"):
            out.append(_subtopics_table(site, m.group("subtopics").strip(), depth, where, md))
            continue
        if m.group("subtopics_here"):
            out.append(_subtopics_table(site, group, depth, where, md))
            continue
        if m.group("keywords"):
            out.append(_keyword_table(site, depth))
            continue
        if m.group("benchmarks"):
            out.append(_bench_bars(site))
            continue
        if m.group("vm_benchmarks"):
            out.append(_vm_bench_table(site))
            continue
        if m.group("metamethods"):
            out.append(_metamethod_table(site, depth, md))
            continue
        if m.group("operators"):
            out.append(_operator_table(site, depth))
            continue
        if m.group("capi"):
            out.append(_capi_table(site, m.group("capi"), where))
            continue
        rel = m.group("example").strip()
        example = load_page_example(rel)
        if example is None:
            raise SystemExit(f"{where}: asks for example {rel!r}, but "
                             f"examples/pages/{rel}.nut does not exist")
        out.append(_example_block(f"examples/pages/{rel}.nut", example))
    rest = text[pos:].strip()
    if rest:
        out.append(md.block(rest))
    return "\n".join(out)


def _example_block(label, example):
    return (
        '<div class="example">'
        f'<div class="exbar"><span>{html.escape(label)}</span>'
        '<button class="run">Run this code</button></div>'
        f'<pre class="code"><code>{highlight(example.source)}</code></pre>'
        '<div class="outlabel">Output:</div>'
        f'<pre class="out"><code>{html.escape(example.output)}</code></pre>'
        "</div>")


def _anchor(heading):
    return model_anchor(heading)


def render_index(site, missing_refs):
    body = [f'<h1>{html.escape(site.meta["title"])}</h1>']

    if site.intro is not None:
        md = Renderer(site, 0, lambda ref: missing_refs.append(("index", ref)),
                      local=site.local_names())
        for heading, text in site.intro.sections:
            if heading:
                body.append(f'<h2 id="{html.escape(_anchor(heading))}">{html.escape(heading)}</h2>')
            if text:
                body.append(_page_body(site, site.intro.path, text, md, 0, INTRO_GROUP))
        # The module tables come after everything the intro says, so the intro's last
        # section is the one that introduces them. It supplies its own heading rather
        # than the generator naming the section here.

    for group in site.groups:
        containers = site.containers_of(group)
        if not containers:
            continue
        body.append(f'<h2 id="{html.escape(_anchor(group))}">{html.escape(group)}</h2>'
                    '<table class="members"><tbody>')
        for c in containers:
            body.append(f'<tr><td class="nm"><a href="{c.url}">{html.escape(c.title)}</a></td>'
                        f'<td>{html.escape(c.blurb)}</td>'
                        f'<td class="count">{c.own_and_inlined}</td></tr>')
        body.append("</tbody></table>")
    return PAGE.format(
        depth=0, root="", main_class="", version=_version_tag(site), title=site.meta["title"],
        crumbs="",
        sidebar=_sidebar(site, None, 0, on_index=True), main="\n".join(body))
