from __future__ import absolute_import
import argparse
import fnmatch
from functools import partial
import json
import os
import sys
import re
import subprocess
import tempfile
from fs_helpers import decode_fname

PY3K = sys.version_info >= (3, 0)


def shell(cmd):
    """do command in shell (like os.system() but with capture output)"""
    cmdshow = cmd
    out = ''
    try:
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE, shell=True, cwd=None)
        out = process.communicate()[0].replace(b'\r\n', b'\n')
    except Exception as err:
        raise RuntimeError(cmd, str(err), -1)
    if process.returncode != 0:
        raise RuntimeError(cmd, out, process.returncode)
    return out

def print_results(failed):
    def print_error(e):
        ee = e.args[1]
        ee = ee.split(b'\n')
        ee = [decode_fname(i) for i in ee]
        ret = ""
        ret = "executing: {0}, get ERROR:".format(str(e.args[0]))
        print(ret)
        for l in ee:
            try:
              print(l)
            except Exception:
              print(l.encode("utf-8"))

    if len(failed) > 0:
        for f in failed:
            print(os.path.join(f[1], f[0]) + "\n")
            print_error(f[2])
        print("FAILED TESTS: {0}".format(len(failed)))
    else:
        print("SUCCESS")

class ExtendAction(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
                items = getattr(namespace, self.dest) or []
                items.extend(values)
                setattr(namespace, self.dest, items)

_jpaths = os.path.join
__utilDir = "windows-x86_64" if os.name == 'nt' else "linux-x86_64"
__CUR_PATH = os.path.dirname(__file__)
_DAGOR2_DIR = os.path.abspath(_jpaths(os.path.dirname(__file__), "..", ".."))
csq_path = os.path.join(_DAGOR2_DIR, "tools", "dagor_cdk", __utilDir, "csq-dev")
csq_args = ["--static-analysis", "--absolute-path"]


def print_csq_version():
    out = shell("{csq_path} --version".format(csq_path=csq_path))
    print("csq version: {out}\n".format(out=out))


def find_config_path(path, startpath="", config_name=".dreyconfig"):
    def rfind(path):
        configpath = _jpaths(path, config_name)
        if os.path.exists(configpath) and os.path.isfile(configpath):
            return configpath
        elif len(os.path.split(path)[1])>0:
            return rfind(os.path.split(path)[0])
        else:
            return None

    abspath = os.path.abspath(path)
    return rfind(abspath)

def loadconfig(path):
    if path is None:
        return None
    with open(path) as data_file:
        config = json.load(data_file)
        config["path_to_this_config"] = os.path.dirname(path)
        return config


def gather_files_in_predefinition_paths(config, config_path):
    if config is None:
        return []
    out = []
    predefinition_paths = config.get("predefinition_paths", [])
    if len(predefinition_paths) == 0:
        return []
    for pp in predefinition_paths:
        combined_path = os.path.normpath(os.path.join(config_path, pp))
        if combined_path.endswith(".nut"):
            out.append(combined_path)
        else:
            for root, dirs, files in os.walk(combined_path):
                out.extend([os.path.join(root, f) for f in files if f.endswith(".nut")])
    return out

def make_fast_definition(config, config_path):
    const_re = re.compile(r"global\sconst\s+[\w|\d_]+\s*\=\s*.*\n")
    vars_re = re.compile(r"(\:\:[\w|\d_]+\s?\<\-)(\s*.+\n)")
    enums_re = re.compile(r"(global\s*enum\s*[\w|\d_]+([\s\n]?|[/\w\s\.\,]+)\{[\@\w\s\,\d\=\n/\"\#\.\(\)\~\+\-]*\})")
    res = []
    files_list = gather_files_in_predefinition_paths(config, config_path)
    if len(files_list) == 0:
        return None
    for file in files_list:
        with open(file, "rt", encoding = "utf-8") as f:
            r = f.read()
            res.extend(const_re.findall(r))
            for i in enums_re.findall(r):
                res.append(i[0])
            for i in vars_re.findall(r):
                if "function" in i[1]:
                    res.append(i[0]+" @(...) null")
                elif "{" in i[1]:
                    res.append(i[0]+" {}")
                elif "[" in i[1]:
                    res.append(i[0]+" []")
                elif "class" in i[1]:
                    res.append(i[0]+" class{}")
                else:
                  res.append(i[0]+" null")
    res_file = tempfile.NamedTemporaryFile(mode = "wt", delete = False)
    res_file.write("// warning disable: -file:egyptian-braces\n")
    res_file.write("\n".join(res))
    res_file.close()
    list_file = tempfile.NamedTemporaryFile(mode = "wt", delete = False)
    list_file.write(res_file.name)
    list_file.close()
    return list_file.name

def make_temp_predefinition_paths(config, config_path):
    if config is None:
        return None

    predefinition_paths = config.get("predefinition_paths", [])
    if len(predefinition_paths) == 0:
        return None

    list_file = tempfile.NamedTemporaryFile(mode = "wt", delete = False)
    print("Config Path: {0}\n".format(config_path))
    out = []
    for pp in predefinition_paths:
        combined_path = os.path.normpath(os.path.join(config_path, pp))
        print("Collecting predefinition files at {0}".format(combined_path))
        if combined_path.endswith(".nut"):
            out.append(combined_path)
        else:
            for root, dirs, files in os.walk(combined_path):
                out.extend([os.path.join(root,f) for f in files if f.endswith(".nut")])
    list_file.write("\n".join(out))
    list_file.close()
    return list_file.name


def checkpath_by_config(config, config_path, p, root, isFile):
    if config is None and isFile:
        return False
    if config is not None and not config.get("enabled", True):
        return False
    if config is None and not isFile:
        config = {}
    dirs_to_skip = config.get("dirs_to_skip",[])
    files_to_skip = config.get("files_to_skip",[])
    if len(dirs_to_skip)+len(files_to_skip) == 0:
        return True
    path_rel_to_config = os.path.relpath(os.path.abspath(_jpaths(root, p)), os.path.dirname(config_path))
    path_rel_to_config = os.path.normpath(path_rel_to_config)
    dirs = os.path.dirname(path_rel_to_config).split(os.path.sep) if isFile else path_rel_to_config.split(os.path.sep)
    for pattern in dirs_to_skip:
        if len(fnmatch.filter(dirs, pattern))>0:
            return False
    if isFile:
        for pattern in files_to_skip:
            if fnmatch.fnmatch(os.path.basename(path_rel_to_config).lower(), pattern):
                return False
    return True

def get_all_files_by_extensions_and_dirs(files, dirs, extensions, exclude_dirs=["CVS",".git"], use_configs = True):
    exclude_dirs[:] = [os.path.normpath(os.path.abspath(e) + os.sep) for e in exclude_dirs]
    def check_file_excluded(file):
        abspath = os.path.abspath(file)
        return any([abspath.startswith(e) for e in exclude_dirs])

    def check_file_ext(file, extensions):
        textensions = tuple(extensions) if isinstance(extensions, list) else extensions
        return file.lower().endswith(textensions)

    collection = {}
    for f in files:
        assert os.path.isfile(f)
        configpath = find_config_path(os.path.dirname(f))
        config = loadconfig(configpath) if use_configs else {}
        collection.update({os.path.normpath(f):config})
    for path in dirs:
        for root, dirs, ffiles in os.walk(path):
            configpath = find_config_path(root)
            config = loadconfig(configpath) if use_configs else {}
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            dirs[:] = [d for d in dirs if checkpath_by_config(config, configpath, d, root, False)]
            ffiles[:] = [f for f in ffiles if check_file_ext(f, extensions)]
            ffiles[:] = [f for f in ffiles if checkpath_by_config(config, configpath, f, root, True)]
            ffiles[:] = [f for f in ffiles if not check_file_excluded(os.path.join(root, f))]
            pathfiles    = [os.path.abspath(os.path.normpath(os.path.join(root, f))) for f in ffiles]
            [collection.update({file:config}) for file in pathfiles]
    return collection

def gather_files(paths, excluded_dirs, use_configs = True):
    files = [f for f in paths if os.path.isfile(f)]
    dirs = [f for f in paths if os.path.isdir(f) ]
    assert (len(files)+len(dirs)) == len(paths), "inexisting path!"
    excluded_dirs.extend(["CVS",".git"])
    res = get_all_files_by_extensions_and_dirs(files, dirs, extensions = (".nut"), exclude_dirs = excluded_dirs, use_configs = use_configs)
    return res

nametonummap = {}
def tidtocmd(tid):
    return "--W:{0}".format(nametonummap.get(tid, tid))

def shell_test(cmd, extras=""):
    try:
        shell("{cmd} {extras}".format(cmd=cmd, extras=extras))
        return True
    except RuntimeError as e:
        return ("", "", e)

def run_tests_on_files(files, cmds):
    totalFailed = []
    for item in files:
        failed = shell_test(
            "{0} {1} {2}".format(csq_path, cmds if isinstance(cmds, str) else " ".join(cmds), item), ""
        )
        failed = [] if failed == True else [failed]
        totalFailed.extend(failed);
    print_results(totalFailed)
    return totalFailed


def check_files(files, cmdchecks = [], inverse_warnings = True, **kwargs):
    numtoname = dict(zip(nametonummap.values(), nametonummap.keys()))
    namedchecks = [nametonummap.get(k,numtoname.get(k, "--W:{0}".format(k))) for k in cmdchecks]
    print("Check squirrel files")
    if inverse_warnings:
        print("Checks: {0}".format(" ".join(namedchecks)))
    else:
        if len(namedchecks) > 0:
            print("Disabled checks: {0}".format(" ".join(namedchecks) if namedchecks else "None"))
    cmdchecks = [tidtocmd(c) for c in cmdchecks]
    print("Checking files:\n    {0}{1}".format("\n    ".join(list(files.keys())[:10]), "\n        and {0} more".format(len(files)-10) if len(files) >10 else ""))

    extracmds = " ".join([ "--{0}:{1}".format(key.replace("_","-"),value) for key, value in list( kwargs.items() )])
    failed = run_tests_on_files(files.keys(), [" ".join(cmdchecks), " --inverse-warnings" if inverse_warnings else "", " ".join(csq_args), extracmds])
    if len(failed)>0:
        sys.exit(1)


def get_files_by_configs(table):
    res = {}
    for file, config in table.items():
        if config is None:
            config = {}
        config_str = json.dumps(config) #.get("checks",[]))
        if not (os.path.basename(file) in config.get("files_to_skip",[])):
            if res.get(config_str) is not None:
                c = res[config_str]
                c.append(file)
                res[config_str] = c
            else:
                res[config_str] = [file]
    return res


def check_files_with_configs(files, cmdchecks = [], **kwargs):
    cfiles = get_files_by_configs(files)

    failed = []
    print("Check squirrel files using .dreyconfig files")
    for config_str, filesList in cfiles.items():
        config = json.loads(config_str)
        configchecks = config.get("checks", [])

        totalchecks = configchecks + cmdchecks
        cmds = ""
        print("Checking files:\n    {0}{1}".format("\n    ".join(filesList[:10]), "\n        and {0} more".format(len(filesList)-10) if len(filesList) >10 else ""))
        if "*" in totalchecks:
            print("All warnings check enabled")
            cmds = ""
        else:
            disabled_checks = [k for k in nametonummap.keys() if k not in totalchecks]
            print("Disabled checks: {0}".format(" ".join(disabled_checks) if disabled_checks else "None"))
            totalchecks = [tidtocmd(c) for c in totalchecks]
            totalchecks = " ".join(totalchecks)
            cmds = "--inverse-warnings {0}".format(totalchecks)
        extracmds = " ".join([ "--{0}:{1}".format(key.replace("_","-"),value) for key, value in list( kwargs.items() )])

        failed.extend(run_tests_on_files(filesList, [cmds, " ".join(csq_args), extracmds]))

    print_results(failed)
    if len(failed)>0:
        sys.exit(1)


def getNameWarningMap():
    findre = re.compile("w(\d+)\s*\((.*)\)")
    out = shell("{0} --warnings-list".format(csq_path)).decode('utf-8')
    out = out.splitlines()
    filtered = []
    for i, o in enumerate(out):
        if i%3==0:
            filtered.append(o)
    res = {}
    for f in filtered:
        m = findre.search(f)
        if m:
            res.update({m.group(2):m.group(1)})
    return res

if __name__ == "__main__":
    gerritModule = None
    try:
      import gerrit
      gerritModule = gerrit
    except ImportError:
      pass
    parser = argparse.ArgumentParser(description='static analysis check')
    parser.register('action', 'extend', ExtendAction)
    parser.add_argument('paths', nargs='*', type=str, default = "")
    parser.add_argument('-x', '--exclude', nargs = '*', type=str, default = [], action="extend", help = "directories which are excluded from check")
    parser.add_argument('-w', '--warnings', nargs = '*', type=str, default = [], action="extend", help = "disable or enable checks by numeric warning like -w227 -w452. * is treated as 'all' warnings.")
    parser.add_argument('-u', '--use-configs', default = False, action="store_true", help = "use .dreyconfig files")
    parser.add_argument('-i', '--inverse-warnings', default = False, action="store_true", help = "check only specified warnings. Ignored work with --use-configs")
    parser.add_argument('-gc', '--changeid', action="store", default = os.environ.get('GERRIT_CHANGE_ID'), type = str)
    parser.add_argument('-gb', '--branch', action="store", default = os.environ.get('GERRIT_BRANCH',"master"), type = str)
    parser.add_argument('-gp', '--project', action="store", default = os.environ.get('GERRIT_PROJECT',"dagor"), type = str)
    parser.add_argument('-gr', '--revisionid', action="store", default = os.environ.get('GERRIT_PATCHSET_REVISION'), type = str)
    args = parser.parse_args()
    if len(sys.argv[1:])==0:
            #parser.print_help()
            parser.print_usage() # for just the usage line
            parser.exit()
    dagor_dir = os.environ.get("DAGOR_DIR")
    if dagor_dir:
        sys.path.append(os.path.join(dagor_dir,"tools","dagor_cdk",__utilDir,"util"))
    nametonummap = getNameWarningMap()
    use_configs = args.use_configs
    print_csq_version()
    if ((args.changeid is not None) and (args.revisionid is not None)):
        if gerritModule is None:
          print("gerrit module doesnt exist")
          sys.exit(1)
        use_configs = True
        gerrit_files = gerrit.list_files(args.changeid, args.revisionid, branch=args.branch, project=args.project)
        gerrit_files = [f[0] for f in gerrit_files.items() if f[1].get("status")!="D" and f[0].lower().endswith(".nut")]
        gerrit_files = [f for f in gerrit_files if not f.startswith("prog/1stPartyLibs/quirrel/quirrel/testData") and not f.startswith("prog/commonFx/")]
        gerrit_files = [f for f in gerrit_files if not f.startswith("skyquake/prog/scripts")]
        files = gather_files(gerrit_files, args.exclude, use_configs)
    else:
        assert len(args.paths)>0
        files = gather_files(args.paths, args.exclude, use_configs)

    if args.use_configs: # <- may be use_configs, not args.use_configs?
        check_files_with_configs(files, args.warnings)
    else:
        check_files(files, args.warnings, args.inverse_warnings)

