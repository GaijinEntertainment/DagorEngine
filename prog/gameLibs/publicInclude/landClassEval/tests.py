#!/usr/bin/env python3
"""Auto-run landClassEvalUnittest when landClassEval headers change."""
import subprocess, sys, os

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "../../../.."))
UNITTEST_DIR = os.path.join(ROOT, "prog", "samples", "unittest", "landClassEvalUnittest")

def run():
    print("=== Building landClassEvalUnittest ===")
    r = subprocess.run(["jam"], cwd=UNITTEST_DIR, capture_output=False)
    if r.returncode != 0:
        print("BUILD FAILED")
        return r.returncode

    # Find the built exe (dev or rel)
    for name in ["landClassEvalUnittest-dev.exe", "landClassEvalUnittest.exe", "landClassEvalUnittest-dev", "landClassEvalUnittest"]:
        exe = os.path.join(ROOT, "tools/util", name)
        if os.path.isfile(exe):
            print(f"\n=== Running {name} ===")
            r = subprocess.run([exe], capture_output=False)
            return r.returncode
        # Also check in the unittest dir itself
        exe = os.path.join(UNITTEST_DIR, name)
        if os.path.isfile(exe):
            print(f"\n=== Running {name} ===")
            r = subprocess.run([exe], capture_output=False)
            return r.returncode

    print("ERROR: could not find built unittest executable")
    return 1

if __name__ == "__main__":
    sys.exit(run())
