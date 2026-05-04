
# removes subpidrs; does nothing if it's not a path
def ensure_no_path(filepath):
    filepath = filepath.replace('\\', '/')  # making sure there's only one type of separator
    return filepath.split('/')[-1]

# removes everything after first '.', does nothing if no dots
def ensure_no_extension(string):
    return string.split('.')[0]

# removes optional parameters like 'texture_name*?q0-0-1'
def ensure_no_params(texture_slot):
    return texture_slot.split('*?')[0]

#returns only name with no path or extension
def asset_basename(filepath):
    filename = ensure_no_path(filepath)
    return ensure_no_extension(filename)

# also removes optional parameters, which exist only for textures
def texture_basename(texture_slot):
    basename = asset_basename(texture_slot)
    return ensure_no_params(basename)


#replacement for splitext because of things like '.tex.blk' and auto indices on duplicates
def basename(name):
    return asset_basename(name)

