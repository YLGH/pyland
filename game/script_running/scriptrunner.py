import code
import sys
import io
import os
import contextlib
import time
import threading

from scoped_interpreter import ScopedInterpreter

""" This file contains all the implementation details of how player scripts are interpreted and run. """

class HaltScriptException(Exception):
    pass

def start(script_api, script_name, script_state_container, engine, parse_error = True, callback = None):
    """ This function runs the script provided in the argument in a seperate thread.

    The script has access to a set of API's defined in script_api that allow
    it to control the player object provided.

    the callback is run after the script has finished running.

    Parameters
    ----------
    script_state_container : ScriptStateContainer
        This is the instance of the script_state_container which is interacted with using the script.
    script_name : str
        The name of the script that you wish to run. The game looks in the script folder for it.
    """

    """ How printing is handled """

    printed_flag = [False]  #This flag used to determine if the player has printed something, because if nothing has been printed there is no point in inserting the "----" in the terminal.
    def user_print(text):
        """ A simple method to print text to the game console for the user, overrides the python default print method """
        printed_flag[0] = True
        engine.print_terminal(text, False) #autoconvert print to strings (do not need to convert within the game)

    #Gets the nth line number from the error message
    def parse_line_number(string, n):

        parse_stage = 0

        start_char = 0
        end_char = 0

        for cur_line_remove in range(n-1):
            if ("line" in string):
                string = string[string.index("line")+4:len(string)]

        #engine.print_terminal("string is now \n "+string + " string ended",True)

        for char in range(string.index("line"),len(string)):
            if parse_stage == 0:
                if string[char] == " ":
                    start_char = char+1
                    parse_stage = 1
            elif parse_stage == 1:
                if ((string[char] == " ") or (string[char] == ",")):
                    end_char = char
                    parse_stage = 2
                    break

        return string[start_char:end_char]


    #Parse any Python error output to be user friendly
    def parse_output(error_message):

        parsed_error_message = ""

        if ("NameError" in error_message):
            command = ""
            line = ""

            parse_stage = 0

            start_char = 0
            end_char = 0
            for char in range(error_message.index("NameError"),len(error_message)):
                if parse_stage == 0:
                    if error_message[char] == "'":
                        start_char = char+1
                        parse_stage = 1
                elif parse_stage == 1:
                    if error_message[char] == "'":
                        end_char = char
                        parse_stage = 2
                        break

            command = error_message[start_char:end_char]

            parse_stage = 0

            start_char = 0
            end_char = 0

            if ("line" in error_message):
                #Output the second line number
                first_line_removed_error_message = error_message[error_message.index("line")+4:len(error_message)]

                if ("line" in first_line_removed_error_message):

                    for char in range(first_line_removed_error_message.index("line"),len(first_line_removed_error_message)):
                        if parse_stage == 0:
                            if first_line_removed_error_message[char] == " ":
                                start_char = char+1
                                parse_stage = 1
                        elif parse_stage == 1:
                            if ((first_line_removed_error_message[char] == " ") or (first_line_removed_error_message[char] == ",")):
                                end_char = char
                                parse_stage = 2
                                break

            line = first_line_removed_error_message[start_char:end_char]

            parsed_error_message = ("PyRunner Error: Command '"+ command +"' in line "+ line +" is not understood")
        elif ("SyntaxError" in error_message) and ("print" in error_message):
            parse_stage = 0

            start_char = 0
            end_char = 0

            line = ""

            if ("line" in error_message):
                #Output the second line number
                first_line_removed_error_message = error_message[error_message.index("line")+4:len(error_message)]
                if ("line" in first_line_removed_error_message):
                    for char in range(first_line_removed_error_message.index("line"),len(first_line_removed_error_message)):
                        if parse_stage == 0:
                            if first_line_removed_error_message[char] == " ":
                                start_char = char+1
                                parse_stage = 1
                        elif parse_stage == 1:
                            if ((first_line_removed_error_message[char] == " ") or (first_line_removed_error_message[char] == ",")):
                                end_char = char
                                parse_stage = 2
                                break

            line = first_line_removed_error_message[start_char:end_char]

            parsed_error_message = ("PyRunner Error: The 'print' command in line "+ line +" must use brackets \nFor example: print('Hello World')")

        else:
            engine.print_terminal(error_message, True)
            engine.print_terminal("---" + script_state_container.get_script_name() + "'s script has terminated early---", False)
            return

        engine.print_terminal("first line number is "+parse_line_number(error_message,1), True)
        engine.print_terminal("second line number is "+parse_line_number(error_message,2), True)
        engine.print_terminal(error_message, True)
        engine.print_terminal(parsed_error_message, True)
        engine.print_terminal("---" + script_state_container.get_script_name() + "'s script has terminated early---", False)

        printed_flag[0] = False


    #Replace print statement in player script so that all their output goes to the terminal. (unless it has already been overidden)
    if not "print" in script_api:
        script_api["print"] = user_print

    #Instantiate the scoped intepreter

    if (parse_error):
        scoped_interpreter = ScopedInterpreter(script_api, lambda error_output: parse_output(error_output)) #create an instance of it
        #scoped_interpreter = ScopedInterpreter(script_api, lambda error_output: engine.print_terminal(parse_output(error_output), True)) #create an instance of it

    else:
        scoped_interpreter = ScopedInterpreter(script_api, lambda error_output: engine.print_terminal(error_output, True)) #create an instance of it
    script_filename = engine.get_config()['files']['player_scripts'] + "/"+str(script_name)+".py" #grab the absolute location of the script TODO: implement this path stuff in a config (ini) file!!!!!

    #open and read the script
    with open(script_filename, encoding="utf8") as script_file:
                script = script_file.read()

    def thread_target(callback = None):
        """ This is the method that is run in the seperate thread.

        It runs the script requested first and then runs the callback.
        the callback is therefore run in the seperate thread.
        """

        try:
            scoped_interpreter.runcode(script, HaltScriptException) #Run the script
        except HaltScriptException: #If an exception is sent to halt the script, catch it and act appropriately
            engine.print_terminal("Halted Script", True)
            printed_flag[0] = True
        finally:
            if printed_flag[0]:
                engine.print_terminal("---" + script_state_container.get_script_name() + "'s script has ended---", False)
            script_state_container.set_running_script_status(False)
            engine.set_finished()
            callback()

    thread = threading.Thread(target = lambda: thread_target(callback))

    engine.print_debug(script)

    thread.start()
    script_state_container.set_thread_id(thread.ident) #Save the player's thread id so that scripts can be halted
    return

def make_blocking(async_function):
    """ Takes an asynchronous function as an argument and returns a version of it that is blocking.

    Works by using callbacks to work out when the function has finished.
    Therefore the function must be a function which takes a callback as an argument which runs when event
    that the function initiates has finished.
    """
    def blocking_function():
        """ This is the blocking version of the async_function that is provided as and argument. """
        lock = threading.Lock()
        lock.acquire()
        async_function(callback = lock.release) #run the async_function with the callback provided above as its argument
        lock.acquire() # Try to a acquire a lock until it is released. It isn't released until the callback releases it.
        lock.release() # release the lock, it isn't needed anymore
        return

    return blocking_function

