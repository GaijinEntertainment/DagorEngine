import sys
import os


def collect_dng_libs(argsList: list[list[str]], dngScriptsPath: str, dngScriptList: list[str], dngScriptArgs: list[str]) -> bool:
    resolvedDngScriptList = []
    for root, dirs, files in os.walk(dngScriptsPath):
        for filename in files:
            if filename in dngScriptList:
                resolvedDngScriptList.append(os.path.join(root, filename))
                dngScriptList.remove(filename)

    if len(dngScriptList) > 0:
        print(f"FAILED: dngScriptList contains incorrect paths: {dngScriptList}")
        return False

    argsList.extend(
        [[script] + dngScriptArgs for script in resolvedDngScriptList])

    return True


def validate_scripts(executable_path: str, argsList: list[list[str]]) -> bool:
    executable = os.path.normpath(executable_path)

    for args in argsList:
        print(f"Running: {args[0]}")
        cmd = f'{executable} {" ".join(args)}'
        if os.system(cmd) != 0:
            print(f"FAILED script {args[0]}")
            return False
    return True


def test_all(executable: str, argsList: list[list[str]], dngScriptsPath: str, dngScriptList: list[str], dngScriptArgs: list[str]) -> None:
    if collect_dng_libs(argsList, dngScriptsPath, dngScriptList, dngScriptArgs) and validate_scripts(executable, argsList):
        print("SUCCESS!")
    else:
        print("FAILED!")
        sys.exit(1)