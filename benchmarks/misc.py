# file: misc.py
# author: Stijn Manhaeve
# project: Benchmarker
#
# Random collection of small functions that I might need throughout this project.


def assertPath(path):
    """
    Creates the folder specified by path, if it doesn't exist already.
    params:
      path: a pathlib Path object denoting a folder.
    returns
      the same path object.
      This allows using a more functional style of programming.
    """
    if not path.exists():
        path.mkdir(parents=True)
    return path
