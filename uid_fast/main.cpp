
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

#define _HAS_EXCEPTIONS 0

#include <cassert>
#include <cstddef>
#include <cstdint>

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

#include "splitmix.hpp"
#if defined ( __GNUC__ ) || defined ( __clang__ )
// #include "lehmer.hpp"
#endif

#include "uniform_int_distribution_fast.hpp"
#include "plf_nanotimer.h"


int main ( ) {

    splitmix32 rng ( [ ] ( ) { std::random_device rdev; return ( static_cast<std::uint64_t> ( rdev ( ) ) << 32 ) | static_cast<std::uint64_t> ( rdev ( ) ); } ( ) );

    std::uniform_int_distribution<std::uint32_t> sdis ( 0, 199 );
    ext::uniform_int_distribution_fast<std::uint32_t> fdis ( 0, 199 );

    {
        std::array<std::uint64_t, 200> freq { 0 };

        plf::nanotimer t;
        t.start ( );

        for ( std::size_t k = 0; k < 100'000'000; ++k ) {
            ++freq [ static_cast< std::size_t > ( sdis ( rng ) ) ];
        }

        double et = t.get_elapsed_ms ( );

        std::cout << sdis.a ( ) << " " << sdis.b ( ) << std::endl;

        for ( auto v : freq ) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        std::cout << ( std::uint64_t ) et << std::endl;
    }

    {
        std::array<std::uint64_t, 200> freq { 0 };

        plf::nanotimer t;
        t.start ( );

        for ( std::size_t k = 0; k < 100'000'000; ++k ) {
            ++freq [ static_cast< std::size_t > ( fdis ( rng ) ) ];
        }

        double et = t.get_elapsed_ms ( );

        std::cout << fdis.a ( ) << " " << fdis.b ( ) << std::endl;

        for ( auto v : freq ) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        std::cout << ( std::uint64_t ) et << std::endl;
    }

    {
        std::array<std::uint64_t, 200> freq { 0 };

        plf::nanotimer t;
        t.start ( );

        for ( std::size_t k = 0; k < 100'000'000; ++k ) {
            ++freq [ static_cast< std::size_t > ( sdis ( rng ) ) ];
        }

        double et = t.get_elapsed_ms ( );

        std::cout << sdis.a ( ) << " " << sdis.b ( ) << std::endl;

        for ( auto v : freq ) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        std::cout << ( std::uint64_t ) et << std::endl;
    }

    {
        std::array<std::uint64_t, 200> freq { 0 };

        plf::nanotimer t;
        t.start ( );

        for ( std::size_t k = 0; k < 100'000'000; ++k ) {
            ++freq [ static_cast< std::size_t > ( fdis ( rng ) ) ];
        }

        double et = t.get_elapsed_ms ( );

        std::cout << fdis.a ( ) << " " << fdis.b ( ) << std::endl;

        for ( auto v : freq ) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        std::cout << ( std::uint64_t ) et << std::endl;
    }

    return EXIT_SUCCESS;
}
