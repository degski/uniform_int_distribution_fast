
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
#include <ciso646>
#include <cstdint>
#include <cstdlib>

#include <functional>
#include <limits>
#include <random>
#include <type_traits>

#include <iostream>


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

#if UINTPTR_MAX == 0xFFFF'FFFF
#define MODEL32 1
#define MODEL64 0
#elif UINTPTR_MAX == 0xFFFF'FFFF'FFFF'FFFF
#define MODEL32 0
#define MODEL64 1
#else
#error funny pointers
#endif


namespace ext {

template<typename IntType = int>
class uniform_int_distribution_fast;

namespace detail {

struct model {
    enum : std::size_t { value = ( sizeof ( void* ) * 8 ) };
};

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
unsigned long leading_zeros ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return leading_zeros_intrin_32 ( *reinterpret_cast<large*> ( &x ) );
    }
    else {
        unsigned long c;
        _BitScanReverse64 ( &c, x );
        return 63u - c;
    }
}
#else
__attribute__ (( always_inline )) int leading_zeros_intrin_32 ( large x ) {
    if ( not ( x.high ) ) {
        return __builtin_clz ( x.low ) + 32;
    }
    return __builtin_clz ( x.high );
}
int leading_zeros ( std::uint64_t x ) noexcept {
    if constexpr ( model::value == 32 ) {
        return leading_zeros_intrin_32 ( *reinterpret_cast<large*> ( &x ) );
    }
    else {
        return __builtin_clzll ( x );
    }
}
#endif

template<typename IT> struct double_width_integer { };
template<> struct double_width_integer<std::uint16_t> { using type = std::uint32_t; };
template<> struct double_width_integer<std::uint32_t> { using type = std::uint64_t; };
#if MSVC

#else
// template<> struct double_width_integer<std::uint64_t> { using type = __uint128_t; };
#endif

template<typename IntType>
using is_distribution_result_type =
    std::disjunction <
        std::is_same<typename std::make_unsigned<IntType>::type, std::uint16_t>,
        std::is_same<typename std::make_unsigned<IntType>::type, std::uint32_t>,
        std::is_same<typename std::make_unsigned<IntType>::type, std::uint64_t>
    >;


template<typename Gen>
struct generator_reference : public std::reference_wrapper<Gen> {

    using std::reference_wrapper<Gen>::reference_wrapper;

    using result_type = typename Gen::result_type;

    [[ nodiscard ]] static constexpr result_type min ( ) noexcept { return Gen::min ( ); }
    [[ nodiscard ]] static constexpr result_type max ( ) noexcept { return Gen::max ( ); }
};

template<typename Gen, typename UnsignedResultType, bool HasBitEngine>
struct bits_engine {
};
template<typename Gen, typename UnsignedResultType>
struct bits_engine<Gen, UnsignedResultType, true> : public std::independent_bits_engine<generator_reference<Gen>, std::numeric_limits<UnsignedResultType>::digits, UnsignedResultType> {
    explicit bits_engine ( Gen & gen ) : std::independent_bits_engine<generator_reference<Gen>, std::numeric_limits<UnsignedResultType>::digits, UnsignedResultType> ( gen ) {
    }
};
template<typename Gen, typename UnsignedResultType>
struct bits_engine<Gen, UnsignedResultType, false> : public generator_reference<Gen> {
    explicit bits_engine ( Gen & gen ) : generator_reference<Gen> ( gen ) {
    }
};


template<typename IntType, typename Distribution>
struct param_type {

    using result_type = IntType;
    using distribution_type = Distribution;

    using unsigned_result_type = typename std::make_unsigned<result_type>::type;

    friend class ::ext::uniform_int_distribution_fast<result_type>;

    explicit param_type ( result_type min_, result_type max_ ) noexcept :
        min ( min_ ),
        range ( max_ - min_ + 1 ) { // wraps to 0 for unsigned max.
        if constexpr ( model::value == 32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
            --range;
        }
    }

    [[ nodiscard ]] constexpr bool operator == ( const param_type & rhs ) const noexcept {
        return min == rhs.min and range == rhs.range;
    }

    [[ nodiscard ]] constexpr bool operator != ( const param_type & rhs ) const noexcept {
        return not ( *this == rhs );
    }

    [[ nodiscard ]] constexpr result_type a ( ) const noexcept {
        return min;
    }

    [[ nodiscard ]] constexpr result_type b ( ) const noexcept {
        if constexpr ( model::value == 32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
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

using namespace detail;

template<typename IntType>
class uniform_int_distribution_fast : public param_type<IntType, uniform_int_distribution_fast<IntType>> {

    static_assert ( is_distribution_result_type<IntType>::value, "char (8-bit) not supported." );

    public:

    using result_type = IntType;
    using param_type = param_type<result_type, uniform_int_distribution_fast>;

    private:

    friend param_type;

    using pt = param_type;
    using unsigned_result_type = typename std::make_unsigned<result_type>::type;
    //#if MSVC
    using double_width_unsigned_result_type = unsigned_result_type; // dummy.
    //#else
    //using double_width_unsigned_result_type = typename double_width_integer<unsigned_result_type>::type;
    //#endif
    [[ nodiscard ]] constexpr unsigned_result_type range_max ( ) const noexcept {
        return unsigned_result_type { 1 } << ( sizeof ( unsigned_result_type ) * 8 - 1 );
    }

    template<typename Gen>
    using generator_reference = bits_engine<Gen, unsigned_result_type, ( Gen::max ( ) < std::numeric_limits<unsigned_result_type>::max ( ) )>;

    public:

    explicit uniform_int_distribution_fast ( ) noexcept :
        param_type ( std::numeric_limits<result_type>::min ( ), std::numeric_limits<result_type>::max ( ) ) {
    }
    explicit uniform_int_distribution_fast ( result_type a, result_type b = std::numeric_limits<result_type>::max ( ) ) noexcept :
        param_type ( a, b ) {
        assert ( b > a );
    }
    explicit uniform_int_distribution_fast ( const param_type & params_ ) noexcept :
        param_type ( params_ ) {
    }

    void reset ( ) const noexcept { }

    template<typename Gen>
    [[ nodiscard ]] result_type operator ( ) ( Gen & rng ) const noexcept {

        // ALL THIS IS GONNA GO AND BE REPLACED BY THE CODE IN generate ( rng ).

        static generator_reference<Gen> rng_ref ( rng ); // duplicating the code in an if constexpr will avoid the reference in most cases. is it worth the code duplication?
        if ( 0 == pt::range ) { // exploits the ub (ub is cool), to deal with interval [ std::numeric_limits<result_type>::max ( ), std::numeric_limits<result_type>::max ( ) ].
            return result_type ( rng_ref ( ) );
        }
        unsigned_result_type x = rng_ref ( );
        if ( pt::range >= range_max ( ) ) {
            do {
                x = rng ( );
            } while ( x >= pt::range );
            return result_type ( x ) + pt::min;
        }
        #if MSVC64
        unsigned_result_type h, l = _umul128 ( x, pt::range, &h );
        #else
        double_width_unsigned_result_type m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( pt::range );
        unsigned_result_type l = unsigned_result_type ( m );
        #endif
        if ( l < pt::range ) {
            #if MSVC // suppress error C4146 (Daniel, your favorite error!).
            unsigned_result_type t = 0 - pt::range;
            #else
            unsigned_result_type t =   - pt::range;
            #endif
            t -= pt::range;
            if ( t >= pt::range ) {
                t %= pt::range;
            }
            while ( l < t ) {
                #if MSVC64
                l = _umul128 ( rng_ref ( ), pt::range, &h );
                #else
                x = rng_ref ( );
                m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( pt::range );
                l = unsigned_result_type ( m );
                #endif
            }
        }
        #if MSVC64
        return h + pt::min;
        #else
        return result_type ( m >> std::numeric_limits<unsigned_result_type>::digits ) + pt::min;
        #endif
    }

    template<typename Gen>
    [ [ nodiscard ] ] result_type generate ( Gen & rng ) const noexcept {
        static generator_reference<Gen> rng_ref ( rng );
        if constexpr ( model::value == 32 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
            // provide bitmask version for the case: 32-bit platform, std::uint64_t range.
            // This seems to be the fastest function on this platform  and a 64-bit range.
            // leading_zeros  ( range )  uses  intrinsics available  to the compiler, i.e.
            //  clang  and  gcc  will  use  __builtin_clz ( ),    while   MSVC   will  use
            // _BitScanReverse.  Overall MSVC seems  seriously slow  on x86 as compared to
            // clang/llvm, see  the  bottom  of the benchmark  project in  this  solution.
            const std::uint64_t mask = std::numeric_limits<unsigned_result_type>::max ( ) >> leading_zeros ( pt::range | unsigned_result_type { 1 } );
            unsigned_result_type x = rng_ref ( ) & mask;
            while ( x > pt::range ) {
                x = rng_ref ( ) & mask;
            }
            return result_type ( x ) + pt::min;
        }
        else if constexpr ( MSVC64 and std::numeric_limits<unsigned_result_type>::digits == 64 ) {
            if ( 0 == pt::range ) {
                return result_type ( rng_ref ( ) );
            }
            unsigned_result_type x = rng_ref ( );
            if ( pt::range >= range_max ( ) ) {
                do {
                    x = rng ( );
                } while ( x >= pt::range );
                return result_type ( x ) + pt::min;
            }
            unsigned_result_type h, l = _umul128 ( x, pt::range, &h );
            if ( l < pt::range ) {
                unsigned_result_type t = 0 - pt::range;
                t -= pt::range;
                if ( t >= pt::range ) {
                    t %= pt::range;
                }
                while ( l < t ) {
                    l = _umul128 ( rng_ref ( ), pt::range, &h );
                }
            }
            return h + pt::min;
        }
        else {
            if ( 0 == pt::range ) {
                return result_type ( rng_ref ( ) );
            }
            unsigned_result_type x = rng_ref ( );
            if ( pt::range >= range_max ( ) ) {
                do {
                    x = rng ( );
                } while ( x >= pt::range );
                return result_type ( x ) + pt::min;
            }
            double_width_unsigned_result_type m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( pt::range );
            unsigned_result_type l = unsigned_result_type ( m );
            if ( l < pt::range ) {
                unsigned_result_type t = -pt::range;
                t -= pt::range;
                if ( t >= pt::range ) {
                    t %= pt::range;
                }
                while ( l < t ) {
                    x = rng_ref ( );
                    m = double_width_unsigned_result_type ( x ) * double_width_unsigned_result_type ( pt::range );
                    l = unsigned_result_type ( m );
                }
            }
            return result_type ( m >> std::numeric_limits<unsigned_result_type>::digits ) + pt::min;
        }
    }

    [[ nodiscard ]] param_type param ( ) const noexcept {
        return *this;
    }

    void param ( const param_type & params ) noexcept {
        *this = params;
    }
};
}

#pragma warning ( pop )
#undef MODEL64
#undef MODEL32
#undef MSVC64
#undef MSVC32
#undef MSVC
#undef GNU
