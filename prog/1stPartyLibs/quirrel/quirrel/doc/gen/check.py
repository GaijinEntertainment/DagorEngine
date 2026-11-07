"""Checks the reference content beyond what a successful build proves.

    python gen/check.py [--coverage] [--sq path]

The build already fails on an unresolved cross-reference, a parameter that the
signature does not have, and a page asking for an example that is not there. This
adds the checks that need to look at the files as a set.
"""

import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import model   # noqa: E402
import render  # noqa: E402

# Output that is not the same everywhere. A float that came out as a special value
# is printed by the C library, so its text varies by compiler: "nan", "-nan(ind)"
# and "-1.#IND" are all the same value.
UNSTABLE = [
    (re.compile(r"(?i)\bnan\b|#IND|#QNAN"), "a nan, whose spelling depends on the C library"),
    (re.compile(r"(?i)(^|[^A-Za-z])inf\b|#INF"), "an infinity, whose spelling depends on the C library"),
]


def check_examples(errors):
    """Every .nut has a .out, every .out has a .nut, and no output is unstable."""
    for dirpath, _dirs, files in os.walk(model.EXAMPLES):
        for name in sorted(files):
            path = os.path.join(dirpath, name)
            stem, ext = os.path.splitext(path)
            if ext == ".nut" and not os.path.exists(stem + ".out"):
                errors.append(f"{path}: no .out; generate it by running the example")
            elif ext == ".out":
                if not os.path.exists(stem + ".nut"):
                    errors.append(f"{path}: no .nut next to it")
                    continue
                with open(path, encoding="utf-8") as f:
                    text = f.read()
                for pattern, why in UNSTABLE:
                    m = pattern.search(text)
                    if m:
                        errors.append(f"{path}: prints {why} ({m.group(0)!r}); "
                                      "describe the case in the Notes section instead")
                if "AN ERROR HAS OCCURRED" in text:
                    errors.append(f"{path}: contains the host error dump, which carries the "
                                  "call stack and the path the example was invoked with. It "
                                  "appears even for a caught throw that crosses a native "
                                  "callback, so rewrite the example not to throw there")
                # A source path in the output is only stable if it is the one CI uses,
                # which is the example's own path relative to quirrel/quirrel.
                expected = os.path.relpath(stem + ".nut", model.ROOT.rsplit(os.sep, 1)[0])
                expected = expected.replace(os.sep, "/")
                for found in set(re.findall(r"[^\s'\"]*\.nut(?=:)", text)):
                    if found != expected:
                        errors.append(f"{path}: names {found!r}, but CI runs this example as "
                                      f"{expected!r}. Regenerate the .out from the quirrel/quirrel "
                                      "directory using the path in README.md")


def check_orphans(site, errors):
    """An example whose symbol the VM does not report is dead content."""
    known = {(s.container.slug, s.name) for s in site.symbols}
    for dirpath, _dirs, files in os.walk(model.EXAMPLES):
        slug = os.path.relpath(dirpath, model.EXAMPLES).replace(os.sep, "/")
        for name in sorted(files):
            if slug.split("/")[0] == "pages":
                continue          # narrative page samples are addressed by path, not by symbol
            if not name.endswith(".nut"):
                continue
            stem = name[:-4]
            if stem.endswith("-basic"):
                stem = stem[:-len("-basic")]   # the small leading example
            if (slug, stem) not in known:
                errors.append(f"examples/{slug}/{name}: no symbol for it in the dump")


LEXER = os.path.join(model.ROOT, "..", "squirrel", "compiler", "lexer.cpp")


def check_keywords(site, errors):
    """The keyword map must match the lexer exactly, and every target must exist.

    ADD_KEYWORD is the only place a keyword is created, so a new one shows up here
    as a missing entry rather than as a keyword nobody can find on the site.
    """
    if not os.path.exists(LEXER):
        errors.append(f"{LEXER}: not found, cannot check the keyword list")
        return
    with open(LEXER, encoding="utf-8", errors="replace") as f:
        source = f.read()
    # skip the #define line itself, whose parameter is also called `key`
    lexer_words = set(re.findall(r"^\s*ADD_KEYWORD\(\s*([A-Za-z_]\w*)\s*,", source, re.M))

    for missing in sorted(lexer_words - set(site.keywords)):
        errors.append(f"content/_keywords.json: the lexer registers {missing!r}, "
                      "but the map has no entry for it")
    for extra in sorted(set(site.keywords) - lexer_words):
        errors.append(f"content/_keywords.json: {extra!r} is not a keyword in lexer.cpp")

    anchors = {p.name: {a for _h, a in p.sections} for p in site.pages}
    for word, target in sorted(site.keywords.items()):
        page_name, _, anchor = target.partition("#")
        if page_name not in site.page_urls:
            errors.append(f"content/_keywords.json: {word!r} points at page {page_name!r}, "
                          "which does not exist")
        elif anchor and anchor not in anchors.get(page_name, set()):
            errors.append(f"content/_keywords.json: {word!r} points at {target!r}, but that "
                          f"page has no section {anchor!r}")


METAMETHODS = os.path.join(model.ROOT, "..", "squirrel", "sqobject.h")


def vm_metamethods(errors):
    """The metamethod names the VM recognizes.

    MM_IMPL is the only place one is declared, so a new metamethod shows up here as
    a missing entry rather than as something nobody can find on the site.
    """
    if not os.path.exists(METAMETHODS):
        errors.append(f"{METAMETHODS}: not found, cannot check the metamethod list")
        return set()
    with open(METAMETHODS, encoding="utf-8", errors="replace") as f:
        return set(re.findall(r'MM_IMPL\(\s*\w+\s*,\s*"(\w+)"', f.read()))


def check_metamethods(site, errors):
    """content/_metamethods.json must name exactly the VM's metamethods, and every
    entry must point at a section that exists."""
    vm = vm_metamethods(errors)
    if not vm:
        return
    for missing in sorted(vm - set(site.metamethods)):
        errors.append(f"content/_metamethods.json: the VM has {missing!r}, "
                      "but the map has no entry for it")
    for extra in sorted(set(site.metamethods) - vm):
        errors.append(f"content/_metamethods.json: {extra!r} is not a metamethod in sqobject.h")

    anchors = {p.name: {a for _h, a in p.sections} for p in site.pages}
    for name, entry in sorted(site.metamethods.items()):
        for field in ("sig", "when", "this", "target"):
            if not entry.get(field):
                errors.append(f"content/_metamethods.json: {name!r} has no {field!r}")
        page_name, _, anchor = entry.get("target", "").partition("#")
        if page_name not in site.page_urls:
            errors.append(f"content/_metamethods.json: {name!r} points at page "
                          f"{page_name!r}, which does not exist")
        elif anchor and anchor not in anchors.get(page_name, set()):
            errors.append(f"content/_metamethods.json: {name!r} points at "
                          f"{entry['target']!r}, but that page has no section {anchor!r}")


def check_operators(site, errors):
    """Every operator target must resolve, and every metamethod must be a real one."""
    known_mm = vm_metamethods(errors)
    anchors = {p.name: {a for _h, a in p.sections} for p in site.pages}
    for group in site.operators:
        for entry in group["operators"]:
            page_name, _, anchor = entry["target"].partition("#")
            where = f"content/_operators.json: {entry['op']!r}"
            if page_name not in site.page_urls:
                errors.append(f"{where} points at page {page_name!r}, which does not exist")
            elif anchor and anchor not in anchors.get(page_name, set()):
                errors.append(f"{where} points at {entry['target']!r}, "
                              f"but that page has no section {anchor!r}")
            mm = entry.get("mm")
            if mm and known_mm and mm not in known_mm:
                errors.append(f"{where} names metamethod {mm!r}, "
                              "which squirrel/sqobject.h does not define")


def check_groups(site, errors):
    """content/_meta.json lists the sidebar groups, and nothing may fall outside it.

    A group is a run of narrative pages or a run of containers, never both: the
    sidebar heads the two kinds differently, so a mixed group would have to guess.
    """
    known = site.groups
    if not known:
        errors.append("content/_meta.json: no groups listed, so the sidebar has no order")
        return
    if len(set(known)) != len(known):
        errors.append(f"content/_meta.json: groups repeats a name: {known}")
    for page in site.pages:
        if page.group not in known:
            errors.append(f"content/pages/{page.name}.md: group {page.group!r} is not in the "
                          "groups list of content/_meta.json, so the sidebar cannot place it")
    for container in site.containers:
        if container.group not in known:
            errors.append(f"content/_meta.json: container {container.id!r} has group "
                          f"{container.group!r}, which the groups list does not have")
    for group in known:
        if site.pages_of(group) and site.containers_of(group):
            errors.append(f"group {group!r} holds both pages and containers; give one of "
                          "them a group of its own")
        if group != render.INTRO_GROUP and not site.pages_of(group) \
                and not site.containers_of(group):
            errors.append(f"content/_meta.json: group {group!r} is listed but empty")


def check_group_overviews(site, errors):
    """Every page group is a chapter a reader can open.

    The sidebar heading links to the page marked `group_index: true`, which must sort
    first in the group and be the only one, and every other page in the group needs
    the `summary:` its row in that overview shows. INTRO_GROUP is the exception: its
    overview is the landing page, content/_index.md, which is not a Page.
    """
    groups = []
    for page in site.pages:
        if page.group not in groups:
            groups.append(page.group)
    for group in groups:
        pages = [p for p in site.pages if p.group == group]
        indexes = [p for p in pages if p.group_index]
        if len(indexes) > 1:
            names = ", ".join(p.name for p in indexes)
            errors.append(f"group {group!r} has more than one group_index page: {names}")
        elif not indexes:
            if group != render.INTRO_GROUP:
                errors.append(
                    f"group {group!r} has no overview: mark its first page "
                    "`group_index: true`, so the sidebar heading can link to it")
            continue
        elif indexes[0] is not pages[0]:
            errors.append(
                f"content/pages/{indexes[0].name}.md: the overview of {group!r} must sort "
                f"first in its group, but {pages[0].name} comes before it")
        for page in pages:
            if not page.group_index and not page.summary:
                errors.append(f"content/pages/{page.name}.md: needs a summary: for its row "
                              f"in the {group} overview")


def check_index_anchors(site, errors):
    """The sidebar sends a container group heading to index.html#<group>, and the
    landing page puts that id on the table for the group. A section of the landing
    page with the same title would take the id first and send the reader to prose."""
    from model import anchor
    sections = {anchor(h) for h, _t in (site.intro.sections if site.intro else ()) if h}
    for group in {c.group for c in site.containers}:
        if anchor(group) in sections:
            errors.append(f"content/_index.md: a section named {group!r} takes the id the "
                          f"sidebar uses for the {group} tables; rename the section")


def check_inlined_anchors(site, errors):
    """A class the module page carries owns an anchor on that page.

    The module's own sections and its note take anchors from the same namespace, so
    a class named `values` or a note heading named after a class would give the page
    two elements with one id, and every link to it would land on whichever came
    first.
    """
    for container in site.containers:
        children = container.inlined_children
        if not children:
            continue
        taken = {"values": "the Values section", "functions": "the Functions section"}
        if container.note is not None:
            for heading, _text in container.note.sections:
                if heading:
                    taken[model.anchor(heading)] = f"the note heading {heading!r}"
        for child in children:
            if child.anchor in taken:
                errors.append(f"{container.id}: the class {child.id} wants the anchor "
                              f"{child.anchor!r}, which {taken[child.anchor]} already has")
            taken[child.anchor] = f"the class {child.id}"


def check_theme(errors):
    """build.py copies these by name, so a rename has to be made in both places.

    A build catches it too, and louder: shutil.copyfile raises on the missing
    source. This names the file for someone running the checks on their own.
    """
    for name in ("style.css", "theme.js", "search.js", "nav.js", "run.js", "run_worker.js", "highlight.js"):
        if not os.path.exists(os.path.join(model.ROOT, "theme", name)):
            errors.append(f"theme/{name} is missing, but build.py copies it into the site")


def check_api(site, errors, sq):
    """The committed API dump must still describe the API the site documents.

    Every signature on every page is read out of gen/api_dump.jsonl, so a native
    binding whose arity, types or attributes changed leaves the pages stale with
    nothing to notice it: an example only goes red when the change alters what it
    prints. This is the guard check_capi gives the C API, for the other dump.

    dump_api.nut sorts every list it emits so its output is byte-stable, which is
    what makes a re-run comparable at all.
    """
    if not os.path.exists(model.API_DUMP):
        errors.append("gen/api_dump.jsonl is missing; regenerate it with the command "
                      "in README.md")
        return

    import subprocess
    try:
        fresh = subprocess.run([sq, os.path.join(HERE, "dump_api.nut")],
                               capture_output=True, text=True, cwd=model.ROOT, timeout=120)
    except (OSError, subprocess.SubprocessError) as e:
        # Every other check still runs without the interpreter. The CI job builds it
        # before this one, so the dump is verified there rather than never.
        print(f"gen/api_dump.jsonl not verified: cannot run {sq} ({e})")
        return
    if fresh.returncode != 0:
        errors.append(f"gen/dump_api.nut failed under {sq}: "
                      f"{fresh.stderr.strip().splitlines()[-1:] or fresh.stdout[-200:]}")
        return

    current = [json.loads(line) for line in fresh.stdout.splitlines() if line.strip()]
    with open(model.API_DUMP, encoding="utf-8") as f:
        committed = [json.loads(line) for line in f if line.strip()]

    # The header records the version of the interpreter that produced the dump, and
    # CI builds that interpreter from source, so a release bump alone would make the
    # files differ. That is not drift: what the pages show is the rows. Nothing else
    # reads the header, and the page banner takes its version from build.py.
    def surface(rows):
        return [r for r in rows if r["row"] != "header"]

    if surface(current) == surface(committed):
        return

    def members(rows):
        return {(r.get("container", ""), r["name"]): r
                for r in rows if r["row"] == "member"}
    have, now = members(committed), members(current)
    detail = []
    added = sorted(now.keys() - have.keys())
    removed = sorted(have.keys() - now.keys())
    changed = sorted(k for k in have.keys() & now.keys() if have[k] != now[k])
    for what, keys in (("added", added), ("removed", removed), ("changed", changed)):
        if keys:
            names = [f"{c}.{n}" if c else n for c, n in keys[:6]]
            more = f" and {len(keys) - 6} more" if len(keys) > 6 else ""
            detail.append(f"{what}: {', '.join(names)}{more}")
    errors.append("gen/api_dump.jsonl is out of date with the VM; regenerate it with "
                  "the command in README.md. " + "; ".join(detail or ["rows differ"]))


SQ_MAIN = os.path.join(model.ROOT, "..", "sq", "sq.cpp")
WASM_RUNNER = os.path.join(model.ROOT, "wasm", "runner.cpp")
_REGISTER = re.compile(r"register([A-Za-z]+)Lib\s*\(\s*\)")


def check_wasm_modules(errors):
    """The browser VM must offer the modules the command line one does.

    A sample runs under sq.cpp in CI and under wasm/runner.cpp on the site, so a
    module registered in one and not the other makes a documented sample fail in
    the browser alone. wasm/check_examples.js would catch it, but only when the VM
    was built and only as a warning, so the comparison belongs here.
    """
    def registered(path):
        if not os.path.exists(path):
            errors.append(f"{path}: not found, cannot compare the module sets")
            return None
        with open(path, encoding="utf-8", errors="replace") as f:
            return set(_REGISTER.findall(f.read()))

    cli, browser = registered(SQ_MAIN), registered(WASM_RUNNER)
    if cli is None or browser is None:
        return
    for name in sorted(cli - browser):
        errors.append(f"doc/wasm/runner.cpp does not register the {name} module, which "
                      "sq/sq.cpp does, so a sample using it fails in the browser only")
    for name in sorted(browser - cli):
        errors.append(f"doc/wasm/runner.cpp registers the {name} module, which sq/sq.cpp "
                      "does not, so a sample using it runs on the site and fails in CI")


def check_capi(site, errors):
    """The committed C API dump must still match the headers, and every description
    must still name a function that exists.

    The dump is committed so the site builds with nothing but python3, which means
    nothing else would notice a function added to a header.
    """
    if not site.capi:
        errors.append("gen/capi_dump.jsonl is missing or empty; run gen/dump_capi.py")
        return

    import subprocess
    fresh = subprocess.run([sys.executable, os.path.join(HERE, "dump_capi.py")],
                           capture_output=True, text=True)
    if fresh.returncode != 0:
        errors.append(f"gen/dump_capi.py failed: {fresh.stderr.strip()}")
        return
    current = [json.loads(line) for line in fresh.stdout.splitlines() if line.strip()]
    if current != site.capi:
        have = {r["name"] for r in site.capi}
        now = {r["name"] for r in current}
        detail = ""
        if now - have:
            detail += f" added: {sorted(now - have)}"
        if have - now:
            detail += f" removed: {sorted(have - now)}"
        errors.append("gen/capi_dump.jsonl is out of date with include/*.h; "
                      f"regenerate it with gen/dump_capi.py.{detail}")

    names = {row["name"] for row in site.capi}
    for extra in sorted(set(site.capi_docs) - names):
        errors.append(f"content/_capi.json describes {extra!r}, which no header declares")

    # A section no page lists is a set of functions with no page and no search entry.
    import render
    shown = render.capi_group_pages(site)
    for group in sorted({row["group"] for row in site.capi} - set(shown)):
        count = sum(1 for row in site.capi if row["group"] == group)
        errors.append(f"the C API section {group!r} ({count} functions) is on no page; "
                      "add a {{capi:...}} for it under content/pages/capi/")


def harness_labels():
    """The interpreter labels bench/benchmarks.py measures now, an empty set when
    the harness cannot be read, or None where bench/ is not checked out at all.

    Read from the harness rather than restated here, so an interpreter cannot be
    current in one place and stale in the other.
    """
    bench = os.path.normpath(os.path.join(model.ROOT, "..", "bench"))
    if not os.path.isdir(bench):
        return None
    sys.path.insert(0, bench)
    try:
        import benchmarks
    except Exception as e:
        print(f"bench labels not checked: cannot read {bench} ({e})")
        return set()
    finally:
        sys.path.pop(0)
    return {lang[0] for lang in benchmarks.ALL_LANGS}


def check_bench(site, errors):
    """The committed numbers must describe the interpreters the harness runs now.

    A renamed row is the failure this catches: run_tests writes by label and -u
    keeps the rows a run does not measure, so an interpreter bumped in
    bench/benchmarks.py and refreshed with -u leaves the old label on the page
    beside the new one, and both render as measured.
    """
    if not site.bench:
        return                  # nobody has run the harness; the page says so
    results = site.bench["results"]
    featured = site.bench.get("featured")
    labels = {label for rows in results.values() for label in rows}

    measured = harness_labels()
    for stale in sorted(labels - measured) if measured is not None else []:
        errors.append(f"content/_bench.json: {stale!r} is not an interpreter that "
                      "bench/benchmarks.py measures now, so the page shows a renamed "
                      "row as measured; rerun the suite without -u")

    if featured not in labels:
        errors.append(f"content/_bench.json: featured is {featured!r}, which no workload "
                      "measured; rerun bench/benchmarks.py")
    for workload, rows in results.items():
        if not rows:
            errors.append(f"content/_bench.json: {workload!r} has no rows")
        for label, value in rows.items():
            if not isinstance(value, (int, float)):
                errors.append(f"content/_bench.json: {workload!r}/{label!r} is {value!r}, "
                              "not a time; the interpreter failed to run")


def report_coverage(site):
    print(f"{'container':22} {'documented':>10} {'example':>8} {'total':>6}")
    for c in site.containers:
        members = c.members
        if not members:
            continue
        documented = sum(1 for m in members if m.documented)
        examples = sum(1 for m in members if m.example)
        print(f"{c.title:22} {documented:>10} {examples:>8} {len(members):>6}")
    print(f"{'TOTAL':22} {sum(1 for s in site.symbols if s.documented):>10} "
          f"{sum(1 for s in site.symbols if s.example):>8} {len(site.symbols):>6}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--coverage", action="store_true", help="print per-module page counts")
    parser.add_argument("--sq", default=None,
                        help="the interpreter to verify the dump with (default: $QUIRREL_SQ, else cmake's)")
    args = parser.parse_args()

    site = model.load()
    errors = []
    check_examples(errors)
    check_groups(site, errors)
    check_group_overviews(site, errors)
    check_index_anchors(site, errors)
    check_inlined_anchors(site, errors)
    check_theme(errors)
    check_wasm_modules(errors)
    check_api(site, errors, model.find_sq(args.sq))
    check_bench(site, errors)
    check_orphans(site, errors)
    check_keywords(site, errors)
    check_metamethods(site, errors)
    check_operators(site, errors)
    check_capi(site, errors)

    if args.coverage:
        report_coverage(site)
        described = sum(1 for r in site.capi if r["name"] in site.capi_docs)
        from_header = sum(1 for r in site.capi
                          if r["name"] not in site.capi_docs and r["note"])
        print(f"\nC API: {len(site.capi)} functions, {described} described, "
              f"{from_header} from a header comment, "
              f"{len(site.capi) - described - from_header} undescribed")

    for err in errors:
        print(f"error: {err}", file=sys.stderr)
    if errors:
        raise SystemExit(f"{len(errors)} problems")
    print("checks passed")


if __name__ == "__main__":
    main()
