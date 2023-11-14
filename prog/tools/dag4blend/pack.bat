@echo off
pushd .
cd ..
copy pythonCommon\pyparsing.py dag4blend\pyparsing.py
copy pythonCommon\datablock.py dag4blend\datablock.py
zip dag4blend\dag4blend.zip dag4blend\__init__.py dag4blend\dagorShaders.cfg dag4blend\fixed_dagorShaders.cfg ^
dag4blend\constants.py dag4blend\face.py dag4blend\mesh.py dag4blend\node.py  ^
dag4blend\material.py dag4blend\nodeMaterial.py dag4blend\dagMath.py ^
dag4blend\settings.py ^
dag4blend\read_config.py dag4blend\cmp\* ^
dag4blend\dagormat\build_node_tree.py dag4blend\dagormat\compare_dagormats.py dag4blend\dagormat\dagormat.py dag4blend\dagormat\rw_dagormat_text.py ^
dag4blend\exporter\export_panel.py dag4blend\exporter\exporter.py dag4blend\exporter\writer.py ^
dag4blend\helpers\* ^
dag4blend\importer\import_panel.py dag4blend\importer\importer.py dag4blend\importer\reader.py ^
dag4blend\object_properties\object_properties.py dag4blend\smooth_groups\smooth_groups.py dag4blend\tools\* ^
dag4blend\colprops\colprops.py dag4blend\extras\* ^
dag4blend\object_properties\presets\* ^
dag4blend\pyparsing.py dag4blend\datablock.py
del dag4blend\pyparsing.py
del dag4blend\datablock.py
popd