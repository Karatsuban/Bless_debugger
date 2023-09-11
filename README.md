# Bless_debugger
A handy tool for quickly debugging scripts from the command line

# Introduction

Bless is a small program made to solve one specific issue:  
+ Do you use Linux?
+ Do you happen to write code and test it with the same console?
+ Do you usually get a ton of error/warning from the code you write?
+ Are you tired of switching back and forth between the errors and your text editor?  

Well, if you answered yes to all of the above, search no more, you've arrived at the right place.  
Introducing _Bless_, a small all-in-one editor/debugger using the power of Curses to show you the errors you got and edit them within the same screen.

# Functionalities

## Existing functionalities

Simply give Bless the command you want to test, with all of its parameters and let it do the rest.  
The error output of the command is parsed and transformed into error object.  
These errors are then shown to the user who can flip through them and edit them as they like.  
The supported functionalities are:
+ Parsing the output of the gcc command
+ Viewing and editing errors
+ Compacting all the errors of the same line in same screen
+ Replacing each error line in file
+ Relaunching the command


## Future functionalities

For now, Bless can only be used with the output of the gcc command.  

+ add template files for other languages than C
+ add automatic detection of extensions/tools used to chose the right template
+ Possiblity to save each error individually
+ Possibility to revert all changes made to error line

# Tests

For the time being, no serious tests have been made on this software, use this at your own risk !  


# TODO list

+ add a reset button to go back to the origin-code (untouched by the user)
+ add a save file/save all button
+ add a save file function : saving only the error being read by the user
+ mofify the actual save all to call save file
+ add a member of the ErrorNode struct to check wether this specific error has been edited or not
+ add tests/QC
