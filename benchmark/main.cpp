
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
#include <limits>
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


#if defined ( _WIN32 ) and not ( defined ( __clang__ ) or defined ( __GNUC__ ) ) // MSVC and not clang or gcc on windows.
#include <intrin.h>
#ifdef _WIN64
#pragma intrinsic ( _umul128 )
#pragma intrinsic ( _BitScanReverse )
#pragma intrinsic ( _BitScanReverse64 )
#else
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
unsigned __int64 _umul128 ( unsigned __int64 Multiplier, unsigned __int64 Multiplicand, unsigned __int64 *HighProduct );
unsigned char _BitScanReverse ( unsigned long *, unsigned long );
unsigned char _BitScanReverse64 ( unsigned long *, unsigned long long );
#endif

#if UINTPTR_MAX == 0xFFFF'FFFF
#define MODEL32 1
#define MODEL64 0
#elif UINTPTR_MAX == 0xFFFF'FFFF'FFFF'FFFF
#define MODEL32 0
#define MODEL64 1
#else
#error funny pointers
#endif


struct model {
    enum : std::size_t { value = ( sizeof ( void* ) * 8 ) };
};

namespace detail {

struct large {
    unsigned long low, high;
};
#if MSVC
__forceinline unsigned long leading_zeros_intrin_32 ( large x ) {
    unsigned long c = 0u;
    if ( not ( x.high ) ) {
        _BitScanReverse ( &c, x.low );
        return 63u - c;
    }
    _BitScanReverse ( &c, x.high );
    return 31u - c;
}
#else
__attribute__ ( ( always_inline ) ) int leading_zeros_intrin_32 ( large x ) {
    if ( not ( x.high ) ) {
        return __builtin_clz ( x.low ) + 32;
    }
    return __builtin_clz ( x.high );
}
#endif
}

std::uint32_t leading_zeros_intrin ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return detail::leading_zeros_intrin_32 ( *reinterpret_cast<detail::large*> ( &x ) );
    }
    else {
        #if MSVC
        unsigned long c;
        _BitScanReverse64 ( &c, x );
        return 63u - c;
        #else
        return __builtin_clzll ( x );
        #endif
    }
}

template<typename Rng>
std::uint64_t br_stl ( Rng & rng, std::uint64_t range ) noexcept {
    std::uniform_int_distribution<uint64_t> dist ( 0, range - 1 );
    return dist ( rng );
}
template<typename Rng>
std::uint64_t br_bitmask_alt ( Rng & rng, std::uint64_t range_ ) noexcept {
    --range_;
    std::uint32_t zeros = leading_zeros_intrin ( range_ | std::uint64_t { 1 } );
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
std::uint64_t br_modx2_topt ( Rng & rng, std::uint64_t range ) {
    std::uint64_t r = rng ( );
    if ( r < range ) {
        std::uint64_t t = ( 0 - range ) % range;
        while ( r < t )
            r = rng ( );
    }
    return r % range;
}
template<typename Rng>
std::uint64_t br_modx2_topt_moptx2 ( Rng & rng, std::uint64_t range ) {
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
std::uint64_t br_modx1_mopt ( Rng & rng, std::uint64_t range ) {
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
std::uint64_t br_modx1 ( Rng & rng, std::uint64_t range ) {
    std::uint64_t x, r;
    do {
        x = rng ( );
        r = x % range;
    } while ( x - r > std::uint64_t ( 0 - range ) );
    return r;
}
template<typename Rng>
std::uint64_t br_bitmask ( Rng & rng, std::uint64_t range ) noexcept {
    --range;
    std::uint64_t mask = UINT64_MAX;
    mask >>= leading_zeros_intrin ( range | 1 );
    std::uint64_t x;
    do {
        x = rng ( ) & mask;
    } while ( x > range );
    return x;
}
#if MODEL64
template<typename Type>
using unsigned_result_type = typename std::make_unsigned<Type>::type;
#if GNU
template<typename IT> struct double_width_integer { };
template<> struct double_width_integer<std::uint16_t> { using type = std::uint32_t; };
template<> struct double_width_integer<std::uint32_t> { using type = std::uint64_t; };
template<> struct double_width_integer<std::uint64_t> { using type = __uint128_t; };

template<typename Rng, typename ResultType = std::uint64_t>
std::uint64_t br_lemire_oneill ( Rng & rng, std::uint64_t range ) noexcept {
    using double_width_unsigned_result_type = typename double_width_integer<unsigned_result_type<ResultType>>::type;
    unsigned_result_type<ResultType> x = rng ( );
    if ( range >= ( unsigned_result_type<ResultType> { 1 } << ( sizeof ( unsigned_result_type<ResultType> ) * 8 - 1 ) ) ) {
        do {
            x = rng ( );
        } while ( x >= range );
        return ResultType ( x );
    }
    double_width_unsigned_result_type m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( range );
    unsigned_result_type<ResultType> l = unsigned_result_type<ResultType> ( m );
    if ( l < range ) {
        unsigned_result_type<ResultType> t = -range;
        t -= range;
        if ( t >= range ) {
            t %= range;
        }
        while ( l < t ) {
            x = rng ( );
            m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( range );
            l = unsigned_result_type<ResultType> ( m );
        }
    }
    return ResultType ( m >> std::numeric_limits<unsigned_result_type<ResultType>>::digits );
}
#else
template<typename Rng, typename ResultType = std::uint64_t>
std::uint64_t br_lemire_oneill ( Rng & rng, std::uint64_t range ) noexcept {
    unsigned_result_type<ResultType> x = rng ( );
    if ( range >= ( unsigned_result_type<ResultType> { 1 } << ( sizeof ( unsigned_result_type<ResultType> ) * 8 - 1 ) ) ) {
        do {
            x = rng ( );
        } while ( x >= range );
        return ResultType ( x );
    }
    unsigned_result_type<ResultType> h, l = _umul128 ( x, range, &h );
    if ( l < range ) {
        unsigned_result_type<ResultType> t = 0 - range;
        t -= range;
        if ( t >= range ) {
            t %= range;
        }
        while ( l < t ) {
            l = _umul128 ( rng ( ), range, &h );
        }
    }
    return h;
}
#endif
#endif

#define BM_BR_TEMPLATE( name, size, range_ ) \
template<class Gen> \
void bm_br_##name##_##size ( benchmark::State & state ) noexcept { \
    static std::uint64_t seed = 0xBE1C0467EBA5FAC; \
    seed *= 0x1AEC805299990163; \
    seed ^= ( seed >> 32 ); \
    Gen gen ( seed ); \
    typename Gen::result_type a = 0; \
    benchmark::DoNotOptimize ( &a ); \
    for ( auto _ : state ) { \
        state.PauseTiming ( ); \
        const std::uint64_t range = std::poisson_distribution<std::uint64_t> ( ( range_ ) ) ( gen ); \
        state.ResumeTiming ( ); \
        for ( int i = 0; i < 128; ++i ) { \
            a += br_##name ( gen, range ); \
        } \
    } \
} \
BENCHMARK_TEMPLATE ( bm_br_##name##_##size, splitmix64 ) \
->Repetitions ( 16 ) \
->ReportAggregatesOnly ( true );


BM_BR_TEMPLATE ( stl, small, 1'000.0 )
#if MODEL64
BM_BR_TEMPLATE ( lemire_oneill, small, 1'000.0 )
#endif
BM_BR_TEMPLATE ( bitmask, small, 1'000.0 )
BM_BR_TEMPLATE ( bitmask_alt, small, 1'000.0 )
BM_BR_TEMPLATE ( modx1, small, 1'000.0 )
BM_BR_TEMPLATE ( modx2_topt, small, 1'000.0 )
BM_BR_TEMPLATE ( modx1_mopt, small, 1'000.0 )
BM_BR_TEMPLATE ( modx2_topt_moptx2, small, 1'000.0 )

BM_BR_TEMPLATE ( stl, medium, 1'000'000.0 )
#if MODEL64
BM_BR_TEMPLATE ( lemire_oneill, medium, 1'000'000.0 )
#endif
BM_BR_TEMPLATE ( bitmask, medium, 1'000'000.0 )
BM_BR_TEMPLATE ( bitmask_alt, medium, 1'000'000.0 )
BM_BR_TEMPLATE ( modx1, medium, 1'000'000.0 )
BM_BR_TEMPLATE ( modx2_topt, medium, 1'000'000.0 )
BM_BR_TEMPLATE ( modx1_mopt, medium, 1'000'000.0 )
BM_BR_TEMPLATE ( modx2_topt_moptx2, medium, 1'000'000.0 )

BM_BR_TEMPLATE ( stl, large, 1'000'000'000'000.0 )
#if MODEL64
BM_BR_TEMPLATE ( lemire_oneill, large, 1'000'000'000'000.0 )
#endif
BM_BR_TEMPLATE ( bitmask, large, 1'000'000'000'000.0 )
BM_BR_TEMPLATE ( bitmask_alt, large, 1'000'000'000'000.0 )
BM_BR_TEMPLATE ( modx1, large, 1'000'000'000'000.0 )
BM_BR_TEMPLATE ( modx2_topt, large, 1'000'000'000'000.0 )
BM_BR_TEMPLATE ( modx1_mopt, large, 1'000'000'000'000.0 )
BM_BR_TEMPLATE ( modx2_topt_moptx2, large, 1'000'000'000'000.0 )
