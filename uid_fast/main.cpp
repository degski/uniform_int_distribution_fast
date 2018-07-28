
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

#include <cassert>
#include <ciso646>
#include <cstddef>
#include <cstdint>

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

#include "uniform_int_distribution_fast.hpp"


int main ( ) {

    splitmix32 rng ( [ ] ( ) { std::random_device rdev; return ( static_cast<std::uint64_t> ( rdev ( ) ) << 32 ) | rdev ( ); } ( ) );
    // ext::uniform_int_distribution_fast<std::uint64_t> dis ( 0, ( std::uint64_t { 1 } << 63 ) - 100 );
    ext::uniform_int_distribution_fast<std::uint64_t> dis ( 0, 9 );

    std::array<std::uint64_t, 10> freq { 0 };

    for ( std::size_t k = 0; k < std::numeric_limits<std::size_t>::max ( ); ++k ) {
        ++freq [ static_cast<std::size_t> ( dis.generate ( rng ) ) ];
    }

    std::cout << dis.a ( ) << " " << dis.b ( ) << std::endl;

    for ( auto v : freq ) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    return EXIT_SUCCESS;
}

// -Xclang -fcxx-exceptions -Xclang -std=c++2a -Xclang -pedantic -Qunused-arguments -Xclang -ffast-math -Xclang -Wno-deprecated-declarations -Xclang -Wno-unknown-pragmas -Xclang -Wno-ignored-pragmas -Xclang -Wno-unused-private-field  -mmmx  -msse  -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -Xclang -Wno-unused-variable -Xclang -Wno-language-extension-token

/*

#if defined ( _WIN32 ) and not ( defined ( __clang__ ) or defined ( __GNUC__ ) )
#include <intrin.h>
#ifdef _WIN64
#define MSVC32 0
#define MSVC64 1
#pragma intrinsic ( _umul128 )
#pragma intrinsic ( _BitScanReverse )
#pragma intrinsic ( _BitScanReverse64 )
#else
#define MSVC32 1
#define MSVC64 0
unsigned __int64 _umul128 ( unsigned __int64 Multiplier, unsigned __int64 Multiplicand, unsigned __int64 *HighProduct );
#pragma intrinsic ( _BitScanReverse )
unsigned char _BitScanReverse64 ( unsigned long *, unsigned long long );
#endif
#define GNU 0
#define MSVC 1
#pragma warning ( push )
#pragma warning ( disable : 4244 )
#else
#define GNU 1
#define MSVC 0
#define MSVC32 0
#define MSVC64 0
unsigned __int64 _umul128 ( unsigned __int64 Multiplier, unsigned __int64 Multiplicand, unsigned __int64 *HighProduct );
unsigned char _BitScanReverse ( unsigned long *, unsigned long );
unsigned char _BitScanReverse64 ( unsigned long *, unsigned long long );
#endif

struct model {
    enum : std::size_t { value = ( sizeof ( void* ) * 8 ) };
};

union large {
    unsigned long long ull;
    struct {
        unsigned long low, high;
    } ul;
};

__forceinline unsigned long leading_zeros_intrin_32 ( large x ) {
    unsigned long c = 0u;
    if ( not ( x.ul.high ) ) {
        _BitScanReverse ( &c, x.ul.low );
        return 63u - c;
    }
    _BitScanReverse ( &c, x.ul.high );
    return 31u - c;
}

std::uint32_t leading_zeros ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return leading_zeros_intrin_32 ( *reinterpret_cast<large*> ( &x ) );
    }
    else {
        unsigned long c;
        _BitScanReverse64 ( &c, x );
        return 63u - c;
    }
}

int leading_zeros_intrin_32_clang ( large x ) {
    if ( not ( x.ul.high ) ) {
        return __builtin_clz ( x.ul.low ) + 32;
    }
    return __builtin_clz ( x.ul.high );
}
int leading_zeros_clang ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return leading_zeros_intrin_32_clang ( *reinterpret_cast< large* > ( &x ) );
    }
    else {
        return __builtin_clzll ( x );
    }
}

template<typename Rng>
std::uint64_t bounded_rand_ ( Rng & rng, std::uint64_t range_ ) noexcept {
    --range_;
    unsigned long zeros = leading_zeros ( range_ | std::uint64_t { 1 } );
    unsigned int zeros2 = leading_zeros_clang ( range_ | 1 );
    if ( zeros != zeros2 ) {
        std::cout << "error\n";
        exit ( 0 );
    }
    const std::uint64_t mask = UINT64_MAX >> zeros;
    while ( true ) {
        std::uint64_t r = rng ( );
        std::uint64_t v = r & mask;
        if ( v <= range_ ) {
            return v;
        }
        unsigned long shift = 32;
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

#include <cstdint>

#include <array>
#include <random>


// BITMASK

template<typename Rng>
std::uint64_t bounded_rand ( Rng & rng, std::uint64_t range ) noexcept {
    --range;
    std::uint64_t mask = UINT64_MAX;
    mask >>= __builtin_clzll ( range | 1 );
    std::uint64_t x;
    do {
        x = rng ( ) & mask;
    } while ( x > range );
    return x;
}

template<typename Rng>
std::uint64_t bounded_rand_alt ( Rng & rng, std::uint64_t range ) noexcept {
    --range;
    unsigned int zeros = __builtin_clzll ( range | 1 );
    std::uint64_t mask = ( UINT64_MAX >> zeros );
    for ( ; ; ) {
        std::uint64_t r = rng ( );
        std::uint64_t v = r & mask;
        if ( v <= range )
            return v;
        unsigned int shift = 32;
        while ( zeros >= shift ) {
            r >>= shift;
            v = r & mask;
            if ( v <= range )
                return v;
            shift = 64 - ( 64 - shift ) / 2;
        }
    }
}

int main ( ) {

    std::mt19937_64 rng ( 123 );
    std::array<std::uint64_t, 10> freq1 { 0 };

    for ( std::uint64_t k = 0; k < 10'000'000; ++k ) {
        ++freq1 [ static_cast<std::size_t> ( bounded_rand ( rng, 10 ) ) ];
    }

    std::array<std::uint64_t, 10> freq2 { 0 };

    for ( std::uint64_t k = 0; k < 10'000'000; ++k ) {
        ++freq2 [ static_cast<std::size_t> ( bounded_rand_alt ( rng, 10 ) ) ];
    }

    for ( auto v : freq1 ) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    for ( auto v : freq2 ) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    return EXIT_SUCCESS;
}

*/
