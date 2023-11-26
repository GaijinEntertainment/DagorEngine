import os
import sys
import subprocess
import multiprocessing

def gather_files_to_check(path, relative="", cwd=None):
  files2check=[]
  for root,dirs,files in os.walk(path):
    dirs[:] = [d for d in dirs if d not in ["zbugs"]]
    files[:] = [f for f in files if not f in ["images_advanced.ui.nut","svg.ui.nut","svg_advanced.ui.nut","input.ui.nut"] and f.endswith(".nut")]
    for file in files:
      path = os.path.join(root,file).replace("\\","/")
      files2check.append({"rel":os.path.relpath(path, relative), "direct":path, "cwd":cwd})
  return files2check

def check(file_info):
  cmd_darg = 'dargbox-64-dev.exe -quiet -silent -config:script:t={file} -config:debug/profiler:t=off -config:debug/limit_updates:i=1 -config:video/driver:t="stub" -fatals_to_stderr -logerr_to_stderr -config:workcycle/act_rate:i=0 -config:debug/useVromSrc:b=yes -config:debug/fatalOnLogerrOnExit:b=no'.format(file=file_info["rel"])
  cmd_sq = '..\\dagor3_cdk\\util64\\csq-dev.exe {file}'.format(file=file_info["rel"])
  failedBy = ""
  failText = ""
  result = subprocess.run(cmd_sq, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=file_info["cwd"])
  if result.returncode != 0:
    failedBy = "csq"
    failText = failText + result.stdout + "\n" + result.stderr + "\n"

  if file_info["rel"].endswith(".ui.nut"):
    result = subprocess.run(cmd_darg, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
      failedBy = "dargbox"
      failText = failText + result.stdout + "\n" + result.stderr + "\n"

  return (failedBy == "", "{file}".format(file=file_info["direct"]), failedBy, failText.strip())


def rerun_dargbox(dargboxFailedScript):
  cmd_darg = f'dargbox-64-dev.exe -quiet -silent -config:script:t={dargboxFailedScript} -config:debug/profiler:t=off -config:debug/limit_updates:i=1 -config:video/driver:t="stub" -config:workcycle/act_rate:i=0 -config:debug/useVromSrc:b=yes -config:debug/fatalOnLogerrOnExit:b=no'
  subprocess.run(cmd_darg, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
  with open(".logs/last_debug", "r") as f:
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

  version_cmd = f'..\\dagor3_cdk\\util64\\csq-dev.exe --version'
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
  for r in res.get(timeout=60):
    if not r[0]:
      failedBy = r[2]
      failedText = r[3]
      failed.append(r[1] + "\n" + failedText + "\n")
      if failedBy == "dargbox":
        dargboxFailedScript = r[1]
    else:
      success.append(r[1])
  if len(success)>0:
    print("SUCCESS:")
    for r in success:
      print(r)
  print("")

  if len(failed)>0:
    if dargboxFailedScript != "":
      print("rerun dargbox for failed script")
      rerun_dargbox(dargboxFailedScript)

    print("FAILED:")
    for r in failed:
      print(r)

  os.chdir(oldir)
  sys.exit(1 if len(failed)>0 else 0)
