#!/usr/bin/python3
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
    help="[default: false] force compilation, regardless of whether the executable already exists."
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
parser.add_argument("-b", "--backup", action="store_true",
    help="Back up existing data files and then run the benchmarks as usual."
    )
parser.add_argument("-B", "--backup-no-benchmark", action="store_true",
    help="Back up existing data files and exit without running any benchmarks."
    )
parser.add_argument("-v", "--verbose", action="store_true",
    help="If true, produce more verbose output."
    )
parser.add_argument("-e", "--regexp", nargs='*', default=[],
    help="If set, only execute benchmarks whose name matches a regular expression in the list. Can be combined with limited."
    )
args = parser.parse_args()
defaults.args = args

# executable names
devstoneEx = "./build/Benchmark/dxexmachina_devstone"
pholdEx = "./build/Benchmark/dxexmachina_phold"
connectEx = "./build/Benchmark/dxexmachina_interconnect"
adevstoneEx = "./build/Benchmark/adevs_devstone"
adevpholdEx = "./build/Benchmark/adevs_phold"
adevconnectEx = "./build/Benchmark/adevs_interconnect"

# different simulation types
SimType = namedtuple('SimType', 'classic optimistic conservative')
simtypes = SimType(["classic"], ["opdevs", '-c', args.cores], ["cpdevs", '-c', args.cores])


# generators for the benchmark parameters
def devstonegen(simtype, executable, doRandom=False):
    for depth in [1, 2, 3]:  # , 4, 8, 16]:
        for width in [2, 3, 4]:  # , 8, 16]:
            for endTime in [50]:
                if simtype is not simtypes.classic and (depth*width+1) < simtype[2]:
                    continue
                yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', width, '-d', depth, '-t', endTime]))  # , ['-r'] if randTa else []
                # return

randdevstonegen = partial(devstonegen, doRandom=True)


def pholdgen(simtype, executable):
    for apn in [2, 4, 8, 16]:
        for nodes in [2, 4]:  # , 8, 16, 32]:
            for iterations in [16, 64, 256, 1024]:
                for remotes in [10]:
                    for endTime in [50]:
                        yield list(chain([executable], simtype, ['-n', nodes, '-s', apn, '-i', iterations, '-r', remotes, '-t', endTime]))
                        # return


def interconnectgen(simtype, executable, doRandom=False):
    for width in [2, 4, 8, 16]:
        for endTime in [50]:
            yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', width, '-t', endTime]))

randconnectgen = partial(interconnectgen, doRandom=True)

csvDelim = ';'
devsArg = [csvDelim, """ "command"{0}"executable"{0}"width"{0}"depth"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), x[-5], x[-3], x[-1])]
pholdArg = [csvDelim, """ "command"{0}"executable"{0}"nodes"{0}"atomics/node"{0}"iterations"{0}"% remotes"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), x[-9], x[-7], x[-5], x[-3], x[-1])]
connectArg = [csvDelim, """ "command"{0}"executable"{0}"width"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), x[-3], x[-1])]


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
        call(['./setup.sh', '-b', target], stdout=None if args.showSTDOUT else DEVNULL)
        printVerbose(args.verbose, "  -> Compilation done.")


if __name__ == '__main__':
    # test if the build folder exists and has a makefile
    if not Path('./build').exists() or not Path('./build/Makefile').exists():
        print("note: this script assumes that the cmake script has run at least once and that the './build/Makefile' file exists.")
        print("Please do so and then rerun the benchmark script.")
        quit()

    dxdevc = partial(unifiedCompiler, 'dxexmachina_devstone', args.force)
    dxpholdc = partial(unifiedCompiler, 'dxexmachina_phold', args.force)
    dxconnectc = partial(unifiedCompiler, 'dxexmachina_interconnect', args.force)
    adevc = partial(unifiedCompiler, 'adevs_devstone', args.force)
    apholdc = partial(unifiedCompiler, 'adevs_phold', args.force)
    aconnectc = partial(unifiedCompiler, 'adevs_interconnect', args.force)
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
    dxconnect = SimType(
        defaults.Benchmark('connect/classic', dxpholdc, partial(interconnectgen, simtypes.classic, connectEx), "dxexmachina high interconnect, classic"),
        defaults.Benchmark('connect/optimistic', dxpholdc, partial(interconnectgen, simtypes.optimistic, connectEx), "dxexmachina high interconnect, optimistic"),
        defaults.Benchmark('connect/conservative', dxpholdc, partial(interconnectgen, simtypes.conservative, connectEx), "dxexmachina high interconnect, conservative"),
        )
    dxrandconnect = SimType(
        defaults.Benchmark('randconnect/classic', dxpholdc, partial(randconnectgen, simtypes.classic, connectEx), "dxexmachina randomized high interconnect, classic"),
        defaults.Benchmark('randconnect/optimistic', dxpholdc, partial(randconnectgen, simtypes.optimistic, connectEx), "dxexmachina randomized high interconnect, optimistic"),
        defaults.Benchmark('randconnect/conservative', dxpholdc, partial(randconnectgen, simtypes.conservative, connectEx), "dxexmachina randomized high interconnect, conservative"),
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
    aconnect = SimType(
        defaults.Benchmark('aconnect/classic', aconnectc, partial(interconnectgen, simtypes.classic, adevconnectEx), "adevs high interconnect, classic"),
        None,
        defaults.Benchmark('aconnect/conservative', aconnectc, partial(interconnectgen, simtypes.conservative, adevconnectEx), "adevs high interconnect, conservative"),
        )
    arandconnect = SimType(
        defaults.Benchmark('arandconnect/classic', aconnectc, partial(partial(interconnectgen, doRandom=True), simtypes.classic, adevconnectEx), "adevs randomized high interconnect, classic"),
        None,
        defaults.Benchmark('arandconnect/conservative', aconnectc, partial(partial(interconnectgen, doRandom=True), simtypes.conservative, adevconnectEx), "adevs randomized high interconnect, conservative"),
        )
    allBenchmark = [dxdevstone, dxranddevstone,
                    dxphold,
                    dxconnect, dxrandconnect,
                    adevstone, aranddevstone,
                    aphold,
                    aconnect, arandconnect]
    bmarkArgParses = [devsArg, devsArg,
                      pholdArg,
                      connectArg, connectArg,
                      devsArg, devsArg,
                      pholdArg,
                      connectArg, connectArg]
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
