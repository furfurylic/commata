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
Tests are written on [GoogleTest](https://github.com/google/googletest) v1.11.0.
Commata employs CMake 3.14 or later to make the test.
You can build it as follows:

 1. In the top directory of the repository, prepare GoogleTest and the tests for
    Commata with CMake:
    ```bash
    $ cmake -S src_test -B build
    ```
    You can also explicitly specify the geneator, the platform, the build type, and so on. For example:
    ```bash
    $ cmake -S src_test -B build -GNinja -DCMAKE_BUILD_TYPE=Debug
    ```
    or
    ```bash
    $ cmake -S src_test -B build -G "Visual Studio 14 2015" -A Win32
    ```
    or
    ```bash
    $ cmake -S src_test -B build -G "Visual Studio 16 2019" -A x64
    ```

 1. Now you can make and execute the tests.
    All you have to do should be `cmake --build .` and then `./test_commata` in that directory.
    Or, with Microsoft Visual Studio, open `commata.sln` in that directory, build `test_commata` project, and run it.
