#ifndef SPRITE_H
#define SPRITE_H

#include <glm/vec2.hpp>
#include <map>
#include <string>
#include <vector>

#include "animation_frames.hpp"
#include "map.hpp"
#include "map_object.hpp"
#include "walkability.hpp"

class Text;

enum class Sprite_Status {NOTHING, RUNNING, STOPPED, FAILED, KILLED};

///
/// Represents a sprite in the engine
///
class Sprite : public MapObject {
private:
    Sprite_Status string_to_status(std::string status);
    std::string status_to_string(Sprite_Status status);

    std::string sprite_name;

    unsigned int inventory_limit = 1;

    bool just_terminated;

protected:
    ///
    /// The text to display above the object
    ///
    Text *object_text = nullptr;

    ///
    /// The status text for the object
    ///
    int status_icon_id;

    ///
    /// status of sprite
    /// TODO: this value isn't used at the moment, use for status images
    Sprite_Status sprite_status;


    ///
    /// Map_objects which move with the sprite
    ///
    std::vector<int> inventory;

    ///
    /// The focus icon, to move with sprite and hide, depending on if sprite is in focus;
    ///
    bool is_focus;

    ///
    /// The focus icon, to move with sprite and hide, depending on if sprite is in focus;
    ///
    int focus_icon_id;

    ///
    /// Instructions for how to complete the current task, as part of the challenge.
    ///
    /// This can vary by character, but the main reason this is a sprite attribute
    /// (rather than a challenge attribute) is because the code-base is a mess.
    ///
    std::string instructions;

public:
    /// Tiles that the object is blocking, probably
    /// by standing on.
    ///
    std::map<std::string, Map::Blocker> blocked_tiles;
    Sprite();
    ///
    /// Constructs a sprite
    /// @param position
    ///     the (x, y) position of the sprite on the map
    /// @param name
    ///     the name of the sprite
    /// @param walkability
    ///     the walkability properties of the sprite
    /// @param tile
    ///     the sprite's image, referenced by an id:sheet_name pair
    /// @param walk_frames
    ///     walking frames to animate movement.
    ///
    Sprite(glm::ivec2 position,
           std::string name,
           Walkability walkability,
           AnimationFrames frames,
           std::string start_frame);

    virtual ~Sprite();

    ///
    /// Get the object's text to display
    /// @return the object's text
    ///
    Text* get_object_text() { return object_text; }

    ///
    /// Set the object's text to be displayed
    /// @param _object_text the object's text
    ///
    void set_object_text(Text* _object_text) {object_text = _object_text; }

    ///
    /// add map_object to sprites inventory
    ///
    bool add_to_inventory(int new_object);

    std::vector<int> get_inventory() { return inventory; }

    void set_position(glm::vec2 position);


    ///
    /// remove the specified object from the sprites inventory, safe to use even if
    /// item isn't in inventory
    /// @return
    ///     true if successfully removed, false if it wasn't present
    bool remove_from_inventory(int old_object);

    bool is_in_inventory(int object);


    void set_sprite_status(std::string _sprite_status);

    std::string get_sprite_status();

    void set_focus(bool _is_focus);

    std::string get_sprite_name() {return sprite_name; }

    void get_inventory_limit (unsigned int _inventory_limit) {inventory_limit = _inventory_limit; };

    void set_instructions(std::string instructions);
    std::string get_instructions();

    bool get_just_terminated();

    void toggle_just_terminated();
};


#endif
