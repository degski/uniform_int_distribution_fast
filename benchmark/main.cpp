
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

#if !defined ( _DEBUG )
#define NOEXCEPT
#define _HAS_EXCEPTIONS 0
#else
#define NOEXCEPT noexcept
#endif

#include <intrin.h>
#include <cassert>
#include <cstddef>
#include <cstdint>

#if UINTPTR_MAX == 0xFFFF'FFFF
#define M32 1
#define M64 0
#elif UINTPTR_MAX == 0xFFFF'FFFF'FFFF'FFFF
#define M32 0
#define M64 1
#else
#error funny pointers detected
#endif

#include <iostream>
#include <limits>
#include <random>
#include <type_traits>

// __builtin_expect

#include <benchmark/benchmark.h>

#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#endif

#include "../uid_fast/splitmix.hpp"

#if M32
#define generator splitmix64
#include <absl/numeric/int128.h>
#else
#define generator splitmix64
#endif

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

#if M32
#if GNU
#if __clang__
#define COMPILER clang_x86
#else
#define COMPILER gcc_x86
#endif
#else
#define COMPILER msvc_x86
#endif
#else
#if GNU
#if __clang__
#define COMPILER clang_x64
#else
#define COMPILER gcc_x64
#endif
#else
#define COMPILER msvc_x64
#endif
#endif


#if GNU
template<typename T>
static void do_not_optimize ( T * p ) {
    asm volatile ( "" : : "g"(p) : "memory" );
}
static void clobber_memory ( ) {
    asm volatile ( "" : : : "memory" );
}
#endif


namespace detail {

struct uint32uint32_t {
    std::uint32_t low, high;
};

#if MSVC
unsigned long leading_zeros_intrin_32 ( uint32uint32_t x ) {
    unsigned long c = 0u;
    if ( not ( x.high ) ) {
        _BitScanReverse ( &c, x.low );
        return 63u - c;
    }
    _BitScanReverse ( &c, x.high );
    return 31u - c;
}
#else
int leading_zeros_intrin_32 ( uint32uint32_t x ) {
    if ( not ( x.high ) ) {
        return __builtin_clz ( x.low ) + 32;
    }
    return __builtin_clz ( x.high );
}
#endif
}

std::uint32_t leading_zeros_intrin ( std::uint32_t x ) NOEXCEPT {
    #if MSVC
    unsigned long c;
    _BitScanReverse ( &c, x );
    return 63u - c;
    #else
    return __builtin_clz ( x );
    #endif
}

std::uint32_t leading_zeros_intrin ( std::uint64_t x ) NOEXCEPT {
    if constexpr ( M32 ) {
        return detail::leading_zeros_intrin_32 ( *reinterpret_cast<detail::uint32uint32_t*> ( &x ) );
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

template<typename Rng, typename Type>
Type br_stl ( Rng & rng, Type range ) NOEXCEPT {
    return std::uniform_int_distribution<Type> ( 0, range - 1 ) ( rng );
}

template<typename Rng, typename Type>
Type br_debiased_div ( Rng & rng, Type range ) NOEXCEPT {
    const Type divisor = ( ( 0 - range ) / range ) + 1;
    if ( divisor == 0 ) {
        return 0;
    }
    while ( true ) {
        Type val = rng ( ) / divisor;
        if ( val < range ) {
            return val;
        }
    }
}

template<typename Rng, typename Type>
Type br_bitmask ( Rng & rng, Type range ) NOEXCEPT {
    --range;
    Type mask = std::numeric_limits<Type>::max ( );
    mask >>= leading_zeros_intrin ( range | Type { 1 } );
    Type x;
    do {
        x = rng ( ) & mask;
    } while ( x > range );
    return x;
}

template<typename Rng, typename Type>
Type br_bitmask_alt ( Rng & rng, Type range_ ) NOEXCEPT {
    --range_;
    std::uint32_t zeros = leading_zeros_intrin ( range_ | Type { 1 } );
    const Type mask = std::numeric_limits<Type>::max ( ) >> zeros;
    while ( true ) {
        Type r = rng ( );
        Type v = r & mask;
        if ( v <= range_ ) {
            return v;
        }
        std::uint32_t shift = 32u;
        while ( zeros >= shift ) {
            r >>= shift;
            v = r & mask;
            if ( v <= range_ ) {
                return v;
            }
            shift = 64u - ( 64u - shift ) / 2;
        }
    }
}


template<typename Rng, typename Type>
Type br_modx1 ( Rng & rng, Type range ) NOEXCEPT {
    Type x, r;
    do {
        x = rng ( );
        r = x % range;
    } while ( x - r > Type ( 0 - range ) );
    return r;
}

template<typename Rng, typename Type>
Type br_modx1_bopt ( Rng & rng, Type range ) NOEXCEPT {
    Type x, r;
    if ( range >= Type { 1 } << ( std::numeric_limits<Type>::digits - 1 ) ) {
        do {
            r = rng ( );
        } while ( r >= range );
        return r;
    }
    do {
        x = rng ( );
        r = x % range;
    } while ( x - r > Type ( 0 - range ) );
    return r;
}

template<typename Rng, typename Type>
Type br_modx1_mopt ( Rng & rng, Type range ) NOEXCEPT {
    Type x, r;
    do {
        x = rng ( );
        r = x;
        if ( r >= range ) {
            r -= range;
            if ( r >= range )
                r %= range;
        }
    } while ( x - r > Type ( 0 - range ) );
    return r;
}

template<typename Rng, typename Type>
Type br_debiased_modx2 ( Rng & rng, Type range ) NOEXCEPT {
    Type t = ( 0 - range ) % range;
    while ( true ) {
        Type r = rng ( );
        if ( r >= t ) {
            return r % range;
        }
    }
}

template<typename Rng, typename Type>
Type br_modx2_topt ( Rng & rng, Type range ) NOEXCEPT {
    Type r = rng ( );
    if ( r < range ) {
        Type t = ( 0 - range ) % range;
        while ( r < t )
            r = rng ( );
    }
    return r % range;
}

template<typename Rng, typename Type>
Type br_modx2_topt_bopt ( Rng & rng, Type range ) NOEXCEPT {
    Type r = rng ( );
    if ( range >= Type { 1 } << ( std::numeric_limits<Type>::digits - 1 ) ) {
        while ( r >= range ) {
            r = rng ( );
        }
        return r;
    }
    if ( r < range ) {
        uint64_t t = ( 0 - range ) % range;
        while ( r < t ) {
            r = rng ( );
        }
    }
    return r % range;
}

template<typename Rng, typename Type>
Type br_modx2_topt_mopt ( Rng & rng, Type range ) NOEXCEPT {
    Type r = rng ( );
    if ( r < range ) {
        Type t = ( 0 - range );
        if ( t >= range ) {
            t -= range;
            if ( t >= range ) {
                t %= range;
            }
        }
        while ( r < t ) {
            r = rng ( );
        }
    }
    return r % range;
}

template<typename Rng, typename Type>
Type br_modx2_topt_moptx2 ( Rng & rng, Type range ) NOEXCEPT {
    Type r = rng ( );
    if ( r < range ) {
        Type t = ( 0 - range );
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

template<typename Type>
using unsigned_result_type = typename std::make_unsigned<Type>::type;

template<typename IT> struct double_width_integer { };
template<> struct double_width_integer<std::uint8_t > { using type = std::uint16_t; };
template<> struct double_width_integer<std::uint16_t> { using type = std::uint32_t; };
template<> struct double_width_integer<std::uint32_t> { using type = std::uint64_t; };
#if M32
template<> struct double_width_integer<std::uint64_t> { using type = absl::uint128; };
#endif
#if M64 and GNU
template<> struct double_width_integer<std::uint64_t> { using type = __uint128_t; };
#endif

template<typename Rng, typename Type, typename ResultType = Type>
ResultType br_lemire_oneill ( Rng & rng, Type range ) NOEXCEPT {
    #if MSVC and M64
    if constexpr ( std::is_same<Type, std::uint64_t>::value ) {
        unsigned_result_type<ResultType> x = rng ( );
        if ( range >= ( unsigned_result_type<ResultType> { 1 } << ( sizeof ( unsigned_result_type<ResultType> ) * 8 - 1 ) ) ) {
            do {
                x = rng ( );
            } while ( x >= range );
            return ResultType ( x );
        }
        unsigned_result_type<ResultType> h, l = _umul128 ( x, range, &h );
        if ( l < range ) {
            unsigned_result_type<ResultType> t = ( 0 - range );
            t -= range;
            if ( t >= range ) {
                t %= range;
            }
            while ( l < t ) {
                l = _umul128 ( rng ( ), range, &h );
            }
        }
        return ResultType ( h );
    }
    else { // range is of type std::uint32_t.
    #endif
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
            unsigned_result_type<ResultType> t = ( 0 - range );
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
    #if MSVC and M64
    }
    #endif
}


template<typename Rng, typename Type, typename ResultType = Type>
ResultType br_lemire ( Rng & rng, Type range ) NOEXCEPT {
    #if MSVC and M64
    if constexpr ( std::is_same<Type, std::uint64_t>::value ) {
        const unsigned_result_type<ResultType> t = ( 0 - range ) % range;
        unsigned_result_type<ResultType> x = rng ( );
        unsigned_result_type<ResultType> h, l = _umul128 ( x, range, &h );
        while ( l < t ) {
            x = rng ( );
            l = _umul128 ( x, range, &h );
        };
        return ResultType ( h );
    }
    else { // range is of type std::uint32_t.
    #endif
        using double_width_unsigned_result_type = typename double_width_integer<unsigned_result_type<ResultType>>::type;
        const unsigned_result_type<ResultType> t = ( 0 - range ) % range;
        unsigned_result_type<ResultType> x = rng ( );
        double_width_unsigned_result_type m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( range );
        unsigned_result_type<ResultType> l = unsigned_result_type<ResultType> ( m );
        while ( l < t ) {
            x = rng ( );
            m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( range );
            l = unsigned_result_type<ResultType> ( m );
        };
        return ResultType ( m >> std::numeric_limits<unsigned_result_type<ResultType>>::digits );
    #if MSVC and M64
    }
    #endif
}


#define BM_BR( func, name, shift ) \
void func ( benchmark::State & state ) NOEXCEPT { \
    static generator seeder ( 0xBE1C0467EBA5FAC ); \
    using result_type = typename generator::result_type; \
    result_type a = 0; \
    benchmark::DoNotOptimize ( &a ); \
    generator gen ( seeder ( ) ); \
    benchmark::DoNotOptimize ( &gen ); \
    for ( auto _ : state ) { \
        const result_type range = result_type { 1 } << state.range ( 0 ); \
        benchmark::DoNotOptimize ( &range ); \
        for ( int i = 0; i < 128; ++i ) { \
            a += br_##name ( gen, range ); \
            benchmark::ClobberMemory ( ); \
        } \
    } \
} \
BENCHMARK ( func ) \
->Repetitions ( 4 ) \
->ReportAggregatesOnly ( true ) \
->Arg ( shift );

// benchmark::DoNotOptimize ( &a );
// benchmark::ClobberMemory ( );

#define CAT7( V1, V2, V3, V4, V5, V6, V7 ) V1 ## V2 ## V3 ## V4 ## V5 ## V6 ## V7
#define FUNC( name, shift, compiler ) CAT7 ( bm_bounded_rand, _, name, _, shift, _, compiler )
#define BM_BR_F( name, shift ) BM_BR ( FUNC ( name, shift, COMPILER ), name, shift )


#define BM_BR_F_N( N ) \
BM_BR_F ( stl, N ) \
BM_BR_F ( lemire, N ) \
BM_BR_F ( lemire_oneill, N ) \
BM_BR_F ( bitmask, N ) \
BM_BR_F ( bitmask_alt, N )

/*
\
BM_BR_F ( modx1, N ) \
BM_BR_F ( modx1_bopt, N ) \
BM_BR_F ( modx1_mopt, N ) \
BM_BR_F ( debiased_modx2, N ) \
BM_BR_F ( modx2_topt, N ) \
BM_BR_F ( modx2_topt_bopt, N ) \
BM_BR_F ( modx2_topt_mopt, N ) \
BM_BR_F ( modx2_topt_moptx2, N )
*/

#if 1
BM_BR_F_N (  1 )
BM_BR_F_N (  2 )
BM_BR_F_N (  3 )
BM_BR_F_N (  4 )
BM_BR_F_N (  5 )
BM_BR_F_N (  6 )
BM_BR_F_N (  7 )
BM_BR_F_N (  8 )
BM_BR_F_N (  9 )
BM_BR_F_N ( 10 )
BM_BR_F_N ( 11 )
BM_BR_F_N ( 12 )
BM_BR_F_N ( 13 )
BM_BR_F_N ( 14 )
BM_BR_F_N ( 15 )
BM_BR_F_N ( 16 )
BM_BR_F_N ( 17 )
BM_BR_F_N ( 18 )
BM_BR_F_N ( 19 )
BM_BR_F_N ( 20 )
BM_BR_F_N ( 21 )
BM_BR_F_N ( 22 )
BM_BR_F_N ( 23 )
BM_BR_F_N ( 24 )
BM_BR_F_N ( 25 )
BM_BR_F_N ( 26 )
BM_BR_F_N ( 27 )
BM_BR_F_N ( 28 )
BM_BR_F_N ( 29 )
BM_BR_F_N ( 30 )
BM_BR_F_N ( 31 )

BM_BR_F_N ( 32 )
BM_BR_F_N ( 33 )
BM_BR_F_N ( 34 )
BM_BR_F_N ( 35 )
BM_BR_F_N ( 36 )
BM_BR_F_N ( 37 )
BM_BR_F_N ( 38 )
BM_BR_F_N ( 39 )
BM_BR_F_N ( 40 )
BM_BR_F_N ( 41 )
BM_BR_F_N ( 42 )
BM_BR_F_N ( 43 )
BM_BR_F_N ( 44 )
BM_BR_F_N ( 45 )
BM_BR_F_N ( 46 )
BM_BR_F_N ( 47 )
BM_BR_F_N ( 48 )
BM_BR_F_N ( 49 )
BM_BR_F_N ( 50 )
BM_BR_F_N ( 51 )
BM_BR_F_N ( 52 )
BM_BR_F_N ( 53 )
BM_BR_F_N ( 54 )
BM_BR_F_N ( 55 )
BM_BR_F_N ( 56 )
BM_BR_F_N ( 57 )
BM_BR_F_N ( 58 )
BM_BR_F_N ( 59 )
BM_BR_F_N ( 60 )
BM_BR_F_N ( 61 )
BM_BR_F_N ( 62 )
BM_BR_F_N ( 63 )

#else
BM_BR_F_N (  1 )
BM_BR_F_N (  2 )
BM_BR_F_N (  4 )
BM_BR_F_N (  8 )
BM_BR_F_N ( 16 )
BM_BR_F_N ( 24 )
BM_BR_F_N ( 25 )
BM_BR_F_N ( 26 )
BM_BR_F_N ( 27 )
BM_BR_F_N ( 28 )
BM_BR_F_N ( 29 )
BM_BR_F_N ( 30 )
BM_BR_F_N ( 31 )
#if M64
BM_BR_F_N ( 32 )
BM_BR_F_N ( 48 )
#if COMPILER == clang_x64
BM_BR_F_N ( 59 )
BM_BR_F_N ( 60 )
BM_BR_F_N ( 61 )
BM_BR_F_N ( 62 )
#endif
BM_BR_F_N ( 63 )
#endif
#endif
