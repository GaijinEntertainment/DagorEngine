from bpy.app    import version_string

def get_blender_version():
    ver = version_string#somewhat like "4.1.0 Releace Candidate"
    ver = ver.split(".")
    ver = ".".join([ver[0], ver[1]])#changes in third part do not affect logic
    float_ver = float(ver)
    return float_ver