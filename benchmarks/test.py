# file: test.py
# author: Stijn Manhaeve
# project: Benchmarker
#
# Some basic tests. Just execute this thing.
import defaults
import benchmarker
from csv import toCSV


def echotestGen():
    for i in range(5):
        yield ['dd', 'if=/dev/zero', 'of=/dev/null', 'count={}'.format(10**(i+2))]


def noop():
    """
    literally does nothing (except for eating away precious cpu time).
    """
    pass

if __name__ == '__main__':
    bmark = defaults.Benchmark('basictest', None, echotestGen, 'Just a basic test that pipes some data from /dev/zero to /dev/null', 'basic test')
    driver = defaults.defaultDriver
    analyzer = defaults.perfAnalyzer
    sysInfo = defaults.getSysInfo()
    folders = defaults.getOFolders(sysInfo, "tests")
    defaults.saveSysInfo(sysInfo, folders)
    files = list(benchmarker.execBenchmark(bmark, driver, folders))
    data = analyzer.open(bmark, folders)
    data = analyzer.read(files, data)
    analyzer.save(data, bmark, folders)
    toCSV(data, folders.plots / "{}.csv".format(bmark.name))
