# Commata

Just another header-only C++14 CSV parser

## For users

 - [Primer](CommataPrimer.md)
 - [Specification](https://furfurylic.github.io/commata/CommataSpecification.xml)

## For developers

Commata is written in C++14 and consists only of headers.
All C++ codes in this repository shall compile and link with
Microsoft Visual Studio 2015 and GCC 6.3.

This repository also contains a test of it.
Tests are written on [Googletest](https://github.com/google/googletest) v1.8.0.
Commata employs CMake 3 to make the test.
You can build it as follows:

 1. First create a blank directory `build` in the top directory of the repository:
    ```bash
    $ mkdir build
    ```
 1. Then in the directory you can make `Makefile` with CMake:
    ```bash
    $ cd build
    $ cmake .. -DGTEST_LIBRARY=$GTEST_LIB/libgtest.a \
               -DGTEST_MAIN_LIBRARY=$GTEST_LIB/libgtest_main.a \
               -DGTEST_INCLUDE_DIR=$GTEST_INCLUDE/
    ```
    provided that `$GTEST_LIB` and `$GTEST_INCLUDE` are set properly to the paths
    that their names mean.

 1. Now you can make the test:
    ```bash
    $ make
    ```

 1. Then you can execute the test:
    ```bash
    $ ./test_commata
    ```
