import os
#replacement for splitext because of things like '.tex.blk' and auto indices on duplicates
def basename(name):
    if name.startswith('//'):#fix for blender relative path
        name = name[2:]
    name=os.path.basename(name)
    dot_pos=name.find('.')
    if dot_pos>-1:
        name=name[:name.find('.')]
    return name