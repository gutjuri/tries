# tries

Tries in C++

## Usage & Requirements

### Usage only

Since this is a header-only library, it is enough to include trie.hpp into the file you want to use tries in!
A C++20 compliant compiler is needed. 
I confirmed that gcc 10.0.1 and 10.1.0 work; Clang doesn't seem to work (at least in version 10.0.0).

### Tests

In order to execute tests, the dependency catch2 must be available under catch2/catch.hpp.
The tests can be executed with `make unittests`.

### Benchmarks

In order to execute benchmarks, the dependency catch2 must be available under catch2/catch.hpp.
Further it is necessary that gcc is used because we compare the library at hand with GNU's trie implementation.
The performance-benchmarks can be executed with `make benchmark`.
The memory-benchmarks can be executed with `make benchmark_memory`.
The results can be found in benchmark/ (I tested on an Intel Core i7 from 2012).
