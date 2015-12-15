#
# This file is part of the DEVS Ex Machina project.
# Copyright 2014 - 2015 University of Antwerp
# https://www.uantwerpen.be/en/
# Licensed under the EUPL V.1.1
# A full copy of the license is in COPYING.txt, or can be found at
# https://joinup.ec.europa.eu/community/eupl/og_page/eupl
#      Author: Stijn Manhaeve
#


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


def printVerbose(doPrint, *args, **kwargs):
    """
    prints only if verbose flag is used
    params:
      doPrint  if True, call the regular print with the remainder of the arguments.
    """
    if doPrint:
        print(args, kwargs)
