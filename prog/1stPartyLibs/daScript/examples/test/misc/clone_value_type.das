require UnitTest

def create_scene_node() : SceneNodeId
    return __create_scene_node()

[export]
def main
    print("hello world\n")
    var node := create_scene_node()
    print("node = {node}\n")
    node = create_scene_node()
    print("node = {node}\n")
    node := create_scene_node()
    print("node = {node}\n")


options log_nodes

