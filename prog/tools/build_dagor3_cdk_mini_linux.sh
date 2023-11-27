# utils
jam -s Root=../.. -f sceneTools/vromfsPacker/jamfile
jam -s Root=../.. -f sceneTools/csvUtil/jamfile
jam -s Root=../.. -f sceneTools/utils/jamfile-blk

jam -s Root=../.. -f sceneTools/dumpGrp/jamfile
jam -s Root=../.. -f sceneTools/dbldUtil/jamfile

jam -s Root=../.. -sConfig=dev -f consoleSq/jamfile

# GUI tools
jam -s Root=../.. -f dargbox/jamfile

# Blender plugin
pushd dag4blend
python3 __build_pack.py FINAL
popd
