#!/usr/bin/env python3
"""Fetch a binary CocoaPods pod into the build cache (see iOS/cocoapods.jam).

Downloads the archive referenced by the pod's podspec 'source' into
<cache>/<pod>/<version>/src and populates <cache>/<pod>/<version>/<slice>
with symlinks to the *.framework / *.bundle / *.a entries matching the
requested platform slice (xcframework slices are resolved via Info.plist).
Writes a .jam-ready stamp on success; jam targets depend on that stamp.

Safe to run concurrently for different slices of the same pod: the src
tree is extracted into a temp dir and published with an atomic rename.
"""

import argparse
import fcntl
import hashlib
import io
import json
import os
import plistlib
import posixpath
import random
import re
import shutil
import ssl
import stat
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
import zipfile

SPEC_URL = "https://trunk.cocoapods.org/api/v1/pods/{pod}/specs/{version}"
USER_AGENT = "dagor-jam-cocoapods/1.0"
RETRIES = 4
# wall-clock budget for RETRYING one pod: the first attempt may legitimately
# be slow (multi-hundred-MB archives), but retry loops must not compound into
# an unbounded stall of the synchronous jam action
RETRY_DEADLINE = 400
_START = time.monotonic()


def out_of_retry_budget():
    return time.monotonic() - _START > RETRY_DEADLINE


def redact_url(url):
    """Distribution links may carry signed tokens in the query/userinfo;
    they must not leak into build logs."""
    parts = urllib.parse.urlsplit(url)
    netloc = parts.hostname or ""
    if parts.port:
        netloc = "%s:%d" % (netloc, parts.port)
    redacted = urllib.parse.urlunsplit((parts.scheme, netloc, parts.path, "", ""))
    if parts.query:
        redacted += "?<redacted>"
    return redacted
RETRIABLE_HTTP = (429, 500, 502, 503, 504)


def log(msg):
    print("fetch_cocoapod: %s" % msg, flush=True)


def fail(msg):
    log("ERROR: %s" % msg)
    sys.exit(1)


def _ssl_contexts():
    """Default context first; python.org builds on macOS often ship without
    CA certs, so fall back to the system pem / certifi."""
    yield None
    try:
        import certifi
        yield ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        pass
    for pem in ("/etc/ssl/cert.pem", "/etc/ssl/certs/ca-certificates.crt"):
        if os.path.exists(pem):
            yield ssl.create_default_context(cafile=pem)


def _curl_get(url, dest_path):
    curl = shutil.which("curl")
    if not curl:
        return False
    # abort on stalls (<1KB/s for 60s) rather than a tight total limit:
    # some SDK archives are hundreds of MBs and legitimately slow
    cmd = [curl, "-fsSL", "--retry", "2",
           "--connect-timeout", "30",
           "--speed-limit", "1024", "--speed-time", "60",
           "--max-time", "1800",
           "-o", dest_path, url]
    try:
        return subprocess.run(cmd, timeout=1900).returncode == 0
    except subprocess.TimeoutExpired:
        log("curl timed out downloading %s" % redact_url(url))
        return False


def _http_get_once(url, dest_path=None):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    last_err = None
    for ctx in _ssl_contexts():
        try:
            with urllib.request.urlopen(req, timeout=300, context=ctx) as resp:
                expected = resp.headers.get("Content-Length")
                if dest_path is None:
                    data = resp.read()
                    got = len(data)
                else:
                    with open(dest_path, "wb") as f:
                        shutil.copyfileobj(resp, f, length=1 << 20)
                    got = os.path.getsize(dest_path)
                # CDNs sometimes drop the connection mid-stream without an
                # error; a truncated archive must not reach the extractors
                if expected is not None and got != int(expected):
                    raise urllib.error.URLError(
                        "truncated download: got %d of %s bytes" % (got, expected))
                return data if dest_path is None else None
        except urllib.error.URLError as e:
            last_err = e
            if not isinstance(getattr(e, "reason", None), ssl.SSLError):
                raise
    # cert store is broken for this python; curl uses the OS trust store
    log("urllib failed (%s), retrying with curl" % last_err)
    tmp = dest_path or (tempfile.mktemp(prefix="fetch_cocoapod."))
    if _curl_get(url, tmp):
        if dest_path is None:
            with open(tmp, "rb") as f:
                data = f.read()
            os.remove(tmp)
            return data
        return None
    raise last_err


def http_get(url, dest_path=None):
    """GET with retries: parallel jam actions easily trip rate limits (429)."""
    for attempt in range(RETRIES):
        try:
            return _http_get_once(url, dest_path)
        except urllib.error.HTTPError as e:
            if e.code not in RETRIABLE_HTTP or attempt == RETRIES - 1:
                raise
            if out_of_retry_budget():
                log("retry budget exhausted for %s" % redact_url(url))
                raise
        except urllib.error.URLError:
            if attempt == RETRIES - 1:
                raise
            if out_of_retry_budget():
                log("retry budget exhausted for %s" % redact_url(url))
                raise
        delay = 2 ** attempt + random.uniform(0, 2)
        log("retrying %s in %.1fs" % (redact_url(url), delay))
        time.sleep(delay)


def get_podspec(pod, version):
    # the CDN is made for mass automated access, the trunk API rate-limits it
    shard = hashlib.md5(pod.encode()).hexdigest()[:3]
    urls = [
        "https://cdn.cocoapods.org/Specs/%s/%s/%s/%s/%s/%s.podspec.json"
        % (shard[0], shard[1], shard[2], pod, version, pod),
        SPEC_URL.format(pod=pod, version=version),
    ]
    last_err = None
    for url in urls:
        try:
            return json.loads(http_get(url))
        except Exception as e:
            last_err = "%s: %s" % (url, e)
            log("podspec fetch failed, %s" % last_err)
    fail("cannot get podspec %s/%s: %s" % (pod, version, last_err))


def archive_url_from_source(src, version):
    """Return (url, kind) for the podspec 'source' dict."""
    if not isinstance(src, dict):
        return None
    if "http" in src:
        return src["http"]
    if "git" in src:
        git = src["git"]
        ref = src.get("commit") or src.get("tag") or version
        m = re.search(r"github\.com[:/](.+?)(?:\.git)?/*$", git)
        if m:
            return "https://codeload.github.com/%s/zip/%s" % (m.group(1), ref)
        m = re.search(r"bitbucket\.org[:/](.+?)(?:\.git)?/*$", git)
        if m:
            return "https://bitbucket.org/%s/get/%s.zip" % (m.group(1), ref)
        fail("unsupported git host in podspec source: %s" % git)
    return None


def _entry_escapes(name, link_target=None):
    """True for archive entries that would write outside the extraction dir
    (zip-slip/tar-slip) or for symlinks pointing out of it."""
    parts = [p for p in name.replace("\\", "/").split("/") if p not in ("", ".")]
    if name.startswith("/") or ".." in parts:
        return True
    if link_target is not None:
        if link_target.startswith("/"):
            return True
        resolved = posixpath.normpath(posixpath.join(posixpath.dirname(name), link_target))
        if resolved == ".." or resolved.startswith("../"):
            return True
    return False


def _check_zip_entries(archive_path):
    """Downloaded archives are untrusted; entries must be vetted before any
    extractor (including ditto/unzip) touches them."""
    with zipfile.ZipFile(archive_path) as zf:
        for info in zf.infolist():
            target = None
            if info.create_system == 3 and stat.S_ISLNK(info.external_attr >> 16):
                target = zf.read(info).decode(errors="replace")
            if _entry_escapes(info.filename, target):
                fail("unsafe entry in %s: %s" % (os.path.basename(archive_path), info.filename))


def _check_tar_members(tf, archive_path):
    for m in tf.getmembers():
        target = m.linkname if (m.issym() or m.islnk()) else None
        if _entry_escapes(m.name, target):
            fail("unsafe entry in %s: %s" % (os.path.basename(archive_path), m.name))


def extract_archive(archive_path, dest_dir):
    name = archive_path.lower()
    if name.endswith((".tar.gz", ".tgz", ".tar.bz2", ".tar.xz", ".txz", ".tar")):
        with tarfile.open(archive_path) as tf:
            try:
                tf.extractall(dest_dir, filter="data")
            except TypeError:  # old python without the filter argument
                _check_tar_members(tf, archive_path)
                tf.extractall(dest_dir)
            except getattr(tarfile, "FilterError", ()) as e:
                # unsafe entry, not a corrupted download - do not retry
                fail("unsafe entry in %s: %s" % (os.path.basename(archive_path), e))
        return
    _check_zip_entries(archive_path)
    # zip: prefer ditto/unzip which preserve symlinks inside frameworks
    for cmd in (["/usr/bin/ditto", "-x", "-k", archive_path, dest_dir],
                ["unzip", "-q", archive_path, "-d", dest_dir]):
        if shutil.which(cmd[0]):
            r = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            if r.returncode == 0:
                return
            log("%s failed (%s), trying next extractor" % (cmd[0], r.stderr.decode(errors="replace").strip()))
    with zipfile.ZipFile(archive_path) as zf:
        zf.extractall(dest_dir)


def check_source_marker(prebuilt_src, pod, version, source_url):
    """Prebuilt mirror trees carry a .source.json provenance marker when they
    are copies of a downloaded src; verify it matches the request so a
    mis-filed or stale mirror entry fails here, not at link/run time."""
    marker_path = os.path.join(prebuilt_src, ".source.json")
    try:
        with open(marker_path) as f:
            marker = json.load(f)
    except OSError:
        log("WARNING: using unverified prebuilt package %s (no .source.json); "
            "populate the mirror by copying a downloaded src dir" % prebuilt_src)
        return
    except ValueError:
        fail("corrupt .source.json in prebuilt package %s" % prebuilt_src)

    if (marker.get("pod"), marker.get("version")) != (pod, version):
        fail("prebuilt package %s was made from %s/%s, but %s/%s is requested"
             % (prebuilt_src, marker.get("pod"), marker.get("version"), pod, version))
    if source_url and marker.get("url") != source_url:
        fail("prebuilt package %s was downloaded from %s, but the build pins %s; "
             "refresh the mirror entry"
             % (prebuilt_src, redact_url(marker.get("url") or ""), redact_url(source_url)))
    log("using prebuilt package %s (source %s)"
        % (prebuilt_src, redact_url(marker.get("url") or "")))


def ensure_src(pod_dir, pod, version, source_url=None, prebuilt_dir=None,
               no_download=False):
    """Download+extract the pod archive into <pod_dir>/src (atomic, reusable).

    A package of the same version pre-downloaded into <prebuilt_dir> (e.g.
    devtools mirror for offline build agents) is used as-is instead of
    downloading; only the prepared slice dirs are written to the cache.
    With no_download the mirror (or an already populated cache) is the only
    permitted source."""
    src_dir = os.path.join(pod_dir, "src")
    if os.path.isdir(src_dir):
        return src_dir

    if prebuilt_dir:
        prebuilt_src = os.path.join(prebuilt_dir, pod, version, "src")
        if os.path.isdir(prebuilt_src):
            check_source_marker(prebuilt_src, pod, version, source_url)
            return prebuilt_src

    if no_download:
        fail("downloads from pod repositories are disabled and %s/%s is not in "
             "the prebuilt dir (%s); either populate %s/%s/%s/src or build with "
             "-sCocoaPodsAllowDownload=yes"
             % (pod, version, prebuilt_dir or "<unset>",
                prebuilt_dir or "<prebuilt-dir>", pod, version))

    spec = {}
    url = source_url
    if not url:
        spec = get_podspec(pod, version)
        url = archive_url_from_source(spec.get("source"), version)
    if not url:
        fail("podspec of %s/%s has no downloadable source (source=%r); "
             "binary pods with vendored frameworks are required" % (pod, version, spec.get("source")))

    os.makedirs(pod_dir, exist_ok=True)
    tmp_dir = tempfile.mkdtemp(prefix="src.tmp.", dir=pod_dir)
    try:
        archive_name = os.path.basename(url.split("?")[0]) or "archive.zip"
        archive_path = os.path.join(tmp_dir, "__" + archive_name)
        extract_dir = os.path.join(tmp_dir, "x")
        # a corrupted archive usually means the download was cut short in a
        # way http could not detect, so re-download instead of giving up
        for attempt in range(RETRIES):
            log("downloading %s" % redact_url(url))
            http_get(url, archive_path)
            shutil.rmtree(extract_dir, ignore_errors=True)
            os.makedirs(extract_dir)
            try:
                extract_archive(archive_path, extract_dir)
                break
            except Exception as e:
                os.remove(archive_path)
                if attempt == RETRIES - 1 or out_of_retry_budget():
                    fail("cannot extract %s for %s/%s: %s" % (archive_name, pod, version, e))
                delay = 2 ** (attempt + 1) + random.uniform(0, 3)
                log("archive is corrupted (%s), re-downloading in %.1fs" % (e, delay))
                time.sleep(delay)
        os.remove(archive_path)
        # save podspec and provenance next to sources: the marker lets a
        # prebuilt mirror copy of this tree be verified later
        with open(os.path.join(extract_dir, ".podspec.json"), "w") as f:
            json.dump(spec, f, indent=2)
        with open(os.path.join(extract_dir, ".source.json"), "w") as f:
            json.dump({"pod": pod, "version": version, "url": url}, f, indent=1)
        try:
            os.rename(extract_dir, src_dir)
        except OSError:
            if not os.path.isdir(src_dir):  # lost the race for a different reason
                raise
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)
    return src_dir


def slice_matches(lib, platform, arch, variant):
    if lib.get("SupportedPlatform") != platform:
        return False
    if (lib.get("SupportedPlatformVariant") or "") != (variant or ""):
        return False
    return arch in lib.get("SupportedArchitectures", [])


def add_bundles_under(root, add):
    """Resource bundles of static frameworks may sit anywhere inside the
    framework/slice dir and must be copied to the app bundle explicitly."""
    for r, dirs, _files in os.walk(root, followlinks=False):
        for d in list(dirs):
            if d.endswith(".bundle"):
                dirs.remove(d)
                add(d, os.path.join(r, d))


def vendored_paths_from_podspec(spec, platform):
    """Collect vendored_frameworks/vendored_libraries relative paths for the
    platform (top level, platform section and subspecs)."""
    paths = []

    def take(d):
        for key in ("vendored_frameworks", "vendored_libraries"):
            v = d.get(key)
            if isinstance(v, str):
                paths.append(v)
            elif isinstance(v, list):
                paths.extend(v)

    def scan(d):
        take(d)
        plat = d.get(platform)
        if isinstance(plat, dict):
            take(plat)

    scan(spec)
    for sub in spec.get("subspecs") or []:
        if isinstance(sub, dict):
            scan(sub)
    return paths


def add_artifact_at(path, platform, arch, variant, add):
    """Add a framework/xcframework/static lib located at an explicit path."""
    if path.endswith(".xcframework") and os.path.isdir(path):
        info = os.path.join(path, "Info.plist")
        if not os.path.isfile(info):
            log("skipping %s: no Info.plist" % path)
            return
        with open(info, "rb") as f:
            plist = plistlib.load(f)
        for lib in plist.get("AvailableLibraries", []):
            if not slice_matches(lib, platform, arch, variant):
                continue
            lib_dir = os.path.join(path, lib["LibraryIdentifier"])
            lib_path = os.path.join(lib_dir, lib["LibraryPath"])
            add(os.path.basename(lib_path), lib_path)
            add_bundles_under(lib_dir, add)
            break
    elif path.endswith(".framework") and os.path.isdir(path):
        add(os.path.basename(path), path)  # plain (fat) framework
        add_bundles_under(path, add)
    elif path.endswith(".a") and os.path.isfile(path):
        add(os.path.basename(path), path)


def find_artifacts(src_dir, platform, arch, variant, spec=None):
    """Return {name: path} of frameworks/bundles/static libs for the slice.

    Artifacts listed in the podspec (vendored_frameworks/libraries) win:
    archives often carry several variants of the same framework and only
    the podspec knows the right one. Everything else found by walking the
    tree (resource bundles, adapter libs) is added unless already taken."""
    import glob as _glob

    found = {}
    vendored_names = set()

    def add_vendored(name, path):
        found.setdefault(name, path)

    def add(name, path):
        if name in vendored_names:
            return
        # archives may carry both variants; we link statically, prefer static
        prev = found.get(name)
        if prev is None:
            found[name] = path
        elif "static" in path.lower() and "static" not in prev.lower():
            log("preferring static variant of %s" % name)
            found[name] = path

    for rel in vendored_paths_from_podspec(spec or {}, platform):
        # archives may unpack with one wrapper dir (github tag zips)
        matches = (_glob.glob(os.path.join(_glob.escape(src_dir), rel), recursive=True)
                   or _glob.glob(os.path.join(_glob.escape(src_dir), "*", rel), recursive=True))
        if not matches:
            log("vendored path not found in archive: %s" % rel)
        for m in sorted(matches):
            add_artifact_at(m, platform, arch, variant, add_vendored)
    vendored_names = set(found)
    if vendored_names:
        log("vendored artifacts: %s" % ", ".join(sorted(vendored_names)))

    for root, dirs, _files in os.walk(src_dir, followlinks=False):
        for d in list(dirs):
            path = os.path.join(root, d)
            if d.endswith(".xcframework") or d.endswith(".framework"):
                dirs.remove(d)
                add_artifact_at(path, platform, arch, variant, add)
            elif d.endswith(".bundle"):
                dirs.remove(d)
                add(d, path)
        for fn in _files:
            if fn.endswith(".a"):
                add(fn, os.path.join(root, fn))
    return found


def slice_lock(slice_dir):
    """Exclusive lock keyed by the slice path: concurrent jam invocations
    (e.g. two configs of the same platform) share the same slice dir and
    must serialize its rebuild."""
    lf = open(slice_dir + ".lock", "w")
    fcntl.flock(lf, fcntl.LOCK_EX)
    return lf  # keep the fd open for the lifetime of the critical section


def populate_slice_dir(slice_dir, artifacts):
    # sweep tmp dirs orphaned by killed processes (we hold the slice lock)
    parent = os.path.dirname(slice_dir)
    for entry in os.listdir(parent):
        if entry.startswith(os.path.basename(slice_dir) + ".tmp."):
            shutil.rmtree(os.path.join(parent, entry), ignore_errors=True)

    tmp_dir = slice_dir + ".tmp.%d" % os.getpid()
    os.makedirs(tmp_dir)
    try:
        for name, path in sorted(artifacts.items()):
            os.symlink(os.path.relpath(path, slice_dir), os.path.join(tmp_dir, name))
        shutil.rmtree(slice_dir, ignore_errors=True)
        os.rename(tmp_dir, slice_dir)
    except BaseException:
        shutil.rmtree(tmp_dir, ignore_errors=True)
        raise


def check_expected_frameworks(expected, available, pod, version):
    missing = [fw for fw in expected if fw + ".framework" not in available]
    if missing:
        fail("pod %s/%s does not provide framework(s): %s; available: %s"
             % (pod, version, ", ".join(missing), ", ".join(sorted(available))))


def check_stamp(stamp, args):
    """Warm cache: re-validate the request against the recorded artifact
    list, so a renamed framework fails here and not at link time."""
    if not os.path.isfile(stamp):
        return False
    try:
        with open(stamp) as f:
            artifacts = json.load(f).get("artifacts", [])
    except (OSError, ValueError):
        return False  # unreadable stamp, rebuild the slice
    check_expected_frameworks(args.frameworks, artifacts, args.pod, args.pod_version)
    return True


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--pod", required=True)
    p.add_argument("--pod-version", required=True)
    p.add_argument("--cache-dir", required=True)
    p.add_argument("--platform", default="ios", choices=["ios", "tvos"])
    p.add_argument("--arch", default="arm64")
    p.add_argument("--variant", default=None, choices=[None, "simulator", "maccatalyst"])
    p.add_argument("--frameworks", nargs="*", default=[],
                   help="framework names the build expects to link (validated)")
    p.add_argument("--source-url", default=None,
                   help="download this archive instead of the podspec source")
    p.add_argument("--prebuilt-dir", default=None,
                   help="use <dir>/<pod>/<version>/src instead of downloading, if present")
    p.add_argument("--no-download", action="store_true",
                   help="forbid fetching from pod repositories; only the prebuilt dir "
                        "or an already populated cache may be used")
    args = p.parse_args()

    slice_id = "%s-%s" % (args.platform, args.arch)
    if args.variant:
        slice_id += "-" + args.variant

    pod_dir = os.path.join(args.cache_dir, args.pod, args.pod_version)
    slice_dir = os.path.join(pod_dir, slice_id)
    stamp = os.path.join(slice_dir, ".jam-ready")
    if check_stamp(stamp, args):
        return

    os.makedirs(pod_dir, exist_ok=True)
    lock = slice_lock(slice_dir)
    if check_stamp(stamp, args):  # done by a concurrent invocation while we waited
        return

    src_dir = ensure_src(pod_dir, args.pod, args.pod_version, args.source_url,
                         args.prebuilt_dir, args.no_download)
    spec = None
    spec_path = os.path.join(src_dir, ".podspec.json")
    if os.path.isfile(spec_path):
        with open(spec_path) as f:
            spec = json.load(f)
    artifacts = find_artifacts(src_dir, args.platform, args.arch, args.variant, spec)
    if not artifacts:
        fail("no frameworks/bundles/libs for slice %s found in %s" % (slice_id, src_dir))

    check_expected_frameworks(args.frameworks, artifacts, args.pod, args.pod_version)

    populate_slice_dir(slice_dir, artifacts)
    with open(stamp, "w") as f:
        json.dump({"pod": args.pod, "version": args.pod_version,
                   "slice": slice_id, "artifacts": sorted(artifacts)}, f, indent=1)
    log("%s/%s [%s]: %s" % (args.pod, args.pod_version, slice_id, ", ".join(sorted(artifacts))))


if __name__ == "__main__":
    main()
