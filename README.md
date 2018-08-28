
# uniform_int_distribution_fast

C++17-compliant uniform_int_distribution_fast, based on bounded_rand-function, as per the [paper by Daniel Lemire](https://arxiv.org/abs/1805.10941) and optimizations added to bounded_rand [published by Melissa E. O'Neill](http://www.pcg-random.org/posts/bounded-rands.html).

This has not yet been completely implemented as according to my testing, things are not as clear as they seemed by looking at the publications by both Daniel Lemire and Melissa E. O'Neill. Most importantly, I'm not convinced the right thing is being measured. The [CppCon15 presentation/talk by Chandler Carruth](https://www.youtube.com/watch?v=nXaxk27zwlk&t=6s) explains most, if not all of the relevant points.

## Testing

### Micro-Benchmarking

I've been testing various functions to perform the same operation, compiled with gcc/clang/vc on Windows, both x86 and x64. Random range values in the ranges 1 < shift_n have been tested. The random numbers generated (splitmix) are the same numbers for every complete comparable test.

* gcc-8.1.0/mingw-5.0.4 (libstdc++)
* clang-8.0.0-r339319 (VC STL)
* vc-15.8 (VC STL)
* google/benchmark-1.4.1-trunk
* optimised for Intel Ci3-5005U (broadwell)

Results in the benchmark folder.

* The tested `bitmask_alt` function, although fast (generally), does however have a bug and does not generate a uniform distribution.
* The simple `bitmask` function is fastest on most platforms/compilers, with the exceptions of clang/vc on x86, where lemire_oneill is fastest for ranges under 2^27 and clang on x64 where lemire_oneill is faster for ranges under 2^59.

### Macro-Benchmarking

The macro-benchmarking (in the main project), does not show the same results as the Micro-Benchmarking. In order to do some Macro-Benchmarking, I have implemented, what I call the 'Bucket-Test', it tests the quality of the distribution of the numbers generated in a range, but equally makes for a meaningful workload for macro performance testing.

This testing shows that `bitmask` is not fast (a Mico-Benchmarking 'Red-Herring') and equally shows that vanilla Lemire is just as fast as Lemire-ONeill [Blue-Herring?] in most cases. Less code seems better to me. All this testing also shows that vc is pretty shit [at optimisation], that clang is better and that gcc is tops (all this on Windows).

Input required, `nix` testing required.
