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

### In Linux

 1. First make Googletest libraries `libgtest.a` and `libgtest_main.a`
    (typically, making a blank directory `build` in the top directory of Googletest
     and doing `cmake ..` in it).

 1. Then create a blank directory `build` in the top directory of the repository of Commata:
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

 1. Now you can make the tests:
    ```bash
    $ make
    ```

 1. Then you can execute the tests:
    ```bash
    $ ./test_commata
    ```

### In Windows with Visual Studio 2015

Here are some rather specific instructions to build and execute the tests.
You should each command in one line, that is, without any line breaks.

 1. First prepare the solution and the projects of Googletest with a solution made with the following commands
    (`%GTEST_ROOT%` should be a path of an empty or a non-existent directory):
    ```cmd
    > cd %TOP_DIRECTORY_OF_GTEST%

    > md build

    > cd build

    > cmake .. -DCMAKE_DEBUG_POSTFIX=d -DCMAKE_INSTALL_PREFIX=%GTEST_ROOT%
               -DBUILD_SHARED_LIBS=OFF -Dgtest_disable_pthreads=ON
               -Dgtest_force_shared_crt=ON -G "Visual Studio 14 Win64"
    ```

 1. Then build the static libraries with the prepared solution and the projects.
    You will get the static libraries (`gtest.lib`, `gtestd.lib`, `gtest_main.lib` and `gtest_maind.lib`)
    in `%GTEST_ROOT%\lib` and the include files in `%GTEST_ROOT%\include`.

 1. Then you can make the solution and the projects of Commata with CMake:
    ```cmd
    > cd %TOP_DIRECTORY_OF_COMMATA%

    > md build

    > cd build

    > cmake .. -Dcommata_disable_pthreads=ON -G "Visual Studio 14 Win64"
               -DGTEST_ROOT=%GTEST_ROOT%
    ```

 1. Now you can build the tests with the solution and the project (`test_commata`) prepared by CMake and run them.
