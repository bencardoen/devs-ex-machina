#!/usr/bin/python
# A very basic timesaver to profile benchmarks.
#@author Ben Cardoen

import sys
from subprocess import call, Popen, PIPE, check_call, CalledProcessError

if(len(sys.argv)<2):
    print "Expecting at least 1 command to profile"
    exit()

profiler = ["perf", "record", "-g", "--call-graph", "dwarf"]
command = sys.argv[1:]
print "Executing {} \nwith profiler {}".format(command, profiler)
try:
    check_call(profiler+command)
except CalledProcessError as e :
    print "Failed executing {} with rcode == {}".format(e.cmd, e.returncode)
    exit()

print "Profiling done, calling analyzer."

commandstring = ''.join(command)
analyzer= "perf script | gprof2dot -s -f perf | dot -Tsvg -o "
try:
    check_call(analyzer+commandstring+".svg", shell=True)
except CalledProcessError as e:
    print "Failed executing analyzer {} with rcode == {}".format(e.cmd, e.returncode)
    exit()
if commandstring.startswith("./"):
    commandstring = commandstring[2:]
print "SVG file {} written to local dir".format(commandstring+".svg")
