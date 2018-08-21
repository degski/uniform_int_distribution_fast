
// MIT License
//
// C++17-compliant uniform_int_distribution_fast, based on bounded_rand-function
// as per the paper by Daniel Lemire:
// https://arxiv.org/abs/1805.10941.
// and optimizations added to bounded_rand by Melissa E. O'Neill:
// http://www.pcg-random.org/posts/bounded-rands.html
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

#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
    #define NOMINMAX_LOCALLY_DEFINED
    #define NOMINMAX
#endif
#endif

#if ! defined ( _DEBUG )
    #define NOEXCEPT
    #if defined ( _HAS_EXCEPTIONS )
        #define ORG_HAS_EXCEPTIONS _HAS_EXCEPTIONS
        #undef _HAS_EXCEPTIONS
    #endif
    #define _HAS_EXCEPTIONS 0
#else
    #define NOEXCEPT noexcept
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if UINTPTR_MAX == 0xFFFF'FFFF
    #define MEMORY_MODEL_32 1
    #define MEMORY_MODEL_64 0
#elif UINTPTR_MAX == 0xFFFF'FFFF'FFFF'FFFF
    #define MEMORY_MODEL_32 0
    #define MEMORY_MODEL_64 1
#else
    #error funny pointers detected
#endif

#include <functional>
#include <limits>
#include <random>
#include <type_traits>

#if defined ( _WIN32 ) && ! ( defined ( __clang__ ) || defined ( __GNUC__ ) ) // MSVC and not clang or gcc on windows.
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

#if MEMORY_MODEL_32
    #include <absl/numeric/int128.h>
#endif


namespace ext {

template<typename IntType = int>
class uniform_int_distribution_fast;

namespace detail {

struct uint32uint32_t {
    std::uint32_t low, high;
};

#if MSVC
unsigned long leading_zeros_intrin_32 ( uint32uint32_t x ) {
    unsigned long c = 0u;
    if ( ! ( x.high ) ) {
        _BitScanReverse ( &c, x.low );
        return 63u - c;
    }
    _BitScanReverse ( &c, x.high );
    return 31u - c;
}
#else
int leading_zeros_intrin_32 ( uint32uint32_t x ) {
    if ( ! ( x.high ) ) {
        return __builtin_clz ( x.low ) + 32;
    }
    return __builtin_clz ( x.high );
}
#endif


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
    if constexpr ( MEMORY_MODEL_32 ) {
        return leading_zeros_intrin_32 ( *reinterpret_cast<uint32uint32_t*> ( &x ) );
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


template<typename Gen>
struct generator_reference : public std::reference_wrapper<Gen> {

    using std::reference_wrapper<Gen>::reference_wrapper;

    using result_type = typename Gen::result_type;

    [[ nodiscard ]] static constexpr result_type min ( ) NOEXCEPT { return Gen::min ( ); }
    [[ nodiscard ]] static constexpr result_type max ( ) NOEXCEPT { return Gen::max ( ); }
};

template<typename Gen, typename UnsignedResultType, bool HasBitEngine>
struct bits_engine { };
template<typename Gen, typename UnsignedResultType>
struct bits_engine<Gen, UnsignedResultType, true> : public std::independent_bits_engine<generator_reference<Gen>, std::numeric_limits<UnsignedResultType>::digits, UnsignedResultType> {
    explicit bits_engine ( Gen & gen ) : std::independent_bits_engine<generator_reference<Gen>, std::numeric_limits<UnsignedResultType>::digits, UnsignedResultType> ( gen ) { }
};
template<typename Gen, typename UnsignedResultType>
struct bits_engine<Gen, UnsignedResultType, false> : public generator_reference<Gen> {
    explicit bits_engine ( Gen & gen ) : generator_reference<Gen> ( gen ) { }
};


template<typename Type>
using unsigned_result_type = typename std::make_unsigned<Type>::type;

template<typename IT> struct double_width_integer { };
template<> struct double_width_integer<std::uint8_t > { using type = std::uint16_t; };
template<> struct double_width_integer<std::uint16_t> { using type = std::uint32_t; };
template<> struct double_width_integer<std::uint32_t> { using type = std::uint64_t; };
#if MEMORY_MODEL_32
template<> struct double_width_integer<std::uint64_t> { using type = absl::uint128; };
#endif


template<typename IntType>
using is_distribution_result_type =
std::disjunction <
    std::is_same<typename std::make_unsigned<IntType>::type, std::uint16_t>,
    std::is_same<typename std::make_unsigned<IntType>::type, std::uint32_t>,
    std::is_same<typename std::make_unsigned<IntType>::type, std::uint64_t>
>;

template<typename IntType, typename Distribution>
struct param_type {

    using result_type = IntType;
    using distribution_type = Distribution;

    using unsigned_result_type = typename std::make_unsigned<result_type>::type;

    friend class ::ext::uniform_int_distribution_fast<result_type>;

    explicit param_type ( result_type min_, result_type max_ ) NOEXCEPT :
        min ( min_ ),
        range ( max_ - min_ + 1 ) { // wraps to 0 for unsigned max.
        if constexpr ( MEMORY_MODEL_32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
            --range;
        }
    }

    [[ nodiscard ]] constexpr bool operator == ( const param_type & rhs ) const NOEXCEPT {
        return min == rhs.min and range == rhs.range;
    }

    [[ nodiscard ]] constexpr bool operator != ( const param_type & rhs ) const NOEXCEPT {
        return not ( *this == rhs );
    }

    [[ nodiscard ]] constexpr result_type a ( ) const NOEXCEPT {
        return min;
    }

    [[ nodiscard ]] constexpr result_type b ( ) const NOEXCEPT {
        if constexpr ( MEMORY_MODEL_32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
            return ( range + 1 ) ? range + min : std::numeric_limits<result_type>::max ( );
        }
        else {
            return range ? range + min - 1 : std::numeric_limits<result_type>::max ( );
        }
    }

    private:

    result_type min;
    unsigned_result_type range;
};
}



template<typename IntType>
class uniform_int_distribution_fast : public detail::param_type<IntType, uniform_int_distribution_fast<IntType>> {

    static_assert ( detail::is_distribution_result_type<IntType>::value, "char (8-bit) not supported." );

    public:

    using result_type = IntType;
    using param_type = detail::param_type<result_type, uniform_int_distribution_fast>;

    private:

    friend param_type;

    using pt = param_type;
    using unsigned_result_type = typename std::make_unsigned<result_type>::type;
    //#if MSVC
    using double_width_unsigned_result_type = unsigned_result_type; // dummy.
    //#else
    //using double_width_unsigned_result_type = typename double_width_integer<unsigned_result_type>::type;
    //#endif
    [[ nodiscard ]] constexpr unsigned_result_type range_max ( ) const NOEXCEPT {
        return unsigned_result_type { 1 } << ( sizeof ( unsigned_result_type ) * 8 - 1 );
    }

    template<typename Gen>
    using generator_reference = detail::bits_engine<Gen, unsigned_result_type, ( Gen::max ( ) < std::numeric_limits<unsigned_result_type>::max ( ) )>;

    public:

    explicit uniform_int_distribution_fast ( ) NOEXCEPT :
        param_type ( std::numeric_limits<result_type>::min ( ), std::numeric_limits<result_type>::max ( ) ) { }
    explicit uniform_int_distribution_fast ( result_type a, result_type b = std::numeric_limits<result_type>::max ( ) ) NOEXCEPT :
        param_type ( a, b ) {
        assert ( b > a );
    }
    explicit uniform_int_distribution_fast ( const param_type & params_ ) NOEXCEPT :
        param_type ( params_ ) { }

    void reset ( ) const NOEXCEPT { }

    template<typename Gen>
    [[ nodiscard ]] result_type operator ( ) ( Gen & rng ) const NOEXCEPT {
        static generator_reference<Gen> rng_ref ( rng );
        return 1;
    }

    template<typename Gen>
    [[ nodiscard ]] result_type generate ( Gen & rng ) const NOEXCEPT {
        static generator_reference<Gen> rng_ref ( rng );
        return 1;
    }

    [[ nodiscard ]] param_type param ( ) const NOEXCEPT {
        return *this;
    }

    void param ( const param_type & params ) NOEXCEPT {
        *this = params;
    }
};
}


// restore warnings

#if MSVC
    #pragma warning ( pop )
#endif

// macro cleanup

#if defined ( NOMINMAX_LOCALLY_DEFINED )
    #undef NOMINMAX
    #undef NOMINMAX_LOCALLY_DEFINED
#endif

#undef _HAS_EXCEPTIONS
#if defined ( ORG_HAS_EXCEPTIONS )
    #define _HAS_EXCEPTIONS ORG_HAS_EXCEPTIONS
    #undef ORG_HAS_EXCEPTIONS
#endif

#undef GNU
#undef MSVC
#undef MEMORY_MODEL_64
#undef MEMORY_MODEL_32
#undef NOEXCEPT
