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
  cmd_sq = '..\\..\\..\\..\\tools\\dagor3_cdk\\util64\\csq-dev.exe {file}'.format(file=file_info["rel"])
  try:
    if file_info["rel"].endswith(".ui.nut"):
      subprocess.check_call(cmd_darg, shell=True, stderr=subprocess.STDOUT)
    subprocess.check_call(cmd_sq, shell=True, stderr=subprocess.STDOUT, cwd=file_info["cwd"])
  except subprocess.CalledProcessError:
    return (False, "{file}".format(file=file_info["direct"]))
  return (True, "{file}".format(file=file_info["direct"]))

if __name__ == "__main__":
  oldir = os.getcwd()
  files = gather_files_to_check("gamebase/samples_prog", "gamebase", os.path.join(oldir, "gamebase"))
  print("files to check = {}".format(len(files)))
  os.chdir(os.path.normpath("../../../tools/dargbox"))
  multiprocessing.freeze_support()
  pool = multiprocessing.Pool(max(min(multiprocessing.cpu_count(), 8), 1))
  res = pool.map_async(check, files)
  success = []
  failed = []
  for r in res.get(timeout=60):
    if not r[0]:
      failed.append(r[1])
    else:
      success.append(r[1])
  os.chdir(oldir)
  if len(success)>0:
    print("SUCCESS:")
    for r in success:
      print(r)
  print("")
  if len(failed)>0:
    print("FAILED:")
    for r in failed:
      print(r)
    sys.exit(1)
