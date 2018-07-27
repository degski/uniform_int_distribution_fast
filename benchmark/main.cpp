
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

/*

MSVC 15.7.5 (x86)

07/27/18 14:41:31
Running Z:\REPOS\uid_fast\benchmark\Win32\Release\benchmark.exe
Run on (4 X 1995 MHz CPU s)
CPU Caches:
L1 Data 32K (x2)
L1 Instruction 32K (x2)
L2 Unified 262K (x2)
L3 Unified 3145K (x1)
--------------------------------------------------------------------------------------
Benchmark                                               Time           CPU Iterations
--------------------------------------------------------------------------------------
bm_bounded_rand3<splitmix64>/repeats:16_mean        44525 ns      44378 ns      15448
bm_bounded_rand3<splitmix64>/repeats:16_median      44422 ns      44504 ns      15448
bm_bounded_rand3<splitmix64>/repeats:16_stddev        466 ns        506 ns      15448
bm_bounded_rand1<splitmix64>/repeats:16_mean         9091 ns       9038 ns      74667
bm_bounded_rand1<splitmix64>/repeats:16_median       9078 ns       8998 ns      74667
bm_bounded_rand1<splitmix64>/repeats:16_stddev         59 ns        157 ns      74667
bm_bounded_rand2<splitmix64>/repeats:16_mean         7652 ns       7608 ns      89600
bm_bounded_rand2<splitmix64>/repeats:16_median       7640 ns       7586 ns      89600
bm_bounded_rand2<splitmix64>/repeats:16_stddev         69 ns        125 ns      89600
bm_bounded_rand4<splitmix64>/repeats:16_mean        22281 ns      22156 ns      32000
bm_bounded_rand4<splitmix64>/repeats:16_median      22264 ns      21973 ns      32000
bm_bounded_rand4<splitmix64>/repeats:16_stddev        205 ns        394 ns      32000
bm_bounded_rand5<splitmix64>/repeats:16_mean        11489 ns      11429 ns      64000
bm_bounded_rand5<splitmix64>/repeats:16_median      11486 ns      11475 ns      64000
bm_bounded_rand5<splitmix64>/repeats:16_stddev         81 ns        160 ns      64000
bm_bounded_rand6<splitmix64>/repeats:16_mean        10644 ns      10574 ns      64000
bm_bounded_rand6<splitmix64>/repeats:16_median      10634 ns      10498 ns      64000
bm_bounded_rand6<splitmix64>/repeats:16_stddev        110 ns        147 ns      64000
bm_bounded_rand7<splitmix64>/repeats:16_mean        19801 ns      19723 ns      34462
bm_bounded_rand7<splitmix64>/repeats:16_median      19797 ns      19496 ns      34462
bm_bounded_rand7<splitmix64>/repeats:16_stddev        131 ns        287 ns      34462
bm_bounded_rand8<splitmix64>/repeats:16_mean         8576 ns       8567 ns      74667
bm_bounded_rand8<splitmix64>/repeats:16_median       8563 ns       8580 ns      74667
bm_bounded_rand8<splitmix64>/repeats:16_stddev         78 ns        142 ns      74667
bm_bounded_rand9<splitmix64>/repeats:16_mean         7212 ns       7204 ns      89600
bm_bounded_rand9<splitmix64>/repeats:16_median       7205 ns       7150 ns      89600
bm_bounded_rand9<splitmix64>/repeats:16_stddev         57 ns        105 ns      89600

MSVC 15.7.5 (x64)

07/27/18 14:56:06
Running Z:\REPOS\uid_fast\benchmark\x64\Release\benchmark.exe
Run on (4 X 1995 MHz CPU s)
CPU Caches:
L1 Data 32K (x2)
L1 Instruction 32K (x2)
L2 Unified 262K (x2)
L3 Unified 3145K (x1)
--------------------------------------------------------------------------------------
Benchmark                                               Time           CPU Iterations
--------------------------------------------------------------------------------------
bm_bounded_rand3<splitmix64>/repeats:16_mean         6709 ns       6703 ns      89600
bm_bounded_rand3<splitmix64>/repeats:16_median       6690 ns       6627 ns      89600
bm_bounded_rand3<splitmix64>/repeats:16_stddev         84 ns        110 ns      89600
bm_bounded_rand1<splitmix64>/repeats:16_mean         1713 ns       1707 ns     407273
bm_bounded_rand1<splitmix64>/repeats:16_median       1711 ns       1726 ns     407273
bm_bounded_rand1<splitmix64>/repeats:16_stddev         14 ns         24 ns     407273
bm_bounded_rand2<splitmix64>/repeats:16_mean         1675 ns       1666 ns     407273
bm_bounded_rand2<splitmix64>/repeats:16_median       1676 ns       1650 ns     407273
bm_bounded_rand2<splitmix64>/repeats:16_stddev          9 ns         20 ns     407273
bm_bounded_rand4<splitmix64>/repeats:16_mean         4492 ns       4486 ns     160000
bm_bounded_rand4<splitmix64>/repeats:16_median       4492 ns       4492 ns     160000
bm_bounded_rand4<splitmix64>/repeats:16_stddev         28 ns         43 ns     160000
bm_bounded_rand5<splitmix64>/repeats:16_mean         3428 ns       3419 ns     213333
bm_bounded_rand5<splitmix64>/repeats:16_median       3430 ns       3442 ns     213333
bm_bounded_rand5<splitmix64>/repeats:16_stddev         10 ns         35 ns     213333
bm_bounded_rand6<splitmix64>/repeats:16_mean         3094 ns       3090 ns     235789
bm_bounded_rand6<splitmix64>/repeats:16_median       3098 ns       3115 ns     235789
bm_bounded_rand6<splitmix64>/repeats:16_stddev         19 ns         33 ns     235789
bm_bounded_rand7<splitmix64>/repeats:16_mean         3496 ns       3496 ns     203636
bm_bounded_rand7<splitmix64>/repeats:16_median       3498 ns       3530 ns     203636
bm_bounded_rand7<splitmix64>/repeats:16_stddev         17 ns         39 ns     203636
bm_bounded_rand8<splitmix64>/repeats:16_mean         1661 ns       1652 ns     407273
bm_bounded_rand8<splitmix64>/repeats:16_median       1664 ns       1650 ns     407273
bm_bounded_rand8<splitmix64>/repeats:16_stddev         18 ns         30 ns     407273
bm_bounded_rand9<splitmix64>/repeats:16_mean         1659 ns       1654 ns     448000
bm_bounded_rand9<splitmix64>/repeats:16_median       1658 ns       1639 ns     448000
bm_bounded_rand9<splitmix64>/repeats:16_stddev         14 ns         22 ns     448000

Clang (trunk) (x86)

07/27/18 14:45:25
Running Z:\REPOS\uid_fast\benchmark\Win32\Release\benchmark.exe
Run on (4 X 1995 MHz CPU s)
CPU Caches:
L1 Data 32K (x2)
L1 Instruction 32K (x2)
L2 Unified 262K (x2)
L3 Unified 3145K (x1)
--------------------------------------------------------------------------------------
Benchmark                                               Time           CPU Iterations
--------------------------------------------------------------------------------------
bm_bounded_rand3<splitmix64>/repeats:16_mean        30914 ns      30643 ns      20364
bm_bounded_rand3<splitmix64>/repeats:16_median      30911 ns      30691 ns      20364
bm_bounded_rand3<splitmix64>/repeats:16_stddev        323 ns        440 ns      20364
bm_bounded_rand1<splitmix64>/repeats:16_mean         5965 ns       5938 ns     112000
bm_bounded_rand1<splitmix64>/repeats:16_median       5970 ns       5999 ns     112000
bm_bounded_rand1<splitmix64>/repeats:16_stddev         35 ns         88 ns     112000
bm_bounded_rand2<splitmix64>/repeats:16_mean         4410 ns       4395 ns     160000
bm_bounded_rand2<splitmix64>/repeats:16_median       4415 ns       4395 ns     160000
bm_bounded_rand2<splitmix64>/repeats:16_stddev         36 ns         62 ns     160000
bm_bounded_rand4<splitmix64>/repeats:16_mean        19606 ns      19592 ns      37333
bm_bounded_rand4<splitmix64>/repeats:16_median      19640 ns      19671 ns      37333
bm_bounded_rand4<splitmix64>/repeats:16_stddev        191 ns        228 ns      37333
bm_bounded_rand5<splitmix64>/repeats:16_mean         8839 ns       8828 ns      89600
bm_bounded_rand5<splitmix64>/repeats:16_median       8841 ns       8894 ns      89600
bm_bounded_rand5<splitmix64>/repeats:16_stddev         43 ns        125 ns      89600
bm_bounded_rand6<splitmix64>/repeats:16_mean         7873 ns       7880 ns      89600
bm_bounded_rand6<splitmix64>/repeats:16_median       7878 ns       7847 ns      89600
bm_bounded_rand6<splitmix64>/repeats:16_stddev         46 ns         95 ns      89600
bm_bounded_rand7<splitmix64>/repeats:16_mean        17191 ns      17168 ns      40727
bm_bounded_rand7<splitmix64>/repeats:16_median      17184 ns      17264 ns      40727
bm_bounded_rand7<splitmix64>/repeats:16_stddev        133 ns        262 ns      40727
bm_bounded_rand8<splitmix64>/repeats:16_mean         5339 ns       5319 ns     112000
bm_bounded_rand8<splitmix64>/repeats:16_median       5342 ns       5301 ns     112000
bm_bounded_rand8<splitmix64>/repeats:16_stddev         28 ns         70 ns     112000
bm_bounded_rand9<splitmix64>/repeats:16_mean         3953 ns       3951 ns     179200
bm_bounded_rand9<splitmix64>/repeats:16_median       3954 ns       3924 ns     179200
bm_bounded_rand9<splitmix64>/repeats:16_stddev         28 ns         61 ns     179200

Clang (trunk) (x64)

07/27/18 14:50:24
Running Z:\REPOS\uid_fast\benchmark\x64\Release\benchmark.exe
Run on (4 X 1995 MHz CPU s)
CPU Caches:
L1 Data 32K (x2)
L1 Instruction 32K (x2)
L2 Unified 262K (x2)
L3 Unified 3145K (x1)
--------------------------------------------------------------------------------------
Benchmark                                               Time           CPU Iterations
--------------------------------------------------------------------------------------
bm_bounded_rand3<splitmix64>/repeats:16_mean         5198 ns       5197 ns     112000
bm_bounded_rand3<splitmix64>/repeats:16_median       5199 ns       5162 ns     112000
bm_bounded_rand3<splitmix64>/repeats:16_stddev         41 ns         81 ns     112000
bm_bounded_rand1<splitmix64>/repeats:16_mean         1718 ns       1705 ns     407273
bm_bounded_rand1<splitmix64>/repeats:16_median       1715 ns       1688 ns     407273
bm_bounded_rand1<splitmix64>/repeats:16_stddev         20 ns         31 ns     407273
bm_bounded_rand2<splitmix64>/repeats:16_mean         1714 ns       1698 ns     407273
bm_bounded_rand2<splitmix64>/repeats:16_median       1714 ns       1688 ns     407273
bm_bounded_rand2<splitmix64>/repeats:16_stddev         11 ns         26 ns     407273
bm_bounded_rand4<splitmix64>/repeats:16_mean         4491 ns       4450 ns     154483
bm_bounded_rand4<splitmix64>/repeats:16_median       4489 ns       4450 ns     154483
bm_bounded_rand4<splitmix64>/repeats:16_stddev         29 ns         83 ns     154483
bm_bounded_rand5<splitmix64>/repeats:16_mean         3340 ns       3328 ns     203636
bm_bounded_rand5<splitmix64>/repeats:16_median       3336 ns       3299 ns     203636
bm_bounded_rand5<splitmix64>/repeats:16_stddev         25 ns         38 ns     203636
bm_bounded_rand6<splitmix64>/repeats:16_mean         2960 ns       2953 ns     235789
bm_bounded_rand6<splitmix64>/repeats:16_median       2961 ns       2982 ns     235789
bm_bounded_rand6<splitmix64>/repeats:16_stddev         16 ns         34 ns     235789
bm_bounded_rand7<splitmix64>/repeats:16_mean         3410 ns       3381 ns     203636
bm_bounded_rand7<splitmix64>/repeats:16_median       3411 ns       3376 ns     203636
bm_bounded_rand7<splitmix64>/repeats:16_stddev         22 ns         59 ns     203636
bm_bounded_rand8<splitmix64>/repeats:16_mean         1623 ns       1616 ns     407273
bm_bounded_rand8<splitmix64>/repeats:16_median       1621 ns       1611 ns     407273
bm_bounded_rand8<splitmix64>/repeats:16_stddev         18 ns         19 ns     407273
bm_bounded_rand9<splitmix64>/repeats:16_mean         1624 ns       1620 ns     448000
bm_bounded_rand9<splitmix64>/repeats:16_median       1625 ns       1604 ns     448000
bm_bounded_rand9<splitmix64>/repeats:16_stddev         14 ns         18 ns     448000

*/

