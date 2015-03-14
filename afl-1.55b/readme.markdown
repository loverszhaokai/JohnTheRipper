# Fuzz JohnTheRipper With American Fuzzy Lop (AFL)

## Quick Start

To get going with AFL fuzzing against john:
JOHN_PATH is the path of JohnTHERipper.

### Compile afl

    $ cd JOHN_PATH/afl-1.55b/
    $ make
    $ sudo make install

### Compile john

    $ cd JOHN_PATH/src
    $ AFL_USE_ASAN=1 AFL_HARDEN=1 ./configure CC=afl-gcc --disable-shared --enable-asan 
    $ make -sj8

### Fuzz

    $ cd JOHN_PATH/afl-1.55b/john_test
    $ ../afl-fuzz -p -j -m none -t 1500+ -i test_cases -o output ../../run/john @@ --nolog --max-run-time=1

Fuzzing results will be placed in output/.

## AFL

For more information about AFL, please see [readme](https://github.com/loverszhaokai/JohnTheRipper/blob/add_afl/afl-1.55b/docs/README).

### Advantages

When source code is available, instrumentation can be injected by a companion tool that works as a drop-in replacement for gcc or clang in any standard build process for third-party code.

The instrumentation has a fairly modest performance impact; in conjunction with other optimizations implemented by afl-fuzz, most programs can be fuzzed as fast or even faster than possible with traditional tools.


### Disadvantages

The input file only can be mutated by the few functions in AFL, but there is more better ways to mutate when it comes to hash file.


## Requirements

* libxml2

## New Features

### John mode

The main goal of john mode is to mutate the hash file as we want. With the built-in support, user can control how to mutate the hash file. I think it is **GREAT**!

The input file is written in xml and it should follow the [strict format](http://address_to_add.com), the idea is inspired by [peach fuzzer pit file](http://old.peachfuzzer.com/v3/PeachPit.html).

To use John mode you should add **'-j'** option in afl-fuzz, such as afl-fuzz -j ...

* print fuzz info

To print fuzz info of each iteration, user can add **'-p'** option, such as afl-fuzz -p ...

* auto save mutated cases

In john mode, each mutated cases will be saved in the out dir, it is easy for users to look the mutated results.