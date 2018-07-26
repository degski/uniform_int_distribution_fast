
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
// #include "lehmer.hpp"
#endif

/*
#include "uniform_int_distribution_fast.hpp"

int main55654 ( ) {

    splitmix64 rng ( [ ] ( ) { std::random_device rdev; return ( static_cast<std::uint64_t> ( rdev ( ) ) << 32 ) | rdev ( ); } ( ) );
    ext::uniform_int_distribution_fast<std::uint64_t> dis ( 0, ( std::uint64_t { 1 } << 63 ) - 100 );

    for ( int k = 0; k < 1000; k++ ) {
        std::cout << dis ( rng ) << std::endl;
    }

    std::cout << dis.a ( ) << " " << dis.b ( ) << std::endl;

    return EXIT_SUCCESS;
}
*/








/*
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
*/

#include <intrin.h>

#pragma intrinsic ( _BitScanReverse )

inline constexpr int leading_zeros_hackers_delight ( std::uint64_t x ) noexcept {
    // by Henry Warren.
    int n = 0;
    if ( x <= 0x0000'0000'FFFF'FFFF ) n += 32, x <<= 32;
    if ( x <= 0x0000'FFFF'FFFF'FFFF ) n += 16, x <<= 16;
    if ( x <= 0x00FF'FFFF'FFFF'FFFF ) n +=  8, x <<=  8;
    if ( x <= 0x0FFF'FFFF'FFFF'FFFF ) n +=  4, x <<=  4;
    if ( x <= 0x3FFF'FFFF'FFFF'FFFF ) n +=  2, x <<=  2;
    if ( x <= 0x7FFF'FFFF'FFFF'FFFF ) ++n;
    return n;
}


union large {
    unsigned long long ll;
    struct {
        unsigned long l, h;
    } s;
};

constexpr char table [ 64 ] = {
    32, 31, 99, 16, 99, 30,  3, 99, 15, 99, 99, 99, 29, 10,  2, 99,
    99, 99, 12, 14, 21, 99, 19, 99, 99, 28, 99, 25, 99,  9,  1, 99,
    17, 99,  4, 99, 99, 99, 11, 99, 13, 22, 20, 99, 26, 99, 99, 18,
     5, 99, 99, 23, 99, 27, 99,  6, 99, 24,  7, 99,  8, 99,  0, 99
};

constexpr int nlz10 ( std::uint32_t x ) noexcept {
    x = x | ( x >> 1 );    // Propagate leftmost
    x = x | ( x >> 2 );    // 1-bit to the right.
    x = x | ( x >> 4 );
    x = x | ( x >> 8 );
    x = x | ( x >> 16 );
    x = x * 0x06EB14F9;    // Multiplier is 7*255**3.
    return table [ x >> 26 ];
}

unsigned long leading_zeros_intrin_impl ( large x ) {
    unsigned long c = 0;
    if ( not ( x.s.h ) ) {
        _BitScanReverse ( &c, x.s.l );
        return 63 - c;
    }
    _BitScanReverse ( &c, x.s.h );
    return 31 - c;
}

std::uint32_t leading_zeros_intrin ( std::uint64_t x ) {
    return leading_zeros_intrin_impl ( *reinterpret_cast<large*> ( &x ) );
}


unsigned long leading_zeros_nlz10_impl ( large x ) {
    if ( not ( x.s.h ) ) {
        return nlz10 ( x.s.l ) + 32;
    }
    return nlz10 ( x.s.h );
}

std::uint32_t leading_zeros_nlz10 ( std::uint64_t x ) {
    return leading_zeros_nlz10_impl ( *reinterpret_cast<large*> ( &x ) );
}


template<typename Rng>
std::uint64_t bounded_random ( Rng & rng, std::uint64_t range_ ) {
    --range_;
    int zeros = leading_zeros_hackers_delight ( range_ | std::uint64_t { 1 } );
    std::uint64_t mask = UINT64_MAX >> zeros;
    while ( true ) {
        std::uint64_t r = rng ( );
        std::uint64_t v = r & mask;
        if ( v <= range_ ) {
            return v;
        }
        int shift = 32;
        while ( zeros >= shift ) {
            r >>= shift;
            v = r & mask;
            if ( v <= range_ ) {
                return v;
            }
            shift = 64 - ( 64 - shift ) / 2;
        }
    }
}

#include "plf_nanotimer.h"


int main ( ) {

    splitmix64 rng ( [ ] ( ) { std::random_device rdev; return rdev ( ); } ( ) );

    plf::nanotimer t;

    std::uint64_t x = 0;

    t.start ( );

    for ( int k = 0; k < 1'000'000; k++ ) {
        x += bounded_random ( rng, std::uint64_t { 1 } << 63 );
    }

    auto r = t.get_elapsed_us ( );

    std::cout << ( std::uint64_t ) r << std::endl;
    std::cout << x << std::endl;

    return 0;
}

