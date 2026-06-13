#!/usr/bin/python3
"""jam -dx -n -a dump -> ninja build file (stdout).

Models the C++ edit/rebuild loop: compile (+ MSVC PCH), static-lib, link and the
post-link manifest embed (mt). Everything else -- codegen, asm (nasm/ml64/ispc),
resources and the jamvar/das stubs jam omits from the dump -- stays with jam, so a
jam build of THAT config must run first to produce those objects; this file then
drives fast source edits. A Jamfile/flag change needs a regenerate.

Header dependencies are left to ninja (deps = msvc | gcc), so the manifest never
goes stale on #include edits. ninja must be seeded once (one full ninja run) to
fill its deps log before incremental header changes are tracked.

Run from the same dir jam runs in (e.g. .../prog), feeding it the raw dump:
    jam <flags> -dx -n -a | gen_ninja.py [--builddir DIR] > build.ninja
    ninja -f build.ninja           # seed once, then run on every source edit

Unrecognised tools (codegen, shaders, resources, ...) are reported as skipped on
stderr, not modelled: jam owns them.
"""

import argparse
import os
import re
import sys

# Sets use the normalized basename (see tool_name): lowercased, no ".exe", no
# "-<version>" suffix, so clang, clang.exe and clang-17/clang-20 all match "clang".
MSVC_CC = {"clang-cl", "cl"}                     # -Fo<obj>, /showIncludes deps
GCC_CC = {"gcc", "g++", "clang", "clang++",      # -o <obj>, -MMD/-MF depfile deps
          "orbis-clang", "orbis-clang++", "prospero-clang"}
MSVC_LD = {"lld-link", "link"}                   # /lib -> static lib, else link
MSVC_AR = {"lib"}                                # -out:<lib>, .obj members, @rsp
MAC_AR = {"libtool"}                             # Apple -static -o<lib>, .o via -filelist
GNU_LD = {"ld", "ld.lld", "orbis-ld", "prospero-ld"}    # raw GNU-style linker (-o out)
GNU_AR = {"ar", "llvm-ar", "orbis-snarl",        # ar-style: <op> <lib> <objs>
          "orbis-llvm-ar", "prospero-llvm-ar"}
OBJ_EXT = (".obj", ".o")
LIB_EXT = (".lib", ".a")
SRC_EXT = (".cpp", ".cxx", ".cc", ".c", ".m", ".mm", ".s", ".asm")


def norm(path, base):
    return os.path.normcase(os.path.normpath(os.path.join(base, path)))


def clean(line):                                 # drop jam -dx wrapping markers
    return line.replace(r"#\(", "").replace(r")\#", "")


def tool_name(path):                             # normalize compiler/linker basename
    t = os.path.basename(path).lower()
    if t.endswith(".exe"):
        t = t[:-4]
    return re.sub(r"-\d+$", "", t)               # drop version suffix: clang-17 -> clang


DEP_NOARG = {"-M", "-MM", "-MD", "-MMD", "-MG", "-MP"}
DEP_ARG = ("-MF", "-MT", "-MQ")                  # take a following path/target token


def strip_dep_flags(cmd):
    """Drop the compiler's own dep-gen flags so we can add a single private one.
    jam emits -MMD -MF x.d; a second -MD/-MF trips clang -Werror
    (unused-command-line-argument) and confuses which depfile ninja reads."""
    out, skip = [], False
    for p in cmd.split():
        if skip:
            skip = False
        elif p in DEP_NOARG or (p[:3] in DEP_ARG and len(p) > 3):
            pass
        elif p in DEP_ARG:
            skip = True
        else:
            out.append(p)
    return " ".join(out)


def derive_builddir(outs):
    """Put ninja state inside the per-config output dir, so each platform/config
    is isolated and gets cleaned together with its outputs. The dir is the common
    ancestor of obj+lib outputs (= jam's '<_output>[/<project>]/<config>' root,
    layout-agnostic). Links are excluded: they live outside the output tree."""
    outs = sorted(outs)
    if len(outs) < 2:                            # nothing meaningful to share
        return ".ninja"
    try:                                         # commonpath of >=2 distinct paths is a dir
        return os.path.join(os.path.commonpath(outs), ".ninja")
    except ValueError:                           # mixed drives
        return ".ninja"


def esc(p):                                      # ninja path (build line)
    return p.replace("$", "$$").replace(" ", "$ ").replace(":", "$:")


def escv(v):                                     # ninja value (command / args)
    return v.replace("$", "$$")


def find_src(toks, base):
    for t in reversed(toks):
        if t.lower().endswith(SRC_EXT):
            return norm(t, base)
    return norm(toks[-1], base)


def compile_obj(toks, mode, base):
    if mode == "msvc":
        t = next((x for x in toks if x.startswith("-Fo")), None)
        return norm(t[3:], base) if t else None
    for i, x in enumerate(toks):                 # gcc: -o <obj>
        if x == "-o" and i + 1 < len(toks):
            return norm(toks[i + 1], base)
    return None


def pch_info(toks, base):                        # MSVC PCH: /Yc make, /Yu use, /Fp<pch>
    role = path = None
    for t in toks:
        tl = t[:3].lower()
        if tl in ("/yc", "-yc"):
            role = "produce"
        elif tl in ("/yu", "-yu"):
            role = "use"
        elif tl in ("/fp", "-fp"):
            path = norm(t[3:], base)
    return (role, path) if (role and path) else (None, None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump", nargs="?", help="jam -dx dump file (default: stdin)")
    ap.add_argument("--builddir", default=None,
                    help="dir for .ninja_log/.ninja_deps "
                         "(default: <config output dir>/.ninja, auto-derived)")
    a = ap.parse_args()
    base = os.path.realpath(os.curdir)           # = the jam build/run dir
    src_in = open(a.dump, encoding="utf-8", errors="replace") if a.dump else sys.stdin

    compiles, archives, links = [], [], []
    seen_obj, seen_lib, seen_exe, skipped = set(), set(), set(), {}
    pch_outputs, ranlib_libs = set(), set()
    manifests = {}                               # exe -> (manifest, resid, mt) for mt embed
    missing_src = 0

    for raw in src_in:
        s = raw.strip()
        head = s.split(None, 2)
        if len(head) < 2 or head[0] not in ("call", "call_filtered"):
            continue
        toks = [t for t in clean(s).split() if t != "@@@"]
        tool = tool_name(toks[1])
        cmd = " ".join(toks[1:])

        if tool in MSVC_CC or (tool in GCC_CC and "-c" in toks):
            mode = "msvc" if tool in MSVC_CC else "gcc"
            obj = compile_obj(toks, mode, base)
            if not obj or obj in seen_obj:
                continue
            src = find_src(toks, base)
            if not os.path.exists(src):       # generated source (codegen) not on disk: leave obj frozen
                missing_src += 1
                continue
            seen_obj.add(obj)
            role, pch = pch_info(toks, base)
            if role == "produce":
                pch_outputs.add(pch)
            compiles.append((obj, src, cmd, mode, role, pch))
        elif tool in GNU_AR:                     # ar <op> <archive> <objs...>
            out = next((norm(t, base) for t in toks if t.lower().endswith(LIB_EXT)), None)
            if not out or out in seen_lib:
                continue
            seen_lib.add(out)
            members = [norm(t, base) for t in toks if t.lower().endswith(OBJ_EXT)]
            if tool == "orbis-snarl":            # snarl: key+archive on argv; objs via $in_newline
                archives.append(("ar_snarl", out, members, toks[1], ""))
            else:
                archives.append(("ar_gnu", out, members, toks[1], " ".join(toks[2:])))
        elif tool in MSVC_AR:                    # lib.exe: -out:<lib>, .obj members (@rsp, no /lib)
            outt = next((t for t in toks if t[:5].lower() == "-out:"), None)
            if not outt or norm(outt[5:], base) in seen_lib:
                continue
            out = norm(outt[5:], base)
            seen_lib.add(out)
            members = [norm(t, base) for t in toks if t.lower().endswith(OBJ_EXT)]
            archives.append(("ar_gnu", out, members, toks[1], " ".join(toks[2:])))
        elif tool in MAC_AR:                     # libtool -static -o <lib> <objs> (macOS/iOS)
            i = toks.index("-o") if "-o" in toks else -1
            if i < 0 or i + 1 >= len(toks) or norm(toks[i + 1], base) in seen_lib:
                if i < 0:
                    skipped[tool] = skipped.get(tool, 0) + 1
                continue
            out = norm(toks[i + 1], base)
            seen_lib.add(out)
            members = [norm(t, base) for t in toks if t.lower().endswith(OBJ_EXT)]
            flags = [t for j, t in enumerate(toks[2:], 2)   # keep jam flags (-static ...);
                     if j not in (i, i + 1) and t != "-"    # drop jam's positional '-' (stdin)
                     and not t.lower().endswith(OBJ_EXT + LIB_EXT)]
            archives.append(("ar_libtool", out, members, toks[1], " ".join(flags)))
        elif tool in GCC_CC or tool in GNU_LD:   # link (driver or raw ld): -o <out>, .o/.a inputs
            i = toks.index("-o") if "-o" in toks else -1
            if i < 0 or i + 1 >= len(toks):
                skipped[tool] = skipped.get(tool, 0) + 1
                continue
            out = norm(toks[i + 1], base)
            if out in seen_exe:
                continue
            seen_exe.add(out)
            ins = [norm(t, base) for t in toks if t.lower().endswith(OBJ_EXT + LIB_EXT)]
            links.append((out, toks[1], " ".join(toks[2:]), ins))
        elif tool in MSVC_LD:                    # lld-link: /lib -> archive, else link
            outt = next((t for t in toks if t[:5].lower() == "-out:"), None)
            if not outt:
                continue
            out = norm(outt[5:], base)
            if "/lib" in toks:
                if out in seen_lib:
                    continue
                seen_lib.add(out)
                i = toks.index("/lib")           # must stay first arg on cmdline
                members = [norm(t, base) for t in toks if t.lower().endswith(OBJ_EXT)]
                archives.append(("ar", out, members, toks[1], " ".join(toks[2:i] + toks[i + 1:])))
            else:
                if out in seen_exe:
                    continue
                seen_exe.add(out)
                ins = [norm(t, base) for t in toks if t.lower().endswith(OBJ_EXT + LIB_EXT)]
                links.append((out, toks[1], " ".join(toks[2:]), ins))
        elif tool == "ranlib" and len(toks) >= 3:  # linux: index archive after `ar -qc`
            ranlib_libs.add(norm(toks[2], base))
        elif tool == "mt":                       # mt -manifest <f> -outputresource:"<exe>;#<id>"
            man = next((toks[j + 1] for j, t in enumerate(toks)
                        if t == "-manifest" and j + 1 < len(toks)), None)
            res = next((t for t in toks if t.lower().startswith("-outputresource:")), None)
            if man and res:
                exe, _, rid = res[len("-outputresource:"):].strip('"').partition(";#")
                manifests[norm(exe, base)] = (norm(man, base), rid or "1", toks[1])
        else:
            skipped[tool] = skipped.get(tool, 0) + 1

    known = seen_obj | seen_lib
    builddir = a.builddir or derive_builddir(known)
    w = sys.stdout.write

    w("builddir = %s\n\n" % builddir.replace("\\", "/"))
    w("rule cc_msvc\n  command = $c\n  deps = msvc\n  description = cc $out\n\n")
    w("rule cc_gcc\n  command = $c\n  deps = gcc\n  depfile = $out.nd\n"
      "  description = cc $out\n\n")
    w("rule cc_asm\n  command = $c\n  description = cc $out\n\n")  # asm: no header deps
    rsp = "  rspfile = $out.rsp\n  rspfile_content = $args\n"
    w("rule ar\n  command = $prog /lib @$out.rsp\n" + rsp + "  description = ar $out\n\n")
    w("rule ar_gnu\n  command = $prog @$out.rsp\n" + rsp + "  description = ar $out\n\n")
    # linux `ar -qc` writes no index and appends on reuse: recreate fresh, then ranlib
    w("rule ar_gnu_ranlib\n  command = rm -f $out && $prog @$out.rsp && ranlib $out\n"
      + rsp + "  description = ar $out\n\n")
    w("rule ar_snarl\n  command = $prog rc $out @$out.rsp\n"   # snarl rsp = newline-separated objs
      "  rspfile = $out.rsp\n  rspfile_content = $in_newline\n  description = ar $out\n\n")
    # Apple libtool reads @rsp (quote-aware, unlike -filelist which takes paths
    # literally and chokes on ninja's shell-quoting of '~' in the config dir).
    w("rule ar_libtool\n  command = $prog $args -o $out @$out.rsp\n"
      "  rspfile = $out.rsp\n  rspfile_content = $in_newline\n  description = ar $out\n\n")
    w("rule ld\n  command = $prog @$out.rsp\n" + rsp + "  description = ld $out\n\n")
    # link, then embed the app manifest in place, as jam's separate mt step does.
    # cmd /c: ninja has no shell on Windows to run the '&&'.
    w("rule ld_mt\n  command = cmd /c \"$prog @$out.rsp && $mt -nologo "
      "-manifest $manifest -outputresource:$out;#$resid\"\n"
      + rsp + "  description = ld $out\n\n")

    for obj, src, cmd, mode, role, pch in compiles:
        if src.lower().endswith((".s", ".asm")):  # assembler ignores dep flags
            rule = "cc_asm"
        elif mode == "msvc":
            if "showincludes" not in cmd.lower():
                cmd += " -showIncludes"
            rule = "cc_msvc"
        else:                                    # private depfile; ninja reads+deletes it
            cmd = strip_dep_flags(cmd) + " -MMD -MF %s.nd" % obj
            rule = "cc_gcc"
        # MSVC PCH: producer emits the .pch too; users depend on it (order + rebuild)
        if role == "produce" and pch:
            out = "%s %s" % (esc(obj), esc(pch))
            w("build %s: %s %s\n  c = %s\n" % (out, rule, esc(src), escv(cmd)))
        elif role == "use" and pch in pch_outputs:
            w("build %s: %s %s | %s\n  c = %s\n" % (esc(obj), rule, esc(src), esc(pch), escv(cmd)))
        else:
            w("build %s: %s %s\n  c = %s\n" % (esc(obj), rule, esc(src), escv(cmd)))

    for rule, lib, members, prog, args in archives:
        if rule == "ar_gnu" and lib in ranlib_libs:   # linux ar -> needs ranlib + fresh recreate
            rule = "ar_gnu_ranlib"
        # all members are inputs, incl. non-modeled asm/codegen objs jam produced:
        # keeps snarl's $in_newline rsp complete, and re-archives if any member changes
        ins = " ".join(esc(o) for o in members)
        w("build %s: %s %s\n  prog = %s\n  args = %s\n" % (esc(lib), rule, ins, prog, escv(args)))

    for exe, prog, args, ins in links:
        deps = " ".join(esc(i) for i in ins if i in known)   # only what we build
        man = manifests.get(exe)
        if man:                                  # embed manifest in place (mt), manifest is a dep
            mpath, rid, mt = man
            w("build %s: ld_mt %s | %s\n  prog = %s\n  args = %s\n"
              "  mt = %s\n  manifest = %s\n  resid = %s\n"
              % (esc(exe), deps, esc(mpath), prog, escv(args), mt, escv(mpath), rid))
        else:
            w("build %s: ld %s\n  prog = %s\n  args = %s\n" % (esc(exe), deps, prog, escv(args)))

    sys.stderr.write("compiles=%d archives=%d links=%d missing_src=%d builddir=%s\n"
                     % (len(compiles), len(archives), len(links), missing_src, builddir))
    if skipped:
        sys.stderr.write("skipped tools: %s\n"
                         % ", ".join("%s x%d" % kv for kv in sorted(skipped.items())))


if __name__ == "__main__":
    main()
