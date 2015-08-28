#!/usr/bin/python3
from benchmarks import benchmarker, defaults
from benchmarks.csv import toCSV
import multiprocessing
import argparse
from collections import namedtuple
from itertools import chain
from pathlib import Path
from subprocess import call
from functools import partial


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
parser.add_argument("-f", "--force", action="store_true",
    help="[default: false] force compilation, regardless of whether the executable already exists."
    )
parser.add_argument("-r", "--repeat", type=boundedValue(int, minv=1), default=1,
    help="[default: 1] number of iterations. Must be at least 1."
    )
parser.add_argument("-c", "--cores", type=boundedValue(int, minv=1), default=multiprocessing.cpu_count(),
    help="[default: {0}] number of simulation cores for parallel simulation. Note that it is generally not smart to set this value higher than the amount of physical cpu cores. (You have {0}.)".format(multiprocessing.cpu_count())
    )
parser.add_argument("limited", nargs='*', default=None,
    help="If set, only execute these benchmarks. Leave out to execute all."
    )
args = parser.parse_args()

# executable names
devstoneEx = "./build/dxexmachina_devstone"
pholdEx = "./build/dxexmachina_phold"
adevstoneEx = "./build/adevs_devstone"
adevpholdEx = "./build/adevs_phold"

# different simulation types
SimType = namedtuple('SimType', 'classic optimistic conservative')
simtypes = SimType(["classic"], ["opdevs", '-c', args.cores], ["cpdevs", '-c', args.cores])


# generators for the benchmark parameters
def devstonegen(simtype, executable):
    for depth in [1, 2, 3]:  # , 4, 8, 16]:
        for width in [2, 3, 4]:  # , 8, 16]:
            if simtype is not simtypes.classic and (depth*width+1) < simtype[2]:
                continue
            yield list(chain([executable], simtype, ['-w', width, '-d', depth]))  # ['-t', endTime], ['-r'] if randTa else []
            # return


def pholdgen(simtype, executable):
    for apn in [2, 4, 8, 16]:
        for nodes in [2, 4]:  # , 8, 16, 32]:
            for iterations in [16, 64, 256, 1024]:
                for remotes in [10]:
                    yield list(chain([executable], simtype, ['-n', nodes, '-s', apn, '-i', iterations, '-r', remotes]))  # ['-t', endTime]
                    # return


# compilation functions
def unifiedCompiler(target, force=False):
    """
    Compiles the requested target executable, if it doesn't exist already.
    params:
      target    The name of the cmake target
      force     [default=False] use a true value to force compilation
    """
    path = Path('./build')/target
    if force or not path.exists():
        # if path.exists():   # the executable already exists. Remove it or make won't do anything
        #     path.unlink()
        call(['make', '--always-make', target], cwd='./build')


if __name__ == '__main__':
    # test if the build folder exists and has a makefile
    if not Path('./build').exists() or not Path('./build/Makefile').exists():
        print("note: this script assumes that the cmake script has run at least once and that the './build/Makefile' file exists.")
        print("Please do so and then rerun the benchmark script.")
        quit()

    dxdevc = partial(unifiedCompiler, 'dxexmachina_devstone', args.force)
    dxpholdc = partial(unifiedCompiler, 'dxexmachina_phold', args.force)
    adevc = partial(unifiedCompiler, 'adevs_devstone', args.force)
    apholdc = partial(unifiedCompiler, 'adevs_phold', args.force)
    dxdevstone = SimType(
        defaults.Benchmark('devstone/classic', dxdevc, partial(devstonegen, simtypes.classic, devstoneEx), "dxexmachina devstone, classic"),
        defaults.Benchmark('devstone/optimistic', dxdevc, partial(devstonegen, simtypes.optimistic, devstoneEx), "dxexmachina devstone, optimistic"),
        defaults.Benchmark('devstone/conservative', dxdevc, partial(devstonegen, simtypes.conservative, devstoneEx), "dxexmachina devstone, conservative"),
        )
    dxphold = SimType(
        defaults.Benchmark('phold/classic', dxpholdc, partial(pholdgen, simtypes.classic, pholdEx), "dxexmachina phold, classic"),
        defaults.Benchmark('phold/optimistic', dxpholdc, partial(pholdgen, simtypes.optimistic, pholdEx), "dxexmachina phold, optimistic"),
        defaults.Benchmark('phold/conservative', dxpholdc, partial(pholdgen, simtypes.conservative, pholdEx), "dxexmachina phold, conservative"),
        )
    adevstone = SimType(
        defaults.Benchmark('adevstone/classic', adevc, partial(devstonegen, simtypes.classic, adevstoneEx), "adevs devstone, classic"),
        None,
        defaults.Benchmark('adevstone/conservative', adevc, partial(devstonegen, simtypes.conservative, adevstoneEx), "adevs devstone, conservative"),
        )
    aphold = SimType(
        defaults.Benchmark('aphold/classic', apholdc, partial(pholdgen, simtypes.classic, adevpholdEx), "adevs phold, classic"),
        None,
        defaults.Benchmark('aphold/conservative', apholdc, partial(pholdgen, simtypes.conservative, adevpholdEx), "adevs phold, conservative"),
        )
    allBenchmark = [dxdevstone, dxphold, adevstone, aphold]
    # do all the preparation stuff
    driver = defaults.defaultDriver
    analyzer = defaults.perfAnalyzer
    sysInfo = defaults.getSysInfo()
    folders = defaults.getOFolders(sysInfo, "bmarkdata")
    defaults.saveSysInfo(sysInfo, folders)

    # do the benchmarks!
    donelist = []
    for bmarkset in allBenchmark:
        doCompile = True
        for bmark in bmarkset:
            if bmark is None or (len(args.limited) > 0 and bmark.name not in args.limited):
                continue
            print("> executing benchmark {}".format(bmark.name))
            donelist.append(bmark.name)
            data = analyzer.open(bmark, folders)
            for i in range(args.repeat):
                files = benchmarker.execBenchmark(bmark, driver, folders, doCompile)
                data = analyzer.read(files, data)
                doCompile = False
            analyzer.save(data, bmark, folders)
            toCSV(data, folders.plots / "{}.csv".format(bmark.name))

    if len(args.limited) > 0:
        wrong = set(args.limited) - set(donelist)
        if len(wrong) > 0:
            print("> Unknown benchmarks requested:")
            for i in wrong:
                print("    {}".format(i))
            print("> Accepted benchmarks:")
            for i in chain(*allBenchmark):
                if i is not None:
                    print("    {}".format(i.name))
    if len(defaults.timeouts) > 0:
        print("A total of {} benchmarks timed out after a waiting period of {} seconds:".format(len(defaults.timeouts), defaults.timeout))
        for i in defaults.timeouts:
            print("  {}".format(i))
