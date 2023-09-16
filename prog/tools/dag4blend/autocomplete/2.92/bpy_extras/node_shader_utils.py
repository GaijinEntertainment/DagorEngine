import sys
import typing


class ShaderImageTextureWrapper:
    NODES_LIST = None
    ''' '''

    colorspace_is_data = None
    ''' '''

    colorspace_name = None
    ''' '''

    extension = None
    ''' '''

    grid_row_diff = None
    ''' '''

    image = None
    ''' '''

    is_readonly = None
    ''' '''

    node_dst = None
    ''' '''

    node_image = None
    ''' '''

    node_mapping = None
    ''' '''

    owner_shader = None
    ''' '''

    projection = None
    ''' '''

    rotation = None
    ''' '''

    scale = None
    ''' '''

    socket_dst = None
    ''' '''

    texcoords = None
    ''' '''

    translation = None
    ''' '''

    use_alpha = None
    ''' '''

    def copy_from(self, tex):
        ''' 

        '''
        pass

    def copy_mapping_from(self, tex):
        ''' 

        '''
        pass

    def extension_get(self):
        ''' 

        '''
        pass

    def extension_set(self, extension):
        ''' 

        '''
        pass

    def has_mapping_node(self):
        ''' 

        '''
        pass

    def image_get(self):
        ''' 

        '''
        pass

    def image_set(self, image):
        ''' 

        '''
        pass

    def node_image_get(self):
        ''' 

        '''
        pass

    def node_mapping_get(self):
        ''' 

        '''
        pass

    def projection_get(self):
        ''' 

        '''
        pass

    def projection_set(self, projection):
        ''' 

        '''
        pass

    def rotation_get(self):
        ''' 

        '''
        pass

    def rotation_set(self, rotation):
        ''' 

        '''
        pass

    def scale_get(self):
        ''' 

        '''
        pass

    def scale_set(self, scale):
        ''' 

        '''
        pass

    def texcoords_get(self):
        ''' 

        '''
        pass

    def texcoords_set(self, texcoords):
        ''' 

        '''
        pass

    def translation_get(self):
        ''' 

        '''
        pass

    def translation_set(self, translation):
        ''' 

        '''
        pass


class ShaderWrapper:
    NODES_LIST = None
    ''' '''

    is_readonly = None
    ''' '''

    material = None
    ''' '''

    node_out = None
    ''' '''

    node_texcoords = None
    ''' '''

    use_nodes = None
    ''' '''

    def node_texcoords_get(self):
        ''' 

        '''
        pass

    def update(self):
        ''' 

        '''
        pass

    def use_nodes_get(self):
        ''' 

        '''
        pass

    def use_nodes_set(self, val):
        ''' 

        '''
        pass


class PrincipledBSDFWrapper(ShaderWrapper):
    NODES_LIST = None
    ''' '''

    alpha = None
    ''' '''

    alpha_texture = None
    ''' '''

    base_color = None
    ''' '''

    base_color_texture = None
    ''' '''

    emission_color = None
    ''' '''

    emission_color_texture = None
    ''' '''

    emission_strength = None
    ''' '''

    emission_strength_texture = None
    ''' '''

    ior = None
    ''' '''

    ior_texture = None
    ''' '''

    is_readonly = None
    ''' '''

    material = None
    ''' '''

    metallic = None
    ''' '''

    metallic_texture = None
    ''' '''

    node_normalmap = None
    ''' '''

    node_out = None
    ''' '''

    node_principled_bsdf = None
    ''' '''

    node_texcoords = None
    ''' '''

    normalmap_strength = None
    ''' '''

    normalmap_texture = None
    ''' '''

    roughness = None
    ''' '''

    roughness_texture = None
    ''' '''

    specular = None
    ''' '''

    specular_texture = None
    ''' '''

    specular_tint = None
    ''' '''

    transmission = None
    ''' '''

    transmission_texture = None
    ''' '''

    use_nodes = None
    ''' '''

    def alpha_get(self):
        ''' 

        '''
        pass

    def alpha_set(self, value):
        ''' 

        '''
        pass

    def alpha_texture_get(self):
        ''' 

        '''
        pass

    def base_color_get(self):
        ''' 

        '''
        pass

    def base_color_set(self, color):
        ''' 

        '''
        pass

    def base_color_texture_get(self):
        ''' 

        '''
        pass

    def emission_color_get(self):
        ''' 

        '''
        pass

    def emission_color_set(self, color):
        ''' 

        '''
        pass

    def emission_color_texture_get(self):
        ''' 

        '''
        pass

    def emission_strength_get(self):
        ''' 

        '''
        pass

    def emission_strength_set(self, value):
        ''' 

        '''
        pass

    def emission_strength_texture_get(self):
        ''' 

        '''
        pass

    def ior_get(self):
        ''' 

        '''
        pass

    def ior_set(self, value):
        ''' 

        '''
        pass

    def ior_texture_get(self):
        ''' 

        '''
        pass

    def metallic_get(self):
        ''' 

        '''
        pass

    def metallic_set(self, value):
        ''' 

        '''
        pass

    def metallic_texture_get(self):
        ''' 

        '''
        pass

    def node_normalmap_get(self):
        ''' 

        '''
        pass

    def node_texcoords_get(self):
        ''' 

        '''
        pass

    def normalmap_strength_get(self):
        ''' 

        '''
        pass

    def normalmap_strength_set(self, value):
        ''' 

        '''
        pass

    def normalmap_texture_get(self):
        ''' 

        '''
        pass

    def roughness_get(self):
        ''' 

        '''
        pass

    def roughness_set(self, value):
        ''' 

        '''
        pass

    def roughness_texture_get(self):
        ''' 

        '''
        pass

    def specular_get(self):
        ''' 

        '''
        pass

    def specular_set(self, value):
        ''' 

        '''
        pass

    def specular_texture_get(self):
        ''' 

        '''
        pass

    def specular_tint_get(self):
        ''' 

        '''
        pass

    def specular_tint_set(self, value):
        ''' 

        '''
        pass

    def transmission_get(self):
        ''' 

        '''
        pass

    def transmission_set(self, value):
        ''' 

        '''
        pass

    def transmission_texture_get(self):
        ''' 

        '''
        pass

    def update(self):
        ''' 

        '''
        pass

    def use_nodes_get(self):
        ''' 

        '''
        pass

    def use_nodes_set(self, val):
        ''' 

        '''
        pass


def rgb_to_rgba(rgb):
    ''' 

    '''

    pass


def rgba_to_rgb(rgba):
    ''' 

    '''

    pass


def values_clamp(val, minv, maxv):
    ''' 

    '''

    pass
