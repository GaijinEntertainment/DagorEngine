#!/usr/bin/env python3

"""FRP test runner for Quirrel FRP library.

Discovers all *.nut files in the tests directory, runs each via csq,
captures stdout, and compares line-by-line to *.nut.expected files.

Modeled on prog/1stPartyLibs/quirrel/quirrel/testRunner.py
"""

import os
import sys
import argparse
import threading
from pathlib import Path
from subprocess import Popen, PIPE
from concurrent.futures import ThreadPoolExecutor, as_completed

CRED = '\33[31m'
CGREEN = '\33[32m'
RESET = "\033[0;0m"
CBOLD = '\33[1m'

THREADS = 12
printLock = threading.Lock()

numOfTests = 0
numOfFailedTests = 0
verbose = False
ciRun = False


def xprint(s, color=''):
    global ciRun
    if not ciRun:
        print(color, end='')
    print(s, end='')
    if not ciRun:
        print(RESET)
    else:
        print('')


def runTest(csq, testFile, updateExpected):
    global numOfTests, numOfFailedTests, verbose

    with printLock:
        numOfTests += 1

    expectedFile = testFile + '.expected'
    cmd = [csq, str(testFile), '-frp-test:1']

    if verbose:
        with printLock:
            xprint(f"  CMD: {cmd}")

    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
    try:
        stdout, stderr = proc.communicate(timeout=30)
    except Exception:
        proc.kill()
        stdout, stderr = proc.communicate()
        with printLock:
            xprint(f"\nTIMEOUT: {testFile}", CBOLD + CRED)
            numOfFailedTests += 1
        return

    LOG_PREFIXES = ('LOG ERROR:', 'LOGERR:', 'LOG WARNING:', 'LOGWARN:')
    allLines = stdout.decode('utf-8', errors='replace').splitlines()
    actualLines = [l for l in allLines if not l.startswith(LOG_PREFIXES)]
    logLines = [l for l in allLines if l.startswith(LOG_PREFIXES)]

    with printLock:
        expectedPath = Path(expectedFile)

        if proc.returncode != 0 and not expectedPath.exists():
            xprint(f"\nCRASH: {testFile} (exit code {proc.returncode})", CBOLD + CRED)
            if actualLines:
                xprint("--stdout------------------", CBOLD + CRED)
                xprint('\n'.join(actualLines))
            stderrText = stderr.decode('utf-8', errors='replace').strip()
            if stderrText:
                xprint("--stderr------------------", CBOLD + CRED)
                xprint(stderrText)
            xprint("--------------------------", CBOLD + CRED)
            xprint("")
            numOfFailedTests += 1
            return
        if updateExpected or not expectedPath.exists():
            if not expectedPath.exists():
                xprint(f"  info: no expected file for {testFile}, creating it")
            else:
                xprint(f"  info: updating expected file for {testFile}")
            with open(expectedFile, 'w', newline='\n') as f:
                for line in actualLines:
                    f.write(line + '\n')
            xprint(f"PASSED: {testFile}", CBOLD + CGREEN)
            return

        with open(expectedFile) as f:
            expectedLines = [l.rstrip() for l in f.readlines()]

        actualStripped = [l.rstrip() for l in actualLines]

        differs = len(expectedLines) != len(actualStripped)
        if not differs:
            for i in range(len(expectedLines)):
                if expectedLines[i] != actualStripped[i]:
                    differs = True
                    break

        if differs:
            xprint(f"\nFAIL: {testFile}", CBOLD + CRED)
            xprint("--Actual------------------", CBOLD + CRED)
            xprint('\n'.join(actualLines))
            xprint("--Expected----------------", CBOLD + CRED)
            xprint('\n'.join(expectedLines))
            xprint("--------------------------", CBOLD + CRED)
            xprint("")
            numOfFailedTests += 1
            return

        # Check expected log errors/warnings (substring matching)
        expectedLogerrFile = testFile + '.expected_logerr'
        expectedLogerrPath = Path(expectedLogerrFile)
        if expectedLogerrPath.exists():
            with open(expectedLogerrFile) as f:
                patterns = [l.strip() for l in f.readlines() if l.strip()]
            missing = [p for p in patterns if not any(p in ll for ll in logLines)]
            if missing:
                xprint(f"\nFAIL: {testFile} (missing expected log messages)", CBOLD + CRED)
                for m in missing:
                    xprint(f"  missing: {m}", CBOLD + CRED)
                if logLines:
                    xprint("--Actual log lines--------", CBOLD + CRED)
                    for ll in logLines:
                        xprint(f"  {ll}")
                else:
                    xprint("  (no log lines in output)", CBOLD + CRED)
                xprint("--------------------------", CBOLD + CRED)
                xprint("")
                numOfFailedTests += 1
                return

        xprint(f"PASSED: {testFile}", CBOLD + CGREEN)


def main():
    global verbose, ciRun, numOfFailedTests

    parser = argparse.ArgumentParser(description='FRP test runner')
    parser.add_argument('--csq', type=str,
                        default=os.path.join('tools', 'dagor_cdk', 'windows-x86_64', 'csq-dev.exe'),
                        help='Path to csq executable')
    parser.add_argument('-v', '--verbose', default=False, action='store_true',
                        help='Show commands being run')
    parser.add_argument('--update', default=False, action='store_true',
                        help='Overwrite .expected files with actual output')
    parser.add_argument('--ci', default=False, action='store_true',
                        help='CI mode (no colors)')

    args = parser.parse_args()
    verbose = args.verbose
    ciRun = args.ci

    csq = args.csq
    if not os.path.isabs(csq):
        # Try relative to script dir's ancestor (repo root)
        scriptDir = Path(__file__).resolve().parent
        # Walk up to find repo root (where tools/ lives)
        for parent in [scriptDir] + list(scriptDir.parents):
            candidate = parent / csq
            if candidate.exists():
                csq = str(candidate)
                break

    if not os.path.exists(csq):
        xprint(f"FAIL: csq executable not found: {csq}", CBOLD + CRED)
        sys.exit(1)

    testDir = Path(__file__).resolve().parent
    EXCLUDE = {'helpers.nut'}
    testFiles = sorted(f for f in testDir.glob('*.nut') if f.name not in EXCLUDE)

    if not testFiles:
        xprint("No test files found!", CBOLD + CRED)
        sys.exit(1)

    xprint(f"Running {len(testFiles)} FRP tests with {csq}\n")

    with ThreadPoolExecutor(max_workers=THREADS) as pool:
        futures = [pool.submit(runTest, csq, str(tf), args.update)
                   for tf in testFiles]
        for fut in as_completed(futures):
            fut.result()

    print()
    if numOfFailedTests:
        xprint(f"Failed tests: {numOfFailedTests}/{numOfTests}", CBOLD + CRED)
    else:
        xprint(f"All {numOfTests} tests passed", CBOLD + CGREEN)

    sys.exit(1 if numOfFailedTests > 0 else 0)


if __name__ == '__main__':
    main()
