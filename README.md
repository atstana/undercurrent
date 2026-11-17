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

For example, `std::ranges::drop_while_view::begin` calls `std::ranges::find_if_not`, so hypothetically, if we just change that to `uc::find_if_not`, that's it (subject to the trade-off discussed in the *One drawback* section below). Or if `std::ranges::find_if_not` were aware of our CPO, we wouldn't need to change any code at all.

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

We simply swap `first` and `last`, apply `advance_while` to the base iterators, and reverse the direction with `!dir`. We shift `pos` by one when `flag` is `false` since `reverse_iterator` keeps its base iterator one position to the right of the element it denotes.

The key is to descend the iterator hierarchy using recursion. The iterators of views produced by adaptors often have a `base()` member, which hints at an underlying hierarchy.

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

## One drawback: The `reverse_iterator` adjustment penalty

As mentioned in the Idea section, `reverse_iterator` needs to adjust the underlying iterator returned by `advance_while`. It turns out that this off-by-one adjustment can introduce a performance penalty under specific conditions.

Consider the following scenario:

```cpp
auto gt0 = [](int i){ return 0 < i; };
auto eq1 = [](int i){ return i == 1; };
std::vector<int> v{ 1, 0, 0, 0, 0 };
auto r = v | uc::filter(gt0) | uc::reverse;
auto pos = uc::find_if(r.begin(), r.end(), eq1);
assert(*pos == 1);
assert(pos.base().base() == v.end());
```

The algorithm traverses `v` from the back to find the position satisfying `eq1` (or more precisely, a position satisfying both `gt0` and `eq1`). Let's call this position `v0`. However, due to the adjustment required for `reverse_iterator`, the algorithm must also find the position to the right of `v0` that satisfies `gt0`. Since no such element exists to the right of `v0`, the search ends up going all the way back to `v.end()`. Consequently, this algorithm results in traversing `v` back and forth.

This example represents a worst-case scenario (yielding roughly a 2x penalty in terms of the filter predicate call count). It occurs when the matching element is the *last* surviving element in the underlying range, followed by a long tail of filtered-out elements.
On the other hand, if the next surviving element is nearby, for instance, with `v = {2, 1, 0, 0, 0}` and searching for `2`, the adjustment steps exactly once, keeping the overhead minimal.
In either case, the result remains correct and the time complexity stays `O(n)`; only the constant factor is affected.

This issue arises only when the adaptor chain takes a form like `filter | ... | reverse` and is used with algorithms requiring early exit, such as `find`. It does not occur with algorithms like `for_each` or `next`.
While we could mitigate this by avoiding `advance_while` within `find`-family algorithms, doing so would forfeit significant optimization opportunities for typical use cases.
Based on this analysis, we classify the algorithms in undercurrent as follows:

**Tier 1 — (`for_each`, `next`).**
These are the parts `std` could adopt with no such downside.

**Tier 2 — (`find_if`, `find_if_not`, and therefore `filter_view::begin` / `drop_while_view::begin`).**
These are faster with typical chains, but the problem above can occur when a `filter`/`join` (any adaptor whose increment isn't `O(1)`) sits *below* a `reverse` in the chain (e.g. `v | filter(pred) | reverse`).

We decided to continue using `advance_while` for `find`-family algorithms in undercurrent, but if you intend to adopt this design, you should carefully consider the trade-offs.

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
ctest --preset clang-release

# Run benchmark
./build/clang-release/benchmark/benchmark1
```

Windows with MSBuild:
```powershell
cmake --preset release
cmake --build --preset release

# Run tests
ctest --preset release

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
