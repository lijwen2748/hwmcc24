## Set up
We use a python script `hwmcc.py` to start a parallel job.
The python script relies on the `click` package to parse command line options.
If you do not have this package, an offline version is provided in this repo.
You could install it with:
```
pip install --no-index --find-links=./package click
```

## Get submodules
```
git submodule init
git submodule update
```
## Build
We use manual-written makefile to build.
use make to compile all the checkers:
```
make -j4
```
and it will make a new directory `bin`, with binaries (`MCAR` and `simplecar`) in it.

## Run
Please run at this directory, because the path to binaries(`./bin`) is fixed in the python script.
Usage: hwmcc.py [OPTIONS]

Options:
  -I, --input TEXT   Path to input aiger file  [required]
  -O, --output TEXT  Path to output cex/cert. Please make sure it's empty.
                     [required]
  -V, --verbose      Turn on printing
  --help             Show help message and exit.

Example:
```
python3 hwmcc.py -I xxx.aig -O ./output
```