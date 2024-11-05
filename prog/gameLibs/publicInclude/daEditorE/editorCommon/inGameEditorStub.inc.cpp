#include "inGameEditor.h"
#include "entityEditor.h"
#include <EASTL/fixed_function.h>

class EntityObj;
static IDaEditor4StubEC daEd4Stub;

void init_entity_object_editor() {}
void init_da_editor4() {}
void term_da_editor4() {}
bool is_editor_activated() { return false; }
bool is_editor_in_reload() { return false; }
void start_editor_reload() {}
void finish_editor_reload() {}
bool is_editor_free_camera_enabled() { return false; }

IDaEditor4EmbeddedComponent &get_da_editor4() { return daEd4Stub; }
EntityObjEditor *get_entity_obj_editor() { return nullptr; }
bool has_in_game_editor() { return false; }
void da_editor4_setup_scene(const char *) {}
void register_editor_script(SqModules *) {}
void entity_obj_editor_for_each_entity(EntityObjEditor &, eastl::fixed_function<sizeof(void *), void(EntityObj *)>) {}
