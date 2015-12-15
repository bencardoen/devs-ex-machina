#
# This file is part of the DEVS Ex Machina project.
# Copyright 2014 - 2015 University of Antwerp
# https://www.uantwerpen.be/en/
# Licensed under the EUPL V.1.1
# A full copy of the license is in COPYING.txt, or can be found at
# https://joinup.ec.europa.eu/community/eupl/og_page/eupl
#      Author: Stijn Manhaeve
#

from collections import namedtuple

Folders = namedtuple('Folders', 'root perf json plots')
Folders.__doc__ = """
Small container class for keeping track of some paths.
members:
  * root:  (field) The root folder of the benchmark system.
  * perf:  (field) The folder that will contain the perf stat output.
  * json:  (field) The folder that will contain json files for each benchmark.
  * plots: (field) The folder where the plots are put.
"""

Benchmark = namedtuple('Benchmark', 'name compile getExecList shortName description')
# evil witchcraft that changes the default arguments of the constructor.
Benchmark.__new__.__defaults__ = ('', '')
Benchmark.__doc__ = """
Represents a single benchmark job.
members:
  * name: (field) A supposedly unique name for this benchmark.
                  his name will be used as a folder name.
  * compile: (function) This function will be executed once, before the actual benchmark starts.
                        You can use this function to compile an executable.
                        signature:
                          void compile()
  * getExecList: (function) Returns a (finite!) iterable that iterates over a list of strings or lists.
                         These items should form the command that is executed with the perf stat calls.
                         signature:
                           iterable getExecList()
  * shortName:   (field) [optional, default value: ""] A shorter name that need not be unique.
  * description: (field) [optional, default value: ""] An optional description.
"""

BenchmarkDriver = namedtuple('BenchmarkDriver', 'filegen foldergen execute')
BenchmarkDriver.__doc__ = """
Collection of functions for executing an entire benchmark.
members:
  * filegen: (function) Generates new filenames.
                        These filenames are used to write the perf stat output to disk.
                        signature:
                          iterable filegen()
  * foldergen: (function) Creates a set of output folders for a specific Benchmark,
                          based on a general folder set.
                          signature:
                            Folders foldergen(Benchmark, Folders)
  * execute: (function) executes a single benchmark with a given list of arguments
                        and a filepath. Returns True if the call was successful, otherwise False
                        signature:
                          bool execute(list, Path)
"""

BenchmarkAnalyzer = namedtuple('BenchmarkAnalyzer', 'open read save')
BenchmarkAnalyzer.__doc__ = """
Collection of functions for analyzing the benchmark results.
members:
  * open: (function) Opens a saved JSON file
                     params:
                       bmark A Benchmark object
                       folders A Folders object
  * read: (function) Analyzes the data contained in a list of files. Returns a dict containing some data.
                     params:
                       fnames A list of newly generated filenames and arguments. @see benchmarker.py/execBenchmark
                       old [optional] If specified, new data will be added to this dict, instead of starting from an empty one
                     returns:
                       A dict object containing raw performance data
                     signature:
                       dict read(fnames, jsonf=None)
  * save: (function) Saves the data to a file.
                     params:
                       data An object containing raw performance data. @see BenchmarkAnalyzer.read and analyze
                       bmark A Benchmark object
                       folders A Folders object
                     signature:
                       void save(data, Path)
"""
