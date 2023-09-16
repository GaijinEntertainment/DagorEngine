import sys
import typing


class NodeCategory:
    def poll(self, _context):
        ''' 

        '''
        pass


class NodeItem:
    label = None
    ''' '''

    translation_context = None
    ''' '''

    def draw(self, layout, _context):
        ''' 

        '''
        pass


class NodeItemCustom:
    pass


def draw_node_categories_menu(context):
    ''' 

    '''

    pass


def has_node_categories(context):
    ''' 

    '''

    pass


def node_categories_iter(context):
    ''' 

    '''

    pass


def node_items_iter(context):
    ''' 

    '''

    pass


def register_node_categories(identifier, cat_list):
    ''' 

    '''

    pass


def unregister_node_cat_types(cats):
    ''' 

    '''

    pass


def unregister_node_categories(identifier):
    ''' 

    '''

    pass
