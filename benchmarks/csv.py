# file: csv.py
# author: Stijn Manhaeve
# project: Benchmarker
#
# collection of functions for saving the data to Tim-readable csv
from .misc import assertPath


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
