
def map_range(value, from_min, from_max, to_min, to_max):
    from_range = from_max - from_min
    to_range = to_max - to_min
    factor = value * from_range
    remapped_value = (value - from_min) / from_range * to_range + to_min
    return remapped_value