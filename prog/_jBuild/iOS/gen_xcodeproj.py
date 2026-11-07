#!/usr/bin/python3
"""jam -dx -n -a dump (iOS) -> Xcode project for debugging and code navigation.

The project does not compile anything itself:
  - target "jam" (legacy, external build system) runs the jam build script;
  - target "index" (never built) gives Xcode the per-file flags for indexing;
  - scheme "<name>" builds with jam and runs/profiles the .app that jam made.
Swift sources are listed in the index target and use Xcode's Swift defaults:
jam passes swiftc only -O and -g, and the SDK and OS version come from the
target settings.

Run from the dir jam runs in, feeding it the raw dump of the iOS build:
    jam <flags> -dx -n -a | gen_xcodeproj.py --name NAME --build-script SCRIPT
The project settings (name, build script, jam flags) belong to a per-project
wrapper, see skyquake/prog/ios_gen_xcodeproj.sh. A Jamfile or flag change
needs a regenerate. Generated sources (codegen) are not on disk before
the first jam build and are left out until the next regenerate.
"""

import argparse
import hashlib
import os
import re
import shlex
import sys
from xml.sax.saxutils import quoteattr

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from gen_ninja import GCC_CC, SRC_EXT, clean, find_src, norm, tool_name  # noqa: E402

LANG_BY_EXT = {".c": "c", ".m": "objc", ".cpp": "cpp", ".cc": "cpp", ".cxx": "cpp", ".mm": "objcpp"}
FILE_TYPE = {".swift": "sourcecode.swift", ".c": "sourcecode.c.c", ".m": "sourcecode.c.objc", ".cpp": "sourcecode.cpp.cpp",
             ".cc": "sourcecode.cpp.cpp", ".cxx": "sourcecode.cpp.cpp",
             ".mm": "sourcecode.cpp.objcpp", ".h": "sourcecode.c.h", ".hpp": "sourcecode.cpp.h",
             ".hh": "sourcecode.cpp.h", ".hxx": "sourcecode.cpp.h", ".inl": "sourcecode.c.h",
             ".inc": "sourcecode.c.h", ".pch": "sourcecode.c.h"}
HEADER_EXT = (".h", ".hpp", ".hh", ".hxx", ".inl", ".inc", ".pch")

# Xcode sets these itself from build settings
DROP_WITH_ARG = {"-o", "-MF", "-MT", "-MQ", "-isysroot", "-arch", "-target", "-x"}
DROP_NOARG = {"-c", "-M", "-MM", "-MD", "-MMD", "-MG", "-MP"}
DROP_PREFIX = ("-g", "-O", "-mios-version-min=", "-miphoneos-version-min=",
               "-mios-simulator-version-min=", "--target=")
PAIR_FLAGS = {"-isystem", "-iquote", "-idirafter", "-include", "-imacros", "-F", "-iframework",
              "-Xclang", "-mllvm", "-iprefix", "-iwithprefix"}
PATH_FLAGS = ("-isystem", "-iquote", "-idirafter", "-F", "-iframework")


def split_cmd(s):
    try:
        return shlex.split(s, posix=False)       # keep quotes: the dump is shell text
    except ValueError:
        return s.split()


def flag_units(toks, src, base):
    """Compile flags as strings; a flag and its argument form one unit."""
    units, i = [], 0
    while i < len(toks):
        t = toks[i]
        i += 1
        if t in DROP_WITH_ARG:
            i += 1
        elif t in DROP_NOARG or t.startswith(DROP_PREFIX) or t == src or t.endswith(SRC_EXT):
            continue
        elif t in ("-I", "-D", "-U") and i < len(toks):
            arg = toks[i]
            i += 1
            units.append(t + (norm(arg, base) if t == "-I" else arg))
        elif t in PAIR_FLAGS and i < len(toks):
            arg = toks[i]
            i += 1
            if t in PATH_FLAGS:
                arg = norm(arg, base)
            units.append(t + " " + arg)
        elif t.startswith("-I") and len(t) > 2:
            units.append("-I" + norm(t[2:], base))
        else:
            units.append(t)
    return [u for u in units if "$(" not in u]


def swift_compile(args, base):
    """Source of a swiftc -c line and its sdk/min version; None for the -parse header step."""
    src = next((t for t in args if t.endswith(".swift")), None)
    if "-c" not in args or not src:
        return None
    sdk = next((args[i + 1] for i, t in enumerate(args[:-1]) if t == "-sdk"), "")
    tgt = next((args[i + 1] for i, t in enumerate(args[:-1]) if t == "-target"), "")
    m = re.search(r"ios(\d+(?:\.\d+)*)", tgt)
    return norm(src, base), "Simulator" in sdk, m.group(1) if m else None


def parse_dump(src_in, base):
    compiles, link_out, sdk, minver, arch = [], None, "iphoneos", "16.0", "arm64"
    seen, missing, swift, skipped, objs = set(), 0, [], {}, []
    for raw in src_in:
        s = raw.strip()
        head = s.split(None, 2)
        if len(head) < 2 or head[0] not in ("call", "call_filtered"):
            continue
        toks = [t for t in split_cmd(clean(s)) if t != "@@@"]
        tool = tool_name(toks[1])
        if tool == "swiftc":
            sw = swift_compile(toks[2:], base)
            if sw and sw[0] not in swift and os.path.exists(sw[0]):
                swift.append(sw[0])
                sdk = "iphonesimulator" if sw[1] else sdk
                minver = sw[2] or minver
            continue
        if tool not in GCC_CC:
            skipped[tool] = skipped.get(tool, 0) + 1
            continue
        args = toks[2:]
        if "-c" not in args:
            if "-o" in args and args.index("-o") + 1 < len(args):
                out = norm(args[args.index("-o") + 1], base)
                if not out.endswith((".a", ".o", ".dylib")) and (link_out is None or ".app/" in out):
                    link_out = out
            continue
        src = find_src(args, base)
        ext = os.path.splitext(src)[1].lower()
        if ext not in LANG_BY_EXT or src in seen:
            continue
        if not os.path.exists(src):
            missing += 1
            continue
        seen.add(src)
        if "-o" in args and args.index("-o") + 1 < len(args):
            objs.append(norm(args[args.index("-o") + 1], base))
        for j, t in enumerate(args[:-1]):
            if t == "-isysroot" and "Simulator" in args[j + 1]:
                sdk = "iphonesimulator"
            elif t == "-arch":
                arch = args[j + 1]
        for t in args:
            m = re.match(r"-m(?:ios|iphoneos)(?:-simulator)?-version-min=([\d.]+)$", t)
            if m:
                minver = m.group(1)
        compiles.append((src, LANG_BY_EXT[ext], flag_units(args, src, base)))
    return compiles, link_out, sdk, minver, arch, missing, swift, skipped, objs


def common_units(group):
    """Units that every compile in the group has; std= stays per file."""
    if not group:
        return []
    others = [set(c[2]) for c in group[1:]]
    return [u for u in group[0][2]
            if not u.startswith("-std=") and all(u in o for o in others)]


def oid(kind, key):
    return hashlib.md5(("%s:%s" % (kind, key)).encode()).hexdigest()[:24].upper()


def pq(s):
    if s and re.match(r"^[A-Za-z0-9_./]+$", s):
        return s
    return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def ser(v, ind=0):
    pad, pad2 = "\t" * ind, "\t" * (ind + 1)
    if isinstance(v, dict):
        keys = (["isa"] if "isa" in v else []) + sorted(k for k in v if k != "isa")
        return "{\n" + "".join("%s%s = %s;\n" % (pad2, pq(k), ser(v[k], ind + 1)) for k in keys) + pad + "}"
    if isinstance(v, list):
        return "(\n" + "".join("%s%s,\n" % (pad2, ser(x, ind + 1)) for x in v) + pad + ")"
    return pq(str(v))


class Project:
    def __init__(self):
        self.objs = {}

    def add(self, kind, key, **kw):
        i = oid(kind, key)
        d = {"isa": kind}
        d.update(kw)
        self.objs[i] = d
        return i

    def text(self, root):
        body = "".join("\t\t%s = %s;\n" % (i, ser(self.objs[i], 2)) for i in sorted(self.objs))
        return ("// !$*UTF8*$!\n{\n\tarchiveVersion = 1;\n\tclasses = {\n\t};\n\tobjectVersion = 56;\n"
                "\tobjects = {\n%s\t};\n\trootObject = %s;\n}\n" % (body, root))


def collect_headers(src_dirs, inc_dirs, root):
    """Headers next to the sources, and every header under the -I roots (engine
    roots hold only subdirs, e.g. dagorInclude/memory/dag_memBase.h)."""
    def inside(d):
        return d == root or d.startswith(root + os.sep)

    def is_header(n):
        return n.lower().endswith(HEADER_EXT)

    out = set()
    for d in {d for d in src_dirs if inside(d)}:
        try:
            out.update(os.path.join(d, n) for n in os.listdir(d)
                       if is_header(n) and os.path.isfile(os.path.join(d, n)))
        except OSError:
            pass
    for top in sorted(d for d in set(inc_dirs) if inside(d)):
        for path, subdirs, names in os.walk(top):
            subdirs[:] = [n for n in subdirs if not n.startswith(".") and n != "CVS"]
            out.update(os.path.join(path, n) for n in names if is_header(n))
    return sorted(out)


def ref_type(path):
    return FILE_TYPE.get(os.path.splitext(path)[1].lower(), "text")


def make_groups(P, files, root):
    tree, external = {}, []
    for p in files:
        rel = os.path.relpath(p, root)
        if rel.startswith(".."):
            external.append(p)
            continue
        node = tree
        parts = rel.split(os.sep)
        for d in parts[:-1]:
            node = node.setdefault(d, {})
        node[parts[-1]] = p
    refs = {}

    def file_ref(p, rel_to_group):
        if p not in refs:
            refs[p] = P.add("PBXFileReference", p, fileEncoding=4, lastKnownFileType=ref_type(p),
                            path=os.path.basename(p) if rel_to_group else p,
                            sourceTree="<group>" if rel_to_group else "<absolute>")
        return refs[p]

    def order(node):
        return sorted(node.items(), key=lambda kv: (not isinstance(kv[1], dict), kv[0]))

    def children(node, key):
        return [build(v, key + "/" + k, k) if isinstance(v, dict) else file_ref(v, True)
                for k, v in order(node)]

    def build(node, key, name):
        return P.add("PBXGroup", key, children=children(node, key), path=name, sourceTree="<group>")

    top = P.add("PBXGroup", "root:" + root, children=children(tree, "root"),
                name=os.path.basename(root), path=root, sourceTree="<absolute>")
    ext = None
    if external:
        ext = P.add("PBXGroup", "external", children=[file_ref(p, False) for p in external],
                    name="external", sourceTree="<group>")
    return top, ext, refs


def cfg_list(P, key, settings):
    cfg = P.add("XCBuildConfiguration", key + "Debug", name="Debug", buildSettings=settings)
    return P.add("XCConfigurationList", key, buildConfigurations=[cfg],
                 defaultConfigurationIsVisible=0, defaultConfigurationName="Debug")


def scheme_xml(proj, jam_id, idx_id, app):
    def ref(i, n):
        return ('<BuildableReference BuildableIdentifier="primary" BlueprintIdentifier="%s" '
                'BuildableName=%s BlueprintName=%s ReferencedContainer=%s />'
                % (i, quoteattr(n), quoteattr(n), quoteattr("container:" + proj)))
    runnable = ('<PathRunnable runnableDebuggingMode="0" FilePath=%s></PathRunnable>' % quoteattr(app)
                if app else "")
    return """<?xml version="1.0" encoding="UTF-8"?>
<Scheme LastUpgradeVersion="2600" version="1.7">
  <BuildAction parallelizeBuildables="NO" buildImplicitDependencies="YES">
    <BuildActionEntries>
      <BuildActionEntry buildForTesting="NO" buildForRunning="YES" buildForProfiling="YES" buildForArchiving="NO" buildForAnalyzing="NO">
        {jam}
      </BuildActionEntry>
      <BuildActionEntry buildForTesting="NO" buildForRunning="NO" buildForProfiling="NO" buildForArchiving="NO" buildForAnalyzing="NO">
        {idx}
      </BuildActionEntry>
    </BuildActionEntries>
  </BuildAction>
  <TestAction buildConfiguration="Debug" selectedDebuggerIdentifier="Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier="Xcode.DebuggerFoundation.Launcher.LLDB" shouldUseLaunchSchemeArgsEnv="YES">
    <Testables></Testables>
  </TestAction>
  <LaunchAction buildConfiguration="Debug" selectedDebuggerIdentifier="Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier="Xcode.DebuggerFoundation.Launcher.LLDB" launchStyle="0" useCustomWorkingDirectory="NO" ignoresPersistentStateOnLaunch="NO" debugDocumentVersioning="YES" debugServiceExtension="internal" allowLocationSimulation="YES">
    {run}
  </LaunchAction>
  <ProfileAction buildConfiguration="Debug" shouldUseLaunchSchemeArgsEnv="YES" savedToolIdentifier="" useCustomWorkingDirectory="NO" debugDocumentVersioning="YES">
    {run}
  </ProfileAction>
  <AnalyzeAction buildConfiguration="Debug"></AnalyzeAction>
  <ArchiveAction buildConfiguration="Debug" revealArchiveInOrganizer="YES"></ArchiveAction>
</Scheme>
""".format(jam=ref(jam_id, "jam"), idx=ref(idx_id, "index"), run=runnable)


ENV_KEEP = ("PATH", "_DEVTOOL", "GDEVTOOL")  # jam reads the devtools dir from env


def build_script(jam_dir, build_cmd, jam_out):
    lines = ["#!/bin/bash", "# Called by the jam legacy target; $1 is the Xcode action.",
             "# Xcode skips the shell profile, so the env is saved at generation time."]
    lines += ["export %s=%s" % (k, shlex.quote(os.environ[k])) for k in ENV_KEEP if os.environ.get(k)]
    clean_msg = "jam: clean is not mapped, delete %s by hand" % (jam_out or "the jam output of this build")
    lines += ['if [ "$1" = clean ]; then echo %s; exit 0; fi' % shlex.quote(clean_msg),
              'cd %s && exec /bin/bash %s' % (shlex.quote(jam_dir), " ".join(shlex.quote(c) for c in build_cmd)), ""]
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump", nargs="?", help="jam -dx dump file (default: stdin)")
    ap.add_argument("--out", default=None, help="output dir (default: <root>/_output/xcode_<lowercase name>)")
    ap.add_argument("--name", required=True, help="project and scheme name")
    ap.add_argument("--build-script", required=True,
                    help="script run by the jam target, relative to the jam dir")
    ap.add_argument("--build-arg", action="append", default=[],
                    help="argument for the build script, repeatable; use --build-arg=-sFoo=bar")
    ap.add_argument("--app", default=None, help="path of the .app to run (default: from the link line)")
    a = ap.parse_args()

    base = os.path.realpath(os.curdir)
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.realpath(os.path.join(here, "..", "..", ".."))
    out_dir = os.path.realpath(a.out or os.path.join(root, "_output", "xcode_" + a.name.lower()))
    src_in = open(a.dump, encoding="utf-8", errors="replace") if a.dump else sys.stdin

    compiles, link_out, sdk, minver, arch, missing, swift, skipped, objs = parse_dump(src_in, base)
    if not compiles:
        sys.exit("no compile commands found: feed the output of 'jam ... -dx -n -a'")

    app = a.app
    if not app and link_out:
        m = re.match(r"^(.*?\.app)(/|$)", link_out)
        app = m.group(1) if m else None
    if not app:
        sys.stderr.write("warning: no .app in the link line, scheme has no executable (use --app)\n")

    by_lang = {"c": [], "cpp": []}
    for c in compiles:
        by_lang["cpp" if c[1] in ("cpp", "objcpp") else "c"].append(c)
    common = {k: common_units(v) for k, v in by_lang.items()}

    P = Project()
    all_files = [c[0] for c in compiles] + swift
    inc = [u[2:] for _, _, us in compiles for u in us if u.startswith("-I")]
    all_files += collect_headers([os.path.dirname(c[0]) for c in compiles], inc, root)
    all_files = sorted(set(all_files))
    top, ext, refs = make_groups(P, all_files, root)

    build_files = []
    for src, lang, units in compiles:
        grp = "cpp" if lang in ("cpp", "objcpp") else "c"
        skip = set(common[grp])
        flags = " ".join(u for u in units if u not in skip)
        kw = {"settings": {"COMPILER_FLAGS": flags}} if flags else {}
        build_files.append(P.add("PBXBuildFile", src, fileRef=refs[src], **kw))
    build_files += [P.add("PBXBuildFile", src, fileRef=refs[src]) for src in swift]
    phase = P.add("PBXSourcesBuildPhase", "sources", buildActionMask=2147483647, files=build_files,
                  runOnlyForDeploymentPostprocessing=0)

    if swift:
        idx_extra = {"SWIFT_VERSION": "5.0", "SWIFT_OPTIMIZATION_LEVEL": "-Onone",
                     "SWIFT_INSTALL_OBJC_HEADER": "NO", "DEFINES_MODULE": "NO"}
    else:
        idx_extra = {}
    prod = P.add("PBXFileReference", "product", explicitFileType="archive.ar", includeInIndex=0,
                 path="lib%sIndex.a" % a.name, sourceTree="BUILT_PRODUCTS_DIR")
    prod_grp = P.add("PBXGroup", "products", children=[prod], name="Products", sourceTree="<group>")
    main_grp = P.add("PBXGroup", "main", children=[top] + ([ext] if ext else []) + [prod_grp],
                     sourceTree="<group>")

    idx_settings = {"PRODUCT_NAME": a.name + "Index", "SDKROOT": sdk, "SUPPORTED_PLATFORMS": sdk,
                    "ARCHS": arch, "ONLY_ACTIVE_ARCH": "YES", "IPHONEOS_DEPLOYMENT_TARGET": minver,
                    "CLANG_ENABLE_MODULES": "NO", "CODE_SIGNING_ALLOWED": "NO", "SKIP_INSTALL": "YES",
                    "OTHER_CFLAGS": " ".join(common["c"]),
                    "OTHER_CPLUSPLUSFLAGS": " ".join(common["cpp"])}
    idx_settings.update(idx_extra)
    idx = P.add("PBXNativeTarget", "index", buildConfigurationList=cfg_list(P, "cfg:index", idx_settings),
                buildPhases=[phase], buildRules=[], dependencies=[], name="index",
                productName="index", productReference=prod, productType="com.apple.product-type.library.static")
    jam_script = os.path.join(out_dir, "xcode_jam.sh")
    jam = P.add("PBXLegacyTarget", "jam", buildArgumentsString='"%s" "$(ACTION)"' % jam_script,
                buildConfigurationList=cfg_list(P, "cfg:jam", {"PRODUCT_NAME": "jam"}),
                buildPhases=[], buildToolPath="/bin/bash", buildWorkingDirectory=base,
                dependencies=[], name="jam", passBuildSettingsInEnvironment=0, productName="jam")
    proj_cfg = cfg_list(P, "cfg:project", {"ALWAYS_SEARCH_USER_PATHS": "NO"})
    proj = P.add("PBXProject", "project", attributes={"BuildIndependentTargetsInParallel": "YES",
                 "LastUpgradeCheck": 2600}, buildConfigurationList=proj_cfg,
                 compatibilityVersion="Xcode 14.0", developmentRegion="en", hasScannedForEncodings=0,
                 knownRegions=["en", "Base"], mainGroup=main_grp, productRefGroup=prod_grp,
                 projectDirPath="", projectRoot="", targets=[jam, idx])

    proj_dir = os.path.join(out_dir, a.name + ".xcodeproj")
    sch_dir = os.path.join(proj_dir, "xcshareddata", "xcschemes")
    os.makedirs(sch_dir, exist_ok=True)
    with open(os.path.join(proj_dir, "project.pbxproj"), "w", encoding="utf-8") as f:
        f.write(P.text(proj))
    with open(os.path.join(sch_dir, a.name + ".xcscheme"), "w", encoding="utf-8") as f:
        f.write(scheme_xml(a.name + ".xcodeproj", jam, idx, app))
    with open(jam_script, "w", encoding="utf-8") as f:
        f.write(build_script(base, [os.path.join(base, a.build_script)] + a.build_arg,
                             os.path.commonpath([os.path.dirname(o) for o in objs]) if objs else None))

    if not (os.environ.get("_DEVTOOL") or os.environ.get("GDEVTOOL")):
        sys.stderr.write("warning: _DEVTOOL and GDEVTOOL are not set, the Xcode build may not find devtools\n")
    sys.stderr.write("sources=%d swift=%d files=%d missing_src=%d app=%s\nwrote %s\n"
                     % (len(compiles), len(swift), len(all_files), missing, app, proj_dir))
    if skipped:
        sys.stderr.write("skipped tools: %s\n" % ", ".join("%s x%d" % kv for kv in sorted(skipped.items())))


if __name__ == "__main__":
    main()
