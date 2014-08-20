#include <algorithm>
#include <fstream>
#include <glog/logging.h>
#include <glm/vec2.hpp>
#include <iterator>
#include <memory>
#include <new>
#include <tuple>
#include <utility>

#ifdef USE_GLES
#include <GLES2/gl2.h>
#endif

#ifdef USE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

#include "engine.hpp"
#include "map_viewer.hpp"
#include "object_manager.hpp"
#include "sprite.hpp"
#include "text.hpp"
#include "text_font.hpp"
#include "texture_atlas.hpp"
#include "walkability.hpp"


glm::vec2 pos_to_status (glm::vec2 position) {
    return glm::vec2(position.x + 0.05, position.y + 0.75);
}

Sprite::Sprite(glm::ivec2 position,
               std::string name,
               Walkability walkability,
               std::pair<int, std::string> tile):

    MapObject(position, name, walkability, tile),
    sprite_name(name),
    is_focus(false) {

        // Setting up sprite text
        TextFont myfont = Engine::get_game_font();

        object_text = new Text(Engine::get_map_viewer()->get_window(), myfont, true);
        object_text->set_bloom_radius(6);
        object_text->set_text(name);
        object_text->resize(100,100);

        object_text->align_centre();
        object_text->align_at_origin(true);
        object_text->vertical_align_top();

        auto status_icon(std::make_shared<MapObject>(
            pos_to_status(position),
            "status icon",
            Walkability::WALKABLE,
            TextureAtlas::from_name("gui/status/stationary")
        ));

        status_icon->set_render_above_sprites(true);
        ObjectManager::get_instance().add_object(status_icon);
        status_icon_id = status_icon->get_id();
        LOG(INFO) << "created focus icon with id: " << status_icon_id;
        Engine::get_map_viewer()->get_map()->add_map_object(status_icon_id);

        // TODO: Starting positions should be integral as of currently. Check or fix.
        //
        // Make a map object blocked
        // In future this might not be needed
        blocked_tiles.insert(std::make_pair(
            "stood on",
            Engine::get_map_viewer()->get_map()->block_tile(position)
        ));

        /// build focus icon
        LOG(INFO) << "setting up focus icon";

        auto focus_icon(std::make_shared<MapObject>(
            position,
            "focus icon",
            Walkability::WALKABLE,
            TextureAtlas::from_name("gui/highlight/selected_object")
        ));

        focus_icon->set_render_above_sprites(false);
        ObjectManager::get_instance().add_object(focus_icon);
        focus_icon_id = focus_icon->get_id();
        LOG(INFO) << "created focus icon with id: " << focus_icon_id;
        Engine::get_map_viewer()->get_map()->add_map_object(focus_icon_id);

        LOG(INFO) << "Sprite initialized";
}

Sprite::~Sprite() {
    ObjectManager::get_instance().remove_object(focus_icon_id);
    // TODO: Smart pointers
    delete object_text;
    ObjectManager::get_instance().remove_object(status_icon_id);
    LOG(INFO) << "Sprite destructed";
}

void Sprite::set_state_on_moving_start(glm::ivec2 target) {
    moving = true;
    // adding blocker to new tile
    blocked_tiles.insert(std::make_pair("walking to", Engine::get_map_viewer()->get_map()->block_tile(target)));
}

void Sprite::set_state_on_moving_finish() {
    moving = false;
    // removing old blocker and changing key for new one
    blocked_tiles.erase("stood on");
    blocked_tiles.insert(std::make_pair("stood on", blocked_tiles.at("walking to")));
    blocked_tiles.erase("walking to");
}

void Sprite::add_to_inventory(int new_object_id) {
    LOG(INFO) << "adding item to sprites inventory";

    auto new_object = ObjectManager::get_instance().get_object<MapObject>(new_object_id);
    if (!new_object) {
        LOG(ERROR) << "Object manager no longer has focus_icon";
        return;
    }

    new_object->set_position(position);
    inventory.push_back(new_object_id);
}

void Sprite::set_position(glm::vec2 position) {
    MapObject::set_position(position);

    for (int item_id : get_inventory()) {
        ObjectManager::get_instance().get_object<MapObject>(item_id)->set_position(position);
    }

    auto focus_icon = ObjectManager::get_instance().get_object<MapObject>(focus_icon_id);
    if (!focus_icon) {
        LOG(ERROR) << "Object manager no longer has focus_icon";
        return;
    }
    focus_icon->set_position(position);

    auto status_icon = ObjectManager::get_instance().get_object<MapObject>(status_icon_id);
    if (!status_icon) {
        LOG(ERROR) << "Object manager no longer has status_icon";
        return;
    }
    status_icon->set_position(pos_to_status(position));
}

bool Sprite::remove_from_inventory(int old_object) {
    // there must be a better way to do this
    auto it = std::find(std::begin(inventory), std::end(inventory), old_object);
    if (it != std::end(inventory)) {
        inventory.erase(it);
        LOG(INFO) << "removing item to sprites inventory";
        return true;
    } else {
        return false;
    }
}

bool Sprite::is_in_inventory(int object) {
    auto it = std::find(std::begin(inventory), std::end(inventory), object);
    return it != std::end(inventory);
}


Sprite_Status Sprite::string_to_status(std::string status) {
    std::map<std::string,Sprite_Status> string_map = {
        { "",        Sprite_Status::NOTHING },
        { "failed",  Sprite_Status::FAILED  },
        { "killed",  Sprite_Status::KILLED  },
        { "running", Sprite_Status::RUNNING },
        { "stopped", Sprite_Status::STOPPED }
    };

    return string_map[status];
}

void Sprite::set_sprite_status(std::string _sprite_status) {
    sprite_status = string_to_status(_sprite_status);

    auto status_icon = ObjectManager::get_instance().get_object<MapObject>(status_icon_id);
    if (!status_icon) {
        LOG(ERROR) << "Object manager no longer has status_icon";
        return;
    }

    switch (sprite_status) {

        case (Sprite_Status::NOTHING):
        status_icon->set_tile(TextureAtlas::from_name("gui/status/stationary"));
        break;

        case (Sprite_Status::KILLED):
        case (Sprite_Status::STOPPED):
        status_icon->set_tile(TextureAtlas::from_name("gui/status/failed"));
        break;

        case (Sprite_Status::RUNNING):
        status_icon->set_tile(TextureAtlas::from_name("gui/status/running"));
        break;

        case (Sprite_Status::FAILED):
        // TODO: stopping should also be here
        status_icon->set_tile(TextureAtlas::from_name("gui/status/failed"));
        break;
    }


}

void Sprite::set_focus(bool _is_focus) {
    // check focus is actually being changed
    if (_is_focus != is_focus) {
        LOG(INFO) << "trying to set focus to "<< is_focus;
        is_focus = _is_focus;

        auto focus_icon = ObjectManager::get_instance().get_object<MapObject>(focus_icon_id);
        if (!focus_icon) {
            LOG(ERROR) << "Object manager no longer has focus_icon";
            return;
        }

        focus_icon->set_renderable(is_focus);
    }
}

