from "dagor.fs" import scan_folder
from "dagor.system" import exit

let __file__ = __FILE__.split("/")
let root =  __file__.len()>1 ? __file__[0] : "."
let selffname = __file__.len()>1 ? __file__.top() : __file__[0]
let tests = scan_folder({root, files_suffix = ".nut"})
let failed = []
foreach( test in tests) {
  if (test.endswith(selffname))
    continue
  println($"Running test: {test}")
  try{
    require(test)
  } catch(e) {
    println($"Error: {e}")
    failed.append([test, e])
  }
}
foreach (test in failed) {
  let [path, reason] = test
  println($"{path}: failed with {reason}")
}
if (failed.len()>0)
  exit(1)