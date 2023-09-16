# copy to c:\Program Files\Blender Foundation\Blender 2.93\2.93\scripts\templates_py\
import bpy
import os
 
filename = os.path.join("D:/dagor2/prog/tools/bl_dag_exporter", "__init__.py")
exec(compile(open(filename).read(), filename, 'exec'))