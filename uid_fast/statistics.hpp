
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

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstddef>

#include <limits>
#include <tuple>


namespace sf {

// Wellford's method: https://www.johndcook.com/blog/standard_deviation/
// returns min, max, mean, variance, sample sd and population sd.
template<typename T>
std::tuple<long double, long double, long double, long double, long double, long double> stats ( T * data, std::size_t n ) noexcept {
    long double min = ( long double ) std::numeric_limits<T>::max ( ), max = ( long double ) std::numeric_limits<T>::min ( );
    long double avg = 0.0, var = 0.0;
    for ( std::size_t i = 0; i < n; i++ ) {
        const long double d = ( long double ) data [ i ];
        if ( d < min ) min = d;
        if ( d > max ) max = d;
        const long double t = d - avg;
        avg += t / ( i + 1 );
        var += t * ( d - avg );
    }
    return { min, max, avg, var, std::sqrt ( var / ( n - 1 ) ), std::sqrt ( var / n ) };
}
}
