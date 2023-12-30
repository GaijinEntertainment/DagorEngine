# dabuild
jam -s Root=../.. -f sceneTools/assetExp/jamfile
jam -s Root=../.. -f converters/ddsxCvt2/jamfile

# DDSx plugins
jam -s Root=../.. -f libTools/pcDdsxConv/jamfile
jam -s Root=../.. -s iOS_exp=yes -f libTools/pcDdsxConv/jamfile
jam -s Root=../.. -s Tegra_exp=yes -f libTools/pcDdsxConv/jamfile

# shader compilers
jam -s Root=../.. -f ShaderCompiler2/jamfile

# utils
jam -s Root=../.. -f sceneTools/vromfsPacker/jamfile
jam -s Root=../.. -f sceneTools/csvUtil/jamfile
jam -s Root=../.. -f converters/ddsxCvt/jamfile
jam -s Root=../.. -f converters/ddsx2dds/jamfile
jam -s Root=../.. -f converters/ddsConverter/jamfile
jam -s Root=../.. -f FontGenerator/jamfile
jam -s Root=../.. -f converters/GuiTex/jamfile
jam -s Root=../.. -f sceneTools/utils/jamfile-binBlk

jam -s Root=../.. -f sceneTools/dumpGrp/jamfile
jam -s Root=../.. -f sceneTools/dbldUtil/jamfile
jam -s Root=../.. -f sceneTools/resDiff/jamfile
jam -s Root=../.. -f sceneTools/resUpdate/jamfile
jam -s Root=../.. -f sceneTools/resClean/jamfile

jam -s Root=../.. -sConfig=dev -f consoleSq/jamfile

# GUI tools
jam -s Root=../.. -f dargbox/jamfile

# Blender plugin
pushd dag4blend
python3 __build_pack.py FINAL
popd
