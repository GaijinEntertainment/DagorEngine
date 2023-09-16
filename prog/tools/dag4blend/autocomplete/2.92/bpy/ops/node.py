import sys
import typing
import bl_operators.node
import bpy.types


def add_and_link_node(
        type: str = "",
        use_transform: bool = False,
        settings: typing.
        Union[typing.Dict[str, 'bl_operators.node.NodeSetting'], typing.
              List['bl_operators.node.NodeSetting'],
              'bpy_prop_collection'] = None,
        link_socket_index: int = 0):
    ''' Add a node to the active tree and link to an existing socket

    :param type: Node Type, Node type
    :type type: str
    :param use_transform: Use Transform, Start transform operator after inserting the node
    :type use_transform: bool
    :param settings: Settings, Settings to be applied on the newly created node
    :type settings: typing.Union[typing.Dict[str, 'bl_operators.node.NodeSetting'], typing.List['bl_operators.node.NodeSetting'], 'bpy_prop_collection']
    :param link_socket_index: Link Socket Index, Index of the socket to link
    :type link_socket_index: int
    '''

    pass


def add_file(filepath: str = "",
             hide_props_region: bool = True,
             filter_blender: bool = False,
             filter_backup: bool = False,
             filter_image: bool = True,
             filter_movie: bool = True,
             filter_python: bool = False,
             filter_font: bool = False,
             filter_sound: bool = False,
             filter_text: bool = False,
             filter_archive: bool = False,
             filter_btx: bool = False,
             filter_collada: bool = False,
             filter_alembic: bool = False,
             filter_usd: bool = False,
             filter_volume: bool = False,
             filter_folder: bool = True,
             filter_blenlib: bool = False,
             filemode: int = 9,
             relative_path: bool = True,
             show_multiview: bool = False,
             use_multiview: bool = False,
             display_type: typing.Union[int, str] = 'DEFAULT',
             sort_method: typing.Union[int, str] = '',
             name: str = "Image"):
    ''' Add a file node to the current node editor

    :param filepath: File Path, Path to file
    :type filepath: str
    :param hide_props_region: Hide Operator Properties, Collapse the region displaying the operator settings
    :type hide_props_region: bool
    :param filter_blender: Filter .blend files
    :type filter_blender: bool
    :param filter_backup: Filter .blend files
    :type filter_backup: bool
    :param filter_image: Filter image files
    :type filter_image: bool
    :param filter_movie: Filter movie files
    :type filter_movie: bool
    :param filter_python: Filter python files
    :type filter_python: bool
    :param filter_font: Filter font files
    :type filter_font: bool
    :param filter_sound: Filter sound files
    :type filter_sound: bool
    :param filter_text: Filter text files
    :type filter_text: bool
    :param filter_archive: Filter archive files
    :type filter_archive: bool
    :param filter_btx: Filter btx files
    :type filter_btx: bool
    :param filter_collada: Filter COLLADA files
    :type filter_collada: bool
    :param filter_alembic: Filter Alembic files
    :type filter_alembic: bool
    :param filter_usd: Filter USD files
    :type filter_usd: bool
    :param filter_volume: Filter OpenVDB volume files
    :type filter_volume: bool
    :param filter_folder: Filter folders
    :type filter_folder: bool
    :param filter_blenlib: Filter Blender IDs
    :type filter_blenlib: bool
    :param filemode: File Browser Mode, The setting for the file browser mode to load a .blend file, a library or a special file
    :type filemode: int
    :param relative_path: Relative Path, Select the file relative to the blend file
    :type relative_path: bool
    :param show_multiview: Enable Multi-View
    :type show_multiview: bool
    :param use_multiview: Use Multi-View
    :type use_multiview: bool
    :param display_type: Display Type * DEFAULT Default, Automatically determine display type for files. * LIST_VERTICAL Short List, Display files as short list. * LIST_HORIZONTAL Long List, Display files as a detailed list. * THUMBNAIL Thumbnails, Display files as thumbnails.
    :type display_type: typing.Union[int, str]
    :param sort_method: File sorting mode
    :type sort_method: typing.Union[int, str]
    :param name: Name, Data-block name to assign
    :type name: str
    '''

    pass


def add_mask(name: str = "Mask"):
    ''' Add a mask node to the current node editor

    :param name: Name, Data-block name to assign
    :type name: str
    '''

    pass


def add_node(type: str = "",
             use_transform: bool = False,
             settings: typing.
             Union[typing.Dict[str, 'bl_operators.node.NodeSetting'], typing.
                   List['bl_operators.node.NodeSetting'],
                   'bpy_prop_collection'] = None):
    ''' Add a node to the active tree

    :param type: Node Type, Node type
    :type type: str
    :param use_transform: Use Transform, Start transform operator after inserting the node
    :type use_transform: bool
    :param settings: Settings, Settings to be applied on the newly created node
    :type settings: typing.Union[typing.Dict[str, 'bl_operators.node.NodeSetting'], typing.List['bl_operators.node.NodeSetting'], 'bpy_prop_collection']
    '''

    pass


def add_reroute(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
                cursor: int = 8):
    ''' Add a reroute node

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param cursor: Cursor
    :type cursor: int
    '''

    pass


def add_search(type: str = "",
               use_transform: bool = False,
               settings: typing.
               Union[typing.Dict[str, 'bl_operators.node.NodeSetting'], typing.
                     List['bl_operators.node.NodeSetting'],
                     'bpy_prop_collection'] = None,
               node_item: typing.Union[int, str] = ''):
    ''' Add a node to the active tree

    :param type: Node Type, Node type
    :type type: str
    :param use_transform: Use Transform, Start transform operator after inserting the node
    :type use_transform: bool
    :param settings: Settings, Settings to be applied on the newly created node
    :type settings: typing.Union[typing.Dict[str, 'bl_operators.node.NodeSetting'], typing.List['bl_operators.node.NodeSetting'], 'bpy_prop_collection']
    :param node_item: Node Type, Node type
    :type node_item: typing.Union[int, str]
    '''

    pass


def attach():
    ''' Attach active node to a frame

    '''

    pass


def backimage_fit():
    ''' Fit the background image to the view

    '''

    pass


def backimage_move():
    ''' Move node backdrop

    '''

    pass


def backimage_sample():
    ''' Use mouse to sample background image

    '''

    pass


def backimage_zoom(factor: float = 1.2):
    ''' Zoom in/out the background image

    :param factor: Factor
    :type factor: float
    '''

    pass


def clear_viewer_border():
    ''' Clear the boundaries for viewer operations

    '''

    pass


def clipboard_copy():
    ''' Copies selected nodes to the clipboard

    '''

    pass


def clipboard_paste():
    ''' Pastes nodes from the clipboard to the active node tree

    '''

    pass


def collapse_hide_unused_toggle():
    ''' Toggle collapsed nodes and hide unused sockets :file: startup/bl_operators/node.py\:267 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/node.py$267> _

    '''

    pass


def cryptomatte_layer_add():
    ''' Add a new input layer to a Cryptomatte node

    '''

    pass


def cryptomatte_layer_remove():
    ''' Remove layer from a Cryptomatte node

    '''

    pass


def delete():
    ''' Delete selected nodes

    '''

    pass


def delete_reconnect():
    ''' Delete nodes; will reconnect nodes as if deletion was muted

    '''

    pass


def detach():
    ''' Detach selected nodes from parents

    '''

    pass


def detach_translate_attach(NODE_OT_detach=None,
                            TRANSFORM_OT_translate=None,
                            NODE_OT_attach=None):
    ''' Detach nodes, move and attach to frame

    :param NODE_OT_detach: Detach Nodes, Detach selected nodes from parents
    :param TRANSFORM_OT_translate: Move, Move selected items
    :param NODE_OT_attach: Attach Nodes, Attach active node to a frame
    '''

    pass


def duplicate(keep_inputs: bool = False):
    ''' Duplicate selected nodes

    :param keep_inputs: Keep Inputs, Keep the input links to duplicated nodes
    :type keep_inputs: bool
    '''

    pass


def duplicate_move(NODE_OT_duplicate=None, NODE_OT_translate_attach=None):
    ''' Duplicate selected nodes and move them

    :param NODE_OT_duplicate: Duplicate Nodes, Duplicate selected nodes
    :param NODE_OT_translate_attach: Move and Attach, Move nodes and attach to frame
    '''

    pass


def duplicate_move_keep_inputs(NODE_OT_duplicate=None,
                               NODE_OT_translate_attach=None):
    ''' Duplicate selected nodes keeping input links and move them

    :param NODE_OT_duplicate: Duplicate Nodes, Duplicate selected nodes
    :param NODE_OT_translate_attach: Move and Attach, Move nodes and attach to frame
    '''

    pass


def find_node(prev: bool = False):
    ''' Search for a node by name and focus and select it

    :param prev: Previous
    :type prev: bool
    '''

    pass


def group_edit(exit: bool = False):
    ''' Edit node group

    :param exit: Exit
    :type exit: bool
    '''

    pass


def group_insert():
    ''' Insert selected nodes into a node group

    '''

    pass


def group_make():
    ''' Make group from selected nodes

    '''

    pass


def group_separate(type: typing.Union[int, str] = 'COPY'):
    ''' Separate selected nodes from the node group

    :param type: Type * COPY Copy, Copy to parent node tree, keep group intact. * MOVE Move, Move to parent node tree, remove from group.
    :type type: typing.Union[int, str]
    '''

    pass


def group_ungroup():
    ''' Ungroup selected nodes

    '''

    pass


def hide_socket_toggle():
    ''' Toggle unused node socket display

    '''

    pass


def hide_toggle():
    ''' Toggle hiding of selected nodes

    '''

    pass


def insert_offset():
    ''' Automatically offset nodes on insertion

    '''

    pass


def join():
    ''' Attach selected nodes to a new common frame

    '''

    pass


def link(detach: bool = False):
    ''' Use the mouse to create a link between two nodes

    :param detach: Detach, Detach and redirect existing links
    :type detach: bool
    '''

    pass


def link_make(replace: bool = False):
    ''' Makes a link between selected output in input sockets

    :param replace: Replace, Replace socket connections with the new links
    :type replace: bool
    '''

    pass


def link_viewer():
    ''' Link to viewer node

    '''

    pass


def links_cut(path: typing.Union[
        typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
        List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
              cursor: int = 12):
    ''' Use the mouse to cut (remove) some links

    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param cursor: Cursor
    :type cursor: int
    '''

    pass


def links_detach():
    ''' Remove all links to selected nodes, and try to connect neighbor nodes together

    '''

    pass


def move_detach_links(NODE_OT_links_detach=None,
                      TRANSFORM_OT_translate=None,
                      NODE_OT_insert_offset=None):
    ''' Move a node to detach links

    :param NODE_OT_links_detach: Detach Links, Remove all links to selected nodes, and try to connect neighbor nodes together
    :param TRANSFORM_OT_translate: Move, Move selected items
    :param NODE_OT_insert_offset: Insert Offset, Automatically offset nodes on insertion
    '''

    pass


def move_detach_links_release(NODE_OT_links_detach=None,
                              NODE_OT_translate_attach=None):
    ''' Move a node to detach links

    :param NODE_OT_links_detach: Detach Links, Remove all links to selected nodes, and try to connect neighbor nodes together
    :param NODE_OT_translate_attach: Move and Attach, Move nodes and attach to frame
    '''

    pass


def mute_toggle():
    ''' Toggle muting of the nodes

    '''

    pass


def new_geometry_node_group_assign():
    ''' Create a new geometry node group and assign it to the active modifier :file: startup/bl_operators/geometry_nodes.py\:82 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/geometry_nodes.py$82> _

    '''

    pass


def new_geometry_nodes_modifier():
    ''' Create a new modifier with a new geometry node group :file: startup/bl_operators/geometry_nodes.py\:62 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/geometry_nodes.py$62> _

    '''

    pass


def new_node_tree(type: typing.Union[int, str] = '', name: str = "NodeTree"):
    ''' Create a new node tree

    :param type: Tree Type
    :type type: typing.Union[int, str]
    :param name: Name
    :type name: str
    '''

    pass


def node_color_preset_add(name: str = "",
                          remove_name: bool = False,
                          remove_active: bool = False):
    ''' Add or remove a Node Color Preset

    :param name: Name, Name of the preset, used to make the path name
    :type name: str
    :param remove_name: remove_name
    :type remove_name: bool
    :param remove_active: remove_active
    :type remove_active: bool
    '''

    pass


def node_copy_color():
    ''' Copy color to all selected nodes

    '''

    pass


def options_toggle():
    ''' Toggle option buttons display for selected nodes

    '''

    pass


def output_file_add_socket(file_path: str = "Image"):
    ''' Add a new input to a file output node

    :param file_path: File Path, Subpath of the output file
    :type file_path: str
    '''

    pass


def output_file_move_active_socket(direction: typing.Union[int, str] = 'DOWN'):
    ''' Move the active input of a file output node up or down the list

    :param direction: Direction
    :type direction: typing.Union[int, str]
    '''

    pass


def output_file_remove_active_socket():
    ''' Remove active input from a file output node

    '''

    pass


def parent_set():
    ''' Attach selected nodes

    '''

    pass


def preview_toggle():
    ''' Toggle preview display for selected nodes

    '''

    pass


def read_viewlayers():
    ''' Read all render layers of all used scenes

    '''

    pass


def render_changed():
    ''' Render current scene, when input node's layer has been changed

    '''

    pass


def resize():
    ''' Resize a node

    '''

    pass


def select(wait_to_deselect_others: bool = False,
           mouse_x: int = 0,
           mouse_y: int = 0,
           extend: bool = False,
           socket_select: bool = False,
           deselect_all: bool = False):
    ''' Select the node under the cursor

    :param wait_to_deselect_others: Wait to Deselect Others
    :type wait_to_deselect_others: bool
    :param mouse_x: Mouse X
    :type mouse_x: int
    :param mouse_y: Mouse Y
    :type mouse_y: int
    :param extend: Extend
    :type extend: bool
    :param socket_select: Socket Select
    :type socket_select: bool
    :param deselect_all: Deselect On Nothing, Deselect all when nothing under the cursor
    :type deselect_all: bool
    '''

    pass


def select_all(action: typing.Union[int, str] = 'TOGGLE'):
    ''' (De)select all nodes

    :param action: Action, Selection action to execute * TOGGLE Toggle, Toggle selection for all elements. * SELECT Select, Select all elements. * DESELECT Deselect, Deselect all elements. * INVERT Invert, Invert selection of all elements.
    :type action: typing.Union[int, str]
    '''

    pass


def select_box(tweak: bool = False,
               xmin: int = 0,
               xmax: int = 0,
               ymin: int = 0,
               ymax: int = 0,
               wait_for_input: bool = True,
               mode: typing.Union[int, str] = 'SET'):
    ''' Use box selection to select nodes

    :param tweak: Tweak, Only activate when mouse is not over a node - useful for tweak gesture
    :type tweak: bool
    :param xmin: X Min
    :type xmin: int
    :param xmax: X Max
    :type xmax: int
    :param ymin: Y Min
    :type ymin: int
    :param ymax: Y Max
    :type ymax: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_circle(x: int = 0,
                  y: int = 0,
                  radius: int = 25,
                  wait_for_input: bool = True,
                  mode: typing.Union[int, str] = 'SET'):
    ''' Use circle selection to select nodes

    :param x: X
    :type x: int
    :param y: Y
    :type y: int
    :param radius: Radius
    :type radius: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_grouped(extend: bool = False,
                   type: typing.Union[int, str] = 'TYPE'):
    ''' Select nodes with similar properties

    :param extend: Extend, Extend selection instead of deselecting everything first
    :type extend: bool
    :param type: Type
    :type type: typing.Union[int, str]
    '''

    pass


def select_lasso(
        tweak: bool = False,
        path: typing.Union[
            typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.
            List['bpy.types.OperatorMousePath'], 'bpy_prop_collection'] = None,
        mode: typing.Union[int, str] = 'SET'):
    ''' Select nodes using lasso selection

    :param tweak: Tweak, Only activate when mouse is not over a node - useful for tweak gesture
    :type tweak: bool
    :param path: Path
    :type path: typing.Union[typing.Dict[str, 'bpy.types.OperatorMousePath'], typing.List['bpy.types.OperatorMousePath'], 'bpy_prop_collection']
    :param mode: Mode * SET Set, Set a new selection. * ADD Extend, Extend existing selection. * SUB Subtract, Subtract existing selection.
    :type mode: typing.Union[int, str]
    '''

    pass


def select_link_viewer(NODE_OT_select=None, NODE_OT_link_viewer=None):
    ''' Select node and link it to a viewer node

    :param NODE_OT_select: Select, Select the node under the cursor
    :param NODE_OT_link_viewer: Link to Viewer Node, Link to viewer node
    '''

    pass


def select_linked_from():
    ''' Select nodes linked from the selected ones

    '''

    pass


def select_linked_to():
    ''' Select nodes linked to the selected ones

    '''

    pass


def select_same_type_step(prev: bool = False):
    ''' Activate and view same node type, step by step

    :param prev: Previous
    :type prev: bool
    '''

    pass


def shader_script_update():
    ''' Update shader script node with new sockets and options from the script

    '''

    pass


def switch_view_update():
    ''' Update views of selected node

    '''

    pass


def translate_attach(TRANSFORM_OT_translate=None,
                     NODE_OT_attach=None,
                     NODE_OT_insert_offset=None):
    ''' Move nodes and attach to frame

    :param TRANSFORM_OT_translate: Move, Move selected items
    :param NODE_OT_attach: Attach Nodes, Attach active node to a frame
    :param NODE_OT_insert_offset: Insert Offset, Automatically offset nodes on insertion
    '''

    pass


def translate_attach_remove_on_cancel(TRANSFORM_OT_translate=None,
                                      NODE_OT_attach=None,
                                      NODE_OT_insert_offset=None):
    ''' Move nodes and attach to frame

    :param TRANSFORM_OT_translate: Move, Move selected items
    :param NODE_OT_attach: Attach Nodes, Attach active node to a frame
    :param NODE_OT_insert_offset: Insert Offset, Automatically offset nodes on insertion
    '''

    pass


def tree_path_parent():
    ''' Go to parent node tree :file: startup/bl_operators/node.py\:297 <https://developer.blender.org/diffusion/B/browse/master/release/scripts/startup/bl_operators/node.py$297> _

    '''

    pass


def tree_socket_add(in_out: typing.Union[int, str] = 'IN'):
    ''' Add an input or output socket to the current node tree

    :param in_out: Socket Type
    :type in_out: typing.Union[int, str]
    '''

    pass


def tree_socket_move(direction: typing.Union[int, str] = 'UP',
                     in_out: typing.Union[int, str] = 'IN'):
    ''' Move a socket up or down in the current node tree's sockets stack

    :param direction: Direction
    :type direction: typing.Union[int, str]
    :param in_out: Socket Type
    :type in_out: typing.Union[int, str]
    '''

    pass


def tree_socket_remove(in_out: typing.Union[int, str] = 'IN'):
    ''' Remove an input or output socket to the current node tree

    :param in_out: Socket Type
    :type in_out: typing.Union[int, str]
    '''

    pass


def view_all():
    ''' Resize view so you can see all nodes

    '''

    pass


def view_selected():
    ''' Resize view so you can see selected nodes

    '''

    pass


def viewer_border(xmin: int = 0,
                  xmax: int = 0,
                  ymin: int = 0,
                  ymax: int = 0,
                  wait_for_input: bool = True):
    ''' Set the boundaries for viewer operations

    :param xmin: X Min
    :type xmin: int
    :param xmax: X Max
    :type xmax: int
    :param ymin: Y Min
    :type ymin: int
    :param ymax: Y Max
    :type ymax: int
    :param wait_for_input: Wait for Input
    :type wait_for_input: bool
    '''

    pass
