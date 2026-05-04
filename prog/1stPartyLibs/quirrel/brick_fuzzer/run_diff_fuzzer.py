"""
Parallel differential fuzzer for Code Bricks - tests Quirrel programs
with and without bytecode optimizer.

Generates programs via code_bricks.py, creates two variants:
  .opt.nut    - optimizer ON  (no pragma)
  .no-opt.nut - optimizer OFF (#disable-optimizer)

Runs both through csq-dev.exe, compares normalized outputs.

Usage:
    python run_diff_fuzzer.py --numtests=10000 --csq=D:/dagor/tools/dagor_cdk/windows-x86_64/csq-dev.exe
    python run_diff_fuzzer.py --singletest=42 --csq=csq-dev.exe --retain
    python run_diff_fuzzer.py --numtests=5000 --csq=csq-dev.exe --csq2=csq-old.exe
    python run_diff_fuzzer.py --minimize --csq=csq-dev.exe
"""

import threading
import multiprocessing
import tempfile
import os
import subprocess
import sys
import time
import re
import signal

# Add code_bricks directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from code_bricks import assemble, _excluded_bricks
import code_bricks

# Exclude bricks that use __FILE__ since it differs between .opt.nut and .no-opt.nut
code_bricks._excluded_bricks = {"magic_file"}

_gen_pool = None  # multiprocessing.Pool for parallel generation
error_detected = False
process_timeout = 30
batch_size = 80
bricks_per_program = 15
csq_args_opt = "--check-stack --opt-closure-hoisting --hide-compilation-error"
csq_args_no_opt = "--check-stack --hide-compilation-error"
csq = "csq"
csq2 = ""
cwd = os.getcwd().replace("\\", "/")
retain_failed_tests = False
do_minimize = False
my_process_id = os.getpid()


def signal_handler(sig, frame):
    global error_detected
    error_detected = True
    print("Fuzzer: Quitting on signal", sig)
    exit(1)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)


def _kill_process_tree(proc):
    """Kill a process and all its children."""
    if os.name == 'nt':
        subprocess.run(f'taskkill /F /T /PID {proc.pid}',
                       shell=True, capture_output=True)
    else:
        proc.kill()
    proc.wait()


def run_cmd(cmd):
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, shell=True)
        stdout, stderr = proc.communicate(timeout=process_timeout)
        return stdout + stderr
    except subprocess.TimeoutExpired:
        _kill_process_tree(proc)
        return "Fatal error: Timeout\n"
    except Exception as e:
        return f"Fatal error: {str(e)}\n"


def normalize_output(s):
    s = s.replace(".no-opt.nut", ".nut").replace(".opt.nut", ".nut")
    s = re.sub(r"0[xX][0-9a-fA-F]+", "0xHEXNUMBER", s)
    # Remove [this] sections (closure hoisting changes local variable visibility)
    s = re.sub(r"\[this\][^\n]*.*?(?====)", "", s, flags=re.DOTALL)
    return s


_MINIMIZE_TIMEOUT = 60      # seconds per file
_MINIMIZE_CSQ_TIMEOUT = 5   # seconds per csq execution during minimize


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
    """Write lines to temp .nut files (opt + no-opt) and check for divergence.

    If capture_outputs is a dict, store 'opt' and 'noopt' output strings in it.
    """
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
        results[0] = _minimize_run(f"{csq} {csq_args_opt}", opt_path)
    def _run_noopt():
        results[1] = _minimize_run(f"{csq} {csq_args_no_opt}", noopt_path)
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
                # Scan from end to start: dump/print lines at the bottom
                # are removed first, avoiding errors from missing variables
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
        f.write(f"//   {csq} {csq_args_opt} {out_path}\n")
        f.write(f"// Optimizer OFF (add #disable-optimizer at the top):\n")
        f.write(f"//   {csq} {csq_args_no_opt} {out_path}\n")
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


def generate_program(seed):
    """Generate a program and write .opt.nut and .no-opt.nut variants."""
    code_bricks._enum_prefix = f"S{seed}_"
    program = assemble(seed, bricks_per_program)

    prefix = f"cbf{my_process_id}-{seed}"

    separator = r'print("===\n")' + '\n'

    # .opt.nut - optimizer ON (pragma commented out)
    with open(f"{prefix}.opt.nut", "w") as f:
        f.write("//#disable-optimizer\n")
        f.write(program)
        f.write(separator)

    # .no-opt.nut - optimizer OFF (pragma active)
    with open(f"{prefix}.no-opt.nut", "w") as f:
        f.write("#disable-optimizer\n")
        f.write(program)
        f.write(separator)


def _pool_init():
    """Initialize pool worker: ignore SIGINT (main process handles it)."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    code_bricks._excluded_bricks = {"magic_file"}


def _generate_one(args):
    """Generate one program text in a pool worker process."""
    seed, num_bricks = args
    code_bricks._enum_prefix = f"S{seed}_"
    return assemble(seed, num_bricks)


def _write_program_files(seed, program):
    """Write .opt.nut and .no-opt.nut files for a generated program."""
    prefix = f"cbf{my_process_id}-{seed}"
    separator = r'print("===\n")' + '\n'
    with open(f"{prefix}.opt.nut", "w") as f:
        f.write("//#disable-optimizer\n")
        f.write(program)
        f.write(separator)
    with open(f"{prefix}.no-opt.nut", "w") as f:
        f.write("#disable-optimizer\n")
        f.write(program)
        f.write(separator)


def on_error_detected(index, message):
    print(message)
    try:
        nut = open(f"cbf{my_process_id}-{index}.opt.nut", "r").read()
        print(f"===========================\n{nut}\n=========================== (seed = {index})\n")
        if retain_failed_tests:
            failed_path = f"{cwd}/cbf{index}.opt.nut.failed"
            open(failed_path, "w").write(nut)
            if do_minimize:
                _minimize_one(failed_path)
    except FileNotFoundError:
        print(f"(could not read program file for seed {index})")


_timing_lock = threading.Lock()
_total_generate_time = 0.0
_total_execute_time = 0.0
_total_generated = 0
_total_executed = 0


def fuzzer_thread_func(begin, end, step):
    global error_detected, _total_generate_time, _total_execute_time
    global _total_generated, _total_executed
    t0 = time.time()

    seed_list = list(range(begin, end, step))
    files_processed = 0
    print_progress_time = t0 + 2

    for x in range(0, len(seed_list), batch_size):
        if error_detected:
            break

        if begin == 0:
            if time.time() > print_progress_time:
                print_progress_time = time.time() + 10
                remaining = (time.time() - t0) / (files_processed + 0.0001) * (len(seed_list) - files_processed)
                minutes = int(remaining / 60) % 60
                seconds = int(remaining) % 60
                print(f"... {files_processed * step} tests done, remaining time: {minutes}:{seconds:02d}")

        batch = seed_list[x : x + batch_size]

        # Generate programs for this batch (parallel via process pool)
        gen_t0 = time.time()
        if _gen_pool is not None:
            programs = _gen_pool.map(_generate_one,
                                     [(s, bricks_per_program) for s in batch])
            for seed_val, program in zip(batch, programs):
                _write_program_files(seed_val, program)
        else:
            for seed in batch:
                generate_program(seed)
        gen_dt = time.time() - gen_t0

        # Build command lines for batch execution
        cmd_opt = ""
        cmd_no_opt = ""
        for seed in batch:
            prefix = f"cbf{my_process_id}-{seed}"
            cmd_opt += f" {prefix}.opt.nut"
            cmd_no_opt += f" {prefix}.no-opt.nut"

        exec_t0 = time.time()
        accum_no_opt = normalize_output(run_cmd(f"{csq} {csq_args_no_opt} {cmd_no_opt}"))
        accum_opt = normalize_output(run_cmd(f"{csq} {csq_args_opt} {cmd_opt}"))
        accum_csq2 = ""
        if csq2:
            accum_csq2 = normalize_output(run_cmd(f"{csq2} {csq_args_opt} {cmd_opt}"))
        exec_dt = time.time() - exec_t0

        with _timing_lock:
            _total_generate_time += gen_dt
            _total_execute_time += exec_dt
            _total_generated += len(batch)
            _total_executed += len(batch)

        # Check for divergence
        if ("Fatal" in accum_opt or "Fatal" in accum_no_opt or "Fatal" in accum_csq2 or
                accum_opt != accum_no_opt or (csq2 and accum_opt != accum_csq2)):

            print("Error detected in batch run, running scripts one by one...")
            error_detected = True

            if retain_failed_tests:
                open(f"{cwd}/cbf_batch_{x}_no_opt.out", "w").write(accum_no_opt)
                open(f"{cwd}/cbf_batch_{x}_opt.out", "w").write(accum_opt)
                if csq2:
                    open(f"{cwd}/cbf_batch_{x}_csq2.out", "w").write(accum_csq2)

            if "Timeout" in accum_no_opt or "Timeout" in accum_opt or "Timeout" in accum_csq2:
                print(f"Timeout detected in batch, running individually...")
                for seed in batch:
                    prefix = f"cbf{my_process_id}-{seed}"
                    out_no = run_cmd(f"{csq} {csq_args_no_opt} {prefix}.no-opt.nut")
                    out_opt = run_cmd(f"{csq} {csq_args_opt} {prefix}.opt.nut")
                    if "Timeout" in out_no:
                        on_error_detected(seed, f"\nERROR: Seed {seed} timed out without optimizer\n")
                    elif "Timeout" in out_opt:
                        on_error_detected(seed, f"\nERROR: Seed {seed} timed out with optimizer\n")
                continue

            for seed in batch:
                prefix = f"cbf{my_process_id}-{seed}"
                out_no = normalize_output(run_cmd(f"{csq} {csq_args_no_opt} {prefix}.no-opt.nut"))
                out_opt = normalize_output(run_cmd(f"{csq} {csq_args_opt} {prefix}.opt.nut"))
                out_csq2 = ""
                if csq2:
                    out_csq2 = normalize_output(run_cmd(f"{csq2} {csq_args_opt} {prefix}.opt.nut"))

                crash_ver = ""
                if "Fatal" in out_no:
                    crash_ver = "csq, without optimizer"
                elif "Fatal" in out_opt:
                    crash_ver = "csq, with optimizer"
                elif "Fatal" in out_csq2:
                    crash_ver = "csq2"

                if crash_ver:
                    on_error_detected(seed, f"\nERROR: Seed {seed} crashed in {crash_ver}\n")
                elif out_no != out_opt:
                    on_error_detected(seed, f"\nERROR: Seed {seed} produced different output with and without optimizer\n")
                elif csq2 and out_opt != out_csq2:
                    on_error_detected(seed, f"\nERROR: Seed {seed} produced different output with two csq versions\n")

        # Cleanup
        for seed in batch:
            files_processed += 1
            prefix = f"cbf{my_process_id}-{seed}"
            try:
                os.remove(f"{prefix}.no-opt.nut")
                os.remove(f"{prefix}.opt.nut")
            except Exception as e:
                if not error_detected:
                    print(f"ERROR: {str(e)}")
                    error_detected = True


def print_usage():
    print("\nUsage: python run_diff_fuzzer.py <arguments>\n"
          "  --numtests=<N>      - number of tests to run (default 5000)\n"
          "  --bricks=<N>        - bricks per program (default 15)\n"
          "  --csq=<path>        - path to csq-dev.exe\n"
          "  --csq2=<path>       - second csq for cross-version comparison\n"
          "  --singletest=<seed> - run single test with specified seed\n"
          "  --retain            - retain files that caused errors\n"
          "  --minimize          - minimize .failed files (standalone or after fuzzing with --retain)\n"
          "\nExample:\n"
          "  python run_diff_fuzzer.py --numtests=10000 --csq=D:/dagor/tools/dagor_cdk/windows-x86_64/csq-dev.exe --retain\n"
          "  python run_diff_fuzzer.py --singletest=42 --csq=csq-dev.exe --retain\n"
          "  python run_diff_fuzzer.py --minimize --csq=csq-dev.exe\n")
    exit(0)


def check_csq_version(exe_path, var_name):
    full = exe_path
    if os.name == 'nt' and not full.lower().endswith('.exe'):
        full += '.exe'
    try:
        if os.name == 'nt':
            if not os.path.isfile(full):
                full = run_cmd(f'where {full}').split('\n')[0].strip()
            csq_time = time.ctime(os.path.getmtime(full))
            print(f"{var_name} file time: {csq_time}")
        ver = run_cmd(f'{full} --version').replace('\n', '').replace('\r', '').split('.')
        print(f"{var_name} version: {ver[0]}.{ver[1]}.{ver[2]}")
    except Exception:
        print(f"\nERROR: {exe_path} not found, specify with --{var_name}=<path>")
        exit(1)


if __name__ == '__main__':
    begin_test = 0
    end_test = 5000

    if len(sys.argv) == 1:
        print_usage()

    for arg in sys.argv[1:]:
        if arg in ("--help", "-h"):
            print_usage()
        elif arg == "--minimize":
            do_minimize = True
        elif arg.startswith("--singletest="):
            begin_test = int(arg.split("=")[1])
            end_test = begin_test + 1
        elif arg.startswith("--numtests="):
            end_test = int(arg.split("=")[1])
        elif arg.startswith("--bricks="):
            bricks_per_program = int(arg.split("=")[1])
        elif arg == "--retain":
            retain_failed_tests = True
        elif arg.startswith("--csq="):
            csq = os.path.abspath(arg.split("=")[1])
        elif arg.startswith("--csq2="):
            csq2 = os.path.abspath(arg.split("=")[1])
        else:
            print(f"ERROR: Unknown argument: {arg}")
            exit(1)

    check_csq_version(csq, "csq")

    # --minimize alone: just minimize existing .failed files and exit
    if do_minimize and "--numtests=" not in " ".join(sys.argv) and "--singletest=" not in " ".join(sys.argv):
        exit(run_minimize())

    if csq2:
        check_csq_version(csq2, "csq2")

    os.chdir(tempfile.gettempdir())

    cores = min(multiprocessing.cpu_count(), 16)
    _gen_pool = multiprocessing.Pool(cores, initializer=_pool_init)

    num_tests = end_test - begin_test
    is_single = num_tests == 1

    # Warmup runs for large test counts
    warmup_failed = False
    if num_tests > 20 and not is_single:
        for warmup_count in [1, 20]:
            error_detected = False
            print(f"\n--- Warmup: {warmup_count} test(s) ---")
            warmup_end = begin_test + warmup_count
            # Run warmup single-threaded
            fuzzer_thread_func(begin_test, warmup_end, 1)
            if error_detected:
                print(f"Warmup ({warmup_count} tests) FAILED, aborting.")
                warmup_failed = True
                break
            print(f"Warmup ({warmup_count} tests) OK")

        if not warmup_failed:
            # Reset for full run
            error_detected = False

    if not warmup_failed:
        print(f"\nRunning code_bricks diff fuzzer with {num_tests} tests, {bricks_per_program} bricks/program")
        print(f"Temporary directory: {tempfile.gettempdir()}")

        wall_t0 = time.time()
        threads = []
        for i in range(cores):
            t = threading.Thread(target=fuzzer_thread_func,
                                 args=(i + begin_test, end_test, cores))
            t.start()
            threads.append(t)

        for t in threads:
            t.join()

        wall_time = time.time() - wall_t0
        print(f"\n--- Timing ---")
        print(f"Wall time:          {wall_time:.2f}s")
        print(f"Generate (sum):     {_total_generate_time:.2f}s ({_total_generated} programs, "
              f"{_total_generate_time / max(_total_generated, 1) * 1000:.1f} ms/program)")
        print(f"Execute  (sum):     {_total_execute_time:.2f}s ({_total_executed} programs, "
              f"{_total_execute_time / max(_total_executed, 1) * 1000:.1f} ms/program)")

        if os.getcwd() != tempfile.gettempdir():
            print("\nERROR: Unexpected directory change during execution, cwd =", os.getcwd())

    _gen_pool.close()
    _gen_pool.join()

    os.chdir(cwd)

    if error_detected:
        exit(1)
    else:
        print("Fuzzer: OK")
        exit(0)
