
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if UINTPTR_MAX == 0xFFFF'FFFF
    #define M32 1
    #define M64 0
#elif UINTPTR_MAX == 0xFFFF'FFFF'FFFF'FFFF
    #define M32 0
    #define M64 1
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
    extern "C" unsigned char _BitScanReverse ( unsigned long *, unsigned long );
    extern "C" unsigned char _BitScanReverse64 ( unsigned long *, unsigned long long );
#endif

#if _HAS_EXCEPTIONS == 0
    #define NOEXCEPT
#else
    #define NOEXCEPT noexcept
#endif

#if HAVE_ABSEIL and M32
    #include <absl/numeric/int128.h>
#endif
#define USE_ABSEIL ( HAVE_ABSEIL and M32 )


namespace ext {

template<typename IntType = int>
class uniform_int_distribution_fast;

namespace detail {

template<typename RangeType>
constexpr bool br_lemire_oneill ( ) NOEXCEPT {
    return true; /* ( CLANG and M64 ) or
           ( CLANG and M32 and not ( std::is_same<RangeType, std::uint64_t>::value ) ) or
           ( MSVC and M32 and not ( std::is_same<RangeType, std::uint64_t>::value ) ); */
}

template<typename RangeType>
constexpr bool br_bitmask ( ) NOEXCEPT {
    return not ( br_lemire_oneill<RangeType> ( ) );
}

struct uint32uint32_t {
    std::uint32_t low, high;
};

template<typename Type>
std::uint32_t leading_zeros ( Type x ) NOEXCEPT {
    if constexpr ( std::is_same<Type, std::uint64_t>::value ) {
        if constexpr ( M32 ) {
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
        else { // M64.
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

template<typename IT> struct double_width_integer { };
template<> struct double_width_integer<std::uint8_t > { using type = std::uint16_t; };
template<> struct double_width_integer<std::uint16_t> { using type = std::uint32_t; };
template<> struct double_width_integer<std::uint32_t> { using type = std::uint64_t; };
#if USE_ABSEIL
template<> struct double_width_integer<std::uint64_t> { using type = absl::uint128; };
#endif
#if GNU and M64
template<> struct double_width_integer<std::uint64_t> { using type = __uint128_t; };
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

    using range_type = typename std::make_unsigned<result_type>::type;

    friend class ::ext::uniform_int_distribution_fast<result_type>;

    explicit param_type ( result_type min_, result_type max_ ) NOEXCEPT :
        min ( min_ ),
        range ( max_ - min_ + 1 ) { // wraps to 0 for unsigned max.
        if constexpr ( br_bitmask<range_type> ( ) ) {
            --range;
        }
    }

    [[ nodiscard ]] constexpr bool operator == ( const param_type & rhs ) const NOEXCEPT {
        return ( min == rhs.min ) and ( range == rhs.range );
    }

    [[ nodiscard ]] constexpr bool operator != ( const param_type & rhs ) const NOEXCEPT {
        return not ( *this == rhs );
    }

    [[ nodiscard ]] constexpr result_type a ( ) const NOEXCEPT {
        return min;
    }

    [[ nodiscard ]] constexpr result_type b ( ) const NOEXCEPT {
        if constexpr ( br_lemire_oneill<range_type> ( ) ) {
            return range ? range + min - 1 : std::numeric_limits<result_type>::max ( );
        }
        if constexpr ( br_bitmask<range_type> ( ) ) {
            return ( range + 1 ) ? range + min : std::numeric_limits<result_type>::max ( );
        }
    }

    private:

    result_type min;
    range_type range;
};
} // namespace detail


template<typename IntType>
class uniform_int_distribution_fast : public detail::param_type<IntType, uniform_int_distribution_fast<IntType>> {

    static_assert ( detail::is_distribution_result_type<IntType>::value, "only 16-, 32- and 64-bit result_types are allowed." );

    public:

    using result_type = IntType;
    using param_type = detail::param_type<result_type, uniform_int_distribution_fast>;

    private:

    friend param_type;

    using pt = param_type;
    using range_type = typename std::make_unsigned<result_type>::type;

    [[ nodiscard ]] constexpr range_type range_mask ( ) const NOEXCEPT {
        return range_type { 1 } << ( std::numeric_limits<range_type>::digits - 1 );
    }

    template<typename Gen>
    using generator_reference = detail::bits_engine<Gen, range_type, ( Gen::max ( ) < std::numeric_limits<range_type>::max ( ) )>;

    public:

    explicit uniform_int_distribution_fast ( ) NOEXCEPT :
        param_type ( std::numeric_limits<result_type>::min ( ), std::numeric_limits<result_type>::max ( ) ) { }
    explicit uniform_int_distribution_fast ( result_type a, result_type b = std::numeric_limits<result_type>::max ( ) ) NOEXCEPT :
        param_type ( a, b ) {
        assert ( b >= a );
    }
    explicit uniform_int_distribution_fast ( const param_type & params_ ) NOEXCEPT :
        param_type ( params_ ) {
    }

    void reset ( ) const NOEXCEPT {
    }

    template<typename Gen>
    [[ nodiscard ]] result_type operator ( ) ( Gen & rng ) const NOEXCEPT {
        generator_reference<Gen> rng_ref ( rng );
        if ( 0 == pt::range ) { // deal with interval [ std::numeric_limits<result_type>::min ( ), std::numeric_limits<result_type>::max ( ) ].
            return static_cast< result_type > ( rng_ref ( ) );
        }
        if constexpr ( detail::br_lemire_oneill<range_type> ( ) ) {
            return bounded_range_lemire ( rng_ref ) + pt::min;
        }
        if constexpr ( detail::br_bitmask<range_type> ( ) ) {
            return bounded_range_bitmask ( rng_ref ) + pt::min;
        }
    }

    [[ nodiscard ]] param_type param ( ) const NOEXCEPT {
        return *this;
    }

    void param ( const param_type & params ) NOEXCEPT {
        *this = params;
    }

    private:

    template<typename Rng>
    result_type bounded_range_bitmask ( Rng & rng ) const NOEXCEPT {
        range_type mask = std::numeric_limits<range_type>::max ( );
        mask >>= detail::leading_zeros<range_type> ( pt::range | range_type { 1 } );
        range_type x;
        do {
            x = rng ( ) & mask;
        } while ( x > pt::range );
        return result_type ( x );
    }

    template<typename Rng>
    result_type bounded_range_lemire ( Rng & rng ) const NOEXCEPT {
        #if MSVC and M64
        if constexpr ( std::is_same<range_type, std::uint64_t>::value ) {
            const range_type t = ( 0 - pt::range ) % pt::range;
            range_type x = rng ( );
            range_type h, l = _umul128 ( x, pt::range, &h );
            while ( l < t ) {
                x = rng ( );
                l = _umul128 ( x, pt::range, &h );
            };
            return result_type ( h );
        }
        else { // pt::range is of type std::uint32_t.
        #endif
            using double_width_unsigned_result_type = typename detail::double_width_integer<range_type>::type;
            const range_type t = ( 0 - pt::range ) % pt::range;
            range_type x = rng ( );
            double_width_unsigned_result_type m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( pt::range );
            range_type l = range_type ( m );
            while ( l < t ) {
                x = rng ( );
                m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( pt::range );
                l = range_type ( m );
            };
            return result_type ( m >> std::numeric_limits<range_type>::digits );
        #if MSVC and M64
        }
        #endif
    }

    template<typename Rng>
    result_type bounded_range_lemire_oneill ( Rng & rng ) const NOEXCEPT {
        #if MSVC and M64
        if constexpr ( std::is_same<range_type, std::uint64_t>::value ) {
            range_type x = rng ( );
            if ( pt::range >= range_mask ( ) ) {
                do {
                    x = rng ( );
                } while ( x >= pt::range );
                return result_type ( x );
            }
            range_type h, l = _umul128 ( x, pt::range, &h );
            if ( l < pt::range ) {
                range_type t = ( 0 - pt::range );
                t -= pt::range;
                if ( t >= pt::range ) {
                    t %= pt::range;
                }
                while ( l < t ) {
                    l = _umul128 ( rng ( ), pt::range, &h );
                }
            }
            return result_type ( h );
        }
        else {
        #endif
            using double_width_range_type = typename detail::double_width_integer<range_type>::type;
            range_type x = rng ( );
            if ( pt::range >= range_mask ( ) ) {
                do {
                    x = rng ( );
                } while ( x >= pt::range );
                return result_type ( x );
            }
            double_width_range_type m = double_width_range_type ( x ) * double_width_range_type ( pt::range );
            range_type l = range_type ( m );
            if ( l < pt::range ) {
                range_type t = ( 0 - pt::range );
                t -= pt::range;
                if ( t >= pt::range ) {
                    t %= pt::range;
                }
                while ( l < t ) {
                    x = rng ( );
                    m = double_width_range_type ( x ) * double_width_range_type ( pt::range );
                    l = range_type ( m );
                }
            }
            return result_type ( m >> std::numeric_limits<range_type>::digits );
        #if MSVC and M64
        }
        #endif
    }
};
} // namespace ext


#if MSVC
    #pragma warning ( pop )
#endif

// macro cleanup

#undef USE_ABSEIL
#undef GNU
#undef MSVC
#undef CLANG
#undef GCC
#undef M64
#undef M32
#undef NOEXCEPT
