import configparser
import os
import bpy
from shutil import copyfile

#modifying of original file to make it readable by python configparser
#TODO: make custom parser, that allows to use original file and separate parameters by cooment-based groups
def cfg_upd():
    blend_cfg=os.path.join(os.path.dirname(__file__), 'fixed_dagorShaders.cfg')
    max_cfg=os.path.join(os.path.dirname(__file__), 'dagorShaders.cfg')
    copyfile (max_cfg,blend_cfg)
    cfg=open(blend_cfg,'r')
    fixed=cfg.read().replace('//','#')#turning comments into python-like
    cfg.close()
    cfg=open(blend_cfg,'w')
    cfg.write(fixed)
    cfg.close()

def read_config():
    cfg_upd()
    shader_classes = []

    config = configparser.ConfigParser()
    config.read(os.path.join(os.path.dirname(__file__), 'fixed_dagorShaders.cfg'))
    category = None
    global_params_section = config["_global_params"]
    global_params = []
    for item in global_params_section.items():
        if not item[1].startswith("enum"):
            values = item[1].split()
            global_params.append([item[0], values[0]])
    for section in config.sections():
        if section.startswith("--"):
            category=section
            shader_classes.append([category,[]])         #[shader_class,  [shaders]]
        elif category is not None:
            shader_classes[-1][1].append([section,[]])#[shader_name,[parameters]]

            for item in config[section].items():
                values = item[1].split()
                if len(values) < 2:
                    values = item[1].split("_")
                shader_classes[-1][1][-1][1].append([item[0], values[0]])
            for param in global_params:
                shader_classes[-1][1][-1][1].append(param)

    return shader_classes
