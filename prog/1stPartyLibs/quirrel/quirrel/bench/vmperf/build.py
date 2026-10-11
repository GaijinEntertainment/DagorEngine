# DEP_DOMAIN prog/tools
"""csq.exe builds of a revision, out of the source tree, in several code layouts.

The build replays the commands of the jam rel build of prog/tools/consoleSq. It
compiles only the objects that include the Quirrel headers, from a copy of the
Quirrel sources of the revision; all other code links from the output of jam.
A build of the sources that jam built has the same .text as jam's csq.exe.

Variants of one revision, in WORK/<label>/<variant>/csq.exe (+ csq.pdb):
  nat     the plain build, as jam makes it
  k<K>    SQVM::Execute aligned to 64 bytes, and K NOP bytes after its prologue
"""
import concurrent.futures as cf
import functools
import hashlib
import io
import os
import re
import shutil
import subprocess
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, *[".."] * 6))
CONSOLE_SQ = os.path.join(REPO, "prog", "tools", "consoleSq")
WORK = os.environ.get("VMPERF_WORK") or os.path.join(REPO, "_output", "quirrel-vmperf")
QUIRREL = "prog/1stPartyLibs/quirrel/quirrel"
SRC_DIRS = [QUIRREL, "prog/gameLibs/quirrel/quirrelHost"]
NOT_COMPILED = [f"{QUIRREL}/{d}" for d in ("bench", "doc", "testData", "sq_llvm_fuzzer")]
LAYOUTS = ["k0", "k16", "k32", "k48"]
VARIANTS = ["nat"] + LAYOUTS
WORKTREE = "wt"


@functools.cache
def jam_commands():
    """The commands of the jam rel build of consoleSq: compiles {obj: argv}, libs {lib: [obj]},
    the csq.exe link argv and the jam output root."""
    proc = subprocess.run("jam -sConfig=rel -n -a -dx", cwd=CONSOLE_SQ, shell=True, capture_output=True,
                          text=True, errors="replace")
    if proc.returncode:
        raise SystemExit(f"jam failed in {CONSOLE_SQ}:\n{proc.stdout[-3000:]}\n{proc.stderr}")
    compiles, libs, link = {}, {}, None
    for line in proc.stdout.splitlines():
        line = line.strip()
        if not line.startswith("call_filtered "):
            continue
        cmd = line[len("call_filtered "):].replace("#\\(", " ").replace(")\\#", " ").replace("@@@", " ").split()
        if cmd[0].endswith("clang-cl.exe"):
            fo = [a for a in cmd if a.startswith("-Fo")]
            if fo:
                compiles[norm(os.path.join(CONSOLE_SQ, fo[0][3:]))] = cmd
        elif cmd[0].endswith("lld-link.exe") and "/lib" in cmd:
            libs[norm(next(a for a in cmd if a.startswith("-out:"))[5:])] = [a for a in cmd if a.endswith(".obj")]
        elif cmd[0].endswith("lld-link.exe") and any(a.startswith("-out:") and a.endswith("csq.exe") for a in cmd):
            link = cmd
    if link is None or not libs:
        raise SystemExit(f"no jam rel commands for csq.exe; check that jam runs in {CONSOLE_SQ}")
    first = next(iter(libs))
    i = first.index("/_output/") + len("/_output/")
    return compiles, libs, link, first[:i] + first[i:].split("/")[0]


def norm(path):
    return os.path.normpath(path).replace("\\", "/").lower()


@functools.cache
def llvm_bin():
    """The LLVM bin directory of the toolchain that jam uses."""
    compiles, _, _, _ = jam_commands()
    return os.path.dirname(next(iter(compiles.values()))[0])


@functools.cache
def label_of(rev):
    if rev == WORKTREE:
        return WORKTREE
    try:
        return git("rev-parse", "--short=12", rev + "^{commit}").strip()
    except subprocess.CalledProcessError:
        raise SystemExit(f"{rev} is neither a git revision, nor `{WORKTREE}`, nor a file") from None


def git(*args):
    return subprocess.run(["git", "-C", REPO] + list(args), capture_output=True, text=True, check=True).stdout


def exe_path(label, variant):
    return os.path.join(WORK, label, variant, "csq.exe")


def sqvm_source(label):
    return os.path.join(WORK, label, "sqvm.cpp")


PATHSPEC = ["--"] + SRC_DIRS + [f":(exclude){d}" for d in NOT_COMPILED]


def worktree_files():
    files = git("ls-files", "-co", "--exclude-standard", *PATHSPEC).splitlines()
    return [rel for rel in files if os.path.isfile(os.path.join(REPO, rel))]


def stamp_of(rev):
    """What a build of rev is made from: the commit, or a digest of the working tree files."""
    if rev != WORKTREE:
        return label_of(rev)
    digest = hashlib.sha1()
    for rel in worktree_files():
        digest.update(rel.encode())
        with open(os.path.join(REPO, rel), "rb") as f:
            digest.update(f.read())
    return "worktree " + digest.hexdigest()


def jam_inputs_stamp(jam):
    """A digest of the files that the link takes from the jam output, so that a build is made
    again when jam rebuilt any of them."""
    compiles, libs, cmd, _ = jam
    files = []
    for a in cmd[1:]:
        if norm(a) in libs:
            members = libs[norm(a)]
            if any(uses_quirrel(compiles.get(norm(o))) for o in members):
                files += [o for o in members if not uses_quirrel(compiles.get(norm(o)))]
            else:
                files.append(a)
        elif a.endswith(".obj") and not uses_quirrel(compiles.get(norm(a))):
            files.append(a)
    digest = hashlib.sha1()
    for f in files:
        try:
            st = os.stat(os.path.join(CONSOLE_SQ, f))
        except FileNotFoundError:
            raise SystemExit(f"{f} is missing; run jam -sConfig=rel in {CONSOLE_SQ}") from None
        digest.update(f"{f} {st.st_size} {st.st_mtime_ns}\n".encode())
    return digest.hexdigest()


def mirror(rev, dst):
    if rev == WORKTREE:
        for rel in worktree_files():
            os.makedirs(os.path.dirname(os.path.join(dst, rel)), exist_ok=True)
            shutil.copy2(os.path.join(REPO, rel), os.path.join(dst, rel))
    else:
        data = subprocess.run(["git", "-C", REPO, "archive", rev] + PATHSPEC, capture_output=True, check=True).stdout
        with tarfile.open(fileobj=io.BytesIO(data)) as tar:
            tar.extractall(dst, filter="data")


def compile_cmd(cmd, src_root, obj, source=None):
    res = [cmd[0], f"/clang:-ffile-prefix-map={src_root}/=../../../"]  # same __FILE__ strings as jam's build
    for a in cmd[1:]:
        if a == "-showIncludes":
            continue
        if a.startswith("-Fo"):
            a = "-Fo" + obj
        elif a.startswith("-Fd"):
            a = "-Fd" + os.path.join(os.path.dirname(obj), "objs.pdb").replace("\\", "/")
        else:
            for d in SRC_DIRS:
                a = a.replace("../../../" + d, f"{src_root}/{d}")
        res.append(a)
    if source:
        res[-1] = source
    return res


def uses_quirrel(cmd):
    return cmd is not None and any(d in a for a in cmd for d in SRC_DIRS)


def run_all(jobs):
    def run(job):
        return job[0], subprocess.run(job[1], cwd=CONSOLE_SQ, capture_output=True, text=True)

    with cf.ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as ex:
        failed = [(name, r.stdout + r.stderr) for name, r in ex.map(run, jobs) if r.returncode]
    if failed:
        raise SystemExit("\n".join(f"FAILED {name}\n{out}" for name, out in failed))


def layout_source(text, k, file_name):
    """sqvm.cpp with Execute aligned to 64 and K NOP bytes at the start of its body; the
    #line directives keep the file name and line numbers of the original."""
    m = re.search(r"^template <bool debughookPresent>\r?\n(bool SQVM::Execute\(.*\)\r?\n\{\r?\n)", text, re.M)
    if not m:
        raise SystemExit("the definition of SQVM::Execute in sqvm.cpp is not where build.py expects it")
    line = text.count("\n", 0, m.end()) + 1
    pad = f'__asm__ volatile(".skip {k}, 0x90");\n' if k else ""
    return (f'#line 1 "{file_name}"\n' + text[:m.start(1)] + "__attribute__((aligned(64))) " + m.group(1) + pad
            + f'#line {line} "{file_name}"\n' + text[m.end():])


def link(jam, objs, out_dir):
    """Link csq.exe into out_dir, objs {jam obj: obj to use instead}."""
    _, libs, cmd, out_root = jam
    os.makedirs(out_dir, exist_ok=True)
    lld = cmd[0]
    inputs = dict(objs)
    for lib, members in libs.items():
        if any(norm(o) in objs for o in members):
            out_lib = os.path.join(out_dir, "lib", lib[len(out_root) + 1:])
            os.makedirs(os.path.dirname(out_lib), exist_ok=True)
            with open(out_lib + ".rsp", "w") as f:
                f.write("\n".join(objs.get(norm(o), o) for o in members))
            subprocess.run([lld, "/lib", "/machine:x64", "-nologo", "-out:" + out_lib, "@" + out_lib + ".rsp"],
                           check=True)
            inputs[lib] = out_lib
    argv = []
    for a in cmd[1:]:
        if a.startswith("-out:"):
            a = "-out:" + os.path.join(out_dir, "csq.exe")
        elif a.startswith("-pdb:"):
            argv.append("/pdbaltpath:" + a[5:])  # the same embedded path as jam's build
            a = "-pdb:" + os.path.join(out_dir, "csq.pdb")
        else:
            a = inputs.get(norm(a), a) if a.endswith((".obj", ".lib")) else a
        argv.append(a)
    rsp = os.path.join(out_dir, "link.rsp")
    with open(rsp, "w") as f:
        f.write("\n".join(f'"{a}"' if " " in a else a for a in argv))
    subprocess.run([lld, "@" + rsp], cwd=CONSOLE_SQ, check=True)
    shutil.rmtree(os.path.join(out_dir, "lib"), ignore_errors=True)


def build(rev, rebuild=False):
    """Build all variants of rev (a git revision or WORKTREE) and return its label. A build
    is used again while its sources and the jam output that it links stay the same."""
    label = label_of(rev)
    root = os.path.join(WORK, label)
    jam = jam_commands()
    stamp, stamp_file = f"{stamp_of(rev)}, jam output {jam_inputs_stamp(jam)}", os.path.join(root, "stamp.txt")
    if not rebuild and os.path.exists(stamp_file) and all(os.path.exists(exe_path(label, v)) for v in VARIANTS):
        with open(stamp_file) as f:
            if f.read().strip() == stamp:
                return label
    print(f"building {rev} -> {root}", flush=True)
    compiles, libs, cmd, out_root = jam
    if os.path.exists(root):
        shutil.rmtree(root)
    src_root = os.path.join(root, "src").replace("\\", "/")
    mirror(rev, src_root)

    used = {norm(o) for members in libs.values() for o in members} | {norm(a) for a in cmd if a.endswith(".obj")}
    objs = {o: os.path.join(root, "obj", o[len(out_root) + 1:]).replace("\\", "/")
            for o in sorted(used) if uses_quirrel(compiles.get(o))}
    for obj in objs.values():
        os.makedirs(os.path.dirname(obj), exist_ok=True)
    run_all([(compiles[o][-1], compile_cmd(compiles[o], src_root, obj)) for o, obj in objs.items()])
    link(jam, objs, os.path.join(root, "nat"))

    sqvm = [o for o in objs if compiles[o][-1].endswith("/squirrel/sqvm.cpp")]
    if len(sqvm) != 1:
        raise SystemExit(f"expected one sqvm.cpp object, found {sqvm}")
    sqvm = sqvm[0]
    file_name = compiles[sqvm][-1]
    original = f"{src_root}/{file_name[len('../../../'):]}"
    with open(original, encoding="utf-8", newline="") as f:
        text = f.read()
    shutil.copy(original, sqvm_source(label))  # for the profile report
    jobs = []
    for variant in LAYOUTS:
        out_dir = f"{root}/{variant}".replace("\\", "/")
        os.makedirs(out_dir, exist_ok=True)
        # next to the original, for its quoted includes
        source = original[:-len(".cpp")] + f"_{variant}.cpp"
        with open(source, "w", encoding="utf-8", newline="") as f:
            f.write(layout_source(text, int(variant[1:]), file_name))
        jobs.append((source, compile_cmd(compiles[sqvm], src_root, f"{out_dir}/sqvm.obj", source)))
    run_all(jobs)
    for variant in LAYOUTS:
        link(jam, {**objs, sqvm: f"{root}/{variant}/sqvm.obj".replace("\\", "/")}, os.path.join(root, variant))
    shutil.rmtree(os.path.join(root, "obj"))
    shutil.rmtree(os.path.join(root, "src"))
    with open(stamp_file, "w") as f:
        f.write(stamp + "\n")
    return label
