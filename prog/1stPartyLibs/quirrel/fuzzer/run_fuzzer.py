import threading
import multiprocessing
import tempfile
import os
import subprocess
import sys
import time
import re
import signal

error_detected = False
process_timeout = 4
batchSize = 80
csqArgsOpt = "--check-stack --opt-closure-hoisting --hide-compilation-error"
csqArgsNoOpt = "--check-stack --hide-compilation-error"
csq = "csq"
csq2 = ""
cwd = os.getcwd().replace("\\", "/")
retain_failed_tests = False
do_minimize = False
my_process_id = os.getpid()


def signal_handler(sig, frame):
    global error_detected
    error_detected = True
    print("Fuzzer: Quitting on signal ", sig)
    exit(1)

signal.signal(signal.SIGINT, signal_handler)  # CTRL+C
signal.signal(signal.SIGTERM, signal_handler) # Termination request


def run_cmd(cmd):
    global process_timeout
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=process_timeout, check=False)
        return result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return "Fatal error: Timeout\n"
    except Exception as e:
        return f"Fatal error: {str(e)}\n"


def normalize_output(s):
    s = s.replace(".no-opt.nut", ".nut").replace(".opt.nut", ".nut")
    s = re.sub(r"0x[0-9a-fA-F]+", "0xHEXNUMBER", s)
    # Remove everything from [this] to === (closure hoisting changes local variable visibility)
    s = re.sub(r"\[this\][^\n]*.*?(?====)", "", s, flags=re.DOTALL)
    return s


_MINIMIZE_TIMEOUT = 60      # seconds per file
_MINIMIZE_CSQ_TIMEOUT = 5   # seconds per csq execution during minimize


def _kill_process_tree(proc):
    """Kill a process and all its children."""
    if os.name == 'nt':
        subprocess.run(f'taskkill /F /T /PID {proc.pid}',
                       shell=True, capture_output=True)
    else:
        proc.kill()
    proc.wait()


def _minimize_run_cmd(cmd):
    """Like run_cmd but with shorter timeout for minimize."""
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, shell=True)
        stdout, stderr = proc.communicate(timeout=_MINIMIZE_CSQ_TIMEOUT)
        return stdout + stderr
    except subprocess.TimeoutExpired:
        _kill_process_tree(proc)
        return "Fatal error: Timeout\n"
    except Exception as e:
        return f"Fatal error: {str(e)}\n"


def _minimize_run(cmd, file_path):
    """Run a csq command with short timeout and return normalized output."""
    out = _minimize_run_cmd(cmd + " " + file_path)
    base = os.path.basename(file_path)
    out = out.replace(file_path, "FILE").replace(base, "FILE")
    return normalize_output(out)


_EMPTY_LOOP_RE = re.compile(
    r'((?:while|for)\s*\([^)]*\)\s*\{)\s*(\})',
    re.MULTILINE,
)


def _patch_empty_loops(content):
    """Insert 'break' into empty loop bodies to prevent infinite loops."""
    return _EMPTY_LOOP_RE.sub(r'\1 break \2', content)


def _minimize_bug_reproduces(lines, tmp_dir, capture_outputs=None):
    """Write lines to temp .nut files (opt + no-opt) and check for divergence."""
    opt_path = os.path.join(tmp_dir, "min.opt.nut")
    noopt_path = os.path.join(tmp_dir, "min.no-opt.nut")
    content = _patch_empty_loops("".join(lines))
    with open(opt_path, "w") as f:
        f.write("//#disable-optimizer\n")
        f.write(content)
    with open(noopt_path, "w") as f:
        f.write("#disable-optimizer\n")
        f.write(content)
    results = [None, None]
    def _run_opt():
        results[0] = _minimize_run(f"{csq} {csqArgsOpt}", opt_path)
    def _run_noopt():
        results[1] = _minimize_run(f"{csq} {csqArgsNoOpt}", noopt_path)
    t = threading.Thread(target=_run_noopt)
    t.start()
    _run_opt()
    t.join()
    out_opt, out_noopt = results
    if capture_outputs is not None:
        capture_outputs['opt'] = out_opt
        capture_outputs['noopt'] = out_noopt
    return out_opt != out_noopt


def _minimize_one(file_path):
    """Line-based delta-debugging minimizer for a single failing .nut file."""
    with open(file_path) as f:
        original_lines = f.readlines()

    # Strip existing optimizer pragmas - we add our own
    lines = [l for l in original_lines
             if l.strip() not in ("#disable-optimizer", "//#disable-optimizer")]

    print(f"  Minimizing: {file_path} ({len(lines)} lines)")

    with tempfile.TemporaryDirectory() as tmp_dir:
        if not _minimize_bug_reproduces(lines, tmp_dir):
            print(f"  SKIP: bug does not reproduce on {file_path}")
            return

        deadline = time.time() + _MINIMIZE_TIMEOUT
        next_progress = time.time() + 10
        start_time = time.time()
        start_lines = len(lines)
        tests_done = 0

        prev_len = len(lines) + 1
        timed_out = False
        while len(lines) < prev_len and not timed_out:
            prev_len = len(lines)

            chunk_size = max(1, len(lines) // 2)
            while chunk_size >= 1 and not timed_out:
                i = max(0, len(lines) - chunk_size)
                while i >= 0:
                    if time.time() > deadline:
                        timed_out = True
                        break
                    if time.time() > next_progress:
                        next_progress = time.time() + 10
                        elapsed = time.time() - start_time
                        remaining = max(0, deadline - time.time())
                        print(f"    {len(lines)} lines, {tests_done} tests, "
                              f"{elapsed:.0f}s elapsed, {remaining:.0f}s remaining")
                    candidate = lines[:i] + lines[i + chunk_size:]
                    tests_done += 1
                    if candidate and _minimize_bug_reproduces(candidate, tmp_dir):
                        lines = candidate
                        i = min(i, max(0, len(lines) - chunk_size))
                    else:
                        i -= 1
                chunk_size //= 2

        if timed_out:
            print(f"    Time limit reached ({_MINIMIZE_TIMEOUT}s)")

        # Capture final outputs for the comment block
        final_outputs = {}
        _minimize_bug_reproduces(lines, tmp_dir, capture_outputs=final_outputs)

    # Save as <name>.min.<ext> - patch empty loops in final output too
    base, ext = os.path.splitext(file_path)
    out_path = f"{base}.min{ext}"

    def _truncate(s, max_lines=20):
        out_lines = s.rstrip('\n').split('\n')
        if len(out_lines) > max_lines:
            return '\n'.join(out_lines[:max_lines]) + f'\n... ({len(out_lines) - max_lines} more lines)'
        return '\n'.join(out_lines)

    with open(out_path, "w") as f:
        f.write(_patch_empty_loops("".join(lines)))
        f.write("\n")
        f.write("// ----- How to reproduce -----\n")
        f.write(f"// Optimizer ON (add //#disable-optimizer at the top):\n")
        f.write(f"//   {csq} {csqArgsOpt} {out_path}\n")
        f.write(f"// Optimizer OFF (add #disable-optimizer at the top):\n")
        f.write(f"//   {csq} {csqArgsNoOpt} {out_path}\n")
        f.write("//\n")
        f.write("// --- Output with optimizer ON ---\n")
        for line in _truncate(final_outputs.get('opt', '')).split('\n'):
            f.write(f"// {line}\n")
        f.write("//\n")
        f.write("// --- Output with optimizer OFF ---\n")
        for line in _truncate(final_outputs.get('noopt', '')).split('\n'):
            f.write(f"// {line}\n")

    print(f"  Result: {start_lines} -> {len(lines)} lines, saved to {out_path}")


def run_minimize():
    """Find all .failed files in cwd and minimize each."""
    import glob
    failed_files = sorted(glob.glob("*.failed"))
    if not failed_files:
        print("No .failed files found in current directory")
        return 1
    print(f"Found {len(failed_files)} failed file(s) to minimize")
    for f in failed_files:
        _minimize_one(f)
    return 0


def on_error_detected(index, message):
    print(message)
    try:
        nut = open(f"sqf{my_process_id}-{index}.opt.nut", "r").read()
        print(f"===========================\n{nut}\n=========================== (seed = {index})\n")
        if retain_failed_tests:
            failed_path = f"{cwd}/sqf{index}.opt.nut.failed"
            open(failed_path, "w").write(nut)
            if do_minimize:
                _minimize_one(failed_path)
    except FileNotFoundError:
        print(f"(could not read program file for seed {index})")


def fuzzer_thread_func(begin, end, step):
    global error_detected
    t0 = time.time()

    seed_list = []
    for x in range(begin, end, step):
        seed_list.append(x)

    filesProcessed = 0
    printProgressTime = t0 + 2

    for x in range(0, len(seed_list), batchSize):
        if error_detected:
            break

        if begin == 0: # Print progress only from the first thread
            if time.time() > printProgressTime:
                printProgressTime = time.time() + 10
                remaining_time = (time.time() - t0) / (filesProcessed + 0.0001) * (end / step - filesProcessed)
                minutes = int(remaining_time / 60) % 60
                seconds = int(remaining_time) % 60
                print(f"... {filesProcessed * step} tests done, remaining time: {minutes}:{seconds:02d}")

        seed_set = ""
        for j in range(batchSize):
            if x + j < len(seed_list):
                if j > 0:
                    seed_set += ","
                seed_set += f"{seed_list[x + j]}"

        # generate scripts for tests
        run_cmd(f"{csq} {csqArgsNoOpt} {cwd}/sqfuzzer.nut -- seedset={seed_set} fileprefix=sqf{my_process_id}-")

        # run all scripts at once
        cmd_opt = ""
        cmd_no_opt = ""
        for i in seed_list[x : x + batchSize]:
            cmd_opt += f" sqf{my_process_id}-{i}.opt.nut"
            cmd_no_opt += f" sqf{my_process_id}-{i}.no-opt.nut"

        accum_output_no_opt = normalize_output(run_cmd(f"{csq} {csqArgsNoOpt} {cmd_no_opt}"))
        accum_output_opt = normalize_output(run_cmd(f"{csq} {csqArgsOpt} {cmd_opt}"))
        accum_output_csq2 = ""
        if csq2 != "":
            accum_output_csq2 = normalize_output(run_cmd(f"{csq2} {csqArgsOpt} {cmd_opt}"))

        # if any of the scripts crashed or produced different output, then run scripts one by one
        if ("Fatal" in accum_output_opt or "Fatal" in accum_output_no_opt or "Fatal" in accum_output_csq2 or
                accum_output_opt != accum_output_no_opt or (csq2 != "" and accum_output_opt != accum_output_csq2)):

            print("Error detected in batch run, running scripts one by one...")
            error_detected = True

            if retain_failed_tests:
                # copy outputs to files for later analysis
                open(f"{cwd}/fuzzer_batch_{x}_no_opt.out", "w").write(accum_output_no_opt)
                open(f"{cwd}/fuzzer_batch_{x}_opt.out", "w").write(accum_output_opt)
                if csq2 != "":
                    open(f"{cwd}/fuzzer_batch_{x}_csq2.out", "w").write(accum_output_csq2)

            if "Timeout" in accum_output_no_opt or "Timeout" in accum_output_opt or "Timeout" in accum_output_csq2:
                print(f"Fuzzer: Timeout detected in batch run {x}, skipping detailed analysis of individual tests in this batch.")
                continue

            for i in seed_list[x : x + batchSize]:
                output_disabled_opt = normalize_output(run_cmd(f"{csq} {csqArgsNoOpt} sqf{my_process_id}-{i}.no-opt.nut"))
                output_enabled_opt = normalize_output(run_cmd(f"{csq} {csqArgsOpt} sqf{my_process_id}-{i}.opt.nut"))
                output_csq2 = ""
                if csq2 != "":
                    output_csq2 = normalize_output(run_cmd(f"{csq2} {csqArgsOpt} sqf{my_process_id}-{i}.opt.nut"))

                crash_version = ""
                if "Fatal" in output_disabled_opt:
                    crash_version = "csq, without optimizer"
                elif "Fatal" in output_enabled_opt:
                    crash_version = "csq, with optimizer"
                elif "Fatal" in output_csq2:
                    crash_version = "csq2"

                if ("Fatal" in output_disabled_opt) or ("Fatal" in output_enabled_opt) or ("Fatal" in output_csq2):
                    on_error_detected(i, f"\nERROR: Fuzzer: Seed {i} crashed in {crash_version}\n")
                elif output_disabled_opt != output_enabled_opt:
                    on_error_detected(i, f"\nERROR: Fuzzer: Seed {i} produced different output with and without optimizer\n")
                elif csq2 != "" and output_enabled_opt != output_csq2:
                    on_error_detected(i, f"\nERROR: Fuzzer: Seed {i} produced different output with two csq versions\n")

        for i in seed_list[x : x + batchSize]:
            filesProcessed += 1
            try:
                os.remove(f"sqf{my_process_id}-{i}.no-opt.nut")
                os.remove(f"sqf{my_process_id}-{i}.opt.nut")
            except Exception as e:
                if not error_detected:
                    print(f"ERROR: Fuzzer: {str(e)}")
                    error_detected = True



def print_usage():
    print("\nUsage: python run_fuzzer.py <arguments>\n" +
          "  --numtests=<num_tests> - number of tests to run\n" +
          "  --csq=<path_to_csq.exe> - path to csq.exe\n" +
          "  --csq2=<path_to_csq2.exe> - path to second csq.exe, if specified, compare outputs of two csq versions\n" +
          "  --singletest=<seed> - run single test with specified seed\n" +
          "  --retain - retain files that caused errors\n" +
          "  --minimize - minimize .failed files (standalone or after fuzzing with --retain)\n"
          "\nExample:\n" +
          "  python run_fuzzer.py --numtests=10000 --csq=D:/dagor/tools/dagor_cdk/windows-x86_64/csq-dev.exe --retain\n" +
          "  python run_fuzzer.py --singletest=12345 --csq=csq-dev.exe --csq2=csq-old-dev.exe --retain\n" +
          "  python run_fuzzer.py --minimize --csq=csq-dev.exe\n"
          )
    exit(0)


def check_csq_version(exe_path, var_name):

    fullExeName = exe_path
    if os.name == 'nt' and not fullExeName.lower().endswith('.exe'):
        fullExeName += '.exe'

    try:
        if os.name == 'nt':
            if not os.path.isfile(fullExeName):
                fullExeName = run_cmd(f'where {fullExeName}').split('\n')[0].strip()
            csqFileTime = time.ctime(os.path.getmtime(fullExeName))
            print(f"{var_name} file time: {csqFileTime}")

        csqVersion = run_cmd(f'{fullExeName} --version').replace('\n', '').replace('\r', '').split('.')

        print(f"{var_name} version: {csqVersion[0]}.{csqVersion[1]}.{csqVersion[2]}")

        if int(csqVersion[0]) * 1e6 + int(csqVersion[1]) * 1e3 + int(csqVersion[2]) < 1e6 + 0 * 1e3 + 33:
            print(f"\nERROR: {exe_path} version must be 1.0.33 or higher, current version: {csqVersion[0]}.{csqVersion[1]}.{csqVersion[2]}")
            exit(1)

    except Exception as e:
        print(f"\nERROR: {exe_path} not found, please specify the path to csq.exe using --{var_name}=<path_to_csq.exe>")
        exit(1)


if __name__ == '__main__':
    begin_test = 0
    end_test = 5000

    if len(sys.argv) == 1:
        print_usage()

    for i in range(1, len(sys.argv)):
        if (sys.argv[i] == "--help") or (sys.argv[i] == "-h"):
            print_usage()
        elif sys.argv[i] == "--minimize":
            do_minimize = True
        elif sys.argv[i].startswith("--singletest="):
            begin_test = int(sys.argv[i].split("=")[1])
            end_test = begin_test + 1
        elif sys.argv[i].startswith("--numtests="):
            begin_test = 0
            end_test = int(sys.argv[i].split("=")[1])
        elif sys.argv[i] == "--retain":
            retain_failed_tests = True
        elif sys.argv[i].startswith("--csq="):
            csq = os.path.abspath(sys.argv[i].split("=")[1])
        elif sys.argv[i].startswith("--csq2="):
            csq2 = os.path.abspath(sys.argv[i].split("=")[1])
        else:
            print(f"ERROR: Unknown argument: {sys.argv[i]}")
            exit(1)

    check_csq_version(csq, "csq")

    # --minimize alone: just minimize existing .failed files and exit
    if do_minimize and "--numtests=" not in " ".join(sys.argv) and "--singletest=" not in " ".join(sys.argv):
        exit(run_minimize())

    if csq2 != "":
        check_csq_version(csq2, "csq2")

    print(f"Running fuzzer with {end_test - begin_test} tests")
    print(f"Temporary directory: {tempfile.gettempdir()}")
    os.chdir(tempfile.gettempdir())

    cores = min(multiprocessing.cpu_count(), 16)
    threads = []
    for i in range(int(cores)):
        t = threading.Thread(target=fuzzer_thread_func, args=(i + begin_test, end_test, int(cores)))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    if os.getcwd() != tempfile.gettempdir():
        print("\nERROR: Fuzzer: Unexpected directory change during execution, cwd = ", os.getcwd())

    if error_detected:
        os.chdir(cwd)
        exit(1)
    else:
        print("Fuzzer: OK")
        os.chdir(cwd)
        exit(0)
