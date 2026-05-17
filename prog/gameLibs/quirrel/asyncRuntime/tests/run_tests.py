#!/usr/bin/env python3

"""asyncRuntime test runner.

Runs each *.nut in this directory through csq, captures stdout, and
diffs against *.nut.expected. Modeled on prog/gameLibs/quirrel/frp/tests/run_tests.py
but kept simple (no parallelism, no logerr matching -- async tests are
self-contained scripts and don't emit log warnings).
"""

import argparse
import os
import sys
from pathlib import Path
from subprocess import Popen, PIPE


def run_test(csq, test_file, update_expected, verbose):
    expected_file = test_file + ".expected"
    cmd = [csq, str(test_file)]

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
    parser = argparse.ArgumentParser(description="asyncRuntime test runner")
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

    failed = 0
    for tf in test_files:
        if not run_test(csq, str(tf), args.update, args.verbose):
            failed += 1

    print()
    print(f"Tests: {len(test_files)}  Failed: {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
