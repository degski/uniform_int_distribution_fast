
# uniform_int_distribution_fast

C++17-compliant uniform_int_distribution_fast, based on bounded_rand-function, as per the [paper by Daniel Lemire](https://arxiv.org/abs/1805.10941) and optimizations added to bounded_rand [published by Melissa E. O'Neill](http://www.pcg-random.org/posts/bounded-rands.html).

This has not yet been completely implemented as according to my testing, things are not as clear as they seemed by looking at the publications by both Daniel Lemire and Melissa E. O'Neill. Most importantly, I'm not convinced they are measuring the right thing. The [CppCon15 presentation/talk by Chandler Carruth](https://www.youtube.com/watch?v=nXaxk27zwlk&t=6s) explains most, if not all of the relevant points.

## Testing

I've been testing various functions to perform the same operation, compiled with gcc/clang/vc on Windows, both x86 and x64. Random range values in the ranges 1 < shift_n have been tested. The random numbers generated (splitmix) are the same numbers for every complete comparable test.

### Results

There are output-txt-files in the benchmark folder.
