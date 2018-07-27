
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

#include <benchmark/benchmark.h>

#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#endif

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

struct model {
    enum : std::size_t { value = ( sizeof ( void* ) * 8 ) };
};

constexpr int leading_zeros_hackers_delight ( std::uint64_t x ) noexcept {
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


#include <intrin.h>

#pragma intrinsic ( _BitScanReverse )
#ifdef _WIN64
#pragma intrinsic ( _BitScanReverse64 )
#else
unsigned char _BitScanReverse64 ( unsigned long *, unsigned long long );
#endif

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

std::uint32_t leading_zeros1 ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return leading_zeros_intrin_32 ( *reinterpret_cast<large*> ( &x ) );
    }
    else {
        unsigned long c;
        _BitScanReverse64 ( &c, x );
        return 63u - c;
    }
}

std::uint32_t leading_zeros2 ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        // returns number of leading zeroes, from Hackers Delight - Henry Warren:
        // http://hackersdelight.org/
        std::uint32_t n = 0;
        if ( x <= 0x0000'0000'FFFF'FFFF ) n += 32, x <<= 32;
        if ( x <= 0x0000'FFFF'FFFF'FFFF ) n += 16, x <<= 16;
        if ( x <= 0x00FF'FFFF'FFFF'FFFF ) n += 8, x <<= 8;
        if ( x <= 0x0FFF'FFFF'FFFF'FFFF ) n += 4, x <<= 4;
        if ( x <= 0x3FFF'FFFF'FFFF'FFFF ) n += 2, x <<= 2;
        if ( x <= 0x7FFF'FFFF'FFFF'FFFF ) ++n;
        return n;
    }
    else {
        unsigned long c;
        _BitScanReverse64 ( &c, x );
        return 63u - c;
    }
}

template<typename Rng>
std::uint64_t random_bounded1 ( Rng & rng, std::uint64_t range_ ) noexcept {
    --range_;
    std::uint32_t zeros = leading_zeros1 ( range_ | std::uint64_t { 1 } );
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

template<typename Rng>
std::uint64_t random_bounded2 ( Rng & rng, std::uint64_t range_ ) noexcept {
    --range_;
    std::uint32_t zeros = leading_zeros2 ( range_ | std::uint64_t { 1 } );
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

template<class Gen>
void bm_random_bounded1 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += random_bounded1 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}

template<class Gen>
void bm_random_bounded2 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += random_bounded2 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
constexpr int repeats = 16;

BENCHMARK_TEMPLATE ( bm_random_bounded1, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_random_bounded2, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );




int main2334234 ( ) {

    splitmix64 rng ( [ ] ( ) { std::random_device rdev; return rdev ( ); } ( ) );

    std::uint64_t x = 0;

    for ( int k = 0; k < 1'000; k++ ) {
        x += random_bounded2 ( rng, std::uint64_t { 1 } << 63 );
    }

    std::cout << x << std::endl;

    return 0;
}

