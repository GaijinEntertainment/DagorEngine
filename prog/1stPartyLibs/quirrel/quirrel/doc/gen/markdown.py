"""A markdown subset for the reference pages, plus the Quirrel highlighter.

Only what the pages use is implemented: paragraphs, `-` lists, tables, fenced
code, inline code, links, bold and italic. Anything else is emitted as plain text.
The dialect is deliberately small so that the build has no third-party
dependency and CI needs nothing but python3.

Two additions carry the cross-referencing the site is built around:

- an inline code span that names a known symbol becomes a link to it, which is
  what makes the pages dense with cross-references without hand-written markup,
- `[text](sym:math.clamp)` links by symbol id, and an unresolved id fails the build.
"""

import html
import re

# Keywords from the compiler lexer; kept in one list because the highlighter only
# needs to tell a keyword from an identifier.
KEYWORDS = """
base break case catch class clone const continue default delete do else enum
false for foreach function global if in instanceof let local null resume
return static switch this throw true try typeof while yield not
""".split()

_CODE_TOKEN = re.compile(r"""
    (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<string>\$?"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*'|@"(?:[^"]|"")*")
  | (?P<number>\b0[xX][0-9a-fA-F]+\b|\b\d+\.?\d*(?:[eE][-+]?\d+)?\b)
  | (?P<name>[A-Za-z_]\w*)
""", re.VERBOSE | re.DOTALL)


def highlight(code):
    """Marks up Quirrel source. Interpolated strings keep one colour: the pages
    show short snippets, so splitting `{}` holes out adds noise, not information."""
    out, pos = [], 0
    for m in _CODE_TOKEN.finditer(code):
        out.append(html.escape(code[pos:m.start()]))
        text = html.escape(m.group(0))
        kind = m.lastgroup
        if kind == "name":
            if m.group(0) in KEYWORDS:
                out.append(f'<span class="k">{text}</span>')
            else:
                out.append(text)
        else:
            out.append(f'<span class="{kind[0]}">{text}</span>')
        pos = m.end()
    out.append(html.escape(code[pos:]))
    return "".join(out)


def js_lexer():
    """What theme/highlight.js needs to colour an edited sample the way this module
    coloured the untouched one. Shared rather than copied, so the two cannot disagree
    about what a token is; gen/check_highlight.js compares the rendered output."""
    # re.VERBOSE drops whitespace outside character classes, and this pattern has
    # none inside them, so stripping all of it leaves the same matcher. Named groups
    # are spelled without the P in JS.
    pattern = re.sub(r"\s+", "", _CODE_TOKEN.pattern).replace("(?P<", "(?<")
    return {"keywords": KEYWORDS, "pattern": pattern}


def _anchor(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


_TABLE_RULE = re.compile(r"^\|(?:\s*:?-{3,}:?\s*\|)+\s*$")


_CELL_SPLIT = re.compile(r"(?<!\\)\|")


def _table_cells(line):
    r"""Splits one `| a | b |` row. A trailing pipe is optional, and `\|` is a pipe
    inside a cell, which a table of operators needs to hold `a || b`."""
    row = line.strip()
    if row.startswith("|"):
        row = row[1:]
    if row.endswith("|") and not row.endswith(r"\|"):
        row = row[:-1]
    return [c.strip().replace(r"\|", "|") for c in _CELL_SPLIT.split(row)]


class Renderer:
    def __init__(self, site, base_depth, on_missing_ref, shadow=(), local=()):
        self.site = site
        self.base_depth = base_depth      # how many `../` a link from this page needs
        self.on_missing_ref = on_missing_ref
        # Names that must not auto-link on this page. A function's own parameters
        # shadow global symbols exactly as they do in the language: on math.clamp,
        # `min` is its lower bound, not math.min. An explicit sym: link still works.
        self.shadow = set(shadow)
        # Bare names that may auto-link here: this container's members, the root
        # globals and the module names. Anything else needs the qualified id, so
        # that `min` meaning a minute field on a datetime page cannot silently
        # become a link to math.min.
        self.local = set(local)

    def rel(self, url):
        return "../" * self.base_depth + url

    # -- inline ----------------------------------------------------------------

    def inline(self, text, autolink=True):
        out, pos = [], 0
        # Bold before italic: at a `**` the italic branch cannot match anyway, since
        # its body excludes `*`, but the order says the intent. A code span is
        # matched first of all, so an asterisk inside one stays literal.
        pattern = re.compile(
            r"`(?P<code>[^`]+)`"
            # A label may hold a code span, and that span may hold a `]`: the label of
            # a link to the array literal page is `[1, 2, 3]`.
            r"|\[(?P<label>(?:[^\]`]|`[^`]*`)+)\]\((?P<href>[^)]+)\)"
            r"|\*\*(?P<bold>[^*]+)\*\*"
            r"|\*(?P<em>[^*]+)\*")
        for m in pattern.finditer(text):
            out.append(html.escape(text[pos:m.start()]))
            if m.group("code") is not None:
                out.append(self._code_span(m.group("code"), autolink))
            elif m.group("bold") is not None:
                # Recurse, so a code span inside bold is still a code span. The
                # pattern cannot match a `*`, so the inner text has no bold of its
                # own and this always terminates.
                out.append(f"<strong>{self.inline(m.group('bold'), autolink)}</strong>")
            elif m.group("em") is not None:
                out.append(f"<em>{self.inline(m.group('em'), autolink)}</em>")
            else:
                out.append(self._link(m.group("label"), m.group("href")))
            pos = m.end()
        out.append(html.escape(text[pos:]))
        return "".join(out)

    def _code_span(self, text, autolink=True):
        # No syntax colouring inline: a span is usually a name or a literal error
        # message, and colouring the english words in "Class not found in ..." as
        # keywords is noise. Fenced blocks still get the full highlighter.
        body = html.escape(text)
        if autolink and text not in self.shadow and ("." in text or text in self.local):
            url = self.site.xrefs.get(text)
            if url:
                return f'<a class="xref" href="{self.rel(url)}"><code>{body}</code></a>'
        return f"<code>{body}</code>"

    def href_of(self, target):
        """A `page:` or `sym:` target as an href, or None when it resolves to nothing.

        A miss is reported to the build, which fails on it: a reference nobody can
        follow is the one defect this site cannot ship.
        """
        if target.startswith("page:"):
            # Narrative pages can sit in a subdirectory, so a plain relative href
            # would resolve against the wrong directory. Go through the registry.
            name, _, anchor = target[5:].partition("#")
            url = self.site.page_urls.get(name)
            if url is None:
                self.on_missing_ref(f"page:{name}")
                return None
            if anchor and anchor not in self.site.page_anchors.get(name, set()):
                self.on_missing_ref(f"page:{name}#{anchor}")
                return None
            return self.rel(url) + (f"#{anchor}" if anchor else "")
        if target.startswith("sym:"):
            url = self.site.xrefs.get(target[4:])
            if url is None:
                self.on_missing_ref(target[4:])
                return None
            return self.rel(url)
        return target

    def _link(self, label, target):
        href = self.href_of(target)
        if href is None:
            return f'<span class="broken">{html.escape(label)}</span>'
        # The label must not auto-link: a nested <a> inside this one is invalid HTML.
        return f'<a href="{html.escape(href)}">{self.inline(label, autolink=False)}</a>'

    def _table(self, head, rows):
        cells = "".join(f"<th>{self.inline(c)}</th>" for c in head)
        return (f'<table class="md"><thead><tr>{cells}</tr></thead>'
                f"<tbody>{''.join(self._row(head, r) for r in rows)}</tbody></table>")

    def _row(self, head, row):
        """One row, which may carry a link target after its last cell.

        `| a | b | sym:x |` under a two column head makes the whole of `b` the link,
        so a reader hits the section from anywhere in the cell rather than aiming at
        one word in it. A cell that already links something of its own is left alone:
        those links say more than one target for the whole row could.
        """
        target = None
        if len(row) == len(head) + 1 and row[-1].startswith(("page:", "sym:")):
            row, target = row[:-1], row[-1]
        out = [self.inline(c) for c in row]
        if target is not None and "<a " not in out[-1]:
            href = self.href_of(target)
            if href is None:
                out[-1] = f'<span class="broken">{out[-1]}</span>'
            else:
                out[-1] = f'<a class="rowref" href="{html.escape(href)}">{out[-1]}</a>'
        return "<tr>" + "".join(f"<td>{c}</td>" for c in out) + "</tr>"

    # -- block -----------------------------------------------------------------

    def block(self, text):
        out = []
        lines = text.split("\n")
        i = 0
        while i < len(lines):
            line = lines[i]
            if line.startswith("```"):
                # ```nut path/to/file.nut names the file the block is, for a sample
                # made of more than one. It gets the same header bar as a runnable
                # example, minus the Run button: these are illustrations, and the
                # test runner executes each committed sample on its own.
                lang, _, label = line[3:].strip().partition(" ")
                label = label.strip()
                body = []
                i += 1
                while i < len(lines) and not lines[i].startswith("```"):
                    body.append(lines[i])
                    i += 1
                i += 1
                code = "\n".join(body)
                rendered = highlight(code) if lang in ("nut", "quirrel", "") else html.escape(code)
                block = f'<pre class="code"><code>{rendered}</code></pre>'
                if label:
                    block = ('<div class="filesample">'
                             f'<div class="exbar"><span>{html.escape(label)}</span></div>'
                             f"{block}</div>")
                out.append(block)
                continue
            if line.startswith("### "):
                # `## ` is consumed by the section splitter, so only sub-headings
                # reach this far.
                text = line[4:].strip()
                out.append(f'<h3 id="{html.escape(_anchor(text))}">{html.escape(text)}</h3>')
                i += 1
                continue
            if line.startswith("|") and i + 1 < len(lines) and _TABLE_RULE.match(lines[i + 1]):
                head = _table_cells(line)
                i += 2
                rows = []
                while i < len(lines) and lines[i].startswith("|"):
                    rows.append(_table_cells(lines[i]))
                    i += 1
                out.append(self._table(head, rows))
                continue
            if line.startswith("- "):
                items = []
                while i < len(lines):
                    cur = lines[i]
                    if cur.startswith("- "):
                        items.append(cur[2:].strip())
                    elif items and cur.strip() and not cur.startswith("```"):
                        items[-1] += " " + cur.strip()   # a wrapped bullet continues the last one
                    else:
                        break
                    i += 1
                out.append("<ul>" + "".join(f"<li>{self.inline(x)}</li>" for x in items) + "</ul>")
                continue
            if not line.strip():
                i += 1
                continue
            para = []
            while i < len(lines) and lines[i].strip() and not lines[i].startswith(("- ", "```")):
                para.append(lines[i])
                i += 1
            out.append(f"<p>{self.inline(' '.join(para))}</p>")
        return "\n".join(out)
