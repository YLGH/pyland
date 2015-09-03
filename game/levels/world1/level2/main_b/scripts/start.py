#commence save-data set-up
world_name = "world_1"
level_name = "level_2"
map_name = "main_b"

engine.update_world_text("1")
engine.update_level_text("2")

player_data.load(engine.get_player_name())
player_data.set_map(world_name, level_name = level_name, map_name = map_name) #change the map and save that the map has changed
#end save-data set-up

player_one.focus()

croc_0.face_east()
croc_1.face_east()
croc_2.face_east()

croc = [
croc_0,
croc_1,
croc_2,
croc_3,
croc_4,
croc_5
]

croc[0].face_east()
croc[1].face_east()
croc[2].face_east()
croc[3].face_east()
croc[4].face_south()
croc[5].face_west()

for c in croc:
    c.killable = [player_one, myla]
    c.still_check_kill()
    
engine.run_callback_list_sequence([
    lambda callback: player_one.face_east(callback = callback),
    lambda callback: player_one.set_busy(True, callback = callback),
    lambda callback: myla.face_west(callback = callback),
    lambda callback: engine.show_dialogue("Lots and lots and lots of crocodiles are out...", callback = callback),
    lambda callback: engine.show_dialogue("In your bag I go!", callback = callback),
    lambda callback: player_one.face_west(callback = callback),
    lambda callback: myla.set_solidity(False, callback = callback),
    lambda callback: myla.move_west(callback = callback),
    lambda callback: myla.set_visible(False, callback = callback),
    lambda callback: engine.show_dialogue("Sweet, you have a book I can read in here!", callback = callback),
    lambda callback: engine.show_dialogue("Oh it's \"Jungle Book\", far too unrealistic for me to enjoy.", callback = callback),
    lambda callback: player_one.set_busy(False, callback =callback)
    ])


first_time_end = True
def player_walked_on_end():
    global first_time_end;
    if first_time_end:
        first_time_end = False
        engine.run_callback_list_sequence([
        lambda callback: player_one.set_busy(True, callback = callback),
        lambda callback: engine.show_dialogue("Myla: You did it!", callback = callback),
        lambda callback: myla.move_to(player_one.get_position(), callback = callback),
        lambda callback: myla.set_visible(True, callback = callback),
        lambda callback: myla.move_south(callback = callback),
        lambda callback: myla.face_north(callback = callback),
        lambda callback: engine.show_dialogue("Myla: See you later alligator!", callback = callback),
        lambda callback: engine.show_dialogue("In a while crocodile!", callback = callback),
        lambda callback: player_one.set_busy(False, callback =callback),
        lambda callback: myla.follow(player_one, callback = callback),
        ])

def level_end(player_object):
    player_data.complete_level_and_save()
    player_data.save_and_exit("/world_1")

trigger_end.player_walked_on = lambda player_object: player_walked_on_end()
level_exit.player_walked_on = level_end

def prev_level(player_object):
    player_data.save_and_exit("/world_1/level_4/main_a")

exit_level_start1.player_walked_on = prev_level
exit_level_start2.player_walked_on = prev_level
