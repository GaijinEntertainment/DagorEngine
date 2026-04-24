import os
import sys
import shutil
import subprocess
from datetime import datetime


def run_test(cdk_arg, dcmd, output_files=[]):
    test_failed = False
    prev_dir = os.getcwd()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    tools_dir = os.path.abspath(os.path.join(script_dir, "..", "..", "..", "tools", f"{cdk_arg}CDK"))
    os.chdir(tools_dir)

    for output_file in output_files:
        if os.path.exists(output_file):
            os.remove(output_file)

    subprocess.call(["cmd.exe", "/c", "setup.cmd"])

    dag_cdk_loc = os.path.join("dagor_cdk", ".local")
    target_dir = os.path.abspath(os.path.join("..", dag_cdk_loc))
    os.makedirs(target_dir, exist_ok=True)

    src_blk = os.path.join(dag_cdk_loc, "workspace.blk")
    dst_blk = os.path.join("..", dag_cdk_loc, "workspace.blk")
    shutil.copy2(src_blk, dst_blk)

    print(datetime.now())

    cmd = [
        os.path.abspath(os.path.join("..", "dagor_cdk", "windows-x86_64", "assetViewer2-dev.exe")),
        "application.blk",
        "-quiet",
        "-fatals_to_stderr",
        "-logerr_to_stderr",
        f"-ws:{cdk_arg}CDK",
        f"-async_batch:{os.path.join(script_dir, 'Tests', dcmd)}"
    ]
    print(cmd)
    result = subprocess.run(cmd)

    if result.returncode != 0:
        test_failed = True

    print(datetime.now())

    if not test_failed:
        for output_file in output_files:
            if not os.path.exists(output_file):
                print(f"No such file exists: {output_file}")
                test_failed = True
                break

    os.chdir(prev_dir)
    if test_failed:
        sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python test_script.py <CDK prefix>")
        sys.exit(1)

    run_test(sys.argv[1], 'screenshot.dcmd', ["av_scrn_001.jpg"])
    run_test(sys.argv[1], 'assets.dcmd')
    run_test(sys.argv[1], 'editor.dcmd')

