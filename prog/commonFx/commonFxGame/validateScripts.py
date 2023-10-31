import sys, os, subprocess

dagor_cdk_path = os.path.abspath(os.path.join(__file__, "../../../../tools/dagor3_cdk"))
if sys.platform == "win32" :
  csq = os.path.join(dagor_cdk_path, "util", "csq-dev.exe")
elif sys.platform == "linux" :
  csq = os.path.join(dagor_cdk_path, "util-linux64", "csq-dev")

def check(cmd):
  print(cmd)
  return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, errors='ignore', timeout=70) #encoding='utf-8',
#    return (r.returncode==0, cmd, r.stdout)
#  except subprocess.TimeoutExpired as e:
#    return (False, cmd, f"{type(e).__name__}: {str(e)}")

def main():
  files = [f for f in os.listdir() if f.endswith(".nut")]
  for f in files:
    check([csq, "--static-analysis", f])
if __name__ == "__main__":
  main()