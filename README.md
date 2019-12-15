# Commata

Just another header-only C++14 CSV parser

## For users

 - [Primer](CommataPrimer.md)
 - [Specification](https://furfurylic.github.io/commata/CommataSpecification.xml)

## For developers

Commata is written in C++14 and consists only of headers.
All C++ codes in this repository shall compile and link with
Microsoft Visual Studio 2015, 2019 and GCC 6.3.

This repository also contains a test of it in `src_test` directory.
Tests are written on [Googletest](https://github.com/google/googletest) v1.10.0.
Commata employs CMake 3.14 or later to make the test.
You can build it as follows:

 1. Create a blank directory `build_test` in the top directory of the repository of Commata:
    ```bash
    $ mkdir build_test
    ```
 1. In the directory prepare Googletest and the tests for Commata with CMake:
    ```bash
    $ cd build_test
    $ cmake ../src_test
    ```
    You can also explicitly specify the geneator, the platform, the build type, and so on. For example:
    ```bash
    $ cmake ../src_test -GNinja -DCMAKE_BUILD_TYPE=Debug
    ```
    or
    ```bash
    $ cmake ../src_test -G "Visual Studio 14 2015" -A Win32
    ```
    or
    ```bash
    $ cmake ../src_test -G "Visual Studio 16 2019" -A x64
    ```

 1. Now you can make and execute the tests.
    All you have to do should be `cmake --build .` and then `./test_commata` in that directory.
    Or, with Microsoft Visual Studio, open `commata.sln` in that directory, build `test_commata` project, and run it.
