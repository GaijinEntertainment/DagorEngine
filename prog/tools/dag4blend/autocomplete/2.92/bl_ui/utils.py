import sys
import typing


class PresetPanel:
    bl_label = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_space_type = None
    ''' '''

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_menu(self, layout, text):
        ''' 

        '''
        pass

    def draw_panel_header(self, layout):
        ''' 

        '''
        pass

    def path_menu(self, searchpaths, operator, props_default, prop_filepath,
                  filter_ext, filter_path, display_name, add_operator):
        ''' 

        '''
        pass
