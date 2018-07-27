
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

#include <iostream>
#include <random>
#include <type_traits>

#include <intrin.h>

#pragma intrinsic ( _BitScanReverse )
#ifdef _WIN64
#pragma intrinsic ( _BitScanReverse64 )
#else
unsigned char _BitScanReverse64 ( unsigned long *, unsigned long long );
#endif


#include <benchmark/benchmark.h>

#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#endif

#include "../uid_fast/splitmix.hpp"


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

std::uint32_t leading_zeros1 ( std::uint64_t x ) noexcept {
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
std::uint32_t leading_zeros2 ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return leading_zeros_intrin_32 ( *reinterpret_cast<large*> ( &x ) );
    }
    else {
        unsigned long c;
        _BitScanReverse64 ( &c, x );
        return 63u - c;
    }
}


template<typename Rng>
std::uint64_t bounded_rand3 ( Rng & rng, std::uint64_t range ) noexcept {
    std::uniform_int_distribution<uint64_t> dist ( 0, range - 1 );
    return dist ( rng );
}
template<typename Rng>
std::uint64_t bounded_rand1 ( Rng & rng, std::uint64_t range_ ) noexcept {
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
std::uint64_t bounded_rand2 ( Rng & rng, std::uint64_t range_ ) noexcept {
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
template<typename Rng>
std::uint64_t bounded_rand4 ( Rng & rng, std::uint64_t range ) {
    std::uint64_t r = rng ( );
    if ( r < range ) {
        std::uint64_t t = ( 0 - range ) % range;
        while ( r < t )
            r = rng ( );
    }
    return r % range;
}
template<typename Rng>
std::uint64_t bounded_rand5 ( Rng & rng, std::uint64_t range ) {
    std::uint64_t r = rng ( );
    if ( r < range ) {
        std::uint64_t t = 0 - range;
        if ( t >= range ) {
            t -= range;
            if ( t >= range )
                t %= range;
        }
        while ( r < t )
            r = rng ( );
    }
    if ( r >= range ) {
        r -= range;
        if ( r >= range )
            r %= range;
    }
    return r;
}
template<typename Rng>
std::uint64_t bounded_rand6 ( Rng & rng, std::uint64_t range ) {
    std::uint64_t x, r;
    do {
        x = rng ( );
        r = x;
        if ( r >= range ) {
            r -= range;
            if ( r >= range )
                r %= range;
        }
    } while ( x - r > std::uint64_t ( 0 - range ) );
    return r;
}
template<typename Rng>
std::uint64_t bounded_rand7 ( Rng & rng, std::uint64_t range ) {
    std::uint64_t x, r;
    do {
        x = rng ( );
        r = x % range;
    } while ( x - r > ( 0-range ) );
    return r;
}
template<typename Rng>
std::uint64_t bounded_rand8 ( Rng & rng, std::uint64_t range ) noexcept {
    --range;
    std::uint64_t mask = UINT64_MAX;
    mask >>= leading_zeros1 ( range | 1 );
    std::uint64_t x;
    do {
        x = rng ( ) & mask;
    } while ( x > range );
    return x;
}

template<typename Rng>
std::uint64_t bounded_rand9 ( Rng & rng, std::uint64_t range ) noexcept {
    --range;
    std::uint64_t mask = UINT64_MAX;
    mask >>= leading_zeros2 ( range | 1 );
    std::uint64_t x;
    do {
        x = rng ( ) & mask;
    } while ( x > range );
    return x;
}

template<class Gen>
void bm_bounded_rand1 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand1 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand2 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand2 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand3 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand3 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand4 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand4 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand5 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand5 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand6 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand6 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand7 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand7 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand8 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand8 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}
template<class Gen>
void bm_bounded_rand9 ( benchmark::State & state ) noexcept {
    static std::uint64_t seed = 0xBE1C0467EBA5FAC;
    seed *= 0x1AEC805299990163;
    seed ^= ( seed >> 32 );
    Gen gen ( seed );
    typename Gen::result_type a = 0;
    benchmark::DoNotOptimize ( &a );
    for ( auto _ : state ) {
        for ( int i = 0; i < 128; ++i ) {
            a += bounded_rand9 ( gen, gen ( ) );
            //benchmark::ClobberMemory ( );
        }
    }
}



constexpr int repeats = 16;

BENCHMARK_TEMPLATE ( bm_bounded_rand3, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand1, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand2, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand4, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand5, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand6, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand7, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand8, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );

BENCHMARK_TEMPLATE ( bm_bounded_rand9, splitmix64 )
->Repetitions ( repeats )
->ReportAggregatesOnly ( true );
