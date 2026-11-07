#!/usr/bin/env python3
"""Makes report.md and report.html of a stack measurement from a template and the result files.

The template is Markdown with these lines:
  {{table:<name>}}   the table of <results>/<name>.json (measure_builds.py)
  {{luau}}           Quirrel against Lua and Luau: <results>/vm_bench-*.json and <results>/vm_bench.json

Usage: python stacks_report.py <template.md> <results dir> <out dir>
"""

import glob
import html
import json
import os
import re
import sys

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "quirrel", "bench")))
from benchmarks import WORKLOADS  # noqa: E402
from run_vm_bench import BENCHES  # noqa: E402

NOISE_PCT = 3.0

GROUPS = [("documentation suite", [row for row, _ in WORKLOADS]), ("VM suite", BENCHES)]


def pct(a, b):
    return (a / b - 1) * 100


def stack_table(data):
    names = list(data["builds"])
    res = data["results"]
    known = {w for _, ws in GROUPS for w in ws}
    groups = [(g, [w for w in ws if w in res]) for g, ws in GROUPS]
    groups.append(("workloads of --extra-dir", [w for w in res if w not in known]))
    head = ["workload", f"{names[0]} ms"] + names[1:] + ["spread"]
    rows = []
    for title, workloads in groups:
        if not workloads:
            continue
        rows.append(("group", title))
        for w in workloads:
            base = min(res[w][names[0]])
            cells = [w, f"{base:.2f}"]
            for n in names[1:]:
                cells.append(f"{pct(min(res[w][n]), base):+.1f}%")
            spread = max(max(v) / min(v) - 1 for v in res[w].values()) * 100
            cells.append(f"{spread:.0f}%")
            rows.append(("row", cells))
    return head, rows


def luau_table(results_dir):
    labels = [os.path.basename(f)[9:-5] for f in glob.glob(os.path.join(results_dir, "vm_bench-*.json"))]
    labels.sort(key=lambda label: (label.split("-")[0], not label.endswith("-base"), label))
    runs = [("4.44.1", os.path.join(results_dir, "vm_bench.json"))] +[(label, os.path.join(results_dir, f"vm_bench-{label}.json")) for label in labels]
    data = []
    for label, path in runs:
        if os.path.exists(path):
            with open(path, encoding="utf-8") as f:
                data.append((label, json.load(f)["results"]))
    if not data:
        return ["workload"], []
    others = [k for k in next(iter(data[0][1].values())) if k != "quirrel"]
    head = ["workload"] + [f"Quirrel {label} ms" for label, _ in data] + others
    rows = []
    for w in data[0][1]:
        cells = [w]
        for _, res in data:
            q = res.get(w, {}).get("quirrel")
            o2 = res.get(w, {}).get("luau-O2")
            cells.append("-" if q is None else f"{q:.1f} ({q / o2:.2f}x)" if o2 else f"{q:.1f}")
        ref = data[0][1][w]
        for o in others:
            v = ref.get(o)
            q = ref.get("quirrel")
            cells.append("-" if v is None else f"{v:.1f} ({q / v:.2f}x)" if q else f"{v:.1f}")
        rows.append(("row", cells))
    return head, rows


def md_table(head, rows):
    out = ["| " + " | ".join(head) + " |", "|" + "|".join(" --- " if i == 0 else " ---: " for i in range(len(head))) + "|"]
    for kind, v in rows:
        if kind == "group":
            out.append(f"| **{v}** |" + " |" * (len(head) - 1))
        else:
            out.append("| " + " | ".join(v) + " |")
    return "\n".join(out)


def cell_class(text):
    m = re.fullmatch(r"([+-][0-9.]+)%", text)
    if not m:
        return "num"
    v = float(m.group(1))
    return "num faster" if v <= -NOISE_PCT else "num slower" if v >= NOISE_PCT else "num"


def html_table(head, rows):
    out = ["<div class=\"scroll\"><table><thead><tr>" + "".join(f"<th>{html.escape(h)}</th>" for h in head) + "</tr></thead><tbody>"]
    for kind, v in rows:
        if kind == "group":
            out.append(f"<tr class=\"group\"><td colspan=\"{len(head)}\">{html.escape(v)}</td></tr>")
        else:
            out.append("<tr>" + "".join(f"<td class=\"{'name' if i == 0 else cell_class(c)}\">{html.escape(c)}</td>"
                                        for i, c in enumerate(v)) + "</tr>")
    return "\n".join(out) + "</tbody></table></div>"


def inline(text):
    text = html.escape(text)
    text = re.sub(r"`([^`]+)`", r"<code>\1</code>", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", text)
    return re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', text)


def md_to_html(text):
    out, para, items, table = [], [], [], []

    def flush():
        if para:
            out.append("<p>" + inline(" ".join(para)) + "</p>")
            para.clear()
        if items:
            out.append("<ul>" + "".join(f"<li>{inline(i)}</li>" for i in items) + "</ul>")
            items.clear()
        if table:
            cells = [[c.strip() for c in line.strip().strip("|").split("|")] for line in table if not re.match(r"^\|[ :|-]+\|$", line.strip())]
            body = "".join("<tr>" + "".join(f"<td>{inline(c)}</td>" for c in r) + "</tr>" for r in cells[1:])
            out.append("<div class=\"scroll\"><table><thead><tr>" + "".join(f"<th>{inline(c)}</th>" for c in cells[0])
                       + f"</tr></thead><tbody>{body}</tbody></table></div>")
            table.clear()

    for line in text.splitlines():
        if m := re.match(r"^(#{1,3}) (.*)", line):
            flush()
            n = len(m.group(1))
            out.append(f"<h{n}>{inline(m.group(2))}</h{n}>")
        elif line.startswith("|"):
            if para or items:
                flush()
            table.append(line)
        elif line.startswith("- "):
            if para or table:
                flush()
            items.append(line[2:])
        elif line.startswith("  ") and items:
            items[-1] += " " + line.strip()
        elif not line.strip():
            flush()
        else:
            if table:
                flush()
            para.append(line.strip())
    flush()
    return "\n".join(out)


CSS = """
:root { --bg: #ffffff; --fg: #1d1f23; --line: #d9dde3; --head: #f3f5f8;
        --good: #1f7a3a; --bad: #b3261e; --code: #eef1f5; }
@media (prefers-color-scheme: dark) { :root {
        --bg: #16181c; --fg: #e4e6ea; --line: #2e333b; --head: #1f2329;
        --good: #6fd08c; --bad: #ff8a80; --code: #23272e; } }
body { background: var(--bg); color: var(--fg); font: 15px/1.5 system-ui, sans-serif; margin: 0 auto;
       max-width: 1100px; padding: 24px 16px; }
h1 { font-size: 1.6rem; } h2 { font-size: 1.25rem; margin-top: 2rem; border-bottom: 1px solid var(--line); }
h3 { font-size: 1.05rem; } code { background: var(--code); padding: 0 3px; border-radius: 3px; }
.scroll { overflow-x: auto; margin: 0.5rem 0 1rem; }
table { border-collapse: collapse; font-size: 13px; }
th, td { border-bottom: 1px solid var(--line); padding: 3px 8px; text-align: left; white-space: nowrap; }
th { background: var(--head); } td.num { text-align: right; font-family: ui-monospace, monospace; }
td.faster { color: var(--good); font-weight: 600; } td.slower { color: var(--bad); font-weight: 600; }
tr.group td { background: var(--head); font-weight: 600; }
"""


def main():
    template, results_dir, out_dir = sys.argv[1:4]
    with open(template, encoding="utf-8") as f:
        src = f.read()
    md_parts, html_parts = [], []
    for block in re.split(r"^(\{\{[^}]+\}\})$", src, flags=re.M):
        m = re.fullmatch(r"\{\{(table:(\w+)|luau)\}\}", block)
        if not m:
            md_parts.append(block)
            html_parts.append(md_to_html(block))
            continue
        if m.group(2):
            with open(os.path.join(results_dir, m.group(2) + ".json"), encoding="utf-8") as f:
                head, rows = stack_table(json.load(f))
        else:
            head, rows = luau_table(results_dir)
        md_parts.append(md_table(head, rows))
        html_parts.append(html_table(head, rows))
    title = re.search(r"^# (.*)$", src, re.M).group(1)
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "report.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(md_parts))
    with open(os.path.join(out_dir, "report.html"), "w", encoding="utf-8", newline="\n") as f:
        f.write(f"<!doctype html>\n<html lang=\"en\"><head><meta charset=\"utf-8\">"
                f"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                f"<title>{html.escape(title)}</title><style>{CSS}</style></head><body>\n"
                + "\n".join(html_parts) + "\n</body></html>\n")


if __name__ == "__main__":
    main()
