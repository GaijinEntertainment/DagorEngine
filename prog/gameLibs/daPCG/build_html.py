import html
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DOC_DIR = ROOT / "_doc"
OUT = DOC_DIR / "index.html"

FENCE_RE = re.compile(r"^```+\s*[\w+-]*\s*$")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
LIST_RE = re.compile(r"^( *)([-*+]|\d{1,9}[.)]|[A-Z]\.)(\s+)(.*)$")
TABLE_SEP_RE = re.compile(r"^\s*\|?\s*:?-+:?\s*(\|\s*:?-+:?\s*)*\|?\s*$")
CODE_SPAN_RE = re.compile(r"(`+)(.+?)\1")
ESCAPE_RE = re.compile(r"\\([\\`*_{}\[\]()#+\-.!|<>~])")
LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)\s]+)\)")
URL_RE = re.compile(r"https?://[^\s<>()]+[^\s<>().,;:'\"]")
BOLD_RE = re.compile(r"\*\*(?=\S)(.+?)(?<=\S)\*\*")
EM_RE = re.compile(r"(?<![\w*])\*(?=[^\s*])(.+?)(?<=[^\s*])\*(?![\w*])")
STASH_RE = re.compile("\x00(\\d+)\x00")


def slug(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-") or "section"


def doc_id(path):
    return "doc-" + slug(path.stem)


class Doc:
    def __init__(self, path, docs):
        self.path = path
        self.docs = docs
        self.id = doc_id(path)
        self.title = path.name
        self.used = set()
        self.frags = []
        self.warnings = []

    def stash(self, fragment):
        self.frags.append(fragment)
        return "\x00%d\x00" % (len(self.frags) - 1)

    def href(self, target):
        if re.match(r"^[a-z]+:", target):
            return target
        name, _, anchor = target.partition("#")
        resolved = Path(os.path.normpath(self.path.parent / name)) if name else self.path
        if resolved in self.docs:
            return "#" + doc_id(resolved) + ("-" + slug(anchor) if anchor else "")
        return os.path.relpath(resolved, DOC_DIR).replace(os.sep, "/") + ("#" + anchor if anchor else "")

    def emphasis(self, text):
        text = html.escape(text, quote=False)
        text = BOLD_RE.sub(r"<strong>\1</strong>", text)
        return EM_RE.sub(r"<em>\1</em>", text)

    def inline(self, text):
        text = CODE_SPAN_RE.sub(lambda m: self.stash("<code>%s</code>" % html.escape(m.group(2))), text)
        text = ESCAPE_RE.sub(lambda m: self.stash(html.escape(m.group(1))), text)
        text = LINK_RE.sub(lambda m: self.stash('<a href="%s">%s</a>' % (
            html.escape(self.href(m.group(2))), self.emphasis(m.group(1)))), text)
        text = URL_RE.sub(lambda m: self.stash('<a href="{0}">{0}</a>'.format(html.escape(m.group(0)))), text)
        text = self.emphasis(text)
        while "\x00" in text:
            text = STASH_RE.sub(lambda m: self.frags[int(m.group(1))], text)
        return text

    def heading(self, level, text):
        plain = re.sub(r"[`*]", "", text)
        hid = self.id + "-" + slug(plain)
        if level == 1 and self.title == self.path.name:
            self.title = plain
            hid = self.id + "-title"
        while hid in self.used:
            hid += "-x"
        self.used.add(hid)
        return '<h%d id="%s">%s</h%d>' % (level, hid, self.inline(text), level)


def is_table_start(lines, i):
    return lines[i].lstrip().startswith("|") and i + 1 < len(lines) and TABLE_SEP_RE.match(lines[i + 1]) is not None


def interrupts_paragraph(lines, i):
    m = LIST_RE.match(lines[i])
    return (FENCE_RE.match(lines[i]) or HEADING_RE.match(lines[i]) or is_table_start(lines, i)
            or m and m.group(2) in ("-", "*", "+", "1.", "1)", "A."))


def split_row(line):
    s = re.sub(r"^\||(?<!\\)\|$", "", line.strip())
    return [c.strip().replace("\\|", "|") for c in re.split(r"(?<!\\)\|", s)]


def render_table(doc, lines, i, out):
    head = split_row(lines[i])
    end = next((k for k in range(i + 2, len(lines)) if not lines[k].lstrip().startswith("|")), len(lines))

    def cells(row, tag):
        if len(row) != len(head):
            doc.warnings.append("%s: a table row has %d cells, the header %d" % (doc.path.name, len(row), len(head)))
        row = (row + [""] * len(head))[:len(head)]
        return "".join('<%s>%s</%s>' % (tag, doc.inline(c), tag) for c in row)

    body = "".join("<tr>%s</tr>\n" % cells(split_row(r), "td") for r in lines[i + 2:end])
    out.append('<div class="table-wrap"><table>\n<thead><tr>%s</tr></thead>\n<tbody>\n%s</tbody></table></div>'
               % (cells(head, "th"), body))
    return end


def new_list(marker, indent):
    if marker in "-*+":
        return {"tag": "ul", "attrs": "", "indent": indent, "items": []}
    start = int(marker[:-1]) if marker[0].isdigit() else ord(marker[0]) - ord("A") + 1
    attrs = (' type="A"' if marker[0].isalpha() else "") + (' start="%d"' % start if start != 1 else "")
    return {"tag": "ol", "attrs": attrs, "indent": indent, "items": []}


def same_kind(lst, marker):
    return lst["tag"] == ("ul" if marker in "-*+" else "ol") and (' type="A"' in lst["attrs"]) == marker[0].isalpha()


def parse_list(lines, i):
    roots, stack = [], []
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            if i < len(lines) and lines[i].strip() and not LIST_RE.match(lines[i]) and not lines[i].startswith(" "):
                break
            continue
        m = LIST_RE.match(line)
        if not m:
            if stack and not line.startswith(" ") and interrupts_paragraph(lines, i):
                break
            indent = len(line) - len(line.lstrip())
            for k in range(len(stack) - 1, -1, -1):
                if stack[k]["items"][-1]["content"] <= indent:
                    del stack[k + 1:]
                    break
            stack[-1]["items"][-1]["text"].append(line.strip())
            i += 1
            continue
        indent, marker = len(m.group(1)), m.group(2)
        item = {"text": [m.group(4)], "content": indent + len(marker) + len(m.group(3)), "children": []}
        while stack and indent < stack[-1]["indent"]:
            stack.pop()
        if stack and indent < stack[-1]["items"][-1]["content"] and not same_kind(stack[-1], marker):
            stack.pop()
        if not stack or indent >= stack[-1]["items"][-1]["content"]:
            lst = new_list(marker, indent)
            (stack[-1]["items"][-1]["children"] if stack else roots).append(lst)
            stack.append(lst)
        stack[-1]["items"].append(item)
        i += 1
    return roots, i


def render_list(doc, lst):
    items = "".join("<li>%s%s</li>\n" % (doc.inline(" ".join(it["text"])),
                                         "".join("\n" + render_list(doc, c) for c in it["children"]))
                    for it in lst["items"])
    return "<%s%s>\n%s</%s>" % (lst["tag"], lst["attrs"], items, lst["tag"])


def render(doc, text):
    lines = text.splitlines()
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        if FENCE_RE.match(line):
            j = next((k for k in range(i + 1, len(lines)) if lines[k].startswith("```")), len(lines))
            out.append("<pre><code>%s</code></pre>" % html.escape("\n".join(lines[i + 1:j])))
            i = j + 1
            continue
        m = HEADING_RE.match(line)
        if m:
            out.append(doc.heading(len(m.group(1)), m.group(2)))
            i += 1
            continue
        if is_table_start(lines, i):
            i = render_table(doc, lines, i, out)
            continue
        if LIST_RE.match(line):
            roots, i = parse_list(lines, i)
            out.extend(render_list(doc, lst) for lst in roots)
            continue
        para = [line.strip()]
        i += 1
        while i < len(lines) and lines[i].strip() and not interrupts_paragraph(lines, i):
            para.append(lines[i].strip())
            i += 1
        out.append("<p>%s</p>" % doc.inline(" ".join(para)))
    return "\n".join(out)


def doc_order(readme):
    order = [readme]
    for m in LINK_RE.finditer(readme.read_text(encoding="utf-8")):
        path = Path(os.path.normpath(ROOT / m.group(2).partition("#")[0]))
        if path.suffix == ".md" and path.parent == DOC_DIR and path.exists() and path not in order:
            order.append(path)
    order += [p for p in sorted(DOC_DIR.glob("*.md")) if p not in order]
    return order


CSS = """
:root { color-scheme: light dark; --bg: #ffffff; --fg: #1f2329; --muted: #5d6672; --border: #d8dde4;
  --nav-bg: #f4f6f9; --code-bg: #eef1f5; --link: #0a58a8; --th-bg: #e9edf2; --current: #dfe7f2; }
@media (prefers-color-scheme: dark) {
  :root { --bg: #14171c; --fg: #e2e5e9; --muted: #98a1ac; --border: #2d333b; --nav-bg: #1a1e24;
    --code-bg: #20252c; --link: #78b4ff; --th-bg: #222830; --current: #263040; }
}
* { box-sizing: border-box; }
html { scroll-behavior: smooth; }
body { margin: 0; background: var(--bg); color: var(--fg);
  font: 15px/1.55 system-ui, -apple-system, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
a { color: var(--link); }
.layout { display: flex; align-items: flex-start; }
nav { position: sticky; top: 0; flex: 0 0 260px; height: 100vh; overflow-y: auto; padding: 20px 14px;
  background: var(--nav-bg); border-right: 1px solid var(--border); }
nav .brand { font-weight: 700; font-size: 17px; margin: 0 6px 12px; }
nav ol { list-style: none; margin: 0; padding: 0; }
nav li a { display: block; padding: 6px; border-radius: 6px; text-decoration: none; color: var(--fg); }
nav li a:hover { background: var(--current); }
nav li span { display: block; color: var(--muted); font-size: 12px; }
main { flex: 1; min-width: 0; max-width: 960px; padding: 24px 36px 64px; }
section.doc { padding-bottom: 28px; margin-bottom: 36px; border-bottom: 1px solid var(--border); }
section.doc:last-child { border-bottom: none; }
.src { color: var(--muted); font-size: 12px; }
h1, h2, h3, h4 { line-height: 1.25; scroll-margin-top: 12px; }
h1 { font-size: 26px; margin: 4px 0 12px; }
h2 { font-size: 20px; margin-top: 28px; padding-bottom: 4px; border-bottom: 1px solid var(--border); }
h3 { font-size: 17px; margin-top: 22px; }
p, li { overflow-wrap: break-word; }
li { margin: 3px 0; }
code { font-family: ui-monospace, SFMono-Regular, Consolas, "Liberation Mono", monospace; font-size: 0.88em;
  background: var(--code-bg); padding: 0.1em 0.3em; border-radius: 4px; overflow-wrap: anywhere; }
pre { background: var(--code-bg); padding: 12px 14px; border-radius: 6px; overflow-x: auto; line-height: 1.45; }
pre code { background: none; padding: 0; font-size: 13px; overflow-wrap: normal; }
.table-wrap { overflow-x: auto; margin: 12px 0; }
table { border-collapse: collapse; font-size: 14px; }
th, td { border: 1px solid var(--border); padding: 6px 10px; text-align: left; vertical-align: top; }
th { background: var(--th-bg); }
td { min-width: 7em; }
@media (max-width: 820px) {
  .layout { display: block; }
  nav { position: static; height: auto; padding: 12px 16px; border-right: none; border-bottom: 1px solid var(--border); }
  main { padding: 16px; }
}
"""


def main():
    order = doc_order(ROOT / "README.md")
    nav, sections, warnings = [], [], []
    for path in order:
        doc = Doc(path, order)
        body = render(doc, path.read_text(encoding="utf-8"))
        rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
        nav.append('<li><a href="#%s">%s<span>%s</span></a></li>' % (doc.id, html.escape(doc.title), rel))
        sections.append('<section class="doc" id="%s">\n<div class="src">%s</div>\n%s\n</section>' % (doc.id, rel, body))
        warnings += doc.warnings
    page = ("<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            "<title>daPCG docs</title>\n<style>%s</style>\n</head>\n<body>\n"
            "<!-- Made by build_html.py from README.md and _doc/*.md: edit those, then run it again. -->\n"
            "<div class=\"layout\">\n<nav>\n<div class=\"brand\">daPCG docs</div>\n<ol>\n%s\n</ol>\n</nav>\n"
            "<main>\n%s\n</main>\n</div>\n</body>\n</html>\n") % (CSS, "\n".join(nav), "\n".join(sections))
    OUT.write_bytes(page.encode("utf-8"))
    for w in warnings:
        print("warning: " + w, file=sys.stderr)
    print("%s: %d docs, %d bytes" % (OUT.relative_to(ROOT).as_posix(), len(order), len(page.encode("utf-8"))))
    return 1 if warnings else 0


if __name__ == "__main__":
    sys.exit(main())
