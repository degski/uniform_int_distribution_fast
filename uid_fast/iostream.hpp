
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

#include <iostream>

// Escape sequences...

inline std::ostream & nl ( std::ostream  & out_ ) { return out_ <<  '\n'; }
inline std::wostream & nl ( std::wostream & out_ ) { return out_ << L'\n'; }

inline std::ostream &  np ( std::ostream  & out_ ) { return out_ <<  '\f'; }
inline std::wostream & np ( std::wostream & out_ ) { return out_ << L'\f'; }

inline std::ostream &  cr ( std::ostream  & out_ ) { return out_ <<  '\r'; }
inline std::wostream & cr ( std::wostream & out_ ) { return out_ << L'\r'; }

inline std::ostream &  bs ( std::ostream  & out_ ) { return out_ <<  '\b'; }
inline std::wostream & bs ( std::wostream & out_ ) { return out_ << L'\b'; }

inline std::ostream &  sp ( std::ostream  & out_ ) { return out_ <<  ' '; }
inline std::wostream & sp ( std::wostream & out_ ) { return out_ << L' '; }

inline std::ostream &  ab ( std::ostream  & out_ ) { return out_ <<  '\a'; }
inline std::wostream & ab ( std::wostream & out_ ) { return out_ << L'\a'; }

inline std::ostream &  ht ( std::ostream  & out_ ) { return out_ <<  '\t'; }
inline std::wostream & ht ( std::wostream & out_ ) { return out_ << L'\t'; }

inline std::ostream &  vt ( std::ostream  & out_ ) { return out_ <<  '\v'; }
inline std::wostream & vt ( std::wostream & out_ ) { return out_ << L'\v'; }

// Spaced dash...

inline std::ostream &  sd ( std::ostream  & out_ ) { return out_ <<  " - "; }
inline std::wostream & sd ( std::wostream & out_ ) { return out_ << L" - "; }
