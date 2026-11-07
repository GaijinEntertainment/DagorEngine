"""Builds the site model: the VM dump plus the authored side-car files.

The dump is the authority on what exists and what its signature is; the side-car
files under content/ only add prose. A symbol with no side-car still gets a page,
marked as undocumented, so a gap is visible on the site instead of silent.
"""

import json
import os
import re

from parse_decl import parse_decl, DeclParseError

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
CONTENT = os.path.join(ROOT, "content")
EXAMPLES = os.path.join(ROOT, "examples")
API_DUMP = os.path.join(HERE, "api_dump.jsonl")
QUIRREL = os.path.dirname(ROOT)

CMAKE_BUILD = os.path.join(QUIRREL, "build")


def _exe(path):
    return path if os.name != "nt" or path.lower().endswith(".exe") else path + ".exe"


def named_sq(explicit=None):
    """The interpreter the caller brought, from --sq or QUIRREL_SQ, or None."""
    named = explicit or os.environ.get("QUIRREL_SQ")
    return os.path.abspath(named) if named else None


def find_sq(explicit=None):
    """The interpreter the dump is taken with: the named one, else the cmake build."""
    named = named_sq(explicit)
    if named:
        return named
    # A multi-config generator puts the configuration in the path.
    candidates = [_exe(os.path.join(CMAKE_BUILD, "bin", *sub, "sq"))
                  for sub in ((), ("Release",), ("RelWithDebInfo",), ("Debug",))]
    return next((c for c in candidates if os.path.isfile(c)), candidates[0])

# Canonical type name -> container id of the type's page. `number` and `any` are
# unions with no single delegate, so they stay unlinked.
TYPE_CONTAINER = {
    "string": "types.String",
    "array": "types.Array",
    "table": "types.Table",
    "int": "types.Integer",
    "float": "types.Float",
    "bool": "types.Bool",
    "function": "types.Function",
    "generator": "types.Generator",
    "thread": "types.Thread",
    "class": "types.Class",
    "instance": "types.Instance",
    "weakref": "types.WeakRef",
    "userdata": "types.UserData",
    "null": "types.Null",
}

TYPE_HINT = {
    "number": "integer or float",
    "any": "any type",
    "userpointer": "opaque pointer from C++",
}


class Symbol:
    def __init__(self, container, row):
        self.container = container
        self.name = row["name"]
        self.kind = row["kind"]
        self.raw = row
        self.doc = row.get("doc") or ""
        self.decl = row.get("decl")
        self.signature = None
        self.body = None            # ContentFile, or None when undocumented
        self.example = None         # Example, or None
        self.basic = None           # the small leading example, or None
        self.names_from_doc = False # true when the page supplied the parameter names

        if self.decl:
            try:
                self.signature = parse_decl(self.decl)
            except DeclParseError as err:
                raise SystemExit(f"cannot parse decl for {self.ref}: {err}")

    @property
    def ref(self):
        """The id an author writes in a cross-reference: `math.clamp`, `print`."""
        return f"{self.container.id}.{self.name}" if self.container.id else self.name

    @property
    def url(self):
        return f"{self.container.slug}/{_file_slug(self.name)}.html"

    @property
    def is_function(self):
        return self.kind == "function"

    @property
    def summary(self):
        if self.doc:
            return self.doc
        if self.body and self.body.lead:
            return self.body.lead
        return ""

    @property
    def documented(self):
        return self.body is not None


class Container:
    def __init__(self, meta, row):
        self.id = row["id"]
        self.kind = row["kind"]
        self.doc = row.get("doc") or ""
        self.slug = meta["slug"]
        self.title = meta.get("title") or (self.id or "globals")
        self.blurb = meta.get("blurb", "")
        self.group = meta.get("group", "")
        self.members = []
        self.note = None        # ContentFile from <slug>/_index.md, or None
        self.parent = None      # the container this one is nested in, or None

    @property
    def url(self):
        return f"{self.slug}/index.html"

    @property
    def inlined(self):
        """True when the parent's page carries this container as a section.

        A class the site lists in the same group as the module it belongs to is part
        of that module: `io.file` is what `io.open` answers, and splitting it off
        would make the group index a list of two kinds of thing. A class the site
        puts in a group of its own, which is how the type delegates are listed,
        keeps its own page.
        """
        return self.parent is not None and self.group == self.parent.group

    @property
    def anchor(self):
        return anchor(self.title)

    @property
    def link_url(self):
        """Where a link to this container goes. Not `url`: an inlined container has
        no page of its own, only a section of the parent's."""
        return f"{self.parent.url}#{self.anchor}" if self.inlined else self.url

    @property
    def own_and_inlined(self):
        """This container's members plus those of the classes it carries."""
        return len(self.members) + sum(len(c.members) for c in self.inlined_children)

    @property
    def inlined_children(self):
        return [c for c in self._children if c.inlined]

    @property
    def functions(self):
        return [m for m in self.members if m.is_function]

    @property
    def values(self):
        """Everything that is not a function and not a nested class: constants, but
        also ready-made objects such as io.stdout."""
        return [m for m in self.members if m.kind not in ("function", "class")]

    @property
    def classes(self):
        return [m for m in self.members if m.kind == "class"]


class ContentFile:
    """An authored side-car: front matter plus markdown sections."""

    def __init__(self, path, front, lead, sections):
        self.path = path
        self.front = front
        self.lead = lead          # first paragraph, used as the summary fallback
        self.sections = sections  # list of (heading, markdown text), heading may be ""

    @property
    def see_also(self):
        return self.front.get("see_also", [])


class Example:
    def __init__(self, ref, source, output):
        self.ref = ref
        self.source = source
        self.output = output


def anchor(heading):
    """The id a `## ` heading gets, and what a link must use to reach it."""
    return re.sub(r"[^a-z0-9]+", "-", heading.lower()).strip("-")


def _file_slug(name):
    # Windows has no case-sensitive file names and URLs should not depend on case
    # either, so `Watched` and `watched` cannot share a directory.
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)


def _parse_front_matter(text, path):
    """Reads the `---` block. Values are scalars or `[a, b]` lists; nothing more is
    needed, so nothing more is supported."""
    if not text.startswith("---"):
        return {}, text
    end = text.find("\n---", 3)
    if end < 0:
        raise SystemExit(f"{path}: unterminated front matter")
    block, rest = text[3:end], text[end + 4:]

    front = {}
    for line in block.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            raise SystemExit(f"{path}: bad front matter line {line!r}")
        key, _, value = line.partition(":")
        value = value.strip()
        if value.startswith("[") and value.endswith("]"):
            items = [v.strip() for v in value[1:-1].split(",")]
            front[key.strip()] = [v for v in items if v]
        else:
            front[key.strip()] = value
    return front, rest.lstrip("\n")


def _split_sections(text):
    sections = []
    heading, buf = "", []
    for line in text.splitlines():
        if line.startswith("## "):
            sections.append((heading, "\n".join(buf).strip()))
            heading, buf = line[3:].strip(), []
        else:
            buf.append(line)
    sections.append((heading, "\n".join(buf).strip()))
    return [(h, b) for h, b in sections if h or b]


def load_json(path):
    """The contents of an optional side-car, or None when it is not there."""
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def load_content(path):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    front, body = _parse_front_matter(text, path)
    sections = _split_sections(body)
    lead = ""
    for heading, block in sections:
        if not heading and block:
            lead = block.split("\n\n")[0].replace("\n", " ")
            break
    return ContentFile(path, front, lead, sections)


def load_page_example(rel):
    """Loads examples/pages/<rel>.nut for a narrative page.

    A page needs several samples, so they are addressed by path rather than by
    symbol. They live under examples/ like every other sample, which is what gets
    them run by the test runner.
    """
    base = os.path.join(EXAMPLES, "pages", *rel.split("/"))
    nut, out = base + ".nut", base + ".out"
    if not os.path.exists(nut):
        return None
    if not os.path.exists(out):
        raise SystemExit(f"{nut}: missing the .out file; generate it by running the example")
    with open(nut, encoding="utf-8") as f:
        source = f.read().rstrip("\n")
    with open(out, encoding="utf-8") as f:
        output = f.read().replace("\r\n", "\n").rstrip("\n")
    return Example(rel, source, output)


def load_example(symbol, suffix=""):
    """Loads a symbol's example.

    `suffix` picks the optional companion: "-basic" is the small example shown
    before the reference sections, so a reader sees what the thing is for before
    every way it can behave.
    """
    # Examples sit under the container slug, the same layout as content/, so a
    # root-table symbol is examples/globals/print.nut and not examples/print.nut.
    base = os.path.join(EXAMPLES, *symbol.container.slug.split("/"),
                        _file_slug(symbol.name) + suffix)
    nut, out = base + ".nut", base + ".out"
    if not os.path.exists(nut):
        return None
    if not os.path.exists(out):
        raise SystemExit(f"{nut}: missing the .out file; generate it by running the example")
    with open(nut, encoding="utf-8") as f:
        source = f.read().rstrip("\n")
    with open(out, encoding="utf-8") as f:
        output = f.read().replace("\r\n", "\n").rstrip("\n")
    return Example(symbol.ref, source, output)


def _apply_param_names(symbol):
    """Applies a `params:` front matter list to a signature the VM could not name.

    A binding registered with a type mask instead of a declaration string reports
    `arg1`, `arg2`, ... The page may supply the real names, but only there: where
    the VM does know a name, the VM wins, so a page can never quietly rename a
    parameter out from under the implementation.
    """
    names = symbol.body.front.get("params")
    if not names:
        return
    where = symbol.body.path
    if not symbol.signature:
        raise SystemExit(f"{where}: params: given, but {symbol.ref} has no signature")
    if not symbol.signature.has_placeholder_names:
        raise SystemExit(f"{where}: params: given, but the VM already names the parameters of "
                         f"{symbol.ref}; drop it rather than disagree with the implementation")
    slots = [p for p in symbol.signature.params if not p.vararg]
    if len(names) != len(slots):
        raise SystemExit(f"{where}: params: lists {len(names)} names but {symbol.ref} takes "
                         f"{len(slots)} (the trailing `...` is not named)")
    for slot, name in zip(slots, names):
        slot.name = name
    symbol.names_from_doc = True


class Page:
    """A hand-written page that is not tied to a symbol.

    `name` is the path under content/pages/ without the extension, so a page may
    sit in a subdirectory and keep that shape in the url.
    """

    def __init__(self, name, content):
        self.name = name
        self.content = content
        self.title = content.front.get("title", name)
        self.group = content.front.get("group", "Guides")
        self.order = int(content.front.get("order", 100))
        # One line for the row this page gets in its group's overview table. It is
        # authored rather than cut from the lead, because a lead that opens with a
        # `##` heading has no first paragraph to cut.
        self.summary = content.front.get("summary", "")
        # The overview of its whole group: the sidebar puts it on the group heading
        # instead of listing it as one more page under it.
        self.group_index = content.front.get("group_index", "") == "true"
        # Lets a page ask for a layout of its own, which only the cheat sheet needs.
        self.main_class = content.front.get("layout", "")

    @property
    def url(self):
        return f"{self.name}.html"

    @property
    def depth(self):
        return self.name.count("/")

    @property
    def sections(self):
        """(heading, anchor) for every `## ` heading, in order."""
        return [(h, anchor(h)) for h, _ in self.content.sections if h]


class Site:
    def __init__(self, containers, symbols, meta, pages, keywords=None, operators=None,
                 intro=None, capi=(), capi_docs=None, metamethods=None, bench=None):
        self.containers = containers
        self.symbols = symbols
        self.meta = meta
        self.pages = pages
        # The sidebar order, from content/_meta.json. A page's own `order` only sorts
        # it inside its group, so adding a group cannot renumber another one.
        self.groups = list(meta.get("groups", []))
        self.intro = intro      # ContentFile from content/_index.md, or None
        # Stamped by build.py from the interpreter, so the header names the version
        # the pages actually describe. Empty when the interpreter could not be run.
        self.version = ""
        # The C API is a separate axis: it comes from the headers rather than from a
        # running VM, so it has its own dump and its own prose side-car.
        self.capi = list(capi)
        self.capi_docs = capi_docs or {}
        # Measured elsewhere and committed, so the page can state numbers this build
        # cannot produce. None when nobody has run the harness.
        self.bench = bench
        self.by_ref = {s.ref: s for s in symbols}
        self.xrefs = self._build_xrefs()

        # Bare names that are unambiguous anywhere on the site: module names and
        # the root-table globals. Everything else only auto-links inside its own
        # container; see Renderer.local.
        self.page_urls = {p.name: p.url for p in pages}
        self.group_indexes = {p.group: p for p in pages if p.group_index}
        # The landing page is not a Page, so `page:index` has to be wired by hand;
        # a chapter overview links back to it like it links to any other page.
        self.page_urls["index"] = "index.html"

        self.page_anchors = {p.name: {a for _h, a in p.sections} for p in pages}
        self.page_anchors["index"] = {anchor(h) for h, _t in (intro.sections if intro else ())
                                     if h}
        self.keywords = keywords
        self.operators = operators
        self.metamethods = metamethods or {}
        self.ambient_names = {c.id for c in containers if c.id}
        self.ambient_names |= {c.id.split(".")[-1] for c in containers if c.id}
        self.ambient_names |= {s.name for s in symbols if not s.container.id}

    def pages_of(self, group):
        return [p for p in self.pages if p.group == group]

    def containers_of(self, group):
        return [c for c in self.containers if c.group == group and not c.inlined]

    def group_index(self, group):
        """The overview page of a page group, or None when the group has none."""
        return self.group_indexes.get(group)

    def local_names(self, container=None):
        names = set(self.ambient_names)
        if container is not None:
            names |= {m.name for m in container.members}
        return names

    def _build_xrefs(self):
        """Reference key -> url.

        Ambiguous short names are dropped rather than resolved arbitrarily: linking
        a bare `len` to whichever type happened to be loaded first would be worse
        than not linking it.
        """
        xrefs = {}
        exact = set()
        clashes = set()

        def add(key, url, is_exact=False):
            """A full id is exact; a shortened one is an alias.

            An alias never takes a key an exact id holds, which is what makes
            `system` the module and `system.system` the function in it."""
            if key in clashes or (key in exact and not is_exact):
                return
            if is_exact:
                if key in exact and xrefs[key] != url:
                    del xrefs[key]
                    exact.discard(key)
                    clashes.add(key)
                    return
                exact.add(key)
                xrefs[key] = url
                return
            if key in xrefs and xrefs[key] != url:
                del xrefs[key]
                clashes.add(key)
                return
            xrefs[key] = url

        for c in self.containers:
            if c.id:
                add(c.id, c.link_url, is_exact=True)
        for s in self.symbols:
            add(s.ref, s.url, is_exact=True)
        for c in self.containers:
            if c.id:
                add(c.id.split(".")[-1], c.link_url)
        for s in self.symbols:
            if s.container.id:
                add(f"{s.container.id.split('.')[-1]}.{s.name}", s.url)
        for s in self.symbols:
            add(s.name, s.url)
        return xrefs

    def type_url(self, type_name):
        container_id = TYPE_CONTAINER.get(type_name)
        if not container_id:
            return None
        for c in self.containers:
            if c.id == container_id:
                return c.link_url
        return None


def load(meta_path=None):
    meta_path = meta_path or os.path.join(CONTENT, "_meta.json")
    with open(meta_path, encoding="utf-8") as f:
        meta = json.load(f)

    wanted = {c["id"]: c for c in meta["containers"]}

    rows = []
    with open(API_DUMP, encoding="utf-8") as f:
        for line in f:
            if line.strip():
                rows.append(json.loads(line))

    containers, symbols = [], []
    current = None
    for row in rows:
        if row["row"] == "container":
            current = Container(wanted[row["id"]], row) if row["id"] in wanted else None
            if current:
                containers.append(current)
        elif row["row"] == "member" and current is not None:
            if row["kind"] == "class":
                continue  # classes get their own container row
            symbol = Symbol(current, row)
            current.members.append(symbol)
            symbols.append(symbol)

    missing = wanted.keys() - {c.id for c in containers}
    if missing:
        raise SystemExit(f"_meta.json lists containers that the dump does not have: {sorted(missing)}")

    for c in containers:
        note_path = os.path.join(CONTENT, *c.slug.split("/"), "_index.md")
        if os.path.exists(note_path):
            c.note = load_content(note_path)

    order = {c["id"]: i for i, c in enumerate(meta["containers"])}
    containers.sort(key=lambda c: order[c.id])

    by_id = {c.id: c for c in containers}
    for c in containers:
        if "." in c.id:
            c.parent = by_id.get(c.id.rsplit(".", 1)[0])
    for c in containers:
        c._children = [k for k in containers if k.parent is c]

    for symbol in symbols:
        path = os.path.join(CONTENT, *symbol.container.slug.split("/"), _file_slug(symbol.name) + ".md")
        if os.path.exists(path):
            symbol.body = load_content(path)
            _apply_param_names(symbol)
        symbol.example = load_example(symbol)
        symbol.basic = load_example(symbol, '-basic')

    pages_dir = os.path.join(CONTENT, "pages")
    pages = []
    if os.path.isdir(pages_dir):
        for dirpath, _dirs, files in os.walk(pages_dir):
            for name in sorted(files):
                if not name.endswith(".md"):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, pages_dir)[:-3].replace(os.sep, "/")
                pages.append(Page(rel, load_content(path)))
    pages.sort(key=lambda p: (p.order, p.title))

    keywords = {}
    kw_path = os.path.join(CONTENT, "_keywords.json")
    if os.path.exists(kw_path):
        with open(kw_path, encoding="utf-8") as f:
            keywords = json.load(f)["keywords"]

    operators = []
    op_path = os.path.join(CONTENT, "_operators.json")
    if os.path.exists(op_path):
        with open(op_path, encoding="utf-8") as f:
            operators = json.load(f)["groups"]

    metamethods = {}
    mm_path = os.path.join(CONTENT, "_metamethods.json")
    if os.path.exists(mm_path):
        with open(mm_path, encoding="utf-8") as f:
            metamethods = json.load(f)["metamethods"]

    intro = None
    intro_path = os.path.join(CONTENT, "_index.md")
    if os.path.exists(intro_path):
        intro = load_content(intro_path)

    capi = []
    capi_path = os.path.join(HERE, "capi_dump.jsonl")
    if os.path.exists(capi_path):
        with open(capi_path, encoding="utf-8") as f:
            capi = [json.loads(line) for line in f if line.strip()]

    capi_docs = {}
    capi_doc_path = os.path.join(CONTENT, "_capi.json")
    if os.path.exists(capi_doc_path):
        with open(capi_doc_path, encoding="utf-8") as f:
            capi_docs = json.load(f)["functions"]

    # The benchmark side-car is written by bench/, on Windows, where the
    # prebuilt interpreters are. A missing one only costs its charts, so the site
    # builds on a machine that has never run a benchmark.
    bench = load_json(os.path.join(CONTENT, "_bench.json"))

    return Site(containers, symbols, meta, pages, keywords, operators, intro, capi, capi_docs,
                metamethods, bench)
