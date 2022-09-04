# Commata

Just another header-only C++17 CSV parser

## For users

 - [Primer](CommataPrimer.md)
 - [Specification](https://furfurylic.github.io/commata/CommataSpecification.xml)

## For developers

Commata is written in C++17 and consists only of headers.
All C++ codes in this repository shall compile and link with
Microsoft Visual Studio 2019, 2022, GCC 7.3 and Clang 7.0.

This repository also contains a test of it in `src_test` directory.
Tests are written on [GoogleTest](https://github.com/google/googletest) v1.12.1.
Commata employs CMake 3.13 or later to make the test.
You can build it as follows:

 1. At the top directory of the repository, prepare GoogleTest and the tests for
    Commata with CMake into `build` directory:
    ```bash
    $ cmake -S . -B build -DCOMMATA_BUILD_TESTS=ON
    ```
    You can also explicitly specify the geneator, the platform, the build type, and so on. For example:
    ```bash
    $ cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCOMMATA_BUILD_TESTS=ON
    ```
    or
    ```bash
    $ cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCOMMATA_BUILD_TESTS=ON
    ```

 1. Now you can make and execute the tests.
    All you have to do should be `cd build`, `cmake --build .`, and then `./test_commata`.
    Or, with Microsoft Visual Studio, open `commata.sln` in `build` directory, build `test_commata` project, and run it.
