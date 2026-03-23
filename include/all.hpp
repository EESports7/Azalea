#pragma once

// BSD Zero Clause License
// 
// Copyright (c) 2026 EESports
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef ALL_STD_HEADERS
#define ALL_STD_HEADERS

// C++ Standard Library

#if __cplusplus < 202603L
    #include <strstream>
#endif
#include <algorithm>
#include <bitset>
#include <complex>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <typeinfo>
#include <utility>
#include <valarray>
#include <vector>

#if __cplusplus >= 201103L
    #if __cplusplus < 202603L
        #include <codecvt>
    #endif
    #include <array>
    #include <atomic>
    #include <chrono>
    #include <condition_variable>
    #include <forward_list>
    #include <future>
    #include <initializer_list>
    #include <mutex>
    #include <random>
    #include <ratio>
    #include <regex>
    #include <scoped_allocator>
    #include <system_error>
    #include <thread>
    #include <tuple>
    #include <type_traits>
    #include <typeindex>
    #include <unordered_map>
    #include <unordered_set>
#endif

#if __cplusplus >= 201402L
    #include <shared_mutex>
#endif

#if __cplusplus >= 201703L
    #include <any>
    #include <charconv>
    #include <execution>
    #include <filesystem>
    #include <memory_resource>
    #include <optional>
    #include <string_view>
    #include <variant>
#endif

#if __cplusplus >= 202002L
    #include <barrier>
    #include <bit>
    #include <compare>
    #include <concepts>
    #include <coroutine>
    #include <format>
    #include <latch>
    #include <numbers>
    #include <ranges>
    #include <semaphore>
    #include <source_location>
    #include <span>
    #include <stop_token>
    #include <syncstream>
    #include <version>
#endif

#if __cplusplus >= 202302L
    #include <expected>
    // #include <flat_map>
    // #include <flat_set>
    #include <generator>
    #include <mdspan>
    #include <print>
    #include <spanstream>
    #include <stacktrace>
    #include <stdfloat>
#endif

#if __cplusplus >= 202603L
    #include <contracts>
    #include <debugging>
    #include <hazard_pointer>
    #include <hive>
    #include <inplace_vector>
    #include <linalg>
    #include <rcu>
    #include <simd>
    #include <text_encoding>
#endif

// C++ Headers for the C Standard Library

#if __cplusplus < 202002L
    #include <ciso646>
#endif
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <cwctype>

#if __cplusplus >= 201103L
    #if __cplusplus < 202002L
        #include <ccomplex>
        #include <cstdalign>
        #include <cstdbool>
        #include <ctgmath>
    #endif
    #include <cfenv>
    #include <cinttypes>
    #include <cstdint>
    #include <cuchar>
#endif

// C Standard Library (exluding those above)

#if __cplusplus >= 202302L
    #include <stdatomic.h>
#endif

#if __cplusplus >= 202603L
    #include <stdbit.h>
    #include <stdchkint.h>
#endif

#endif // Include Guard