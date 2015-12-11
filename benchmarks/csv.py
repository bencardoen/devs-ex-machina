# file: csv.py
# author: Stijn Manhaeve
# project: Benchmarker
#
# collection of functions for saving the data to Tim-readable csv
from .misc import assertPath
import traceback


def _addDataLine(arg, res, f, delim=';', argparse=lambda x: "\"{}\"".format(" ".join(map(str, x)))):
    """
    Adds the data lines for a single argument and the individual results.
    params:
      arg: The given arguments for the benchmark
      res: A dictionary containing all the results
      f: a file object to which the data is written
    """
    # convert the arguments to something we can actually write to the file
    argstr = argparse(arg)
    if type(argstr) is not str:
        argstr = delim.join(map(str, argstr))
    # keep a list of lists that will contain all the data points
    values = []
    for _, val in sorted(res.items()):
        if val is None:
            continue
        values.append(val["value"])
        if "extra" in val.keys():
            values.append(val["extra"]["value"])
    # values now contains a list of lists of values.
    # Just iterate over it and write it to the file
    for vals in zip(*values):
        f.write(argstr)
        f.write(delim)
        f.write(delim.join(map(str, vals)))
        f.write('\n')


def _addHeaderLine(res, f, delim=';', argColumns="\"command\""):
    """
    Adds a header line.
    """
    f.write(argColumns)
    for name, val in sorted(res.items()):
        if val is None:
            continue
        f.write(delim)
        if val["unit"] is not None:
            f.write("\"{} ({})\"".format(name, val["unit"]))
        else:
            f.write("\"{}\"".format(name))
        if "extra" in val.keys():
            val = val["extra"]
            f.write(delim)
            if val["unit"] is not None:
                f.write("\"{} ({})\"".format(name, val["unit"]))
            else:
                f.write("\"{}\"".format(name))
    f.write('\n')


def toCSV(data, path, delim=';', argColumns="\"command\"", argParse=lambda x: "\"{}\"".format(" ".join(map(str, x)))):
    """
    Saves the data to path in csv format.
    """
    args = data["args"]
    results = data["results"]
    if len(results) == 0:
        print("! no results for benchmark !")
        return
    assertPath(path.parent)
    with path.open('w') as f:
        _addHeaderLine(results[0], f, delim, argColumns)
        for arg, res in zip(args, results):
            # now we have all the data per set of arguments
            _addDataLine(arg, res, f, delim, argParse)


def readCSV(path, delim=';', stringquote='"', keepheader=True):
    """
    Reads a csv file into a list of lists of data.
    """
    def extractData(item):
        item = item.strip()
        if len(item) == 0:
            return None
        if len(item) >= 2 and item[0] == stringquote and item[-1] == stringquote:
            return item[1:-1]
        try:
            f = float(item)
            return int(f) if '.' not in item else f
        except:
            # no idea which data type, panic!
            raise Exception("Panic! received {}".format(item))

    assertPath(path.parent)
    theData = []
    with path.open('r') as f:
        if not keepheader:
            next(f)
        for line in f:
            try:
                theData.append(list(map(extractData, line.split(delim))))
            except:
                pass  # traceback.print_exc()
    return theData


def writeCSV(path, data, delim=';', stringquote='"'):
    """
    writes a list of lists of data into a csv file.
    """
    def formatData(item):
        if item is None:
            return ""
        elif type(item) in [int, float]:
            return str(item)
        elif type(item) in [str]:
            return "{0}{1}{0}".format(stringquote, item)
        else:
            # no idea which data type, panic!
            raise Exception("Panic! received {}".format(item))
    assertPath(path.parent)
    with path.open('w') as f:
        for i in data:
            f.write(delim.join(map(formatData, i)))
            f.write('\n')


def removeColums(data, begin, end, step=1):
    """
    Removes the columns defined by the slice begin, end, step
    """
    for i in data:
        del i[begin:end:step]


def groupN(data, n):
    """
    groups every n rows together
    """
    return map(list, zip(*[iter(data)]*n))


def reduceN(data, n, reduceFunc):
    return map(reduceFunc, list(groupN(data, n)))


def mean(items):
    items = list(items)
    return sum(items)/len(items)


def meanReduce(items, i=-1):
    return items[0][0:i] + [mean(list(zip(*items))[i])] + (items[0][i+1:] if i != -1 else [])


def transformPHold(pathIn, pathOut):
    data = readCSV(pathIn, keepheader=True)
    removeColums(data, 0, 2)
    removeColums(data, 1, 5)
    removeColums(data, 3, -1)
    header = data[0]

    dataiter = iter(data)
    next(dataiter)
    reduced = reduceN(dataiter, 15, meanReduce)
    # print("--")
    # print(list(groupN(dataiter, 5)))
    # print("--")
    reduced = list(reduced)
    # print(reduced)
    # print("--")
    # print([header] + reduced)

    writeCSV(pathOut, [header] + reduced)


def transformConnect(pathIn, pathOut):
    data = readCSV(pathIn, keepheader=True)
    removeColums(data, 0, 2)
    removeColums(data, 1, 2)
    removeColums(data, 2, -1)
    header = data[0]

    dataiter = iter(data)
    next(dataiter)
    reduced = reduceN(dataiter, 15, meanReduce)
    # print("--")
    # print(list(groupN(dataiter, 5)))
    # print("--")
    reduced = list(reduced)
    # print(reduced)
    # print("--")
    # print([header] + reduced)

    writeCSV(pathOut, [header] + reduced)


def transformConnectSpeed(pathIn, pathOut):
    data = readCSV(pathIn, keepheader=True)
    removeColums(data, 0, 2)
    removeColums(data, 2, -1)
    header = data[0]

    dataiter = iter(data)
    next(dataiter)
    reduced = reduceN(dataiter, 15, meanReduce)
    # print("--")
    # print(list(groupN(dataiter, 5)))
    # print("--")
    reduced = list(reduced)
    # print(reduced)
    # print("--")
    # print([header] + reduced)

    writeCSV(pathOut, [header] + reduced)


def transformDevstone(pathIn, pathOut):
    data = readCSV(pathIn, keepheader=True)
    removeColums(data, 0, 2)
    removeColums(data, 4, -1)
    header = data[0]

    dataiter = iter(data)
    next(dataiter)
    reduced = reduceN(dataiter, 15, meanReduce)
    # print("--")
    # print(list(groupN(dataiter, 5)))
    # print("--")
    reduced = list(reduced)
    # print(reduced)
    # print("--")
    # print([header] + reduced)

    writeCSV(pathOut, [header] + reduced)

if __name__ == "__main__":
    from pathlib import Path
    pathroot = Path("./bmarkdata/47IPRO/")
    plotsroot = pathroot/"plots"
    backuproot = pathroot/"backup"
    namesdxex = ["classic.csv", "conservative.csv", "optimistic.csv"]
    namesadevs = ["classic.csv", "conservative.csv"]
    pholdFolders = ["phold", "phold_remotes"], ["aphold", "aphold_remotes"]
    pholdTransforms = [[transformPHold, transformPHold]]*2
    connectFolders = ["connect", "connect_speedup"], ["aconnect", "aconnect_speedup"]
    connectTransform = [[transformConnect, transformConnectSpeed]]*2
    devstoneFolders = ["devstone"], ["adevstone"]
    devstoneTransform = [[transformDevstone]]*2

    for folders, transform in [(pholdFolders, pholdTransforms),
                               (connectFolders, connectTransform),
                               (devstoneFolders, devstoneTransform)]:
        dxex, adevs = folders
        dxext, adevst = transform
        for f, t in zip(dxex, dxext):
            for name in namesdxex:
                try:
                    t((plotsroot/f)/name, (backuproot/f)/name)
                    print("{} -> {}".format((plotsroot/f)/name, (backuproot/f)/name))
                except FileNotFoundError:
                    pass

        for f, t in zip(adevs, adevst):
            for name in namesadevs:
                try:
                    t((plotsroot/f)/name, (backuproot/f)/name)
                    print("{} -> {}".format((plotsroot/f)/name, (backuproot/f)/name))
                except FileNotFoundError:
                    pass

    # transformPHold(Path("./bmarkdata/M35Y2O/plots/aphold/classic.csv"),
    #                Path("./bmarkdata/M35Y2O/backup/aphold/classic.csv"))
    # transformPHold(Path("./bmarkdata/M35Y2O/plots/aphold/conservative.csv"),
    #                Path("./bmarkdata/M35Y2O/backup/aphold/conservative.csv"))
    # # transformPHold(Path("./bmarkdata/M35Y2O/plots/aphold/optimistic.csv"),
    # #                Path("./bmarkdata/M35Y2O/backup/aphold/optimistic.csv"))
    # transformPHold(Path("./bmarkdata/M35Y2O/plots/phold/classic.csv"),
    #                Path("./bmarkdata/M35Y2O/backup/phold/classic.csv"))
    # transformPHold(Path("./bmarkdata/M35Y2O/plots/phold/conservative.csv"),
    #                Path("./bmarkdata/M35Y2O/backup/phold/conservative.csv"))
    # transformPHold(Path("./bmarkdata/M35Y2O/plots/phold/optimistic.csv"),
    #                Path("./bmarkdata/M35Y2O/backup/phold/optimistic.csv"))
