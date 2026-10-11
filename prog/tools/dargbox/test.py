import os
import sys
import subprocess
import multiprocessing

# exe lives in a per-arch subdir of tools/dargbox (see prog/tools/dargbox/jamfile);
# all paths here are relative to tools/dargbox, the cwd when dargbox is run
def get_dargbox_exe():
  arch = (os.environ.get("PROCESSOR_ARCHITEW6432") or os.environ.get("PROCESSOR_ARCHITECTURE", "")).upper()
  # arm64 hosts can run the x86_64 exe through emulation; the reverse cannot work
  arch_dirs = ["windows-arm64", "windows-x86_64"] if arch == "ARM64" else ["windows-x86_64"]
  for d in arch_dirs:
    exe = os.path.join(d, "dargbox-dev.exe")
    if os.path.exists(exe):
      return exe
  return os.path.join(arch_dirs[0], "dargbox-dev.exe")

def hidden_window_startupinfo():
  si = subprocess.STARTUPINFO()
  si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
  si.wShowWindow = subprocess.SW_HIDE
  return si

def gather_files_to_check(path, relative="", cwd=None):
  files2check=[]
  for root,dirs,files in os.walk(path):
    dirs[:] = [d for d in dirs if d not in ["zbugs"]]
    files[:] = [f for f in files if not f in ["images_advanced.ui.nut","svg.ui.nut","input.ui.nut","robj_shader.ui.nut","nu_pogodi.ui.nut", "all_ui.ui.nut"] and f.endswith(".nut")]
    for file in files:
      path = os.path.join(root,file).replace("\\","/")
      files2check.append({"rel":os.path.relpath(path, relative), "direct":path, "cwd":cwd})
  return files2check

# One render frame is enough to smoke-check a scene: it catches a load error or a
# logerr. A test that has to drive several act frames - scrolling, focus moves -
# names its own settings here and calls exit() itself, non-zero when it failed.
# limit_updates counts RENDER frames, so it is only a backstop against a hang.
# Every subprocess a worker starts is bounded here, so the pool below needs no
# deadline of its own: a script error does not stop dargbox, which would
# otherwise run to that backstop, and csq executes the module it checks.
DARG_TIMEOUT_SEC = 30
CSQ_TIMEOUT_SEC = 60

MULTI_FRAME_TESTS = {
  "samples_prog/benchmarks/test_virtual_list_focus.ui.nut": {"limit_updates": 200000, "act_rate": 1000},
  "samples_prog/benchmarks/test_virtual_list_horiz.ui.nut": {"limit_updates": 200000, "act_rate": 1000},
  "samples_prog/benchmarks/test_virtual_list_keepfocus.ui.nut": {"limit_updates": 200000, "act_rate": 1000},
  "samples_prog/benchmarks/test_virtual_list_padding.ui.nut": {"limit_updates": 200000, "act_rate": 1000},
  "samples_prog/benchmarks/test_touch_pad_gestures.ui.nut": {"limit_updates": 200000, "act_rate": 1000},
}

def check(file_info):
  cfg = MULTI_FRAME_TESTS.get(file_info["rel"].replace("\\", "/"), {"limit_updates": 1, "act_rate": 0})
  cmd_darg = '{exe} -quiet -silent -config:script:t={file} -config:debug/profiler:t=off -config:debug/limit_updates:i={limit} -config:video/driver:t="stub" -fatals_to_stderr -logerr_to_stderr -config:workcycle/act_rate:i={rate} -config:debug/useAddonVromSrc:b=yes -config:debug/fatalOnLogerrOnExit:b=no'.format(exe=get_dargbox_exe(), file=file_info["rel"], limit=cfg["limit_updates"], rate=cfg["act_rate"])
  cmd_sq = '..\\dagor_cdk\\windows-x86_64\\csq-dev.exe {file}'.format(file=file_info["rel"])
  failedBy = ""
  failText = ""
  try:
    result = subprocess.run(cmd_sq, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=file_info["cwd"], timeout=CSQ_TIMEOUT_SEC)
    if result.returncode != 0:
      failedBy = "csq"
      failText = failText + result.stdout + "\n" + result.stderr + "\n"
  except subprocess.TimeoutExpired:
    failedBy = "csq"
    failText = failText + "timed out after {}s".format(CSQ_TIMEOUT_SEC)

  if file_info["rel"].endswith(".ui.nut"):
    try:
      result = subprocess.run(cmd_darg, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=DARG_TIMEOUT_SEC,
                              startupinfo=hidden_window_startupinfo())
      if result.returncode != 0:
        failedBy = "dargbox"
        failText = failText + result.stdout + "\n" + result.stderr + "\n"
    except subprocess.TimeoutExpired:
      failedBy = "dargbox"
      failText = failText + "timed out after {}s without exiting".format(DARG_TIMEOUT_SEC)

  return {
    "success": (failedBy == ""),
    "fileInfo": file_info,
    "failedBy": failedBy,
    "failText": failText.strip()
  }


def rerun_dargbox(dargboxFailedScript):
  cmd_darg = f'{get_dargbox_exe()} -quiet -silent -config:script:t={dargboxFailedScript} -config:debug/profiler:t=off -config:debug/limit_updates:i=1 -config:video/driver:t="stub" -config:workcycle/act_rate:i=0 -config:debug/useAddonVromSrc:b=yes -config:debug/fatalOnLogerrOnExit:b=no'
  subprocess.run(cmd_darg, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, startupinfo=hidden_window_startupinfo())
  with open(".log/last_debug", "r") as f:
    debugDir = f.read().strip()
  if os.path.exists(debugDir + "/fatalerr"):
    print("\ndargbox fatalerr:\n")
    with open(debugDir + "/fatalerr", "r") as f:
      print(f.read())
  if os.path.exists(debugDir + "/logerr"):
    print("\ndargbox logerr:\n")
    with open(debugDir + "/logerr", "r") as f:
      print(f.read())
  print("\n")


if __name__ == "__main__":
  oldir = os.getcwd()
  files = gather_files_to_check("gamebase/samples_prog", "gamebase", os.path.join(oldir, "gamebase"))
  print("files to check = {}".format(len(files)))
  os.chdir(os.path.normpath("../../../tools/dargbox"))

  version_cmd = f'..\\dagor_cdk\\windows-x86_64\\csq-dev.exe --version'
  try:
    print("csq version:")
    subprocess.check_call(version_cmd, shell=True, stderr=subprocess.STDOUT)
  except subprocess.CalledProcessError:
    print("ERROR: csq not found")
    sys.exit(1)

  multiprocessing.freeze_support()
  pool = multiprocessing.Pool(max(min(multiprocessing.cpu_count(), 8), 1))
  res = pool.map_async(check, files)
  success = []
  failed = []
  dargboxFailedScript = ""
  for r in res.get():
    if not r["success"]:
      failedBy = r["failedBy"]
      failed.append(r["fileInfo"]["direct"] + "\n" + r["failText"] + "\n")
      if failedBy == "dargbox":
        dargboxFailedScript = r["fileInfo"]["rel"]
    else:
      success.append(r["fileInfo"]["direct"])

  if len(success)>0:
    print("SUCCESS:")
    for fn in success:
      print(fn)
  print("")

  if len(failed)>0:
    if dargboxFailedScript != "":
      print("rerun dargbox for failed script")
      rerun_dargbox(dargboxFailedScript)

    print("FAILED:")
    for fn in failed:
      print(fn)

  os.chdir(oldir)
  sys.exit(1 if len(failed)>0 else 0)
