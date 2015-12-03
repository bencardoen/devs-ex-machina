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
parser.add_argument("-T", "--timeout-time", type=boundedValue(int, minv=1), default=90,
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
adevstoneEx = "./build/Benchmark/adevs_devstone"
adevpholdEx = "./build/Benchmark/adevs_phold"
adevconnectEx = "./build/Benchmark/adevs_interconnect"
adevnetworkEx = "./build/Benchmark/adevs_network"

# different simulation types
SimType = namedtuple('SimType', 'classic optimistic conservative')
simtypes = SimType(["classic"], ["opdevs", '-c', args.cores], ["cpdevs", '-c', args.cores])


# generators for the benchmark parameters
def devstonegen(simtype, executable, doRandom=False):
    # time 500 000
    for depth in [10, 15, 20, 25, 30]:  # , 4, 8, 16]:
        for endTime in [500000]:
            yield list(chain([executable], simtype, ['-r' if doRandom else '', '-w', depth, '-d', depth, '-t', endTime]))  # , ['-r'] if randTa else []
            # return

randdevstonegen = partial(devstonegen, doRandom=True)
lenParallelDevstone = len(list(chain(["devExec"], simtypes.optimistic, ['-r', '-w', 0, '-d', 0, '-t', 0])))


# Cores must match nodes.
def pholdgen(simtype, executable):
    # time single 5 000 000
    # single core s=[2, 4, 8, 16], r=[10]
    # for single & conservative
    if args.cores != 4:
        print("Refusing to run benchmark with != 4 cores.")
        return
    for nodes in [args.cores]:
        for apn in [2, 4, 8, 16]:  # , 8, 16, 32]:
            for iterations in [0]:
                for remotes in [10]:
                    for endTime in [1000000]:
                        yield list(chain([executable], simtype, ['-n', nodes, '-s', apn, '-i', iterations, '-r', remotes, '-t', endTime]))
                        # return
lenParallelPhold = len(list(chain(["pholdExec"], simtypes.optimistic, ['-n', 0, '-s', 0, '-i', 0, '-r', 0, '-t', 0])))


def interconnectgen(simtype, executable, doRandom=False):
    if simtype == simtypes.optimistic:
        print("Refusing to run interconnect with optimistic!")
        return
    if simtype == simtypes.classic:
        # time 5 000 000
        for width in [10, 20, 30, 40, 50, 60, 70]:
            for endTime in [1000000]:
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


csvDelim = ';'
devsArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}width"{0}"depth"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), x[3] if len(x) == lenParallelDevstone else 1, x[-5], x[-3], x[-1])]
pholdArg = [csvDelim, """ "command"{0}"executable"{0}"simtype"{0}"ncores"{0}"nodes"{0}"atomics/node"{0}"iterations"{0}"% remotes"{0}"end time" """.format(csvDelim), lambda x: ("\"{}\"".format(" ".join(map(str, x))), "\"{0}\"".format(x[0].split('/')[-1]), "\"{}\"".format(x[1]), x[3] if len(x) == lenParallelPhold else 1, x[-9], x[-7], x[-5], x[-3], x[-1])]
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
        call(['./setup.sh', '-b', '-f' if force else '', target], stdout=None if args.showSTDOUT else DEVNULL)
        printVerbose(args.verbose, "  -> Compilation done.")


if __name__ == '__main__':
    # test if the build folder exists and has a makefile
    if not Path('setup.sh').exists():
        print("note: setup shellscript not found. Please make sure that it exists.")
        quit()

    dxdevc = partial(unifiedCompiler, 'dxexmachina_devstone', args.force)
    dxpholdc = partial(unifiedCompiler, 'dxexmachina_phold', args.force)
    dxconnectc = partial(unifiedCompiler, 'dxexmachina_interconnect', args.force)
    dxnetworkc = partial(unifiedCompiler, 'dxexmachina_network', args.force)
    dxpriorityc = partial(unifiedCompiler, 'dxexmachina_priority', args.force)
    adevc = partial(unifiedCompiler, 'adevs_devstone', args.force)
    apholdc = partial(unifiedCompiler, 'adevs_phold', args.force)
    aconnectc = partial(unifiedCompiler, 'adevs_interconnect', args.force)
    anetworkc = partial(unifiedCompiler, 'adevs_network', args.force)
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
        defaults.Benchmark('connect/classic', dxconnectc, partial(interconnectgen, simtypes.classic, connectEx), "dxexmachina high interconnect, classic"),
        defaults.Benchmark('connect/optimistic', dxconnectc, partial(interconnectgen, simtypes.optimistic, connectEx), "dxexmachina high interconnect, optimistic"),
        defaults.Benchmark('connect/conservative', dxconnectc, partial(interconnectgen, simtypes.conservative, connectEx), "dxexmachina high interconnect, conservative"),
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
        defaults.Benchmark('arandconnect/classic', aconnectc, partial(randconnectgen, simtypes.classic, adevconnectEx), "adevs randomized high interconnect, classic"),
        None,
        defaults.Benchmark('arandconnect/conservative', aconnectc, partial(randconnectgen, simtypes.conservative, adevconnectEx), "adevs randomized high interconnect, conservative"),
        )
    anetwork = SimType(
        defaults.Benchmark('network/classic', anetworkc, partial(networkgen, simtypes.classic, adevnetworkEx), "adevs queue network, classic"),
        None,
        defaults.Benchmark('network/conservative', anetworkc, partial(networkgen, simtypes.conservative, adevnetworkEx), "adevs queue network, conservative"),
        )
    afeednetwork = SimType(
        defaults.Benchmark('feednetwork/classic', anetworkc, partial(feedbacknetworkgen, simtypes.classic, adevnetworkEx), "adevs queue network with feedback loop, classic"),
        None,
        defaults.Benchmark('feednetwork/conservative', anetworkc, partial(feedbacknetworkgen, simtypes.conservative, adevnetworkEx), "adevs queue network with feedback loop, conservative"),
        )
    allBenchmark = [dxdevstone, dxranddevstone,
                    dxphold,
                    dxconnect, dxrandconnect,
                    dxnetwork, dxfeednetwork,
                    dxpriority,
                    adevstone, aranddevstone,
                    aphold,
                    aconnect, arandconnect,
                    anetwork, afeednetwork]
    bmarkArgParses = [devsArg, devsArg,
                      pholdArg,
                      connectArg, connectArg,
                      networktArg, networktArg,
                      priorityArg,
                      devsArg, devsArg,
                      pholdArg,
                      connectArg, connectArg,
                      networktArg, networktArg]
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
