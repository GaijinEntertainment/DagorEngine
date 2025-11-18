from os         import walk
from os.path    import join

import bpy,pickle
from time                   import time
from ..helpers.texts        import log
from ..helpers.getters      import get_preferences, get_addon_directory

def skip(filename):
    skip = True
    for e in ['.gameobj.blk', '.composit.blk', '.dag']:
        if filename.endswith(e):
            skip = False
    return skip

def is_prefab(filename):
    if filename.endswith('.dag'):
        if filename.replace('.dag','').find('.')==-1:
            return True
    return False

#rendinst, riextra, dynmodel - can't guarantee by name which one is which
def is_geometry(filename):
    if filename.endswith('.lod00.dag'):
        return True
    return False

def is_composit(filename):
    if filename.endswith('.composit.blk'):
        return True
    return False

def is_gameobj(filename):
    if filename.endswith('.gameobj.blk'):
        return True
    return False

def build_cache():
    entities = {}
    start = time()
    pref = get_preferences()
    index = int(pref.project_active)
    lib_path = pref.projects[index]['path']
    for subdir,dirs,files in walk(lib_path):
        for file in files:
            filename=file.lower()
            if skip(filename):
                continue
            assetname = filename[0:filename.find('.')]
            assetname = assetname.lower()
            if assetname not in entities:
                entities[assetname] = []
            if is_prefab(filename):
                entities[assetname].append(['prefab', join(subdir,filename)])

#TODO: exclude dynmodels. Dynmodels doesn't work in composits anyway, so this shouldn't be an issue
            elif is_geometry(filename):
                entities[assetname].append(['rendinst',join(subdir,filename)])
#'rendinst' instead of 'geometry' to skip extra steps on processing, since cmp can contain only RI anyway
            elif is_composit(filename):
                entities[assetname].append(['composit',join(subdir,filename)])
            elif is_gameobj(filename):
                entities[assetname].append(['gameobj',join(subdir,filename)])
    project_name = pref.projects[index]['name']
    cache_path = get_addon_directory() + f'/{project_name}.bin'
    try:
        cache = open(cache_path,'wb')
        pickle.dump(entities,cache)
        cache.close()
    except:
        msg = f'Can not write {project_name} paths to cache!\n'
        log(msg, type = 'ERROR', show = True)
        return entities
    msg = f'Cache updated in {round(time()-start,2)} sec.\n'
    log(msg, show = True)
    return entities

def read_cache():
    pref = get_preferences()
    index = int(pref.project_active)
    project_name = pref.projects[index]['name']
    cache_path = get_addon_directory() + f'/{project_name}.bin'
    try:
        cache = open(cache_path,'rb')
        entities = pickle.load(cache)
        cache.close()
    except:
        msg =f'Can not read {project_name} paths from cache!\n'
        log(msg, type = 'ERROR')
    return entities