
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
#include <iostream>
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
        #pragma intrinsic ( _BitScanReverse )
    #endif
    #define GNU 0
    #define MSVC 1
    #define GCC 0
    #define CLANG 0
    #pragma warning ( push )
    #pragma warning ( disable : 4244 )
#else
    #define GNU 1
    #define MSVC 0
    #if defined ( __clang__ )
        #define CLANG 1
        #define GCC 0
    #else
        #define CLANG 0
        #define GCC 1
    #endif
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

template<typename Type>
std::uint32_t leading_zeros ( Type x ) NOEXCEPT {
    if constexpr ( std::is_same<Type, std::uint64_t>::value ) {
        if constexpr ( MEMORY_MODEL_32 ) {
            if constexpr ( MSVC ) {
                unsigned long c = 0u;
                if ( !( ( *reinterpret_cast<uint32uint32_t*> ( &x ) ).high ) ) {
                    _BitScanReverse ( &c, ( *reinterpret_cast<uint32uint32_t*> ( &x ) ).low );
                    return 63u - c;
                }
                _BitScanReverse ( &c, ( *reinterpret_cast<uint32uint32_t*> ( &x ) ).high );
                return 31u - c;
            }
            else { // GNU.
                if ( !( ( *reinterpret_cast<uint32uint32_t*> ( &x ) ).high ) ) {
                    return __builtin_clz ( ( *reinterpret_cast<uint32uint32_t*> ( &x ) ).low ) + 32;
                }
                return __builtin_clz ( ( *reinterpret_cast<uint32uint32_t*> ( &x ) ).high );
            }
        }
        else { // MEMORY_MODEL_64.
            if constexpr ( MSVC ) {
                unsigned long c;
                _BitScanReverse64 ( &c, x );
                return static_cast<std::uint32_t> ( std::numeric_limits<Type>::digits - 1 ) - c;
            }
            else { // GNU.
                return __builtin_clzll ( x );
            }
        }
    }
    else { // 16 or 32 bits.
        if constexpr ( MSVC ) {
            unsigned long c;
            _BitScanReverse ( &c, static_cast<std::uint32_t> ( x ) );
            return static_cast<std::uint32_t> ( std::numeric_limits<Type>::digits - 1 ) - c;
        }
        else { // GNU.
            return __builtin_clz ( static_cast<std::uint32_t> ( x ) );
        }
    }
}

template<typename Rng, typename RangeType, typename ResultType>
ResultType br_bitmask ( Rng & rng, RangeType range ) NOEXCEPT {
    --range;
    RangeType mask = std::numeric_limits<RangeType>::max ( );
    mask >>= leading_zeros<RangeType> ( range | RangeType { 1 } );
    RangeType x;
    do {
        x = rng ( ) & mask;
    } while ( x > range );
    return ResultType ( x );
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
#else
#if GNU
template<> struct double_width_integer<std::uint64_t> { using type = __uint128_t; };
#endif
#endif

template<typename Type>
constexpr unsigned_result_type<Type> make_mask ( ) noexcept {
    return unsigned_result_type<Type> { 1 } << ( std::numeric_limits<unsigned_result_type<Type>>::digits - 1 );
}

template<typename Rng, typename RangeType, typename ResultType>
ResultType br_lemire_oneill ( Rng & rng, RangeType range ) NOEXCEPT {
    #if MSVC && MEMORY_MODEL_64
    if constexpr ( std::is_same<RangeType, std::uint64_t>::value ) {
        unsigned_result_type<ResultType> x = rng ( );
        if ( range >= make_mask<ResultType> ( ) ) {
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
        if ( range >= make_mask<ResultType> ( ) ) {
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
    #if MSVC && MEMORY_MODEL_64
    }
    #endif
}


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
        //if constexpr ( MEMORY_MODEL_32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
        //    --range;
       // }
    }

    [[ nodiscard ]] constexpr bool operator == ( const param_type & rhs ) const NOEXCEPT {
        return ( min == rhs.min ) && ( range == rhs.range );
    }

    [[ nodiscard ]] constexpr bool operator != ( const param_type & rhs ) const NOEXCEPT {
        return ! ( *this == rhs );
    }

    [[ nodiscard ]] constexpr result_type a ( ) const NOEXCEPT {
        return min;
    }

    [[ nodiscard ]] constexpr result_type b ( ) const NOEXCEPT {
        //if constexpr ( MEMORY_MODEL_32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
        //    return ( range + 1 ) ? range + min : std::numeric_limits<result_type>::max ( );
        //}
       // else {
            return range ? range + min - 1 : std::numeric_limits<result_type>::max ( );
        //}
    }

    private:

    result_type min;
    unsigned_result_type range;
};
}



template<typename IntType>
class uniform_int_distribution_fast : public detail::param_type<IntType, uniform_int_distribution_fast<IntType>> {

    static_assert ( detail::is_distribution_result_type<IntType>::value, "only 16-, 32- and 64-bit result_types supported." );

    public:

    using result_type = IntType;
    using param_type = detail::param_type<result_type, uniform_int_distribution_fast>;

    private:

    friend param_type;

    using pt = param_type;
    using unsigned_result_type = typename std::make_unsigned<result_type>::type;

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
        if ( 0 == pt::range ) { // exploits the ub (ub is cool), to deal with interval [ std::numeric_limits<result_type>::max ( ), std::numeric_limits<result_type>::max ( ) ].
            return static_cast<result_type> ( rng_ref ( ) ) + pt::min;
        }
        if constexpr ( CLANG || ( MSVC && MEMORY_MODEL_32 ) ) {
            return detail::br_lemire_oneill<generator_reference<Gen>, unsigned_result_type, result_type> ( rng_ref, pt::range ) + pt::min;
        }
        else {
            return detail::br_bitmask<generator_reference<Gen>, unsigned_result_type, result_type> ( rng_ref, pt::range ) + pt::min;
        }
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
#undef CLANG
#undef GCC
#undef MEMORY_MODEL_64
#undef MEMORY_MODEL_32
#undef NOEXCEPT
