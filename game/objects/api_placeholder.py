import multiprocessing
import time

class API:

    object_properties = {}
    
    def set_sprite(self, game_object, sprite_location):
        print('API: set sprite location of "{}" to "{}"'.format(game_object.get_name(), sprite_location))
        #self.object_properties[game_object.get_name()]["sprite_location"] = sprite_location # a placeholder way of holding the information
        return

    def set_visible(self, game_object, visible):
        print('API: set object visibilty of "{}" to "{}"'.format(game_object.get_name(), visible))
        return

    def set_solid(self, game_object, solid):
        print('API: set solidity of "{}" to "{}"'.format(game_object.get_name(), solid))
        return

    def move_north(self, game_object, callback):
        print("Testing threading in python!!!!")
        p = multiprocessing.Process(target = lambda: self.wait(callback)) # Work out wether to use this or threading.Thread!
        p.start()
        print("Thread made and running ...")
        return

    def __move(self, direction, callback):
        print("Moving {}...")
        time.sleep(5)
        print("exiting thread...")
        callback()
        return
        
        
