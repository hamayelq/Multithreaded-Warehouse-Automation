# Warehouse Automation

## Quick Summary

Almost all information is stored in structs. In situations where a que is needed (a pile of pending
packages or robot queues), I implemented a linked list of package/robot structs. In other situations where a strict queue
was not necessary, I simply implemented global arrays of structs. In one situation, I have an array of linked lists - an array representing each team's robot queue. Code comments are extensive and go into depth about how I implemented 
my novel mutex algorithms and data structures for reference.'

## Output Explanation

I developed abbreviations/codes that signify what each robot is doing as it works through the pile of packages. 
Because so much information is printed (which I believe is relevant and something a real life warehouse automation company
may like to see), the output for each run is around two thousand lines long. This is also because there are 80 packages
to process, with each package having between 1 and 4 stations to be processed through.

The codes/abbreviations and their meanings are as follows:

BUSY: indicates that the robot cannot work on its package yet, because the team already has a robot working on a package

GRAB: self explanatory, signifies that a robot has grabbed a package

STRT: means start, indicates that a robot has started to work on a package

INST: means number of instructions for the package, the following information will tell the user how many instructions the package has

INFO: the array of instructions that a package has attached to it (in order)

MOVE: signifies that the robot is /attempting/ to move the package across a conveyor belt and its destination

WAIT: indicates that the station that the robot is attempting to move a package to is currently busy with a package and it must wait

WORK: indicates that a station was free, and that the robot is working on their package at that station

VLNT: indicates that a package is being shaken violently if it is fragile and if the station is the 'jostle' station

DONE: signifies that a robot is done working on a package at a particular station

FREE: indicates that the station is now free for use

CMLT: indicates that the robot is completely done with a package

