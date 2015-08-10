# file: benchmarker.py
# author: Stijn Manhaeve
# project: Benchmarker
#
# Main file of the project. This file contains only the most high level functions


def execBenchmark(bmark, driver, folders, compile=True):
    """
    Executes a benchmark and returns a list of output files.
    params:
      bmark: A Benchmark object
      driver: A BenchmarkDriver object
      folders: A Folders object. The data will be stored in this folder
      compile: [default: True] Use the compilation function
    returns:
      an iterable over (Path, args)
    """
    # get real output folders
    folders = driver.foldergen(bmark, folders)
    # compile benchmark
    if bmark.compile is not None and compile:
        bmark.compile()
    # use generator pattern to return a list with filenames
    for args, gfile in zip(bmark.getExecList(), driver.filegen()):
        rfile = folders.perf / gfile
        driver.execute(args, rfile)
        yield rfile, args


def execAnalyze(bmark, driver, analyzer, folders, save=True):
    """
    Executes a benchmark and performs the analysis.
    params:
      bmark: A Benchmark object
      driver: A BenchmarkDriver object
      analyzer: A BenchmarkAnalyzer object
      folders: A Folders object. The data will be stored in this folder
      save: [optional, default is True] If this argument evaluates to True,
                                        the gathered data will also be saved to a file.
    returns:
      An object that contains all the gathered data.
    """
    files = list(execBenchmark(bmark, driver, folders))
    data = analyzer.open(bmark, folders)
    data = analyzer.read(files, data)
    if save:
        analyzer.save(data, bmark, folders)
    return data
