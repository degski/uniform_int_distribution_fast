
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

#include <ciso646>

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
#include "lehmer.hpp"
#include "uniform_int_distribution_fast.hpp"

template<typename IntType>
using is_generator_result_type =
std::disjunction <
    std::is_same<typename std::make_unsigned<IntType>::type, std::uint16_t>,
    std::is_same<typename std::make_unsigned<IntType>::type, std::uint32_t>,
    std::is_same<typename std::make_unsigned<IntType>::type, std::uint64_t>
>;

int main ( ) {

    splitmix64 rng ( [ ] ( ) { std::random_device rdev; return ( static_cast<std::uint64_t> ( rdev ( ) ) << 32 ) | rdev ( ); } ( ) );
    ext::uniform_int_distribution_fast<std::int64_t> dis;

    for ( int k = 0; k < 1000; k++ ) {
        std::cout << dis ( rng ) << std::endl;
    }

    std::cout << dis.a ( ) << " " << dis.b ( ) << std::endl;

    return EXIT_SUCCESS;
}
