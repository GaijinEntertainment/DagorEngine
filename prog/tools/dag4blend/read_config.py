import configparser
import os
import bpy
from shutil import copyfile

from  .helpers.basename         import basename
from  .helpers.get_preferences  import get_preferences

#modifying of original file to make it readable by python configparser
#TODO: make custom parser instead of fixing unreadable comments in copy
def cfg_upd():
    blend_cfg=os.path.join(os.path.dirname(__file__), 'fixed_dagorShaders.cfg')
    addon_name = basename(__package__)
    pref = get_preferences()
    max_cfg = pref.cfg_path
    copyfile (max_cfg,blend_cfg)
    cfg=open(blend_cfg,'r')
    fixed=cfg.read().replace('//','#')#turning comments into python-like
    cfg.close()
    cfg=open(blend_cfg,'w')
    cfg.write(fixed)
    cfg.close()

def read_config():
    cfg_upd()
    shader_categories = []

    config = configparser.ConfigParser()
    config_filepath = os.path.join(os.path.dirname(__file__), 'fixed_dagorShaders.cfg')
    config.read(config_filepath)
    os.remove(config_filepath)
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
            shader_categories.append([category,[]])         #[shader_class,  [shaders]]
        elif category is not None:
            shader_categories[-1][1].append([section,[]])#[shader_name,[parameters]]

            for item in config[section].items():
                values = item[1].split()
                if len(values) < 2:
                    values = item[1].split("_")
                shader_categories[-1][1][-1][1].append([item[0], values[0]])
            for param in global_params:
                shader_categories[-1][1][-1][1].append(param)

    return shader_categories
