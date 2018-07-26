
// MIT License
//
// Copyright (c) 2018 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <ciso646>

#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "splitmix.hpp"
#if defined ( __GNUC__ ) || defined ( __clang__ )
#include "lehmer.hpp"
#endif
#include "uniform_int_distribution_fast.hpp"

int main ( ) {

    splitmix64 rng ( [ ] ( ) { std::random_device rdev; return ( static_cast<std::uint64_t> ( rdev ( ) ) << 32 ) | rdev ( ); } ( ) );
    ext::uniform_int_distribution_fast<std::uint64_t> dis ( 0, ( std::uint64_t { 1 } << 63 ) - 100 );

    for ( int k = 0; k < 1000; k++ ) {
        std::cout << dis ( rng ) << std::endl;
    }

    std::cout << dis.a ( ) << " " << dis.b ( ) << std::endl;

    return EXIT_SUCCESS;
}

template<typename Rng>
uint32_t random_bounded1 ( Rng & rng, uint32_t range ) {
    uint32_t random32bit = rng ( ); //32-bit random number
    uint64_t multiresult = uint64_t { random32bit } * uint64_t { range };
    return multiresult >> 32;
}

template<typename Rng>
uint64_t random_bounded ( Rng & rng, uint64_t range) {
    uint64_t highproduct;
    uint64_t lowproduct = _umul128 ( rng ( ), range, &highproduct );
    if ( lowproduct < range ) {
        const uint64_t threshold = (0-range) % range;
        while ( lowproduct < threshold ) {
            lowproduct = _umul128 ( rng ( ), range, &highproduct );
        }
    }
    return highproduct;
}

int main4346463 ( ) {

    splitmix64 rng ( /* [ ] ( ) { std::random_device rdev; return rdev ( ); } ( ) */ 123 );

    for ( int k = 0; k < 10'000; k++ ) {
        std::cout << random_bounded ( rng, 1'000'000'000 ) << std::endl;
    }

    return 0;
}
