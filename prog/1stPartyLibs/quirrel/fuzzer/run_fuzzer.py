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
csq = "csq"
csq2 = ""
cwd = os.getcwd().replace("\\", "/")
retain_failed_tests = False
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
    return s


def on_error_detected(index, message):
    print(message)
    nut = open(f"sqf{my_process_id}-{index}.opt.nut", "r").read()
    print(f"===========================\n{nut}\n=========================== (seed = {index})\n")
    if retain_failed_tests:
        open(f"{cwd}/sqf{index}.opt.nut.failed", "w").write(nut)


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
        run_cmd(f"{csq} {cwd}/sqfuzzer.nut -- seedset={seed_set} fileprefix=sqf{my_process_id}-")

        # run all scripts at once
        cmd_opt = ""
        cmd_no_opt = ""
        for i in seed_list[x : x + batchSize]:
            cmd_opt += f" sqf{my_process_id}-{i}.opt.nut"
            cmd_no_opt += f" sqf{my_process_id}-{i}.no-opt.nut"

        accum_output_no_opt = normalize_output(run_cmd(f"{csq}{cmd_no_opt}"))
        accum_output_opt = normalize_output(run_cmd(f"{csq}{cmd_opt}"))
        accum_output_csq2 = ""
        if csq2 != "":
            accum_output_csq2 = normalize_output(run_cmd(f"{csq2}{cmd_opt}"))

        # if any of the scripts crashed or produced different output, then run scripts one by one
        if ("Fatal" in accum_output_opt or "Fatal" in accum_output_no_opt or "Fatal" in accum_output_csq2 or
                accum_output_opt != accum_output_no_opt or (csq2 != "" and accum_output_opt != accum_output_csq2)):
            print("Error detected in batch run, running scripts one by one...")
            error_detected = True
            for i in seed_list[x : x + batchSize]:
                output_disabled_opt = normalize_output(run_cmd(f"{csq} sqf{my_process_id}-{i}.no-opt.nut"))
                output_enabled_opt = normalize_output(run_cmd(f"{csq} sqf{my_process_id}-{i}.opt.nut"))
                output_csq2 = ""
                if csq2 != "":
                    output_csq2 = normalize_output(run_cmd(f"{csq2} sqf{my_process_id}-{i}.opt.nut"))

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
          "  --retain - retain files that caused errors\n"
          "\nExample:\n" +
          "  python run_fuzzer.py --numtests=10000 --csq=D:/dagor/tools/dagor_cdk/windows-x86_64/csq-dev.exe --retain\n" +
          "  python run_fuzzer.py --singletest=12345 --csq=csq-dev.exe --csq2=csq-old-dev.exe --retain\n"
          )
    exit(0)


def check_csq_version(exe_path, var_name):
    try:
        csqVersion = run_cmd(f'{exe_path} --version').replace('\n', '').replace('\r', '').split('.')
        if int(csqVersion[0]) * 1e6 + int(csqVersion[1]) * 1e3 + int(csqVersion[2]) < 1e6 + 0 * 1e3 + 23:
            print(f"\nERROR: {exe_path} version must be 1.0.23 or higher, current version: {csqVersion[0]}.{csqVersion[1]}.{csqVersion[2]}")
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
