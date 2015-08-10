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
# TODO add options to do only one particular benchmark
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

# different simulation types
SimType = namedtuple('SimType', 'classic optimistic conservative')
simtypes = SimType(["classic"], ["opdevs", args.cores], ["cpdevs", args.cores])


# generators for the benchmark parameters
def devstonegen(simtype):
    for depth in [1, 2, 3]:#, 4, 8, 16]:
        for width in [2, 3, 4]:#, 8, 16]:
            yield list(chain([devstoneEx], simtype, [width, depth]))


def pholdgen(simtype):
    for depth in [1, 2, 4, 8, 16]:
        for width in [1, 2]:#, 4, 8, 16]:
            for iterations in [16, 64, 256, 1024]:
                for remotes in [10]:
                    yield list(chain([pholdEx], simtype, [width, depth, iterations, remotes]))


# compilation functions
def compileDevstone(force=False):
    """
    Compiles the devstone executable, if it doesn't exist already.
    params:
      force [default=False] use a true value to force compilation
    """
    path = Path(devstoneEx)
    if force or not path.exists():
        # if path.exists():   # the executable already exists. Remove it or make won't do anything
        #     path.unlink()
        call(['make', '--always-make', 'dxexmachina_devstone'], cwd='./build')


def compilePhold(force=False):
    """
    Compiles the devstone executable, if it doesn't exist already.
    params:
      force [default=False] use a true value to force compilation
    """
    path = Path(pholdEx)
    if force or not path.exists():
        # if path.exists():   # the executable already exists. Remove it or make won't do anything
        #     path.unlink()
        call(['make', '--always-make', 'dxexmachina_phold'], cwd='./build')


if __name__ == '__main__':
    # test if the build folder exists and has a makefile
    if not Path('./build').exists() or not Path('./build/Makefile').exists():
        print("note: this script assumes that the cmake script has run at least once and that the './build/Makefile' file exists.")
        print("Please do so and then rerun the benchmark script.")
        quit()

    devc = partial(compileDevstone, args.force)
    pholdc = partial(compilePhold, args.force)
    devstone = SimType(
        defaults.Benchmark('devstone/classic', devc, partial(devstonegen, simtypes.classic), "devstone, classic"),
        defaults.Benchmark('devstone/optimistic', devc, partial(devstonegen, simtypes.optimistic), "devstone, optimistic"),
        defaults.Benchmark('devstone/conservative', devc, partial(devstonegen, simtypes.conservative), "devstone, conservative"),
        )
    phold = SimType(
        defaults.Benchmark('phold/classic', pholdc, partial(pholdgen, simtypes.classic), "phold, classic"),
        defaults.Benchmark('phold/optimistic', pholdc, partial(pholdgen, simtypes.optimistic), "phold, optimistic"),
        defaults.Benchmark('phold/conservative', pholdc, partial(pholdgen, simtypes.conservative), "phold, conservative"),
        )
    # do all the preparation stuff
    driver = defaults.defaultDriver
    analyzer = defaults.perfAnalyzer
    sysInfo = defaults.getSysInfo()
    folders = defaults.getOFolders(sysInfo, "bmarkdata")
    defaults.saveSysInfo(sysInfo, folders)

    # do the benchmarks!
    donelist = []
    for bmarkset in [devstone, phold]:
        doCompile = True
        for bmark in bmarkset:
            if args.limited is not None and bmark.name not in args.limited:
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

    if args.limited is not None:
        wrong = set(args.limited) - set(donelist)
        if len(wrong) > 0:
            print("> Unknown benchmarks requested:")
            for i in wrong:
                print("    {}".format(i))
            print("> Accepted benchmarks:")
            for i in chain(devstone, phold):
                print("    {}".format(i.name))
