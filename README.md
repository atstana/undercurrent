[![CI](https://github.com/atstana/undercurrent/actions/workflows/ci.yml/badge.svg)](https://github.com/atstana/undercurrent/actions/workflows/ci.yml)

# undercurrent

undercurrent is a proof-of-concept C++ library designed to eliminate inefficiencies stemming from certain range adaptors, while maintaining the existing ranges/algorithm interfaces.

## The problem and the solutions

There's a known issue with the C++ ranges library where applying certain range adaptors doesn't deliver the expected performance. The problem tends to occur when `filter` or `reverse` is mixed into an adaptor chain.
Barry Revzin has written [an excellent blog post][revzin] explaining why this happens, so please refer to that.

Various ideas have been tried to overcome this problem. The blog post above introduces some approaches, including the one using code injection via token sequences (Revzin's idea) and a library called [Flux][Flux].

There are also other approaches, such as [transrangers][transrangers] (An Efficient, Composable Design Pattern for Range Processing) and [daisychains][daisychains] (Demo library for output-range-style composition).

[revzin]: https://brevzin.github.io/c++/2025/04/03/token-sequence-for/
[flux]: https://github.com/tcbrindle/flux
[transrangers]: https://github.com/joaquintides/transrangers
[daisychains]: https://github.com/dhollman/daisychains

## What undercurrent brings

Like other libraries, undercurrent brings performance improvements, but it is also designed to maintain the existing ranges/algorithm interfaces, so that changes to user code are minimal, or even zero, if the standard library itself adopts undercurrent's design.

## The idea

undercurrent exposes a single customizable function.

- **`uc::advance_while(first, last, pred, dir)`** 

This function iterates over the range `[first, last)`, starting from either `first` or `last`. It returns the first iterator `pos` for which `pred(*pos)` is false, together with a boolean value indicating whether every evaluation of `pred` was true. If `pred` holds for the entire range, the completion flag is `true` and `pos` is the far end of the traversal (`last` for forward, `first` for backward). The `dir` parameter controls which end the iteration starts from.

It is a customization point object (CPO), and authors of iterators or sentinels who wish to extract more performance are expected to customize it using `tag_invoke`.
The definition of `uc::advance_while` is as follows:

```cpp
export
template<typename I>
struct advance_while_result{
    I pos;
    bool completion_flag;
};

struct advance_while_fn{
    ...
    template<...>
    friend constexpr advance_while_result<I>
    tag_invoke(advance_while_fn, I first, S last, P pred, Dir dir) {
        if constexpr (std::is_same_v<P, skip_t>) {
            // "skip": no predicate to evaluate. Advance to the far end.
            if constexpr (dir) {
                return {
                    std::ranges::next(std::move(first), std::move(last)),
                    true
                };
            }
            else {
                return { std::move(first), true };
            }
        }
        else if constexpr (segmented_iterator<I> && std::is_same_v<I, S>) {
            // Specialized loop structure for segmented iterators.
            // Body omitted for brevity; see source/iterator/iteration.ixx.
            ...
        }
        else {
            // The default loop structure.
            if constexpr (dir) {
                // forward iteration.
                while(first != last) {
                    if (!std::invoke(pred, *first)) {
                        return { std::move(first), false };
                    }
                    ++first;
                }
                return { std::move(first), true };
            }
            else {
                // backward iteration.
                while(first != last) {
                    --last;
                    if (!std::invoke(pred, *last)) {
                        return { std::move(last), false };
                    }
                }
                return { std::move(last), true };
            }
        }
    }
};
export
constexpr advance_while_fn advance_while{};
```

`skip_t` is a predicate type that always returns true, but carries an additional meaning.
Think of it as the caller sending the signal: "Compute the value of `pos` that satisfies `pos == last` (or `pos == first`) as quickly as possible."
`dir` is an instance of a tag type (`uc::forward_tag` or `uc::backward_tag`) that is convertible to a boolean at compile time, and it controls the direction.

Note: undercurrent now integrates `segmented_iterator` support.
For more information on `segmented_iterator`, please see [Segmented Iterators and Hierarchical Algorithms](https://lafstern.org/matt/segmented.pdf).

Some algorithms (`uc::find_if`, `uc::find_if_not`, `uc::for_each`, `uc::next`) internally use `uc::advance_while`.
For example, `uc::next` is defined as follows:

```cpp
struct next_fn{
    template<...>
    I operator()(I cur, S bound) const {
        return uc::advance_while(
            std::move(cur),
            std::move(bound),
            skip,
            forward_tag{}
        ).pos;
    }
};
export
constexpr next_fn next{};
```

`uc::next` is basically the same as `std::ranges::next`, except that it is aware of our CPO.

Now, let's look at this from a broader perspective. We've made several algorithms (`uc::next`, `uc::find_if`, `uc::for_each`, etc.) CPO-aware. Their signatures are basically the same as the corresponding `std::ranges` algorithms. This means that authors of range adaptors can simply use these algorithms instead of the standard ones and be ready to benefit from customized logic.

For example, `std::ranges::drop_while_view::begin` calls `std::ranges::find_if_not`, so hypothetically, if we just change that to `uc::find_if_not`, that's it. Or if `std::ranges::find_if_not` were aware of our CPO, we wouldn't need to change any code at all.

The rest of the work falls to the iterator/sentinel authors, who tweak `uc::advance_while` appropriately.
See the source code for details on how to customize the CPO. Or, if you have any questions, please feel free to ask. Here, I'll walk through the `reverse_iterator` case as an example.

```cpp
template<...>
class reverse_iterator {
  ...
  template<...>
  friend constexpr advance_while_result<reverse_iterator>
  tag_invoke(
      tag_t<uc::advance_while>,
      reverse_iterator first,
      reverse_iterator last,
      P pred,
      Dir dir
  ) {
    auto [pos, flag] = uc::advance_while(
        std::move(last).base(),
        std::move(first).base(),
        std::move(pred),
        !dir
    );
    return {
        reverse_iterator{ flag ? std::move(pos) : std::move(++pos) },
        flag
    };
  }
  ...
};
```

We simply swap `first` and `last`, apply `advance_while` to the base iterators, and reverse the direction with `!dir`. Shifting `pos` by one when `flag` is `false` is due to the unique semantics of `reverse_iterator`. It's just a small detail, so don't worry about it.

The key is to descends the iterator hierarchy using recursion. The iterators of views produced by adaptors often have a `base()` member, which hints at an underlying hierarchy.

For example, consider the following,

```cpp
std::vector<int> v{ ... };
auto r = v | transform(square) | filter(even);
assert((std::is_same_v<decltype(v.begin()), decltype(r.begin().base().base())>));
```

Here, `r.begin().base().base()` is `vector<int>::iterator` which is basically a pointer wrapper.

A standard algorithm drives the top-level iterators and does all its work there.
On the other hand, `uc::advance_while` descends the iterator hierarchy via recursion and does the work at the lowest level instead. A custom `uc::advance_while` captures whatever it needs from the current level such as the predicate, a transformation function, and so on, then composes a new predicate, and forwards it down to the level below. The chain of calls eventually reaches the default `uc::advance_while`, where the composed predicate is evaluated in a simple while-loop driven by the low-level iterators. Once the work is done, it ascends as the recursive calls return, reconstructing the iterator at each level. 
That's the common tactic of undercurrent.

## Scope

| Adaptors                                 | Algorithms                       |
| ---------------------------------------- | -------------------------------- |
| `reverse_view`, `take_while_view`,       | `find_if`, `find_if_not`,        |
| `transform_view`, `filter_view`,         | `for_each`, `next`               |
| `join_view`, `drop_while_view`           |                                  |

This set is intentionally minimal.
All `uc::` views satisfy the `std::ranges::view` requirements and can be mixed with `std::ranges::views::*`.
However, the recursion stops at any `std::ranges::views::*` boundary in the chain, unless the standard library itself adopts undercurrent's design.
`std::ranges::subrange` benefits naturally if its iterator and sentinel come from `uc::` views.

Example:
```cpp
std::vector<int> v{ ... };
auto r = v | uc::transform(square) | uc::filter(even);
std::ranges::subrange sub{ r.begin(), r.end() };
uc::for_each(sub.begin(), sub.end(), do_something);
```
In short, the type of the range itself doesn't matter much; what matters is the type of the iterator and sentinel.

## Measurements

We will compare the time taken to sum a sequence of 2000 random unique integers (values spanning [0, 2000)) after applying `take_while`, `transform`, `filter`, and `reverse` in order, using `std::ranges` versus undercurrent.
This corresponds to the code below. Replacing `sr::` and `sv::` with `uc::` turns it into the undercurrent version.

```cpp
namespace sr = std::ranges;
namespace sv = std::ranges::views;
auto lt2000 = [](int i) { return i < 2000; };
auto square = [](int i) { return i * i; };
auto even = [](int i) { return i % 2 == 0; };
std::vector<int> v{ /*bunch of random integer values*/ };

auto r = v
    | sv::take_while(lt2000)
    | sv::transform(square)
    | sv::filter(even)
    | sv::reverse;

// r.begin();   // for `warm`: call once before starting the measurement
int sum{};
sr::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
```
The difference between `cold` and `warm` is whether measurement starts before or after calling `r.begin()`.
Numbers are medians of 20 runs in microseconds.

CPU: Ryzen 5 9600X

### Clang 22 + libc++ (Ubuntu 24.04 (on wsl2), `-O3`)

| Variant       | Time (μs) | Notes                                                 |
| ------------- | --------: | ----------------------------------------------------- |
| `raw loop`    |      0.74 | Hand-written loop, baseline                           |
| `std cold`    |      12.0 | `std::ranges::views::*`, fresh `begin()`              |
| `uc cold`     |      0.74 | `uc::*` + `uc::for_each`, fresh `begin()`             |
| `std warm`    |       6.2 | `std::ranges::views::*`, `begin()` already cached     |
| `uc warm`     |      0.32 | `uc::*` + `uc::for_each`, `begin()` already cached    |

On this configuration, `uc cold` is roughly **16x** faster than `std cold`.
There is no significant difference between `uc cold` and `raw loop`.

### MSVC (Windows11, `/O2`)

| Variant       | Time (μs) |
| ------------- | --------: |
| `raw loop`    |       1.3 |
| `std cold`    |      14.6 |
| `uc cold`     |       7.0 |
| `std warm`    |       7.0 |
| `uc warm`     |       6.5 |

MSVC's speedup is smaller (~2× over std). `raw loop` is 5x faster than `uc cold`.
This appears to be a code-generation gap, not a library limitation.
The numbers are reported for transparency.

See [benchmark/benchmark1.cpp](benchmark/benchmark1.cpp) for the harness.

This project includes a test suite that pins exact predicate call counts to verify the effect of our design. The code is roughly as follows:

```cpp
namespace sr = std::ranges;
namespace sv = std::ranges::views;
auto lt5 = [](int i) { return i < 5; };
auto square = [](int i) { return i * i; };
auto even = [](int i) { return i % 2 == 0; };
std::vector<int> v{ 1, 2, 3, 4, 5, 6 };

auto r = v
    | sv::take_while(lt5)
    | sv::transform(square)
    | sv::filter(even)
    | sv::reverse;

// r.begin();   // for `warm`: call once before starting the measurement
sr::for_each(r.begin(), r.end(), [&](int i) {});
```

| Variant       | `lt5` | `square` | `even` |
| --------------| ----: | -------: | -----: |
| `raw loop`    |     5 |        4 |      4 |
| `std cold`    |     8 |       12 |     10 |
| `uc cold`     |     6 |        5 |      5 |
| `std warm`    |     0 |        8 |      6 |
| `uc warm`     |     0 |        3 |      3 |


`uc cold` is slightly worse than `raw loop`. This is because `filter_view`'s `begin` finds the first matching element, returns the iterator, and then stops. This might be solvable by exposing an API like `approx_begin`, but since it wouldn't be safe, I haven't implemented it for now. In practice, the performance impact is likely negligible, as the benchmark above shows.

See [test/test1.cpp](test/test1.cpp) for the full counter setup.

## How to build

You need a compiler with C++20 module support and CMake (version 4.0 or newer).
I tested with Clang-22 and MSVC 19.50+.
Ninja is required if your compiler is Clang.
I couldn't build undercurrent with GCC-15 + Ninja.
Catch2 (tests) is fetched automatically via `FetchContent`. No system install needed.

Linux:
```bash
cmake --preset clang-release
cmake --build --preset clang-release

# Run tests
./build/clang-release/test/test1

# Run benchmark
./build/clang-release/benchmark/benchmark1
```

Windows with MSBuild:
```powershell
cmake --preset release
cmake --build --preset release

# Run tests
./build/release/test/release/test1

# Run benchmark
./build/release/benchmark/release/benchmark1
```

Available presets:

| Preset          | Toolchain                                  | Use case                       |
| --------------- | ------------------------------------------ | ------------------------------ |
| `debug`         | Host default (e.g. MSVC on Windows)        | Day-to-day work                |
| `release`       | Host default                               | Optimized build on host setup  |
| `clang-debug`   | Clang + Ninja + libc++ (Linux / macOS)     | Cross-check on Clang           |
| `clang-release` | Clang + Ninja + libc++ (Linux / macOS)     | Benchmarks, recommended        |

## How to use

Use CMake's `FetchContent`, then link it to your target with `target_link_libraries`. Import it with `import undercurrent;` in your code.

Example:

`CMakeLists.txt`:
```cmake
include(FetchContent)
FetchContent_Declare(
    undercurrent
    GIT_REPOSITORY https://github.com/atstana/undercurrent.git
    GIT_TAG main
)
FetchContent_MakeAvailable(undercurrent)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE undercurrent)
```

`main.cpp`:
```cpp
#include <vector>
import undercurrent;

int main() {
    std::vector<int> v{ 1, 2, 3, 4, 5, 6 };
    auto square = [](int i) { return i * i; };
    auto even   = [](int i) { return i % 2 == 0; };

    auto r = v
        | uc::transform(square)
        | uc::filter(even)
        | uc::reverse;

    int sum{};
    uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
    // sum == 56  (36 + 16 + 4)
}
```

## License

Distributed under the Boost Software License, Version 1.0. See [LICENSE](LICENSE)
