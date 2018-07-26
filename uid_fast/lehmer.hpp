#ifndef LEHMER_HPP_INCLUDED
#define LEHMER_HPP_INCLUDED 1

/*
 * A C++ implementation of fast, 128-bit, Lehmer-style PRNGs
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Melissa E. O'Neill
 * Copyright (c) 2018 degski
 *
 * degski: eliminated passing the multiplier as a template parameter to
 *         work around an ICE in Clang/LLVM front-end.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cstdint>

namespace lehmer_detail {

template <typename rtype, typename stype>
class mcg128 {
    stype state_;
    static constexpr __uint128_t MCG_MULT = ( __uint128_t { 5017888479014934897ULL } << 64 ) + 2747143273072462557ULL; // passing as a template parameter crashes clang frontend.

    static constexpr unsigned int STYPE_BITS = 8 * sizeof ( stype );
    static constexpr unsigned int RTYPE_BITS = 8 * sizeof ( rtype );


    public:
    using result_type = rtype;
    static constexpr result_type min ( ) { return result_type ( 0 ); }
    static constexpr result_type max ( ) { return ~result_type ( 0 ); }

    mcg128 ( stype state = stype ( 0x9f57c403d06c42fcUL ) )
        : state_ ( state | 1 ) {
        // Nothing (else) to do.
    }

    void advance ( ) {
        state_ *= MCG_MULT;
    }

    result_type operator()( ) {
        advance ( );
        return result_type ( state_ >> ( STYPE_BITS - RTYPE_BITS ) );
    }

    bool operator==( const mcg128& rhs ) {
        return ( state_ == rhs.state_ );
    }

    bool operator!=( const mcg128& rhs ) {
        return !operator==( rhs );
    }

    // Not (yet) implemented:
    //   - arbitrary jumpahead (see PCG code for an implementation)
    //   - I/O
    //   - Seeding from a seed_seq.
};


template <typename rtype, typename stype>
class mcg128_fast {
    stype state_;
    static constexpr uint64_t MCG_MULT = 0xda942042e4dd58b5ULL;

    static constexpr unsigned int STYPE_BITS = 8 * sizeof ( stype );
    static constexpr unsigned int RTYPE_BITS = 8 * sizeof ( rtype );


    public:
    using result_type = rtype;
    static constexpr result_type min ( ) { return result_type ( 0 ); }
    static constexpr result_type max ( ) { return ~result_type ( 0 ); }

    mcg128_fast ( stype state = stype ( 0x9f57c403d06c42fcUL ) )
        : state_ ( state | 1 ) {
        // Nothing (else) to do.
    }

    void advance ( ) {
        state_ *= MCG_MULT;
    }

    result_type operator()( ) {
        advance ( );
        return result_type ( state_ >> ( STYPE_BITS - RTYPE_BITS ) );
    }

    bool operator==( const mcg128_fast& rhs ) {
        return ( state_ == rhs.state_ );
    }

    bool operator!=( const mcg128_fast& rhs ) {
        return !operator==( rhs );
    }

    // Not (yet) implemented:
    //   - arbitrary jumpahead (see PCG code for an implementation)
    //   - I/O
    //   - Seeding from a seed_seq.
};
}

using mcg128 = lehmer_detail::mcg128<uint64_t, __uint128_t>;
using mcg128_fast = lehmer_detail::mcg128_fast<uint64_t, __uint128_t>;

#endif // LEHMER_HPP_INCLUDED
