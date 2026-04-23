#include "register_types.h"

#include "enemy.h"
#include "game_world.h"
#include "player_controller.h"

#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_topdown_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<GameWorld>();
    ClassDB::register_class<PlayerController>();
    ClassDB::register_class<Enemy>();
}

void uninitialize_topdown_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT topdown_shooter_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_topdown_module);
    init_obj.register_terminator(uninitialize_topdown_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
