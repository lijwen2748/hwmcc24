# SuperCAR


## Set up
We use a python script `superCAR.py` to start a parallel job.
The python script relies on the `click` package to parse command line options.
If you do not have this package, an offline version is provided in this repo.
You could install it with:
```
pip install --no-index --find-links=./package click
```

## Get submodules
```
git submodule update --init --recursive
```
## Build
Build CaDiCaL lib:
```
cd simple_CAR/src/sat/cadical/
./configure && make
cd ../../../..
```
We use manual-written makefile to build.
use make to compile all the checkers:
```
make
```
and it will make a new directory `bin`, with binaries (`MCAR` and `simplecar`s) in it.

## Run
Please run at this directory, because the path to binaries(`./bin`) is fixed in the python script.
Usage: superCAR.py [OPTIONS]

Options:
  -I, --input TEXT   Path to input aiger file  [required]
  -O, --output TEXT  Path to output cex/cert. [required]
  -V, --verbose      Turn on printing
  --help             Show help message and exit.

Example:
```
python3 superCAR.py -I xxx.aig -O ./output
```
Safe: ```./output/xxx.w.aag```

UnSafe: ```./output/xxx.cex```

# Authors(In Alphabetical Order)

Yibo Dong (prodongf@gmail.com), YeChuan Xia (xiaozi465@gmail.com)

Supervisors:

Jianwen Li (lijwen2748@gmail.com), Geguang Pu (ggpu@sei.ecnu.edu.cn)

All from Software Engineering Institute, East China Normal University.