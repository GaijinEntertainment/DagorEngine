#!/usr/bin/env python3

"""httpFetch test runner.

Spins up a small in-process HTTP server on 127.0.0.1:<ephemeral>, runs each
*.nut in this directory through csq with the base URL passed as the first
script argument (read from `__argv` on the script side), and diffs the
captured stdout against *.nut.expected.

Routes served by the fixture:
  GET  /ok          -> 200, application/json, {"hello":"world"}
  GET  /notfound    -> 404, text/plain, "missing"
  POST /echo        -> 200, content-type echoes request, body echoed verbatim
  GET  /slow?ms=N   -> 200, after N ms, plain "slept"
"""

import argparse
import http.server
import os
import socket
import sys
import threading
import time
import urllib.parse
from pathlib import Path
from subprocess import Popen, PIPE


class FixtureHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        # Silence the default per-request stderr logging.
        pass

    def _send_text(self, code, ctype, body):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        if url.path == "/ok":
            self._send_text(200, "application/json", b'{"hello":"world"}')
        elif url.path == "/notfound":
            self._send_text(404, "text/plain", b"missing")
        elif url.path == "/slow":
            qs = urllib.parse.parse_qs(url.query)
            try:
                ms = int(qs.get("ms", ["50"])[0])
            except ValueError:
                ms = 50
            time.sleep(ms / 1000.0)
            self._send_text(200, "text/plain", b"slept")
        else:
            self._send_text(404, "text/plain", b"unknown")

    def do_POST(self):
        url = urllib.parse.urlparse(self.path)
        if url.path == "/echo":
            length = int(self.headers.get("Content-Length", "0"))
            body = self.rfile.read(length) if length > 0 else b""
            ctype = self.headers.get("Content-Type", "application/octet-stream")
            self._send_text(200, ctype, body)
        else:
            self._send_text(404, "text/plain", b"unknown")


def start_fixture():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), FixtureHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, port


def run_test(csq, test_file, base_url, update_expected, verbose):
    expected_file = test_file + ".expected"
    cmd = [csq, str(test_file), "--", base_url]

    if verbose:
        print(f"  CMD: {cmd}")

    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
    try:
        stdout, stderr = proc.communicate(timeout=30)
    except Exception:
        proc.kill()
        stdout, stderr = proc.communicate()
        print(f"TIMEOUT: {test_file}")
        return False

    actual_lines = stdout.decode("utf-8", errors="replace").splitlines()

    expected_path = Path(expected_file)
    if proc.returncode != 0 and not expected_path.exists():
        print(f"CRASH: {test_file} (exit code {proc.returncode})")
        if actual_lines:
            print("--stdout------------------")
            print("\n".join(actual_lines))
        stderr_text = stderr.decode("utf-8", errors="replace").strip()
        if stderr_text:
            print("--stderr------------------")
            print(stderr_text)
        print("--------------------------")
        return False

    if update_expected or not expected_path.exists():
        if not expected_path.exists():
            print(f"  info: no expected file for {test_file}, creating it")
        else:
            print(f"  info: updating expected file for {test_file}")
        with open(expected_file, "w", newline="\n") as f:
            for line in actual_lines:
                f.write(line + "\n")
        print(f"PASSED: {test_file}")
        return True

    with open(expected_file) as f:
        expected_lines = [l.rstrip() for l in f.readlines()]

    actual_stripped = [l.rstrip() for l in actual_lines]

    differs = len(expected_lines) != len(actual_stripped) or any(
        a != e for a, e in zip(actual_stripped, expected_lines))

    if differs:
        print(f"FAIL: {test_file}")
        print("--Actual------------------")
        print("\n".join(actual_lines))
        print("--Expected----------------")
        print("\n".join(expected_lines))
        print("--------------------------")
        return False

    print(f"PASSED: {test_file}")
    return True


def main():
    parser = argparse.ArgumentParser(description="httpFetch test runner")
    parser.add_argument("--csq", type=str,
                        default=os.path.join("tools", "dagor_cdk", "windows-x86_64", "csq-dev.exe"),
                        help="Path to csq executable")
    parser.add_argument("-v", "--verbose", default=False, action="store_true")
    parser.add_argument("--update", default=False, action="store_true",
                        help="Overwrite .expected files with actual output")
    args = parser.parse_args()

    csq = args.csq
    if not os.path.isabs(csq):
        # Resolve relative to repo root: this file is 4 levels deep under prog/.
        repo_root = Path(__file__).resolve().parents[5]
        csq = str(repo_root / args.csq)

    if not os.path.isfile(csq):
        print(f"csq not found: {csq}", file=sys.stderr)
        return 2

    tests_dir = Path(__file__).resolve().parent
    test_files = sorted(tests_dir.glob("*.nut"))
    if not test_files:
        print(f"No tests found in {tests_dir}", file=sys.stderr)
        return 2

    server, port = start_fixture()
    base_url = f"http://127.0.0.1:{port}"
    if args.verbose:
        print(f"  fixture URL: {base_url}")
    try:
        failed = 0
        for tf in test_files:
            if not run_test(csq, str(tf), base_url, args.update, args.verbose):
                failed += 1
    finally:
        server.shutdown()

    print()
    print(f"Tests: {len(test_files)}  Failed: {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
