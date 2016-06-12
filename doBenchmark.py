#!/usr/bin/python3
#
# This file is part of the DEVS Ex Machina project.
# Copyright 2014 - 2015 University of Antwerp
# https://www.uantwerpen.be/en/
# Licensed under the EUPL V.1.1
# A full copy of the license is in COPYING.txt, or can be found at
# https://joinup.ec.europa.eu/community/eupl/og_page/eupl
#      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn
#

from benchmarks import benchmarker, defaults
from benchmarks.csv import toCSV
from benchmarks.misc import printVerbose
import multiprocessing
import argparse
import re
from collections import namedtuple
from itertools import chain
from pathlib import Path
from subprocess import call, DEVNULL
from functools import partial
from datetime import datetime


# small helper function for setting bounds on a argparse argument
def boundedValue(t, minv=None, maxv=None):
    """
    params:
      t     The type
      min   The min value
      max   The maximum value
    """
    def func(x):
        x = t(x)
        if minv is not None and x < minv:
            raise argparse.ArgumentTypeError("value should not be smaller than {}.".format(minv))
        if maxv is not None and x > maxv:
            raise argparse.ArgumentTypeError("value should not be greater than {}.".format(maxv))
        return x
    return func

def frange(minv, maxv, stepv):
    i = 0
    r = minv
    while r < maxv:
        r = minv + i * stepv
        yield r
        i += 1

# get the force compilation from the command line arguments
# -f force compilation
# -r repeat a number of times
# -c number of simulation cores
# limited state the names of the benchmarks you want to use
parser = argparse.ArgumentParser()
parser.add_argument("limited", nargs='*', default=set(),
    help="If set, only execute these benchmarks. Leave out to execute all."
    )
parser.add_argument("-f", "--force", action="store_true",
    help="force compilation, regardless of whether the executable already exists."
    )
parser.add_argument("--showSTDOUT", action="store_true",
    help="Allow subprocesses, such as compilation scripts and benchmarks, to send output to stdout."
    )
parser.add_argument("-r", "--repeat", type=boundedValue(int, minv=1), default=1,
    help="[default: 1] number of iterations. Must be at least 1."
    )
parser.add_argument("-c", "--cores", type=boundedValue(int, minv=1), default=multiprocessing.cpu_count(),
    help="[default: {0}] number of simulation cores for parallel simulation. Note that it is generally not smart to set this value higher than the amount of physical cpu cores. (You have {0}.)".format(multiprocessing.cpu_count())
    )
parser.add_argument("-t", "--endtime", type=boundedValue(int, minv=1), default=50,
    help="[default: 50] The end time of all benchmarks, must be at least 1."
    )
parser.add_argument("-T", "--timeout-time", type=boundedValue(int, minv=1), default=600,
    help="[default: 90] Timeout time for all benchmarks. When a benchmark takes more than this amount of seconds, it is terminated."
    )
parser.add_argument("-b", "--backup", action="store_true",
    help="Back up existing data files and then run the benchmarks as usual."
    )
parser.add_argument("-B", "--backup-no-benchmark", action="store_true",
    help="Back up existing data files and exit without running any benchmarks."
    )
parser.add_argument("-v", "--verbose", action="store_true",
    help="If true, produce more verbose output."
    )
parser.add_argument("-s", "--collectStats", type=str,
    help="If true, try to collect statistics. In this mode, benchmarks are executed in Release mode. Statistics are gathered from a 'stats.txt' file, so make sure you write them to that file."
    )
parser.add_argument("-e", "--regexp", nargs='*', default=[],
    help="If set, only execute benchmarks whose name matches a regular expression in the list. Can be combined with limited."
    )
args = parser.parse_args()
defaults.args = args
defaults.timeout = args.timeout_time

# executable names
devstoneEx = "./build/Benchmark/dxexmachina_devstone"
pholdEx = "./build/Benchmark/dxexmachina_phold"
connectEx = "./build/Benchmark/dxexmachina_interconnect"
networkEx = "./build/Benchmark/dxexmachina_network"
priorityEx = "./build/Benchmark/dxexmachina_priority"
pholdtreeEx = "./build/Benchmark/dxexmachina_pholdtree"
adevstoneEx = "./build/Benchmark/adevs_devstone"
adevpholdEx = "./build/Benchmark/adevs_phold"
adevconnectEx = "./build/Benchmark/adevs_interconnect"
adevnetworkEx = "./build/Benchmark/adevs_network"
adevpholdtreeEx = "./build/Benchmark/adevs_pholdtree"

# different simulation types
SimType = namedtuple('SimType', 'classic optimistic conservative')
simtypes = SimType(["classic"], ["opdevs", '-c', args.cores], ["cpdevs", '-c', args.cores])


# generators for the benchmark parameters
def devstonegen(simtype, executable, doRandom=False):
    # time 500 000
    if simtype == simtypes.classic :
        for depth in [10, 20, 30, 40]:  # , 4, 8, 16]:
            for endTime in [5000000]:
                yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', depth, '-d', depth, '-t', endTime]))  # , ['-r'] if randTa else []
                # return
    else:
        oldNumCores = simtype[-1]
        for core in range(2, min(48, args.cores+1), 1):
            simtype[-1] = core
            for depth in [10, 20, 30, 40]:  # , 4, 8, 16]:
                for endTime in [5000000]:
                    yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', depth, '-d', depth, '-t', endTime]))  # , ['-r'] if randTa else []
                    # return
        simtype[-1]=oldNumCores

randdevstonegen = partial(devstonegen, doRandom=True)
lenParallelDevstone = len(list(chain(["devExec"], simtypes.optimistic, ['-r', '-w', 0, '-d', 0, '-t', 0])))


# Cores must match nodes.
def pholdgen(simtype, executable):
    for nodes in [args.cores]:
        for apn in [4]:  # , 8, 16, 32]:
            for iterations in [0]:
                for remotes in [90]:
                    for priority in range(10, 100, 10):  #frange(0.0, 1.0, 0.1)
                        for endTime in [1000000]:                            
                            yield list(chain([executable], simtype, ['-n', nodes, '-s', apn, '-i', iterations, '-r', remotes, '-p', float(priority)/100, '-t', endTime]))
                            # return
lenParallelPhold = len(list(chain(["pholdExec"], simtypes.optimistic, ['-n', 0, '-s', 0, '-i', 0, '-r', 0, '-p', 0, '-t', 0])))

def pholdgen_remotes(simtype, executable):
    for nodes in [args.cores]:
        for apn in [4]:  # , 8, 16, 32]:
            for iterations in [0]:
                for priority in [0]:  #frange(0.0, 1.0, 0.1)
                    for remotes in range(10, 100, 10):
                        for endTime in [1000000]:                            
                            yield list(chain([executable], simtype, ['-n', nodes, '-s', apn, '-i', iterations, '-r', remotes, '-p', float(priority)/100, '-t', endTime]))
                            # return


def interconnectgen(simtype, executable, doRandom=False):
    if simtype == simtypes.optimistic or simtype == simtypes.conservative:
        print("Refusing to run interconnect parallel with this modelload!")
        return

    if simtype == simtypes.classic:
        # time 5 000 000
        for width in [10, 20, 30, 40, 50, 60, 70]:
            for endTime in [10000000]:
                yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', width, '-t', endTime]))
                # return
    else:
        # time 5 000 000
        oldNumCores = simtype[-1]
        for width in [10, 20, 30, 40, 50, 60, 70]:
            for endTime in [1000000]:
                for cores in [2, 4]:
                    simtype[-1] = cores
                    yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', width, '-t', endTime]))
                # simtype[-1] = oldNumCores; return
        simtype[-1] = oldNumCores

randconnectgen = partial(interconnectgen, doRandom=True)
lenParallelConnect = len(list(chain(["connectExec"], simtypes.optimistic, ['-r', '-w', 0, '-t', 0])))

def interconnectgen_speedup(simtype, executable, doRandom=False):

    if simtype == simtypes.classic:
        # time 5 000 000
        for width in [8]:
            for endTime in [4000000]:
                yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', width, '-t', endTime]))
                # return
    else:
        # time 5 000 000
        oldNumCores = simtype[-1]
        for width in [8]:
            for endTime in [4000000]:
                for cores in range(2, oldNumCores+1, 1):
                    simtype[-1] = cores
                    yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', width, '-t', endTime]))
                # simtype[-1] = oldNumCores; return
        simtype[-1] = oldNumCores


def networkgen(simtype, executable, feedback=False):
    for width in [10, 50, 100]:
        for endTime in [args.endtime]:
            yield list(chain([executable], simtype, ['-f' if feedback else '', '-w', width, '-t', endTime]))
            # return


feedbacknetworkgen = partial(networkgen, feedback=True)

lenParallelNetwork = len(list(chain(["networkExec"], simtypes.optimistic, ['-f', '-w', 0, '-t', 0])))


def prioritygen(simtype, executable):
    # time 50 000 000
    # for endTime in range(5000000, 50000000, 5000000):
    for endTime in [200000000]:
        # for endTime in [args.endtime]:
        for p in [90]:  # range(0, 100, 20):
            for n in [128]:
                for m in range(0, 17, 2):  # [0, 1, int(1/(p/100.0)) if p > 0 else 1]:
                    yield list(chain([executable, simtype[0]], ['-n', n, '-p', p, '-m', m, '-t', endTime]))
                    # return

def pholdtreegen(simtype, executable):
    if simtype == simtypes.classic :
        for fanout in [2, 3, 4]:
            for depth in [2, 3, 4]:  
                for priority in [0.1, 0.2, 0.4, 0.8]:  # frange(0.0, 1.0, 0.1)
                    for endTime in [50000000]:
                        yield list(chain([executable], simtype, ['-d', depth, '-n', fanout, '-p', priority, '-t', endTime]))
    else:
        oldNumCores = simtype[-1]
        for core in [2, 4, 8, 16]:
            simtype[-1] = core
            for depthFirst in [['-F'], []]:
                for fanout in [2, 3, 4]:
                    for depth in [2, 3, 4]:  
                        for priority in [0.1, 0.2, 0.4, 0.8]:  # frange(0.0, 1.0, 0.1)
                            for endTime in [50000000]:
                                yield list(chain([executable], simtype, depthFirst, ['-d', depth, '-n', fanout, '-p', priority, '-t', endTime]))
        simtype[-1]=oldNumCores

lenParallelPholdTree = len(list(chain(["devExec"], simtypes.classic, ['-d', 0, '-n', 0, '-p', 0, '-t', 0])))

csvDelim = ';'
devsArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"width"{0}"depth"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), x[3] if len(x) == lenParallelDevstone else 1, x[-5], x[-3], x[-1])]
pholdArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"nodes"{0}"atomics/node"{0}"iterations"{0}"% remotes"{0}"% priority"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), x[3] if len(x) == lenParallelPhold else 1, x[-11], x[-9], x[-7], x[-5], x[-3]*100.0, x[-1])]
pholdTreeArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"depth first allocation"{0}"depth"{0}"children"{0}"priority"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), 1 if len(x) == lenParallelPholdTree else x[3], '-F' in x, x[-7], x[-5], x[-3], x[-1])]
connectArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"width"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), x[3] if len(x) == lenParallelConnect else 1, x[-3], x[-1])]
networktArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"width"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), x[3] if len(x) == lenParallelNetwork else 1, x[-3], x[-1])]
priorityArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"nodes"{0}"priority"{0}"messages"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), 2 if [1] != "classic" else 1, x[-7], x[-5], x[-3], x[-1])]


# compilation functions
def unifiedCompiler(target, force=False):
    """
    Compiles the requested target executable, if it doesn't exist already.
    params:
      target    The name of the cmake target
      force     [default=False] use a true value to force compilation
    """
    path = Path('./build/Benchmark')/target
    if force or not path.exists():
        # if path.exists():   # the executable already exists. Remove it or make won't do anything
        #     path.unlink()
        print("Compiling target {}".format(target))
        compTargets = ['./setup.sh', '-b', '-f' if force else '', target]
        if args.collectStats:
            compTargets += ["-x", "-DSHOWSTAT=ON"]
        call(compTargets, stdout=None if args.showSTDOUT else DEVNULL)
        printVerbose(args.verbose, "  -> Compilation done.")

if args.collectStats:
    #we need to parse some statistics, try to append them to the perf stat file
    defExec = defaults.defaultDriver.execute
    from os import system as systemExec
    def fnNewExec(arg, path):
        systemExec("rm -f {}".format(args.collectStats))
        retVal = defExec(arg, path)
        if retVal:
            if args.verbose:
                print("Accumulating statistics from '{}' and appending them to '{}'".format(args.collectStats, str(path)))
            with open(args.collectStats) as infile, open(str(path), 'a') as outfile:
                totalVal = 0
                for l in infile:
                    spl = l.split()
                    if len(spl) >= 2:
                        totalVal += int(spl[0])
                outfile.write("\n{} output-events".format(totalVal))

        return retVal
    defaults.defaultDriver = defaults.BenchmarkDriver(defaults.defaultFilegen, defaults.defaultFoldergen, fnNewExec)


if __name__ == '__main__':
    # test if the build folder exists and has a makefile
    if not Path('setup.sh').exists():
        print("note: setup shellscript not found. Please make sure that it exists.")
        quit()

    dxdevc = partial(unifiedCompiler, 'dxexmachina_devstone', args.force)
    dxpholdc = partial(unifiedCompiler, 'dxexmachina_phold', args.force)
    dxconnectc = partial(unifiedCompiler, 'dxexmachina_interconnect', args.force)
    dxnetworkc = partial(unifiedCompiler, 'dxexmachina_network', args.force)
    dxpholdtreec = partial(unifiedCompiler, 'dxexmachina_pholdtree', args.force)
    dxpriorityc = partial(unifiedCompiler, 'dxexmachina_priority', args.force)
    adevc = partial(unifiedCompiler, 'adevs_devstone', args.force)
    apholdc = partial(unifiedCompiler, 'adevs_phold', args.force)
    apholdtreec = partial(unifiedCompiler, 'adevs_pholdtree', args.force)
    aconnectc = partial(unifiedCompiler, 'adevs_interconnect', args.force)
    anetworkc = partial(unifiedCompiler, 'adevs_network', args.force)
    apholdc = partial(unifiedCompiler, 'adevs_pholdtree', args.force)
    dxdevstone = SimType(
        defaults.Benchmark('devstone/classic', dxdevc, partial(devstonegen, simtypes.classic, devstoneEx), "dxexmachina devstone, classic"),
        defaults.Benchmark('devstone/optimistic', dxdevc, partial(devstonegen, simtypes.optimistic, devstoneEx), "dxexmachina devstone, optimistic"),
        defaults.Benchmark('devstone/conservative', dxdevc, partial(devstonegen, simtypes.conservative, devstoneEx), "dxexmachina devstone, conservative"),
        )
    dxranddevstone = SimType(
        defaults.Benchmark('randdevstone/classic', dxdevc, partial(randdevstonegen, simtypes.classic, devstoneEx), "dxexmachina randomized devstone, classic"),
        defaults.Benchmark('randdevstone/optimistic', dxdevc, partial(randdevstonegen, simtypes.optimistic, devstoneEx), "dxexmachina randomized devstone, optimistic"),
        defaults.Benchmark('randdevstone/conservative', dxdevc, partial(randdevstonegen, simtypes.conservative, devstoneEx), "dxexmachina randomized devstone, conservative"),
        )
    dxphold = SimType(
        defaults.Benchmark('phold/classic', dxpholdc, partial(pholdgen, simtypes.classic, pholdEx), "dxexmachina phold, classic"),
        defaults.Benchmark('phold/optimistic', dxpholdc, partial(pholdgen, simtypes.optimistic, pholdEx), "dxexmachina phold, optimistic"),
        defaults.Benchmark('phold/conservative', dxpholdc, partial(pholdgen, simtypes.conservative, pholdEx), "dxexmachina phold, conservative"),
        )
    dxphold_remotes = SimType(
        defaults.Benchmark('phold_remotes/classic', dxpholdc, partial(pholdgen_remotes, simtypes.classic, pholdEx), "dxexmachina phold, classic"),
        defaults.Benchmark('phold_remotes/optimistic', dxpholdc, partial(pholdgen_remotes, simtypes.optimistic, pholdEx), "dxexmachina phold, optimistic"),
        defaults.Benchmark('phold_remotes/conservative', dxpholdc, partial(pholdgen_remotes, simtypes.conservative, pholdEx), "dxexmachina phold, conservative"),
        )
    dxconnect = SimType(
        defaults.Benchmark('connect/classic', dxconnectc, partial(interconnectgen, simtypes.classic, connectEx), "dxexmachina high interconnect, classic"),
        defaults.Benchmark('connect/optimistic', dxconnectc, partial(interconnectgen, simtypes.optimistic, connectEx), "dxexmachina high interconnect, optimistic"),
        defaults.Benchmark('connect/conservative', dxconnectc, partial(interconnectgen, simtypes.conservative, connectEx), "dxexmachina high interconnect, conservative"),
        )
    dxconnect_speedup = SimType(
        defaults.Benchmark('connect_speedup/classic', dxconnectc, partial(interconnectgen_speedup, simtypes.classic, connectEx), "dxexmachina high interconnect, classic"),
        defaults.Benchmark('connect_speedup/optimistic', dxconnectc, partial(interconnectgen_speedup, simtypes.optimistic, connectEx), "dxexmachina high interconnect, optimistic"),
        defaults.Benchmark('connect_speedup/conservative', dxconnectc, partial(interconnectgen_speedup, simtypes.conservative, connectEx), "dxexmachina high interconnect, conservative"),
        )
    dxrandconnect = SimType(
        defaults.Benchmark('randconnect/classic', dxconnectc, partial(randconnectgen, simtypes.classic, connectEx), "dxexmachina randomized high interconnect, classic"),
        defaults.Benchmark('randconnect/optimistic', dxconnectc, partial(randconnectgen, simtypes.optimistic, connectEx), "dxexmachina randomized high interconnect, optimistic"),
        defaults.Benchmark('randconnect/conservative', dxconnectc, partial(randconnectgen, simtypes.conservative, connectEx), "dxexmachina randomized high interconnect, conservative"),
        )
    dxnetwork = SimType(
        defaults.Benchmark('network/classic', dxnetworkc, partial(networkgen, simtypes.classic, networkEx), "dxexmachina queue network, classic"),
        defaults.Benchmark('network/optimistic', dxnetworkc, partial(networkgen, simtypes.optimistic, networkEx), "dxexmachina queue network, optimistic"),
        defaults.Benchmark('network/conservative', dxnetworkc, partial(networkgen, simtypes.conservative, networkEx), "dxexmachina queue network, conservative"),
        )
    dxfeednetwork = SimType(
        defaults.Benchmark('feednetwork/classic', dxnetworkc, partial(feedbacknetworkgen, simtypes.classic, networkEx), "dxexmachina queue network with feedback loop, classic"),
        defaults.Benchmark('feednetwork/optimistic', dxnetworkc, partial(feedbacknetworkgen, simtypes.optimistic, networkEx), "dxexmachina queue network with feedback loop, optimistic"),
        defaults.Benchmark('feednetwork/conservative', dxnetworkc, partial(feedbacknetworkgen, simtypes.conservative, networkEx), "dxexmachina queue network with feedback loop, conservative"),
        )
    dxpriority = SimType(
        None,
        defaults.Benchmark('priority/optimistic', dxpriorityc, partial(prioritygen, simtypes.optimistic, priorityEx), "dxexmachina priority model, optimistic"),
        defaults.Benchmark('priority/conservative', dxpriorityc, partial(prioritygen, simtypes.conservative, priorityEx), "dxexmachina priority model, conservative"),
        )
    dxpholdtree = SimType(
        defaults.Benchmark('pholdtree/classic', dxpholdtreec, partial(pholdtreegen, simtypes.classic, pholdtreeEx), "dxexmachina pholdtree, classic"),
        defaults.Benchmark('pholdtree/optimistic', dxpholdtreec, partial(pholdtreegen, simtypes.optimistic, pholdtreeEx), "dxexmachina pholdtree, optimistic"),
        defaults.Benchmark('pholdtree/conservative', dxpholdtreec, partial(pholdtreegen, simtypes.conservative, pholdtreeEx), "dxexmachina pholdtree, conservative"),
        )
    adevstone = SimType(
        defaults.Benchmark('adevstone/classic', adevc, partial(devstonegen, simtypes.classic, adevstoneEx), "adevs devstone, classic"),
        None,
        defaults.Benchmark('adevstone/conservative', adevc, partial(devstonegen, simtypes.conservative, adevstoneEx), "adevs devstone, conservative"),
        )
    aranddevstone = SimType(
        defaults.Benchmark('aranddevstone/classic', adevc, partial(randdevstonegen, simtypes.classic, adevstoneEx), "adevs randomized devstone, classic"),
        None,
        defaults.Benchmark('aranddevstone/conservative', adevc, partial(randdevstonegen, simtypes.conservative, adevstoneEx), "adevs randomized devstone, conservative"),
        )
    aphold = SimType(
        defaults.Benchmark('aphold/classic', apholdc, partial(pholdgen, simtypes.classic, adevpholdEx), "adevs phold, classic"),
        None,
        defaults.Benchmark('aphold/conservative', apholdc, partial(pholdgen, simtypes.conservative, adevpholdEx), "adevs phold, conservative"),
        )
    aphold_remotes = SimType(
        defaults.Benchmark('aphold_remotes/classic', apholdc, partial(pholdgen_remotes, simtypes.classic, adevpholdEx), "adevs phold, classic"),
        None,
        defaults.Benchmark('aphold_remotes/conservative', apholdc, partial(pholdgen_remotes, simtypes.conservative, adevpholdEx), "adevs phold, conservative"),
        )
    aconnect = SimType(
        defaults.Benchmark('aconnect/classic', aconnectc, partial(interconnectgen, simtypes.classic, adevconnectEx), "adevs high interconnect, classic"),
        None,
        defaults.Benchmark('aconnect/conservative', aconnectc, partial(interconnectgen, simtypes.conservative, adevconnectEx), "adevs high interconnect, conservative"),
        )
    arandconnect = SimType(
        defaults.Benchmark('arandconnect/classic', aconnectc, partial(randconnectgen, simtypes.classic, adevconnectEx), "adevs randomized high interconnect, classic"),
        None,
        defaults.Benchmark('arandconnect/conservative', aconnectc, partial(randconnectgen, simtypes.conservative, adevconnectEx), "adevs randomized high interconnect, conservative"),
        )
    aconnect_speedup = SimType(
        defaults.Benchmark('aconnect_speedup/classic', aconnectc, partial(interconnectgen_speedup, simtypes.classic, adevconnectEx), "adevs high interconnect, classic"),
        None,
        defaults.Benchmark('aconnect_speedup/conservative', aconnectc, partial(interconnectgen_speedup, simtypes.conservative, adevconnectEx), "adevs high interconnect, conservative"),
        )
    anetwork = SimType(
        defaults.Benchmark('anetwork/classic', anetworkc, partial(networkgen, simtypes.classic, adevnetworkEx), "adevs queue network, classic"),
        None,
        defaults.Benchmark('anetwork/conservative', anetworkc, partial(networkgen, simtypes.conservative, adevnetworkEx), "adevs queue network, conservative"),
        )
    afeednetwork = SimType(
        defaults.Benchmark('afeednetwork/classic', anetworkc, partial(feedbacknetworkgen, simtypes.classic, adevnetworkEx), "adevs queue network with feedback loop, classic"),
        None,
        defaults.Benchmark('afeednetwork/conservative', anetworkc, partial(feedbacknetworkgen, simtypes.conservative, adevnetworkEx), "adevs queue network with feedback loop, conservative"),
        )
    apholdtree = SimType(
        defaults.Benchmark('apholdtree/classic', apholdtreec, partial(pholdtreegen, simtypes.classic, adevpholdtreeEx), "adevs pholdtree, classic"),
        None,
        defaults.Benchmark('apholdtree/conservative', apholdtreec, partial(pholdtreegen, simtypes.conservative, adevpholdtreeEx), "adevs pholdtree, conservative"),
        )
    allBenchmark = [dxdevstone, dxranddevstone,
                    dxphold,dxphold_remotes,
                    dxconnect, dxrandconnect,
                    dxconnect_speedup,
                    dxnetwork, dxfeednetwork,
                    dxpriority, dxpholdtree,
                    adevstone, aranddevstone,
                    aphold,aphold_remotes,
                    aconnect, arandconnect,
                    aconnect_speedup,
                    anetwork, afeednetwork,
                    apholdtree]
    bmarkArgParses = [devsArg, devsArg,
                      pholdArg,pholdArg,
                      connectArg, connectArg,
                      connectArg,
                      networktArg, networktArg,
                      priorityArg, pholdTreeArg,
                      devsArg, devsArg,
                      pholdArg, pholdArg,
                      connectArg, connectArg,
                      connectArg,
                      networktArg, networktArg,
                      pholdTreeArg]
    if len(allBenchmark) != len(bmarkArgParses):
        raise 42
    # do all the preparation stuff
    driver = defaults.defaultDriver
    analyzer = defaults.perfAnalyzer
    sysInfo = defaults.getSysInfo()
    folders = defaults.getOFolders(sysInfo, "bmarkdata")
    defaults.saveSysInfo(sysInfo, folders)

    if args.backup or args.backup_no_benchmark:
        # do the backup
        print("Backing up current data files...")
        for i in chain(*allBenchmark):
            if i is not None:
                date = '{0:%Y}.{0:%m}.{0:%d}_{0:%H}.{0:%M}.{0:%S}'.format(datetime.now())
                plotPath = (folders.plots / "{}.csv".format(i.name), folders.plots / "{}_{}.csv".format(i.name, date))
                jsonPath = (folders.json / "{}.json".format(i.name), folders.json / "{}_{}.json".format(i.name, date))
                for curPath, newPath in [plotPath, jsonPath]:
                    if not curPath.exists():
                        printVerbose(args.verbose, "  > could not find {}, skipping this file.".format(curPath))
                        continue
                    curPath.rename(newPath)
                    printVerbose(args.verbose, "  > moved {} \t-> {}".format(curPath, newPath))
        print("done!")
        if args.backup_no_benchmark:
            exit(0)

    # do the benchmarks!
    print("args.limited: {}".format(args.limited))
    allNames = [b.name for b in chain(*allBenchmark) if b is not None]
    if len(args.limited) == 0 and len(args.regexp) == 0:
        args.limited = allNames
    else:
        args.limited = set(args.limited)
    # get additional names from the regular expressions
    regexpfail = []
    if len(args.regexp) != 0:
        regexList = []
        for regexp in args.regexp:
            try:
                compiledRegExp = re.compile(regexp)
            except:
                regexpfail.append(regexp)
                continue
            for i in allNames:
                if compiledRegExp.fullmatch(i):
                    regexList.append(i)
        args.limited.update(regexList)
    print("args.limited: {}".format(args.limited))

    print("Executing benchmarks...")
    donelist = []
    for bmarkset, argp in zip(allBenchmark, bmarkArgParses):
        doCompile = True
        for bmark in bmarkset:
            if bmark is None or (args.limited is not allNames and bmark.name not in args.limited):
                continue
            print("  > executing benchmark {}".format(bmark.name))
            donelist.append(bmark.name)
            data = analyzer.open(bmark, folders)
            for i in range(args.repeat):
                files = benchmarker.execBenchmark(bmark, driver, folders, doCompile)
                data = analyzer.read(files, data)
                doCompile = False
            analyzer.save(data, bmark, folders)
            toCSV(data, folders.plots / "{}.csv".format(bmark.name), *argp)
    if len(donelist) > 0:
        print("done!")

    printAccepted = False
    if len(args.limited) > 0:
        wrong = set(args.limited) - set(donelist)
        if len(wrong) > 0:
            print("Unknown benchmarks requested:")
            for i in wrong:
                print("    {}".format(i))
            printAccepted = True
    if len(regexpfail) > 0:
        print("Badly formatted regular expressions:")
        for i in regexpfail:
            print("    {}".format(i))
        printAccepted = True
    if printAccepted:
        print("Accepted benchmarks:")
        for i in allNames:
            if i is not None:
                print("    {}".format(i))
    if len(defaults.timeouts) > 0:
        print("A total of {} benchmarks timed out after a waiting period of {} seconds:".format(len(defaults.timeouts), defaults.timeout))
        for i in defaults.timeouts:
            print("  {}".format(i))
