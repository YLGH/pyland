#import Player from characters/player 

#player_one = Player()

""" Auto generate objects based on level data """
""" Load in saved states """

#print("Creating the level")
#move_north()
#player_one.move_north()

#croc_one.follow_path("north, east, south")

#portal_one.set_destination("level_two")


#boulder_one.move_north()

#boulder_one.callback_test(lambda: boulder_one.callback_test(lambda: print("hohohoho")))

boulder_one.focus()
boulder_one.move_south(lambda: boulder_one.move_south(lambda: boulder_one.move_west(lambda: boulder_one.move_south(lambda: boulder_one.move_south(lambda: boulder_one.move_south())))))
croc_one.focus()
croc_one.follow_path("north, east, east, south, west, west", True)


#boulder_four.callback_test(lambda: boulder_four.callback_test(lambda: boulder_four.callback_test(lambda: print(dialogue))))
boulder_four.callback_test(lambda: boulder_four.callback_test(lambda: boulder_four.callback_test(lambda: print(game.getDialogue("welcome")))))
print("wohooo")
