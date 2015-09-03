# file: defaults.py
# author: Stijn Manhaeve
# project: Benchmarker
#
# A collection of default implementations for some parts of this project.

from .misc import assertPath, printVerbose
from .structs import Benchmark, Folders, BenchmarkDriver, BenchmarkAnalyzer
from itertools import count
from datetime import datetime
from pathlib import Path
import base64
import json
import hashlib
import platform
import multiprocessing
import locale
from subprocess import call, TimeoutExpired, DEVNULL
from collections import Iterable


# global object for passing command line arguments.
args = None


# first, get user locale by forcing it to load it instead of the C locale
locale.setlocale(locale.LC_ALL, '')
# custom parsing function for floats
def pfloat(f):
    """
    Tries to parse a float.
    """
    if len(f) == 0:
        raise FloatingPointError("failed to parse")
    try:
        return locale.atof(f)
    except:
        try:
            printVerbose(args.verbose, "note: failed to directly parse string: {}\tWill try again by removing last character".format(f))
            if f.endswith('%'):
                return pfloat(f[:-1])/100
            return pfloat(f[:-1])
        except:
            raise ValueError("Failed to parse as float: {}".format(f))


def defaultFilegen(format=None, **kwargs):
    """
    Generates an endless list of file names for perf output files.
    You can optionally give a different format and some kwargs.
    """
    format = '{:04d}.perf' if format is None else format
    for i in count():
        yield format.format(i, **kwargs)


def defaultFoldergen(benchmark, folders):
    """
    Opens the necesarry set of folders for a benchmark and returns these folders.
    params:
      benchmark: A Benchmark object. @see structs.py/Benchmark
      folders: A Folders object, @see structs.py/Folders
    """
    fmt = '{0:%Y}.{0:%m}.{0:%d}_{0:%H}.{0:%M}.{0:%S}'
    newpath = assertPath(folders.perf / benchmark.name / fmt.format(datetime.now()))
    newpath.resolve()
    return folders._replace(perf=newpath)


def getOFolders(sysInfo, target="."):
    """
    Creates top-level output folders based on some data.
    params:
      sysInfo Either an object that contains system dependent information or a hash.
              if sysInfo is a raw string, it is used as the system hash.
              Otherwise, the hash is calculated based on this object, which will first be pickled into JSON text data.
              The hash is then used as a folder name, which can (hopefully) uniquely identify this system.
              JUST TO BE CLEAR: to use a known hash, use a _raw string_
      target [optional, default = "."] The folder that will contain the output folders.
    returns:
      A Folders object that describes the several output folders. @see structs.py/Folders for more information
    """
    # get hash
    if isinstance(sysInfo, type(b'')):
        syshash = sysInfo.decode('ascii')
    else:
        # convert the sysInfo object to JSON and take the md5 hash
        syshash = hashlib.md5(json.dumps(sysInfo, sort_keys=True).encode('ascii')).digest()
        # take only the first 6 bytes of the hash and convert it to base32
        syshash = base64.b32encode(syshash)[:6].decode('ascii')

    # get Path object to target folder
    root = Path(target, syshash)

    # get the paths
    paths = ['', 'perf', 'json', 'plots']
    paths = map(lambda x: assertPath(root / x), paths)

    # return result
    return Folders(*paths)


def getSysInfo():
    """
    Creates an object describing this system
    """
    sysInfo = {
                'platform':  platform.platform(),
                'numcores':  multiprocessing.cpu_count(),
                'processor': platform.processor(),
                'architecture': platform.architecture(),
                'node':  platform.node(),
                'uname': platform.uname()
              }
    return sysInfo


def saveSysInfo(sysInfo, folders):
    """
    Saves the sysInfo information to <root>/system.json
    params:
        sysInfo: A system information object. @see getSysInfo
        folders: A Folders object. @see structs.py/Folders
    """
    with (folders.root/'system.json').open('w') as f:
        f.write(json.dumps(sysInfo, indent=2, sort_keys=True))
        f.close()


timeout = 50
"""
Waiting time in seconds before a subprocess is aborted. Note that no data will be collected from this instance.
"""

timeouts = []
"""
List of arguments that resulted into a timeout
"""


def defaultExec(arg, path):
    """
    Default implementation of BenchmarkDriver.execute
    """
    if isinstance(arg, Iterable):
        arg = " ".join(map(str, arg))
    fmtcommand = "perf stat -o {} {}".format(str(path), arg)
    print(fmtcommand)
    try:
        call(fmtcommand.split(), timeout=timeout, stdout=DEVNULL if (args is None or not args.showSTDOUT) else None)
    except TimeoutExpired:
        print("subprocess timed out after more than {} seconds.".format(timeout))
        timeouts.append(arg)

defaultDriver = BenchmarkDriver(defaultFilegen, defaultFoldergen, defaultExec)


def perfParser(path, old=None):
    """
    Parses output by perf stat from a file.
    params:
        path The filenames and arguments. @see benchmarker.py/execBenchmark
        old [optional, default None] If specified, start from this data
    """
    # force path to be a list
    if not isinstance(path, list):
        path = list(path)
    rdata = {} if old is None else old
    # get some place to store the results
    resdata = rdata.setdefault('results', [{} for i in range(len(path))])
    # store the arguments
    rdata['args'] = [k for _, k in path]
    for ((p, _), i) in zip(path, count()):
        data = resdata[i]
        with p.open() as f:
            for line in map(str.strip, f):
                # iterate over all lines with removed leading/trailing whitespace
                if len(line) == 0:
                    continue
                # parse all data
                #  data can have the following forms:
                #   0:  [text]
                #   1:  [number] [name] [(unit)]? [# [number] [more info]]?
                #   2:  <not supported> [name]
                #   3:  <not counted>
                #   4:  [number] [seconds time elapsed]
                # can't use perf stat -x: argument because then, the output doesn't include total running time
                if line.startswith("<not supported>"):  # 2
                    data[line.split()[2]] = None
                elif line.startswith("<not counted>"):  # 3
                    print("encountered a <not counted>. aborting.")
                    quit()
                elif line.endswith('seconds time elapsed'):  # 4
                    subData = data.setdefault('time-elapsed', {'value': [], 'unit': None})
                    subData['unit'] = 'seconds'
                    subData.setdefault('value', []).append(pfloat(line.split()[0]))
                elif not line.startswith('Performance') and not line.startswith('#'):  # 1
                    spl = line.split('#')
                    ksplit = spl[0].split()
                    subData = data.setdefault(ksplit[1], {'value': [], 'unit': None})
                    subData['unit'] = ksplit[2][1:-1] if len(ksplit) == 3 else None
                    subData.setdefault('value', []).append(pfloat(ksplit[0]))
                    if len(spl) == 2:
                        k2split = spl[1].split(maxsplit=1)
                        subsubData = subData.setdefault('extra', {'value': [], 'unit': None})
                        subsubData['unit'] = k2split[1] if len(k2split) == 2 else None
                        subsubData.setdefault('value', []).append(pfloat(k2split[0]))

    return rdata


def perfSave(data, bmark, folders):
    """
    Saves perf data to a file.
    params:
      data An object containing raw performance data. @see BenchmarkAnalyzer.read and analyze
      bmark A Benchmark object
      folders A Folders object
    """
    path = folders.json / "{}.json".format(bmark.name)
    assertPath(path.parent)
    with path.open('w') as f:
        f.write(json.dumps(data, indent=True, sort_keys=True))


def perfOpen(bmark, folders):
    """
    Opens perf data from a benchmark
    params:
      bmark A Benchmark object
      folders A Folders object
    """
    path = folders.json / "{}.json".format(bmark.name)
    if not path.exists():
        return {}
    return json.load(path.open('r'))

perfAnalyzer = BenchmarkAnalyzer(perfOpen, perfParser, perfSave)
