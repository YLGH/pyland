#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include <glm/vec2.hpp>

#ifdef USE_GLES
#include "typeface.hpp"
#include "text_font.hpp"
#include "text.hpp"
#endif

class Challenge;
class ChallengeData;
class InputManager;

#include "interpreter.hpp"
#include "gui_manager.hpp"
#include "game_window.hpp"
#include "map_viewer.hpp"
#include "notification_bar.hpp"
#include "mouse_cursor.hpp"

#include <chrono>

class GameMain{
private:
    std::string map_path;
    Interpreter interpreter;
    GUIManager gui_manager;
    GameWindow embedWindow;
    InputManager* input_manager;
    MapViewer map_viewer;
    NotificationBar notification_bar;
    ChallengeData *challenge_data;
    std::chrono::time_point<std::chrono::steady_clock> last_clock;
    glm::ivec2 tile_identifier_old_tile;
    Text tile_identifier_text;
    MouseCursor cursor;

public:
    GameMain(int argc, char *argv[]);
    void game_loop();
    Challenge* pick_challenge(ChallengeData* challenge_data);

};

#endif // GAME_MAIN_H
