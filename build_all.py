import os
import sys
import subprocess

if sys.platform.startswith('win'):
    script = "cmd build_all.cmd"
elif sys.platform.startswith('darwin'):
    script = "bash build_all_macOS.sh"
elif sys.platform.startswith('linux'):
    script = "bash build_all_linux.sh"
try:
    print(f"Running { script }...")
    os.system(script)
except subprocess.CalledProcessError as e:
    print("subprocess.run failed with a non-zero exit code. Error: {0}".format(e))
except OSError as e:
    print("An OSError occurred, subprocess.run command may have failed. Error: {0}".format(e))