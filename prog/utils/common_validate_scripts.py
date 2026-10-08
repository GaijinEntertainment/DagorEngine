import json
import sys, os, subprocess
import multiprocessing, multiprocessing.pool
import tempfile

def read_dreyconfig(nutFilePath):
  dirPath = os.path.dirname(nutFilePath)
  dreyConfigPath = None
  for i in range(10):
    tryPath = os.path.join(dirPath, ".dreyconfig")
    if os.path.exists(tryPath):
      dreyConfigPath = tryPath
      break
    dirPath += "/.."

  if not dreyConfigPath or not os.path.exists(dreyConfigPath):
    return None

  print("Reading .dreyconfig from", dreyConfigPath)

  with open(dreyConfigPath) as f:
    try:
      j = json.load(f)
      return j
    except json.JSONDecodeError as e:
      print("ERROR: While reading .dreyconfig:", e)
      sys.exit(1)

  return None


def get_validator_cmds(commands, csq, filesListList, do_analysis):
  res = []
  for cmdcfg in commands:
    platforms = cmdcfg.get("platforms", ["_undefined_"])
    args = cmdcfg.get("args",[])
    assert type(args) in (list, tuple, set)
    configs = cmdcfg.get("configs", [f"-platform:{platform}" for platform in platforms])
    wdir = cmdcfg.get('wdir')
    cmd = cmdcfg["script"]

    dreyConfig = read_dreyconfig(cmd) if do_analysis else None
    disabledWarnings = dreyConfig.get("__disabled_checks", []) if dreyConfig else []
    disabledWarnings = [f"--D:{w}" for w in disabledWarnings]

    for cfg in configs:
      fcmd = [csq, cmd]

      if do_analysis:
        fileList = tempfile.NamedTemporaryFile(delete=False)
        filesListList.append(fileList.name)
        fcmd.append("--static-analysis")
        fcmd.extend(disabledWarnings)
        fcmd.append(f"--visited-files-list:{fileList.name}")

      if isinstance(cfg, str):
        fcmd.extend(["--absolute-path", cfg])
      else:
        fcmd.extend(["--absolute-path"])
        fcmd.extend(list(cfg))

      for a in args:
        fcmd.append(a)
      if wdir:
        res.append({'wdir':wdir, 'cmd':fcmd})
      else:
        res.append(fcmd)

  return res

def check(cmd):
  print(cmd)
  wdir = None
  if not isinstance(cmd, list):
    wdir =cmd['wdir']
    cmd = cmd['cmd']
  try:
    if wdir:
      r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, errors='ignore', timeout=70, cwd=wdir) #encoding='utf-8',
    else:
      r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, errors='ignore', timeout=70) #encoding='utf-8',
    return (r.returncode==0, cmd, r.stdout)

  except subprocess.TimeoutExpired as e:
    return (False, cmd, f"{type(e).__name__}: {str(e)}")

def getAllNutFileNames(path):
  result = []
  for root, dirs, files in os.walk(path):
    for file in files:
        if file.endswith('.nut'):
            fullName = os.path.abspath(os.path.join(root, file))
            result.append(fullName)
  return result

def computeCollectionDiff(actual, expected):
  result = []
  for file in expected:
    if file not in actual:
      result.append(file)
  return result

def computeFilesDiff(filesListList):
  actualFileSet = set()
  for filesList in filesListList:
    with open(filesList) as list:
      for line in list:
        actualFileSet.add(line.rstrip())
  directoryFiles = getAllNutFileNames(".")
  return computeCollectionDiff(actualFileSet, directoryFiles)


def _test(commands, analize_commands, use_builtin_analyzer, fail_unvisited):
  dagor_cdk_path = os.path.abspath(os.path.join(__file__, "../../../tools/dagor_cdk"))
  if sys.platform == "win32" :
    csq = os.path.join(dagor_cdk_path, "windows-x86_64", "csq-dev.exe")
  elif sys.platform.startswith("linux") :
    csq = os.path.join(dagor_cdk_path, "linux-x86_64", "csq-dev")
  else :
    print("ERROR: (validateScripts) unknown platform: " + sys.platform)
    sys.exit(1)
  print(dagor_cdk_path, csq)
  print("csq version: " + subprocess.run([csq, "--version"], stdout=subprocess.PIPE).stdout.decode('utf-8'))
  os.environ["QUIRREL_SCRIPTS_TESTS"] = "true"
  multiprocessing.freeze_support()
  filesListList = []
  cmds = get_validator_cmds(commands, csq, filesListList, use_builtin_analyzer)

  poolSize = min(multiprocessing.cpu_count(), len(cmds), 60 if sys.platform == 'win32' else 1000) # details about 60 - https://stackoverflow.com/a/65290362
  print("CPU count", multiprocessing.cpu_count(), "poolSize", poolSize)
  # We are executing system commands, no reason to use actual processes for that (hence ThreadPool instead of Pool). Also it interrupts better
  pool = multiprocessing.pool.ThreadPool(processes = poolSize)

  res = pool.map_async(check, cmds)
  pool.close()
  success = []
  failed = []
  try:
    for r in res.get():
      if not r[0]:
        failed.append(f'{r[1]}\n\n{r[2]}')
      else:
        success.append(f'{r[1]}\n\n{r[2]}')
  finally:
    pool.join()
  if len(success)>0:
    print("SUCCESS:")
    for r in success:
      print(r)
  print("")

  testFailed = False

  if len(failed)>0:
    print("FAILED:")
    for r in failed:
      try:
        print(r)
      except UnicodeEncodeError:
        print("UnicodeEncodeError")
        try:
          print(bytes(r, encoding="utf-8"))
        except:
          print("can't print error")
          raise
    print("total failed:", len(failed))
    testFailed = True

  if use_builtin_analyzer:
    checkUnvisited = fail_unvisited and not testFailed
    if checkUnvisited:
      unvisitedFiles = computeFilesDiff(filesListList)
    for file in filesListList:
      try:
        os.remove(file)
      except OSError as e:
        print(f"Failed to remove tmp file {file}")
    if checkUnvisited:
      if len(unvisitedFiles) > 0:
        print("There are unvisited files")
        for file in unvisitedFiles:
          print(f"  {file}")
        print("FAILED: There are unvisited files. Make sure all files are being involved into CSQ verification")
        testFailed = True

  if testFailed:
    sys.exit(1)
  else:
    print("DONE.")


def test_csq_analyzer(commands, fail_unvisited = False):
  _test(commands, [], True, fail_unvisited)
def test_separate_analyzer(commands, analyze_commands, fail_unvisited = False):
  _test(commands, analyze_commands, False, fail_unvisited)
